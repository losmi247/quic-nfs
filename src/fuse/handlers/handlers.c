#include "handlers.h"

pthread_mutex_t nfs_mutex = PTHREAD_MUTEX_INITIALIZER;

void wait_for_nfs_reply(CallbackData *callback_data) {
    pthread_mutex_lock(&callback_data->lock);

    while(!callback_data->is_finished) {
        pthread_cond_wait(&callback_data->cond, &callback_data->lock);  // releases the lock while waiting
    }

    pthread_mutex_unlock(&callback_data->lock);
}