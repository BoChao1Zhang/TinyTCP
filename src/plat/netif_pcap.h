//
// Created by 32101 on 25-1-16.
//

#ifndef NETIF_PCAP_H
#define NETIF_PCAP_H

#include <stdint.h>

#include "netif.h"
#include "net_err.h"

typedef struct  _pcap_data_t {
    const char * ip;
    const uint8_t * hwaddr;
}pcap_data_t;

extern const netif_ops_t netdev_ops;

net_err_t netif_pcap_open();






#endif //NETIF_PCAP_H
