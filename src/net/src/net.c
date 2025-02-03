#include "net.h"
#include "exmsg.h"
#include "net_plat.h"
#include "pktbuf.h"
#include "dbg.h"
#include "loop.h"
#include "netif.h"
#include "ether.h"
#include "tools.h"

net_err_t net_init(void){
    dbg_info(DBG_INIT,"init net\n");

    net_plat_init();
    tools_init();
    exmsg_init();
    pktbuf_init();
    netif_init();
    loop_init();
    ether_init();
    return NET_ERR_OK;
}

net_err_t net_start(void){
    exmsg_start();
    return NET_ERR_OK;
}