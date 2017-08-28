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
#include <dirent.h>

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

#define SESSIONTYPE_ACCEPTEDSESSION	1 /* 客户端连接会话类型 */
#define SESSIONTYPE_FORWARDSESSION	2 /* 服务端转发会话类型 */

#define SESSIONSTATUS_BEFORE_SENDING_HANDSHAKE						1 /* 转发端向客户端发送握手信息前 */
#define SESSIONSTATUS_AFTER_SENDING_HANDSHAKE_AND_BEFORE_RECEIVING_AUTHENTICATION	2 /* 转发端向客户端发送握手信息后，接收客户端认证信息前 */
#define SESSIONSTATUS_AFTER_SENDING_AUTH_FAIL_AND_BEFORE_FORWARDING			3 /* 转发端发送客户端认证失败信息后 */
#define SESSIONSTATUS_AFTER_SENDING_AUTH_OK_AND_BEFORE_RECEIVING_SELECT_LIBRARY		4 /* 转发端发送客户端认证成功信息后，接收选择库前 */
#define SESSIONSTATUS_AFTER_SENDING_SELECT_LIBRARY_AND_BEFORE_FORDWARD			5 /* 接收选择库后，全双工互转发前 */
#define SESSIONSTATUS_FORWARDING							7 /* 全双工互转发 */

#define MYSQL_COMMLEN(_cl_)	(((unsigned char)((_cl_)[0])+(unsigned char)((_cl_)[1])*256+(unsigned char)((_cl_)[2])*256*256))
#define MYSQL_OPTIONS_2(_cl_)	(((unsigned char)((_cl_)[0])+(unsigned char)((_cl_)[1])*256))

/* 存活管道会话 结构 */
struct AlivePipeSession
{
	int			alive_pipe[ 2 ] ; /* 管道描述字 */
} ;

/* 客户端连接会话 结构 */
#define MAXLEN_ACCEPTED_SESSION_COMM_BUFFER	(3+1+(2<<24)) /* 通讯数据缓冲区大小（待优化） */

struct ForwardSession ;
struct AcceptedSession
{
	unsigned char		type ; /* 会话类型 */
	int			status ; /* 会话连接状态 */
	
	struct NetAddress	netaddr ; /* 网络地址 */
	
	char			*comm_buffer ; /* 通讯缓冲区基地址 */
	int			fill_len ; /* 数据填充长度 */
	int			process_len ; /* 数据处理长度 */
	int			comm_body_len ; /* mysql协议体长度 */
	
	char			random_data[ 20 ] ; /* mysql认证随机数 */
	
	struct ForwardSession	*p_pair_forward_session ; /* 对应 服务端转发会话 */
} ;

/* 服务端转发会话 结构 */
struct ForwardInstance ;
struct ForwardSession
{
	char			type ; /* 会话类型 */
	
	struct AcceptedSession	*p_pair_accepted_session ; /* 对应 客户端连接会话 */
	struct ForwardInstance	*p_forward_instance ; /* 对应 服务端转发库 */
	struct ForwardServer	*p_forward_server ; /* 对应 服务端转发服务器 */
	
	MYSQL			*mysql_connection ; /* mysql连接句柄 */
	
	struct lk_list_head	forward_session_listnode ; /* 服务端转发会话 工作会话 链表节点 */
	time_t			close_unused_forward_session_timestamp ; /* 服务端转发会话 缓存会话 未来清理时间 */
	struct lk_list_head	unused_forward_session_listnode ; /* 服务端转发会话 缓存会话 链表节点 */
} ;

/* 服务端转发服务器信息 结构 */
struct ForwardServer
{
	struct NetAddress	netaddr ; /* 网络地址 */
	
	struct lk_list_head	forward_session_list ; /* 服务端转发会话 工作会话 链表 */
	struct lk_list_head	unused_forward_session_list ; /* 服务端转发会话 缓存会话 链表 */
	
	struct lk_list_head	forward_server_listnode ; /* 链表节点 */
} ;

/* 服务端转发库 结构 */
struct ForwardInstance
{
	char			instance[ sizeof(((mysqlda_conf*)0)->forwards[0].instance) ] ; /* 实例名 */
	struct rb_node		forward_instance_rbnode ; /* 实例名 树节点 */
	
	struct lk_list_head	forward_server_list ; /* 服务端转发服务器信息 链表 */
	
	unsigned long		power ; /* 权重 */
	
	unsigned long		serial_range_begin ; /* 开始序号 */
	struct rb_node		forward_serial_range_rbnode ; /* 开始序号 树节点 */
} ;

/* 服务端转发规则 结构 */
#define MAXLEN_LIBRARY		64

struct ForwardLibrary
{
	char			library[ MAXLEN_LIBRARY + 1 ] ; /* 核心业务对象 值 */
	struct ForwardInstance	*p_forward_instance ; /* 对应 服务端转发库 */
	
	struct rb_node		forward_library_rbnode ; /* 树节点 */
} ;

/* 服务端转发关联对象 结构 */
#define MAXLEN_CORRELOBJECT	64

struct ForwardCorrelObject
{
	char			correl_object[ MAXLEN_CORRELOBJECT + 1 ] ; /* 核心业务关联对象名 */
	struct ForwardLibrary	*p_forward_library ; /* 服务端转发规则 */
	
	struct rb_node		forward_correl_object_rbnode ; /* 树节点 */
} ;

#define MAXLEN_CORRELOBJECT_CLASS	64

/* 服务端转发关联对象类 结构 */
struct ForwardCorrelObjectClass
{
	char			correl_object_class[ MAXLEN_CORRELOBJECT_CLASS + 1 ] ; /* 核心业务关联对象类 */
	
	struct rb_root		forward_correl_object_rbtree ; /* 服务端转发关联对象 类-对象 关系树 */
	
	struct rb_node		forward_correl_object_class_rbnode ; /* 树节点 */
} ;

/* MySQL分布式代理 环境结构 */
struct MysqldaEnvironment
{
	char			*config_filename ; /* 配置路径文件 */
	char			*save_filename ; /* 转发规则历史 持久化 路径文件 */
	int			no_daemon_flag ; /* 是否以守护进程模式运行 */
	char			*action ; /* 行为方式 */
	
	char			user[ sizeof(((mysqlda_conf*)0)->auth.user) ] ; /* 数据库连接用户名 */
	char			pass[ sizeof(((mysqlda_conf*)0)->auth.pass) ] ; /* 数据库连接用户密码 */
	char			db[ sizeof(((mysqlda_conf*)0)->auth.db) ] ; /* 数据库名 */
	time_t			unused_forward_session_timeout ; /* 服务端转发会话缓存会话池超时时间 */
	
	int			handshake_request_message_length ; /* 模拟握手信息体长 */
	char			*handshake_request_message ; /* 模拟握手信息体 */
	
	int			select_version_comment_response_message_length ; /* 模拟查询版本号信息体长 */
	char			*select_version_comment_response_message ; /* 模拟查询版本号信息体 */
	int			select_version_comment_response_message2_length ; /* 模拟查询版本号信息2体长 */
	char			*select_version_comment_response_message2 ; /* 模拟查询版本号信息2体 */
	int			select_version_comment_response_message3_length ; /* 模拟查询版本号信息3体长 */
	char			*select_version_comment_response_message3 ; /* 模拟查询版本号信息3体 */
	int			select_version_comment_response_message4_length ; /* 模拟查询版本号信息4体长 */
	char			*select_version_comment_response_message4 ; /* 模拟查询版本号信息4体 */
	int			select_version_comment_response_message5_length ; /* 模拟查询版本号信息5体长 */
	char			*select_version_comment_response_message5 ; /* 模拟查询版本号信息5体 */
	
	struct rb_root		forward_instance_rbtree ; /* 服务端转发库 树（实例名为排序索引） */
	struct rb_root		forward_serial_range_rbtree ; /* 服务端转发库 树（开始序号为排序索引） */
	unsigned long		total_power ; /* 总权重 */
	
	struct AlivePipeSession	alive_pipe_session ; /* 存活管道会话 */
	struct ListenSession	listen_session ; /* 侦听会话 */
	
	int			epoll_fd ; /* epoll描述字 */
	
	struct rb_root		forward_library_rbtree ; /* 服务端转发规则 历史树 */
	struct rb_root		forward_correl_object_class_rbtree ; /* 服务端转发关联对象类 历史树 */
} ;

/*
 * util
 */

int WriteEntireFile( char *pathfilename , char *file_content , int file_len );
char *StrdupEntireFile( char *pathfilename , int *p_file_len );

int BindDaemonServer( int (* ServerMain)( void *pv ) , void *pv , int close_flag );

void GenerateRandomDataWithoutNull( char *data , int data_len );

unsigned long CalcHash( char *str , int str_len );

char *wordncasecmp( char *s1 , char *s2 , size_t n );

/*
 * config
 */

int InitConfigFile( struct MysqldaEnvironment *p_env );

void IncreaseForwardInstanceTreeNodePower( struct MysqldaEnvironment *p_env , struct ForwardInstance *p_this_forward_instance );

int LoadConfig( struct MysqldaEnvironment *p_env );
int ReloadConfig( struct MysqldaEnvironment *p_env );
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
int FormatSelectVersionCommentResponse( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session );

int SelectDatabaseLibrary( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session , char *library , int library_len );
int SetDatabaseCorrelObject( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session , char *correl_object_class , int correl_object_class_len , char *correl_object_name , int correl_object_name_len , char *library , int library_len );
int SelectDatabaseLibraryByCorrelObject( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session , char *correl_object_class , int correl_object_class_len , char *correl_object_name , int correl_object_name_len );

/*
 * rbtree
 */

int LinkForwardInstanceTreeNode( struct MysqldaEnvironment *p_env , struct ForwardInstance *p_forward_instance );
struct ForwardInstance *QueryForwardInstanceTreeNode( struct MysqldaEnvironment *p_env , struct ForwardInstance *p_forward_instance );
struct ForwardInstance *TravelForwardInstanceTreeNode( struct MysqldaEnvironment *p_env , struct ForwardInstance *p_forward_instance );
void UnlinkForwardInstanceTreeNode( struct MysqldaEnvironment *p_env , struct ForwardInstance *p_forward_instance );
void DestroyForwardInstanceTree( struct MysqldaEnvironment *p_env );

int LinkForwardSerialRangeTreeNode( struct MysqldaEnvironment *p_env , struct ForwardInstance *p_forward_instance );
struct ForwardInstance *QueryForwardSerialRangeTreeNode( struct MysqldaEnvironment *p_env , unsigned long serial_no );
struct ForwardInstance *TravelForwardSerialRangeTreeNode( struct MysqldaEnvironment *p_env , struct ForwardInstance *p_forward_instance );
void UnlinkForwardSerialRangeTreeNode( struct MysqldaEnvironment *p_env , struct ForwardInstance *p_forward_instance );
void DestroyForwardSerialRangeTree( struct MysqldaEnvironment *p_env );

int LinkForwardLibraryTreeNode( struct MysqldaEnvironment *p_env , struct ForwardLibrary *p_forward_library );
struct ForwardLibrary *QueryForwardLibraryTreeNode( struct MysqldaEnvironment *p_env , struct ForwardLibrary *p_forward_library );
void UnlinkForwardLibraryTreeNode( struct MysqldaEnvironment *p_env , struct ForwardLibrary *p_forward_library );
void DestroyForwardLibraryTree( struct MysqldaEnvironment *p_env );

int LinkForwardCorrelObjectClassTreeNode( struct MysqldaEnvironment *p_env , struct ForwardCorrelObjectClass *p_forward_correl_object_class );
struct ForwardCorrelObjectClass *QueryForwardCorrelObjectClassTreeNode( struct MysqldaEnvironment *p_env , struct ForwardCorrelObjectClass *p_forward_correl_object_class );
void UnlinkForwardCorrelObjectClassTreeNode( struct MysqldaEnvironment *p_env , struct ForwardCorrelObjectClass *p_forward_correl_object_class );
void DestroyForwardCorrelObjectClassTree( struct MysqldaEnvironment *p_env );

int LinkForwardCorrelObjectTreeNode( struct ForwardCorrelObjectClass *p_forward_correl_object_class , struct ForwardCorrelObject *p_forward_correl_object );
struct ForwardCorrelObject *QueryForwardCorrelObjectTreeNode( struct ForwardCorrelObjectClass *p_forward_correl_object_class , struct ForwardCorrelObject *p_forward_correl_object );
void UnlinkForwardCorrelObjectTreeNode( struct ForwardCorrelObjectClass *p_forward_correl_object_class , struct ForwardCorrelObject *p_forward_correl_object );
void DestroyForwardCorrelObjectTree( struct ForwardCorrelObjectClass *p_forward_correl_object_class );

#ifdef __cplusplus
}
#endif

#endif

