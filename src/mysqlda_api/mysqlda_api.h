#ifndef _H_MYSQLDA_API_
#define _H_MYSQLDA_API_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "mysql.h"

#ifdef __cplusplus
extern "C" {
#endif

int STDCALL mysql_select_library( MYSQL *mysql , const char *library , char *instance , int instance_bufsize );

#ifdef __cplusplus
}
#endif

#endif

