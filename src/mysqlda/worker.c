#include "mysqlda_in.h"

static int _worker( struct MysqldaEnvironment *p_env )
{
	struct AcceptedSession	*p_accepted_session = NULL ;
	struct ForwardInstance	*p_forward_instance = NULL ;
	MYSQL			*mysql_connection = NULL ;
	int			sock ;
	int			len , length ;
	struct ForwardServer	*p_forward_server = NULL ;
	struct ForwardSession	*p_forward_session = NULL ;
	struct ForwardSession	*p_unused_forward_session = NULL ;
	struct ForwardSession	*p_next_unused_forward_session = NULL ;
	struct epoll_event	event ;
	struct epoll_event	events[ 1024 ] ;
	int			epoll_nfds ;
	int			i ;
	struct epoll_event	*p_event = NULL ;
	char			pipe_data ;
	int			exit_flag ;
	time_t			now_timestamp ;
	
	int			nret = 0 ;
	
	/* 设置信号灯 */
	signal( SIGTERM , SIG_IGN );
	signal( SIGINT , SIG_IGN );
	
	/* 创建epoll池 */
	p_env->epoll_fd = epoll_create( 1024 ) ;
	if( p_env->epoll_fd == -1 )
	{
		ERRORLOG( "epoll_create failed , errno[%d]" , errno );
		return -1;
	}
	else
	{
		INFOLOG( "epoll_create ok , #%d#" , p_env->epoll_fd );
	}
	
	/* 加入存活管道可读事件到epoll */
	memset( & event , 0x00 , sizeof(struct epoll_event) );
	event.events = EPOLLIN | EPOLLERR ;
	event.data.ptr = & (p_env->alive_pipe_session) ;
	nret = epoll_ctl( p_env->epoll_fd , EPOLL_CTL_ADD , p_env->alive_pipe_session.alive_pipe[0] , & event ) ;
	if( nret == -1 )
	{
		ERRORLOG( "epoll_ctl #%d# add alive_pipe_session failed , errno[%d]" , p_env->epoll_fd , errno );
		return -1;
	}
	else
	{
		INFOLOG( "epoll_ctl #%d# add alive_pipe_session #%d# ok" , p_env->epoll_fd , p_env->alive_pipe_session.alive_pipe[0] );
	}
	
	/* 创建套接字 */
	p_env->listen_session.netaddr.sock = socket( AF_INET , SOCK_STREAM , IPPROTO_TCP ) ;
	if( p_env->listen_session.netaddr.sock == -1 )
	{
		ERRORLOG( "socket failed , errno[%d]" , errno );
		return -1;
	}
	else
	{
		INFOLOG( "socket ok , #%d#" , p_env->listen_session.netaddr.sock );
	}
	
	SetHttpNonblock( p_env->listen_session.netaddr.sock );
	SetHttpReuseAddr( p_env->listen_session.netaddr.sock );
	SetHttpNodelay( p_env->listen_session.netaddr.sock , 1 );
	
	/* 绑定套接字到侦听端口 */
	SETNETADDRESS( p_env->listen_session.netaddr )
	nret = bind( p_env->listen_session.netaddr.sock , (struct sockaddr *) & (p_env->listen_session.netaddr.addr) , sizeof(struct sockaddr) ) ;
	if( nret == -1 )
	{
		ERRORLOG( "bind[%s:%d] #%d# failed , errno[%d]" , p_env->listen_session.netaddr.ip , p_env->listen_session.netaddr.port , p_env->listen_session.netaddr.sock , errno );
		return -1;
	}
	else
	{
		INFOLOG( "bind[%s:%d] #%d# ok" , p_env->listen_session.netaddr.ip , p_env->listen_session.netaddr.port , p_env->listen_session.netaddr.sock );
	}
	
	/* 处于侦听状态了 */
	nret = listen( p_env->listen_session.netaddr.sock , 10240 ) ;
	if( nret == -1 )
	{
		ERRORLOG( "listen[%s:%d] #%d# failed , errno[%d]" , p_env->listen_session.netaddr.ip , p_env->listen_session.netaddr.port , p_env->listen_session.netaddr.sock , errno );
		return -1;
	}
	else
	{
		INFOLOG( "listen[%s:%d] #%d# ok" , p_env->listen_session.netaddr.ip , p_env->listen_session.netaddr.port , p_env->listen_session.netaddr.sock );
	}
	
	/* 加入侦听可读事件到epoll */
	memset( & event , 0x00 , sizeof(struct epoll_event) );
	event.events = EPOLLIN | EPOLLERR ;
	event.data.ptr = & (p_env->listen_session) ;
	nret = epoll_ctl( p_env->epoll_fd , EPOLL_CTL_ADD , p_env->listen_session.netaddr.sock , & event ) ;
	if( nret == -1 )
	{
		ERRORLOG( "epoll_ctl #%d# add listen_session failed , errno[%d]" , p_env->epoll_fd , errno );
		return -1;
	}
	else
	{
		INFOLOG( "epoll_ctl #%d# add listen_session #%d# ok" , p_env->epoll_fd , p_env->listen_session.netaddr.sock );
	}
	
	/* 检查所有后端数据库连接 */
	while(1)
	{
		p_forward_instance = TravelForwardSerialRangeTreeNode( p_env , p_forward_instance ) ;
		if( p_forward_instance == NULL )
			break;
		
		mysql_connection = mysql_init( NULL ) ;
		if( mysql_connection == NULL )
		{
			ERRORLOG( "mysql_init failed , errno[%d]" , errno );
			return -1;
		}
		
		p_forward_server = lk_list_first_entry( & (p_forward_instance->forward_server_list) , struct ForwardServer , forward_server_listnode ) ;
		INFOLOG( "[%s]mysql_real_connect[%s][%d][%s][%s][%s] connecting ..." , p_forward_instance->instance , p_forward_server->netaddr.ip , p_forward_server->netaddr.port , p_env->user , p_env->pass , p_env->db );
		if( mysql_real_connect( mysql_connection , p_forward_server->netaddr.ip , p_env->user , p_env->pass , p_env->db , p_forward_server->netaddr.port , NULL , 0 ) == NULL )
		{
			ERRORLOG( "[%s]mysql_real_connect[%s][%d][%s][%s][%s] failed , mysql_errno[%d][%s]" , p_forward_instance->instance , p_forward_server->netaddr.ip , p_forward_server->netaddr.port , p_env->user , p_env->pass , p_env->db , mysql_errno(mysql_connection) , mysql_error(mysql_connection) );
			mysql_close( mysql_connection );
			return -1;
		}
		else
		{
			INFOLOG( "[%s]mysql_real_connect[%s][%d][%s][%s][%s] connecting ok" , p_forward_instance->instance , p_forward_server->netaddr.ip , p_forward_server->netaddr.port , p_env->user , p_env->pass , p_env->db );
		}
		
		INFOLOG( "[%s]mysql_close[%s][%d] ok" , p_forward_instance->instance , p_forward_server->netaddr.ip , p_forward_server->netaddr.port );
		mysql_close( mysql_connection );
	}
	
	/* 骗到握手信息头 */
	sock = socket( AF_INET , SOCK_STREAM , IPPROTO_TCP ) ;
	if( sock == -1 )
	{
		ERRORLOG( "socket failed , errno[%d]" , errno );
		return -1;
	}
	
	p_forward_instance = TravelForwardInstanceTreeNode( p_env , NULL ) ;
	p_forward_server = lk_list_first_entry( & (p_forward_instance->forward_server_list) , struct ForwardServer , forward_server_listnode ) ;
	nret = connect( sock , (struct sockaddr *) & (p_forward_server->netaddr.addr) , sizeof(struct sockaddr) ) ;
	if( nret == -1 )
	{
		ERRORLOG( "connect failed , errno[%d]" , errno );
		return -1;
	}
	
	length = 0 ;
	while( length < 4 )
	{
		len = recv( sock , p_env->handshake_head+length , 4-length , 0 ) ;
		if( len == -1 )
		{
			ERRORLOG( "recv failed , errno[%d]" , errno );
			return -1;
		}
		else if( len == 0 )
		{
			ERRORLOG( "recv close , errno[%d]" , errno );
			return -1;
		}
		
		length += len ;
	}
	DEBUGHEXLOG( p_env->handshake_head , 4 , "handshake_head" );
	
	p_env->handshake_message_length = MYSQL_COMMLEN( p_env->handshake_head ) ;
	p_env->handshake_message = (char*)malloc( 4+p_env->handshake_message_length ) ;
	if( p_env->handshake_message == NULL )
	{
		ERRORLOG( "malloc failed , errno[%d]" , errno );
		return -1;
	}
	memcpy( p_env->handshake_message , p_env->handshake_head , 4 );
	
	length = 0 ;
	while( length < p_env->handshake_message_length )
	{
		len = recv( sock , p_env->handshake_message+4+length , p_env->handshake_message_length-length , 0 ) ;
		if( len == -1 )
		{
			ERRORLOG( "recv failed , errno[%d]" , errno );
			return -1;
		}
		else if( len == 0 )
		{
			ERRORLOG( "recv close , errno[%d]" , errno );
			return -1;
		}
		
		length += len ;
	}
	DEBUGHEXLOG( p_env->handshake_message , 4+p_env->handshake_message_length , "handshake_message" );
	
	/* 子进程主循环 */
	exit_flag = 0 ;
	while( ! exit_flag )
	{
		/* 等待epoll事件，或者1秒超时 */
		InfoLog( __FILE__ , __LINE__ , "epoll_wait #%d# ..." , p_env->epoll_fd );
		memset( events , 0x00 , sizeof(events) );
		epoll_nfds = epoll_wait( p_env->epoll_fd , events , sizeof(events)/sizeof(events[0]) , 1000 ) ;
		if( epoll_nfds == -1 )
		{
			if( errno == EINTR )
			{
				INFOLOG( "epoll_wait #%d# interrupted" , p_env->epoll_fd );
				break;
			}
			else
			{
				ERRORLOG( "epoll_wait #%d# failed , errno[%d]" , p_env->epoll_fd , errno );
				return -1;
			}
		}
		else
		{
			INFOLOG( "epoll_wait #%d# return[%d]events" , p_env->epoll_fd , epoll_nfds );
		}
		
		/* 处理所有事件 */
		for( i = 0 , p_event = events ; i < epoll_nfds ; i++ , p_event++ )
		{
			/* 侦听套接字事件 */
			if( p_event->data.ptr == & (p_env->listen_session) )
			{
				/* 可读事件 */
				if( p_event->events & EPOLLIN )
				{
					INFOLOG( "OnAcceptingSocket ..." );
					nret = OnAcceptingSocket( p_env , & (p_env->listen_session) ) ;
					if( nret < 0 )
					{
						FATALLOG( "OnAcceptingSocket failed[%d]" , nret );
						return -1;
					}
					else if( nret > 0 )
					{
						INFOLOG( "OnAcceptingSocket return[%d]" , nret );
					}
					else
					{
						DEBUGLOG( "OnAcceptingSocket ok" );
					}
				}
				/* 出错事件 */
				else if( ( p_event->events & EPOLLERR ) || ( p_event->events & EPOLLHUP ) )
				{
					FatalLog( __FILE__ , __LINE__ , "listen session err or hup event[0x%X]" , p_event->events );
					return -1;
				}
				/* 其它事件 */
				else
				{
					FatalLog( __FILE__ , __LINE__ , "Unknow listen session event[0x%X]" , p_event->events );
					return -1;
				}
			}
			/* 存活管道事件 */
			else if( p_event->data.ptr == & (p_env->alive_pipe_session) )
			{
				INFOLOG( "read alive_pipe ..." );
				pipe_data = 0 ;
				nret = read( p_env->alive_pipe_session.alive_pipe[0] , & pipe_data , 1 ) ;
				INFOLOG( "read alive_pipe ok[%d] , pipe_data[%c]" , nret , pipe_data );
				if( nret == 1 && pipe_data == 'R' )
				{
					nret = ReloadConfig( p_env ) ;
					if( nret )
					{
						ERRORLOG( "ReloadConfig failed[%d]" , nret );
					}
					else
					{
						INFOLOG( "ReloadConfig ok" );
					}
				}
				else if( nret == 0 )
				{
					exit_flag = 1 ;
				}
			}
			/* 其它事件，即客户端连接会话事件 */
			else
			{
				int		type = ((char*)(p_event->data.ptr))[0] ;
				
				if( type == SESSIONTYPE_ACCEPTEDSESSION )
				{
					p_accepted_session = (struct AcceptedSession *)(p_event->data.ptr) ;
					
					/* 可读事件 */
					if( p_event->events & EPOLLIN )
					{
						INFOLOG( "OnReceivingAcceptedSocket ..." );
						nret = OnReceivingAcceptedSocket( p_env , p_accepted_session ) ;
						if( nret < 0 )
						{
							FATALLOG( "OnReceivingAcceptedSocket failed[%d]" , nret );
							return -1;
						}
						else if( nret > 0 )
						{
							INFOLOG( "OnReceivingAcceptedSocket return[%d]" , nret );
							OnClosingAcceptedSocket( p_env , p_accepted_session );
						}
						else
						{
							DebugLog( __FILE__ , __LINE__ , "OnReceivingAcceptedSocket ok" );
						}
					}
					/* 可写事件 */
					else if( p_event->events & EPOLLOUT )
					{
						INFOLOG( "OnSendingAcceptedSocket ..." );
						nret = OnSendingAcceptedSocket( p_env , p_accepted_session ) ;
						if( nret < 0 )
						{
							FATALLOG( "OnSendingAcceptedSocket failed[%d]" , nret );
							return -1;
						}
						else if( nret > 0 )
						{
							INFOLOG( "OnSendingAcceptedSocket return[%d]" , nret );
							OnClosingAcceptedSocket( p_env , p_accepted_session );
						}
						else
						{
							DEBUGLOG( "OnSendingAcceptedSocket ok" );
						}
					}
					/* 出错事件 */
					else if( ( p_event->events & EPOLLERR ) || ( p_event->events & EPOLLHUP ) )
					{
						FATALLOG( "accepted session err or hup event[0x%X]" , p_event->events );
						OnClosingAcceptedSocket( p_env , p_accepted_session );
					}
					/* 其它事件 */
					else
					{
						FATALLOG( "Unknow accepted session event[0x%X]" , p_event->events );
						return -1;
					}
				}
				else if( type == SESSIONTYPE_FORWARDSESSION )
				{
					p_forward_session = (struct ForwardSession *)(p_event->data.ptr) ;
					
					/* 可读事件 */
					if( p_event->events & EPOLLIN )
					{
						INFOLOG( "OnReceivingForwardSocket ..." );
						nret = OnReceivingForwardSocket( p_env , p_forward_session ) ;
						if( nret < 0 )
						{
							FATALLOG( "OnReceivingForwardSocket failed[%d]" , nret );
							return -1;
						}
						else if( nret > 0 )
						{
							INFOLOG( "OnReceivingForwardSocket return[%d]" , nret );
							OnClosingForwardSocket( p_env , p_forward_session );
						}
						else
						{
							DEBUGLOG( "OnReceivingForwardSocket ok" );
						}
					}
					/* 可写事件 */
					else if( p_event->events & EPOLLOUT )
					{
						INFOLOG( "OnSendingForwardSocket ..." );
						nret = OnSendingForwardSocket( p_env , p_forward_session ) ;
						if( nret < 0 )
						{
							FATALLOG( "OnSendingForwardSocket failed[%d]" , nret );
							return -1;
						}
						else if( nret > 0 )
						{
							INFOLOG( "OnSendingForwardSocket return[%d]" , nret );
							OnClosingForwardSocket( p_env , p_forward_session );
						}
						else
						{
							DEBUGLOG( "OnSendingForwardSocket ok" );
						}
					}
					/* 出错事件 */
					else if( ( p_event->events & EPOLLERR ) || ( p_event->events & EPOLLHUP ) )
					{
						FATALLOG( "forward session err or hup event[0x%X]" , p_event->events );
						OnClosingForwardSocket( p_env , p_forward_session );
					}
					/* 其它事件 */
					else
					{
						FATALLOG( "Unknow forward session event[0x%X]" , p_event->events );
						return -1;
					}
				}
				else
				{
					FATALLOG( "Unknow session type[%d]" , type );
					return -1;
				}
			}
		}
		
		/* 清理超时的 服务端转发会话 缓存会话 */
		now_timestamp = time(NULL) ;
		p_forward_instance = NULL ;
		while(1)
		{
			p_forward_instance = TravelForwardSerialRangeTreeNode( p_env , p_forward_instance ) ;
			if( p_forward_instance == NULL )
				break;
			
			lk_list_for_each_entry( p_forward_server , & (p_forward_instance->forward_server_list) , struct ForwardServer , forward_server_listnode )
			{
				lk_list_for_each_entry_safe( p_unused_forward_session , p_next_unused_forward_session , & (p_forward_server->unused_forward_session_list) , struct ForwardSession , unused_forward_session_listnode )
				{
					if( now_timestamp >= p_unused_forward_session->close_unused_forward_session_timestamp )
					{
						INFOLOG( "[%s] #%d# mysql_close[%s][%d] ok" , p_unused_forward_session->p_forward_instance->instance , p_unused_forward_session->mysql_connection->net.fd , p_unused_forward_session->p_forward_server->netaddr.ip , p_unused_forward_session->p_forward_server->netaddr.port );
						mysql_close( p_unused_forward_session->mysql_connection );
						p_unused_forward_session->mysql_connection = NULL ;
						
						lk_list_del( & (p_unused_forward_session->unused_forward_session_listnode) );
						
						free( p_unused_forward_session );
					}
				}
			}
		}
	}
	
	/* 释放握手信息 */
	free( p_env->handshake_message );
	
	/* 关闭epoll池 */
	close( p_env->epoll_fd );
	INFOLOG( "close epoll_fd" );
	
	return 0;
}

int worker( void *pv )
{
	struct MysqldaEnvironment	*p_env = (struct MysqldaEnvironment *) pv ;
	
	int				nret = 0 ;
	
	SetLogPid();
	
	/* 装载配置 */
	nret = LoadConfig( p_env ) ;
	if( nret )
	{
		UnloadConfig( p_env ) ;
		return 1;
	}
	
	/* 进入子进程主函数 */
	nret = _worker( p_env ) ;
	
	/* 卸载配置 */
	UnloadConfig( p_env );
	
	INFOLOG( "worker exit ..." );
	
	return -nret;
}

