/*
** ZoolRunner compatibility entry points for the Mozilla 1.8.1-era
** mozStorage integration.
*/

#include "sqlite3.h"

#include <string.h>

SQLITE_API int sqlite3_bind_parameter_indexes(
  sqlite3_stmt *pStmt,
  const char *zName,
  int **pIndexes
){
  int i;
  int j;
  int nCount;
  int nMatch;
  int *aIndexes;

  if( pIndexes ){
    *pIndexes = 0;
  }
  if( !pStmt || !zName || !pIndexes ){
    return 0;
  }

  nCount = sqlite3_bind_parameter_count(pStmt);
  nMatch = 0;
  for(i=1; i<=nCount; i++){
    const char *zParam = sqlite3_bind_parameter_name(pStmt, i);
    if( zParam && strcmp(zParam, zName)==0 ){
      nMatch++;
    }
  }
  if( nMatch==0 ){
    return 0;
  }

  aIndexes = (int *)sqlite3_malloc((int)(sizeof(int) * nMatch));
  if( !aIndexes ){
    return 0;
  }

  j = 0;
  for(i=1; i<=nCount; i++){
    const char *zParam = sqlite3_bind_parameter_name(pStmt, i);
    if( zParam && strcmp(zParam, zName)==0 ){
      aIndexes[j++] = i;
    }
  }

  *pIndexes = aIndexes;
  return nMatch;
}

SQLITE_API void sqlite3_free_parameter_indexes(int *pIndexes)
{
  sqlite3_free(pIndexes);
}

SQLITE_API int sqlite3Preload(sqlite3 *db)
{
  sqlite3_stmt *stmt;
  int rc;
  int loaded;

  if( !db ){
    return SQLITE_MISUSE;
  }

  rc = sqlite3_prepare(db, "PRAGMA database_list", -1, &stmt, 0);
  if( rc!=SQLITE_OK ){
    return rc;
  }

  loaded = 0;
  while( (rc = sqlite3_step(stmt))==SQLITE_ROW ){
    const unsigned char *zSchema = sqlite3_column_text(stmt, 1);
    char *zSql;
    sqlite3_stmt *preloadStmt;

    if( !zSchema ){
      continue;
    }

    zSql = sqlite3_mprintf("PRAGMA \"%w\".quick_check", (const char *)zSchema);
    if( !zSql ){
      sqlite3_finalize(stmt);
      return SQLITE_NOMEM;
    }

    rc = sqlite3_prepare(db, zSql, -1, &preloadStmt, 0);
    sqlite3_free(zSql);
    if( rc!=SQLITE_OK ){
      sqlite3_finalize(stmt);
      return rc;
    }

    while( (rc = sqlite3_step(preloadStmt))==SQLITE_ROW ){
      loaded = 1;
    }
    rc = sqlite3_finalize(preloadStmt);
    if( rc!=SQLITE_OK ){
      sqlite3_finalize(stmt);
      return rc;
    }
  }

  rc = sqlite3_finalize(stmt);
  if( rc!=SQLITE_OK ){
    return rc;
  }

  return loaded ? SQLITE_OK : SQLITE_ERROR;
}
