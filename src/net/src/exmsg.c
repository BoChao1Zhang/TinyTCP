#include "exmsg.h"

#include "dbg.h"
#include "sys_plat.h"
#include "fixq.h"
#include "ipv4.h"
#include "mblock.h"
#include "timer.h"
#include "sys.h"
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

static net_err_t do_netif_in(exmsg_t *msg) {
    netif_t * netif = msg->netif.netif;

    pktbuf_t *buf;
    while (buf = netif_get_in(netif,-1)) {
        dbg_info(DBG_MSG,"recv a packet %p\n",buf);

        if (netif->link_layer) {
            net_err_t err = netif->link_layer->in(netif,buf);
            if (err < 0) {
                pktbuf_free(buf);
                dbg_warning(DBG_MSG,"netif in fail, error=%d\n",err);
            }
        } else {
            net_err_t err = ipv4_in(netif,buf);
            if (err < 0) {
                pktbuf_free(buf);
                dbg_warning(DBG_MSG,"netif in fail, error=%d\n",err);
            }

        }
    }
    return NET_ERR_OK;
}

static void work_thread(void *arg) {
    dbg_info(DBG_MSG, "exmsg work thread is running...\n");
    net_time_t time;
    sys_time_curr(&time);
    while (1) {
        int first_tmo = net_timer_first_tmo();
        exmsg_t *msg = (exmsg_t *) fixq_recv(&msg_queue, first_tmo);
        if (msg) {
            dbg_info(DBG_MSG,"recv a msg %p: %d\n",msg,msg->type);
            switch (msg->type) {
                case NET_EXMSG_NETIF_IN:
                    do_netif_in(msg);
                break;
                default: break;
            }
            mblock_free(&msg_mblock, msg);
        }
        int diff_ms = sys_time_goes(&time);
        net_timer_check_tmo(diff_ms);
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

    msg->netif.netif = netif;
    msg->type = NET_EXMSG_NETIF_IN;

    net_err_t err = fixq_send(&msg_queue, msg,0);
    if (err!= NET_ERR_OK) {
        dbg_warning(DBG_MSG, "fixq send fail\n");
        mblock_free(&msg_mblock, msg);
        return NET_ERR_SYS;
    }

    return err;


}