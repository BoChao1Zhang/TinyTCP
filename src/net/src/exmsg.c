#include "exmsg.h"
#include "sys_plat.h"


net_err_t exmsg_init(void){
    return NET_ERR_OK;
}

static void work_thread(void * arg){
    plat_printf("exmsg")
    while(1){
        sys_sleep(1);
    }
}

net_err_t exmsg_start(void){
    sys_thread_t  thread = sys_thread_new()
}