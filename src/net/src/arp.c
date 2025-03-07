#include "arp.h"
#include "dbg.h"
#include "mblock.h"
#include "tools.h"
#include "protocol.h"
#include "timer.h"

#define to_scan_cnt(tmo)    (tmo / ARP_TIMER_TMO )

static net_timer_t cache_timer;
static arp_entry_t cache_tbl[ARP_CACHE_SIZE];
static mblock_t cache_block;
static nlist_t cache_list;
static const uint8_t empty_hwaddr[6] = {0,0,0,0,0};
#if DBG_DISP_ENABLED(DBG_ARP)

static void arp_entry_display(arp_entry_t *entry) {
    plat_printf("%d: ", (int) (entry - cache_tbl));
    dbg_dump_ip_buf("   ip: ", entry->ipaddr);
    dbg_dump_hwaddr("   hw: ", entry->hwaddr, ETHER_HWA_SIZE);
    plat_printf("tmo: %d,retry: %d,%s,buf: %d\n", entry->tmo, entry->retry,
                entry->state == NET_ARP_RESOLVED ? "resolved" : "pending", nlist_count(&entry->buf_list));
}

static void arp_tbl_display(void) {
    plat_printf("------- arp tbl start---------\n");
    arp_entry_t *entry = cache_tbl;
    for (int i = 0; i < ARP_CACHE_SIZE; i++,entry++) {
        if (entry->state != NET_ARP_WAITING && entry->state != NET_ARP_RESOLVED) {
            continue;
        }

        arp_entry_display(entry);
    }
    plat_printf("------- arp tbl end---------\n");
}

static void arp_pkt_display(arp_pkt_t *packet) {
    plat_printf("------------arp packet ----------\n");
    uint16_t opcode = x_ntohs(packet->opcode);
    plat_printf("    htype: %d\n",x_ntohs(packet->htype));
    plat_printf("    ptype: %04x\n",x_ntohs(packet->ptype));
    plat_printf("    hwlen: %d\n",packet->hwlen);
    plat_printf("    plen: %d\n", packet->plen);
    plat_printf("    type: %d ", opcode);
    switch (opcode) {
        case ARP_REQUEST:
            plat_printf("request");
            break;
        case ARP_REPLY:
            plat_printf("reply");
            break;
        default:
            plat_printf("unknown");
            break;
    }
    dbg_dump_ip_buf("\n   sender:", packet->sender_ipaddr);
    dbg_dump_hwaddr("  mac:", packet->sender_hwaddr, ETHER_HWA_SIZE);
    dbg_dump_ip_buf("  target:", packet->target_ipaddr);
    dbg_dump_hwaddr("  mac:", packet->target_hwaddr, ETHER_HWA_SIZE);
}
#else
#define arp_tbl_display()
#define arp_pkt_display(packet)
#define arp_entry_display(entry)
#endif

static net_err_t cache_init(void) {
    nlist_init(&cache_list);

    net_err_t err = mblock_init(&cache_block, cache_tbl, sizeof(arp_entry_t), ARP_CACHE_SIZE, NLOCKER_NONE);
    if (err < 0) {
        dbg_error(DBG_ARP, "arp cache mblock init failed\n");
        return err;
    }

    return NET_ERR_OK;
}

static void cache_clear_all(arp_entry_t* entry) {

    nlist_node_t* first;
    while (first = nlist_remove_first(&entry->buf_list)) {
        pktbuf_t *buf = nlist_entry(first,pktbuf_t,node);
        pktbuf_free(buf);
    }
}

static arp_entry_t * cache_alloc(int force) {
    arp_entry_t * entry= (arp_entry_t *)mblock_alloc(&cache_block,-1);
    if (!entry && force) {
        nlist_node_t *node = nlist_remove_last(&cache_list);
        if (!node) {
            dbg_warning(DBG_ARP, "alloc arp entry failed\n");
            return (arp_entry_t *)0;
        }

        entry = nlist_entry(node,arp_entry_t,node);
        cache_clear_all(entry);
    }

    if (entry) {
        plat_memset(entry,0,sizeof(arp_entry_t));
        entry->state = NET_ARP_FREE;
        nlist_node_init(&entry->node);
        nlist_init(&entry->buf_list);
    }

    return entry;
}

static void cache_free(arp_entry_t *entry) {
    cache_clear_all(entry);
    nlist_remove(&cache_list,&entry->node);
    mblock_free(&cache_block,entry);
}

static arp_entry_t * cache_find(uint8_t *ip) {
    nlist_node_t *node;
    nlist_foreach(node,&cache_list) {
        arp_entry_t* entry = nlist_entry(node,arp_entry_t,node);
        if (plat_memcmp(ip,entry->ipaddr,IPV4_ADDR_SIZE) == 0) {
            nlist_remove(&cache_list,&entry->node);
            nlist_insert_first(&cache_list,&entry->node);
            return entry;
        }
    }
    return (arp_entry_t *)0;
}

static void cache_entry_set(arp_entry_t* entry, uint8_t* ip,uint8_t* hwaddr, netif_t* netif,int state) {
    plat_memcpy(entry->ipaddr,ip,IPV4_ADDR_SIZE);
    plat_memcpy(entry->hwaddr,hwaddr,ETHER_HWA_SIZE);
    entry->state = state;
    entry->netif = netif;

    if (state == NET_ARP_RESOLVED) {
        entry->tmo = to_scan_cnt(ARP_ENTRY_STABLE_TMO);
    } else {
        entry->retry = to_scan_cnt(ARP_ENTRY_PENDING_TMO);
    }
}

static net_err_t cache_send_all(arp_entry_t *entry) {
    dbg_info(DBG_ARP, "send all packet\n");
    dbg_dump_ip_buf("ip:",entry->ipaddr);
    nlist_node_t* first;
    while (first = nlist_remove_first(&entry->buf_list)) {
        pktbuf_t *buf = nlist_entry(first,pktbuf_t,node);

        net_err_t err = ether_raw_out(entry->netif,NET_PROTOCOL_IPV4, entry->hwaddr, buf);
        if (err < 0) {
            pktbuf_free(buf);
        }
    }

    return  NET_ERR_OK;
}

static net_err_t cache_insert(netif_t *netif, uint8_t* ip,uint8_t* hwaddr, int force) {
    if (*(uint32_t *)ip == 0) {
        return NET_ERR_STATE;
    }

    arp_entry_t *entry = cache_find(ip);
    if (!entry) {
        entry = cache_alloc(force);
        if (!entry) {
            dbg_dump_ip_buf("alloc failed. ip:",ip);
            return NET_ERR_NONE;
        }

        cache_entry_set(entry,ip,hwaddr,netif,NET_ARP_RESOLVED);
        nlist_insert_first(&cache_list,&entry->node);
    } else {
        // dbg_dump_ip_buf("update arp entry ip:\n",ip);
        cache_entry_set(entry,ip,hwaddr,netif,NET_ARP_RESOLVED);
        if (nlist_first(&cache_list) != &entry->node) {
            nlist_remove(&cache_list,&entry->node);
            nlist_insert_first(&cache_list,&entry->node);
        }

        net_err_t err = cache_send_all(entry);
        if (err < 0) {
            dbg_error(DBG_ARP, "send arp failed\n");
            return err;
        }
    }

    arp_tbl_display();
    return NET_ERR_OK;
}

static void arp_cache_tmo(net_timer_t * timer, void *arg) {
    int changed_cnt = 0;


    nlist_node_t * curr, *next;

    for (curr = cache_list.first; curr; curr = next) {
        next = nlist_node_next(curr);

        arp_entry_t *entry = nlist_entry(curr,arp_entry_t,node);

        if (--entry->tmo > 0) {
            continue;
        }
        changed_cnt ++;
        switch (entry->state) {
            case NET_ARP_RESOLVED: {
                dbg_info(DBG_ARP, "state to pending\n");
                arp_entry_display(entry);
                ipaddr_t ipaddr;
                ipaddr_from_buf(&ipaddr, entry->ipaddr);

                entry->state = NET_ARP_WAITING;
                entry->tmo = to_scan_cnt(ARP_ENTRY_PENDING_TMO);
                entry->retry = ARP_ENTRY_RETRY_CNT;
                arp_make_request(entry->netif,&ipaddr);

                break;
            }
            case NET_ARP_WAITING: {
                if (--entry->retry == 0) {
                    dbg_info(DBG_ARP,"pending tmo, free it\n");
                    arp_entry_display(entry);
                    cache_free(entry);
                } else {
                    dbg_info(DBG_ARP, "pending tmo,retry:\n");
                    arp_entry_display(entry);
                    ipaddr_t ipaddr;
                    ipaddr_from_buf(&ipaddr, entry->ipaddr);

                    entry->state = NET_ARP_WAITING;
                    entry->tmo = to_scan_cnt(ARP_ENTRY_PENDING_TMO);
                    arp_make_request(entry->netif,&ipaddr);
                }
                break;
            }
            default: {
                dbg_error(DBG_ARP, "unknown state\n");
                break;
            }
        }
    }

    if (changed_cnt) {
        dbg_info(DBG_ARP,"%d arp entry changed\n",changed_cnt);
        arp_tbl_display();
    }

}

net_err_t arp_init(void) {
    net_err_t err = cache_init();
    if (err < 0) {
        dbg_error(DBG_ARP, "arp cache init failed\n");
        return err;
    }

    err = net_timer_add(&cache_timer,"arp timer",arp_cache_tmo,(void *)0, ARP_TIMER_TMO * 1000, NET_TIMER_RELOAD);
    if (err < 0) {
        dbg_error(DBG_ARP, "arp timer init failed\n");
        return err;
    }
    return NET_ERR_OK;
}

net_err_t arp_make_request(netif_t *netif, ipaddr_t *dest) {
    pktbuf_t *buf = pktbuf_alloc(sizeof(arp_pkt_t));
    if (buf == (pktbuf_t *) 0) {
        dbg_error(DBG_ARP, "alloc pktbuf failed\n");
        return NET_ERR_NONE;
    }

    pktbuf_set_cont(buf, sizeof(arp_pkt_t));
    arp_pkt_t *arp_pkt = (arp_pkt_t *) pktbuf_data(buf);
    arp_pkt->htype = x_htons(ARP_HW_ETHER);
    arp_pkt->ptype = x_htons(NET_PROTOCOL_IPV4);
    arp_pkt->hwlen = ETHER_HWA_SIZE;
    arp_pkt->plen = IPV4_ADDR_SIZE;
    arp_pkt->opcode = x_htons(ARP_REQUEST);
    plat_memcpy(arp_pkt->sender_hwaddr, netif->hwaddr.addr,ETHER_HWA_SIZE);
    ipaddr_to_buf(&netif->ipaddr, arp_pkt->sender_ipaddr);
    plat_memset(arp_pkt->target_hwaddr, 0,ETHER_HWA_SIZE);
    ipaddr_to_buf(dest, arp_pkt->target_ipaddr);

    arp_pkt_display(arp_pkt);

    const net_err_t err = ether_raw_out(netif, NET_PROTOCOL_ARP, ether_broadcast_addr(), buf);
    if (err < 0) {
        pktbuf_free(buf);
    }

    return err;
}

net_err_t arp_make_gratuitous(netif_t *netif) {
    dbg_info(DBG_ARP, "send an gratuitous arp...\n");
    return arp_make_request(netif, &netif->ipaddr);
}

net_err_t arp_make_reply(netif_t *netif, pktbuf_t *buf) {
    arp_pkt_t *arp_pkt = (arp_pkt_t *) pktbuf_data(buf);
    arp_pkt->opcode = x_htons(ARP_REPLY);
    plat_memcpy(arp_pkt->target_hwaddr, arp_pkt->sender_hwaddr,ETHER_HWA_SIZE);
    plat_memcpy(arp_pkt->target_ipaddr, arp_pkt->sender_ipaddr,IPV4_ADDR_SIZE);
    plat_memcpy(arp_pkt->sender_hwaddr, netif->hwaddr.addr,ETHER_HWA_SIZE);
    ipaddr_to_buf(&netif->ipaddr, arp_pkt->sender_ipaddr);

    arp_pkt_display(arp_pkt);

    return ether_raw_out(netif, NET_PROTOCOL_ARP, arp_pkt->target_hwaddr, buf);
}

static net_err_t is_pkt_ok(arp_pkt_t *arp_packet, uint16_t size, netif_t *netif) {
    if (size < sizeof(arp_pkt_t)) {
        dbg_warning(DBG_ARP, "packet size error");
        return NET_ERR_SIZE;
    }

    if (x_ntohs(arp_packet->htype) != ARP_HW_ETHER
        || arp_packet->hwlen != ETHER_HWA_SIZE
        || x_ntohs(arp_packet->ptype) != NET_PROTOCOL_IPV4
        || arp_packet->plen != IPV4_ADDR_SIZE) {
        dbg_warning(DBG_ARP, "packet incorrect\n");
        return NET_ERR_NOT_SUPPORT;
    }

    uint16_t opcode = x_ntohs(arp_packet->opcode);
    if (opcode != ARP_REQUEST && opcode != ARP_REPLY) {
        dbg_warning(DBG_ARP, "arp opcode error");
        return NET_ERR_NOT_SUPPORT;
    }

    return NET_ERR_OK;
}

net_err_t arp_in(netif_t *netif, pktbuf_t *buf) {
    dbg_info(DBG_ARP, "arp in\n");
    net_err_t err = pktbuf_set_cont(buf, sizeof(arp_pkt_t));
    if (err < 0) {
        return err;
    }
    arp_pkt_t *arp_packet = (arp_pkt_t *) pktbuf_data(buf);
    if (is_pkt_ok(arp_packet, buf->total_size, netif) != NET_ERR_OK) {
        return err;
    }

    arp_pkt_display(arp_packet);

    ipaddr_t target_ip;
    ipaddr_from_buf(&target_ip,arp_packet->target_ipaddr);

    if (ipaddr_is_equal(&netif->ipaddr, &target_ip)) {
        dbg_info(DBG_ARP,"receive an arp for me\n");

        cache_insert(netif,arp_packet->sender_ipaddr,arp_packet->sender_hwaddr,1);
        if (x_ntohs(arp_packet->opcode) == ARP_REQUEST) {
            dbg_info(DBG_ARP, "arp request");
            return arp_make_reply(netif, buf);
        }
    } else {
        dbg_info(DBG_ARP, "receive an arp for other\n");
        cache_insert(netif,arp_packet->sender_ipaddr,arp_packet->sender_hwaddr,0);
    }

    pktbuf_free(buf);
    return NET_ERR_OK;
}

net_err_t arp_resolve(netif_t *netif, ipaddr_t *dest, pktbuf_t *buf) {
    arp_entry_t * entry = cache_find(dest->a_addr);
    if (entry) {
        dbg_info(DBG_ARP, "find arp entry\n");

        if (entry->state == NET_ARP_RESOLVED) {
            return ether_raw_out(netif, NET_PROTOCOL_IPV4, entry->hwaddr, buf);
        }

        if (nlist_count(&entry->buf_list) <= ARP_MAX_PKT_WAIT) {
            dbg_info(DBG_ARP, "insert buf to arp entry\n");
            nlist_insert_last(&entry->buf_list,&buf->node);
            return NET_ERR_OK;
        } else {
            dbg_warning(DBG_ARP, "too many waiting....\n");
        }
    } else {
        dbg_info(DBG_ARP,"make arp request\n");
        entry = cache_alloc(1);
        if (entry == (arp_entry_t *)0) {
            dbg_error(DBG_ARP, "alloc arp entry failed\n");
            return NET_ERR_NONE;
        }

        cache_entry_set(entry, dest->a_addr, empty_hwaddr,netif,NET_ARP_WAITING);
        nlist_insert_first(&cache_list, &entry->node);


        nlist_insert_last(&entry->buf_list,&buf->node);
        arp_tbl_display();

        return arp_make_request(netif,dest);
    }

    return NET_ERR_OK;
}

void arp_clear(netif_t *netif) {
    nlist_node_t *node,*next;
    for (node = nlist_first(&cache_list);node;node = next) {
        next = nlist_node_next(node);
        arp_entry_t* entry = nlist_entry(node, arp_entry_t, node);
        if (entry->netif == netif) {
            nlist_remove(&cache_list, node);
        }
    }
}

