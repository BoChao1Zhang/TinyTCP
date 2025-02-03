//
// Created by 32101 on 25-2-1.
//
#include "ether.h"

#include "dbg.h"
#include "netif.h"
#include "tools.h"
#include "protocol.h"
#if DBG_DISP_ENABLED(DBG_ETHER)
static void display_ether_pkt(char * title, ether_pkt_t *pkt, int total_size) {
    ether_hdr_t *hdr = &pkt->hdr;
    plat_printf("------- %s --------\n", title);
    plat_printf("\t len : %d bytes \n",total_size);
    dbg_dump_hwaddr("\t dest: ",hdr->dest,ETHER_HWA_SIZE);
    dbg_dump_hwaddr("\t src: ",hdr->src,ETHER_HWA_SIZE);
    plat_printf("\t type: %x \t",x_ntohs(hdr->protocol));

    switch (x_htons(hdr->protocol)) {
        case NET_PROTOCOL_ARP:
            plat_printf("arp\n");
            break;
        case NET_PROTOCOL_IPV4:
            plat_printf("ipv4\n");
            break;
        default:
            plat_printf("unknown\n");
            break;
    }
    plat_printf("\n");
}
#else

#endif


static net_err_t ether_open(struct _netif_t *netif) {
    return NET_ERR_OK;
}

static void ether_close(struct _netif_t *netif) {

}
static net_err_t is_pkt_ok(ether_pkt_t *pkt,int total_size) {
    if (total_size > (sizeof(ether_hdr_t) + ETHER_MTU)) {
        dbg_warning(DBG_ETHER, "frame size to big : %d\n",total_size);
        return NET_ERR_SIZE;
    }

    //pcap 在处理包时会自动删除填充，所以接收的字节可能小于64字节
    if (total_size < sizeof(ether_hdr_t)) {
        dbg_warning(DBG_ETHER, "frame size to small: %d\n",total_size);
        return NET_ERR_SIZE;
    }
    return NET_ERR_OK;
}


static net_err_t ether_in(struct _netif_t *netif, pktbuf_t * buf) {
    dbg_info(DBG_ETHER,"ether in\n");
    pktbuf_set_cont(buf,sizeof(ether_hdr_t));
    ether_pkt_t *pkt =(ether_pkt_t *)pktbuf_data(buf);
    net_err_t err;

    if ((err = is_pkt_ok(pkt,buf->total_size) < 0)){
        dbg_warning(DBG_ETHER,"ether pkt error");
        return err;
    }

    display_ether_pkt("ether in",pkt,buf->total_size);
    pktbuf_free(buf);
    return NET_ERR_OK;
}

static net_err_t ether_out(struct _netif_t *netif, ipaddr_t * dest ,pktbuf_t * data) {
    return NET_ERR_OK;
}



net_err_t ether_init(void) {
    static const link_layer_t link_layer = {
        .type = NETIF_TYPE_ETHER,
        .open = ether_open,
        .close = ether_close,
        .in = ether_in,
        .out = ether_out,
    };
    dbg_info(DBG_ETHER, "init ether\n");

    net_err_t err = netif_register_layer(NETIF_TYPE_ETHER,&link_layer);
    if (err < 0) {
        dbg_info(DBG_ETHER, "register ether failed\n");
        return err;
    }

    dbg_info(DBG_ETHER, "init ether done\n");
    return NET_ERR_OK;
}
