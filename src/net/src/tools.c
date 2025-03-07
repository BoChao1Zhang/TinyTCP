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

//blk0 blk1
uint16_t checksum16(void* buf, uint16_t len, uint32_t pre_sum, int complement) {
    uint16_t* curr_buf = (uint16_t *)buf;
    uint32_t checksum = pre_sum;

    while (len > 1) {
        checksum += *curr_buf++;
        len -= 2;
    }

    if (len > 0) {
        checksum += *(uint8_t*)curr_buf;
    }

    // 注意，这里要不断累加。不然结果在某些情况下计算不正确
    uint16_t high;
    while ((high = checksum >> 16) != 0) {
        checksum = high + (checksum & 0xffff);
    }

    return complement ? (uint16_t)~checksum : (uint16_t)checksum;
}
