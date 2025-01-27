//
// Created by 32101 on 25-1-16.
//

#include "net_err.h"
#include "netif_pcap.h"

#include "dbg.h"
#include "exmsg.h"
#include "sys_plat.h"

void recv_thread(void * arg) {
    plat_printf("recv thread is running...\n");

    while (1) {
        sys_sleep(10);

        exmsg_netif_in((netif_t *)0);
    }
}

void xmit_thread(void * arg) {
    plat_printf("xmit thread is running...\n");

    while (1) {
        sys_sleep(10);
    }
}

net_err_t netif_netdev_open(struct _netif_t *netif,void *data) {
    pcap_data_t* dev_data = (pcap_data_t*)data;

    pcap_t * pcap =  pcap_device_open(dev_data->ip,dev_data->hwaddr);
    if (pcap == (pcap_t * )0) {
        dbg_error(DBG_NETIF,"open pcap failed\n name=%s\n",netif->name);
        return NET_ERR_IO;
    }

    netif->mtu = 1500;
    //收发数据包都需要pcap 这个结构体
    netif->ops_data = pcap;
    netif_set_hwaddr(netif,dev_data->hwaddr,6);
    netif->type = NETIF_TYPE_ETHER;


    sys_thread_create(recv_thread,netif);
    sys_thread_create(xmit_thread,netif);
    return NET_ERR_OK;
}

void netif_netdev_close(struct  _netif_t* netif) {
    pcap_t *pcap= (pcap_t *)netif->ops_data;
    pcap_close(pcap);

}

net_err_t netif_netdev_xmit(struct _netif_t *netif) {
    pktbuf_t * pktbuf = netif_get_out(netif,-1);
    if (pktbuf) {
        net_err_t err = netif_put_in(netif,pktbuf,-1);
        if (err < 0) {
            pktbuf_free(pktbuf);
            return err;
        }

    }
    return NET_ERR_OK;
}

const netif_ops_t netdev_ops = {
    .open =netif_netdev_open,
    .close =netif_netdev_close,
    .xmit = netif_netdev_xmit,
};
