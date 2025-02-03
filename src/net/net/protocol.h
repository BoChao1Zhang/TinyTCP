//
// Created by 32101 on 25-2-3.
//

#ifndef PROTOCOL_H
#define PROTOCOL_H

typedef enum _protocol_t {
    NET_PROTOCOL_ARP = 0x0806,
    NET_PROTOCOL_IPV4 = 0x0800,
    NET_PROTOCOL_ICMP = 0x0800,
    NET_PROTOCOL_TCP = 0x0800,
    NET_PROTOCOL_UDP = 0x0800,
}protocol_t;

#endif //PROTOCOL_H
