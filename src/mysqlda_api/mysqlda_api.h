#ifndef _H_MYSQLDA_API_
#define _H_MYSQLDA_API_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "mysql.h"

int STDCALL mysql_select_library( MYSQL *mysql , const char *library , int *p_index );

#ifdef __cplusplus
}
#endif

#endif

