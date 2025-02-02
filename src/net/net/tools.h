//
// Created by 32101 on 25-2-2.
//

#ifndef TOOLS_H
#define TOOLS_H
#include "net_cfg.h"
#include<stdint.h>

static inline uint16_t swap_u16(uint16_t v) {
    const uint16_t r = ((v & 0xff) << 8) | ((v >> 8) & 0xff);
    return r;
}

static inline uint32_t swap_u32(uint32_t v) {
    const uint32_t r =
              (((v >> 0) & 0xff) << 24)
            | (((v >> 8) & 0xff) << 16)
            | (((v >> 16) & 0xff) << 8)
            | (((v >> 24) & 0xff) << 0);
    return  r;
}

// h - host n-net s-short 16 l-32
#if NET_ENDIAN_LITTLE
#define x_htons(v)  swap_u16(v)
#define x_ntohs(v)  swap_u16(v)
#define x_htonl(v)  swap_u32(v)
#define x_ntohl(v)  swap_u32(v)
#else
#define x_htons(v)
#define x_ntohs(v)
#define x_htonl(v)
#define x_ntohl(v)
#endif

net_err_t tools_init(void);

#endif //TOOLS_H
