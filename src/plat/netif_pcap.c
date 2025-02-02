//
// Created by 32101 on 25-1-16.
//

#include "net_err.h"
#include "netif_pcap.h"

#include "dbg.h"
#include "ether.h"
#include "exmsg.h"
#include "sys_plat.h"

void recv_thread(void * arg) {
    plat_printf("recv thread is running...\n");

    netif_t *netif = (netif_t *)arg;
    pcap_t *pcap = (pcap_t *)netif->ops_data;
    while (1) {
        struct pcap_pkthdr *pkthdr;
        const uint8_t * pkt_data;
        //数据包的元数据包含在pkthdr中，数据包的内容在pkt_data中
        if (pcap_next_ex(pcap,&pkthdr,&pkt_data) !=1) {
            continue;
        }

        pktbuf_t *buf = pktbuf_alloc(pkthdr->len);
        if (buf == (pktbuf_t *)0) {
            dbg_warning(DBG_NETIF,"buf == NULL");
            continue;
        }
        pktbuf_write(buf,pkt_data,pkthdr->len);

        int err = netif_put_in(netif,buf,0);

        if (err < 0) {
            dbg_warning(DBG_NETIF,"write buf failed\n");
            pktbuf_free(buf);
        }
    }
}

void xmit_thread(void * arg) {
    plat_printf("xmit thread is running...\n");
    // 1500 + 6(目的) + 6(源) + 2(类型) = 1514
    static uint8_t rw_buffer[1514];
    netif_t * netif = (netif_t*)arg;
    pcap_t * pcap = (pcap_t *)netif->ops_data;

    while (1) {
        pktbuf_t *buf = netif_get_out(netif,0);
        if (buf == (pktbuf_t *)0) {
            continue;
        }

        int total_size = buf->total_size;
        plat_memset(rw_buffer,0,sizeof(rw_buffer));
        pktbuf_read(buf,rw_buffer,total_size);
        pktbuf_free(buf);
        int err = pcap_inject(pcap,rw_buffer,total_size);
        if (err < 0) {
            fprintf(stderr, "pcap send: send packet failed!:%s\n", pcap_geterr(pcap));
            fprintf(stderr, "pcap send: pcaket size %d\n", total_size);
        }
    }
}

net_err_t netif_netdev_open(struct _netif_t *netif,void *data) {
    pcap_data_t* dev_data = (pcap_data_t*)data;

    pcap_t * pcap =  pcap_device_open(dev_data->ip,dev_data->hwaddr);
    if (pcap == (pcap_t * )0) {
        dbg_error(DBG_NETIF,"open pcap failed\n name=%s\n",netif->name);
        return NET_ERR_IO;
    }
    //收发数据包都需要pcap 这个结构体
    netif->ops_data = pcap;

    netif->mtu = ETHER_MTU;
    netif->type = NETIF_TYPE_ETHER;
    netif_set_hwaddr(netif,dev_data->hwaddr,6);

    sys_thread_create(recv_thread,netif);
    sys_thread_create(xmit_thread,netif);
    return NET_ERR_OK;
}

void netif_netdev_close(struct  _netif_t* netif) {
    pcap_t *pcap= (pcap_t *)netif->ops_data;
    pcap_close(pcap);

}

net_err_t netif_netdev_xmit(struct _netif_t *netif) {
    return NET_ERR_OK;
}

const netif_ops_t netdev_ops = {
    .open =netif_netdev_open,
    .close =netif_netdev_close,
    .xmit = netif_netdev_xmit,
};
