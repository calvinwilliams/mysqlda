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

#include "IDL_mysqlda_conf.dsc.h"

struct MysqlForwardClient
{
	char		instance[ sizeof(((mysqlda_conf*)0)->forward[0].instance) ] ;
	char		ip[ sizeof(((mysqlda_conf*)0)->forward[0].ip) ] ;
	int		port ;
	char		user[ sizeof(((mysqlda_conf*)0)->forward[0].user) ] ;
	char		pass[ sizeof(((mysqlda_conf*)0)->forward[0].pass) ] ;
	char		db[ sizeof(((mysqlda_conf*)0)->forward[0].db) ] ;
	unsigned int	power ;

	unsigned long	serial_range_begin ;
	
	MYSQL		*mysql_connection ;
	
	struct rb_node	forward_rbnode ;
} ;

struct MysqldaEnvironment
{
	char		*config_filename ;
	int		no_daemon_flag ;
	char		*action ;
	
	struct rb_root	forward_rbtree ;
	unsigned long	total_power ;
	
	char		listen_ip[ sizeof(((mysqlda_conf*)0)->server.listen_ip) ] ;
	int		listen_port ;
} ;

/*
 * util
 */

int WriteEntireFile( char *pathfilename , char *file_content , int file_len );
char *StrdupEntireFile( char *pathfilename , int *p_file_len );

int BindDaemonServer( int (* ServerMain)( void *pv ) , void *pv , int close_flag );

/*
 * config
 */

int InitConfigFile( struct MysqldaEnvironment *p_env );
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

