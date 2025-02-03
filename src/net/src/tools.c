//
// Created by 32101 on 25-2-2.
//
#include "tools.h"

#include "dbg.h"

static int is_litte_endian(void) {
    uint16_t v = 0x1234;
    return *((uint8_t *)&v) == 0x34;
}
net_err_t tools_init(void) {
    dbg_info(DBG_TOOLS, "init tools\n");

    if (is_litte_endian() != NET_ENDIAN_LITTLE) {
        dbg_error(DBG_TOOLS,"check endian failed\n");
        return NET_ERR_SYS;
    }
    dbg_info(DBG_TOOLS, "init tools done\n");
    return NET_ERR_OK;
}
