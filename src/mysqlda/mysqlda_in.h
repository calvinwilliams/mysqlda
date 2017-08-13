#ifndef _H_MYSQLDA_IN_
#define _H_MYSQLDA_IN_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/wait.h>

#include "LOGC.h"
#include "lk_list.h"
#include "rbtree.h"
#include "network.h"
#include "ctl_epoll.h"

#include "my_global.h"
#include "mysql.h"

#include "openssl/evp.h"
#include "openssl/sha.h"

#include "IDL_mysqlda_conf.dsc.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LIKELY
#if __GNUC__ >= 3
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif
#endif

/* 通讯基础信息结构 */
struct NetAddress
{
	char			ip[ sizeof(((mysqlda_conf*)0)->forwards[0].forward[0].ip) + 1 ] ;
	int			port ;
	int			sock ;
	struct sockaddr_in	addr ;
} ;

/* 侦听会话结构 */
struct ListenSession
{
	struct NetAddress	netaddr ;
} ;

#define SESSIONTYPE_ALIVEPIPESESSION	0
#define SESSIONTYPE_ACCEPTEDSESSION	1
#define SESSIONTYPE_FORWARDSESSION	2

#define SESSIONSTATUS_BEFORE_SENDING_HANDSHAKE						1
#define SESSIONSTATUS_AFTER_SENDING_HANDSHAKE_AND_BEFORE_RECEIVING_AUTHENTICATION	2
#define SESSIONSTATUS_AFTER_SENDING_AUTH_FAIL_AND_BEFORE_FORWARDING			3
#define SESSIONSTATUS_AFTER_SENDING_AUTH_OK_AND_BEFORE_RECEIVING_SELECT_LIBRARY		4
#define SESSIONSTATUS_AFTER_SENDING_SELECT_LIBRARY_AND_BEFORE_FORDWARD			5
#define SESSIONSTATUS_FORWARDING							7

#define MYSQL_COMMLEN(_cl_)	((_cl_[0]+_cl_[1]*0xFF+_cl_[2]*0xFF*0xFF))

/* 存活管道会话 结构 */
struct AlivePipeSession
{
	unsigned char		type ;
	
	int			alive_pipe[ 2 ] ;
} ;

/* 客户端连接会话 结构 */
#define MAXLEN_ACCEPTED_SESSION_COMM_BUFFER	(3+1+(2<<24))

struct ForwardSession ;
struct AcceptedSession
{
	unsigned char		type ;
	int			status ;
	
	struct ForwardServer	*p_forward_server ;
	struct NetAddress	netaddr ;
	
	char			*comm_buffer ;
	int			fill_len ;
	int			process_len ;
	int			comm_body_len ;
	
	char			random_data[ 20 ] ;
	
	struct ForwardSession	*p_pair_forward_session ;
} ;

/* 服务端转发会话 结构 */
struct ForwardPower ;
struct ForwardSession
{
	char			type ;
	
	struct ForwardPower	*p_forward_power ;
	struct ForwardServer	*p_forward_server ;
	
	MYSQL			*mysql_connection ;
	struct AcceptedSession	*p_pair_accepted_session ;
	
	struct rb_node		forward_session_rbnode ;
} ;

/* 服务端转发服务器信息 结构 */
struct ForwardServer
{
	struct NetAddress	netaddr ;
	
	struct lk_list_head	forward_server_listnode ;
} ;

/* 服务端转发库权重 结构 */
struct ForwardPower
{
	char			instance[ sizeof(((mysqlda_conf*)0)->forwards[0].instance) ] ;
	struct rb_node		forward_instance_rbnode ;
	
	struct lk_list_head	forward_server_list ;
	
	unsigned long		power ;
	
	unsigned long		serial_range_begin ;
	struct rb_node		forward_serial_range_rbnode ;
} ;

/* 服务端转发规则历史 结构 */
#define MAXLEN_LIBRARY		64

struct ForwardLibrary
{
	char			library[ MAXLEN_LIBRARY + 1 ] ;
	struct rb_node		forward_library_rbnode ;
	
	struct ForwardPower	*p_forward_power ;
} ;

/* MySQL分布式代理 环境结构 */
struct MysqldaEnvironment
{
	char			*config_filename ;
	char			*save_filename ;
	int			no_daemon_flag ;
	char			*action ;
	
	char			user[ sizeof(((mysqlda_conf*)0)->auth.user) ] ;
	char			pass[ sizeof(((mysqlda_conf*)0)->auth.pass) ] ;
	char			db[ sizeof(((mysqlda_conf*)0)->auth.db) ] ;
	
	struct rb_root		forward_power_rbtree ;
	struct rb_root		forward_instance_rbtree ;
	unsigned long		total_power ;
	
	struct AlivePipeSession	alive_pipe_session ;
	struct ListenSession	listen_session ;
	
	int			epoll_fd ;
	
	struct rb_root		forward_library_rbtree ;
} ;

/*
 * util
 */

int WriteEntireFile( char *pathfilename , char *file_content , int file_len );
char *StrdupEntireFile( char *pathfilename , int *p_file_len );

int BindDaemonServer( int (* ServerMain)( void *pv ) , void *pv , int close_flag );

void GenerateRandomDataWithoutNull( char *data , int data_len );

unsigned long CalcHash( char *str , int str_len );

/*
 * config
 */

int InitConfigFile( struct MysqldaEnvironment *p_env );

void AddForwardPowerTreeNodePower( struct MysqldaEnvironment *p_env , struct ForwardPower *p_this_forward_power );

int LoadConfig( struct MysqldaEnvironment *p_env );
void UnloadConfig( struct MysqldaEnvironment *p_env );

/*
 * monitor
 */

int monitor( void *pv );

/*
 * worker
 */

int worker( void *pv );

/*
 * comm
 */

int OnAcceptingSocket( struct MysqldaEnvironment *p_env , struct ListenSession *p_listen_session );
int OnReceivingAcceptedSocket( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session );
int OnSendingAcceptedSocket( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session );
int OnClosingAcceptedSocket( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session );

int OnReceivingForwardSocket( struct MysqldaEnvironment *p_env , struct ForwardSession *p_forward_session );
int OnSendingForwardSocket( struct MysqldaEnvironment *p_env , struct ForwardSession *p_forward_session );
int OnClosingForwardSocket( struct MysqldaEnvironment *p_env , struct ForwardSession *p_forward_session );

/*
 * app
 */

int FormatHandshakeMessage( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session );
int CheckAuthenticationMessage( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session );
int FormatAuthResultFail( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session );
int FormatAuthResultOk( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session );
int DatabaseSelectLibrary( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session );

/*
 * rbtree
 */

int LinkForwardInstanceTreeNode( struct MysqldaEnvironment *p_env , struct ForwardPower *p_forward_power );
struct ForwardPower *QueryForwardInstanceTreeNode( struct MysqldaEnvironment *p_env , struct ForwardPower *p_forward_power );
void UnlinkForwardInstanceTreeNode( struct MysqldaEnvironment *p_env , struct ForwardPower *p_forward_power );
void DestroyForwardInstanceTree( struct MysqldaEnvironment *p_env );

int LinkForwardSerialRangeTreeNode( struct MysqldaEnvironment *p_env , struct ForwardPower *p_forward_power );
struct ForwardPower *QueryForwardSerialRangeTreeNode( struct MysqldaEnvironment *p_env , unsigned long serial_no );
struct ForwardPower *TravelForwardSerialRangeTreeNode( struct MysqldaEnvironment *p_env , struct ForwardPower *p_forward_power );
void UnlinkForwardSerialRangeTreeNode( struct MysqldaEnvironment *p_env , struct ForwardPower *p_forward_power );
void DestroyForwardSerialRangeTree( struct MysqldaEnvironment *p_env );

int LinkForwardLibraryTreeNode( struct MysqldaEnvironment *p_env , struct ForwardLibrary *p_forward_library );
struct ForwardLibrary *QueryForwardLibraryTreeNode( struct MysqldaEnvironment *p_env , struct ForwardLibrary *p_forward_library );
void UnlinkForwardLibraryTreeNode( struct MysqldaEnvironment *p_env , struct ForwardLibrary *p_forward_library );
void DestroyForwardLibraryTree( struct MysqldaEnvironment *p_env );

#ifdef __cplusplus
}
#endif

#endif

