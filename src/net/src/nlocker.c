//
// Created by 32101 on 25-1-16.
//
#include "nlocker.h"

net_err_t nlocker_init(nlocker_t *locker, nlocker_type_t type) {
    if (type == NLOKCER_THREAD) {
        sys_mutex_t mutex = sys_mutex_create();
        if (mutex == SYS_MUTEX_INVALID) {
            return NET_ERR_SYS;
        }
    }

    locker->type = type;
    return NET_ERR_OK;
}

void nlocker_destroy(nlocker_t *locker) {
    if (locker->type == NLOKCER_THREAD) {
        sys_mutex_free(locker->mutex);
    }
}

void nlocker_lock(nlocker_t *locker) {
    if (locker->type == NLOKCER_THREAD) {
        sys_mutex_lock(locker->mutex);
    }
}

void nlocker_unlock(nlocker_t *locker) {
    if (locker->type == NLOKCER_THREAD) {
        sys_mutex_unlock(locker->mutex);
    }
}



