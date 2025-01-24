#include "exmsg.h"

#include "dbg.h"
#include "sys_plat.h"
#include "fixq.h"
#include "mblock.h"
static void *msg_tbl[EXMSG_MSG_CNT];
static fixq_t msg_queue;
static exmsg_t msg_buffer[EXMSG_MSG_CNT];
static mblock_t msg_mblock;

net_err_t exmsg_init(void) {
    dbg_info(DBG_MSG, "exmsg init\n");

    net_err_t err = fixq_init(&msg_queue, msg_tbl,EXMSG_MSG_CNT, NLOKCER_THREAD);
    if (err != NET_ERR_OK) {
        dbg_error(DBG_MSG, "fixq init fail\n");
        return NET_ERR_SYS;
    }

    err = mblock_init(&msg_mblock, msg_buffer, sizeof(exmsg_t),EXMSG_MSG_CNT,EXMSG_LOCKER);
    if (err != NET_ERR_OK) {
        dbg_error(DBG_MSG, "mblock init fail\n");
        return NET_ERR_SYS;
    }

    dbg_info(DBG_MSG, "exmsg init ok\n");
    return NET_ERR_OK;
}

static void work_thread(void *arg) {
    dbg_info(DBG_MSG, "exmsg work thread is running...\n");
    while (1) {
        exmsg_t *msg = (exmsg_t *) fixq_recv(&msg_queue, 0);
        dbg_info(DBG_MSG, "recv msg type:%d,id %d\n", msg->type, msg->id);

        mblock_free(&msg_mblock, msg);
    }
}

net_err_t exmsg_start(void) {
    sys_thread_t thread = sys_thread_create(work_thread, (void *) 0);

    if (thread == SYS_THREAD_INVALID) {
        return NET_ERR_SYS;
    }

    return NET_ERR_OK;
}

net_err_t exmsg_netif_in(netif_t* netif) {
    exmsg_t *msg = (exmsg_t *) mblock_alloc(&msg_mblock, -1);
    if (!msg) {
        dbg_warning(DBG_MSG, "no free exmsg \n");
        return NET_ERR_MEM;
    }

    static int id = 0;
    msg->id = id++;
    msg->type = NET_EXMSG_NETIF_IN;

    net_err_t err = fixq_send(&msg_queue, msg,-1);
    if (err!= NET_ERR_OK) {
        dbg_warning(DBG_MSG, "fixq send fail\n");
        mblock_free(&msg_mblock, msg);
        return NET_ERR_SYS;
    }

    return err;


}