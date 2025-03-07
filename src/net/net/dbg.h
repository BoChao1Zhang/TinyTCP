
// Created by 32101 on 25-1-16.
//

#ifndef DBG_H
#define DBG_H

#include <stdint.h>

#include "ipaddr.h"
#include "net_cfg.h"

#define DBG_STYLE_ERROR     "\033[31m"
#define DBG_STYLE_WARNING   "\033[33m"
#define DBG_STYLE_RESET     "\033[0m"

#define DBG_LEVEL_NONE      0
#define DBG_LEVEL_ERROR     1
#define DBG_LEVEL_WARNING   2
#define DBG_LEVEL_INFO      3



void dbg_print(int m_level,int s_level, const char * file, const char * func, int line,const char * fmt,...);
void dbg_println(int m_level,int s_level, const char * file, const char * func, int line,const char * fmt,...);

void dump_mac(const char *msg, const uint8_t *data, int len);
void dump_ip_buf(const char *msg, uint8_t *ipaddr);

#define dbg_info(module,fmt,...) dbg_print(module,DBG_LEVEL_INFO,__FILE__,__FUNCTION__,__LINE__,fmt,##__VA_ARGS__)
#define dbg_warning(module,fmt,...) dbg_print(module,DBG_LEVEL_WARNING,__FILE__,__FUNCTION__,__LINE__,fmt,##__VA_ARGS__)
#define dbg_error(module,fmt,...) dbg_print(module,DBG_LEVEL_ERROR,__FILE__,__FUNCTION__,__LINE__,fmt,##__VA_ARGS__)


#define dbg_dump_ip_buf(module, msg, ip)   {if (module >= DBG_LEVEL_INFO) dump_ip_buf(msg, ip); }
#define dbg_dump_ip(module, msg, ip)   {if (module >= DBG_LEVEL_INFO) dump_ip_buf(msg, (ip)->a_addr); }
#define dbg_dump_mac(module, msg, mac)   {if (module >= DBG_LEVEL_INFO) dump_mac(msg, mac); }

#define dbg_assert(expr,msg) {\
    if(!(expr)) {\
        dbg_print(DBG_LEVEL_ERROR,DBG_LEVEL_ERROR,__FILE__,__FUNCTION__,__LINE__,"assert failed:"#expr","msg); \
        while(1);   \
    }\
}

#endif //DBG_H
