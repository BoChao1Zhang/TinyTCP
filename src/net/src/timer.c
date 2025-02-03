//
// Created by 32101 on 25-2-3.
//

#include "timer.h"
#include "dbg.h"

static nlist_t timer_list;

net_err_t net_timer_init(void) {
    dbg_info(DBG_TIMER, "timer init\n");
    nlist_init(&timer_list);
    dbg_info(DBG_TIMER, "timer init done\n");
    return NET_ERR_OK;
}

net_err_t net_timer_add(net_timer_t *timer, const char *name, timer_proc_t proc, void *arg, int ms, int flags) {
}
