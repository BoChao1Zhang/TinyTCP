//
// Created by 32101 on 25-2-3.
//

#ifndef TIMER_H
#define TIMER_H

#include "net_cfg.h"
#include "net_err.h"
#include "nlist.h"

#define NET_TIMER_RELOAD    (1<<0)

struct _net_timer_t;
typedef void (*timer_proc_t)(struct _net_timer_t *timer,void *arg);


// 定义网络定时器结构体
typedef struct _net_timer_t {
    char name[TIMER_NAME_SIZE]; // 定时器名称，用于标识定时器
    int flags; // 定时器标志，用于控制定时器的行为
    int curr; // 当前计时值，单位为毫秒，用于记录定时器当前经过的时间
    int reload; // 重载值，当定时器计时达到此值时重新计时
    timer_proc_t proc; // 定时器处理函数指针，当定时器触发时执行的函数
    void *arg; // 定时器处理函数的参数，用于传递额外的数据
    nlist_node_t node; // 定时器节点，用于将定时器挂载到链表或树等数据结构中
}net_timer_t;


/**
 * 初始化网络定时器列表
 *
 * @return 网络错误代码，表示初始化结果
 */
net_err_t net_timer_init(void);

/**
 * 向网络定时器中添加一个新的定时任务
 *
 *
 * @param timer 指向要添加的定时器的指针
 * @param name 定时器的名称，用于调试和日志记录
 * @param proc 定时器触发时执行的回调函数
 * @param arg 回调函数的参数，可以是任意类型
 * @param ms 定时器的触发时间间隔，单位为毫秒
 * @param flags 定时器的选项标志，控制定时器的行为
 *
 * @return 网络错误代码，表示添加结果
 */
net_err_t net_timer_add(net_timer_t *timer, const char *name, timer_proc_t proc, void *arg, int ms,int flags);

/**
 * 从网络定时器中移除一个定时任务
 *
 * @param timer 指向要移除的定时器的指针
 *
 * @return 网络错误代码，表示移除结果
 */
net_err_t net_timer_remove(net_timer_t *timer);

/**
 * 检查网络定时器中的任务是否已超时
 *
 * @param diff_ms 自上次检查以来经过的时间，单位为毫秒
 *
 * @return 网络错误代码，表示检查结果
 */
net_err_t net_timer_check_tmo(int diff_ms);

/**
 * 获取网络定时器中第一个即将超时的任务的超时时间
 *
 * @return 第一个即将超时的任务的超时时间，单位为毫秒如果定时器列表为空，则返回特定值
 */
int net_timer_first_tmo(void);

#endif //TIMER_H
