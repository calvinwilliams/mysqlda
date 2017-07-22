#ifndef _H_MYSQLDA_IN_
#define _H_MYSQLDA_IN_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include "LOGC.h"
#include "rbtree.h"

#include "my_global.h"
#include "mysql.h"

struct MysqlForwardClient
{
	char			instance[ 20 + 1 ] ;
	char			ip[ 20 + 1 ] ;
	int			port ;
	char			user[ 32 + 1 ] ;
	char			pass[ 32 + 1 ] ;
	char			db[ 32 + 1 ] ;
	unsigned long		power ;
	
	unsigned long		serial_range_begin ;
	
	MYSQL			*mysql_connection ;
	
	struct rb_node		client_rbnode ;
} ;

struct MysqldaEnvironment
{
	char			*config_filename ;
	int			no_daemon_flag ;
	
	struct rb_root		client_rbtree ;
	unsigned long		total_power ;
} ;

/*
 * util
 */

int BindDaemonServer( int (* ServerMain)( void *pv ) , void *pv , int close_flag );

/*
 * config
 */

int LoadConfig( struct MysqldaEnvironment *p_env );

/*
 * worker
 */

int worker( void *pv );

/*
 * rbtree
 */

int LinkMysqlForwardClientTreeNode( struct MysqldaEnvironment *p_env , struct MysqlForwardClient *p_client_rbnode );
void UnlinkMysqlForwardClientTreeNode( struct MysqldaEnvironment *p_env , struct MysqlForwardClient *p_client_rbnode );
void DestroyTcpdaemonAcceptedSessionTree( struct MysqldaEnvironment *p_env );
struct MysqlForwardClient *QueryMysqlForwardClientRangeTreeNode( struct MysqldaEnvironment *p_env , unsigned long serial_no );
struct MysqlForwardClient *TravelMysqlForwardClientTreeNode( struct MysqldaEnvironment *p_env , struct MysqlForwardClient *p_client );

#endif

