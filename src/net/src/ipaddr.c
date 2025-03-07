//
// Created by 32101 on 25-1-24.
//
#include "ipaddr.h"

#include "ipv4.h"
#include "sys_plat.h"

void ipaddr_set_any(ipaddr_t *ip) {
    ip->type = IPADDR_V4;
    ip->q_addr = 0;
}

ipaddr_t * ipaddr_get_any(void) {
    static const ipaddr_t ipaddr_any= {
        .type = IPADDR_V4,
        .q_addr = 0,
    };
    return (ipaddr_t *)&ipaddr_any;
}

net_err_t ipaddr_from_str(ipaddr_t *dest, const char *str) {
    if (!dest || !str) {
        return NET_ERR_PARAM;
    }

    dest->type = IPADDR_V4;
    dest->q_addr = 0;

    uint8_t * p = dest->a_addr;
    char c;
    uint8_t sub_addr = 0;
    while ((c = *str++) != '\0') {
        if ((c >= '0' && c <= '9') || c == '.') {
            if (c == '.') {
                *p++ = sub_addr;
                sub_addr = 0;
            } else {
                sub_addr = sub_addr * 10 + (c - '0');
            }
        } else {
            return NET_ERR_PARAM;
        }
    }
    *p = sub_addr;
    return  NET_ERR_OK;
}

void ipaddr_copy(ipaddr_t *dest, const ipaddr_t *src) {
    if (!dest || !src) {
        return;
    }
    dest->type = src->type;
    dest->q_addr = src->q_addr;
}

int ipaddr_is_equal(const ipaddr_t *ipaddr1, const ipaddr_t *ipaddr2) {
    return ipaddr1->q_addr == ipaddr2->q_addr;
}

void ipaddr_to_buf(ipaddr_t *src, uint8_t *in_buf) {
    *(uint32_t *)in_buf = src->q_addr;
}

void ipaddr_from_buf(ipaddr_t *dest, uint8_t *ip_buf) {
    dest->type = IPADDR_V4;
    dest->q_addr = *(uint32_t *)ip_buf;
}

/**
 * 判断ip地址是否为受限广播地址
 *
 * 本地受限广播，即向本地网络中所有主机发送消息时，使用的地址。
 * 广播的数据包，不能被路由器转发，仅限于该网络内部发送。
 *
 * 例如，主机在初始启动时，可能还没有ip地址，这里需要连接DHCP服务器获取ip
 * 此时，就可以利用受限广播，从本地网络上连接某个DHCP服务器，从而分配得到新ip
 */
int ipaddr_is_local_broadcast(const ipaddr_t * ipaddr) {
    return ipaddr->q_addr == IPV4_ADDR_BROADCAST;
}


ipaddr_t ipaddr_get_host(const ipaddr_t * ipaddr, const ipaddr_t * netmask) {
    ipaddr_t hostid;

    hostid.q_addr = ipaddr->q_addr & ~netmask->q_addr;
    return hostid;
}

ipaddr_t ipaddr_get_net(const ipaddr_t * ipaddr, const ipaddr_t * netmask) {
    ipaddr_t hostid;

    hostid.q_addr = ipaddr->q_addr & netmask->q_addr;

    return hostid;
}


int ipaddr_is_direct_broadcast(const ipaddr_t * ipaddr, const ipaddr_t * netmask) {
    ipaddr_t hostid = ipaddr_get_host(ipaddr, netmask);

    // 判断host_id部分是否为全1
    return hostid.q_addr == (IPV4_ADDR_BROADCAST & ~netmask->q_addr);
}

int ipaddr_is_match(const ipaddr_t *dest, const ipaddr_t *src, const ipaddr_t *netmask) {
    ipaddr_t dest_netid = ipaddr_get_net(dest, netmask);
    ipaddr_t src_netid = ipaddr_get_net(src, netmask);


    if (ipaddr_is_local_broadcast(dest)) {
        return 1;
    }

    if (ipaddr_is_direct_broadcast(dest,netmask) && ipaddr_is_equal(&dest_netid,&src_netid)) {
        return 1;
    }
    return ipaddr_is_equal(dest,src);
}
