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

#include "LOGC.h"
#include "rbtree.h"

#include "my_global.h"
#include "mysql.h"

#include "IDL_mysqlda_conf.dsc.h"

/* 通讯基础信息结构 */
struct NetAddress
{
	char			ip[ sizeof(((mysqlda_conf*)0)->forward[0].ip) + 1 ] ;
	int			port ;
	int			sock ;
	struct sockaddr_in	addr ;
} ;

/* 侦听会话结构 */
struct ListenSession
{
	struct NetAddress	netaddr ;
} ;

#define SESSIONTYPE_ACCEPTEDSESSION	1
#define SESSIONTYPE_FORWARDSESSION	2

/* 客户端连接会话结构 */
struct AcceptedSession
{
	unsigned char		type ;
	struct NetAddress	netaddr ;
	int			status ;
} ;

/* 服务端转发会话结构 */
struct ForwardSession
{
	char			type ;
	char			instance[ sizeof(((mysqlda_conf*)0)->forward[0].instance) ] ;
	struct NetAddress	netaddr ;
	char			user[ sizeof(((mysqlda_conf*)0)->forward[0].user) ] ;
	char			pass[ sizeof(((mysqlda_conf*)0)->forward[0].pass) ] ;
	char			db[ sizeof(((mysqlda_conf*)0)->forward[0].db) ] ;
	unsigned int		power ;

	unsigned long		serial_range_begin ;
	
	MYSQL			*mysql_connection ;
	
	struct rb_node		forward_rbnode ;
} ;

/* 从NetAddress中设置、得到IP、PORT宏 */
#define SETNETADDRESS(_netaddr_) \
	memset( & ((_netaddr_).addr) , 0x00 , sizeof(struct sockaddr_in) ); \
	(_netaddr_).addr.sin_family = AF_INET ; \
	if( (_netaddr_).ip[0] == '\0' ) \
		(_netaddr_).addr.sin_addr.s_addr = INADDR_ANY ; \
	else \
		(_netaddr_).addr.sin_addr.s_addr = inet_addr((_netaddr_).ip) ; \
	(_netaddr_).addr.sin_port = htons( (unsigned short)((_netaddr_).port) );

#define GETNETADDRESS(_netaddr_) \
	strcpy( (_netaddr_).ip , inet_ntoa((_netaddr_).addr.sin_addr) ); \
	(_netaddr_).port = (int)ntohs( (_netaddr_).addr.sin_port ) ;

#define SetHttpReuseAddr(_sock_) \
	{ \
		int	onoff = 1 ; \
		setsockopt( _sock_ , SOL_SOCKET , SO_REUSEADDR , (void *) & onoff , sizeof(int) ); \
	}

#if ( defined __linux ) || ( defined __unix )
#define SetHttpNonblock(_sock_) \
	{ \
		int	opts; \
		opts = fcntl( _sock_ , F_GETFL ); \
		opts |= O_NONBLOCK ; \
		fcntl( _sock_ , F_SETFL , opts ); \
	}
#define SetHttpBlock(_sock_) \
	{ \
		int	opts; \
		opts = fcntl( _sock_ , F_GETFL ); \
		opts &= ~O_NONBLOCK ; \
		fcntl( _sock_ , F_SETFL , opts ); \
	}
#elif ( defined _WIN32 )
#define SetHttpNonblock(_sock_) \
	{ \
		u_long	mode = 1 ; \
		ioctlsocket( _sock_ , FIONBIO , & mode ); \
	}
#define SetHttpBlock(_sock_) \
	{ \
		u_long	mode = 0 ; \
		ioctlsocket( _sock_ , FIONBIO , & mode ); \
	}
#endif

#define SetHttpNodelay(_sock_,_onoff_) \
	{ \
		int	onoff = _onoff_ ; \
		setsockopt( _sock_ , IPPROTO_TCP , TCP_NODELAY , (void*) & onoff , sizeof(int) ); \
	}

#define SetHttpLinger(_sock_,_linger_) \
	{ \
		struct linger   lg; \
		if( _linger_ >= 0 ) \
		{ \
			lg.l_onoff = 1 ; \
			lg.l_linger = _linger_ ; \
		} \
		else \
		{ \
			lg.l_onoff = 0 ; \
			lg.l_linger = 0 ; \
		} \
		setsockopt( _sock_ , SOL_SOCKET , SO_LINGER , (void*) & lg , sizeof(struct linger) ); \
	}

#if ( defined __linux ) || ( defined __unix )
#define SetHttpCloseExec(_sock_) \
	{ \
		int	val ; \
		val = fcntl( _sock_ , F_GETFD ) ; \
		val |= FD_CLOEXEC ; \
		fcntl( _sock_ , F_SETFD , val ); \
	}
#elif ( defined _WIN32 )
#define SetHttpCloseExec(_sock_)
#endif

struct MysqldaEnvironment
{
	char			*config_filename ;
	int			no_daemon_flag ;
	char			*action ;
	
	struct rb_root		forward_rbtree ;
	unsigned long		total_power ;
	
	struct ListenSession	listen_session ;
	int			epoll_fd ;
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
 * rbtree
 */

int LinkForwardSessionTreeNode( struct MysqldaEnvironment *p_env , struct ForwardSession *p_forward_session );
void UnlinkForwardSessionTreeNode( struct MysqldaEnvironment *p_env , struct ForwardSession *p_forward_session );
void DestroyTcpdaemonAcceptedSessionTree( struct MysqldaEnvironment *p_env );
struct ForwardSession *QueryForwardSessionRangeTreeNode( struct MysqldaEnvironment *p_env , unsigned long serial_no );
struct ForwardSession *TravelForwardSessionTreeNode( struct MysqldaEnvironment *p_env , struct ForwardSession *p_forward_session );

#endif

