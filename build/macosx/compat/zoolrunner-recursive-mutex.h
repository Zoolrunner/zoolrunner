#ifndef ZOOLRUNNER_RECURSIVE_MUTEX_H
#define ZOOLRUNNER_RECURSIVE_MUTEX_H
#include <pthread.h>
#include <errno.h>
#include <limits.h>
/* Early Darwin lacks recursive mutex attributes. The gate remains owned by
 * the logical owner; state protects recursion bookkeeping. Using mutexes
 * instead of condition waits introduces no cancellation point into lock(). */
typedef struct {
    pthread_mutex_t state;
    pthread_mutex_t gate;
    pthread_t owner;
    unsigned int depth;
} zr_recursive_mutex;
#define ZR_RECURSIVE_MUTEX_INITIALIZER \
    { PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, 0, 0 }
static inline int zr_recursive_init(zr_recursive_mutex *mutex)
{
    int result = pthread_mutex_init(&mutex->state, 0);
    if (result) return result;
    result = pthread_mutex_init(&mutex->gate, 0);
    if (result) {
        pthread_mutex_destroy(&mutex->state);
        return result;
    }
    mutex->owner = 0;
    mutex->depth = 0;
    return 0;
}
static inline int zr_recursive_destroy(zr_recursive_mutex *mutex)
{
    int result = pthread_mutex_lock(&mutex->state);
    if (result) return result;
    if (mutex->depth) {
        pthread_mutex_unlock(&mutex->state);
        return EBUSY;
    }
    pthread_mutex_unlock(&mutex->state);
    result = pthread_mutex_destroy(&mutex->gate);
    if (result) return result;
    return pthread_mutex_destroy(&mutex->state);
}
static inline int zr_recursive_lock(zr_recursive_mutex *mutex, int try_only)
{
    int result = pthread_mutex_lock(&mutex->state);
    if (result) return result;
    pthread_t self = pthread_self();
    if (mutex->depth && pthread_equal(mutex->owner, self)) {
        if (mutex->depth == UINT_MAX) {
            pthread_mutex_unlock(&mutex->state);
            return EAGAIN;
        }
        ++mutex->depth;
        return pthread_mutex_unlock(&mutex->state);
    }
    result = pthread_mutex_unlock(&mutex->state);
    if (result) return result;
    result = try_only ? pthread_mutex_trylock(&mutex->gate)
                      : pthread_mutex_lock(&mutex->gate);
    if (result) return result;
    result = pthread_mutex_lock(&mutex->state);
    if (result) {
        pthread_mutex_unlock(&mutex->gate);
        return result;
    }
    mutex->owner = self;
    mutex->depth = 1;
    return pthread_mutex_unlock(&mutex->state);
}
static inline int zr_recursive_unlock(zr_recursive_mutex *mutex)
{
    int result = pthread_mutex_lock(&mutex->state);
    if (result) return result;
    if (!mutex->depth || !pthread_equal(mutex->owner, pthread_self())) {
        pthread_mutex_unlock(&mutex->state);
        return EPERM;
    }
    --mutex->depth;
    result = mutex->depth ? 0 : pthread_mutex_unlock(&mutex->gate);
    int unlock_result = pthread_mutex_unlock(&mutex->state);
    return result ? result : unlock_result;
}
#endif
