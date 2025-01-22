//
// Created by 32101 on 25-1-16.
//

#ifndef NET_CFG_H
#define NET_CFG_H


#define DBG_MBLOCK  DBG_LEVEL_INFO
#define DBG_QUEUE   DBG_LEVEL_INFO
#define DBG_MSG     DBG_LEVEL_INFO
#define DBG_BUF     DBG_LEVEL_INFO
#define DBG_INIT    DBG_LEVEL_INFO
#define DBG_PLAT    DBG_LEVEL_INFO


#define EXMSG_MSG_CNT 10
#define EXMSG_LOCKER NLOKCER_THREAD

#define PKTBUF_BLK_SIZE 128
#define PKTBUF_BLK_CNT  100
#define PKTBUF_BUF_CNT  100

#define DBG_DISP_ENABLED(module) (module >= DBG_LEVEL_INFO)

#endif //NET_CFG_H
