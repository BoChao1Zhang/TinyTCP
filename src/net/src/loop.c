//
// Created by 32101 on 25-1-24.
//

#include "loop.h"
#include "dbg.h"
#include "exmsg.h"

net_err_t loop_open(struct _netif_t *netif,void *data) {
    netif->type = NETIF_TYPE_LOOP;
    return NET_ERR_OK;
}

void loop_close(struct  _netif_t*netif) {

}

net_err_t loop_xmit(struct _netif_t *netif) {
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

static const netif_ops_t loop_ops = {
    .open =loop_open,
    .close =loop_close,
    .xmit = loop_xmit,
};

net_err_t loop_init(void) {
    dbg_info(DBG_NETIF, "init loop netif\n");

    netif_t * netif = netif_open("loop",&loop_ops,(void *)0);
    if (!netif) {
        dbg_error(DBG_NETIF,"open loop failed\n");
        return NET_ERR_NONE;
    }
    ipaddr_t ip,mask;
    ipaddr_from_str(&ip,"127.0.0.1");
    ipaddr_from_str(&mask,"255.0.0.0");
    net_err_t err = netif_set_addr(netif,&ip,&mask,(ipaddr_t *)0);
    if (err < 0) {
        dbg_error(DBG_NETIF,"set loop addr failed\n");
    }
    netif_set_active(netif);

    pktbuf_t *buf = pktbuf_alloc(100);
    netif_out(netif,(ipaddr_t *)0,buf);

    dbg_info(DBG_NETIF,"init loop done\n");
    return NET_ERR_OK;
}
