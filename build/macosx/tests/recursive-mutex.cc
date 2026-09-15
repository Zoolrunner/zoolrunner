#include "../compat/zoolrunner-recursive-mutex.h"
#include <assert.h>
#include <stdio.h>
static zr_recursive_mutex mutex = ZR_RECURSIVE_MUTEX_INITIALIZER;
static int count;
static pthread_mutex_t ready_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ready_condition = PTHREAD_COND_INITIALIZER;
static int cancel_ready;
static void* increment(void*) {
 for(int i=0;i<1000;i++) {
  assert(zr_recursive_lock(&mutex,0)==0);
  assert(zr_recursive_lock(&mutex,0)==0);
  ++count;
  assert(zr_recursive_unlock(&mutex)==0);
  assert(zr_recursive_unlock(&mutex)==0);
 }
 return 0;
}
static void* busy(void*) {
 assert(zr_recursive_lock(&mutex,1)==EBUSY);
 assert(zr_recursive_unlock(&mutex)==EPERM);
 return 0;
}
static void* cancel_waiter(void*) {
 pthread_mutex_lock(&ready_lock);
 cancel_ready=1;
 pthread_cond_signal(&ready_condition);
 pthread_mutex_unlock(&ready_lock);
 assert(zr_recursive_lock(&mutex,0)==0);
 assert(zr_recursive_unlock(&mutex)==0);
 pthread_testcancel();
 return 0;
}
int main() {
 zr_recursive_mutex dynamic;
 assert(zr_recursive_init(&dynamic)==0);
 assert(zr_recursive_lock(&dynamic,0)==0);
 assert(zr_recursive_lock(&dynamic,1)==0);
 assert(zr_recursive_destroy(&dynamic)==EBUSY);
 assert(zr_recursive_unlock(&dynamic)==0);
 assert(zr_recursive_unlock(&dynamic)==0);
 assert(zr_recursive_destroy(&dynamic)==0);
 pthread_t threads[8];
 for(int i=0;i<8;i++) assert(pthread_create(&threads[i],0,increment,0)==0);
 for(int i=0;i<8;i++) assert(pthread_join(threads[i],0)==0);
 assert(count==8000);
 assert(zr_recursive_lock(&mutex,0)==0);
 assert(pthread_create(&threads[0],0,busy,0)==0);
 assert(pthread_join(threads[0],0)==0);
 mutex.depth=UINT_MAX;
 assert(zr_recursive_lock(&mutex,0)==EAGAIN);
 mutex.depth=1;
 assert(zr_recursive_unlock(&mutex)==0);
 assert(zr_recursive_unlock(&mutex)==EPERM);
 assert(zr_recursive_lock(&mutex,0)==0);
 assert(pthread_create(&threads[0],0,cancel_waiter,0)==0);
 pthread_mutex_lock(&ready_lock);
 while(!cancel_ready) pthread_cond_wait(&ready_condition,&ready_lock);
 pthread_mutex_unlock(&ready_lock);
 assert(pthread_cancel(threads[0])==0);
 assert(zr_recursive_unlock(&mutex)==0);
 void* result=0;
 assert(pthread_join(threads[0],&result)==0 && result==PTHREAD_CANCELED);
 assert(zr_recursive_lock(&mutex,1)==0);
 assert(zr_recursive_unlock(&mutex)==0);
 assert(zr_recursive_destroy(&mutex)==0);
 puts("recursive mutex: contention, recursion, ownership, overflow and cancellation passed");
}
