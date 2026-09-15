#include "sqlite3.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static sqlite3 *database;

static void *insert_rows(void *unused){
  int i;
  for(i=0; i<100; ++i){
    if(sqlite3_exec(database, "INSERT INTO sample VALUES(1)", 0, 0, 0)!=SQLITE_OK)
      return (void *)1;
  }
  return 0;
}

int main(int argc, char **argv){
  pthread_t threads[8];
  sqlite3_stmt *statement;
  int i;
  setbuf(stdout, 0);
  if(sqlite3_sleep(2)<2) return 10;
  if(sqlite3_open(argc>1 ? argv[1] : ":memory:", &database)!=SQLITE_OK) return 1;
  /* Entering SQLite while explicitly owning its connection mutex tests the
   * recursive path through the public API. */
  sqlite3_mutex_enter(sqlite3_db_mutex(database));
  if(sqlite3_exec(database, "CREATE TABLE sample(n INTEGER)", 0, 0, 0)!=SQLITE_OK) return 2;
  sqlite3_mutex_leave(sqlite3_db_mutex(database));
  for(i=0; i<8; ++i) if(pthread_create(&threads[i], 0, insert_rows, 0)) return 3;
  for(i=0; i<8; ++i){
    void *result;
    if(pthread_join(threads[i], &result) || result) return 4;
  }
  if(sqlite3_prepare_v2(database, "SELECT count(*),sum(n) FROM sample", -1, &statement, 0)!=SQLITE_OK) return 5;
  if(sqlite3_step(statement)!=SQLITE_ROW || sqlite3_column_int(statement,0)!=800 ||
     sqlite3_column_int(statement,1)!=800) return 6;
  if(sqlite3_finalize(statement)!=SQLITE_OK) return 7;
  if(sqlite3_prepare_v2(database, "PRAGMA integrity_check", -1, &statement, 0)!=SQLITE_OK) return 8;
  if(sqlite3_step(statement)!=SQLITE_ROW ||
     strcmp((const char *)sqlite3_column_text(statement,0), "ok") ||
     sqlite3_step(statement)!=SQLITE_DONE || sqlite3_finalize(statement)!=SQLITE_OK) return 8;
  if(sqlite3_close(database)!=SQLITE_OK) return 9;
  puts("SQLite: recursive connection mutex and 800 concurrent inserts passed");
  return 0;
}
