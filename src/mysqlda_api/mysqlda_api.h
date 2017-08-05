#ifndef _H_MYSQLDA_API_
#define _H_MYSQLDA_API_

#ifdef __cplusplus
extern "C" {
#endif

#include "mysql.h"

int STDCALL mysql_select_library( MYSQL *mysql , const char *library );

#ifdef __cplusplus
}
#endif

#endif

