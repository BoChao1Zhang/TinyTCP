//
// Created by 32101 on 25-1-22.
//

#ifndef NETIF_H
#define NETIF_H
#include <stdint.h>
#include "ipaddr.h"
#include "nlist.h"
#include "fixq.h"
#include "net_cfg.h"
//ip 32bit
typedef struct _netif_hwaddr_t {
    uint8_t addr[NETIF_HWADDR_SIZE];
    uint8_t len;
}netif_hwaddr_t;

typedef enum _netif_type_t {
    NETIF_TYPE_NONE = 0,
    NETIF_TYPE_ETHER,
    NETIF_TYPE_LOOP,

    NETIF_TYPE_SIZE,
}netif_type_t;

typedef struct _netif_t {
    char name [NETIF_NAME_SIZE];
    netif_hwaddr_t hwaddr;

    ipaddr_t ipaddr;
    ipaddr_t netmask;
    ipaddr_t gateway;

    netif_type_t type;
    int mtu;

    enum {
        NETIF_CLOSED,
        NETIF_OPENED,
        NETIF_ACTIVE,
    }state;

    // 用于链接多张网卡
    nlist_node_t node;
    fixq_t in_q;
    void *in_q_buf[NET_INQ_SIZE];
    fixq_t out_q;
    void *out_q_buf[NET_OUTQ_SIZE];
} netif_t;


net_err_t netif_init(void);

#endif //NETIF_H
