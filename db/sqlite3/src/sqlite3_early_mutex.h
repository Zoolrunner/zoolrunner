/* SQLite's mutex-method interface over the early Darwin recursive helper.
 * Included by the amalgamation so allocation uses SQLite's configured allocator.
 * Fast/static mutexes may also be recursive, as permitted by this interface. */
#include "../../../build/macosx/compat/zoolrunner-recursive-mutex.h"

struct sqlite3_mutex {
  zr_recursive_mutex native;
  int dynamic;
};

SQLITE_PRIVATE void sqlite3MemoryBarrier(void){
#ifdef SQLITE_MEMORY_BARRIER
  SQLITE_MEMORY_BARRIER;
#elif defined(__clang__) || (defined(__GNUC__) && GCC_VERSION>=4001000)
  __sync_synchronize();
#else
  static pthread_mutex_t barrier = PTHREAD_MUTEX_INITIALIZER;
  if( pthread_mutex_lock(&barrier) || pthread_mutex_unlock(&barrier) ) abort();
#endif
}

static int zrSqliteMutexInit(void) { return SQLITE_OK; }
static int zrSqliteMutexEnd(void) { return SQLITE_OK; }

static sqlite3_mutex *zrSqliteMutexAlloc(int type){
#define ZR_SQLITE_MUTEX_INIT { ZR_RECURSIVE_MUTEX_INITIALIZER, 0 }
  static sqlite3_mutex shared[] = {
    ZR_SQLITE_MUTEX_INIT, ZR_SQLITE_MUTEX_INIT, ZR_SQLITE_MUTEX_INIT,
    ZR_SQLITE_MUTEX_INIT, ZR_SQLITE_MUTEX_INIT, ZR_SQLITE_MUTEX_INIT,
    ZR_SQLITE_MUTEX_INIT, ZR_SQLITE_MUTEX_INIT, ZR_SQLITE_MUTEX_INIT,
    ZR_SQLITE_MUTEX_INIT, ZR_SQLITE_MUTEX_INIT, ZR_SQLITE_MUTEX_INIT
  };
#undef ZR_SQLITE_MUTEX_INIT
  sqlite3_mutex *mutex;
  if( type==SQLITE_MUTEX_FAST || type==SQLITE_MUTEX_RECURSIVE ){
    mutex = sqlite3MallocZero(sizeof(*mutex));
    if( mutex && zr_recursive_init(&mutex->native) ){
      sqlite3_free(mutex);
      return 0;
    }
    if( mutex ) mutex->dynamic = 1;
    return mutex;
  }
  if( type<2 || type-2>=ArraySize(shared) ) return 0;
  return &shared[type-2];
}

static void zrSqliteMutexFree(sqlite3_mutex *mutex){
  if( !mutex || !mutex->dynamic ) return;
  if( zr_recursive_destroy(&mutex->native) ) abort();
  sqlite3_free(mutex);
}
static void zrSqliteMutexEnter(sqlite3_mutex *mutex){
  if( zr_recursive_lock(&mutex->native, 0) ) abort();
}
static int zrSqliteMutexTry(sqlite3_mutex *mutex){
  return zr_recursive_lock(&mutex->native, 1) ? SQLITE_BUSY : SQLITE_OK;
}
static void zrSqliteMutexLeave(sqlite3_mutex *mutex){
  if( zr_recursive_unlock(&mutex->native) ) abort();
}
#ifdef SQLITE_DEBUG
static int zrSqliteMutexHeld(sqlite3_mutex *mutex){
  int held;
  if( !mutex ) return 1;
  if( pthread_mutex_lock(&mutex->native.state) ) abort();
  held = mutex->native.depth && pthread_equal(mutex->native.owner, pthread_self());
  if( pthread_mutex_unlock(&mutex->native.state) ) abort();
  return held;
}
static int zrSqliteMutexNotheld(sqlite3_mutex *mutex){
  return !mutex || !zrSqliteMutexHeld(mutex);
}
#endif
SQLITE_PRIVATE sqlite3_mutex_methods const *sqlite3DefaultMutex(void){
  static const sqlite3_mutex_methods methods = {
    zrSqliteMutexInit, zrSqliteMutexEnd, zrSqliteMutexAlloc, zrSqliteMutexFree,
    zrSqliteMutexEnter, zrSqliteMutexTry, zrSqliteMutexLeave,
#ifdef SQLITE_DEBUG
    zrSqliteMutexHeld, zrSqliteMutexNotheld
#else
    0, 0
#endif
  };
  return &methods;
}
