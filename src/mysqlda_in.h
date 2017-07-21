#ifndef _H_MYSQLDA_IN_
#define _H_MYSQLDA_IN_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include "LOGC.h"
#include "list.h"
#include "rbtree.h"

#include "my_global.h"
#include "mysql.h"

struct MysqlClientForward
{
	char			ip[ 20 + 1 ] ;
	int			port ;
	char			user[ 32 + 1 ] ;
	char			pass[ 32 + 1 ] ;
	
	MYSQL			*mysql_connect ;
	
	struct list_head	*node ;
} ;

struct MysqldaEnvironment
{
	char				*config_filename ;
	int				no_daemon_flag ;
	
	struct MysqlClientForward	forward_list ;
	
} ;

/*
 * util
 */

int BindDaemonServer( int (* ServerMain)( void *pv ) , void *pv , int close_flag );

/*
 * worker
 */

int mysqlda( void *pv );

#endif

