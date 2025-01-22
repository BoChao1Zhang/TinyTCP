//
// Created by 32101 on 25-1-16.
//

#include "net_plat.h"
#include "dbg.h"

net_err_t net_plat_init(void){
    dbg_info(DBG_PLAT,"init plat...");
    dbg_info(DBG_PLAT,"init down...");
    return NET_ERR_OK;
}