//
// Created by bc on 25-3-6.
//

#include "ipv4.h"

#include "dbg.h"
#include "protocol.h"
#include "tools.h"

#if DBG_DISP_ENABLED(DBG_IP)

static uint16_t packet_id = 0;

static void display_ip_packet(ipv4_pkt_t* pkt) {
    ipv4_hdr_t* ip_hdr = (ipv4_hdr_t*)&pkt->hdr;

    plat_printf("--------------- ip ------------------ \n");
    plat_printf("    Version:%d\n", ip_hdr->version);
    plat_printf("    Header len:%d bytes\n", ipv4_hdr_size(pkt));
    plat_printf("    Totoal len: %d bytes\n", ip_hdr->total_len);
    plat_printf("    Id:%d\n", ip_hdr->id);
    plat_printf("    TTL: %d\n", ip_hdr->ttl);
    plat_printf("    Protocol: %d\n", ip_hdr->protocol);
    plat_printf("    Header checksum: 0x%04x\n", ip_hdr->hdr_checksum);
    dbg_dump_ip_buf(DBG_IP, "    src ip:", ip_hdr->dest_ip);
    dbg_dump_ip_buf(DBG_IP, "    dest ip:", ip_hdr->src_ip);
    plat_printf("--------------- ip end ------------------ \n");
}

#else
#define display_ip_packet(pkt)
#endif


net_err_t ipv4_init(void)
{
    dbg_info(DBG_IP,"init ip \n");


    dbg_info(DBG_IP,"init done \n");



    return NET_ERR_OK;
}

static net_err_t is_pkt_ok(ipv4_pkt_t *pkt, int size,netif_t *netif) {
    if (pkt->hdr.version != NET_VERSION_IPV4) {
        dbg_warning(DBG_IP,"invalid ip version\n");
        return NET_ERR_NOT_SUPPORT;
    }

    int hdr_len  = ipv4_hdr_size(pkt);
    if (hdr_len < sizeof(ipv4_hdr_t)) {
        dbg_warning(DBG_IP,"invalid hdr size\n");
        return NET_ERR_SIZE;
    }

    int total_size = x_ntohs(pkt->hdr.total_len);
    if ((total_size < sizeof(ipv4_hdr_t)) || (size < total_size)) {
        dbg_warning(DBG_IP,"invalid ipv4 packet size\n");
    }

    if (pkt->hdr.hdr_checksum) {
        uint16_t c = checksum16(pkt,hdr_len,0,1);
        if (c!=0) {
            dbg_warning(DBG_IP,"invalid checksum\n");
            return NET_ERR_BROKEN;
        }
    }

    return NET_ERR_OK;
}

static void iphdr_ntohs(ipv4_pkt_t *pkt) {
    pkt->hdr.total_len = x_ntohs(pkt->hdr.total_len);
    pkt->hdr.id = x_ntohs(pkt->hdr.id);
    pkt->hdr.frag_all = x_ntohs(pkt->hdr.frag_all);
}

static void iphdr_htons(ipv4_pkt_t *pkt) {
    pkt->hdr.total_len = x_htons(pkt->hdr.total_len);
    pkt->hdr.id = x_htons(pkt->hdr.id);
    pkt->hdr.frag_all = x_htons(pkt->hdr.frag_all);

}

static net_err_t ip_normal_in(netif_t *netif,pktbuf_t *buf,ipaddr_t *src_ip, ipaddr_t *dest_ip) {
    ipv4_pkt_t *pkt = (ipv4_pkt_t *)pktbuf_data(buf);
    display_ip_packet(pkt);
    switch (pkt->hdr.protocol) {
        case NET_PROTOCOL_ICMPv4: {
            break;
        }
        case NET_PROTOCOL_TCP:{
            break;
        }
        case NET_PROTOCOL_UDP: {
            break;
        }
        default: {
            dbg_warning(DBG_IP,"unknown protocol");
            break;
        }
    }
    return NET_ERR_UNREACH;
}

net_err_t ipv4_in(netif_t *netif, pktbuf_t *buf) {
    dbg_info(DBG_IP, "ip in\n");

    net_err_t err = pktbuf_set_cont(buf,sizeof(ipv4_hdr_t));
    if (err < 0) {
        dbg_error(DBG_IP, "adjust header,err - %d\n",err);
        return err;
    }

    ipv4_pkt_t *pkt = (ipv4_pkt_t *)pktbuf_data(buf);

    if (is_pkt_ok(pkt,buf->total_size,netif)!= NET_ERR_OK) {
        dbg_warning(DBG_INIT, "packet is broken.");
        return err;
    }

    iphdr_ntohs(pkt);

    err = pktbuf_resize(buf,pkt->hdr.total_len);

    ipaddr_t dest_ip,src_ip;
    ipaddr_from_buf(&dest_ip,pkt->hdr.dest_ip);
    ipaddr_from_buf(&src_ip,pkt->hdr.src_ip);


    if (err < 0) {
        dbg_error(DBG_IP, "resize packet buffer failed\n");
        return err;
    }

    //192.168.74.2 --- 192.168.7.255 --- 255.255.255.255
    if (!ipaddr_is_match(&dest_ip, &netif->ipaddr, &netif->netmask)) {
        dbg_error(DBG_IP, "ip not match");
        return NET_ERR_UNREACH;
    }

    err = ip_normal_in(netif,buf,&src_ip,&dest_ip);


    pktbuf_free(buf);
    return NET_ERR_OK;
}

net_err_t ipv4_out(uint8_t protocol, ipaddr_t *dest, ipaddr_t* src, pktbuf_t *buf) {
    dbg_info(DBG_IP, "ip out\n");

    net_err_t err = pktbuf_add_header(buf,sizeof(ipv4_hdr_t),1);
    if (err < 0) {
        dbg_error(DBG_IP, "add header,err %d\n",err);
        return NET_ERR_SIZE;
    }

    ipv4_pkt_t *pkt = (ipv4_pkt_t *)pktbuf_data(buf);
    pkt->hdr.shdr_all = 0;
    pkt->hdr.version = NET_VERSION_IPV4;
    ipv4_set_hdr_size(pkt,sizeof(ipv4_hdr_t));
    pkt->hdr.total_len = buf->total_size;
    pkt->hdr.id = packet_id++;
    pkt->hdr.frag_all = 0;
    pkt->hdr.ttl = NET_IP_DEFAULT_TTL;
    pkt->hdr.protocol = protocol;
    pkt->hdr.hdr_checksum = 0;
    ipaddr_to_buf(src,pkt->hdr.src_ip);
    ipaddr_to_buf(dest,pkt->hdr.dest_ip);
    iphdr_htons(pkt);
    //为什么要复位？ buf的读写指针已经被修改了
    pktbuf_reset_acc(buf);

    //为什么不做大小端的转换？ 不做大小端的转化也会通过
    pkt->hdr.hdr_checksum = pktbuf_checksum16(buf,ipv4_hdr_size(pkt),0,1);


    display_ip_packet(pkt);

    err = netif_out(netif_get_default(),dest,buf);
    if (err < 0) {
        dbg_warning(DBG_IP, "send ip failed");
        return err;
    }


    return NET_ERR_OK;




}
