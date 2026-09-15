#include <stdio.h>
#include <pthread.h>
#include <new>
static pthread_mutex_t start_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t start_condition = PTHREAD_COND_INITIALIZER;
static int started, ready, attempts, nested_count, live;
struct Nested {
 int value;
 Nested() : value(21) { ++nested_count; }
};
static Nested& nested() { static Nested n; return n; }
struct Shared {
 int value;
 Shared() {
  ++attempts;
  if (attempts==1) throw 7;
  value=2*nested().value;
 }
};
static Shared& shared() { static Shared s; return s; }
static void* worker(void*) {
 pthread_mutex_lock(&start_lock);
 ++ready; pthread_cond_broadcast(&start_condition);
 while (!started) pthread_cond_wait(&start_condition,&start_lock);
 pthread_mutex_unlock(&start_lock);
 return shared().value==42 ? 0 : (void*)1;
}
struct Base { virtual ~Base() {} };
struct Derived : Base { int value; Derived() : value(42) {} };
struct Lifetime {
 Lifetime() { ++live; }
 ~Lifetime() { --live; }
};
static void unwind_test() { Lifetime value; throw 42; }
int main() {
 setbuf(stdout,0);
 int checks=0;
 puts("allocation and RTTI");
 Base* p=new Derived;
 Derived* d=dynamic_cast<Derived*>(p);
 if (!d || d->value!=42) return 1;
 delete p; ++checks;
 int* array=new int[128]; array[127]=42;
 if (array[127]!=42) return 2;
 delete[] array; ++checks;
 puts("exception unwinding");
 try { unwind_test(); return 3; }
 catch (int value) { if(value!=42 || live!=0) return 4; }
 ++checks;
 puts("static initialization retry");
 try { shared(); return 5; }
 catch (int value) { if(value!=7) return 6; }
 ++checks;
 puts("threaded static initialization");
 pthread_t threads[8];
 for(int i=0;i<8;i++) if(pthread_create(&threads[i],0,worker,0)) return 7;
 pthread_mutex_lock(&start_lock);
 while(ready!=8) pthread_cond_wait(&start_condition,&start_lock);
 started=1; pthread_cond_broadcast(&start_condition);
 pthread_mutex_unlock(&start_lock);
 for(int i=0;i<8;i++) { void* result; if(pthread_join(threads[i],&result) || result) return 8; }
 if(attempts!=2 || nested_count!=1 || shared().value!=42) return 9;
 ++checks;
 printf("C++ runtime: checks=%d attempts=%d nested=%d live=%d\n",checks,attempts,nested_count,live);
 return 0;
}
