#include "mysqlda_in.h"

static int _worker( struct MysqldaEnvironment *p_env )
{
	struct AcceptedSession	*p_accepted_session = NULL ;
	struct ForwardSession	*p_forward_session = NULL ;
	struct epoll_event	event ;
	struct epoll_event	events[ 1024 ] ;
	int			epoll_nfds ;
	int			i ;
	struct epoll_event	*p_event = NULL ;
	int			nret = 0 ;
	
	/* 创建套接字 */
	p_env->listen_session.netaddr.sock = socket( AF_INET , SOCK_STREAM , IPPROTO_TCP ) ;
	if( p_env->listen_session.netaddr.sock == -1 )
	{
		ERRORLOG( "socket failed , errno[%d]" , errno );
		return -1;
	}
	else
	{
		INFOLOG( "socket ok[%d]" , p_env->listen_session.netaddr.sock );
	}
	
	SetHttpNonblock( p_env->listen_session.netaddr.sock );
	SetHttpReuseAddr( p_env->listen_session.netaddr.sock );
	SetHttpNodelay( p_env->listen_session.netaddr.sock , 1 );
	
	/* 绑定套接字到侦听端口 */
	SETNETADDRESS( p_env->listen_session.netaddr )
	nret = bind( p_env->listen_session.netaddr.sock , (struct sockaddr *) & (p_env->listen_session.netaddr.addr) , sizeof(struct sockaddr) ) ;
	if( nret == -1 )
	{
		ERRORLOG( "bind[%s:%d][%d] failed , errno[%d]" , p_env->listen_session.netaddr.ip , p_env->listen_session.netaddr.port , p_env->listen_session.netaddr.sock , errno );
		return -1;
	}
	else
	{
		INFOLOG( "bind[%s:%d][%d] ok" , p_env->listen_session.netaddr.ip , p_env->listen_session.netaddr.port , p_env->listen_session.netaddr.sock );
	}
	
	/* 处于侦听状态了 */
	nret = listen( p_env->listen_session.netaddr.sock , 10240 ) ;
	if( nret == -1 )
	{
		ERRORLOG( "listen[%s:%d][%d] failed , errno[%d]" , p_env->listen_session.netaddr.ip , p_env->listen_session.netaddr.port , p_env->listen_session.netaddr.sock , errno );
		return -1;
	}
	else
	{
		INFOLOG( "listen[%s:%d][%d] ok" , p_env->listen_session.netaddr.ip , p_env->listen_session.netaddr.port , p_env->listen_session.netaddr.sock );
	}
	
	/* 创建epoll池 */
	p_env->epoll_fd = epoll_create( 1024 ) ;
	if( p_env->epoll_fd == -1 )
	{
		ERRORLOG( "epoll_create failed , errno[%d]" , errno );
		return -1;
	}
	else
	{
		INFOLOG( "epoll_create ok" );
	}
	
	/* 加入侦听可读事件到epoll */
	memset( & event , 0x00 , sizeof(struct epoll_event) );
	event.events = EPOLLIN | EPOLLERR ;
	event.data.ptr = & (p_env->listen_session) ;
	nret = epoll_ctl( p_env->epoll_fd , EPOLL_CTL_ADD , p_env->listen_session.netaddr.sock , & event ) ;
	if( nret == -1 )
	{
		ERRORLOG( "epoll_ctl[%d] add listen_session failed , errno[%d]" , p_env->epoll_fd , errno );
		return -1;
	}
	else
	{
		INFOLOG( "epoll_ctl[%d] add listen_session[%d] ok" , p_env->epoll_fd , p_env->listen_session.netaddr.sock );
	}
	
	/* 连接后端数据库 */
	while(1)
	{
		p_forward_session = TravelForwardSessionTreeNode( p_env , p_forward_session ) ;
		if( p_forward_session == NULL )
			break;
		
		p_forward_session->mysql_connection = mysql_init( NULL ) ;
		if( p_forward_session->mysql_connection == NULL )
		{
			ERRORLOG( "mysql_init failed , errno[%d]" , errno );
			return -1;
		}
		
		INFOLOG( "[%s]mysql_real_connect[%s][%d][%s][%s][%s] connecting ..." , p_forward_session->instance , p_forward_session->netaddr.ip , p_forward_session->netaddr.port , p_forward_session->user , p_forward_session->pass , p_forward_session->db );
		if( mysql_real_connect( p_forward_session->mysql_connection , p_forward_session->netaddr.ip , p_forward_session->user , p_forward_session->pass , p_forward_session->db , p_forward_session->netaddr.port , NULL , 0 ) == NULL )
		{
			ERRORLOG( "[%s]mysql_real_connect[%s][%d][%s][%s][%s] failed , mysql_errno[%d][%s]" , p_forward_session->instance , p_forward_session->netaddr.ip , p_forward_session->netaddr.port , p_forward_session->user , p_forward_session->pass , p_forward_session->db , mysql_errno(p_forward_session->mysql_connection) , mysql_error(p_forward_session->mysql_connection) );
			return -2;
		}
		else
		{
			INFOLOG( "[%s]mysql_real_connect[%s][%d][%s][%s][%s] connecting ok" , p_forward_session->instance , p_forward_session->netaddr.ip , p_forward_session->netaddr.port , p_forward_session->user , p_forward_session->pass , p_forward_session->db );
		}
	}
	
	while(1)
	{
		/* 等待epoll事件，或者1秒超时 */
		InfoLog( __FILE__ , __LINE__ , "epoll_wait[%d] ..." , p_env->epoll_fd );
		memset( events , 0x00 , sizeof(events) );
		epoll_nfds = epoll_wait( p_env->epoll_fd , events , sizeof(events)/sizeof(events[0]) , 1000 ) ;
		if( epoll_nfds == -1 )
		{
			if( errno == EINTR )
			{
				InfoLog( __FILE__ , __LINE__ , "epoll_wait[%d] interrupted" , p_env->epoll_fd );
				continue;
			}
			else
			{
				ErrorLog( __FILE__ , __LINE__ , "epoll_wait[%d] failed , errno[%d]" , p_env->epoll_fd , errno );
				return -1;
			}
		}
		else
		{
			InfoLog( __FILE__ , __LINE__ , "epoll_wait[%d] return[%d]events" , p_env->epoll_fd , epoll_nfds );
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
					nret = OnAcceptingSocket( p_env , & (p_env->listen_session) ) ;
					if( nret < 0 )
					{
						FatalLog( __FILE__ , __LINE__ , "OnAcceptingSocket failed[%d]" , nret );
						return -1;
					}
					else if( nret > 0 )
					{
						InfoLog( __FILE__ , __LINE__ , "OnAcceptingSocket return[%d]" , nret );
					}
					else
					{
						DebugLog( __FILE__ , __LINE__ , "OnAcceptingSocket ok" );
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
						nret = OnReceivingAcceptedSocket( p_env , p_accepted_session ) ;
						if( nret < 0 )
						{
							FatalLog( __FILE__ , __LINE__ , "OnReceivingAcceptedSocket failed[%d]" , nret );
							return -1;
						}
						else if( nret > 0 )
						{
							InfoLog( __FILE__ , __LINE__ , "OnReceivingAcceptedSocket return[%d]" , nret );
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
						nret = OnSendingAcceptedSocket( p_env , p_accepted_session ) ;
						if( nret < 0 )
						{
							FatalLog( __FILE__ , __LINE__ , "OnSendingAcceptedSocket failed[%d]" , nret );
							return -1;
						}
						else if( nret > 0 )
						{
							InfoLog( __FILE__ , __LINE__ , "OnSendingAcceptedSocket return[%d]" , nret );
							OnClosingAcceptedSocket( p_env , p_accepted_session );
						}
						else
						{
							DebugLog( __FILE__ , __LINE__ , "OnSendingAcceptedSocket ok" );
						}
					}
					/* 出错事件 */
					else if( ( p_event->events & EPOLLERR ) || ( p_event->events & EPOLLHUP ) )
					{
						FatalLog( __FILE__ , __LINE__ , "accepted session err or hup event[0x%X]" , p_event->events );
						OnClosingAcceptedSocket( p_env , p_accepted_session );
					}
					/* 其它事件 */
					else
					{
						FatalLog( __FILE__ , __LINE__ , "Unknow accepted session event[0x%X]" , p_event->events );
						return -1;
					}
				}
				else if( type == SESSIONTYPE_FORWARDSESSION )
				{
					p_forward_session = (struct ForwardSession *)(p_event->data.ptr) ;
					
					/* 可读事件 */
					if( p_event->events & EPOLLIN )
					{
						nret = OnReceivingForwardSocket( p_env , p_forward_session ) ;
						if( nret < 0 )
						{
							FatalLog( __FILE__ , __LINE__ , "OnReceivingForwardSocket failed[%d]" , nret );
							return -1;
						}
						else if( nret > 0 )
						{
							InfoLog( __FILE__ , __LINE__ , "OnReceivingForwardSocket return[%d]" , nret );
							OnClosingForwardSocket( p_env , p_forward_session );
						}
						else
						{
							DebugLog( __FILE__ , __LINE__ , "OnReceivingForwardSocket ok" );
						}
					}
					/* 可写事件 */
					else if( p_event->events & EPOLLOUT )
					{
						nret = OnSendingForwardSocket( p_env , p_forward_session ) ;
						if( nret < 0 )
						{
							FatalLog( __FILE__ , __LINE__ , "OnSendingForwardSocket failed[%d]" , nret );
							return -1;
						}
						else if( nret > 0 )
						{
							InfoLog( __FILE__ , __LINE__ , "OnSendingForwardSocket return[%d]" , nret );
							OnClosingForwardSocket( p_env , p_forward_session );
						}
						else
						{
							DebugLog( __FILE__ , __LINE__ , "OnSendingForwardSocket ok" );
						}
					}
					/* 出错事件 */
					else if( ( p_event->events & EPOLLERR ) || ( p_event->events & EPOLLHUP ) )
					{
						FatalLog( __FILE__ , __LINE__ , "forward session err or hup event[0x%X]" , p_event->events );
						OnClosingForwardSocket( p_env , p_forward_session );
					}
					/* 其它事件 */
					else
					{
						FatalLog( __FILE__ , __LINE__ , "Unknow forward session event[0x%X]" , p_event->events );
						return -1;
					}
				}
				else
				{
					FatalLog( __FILE__ , __LINE__ , "Unknow session type[%d]" , type );
					return -1;
				}
			}
		}
	}
	
	while(1)
	{
		p_forward_session = TravelForwardSessionTreeNode( p_env , p_forward_session ) ;
		if( p_forward_session == NULL )
			break;
		
		mysql_close( p_forward_session->mysql_connection );
		INFOLOG( "[%s]mysql_close[%s][%d][%s][%s][%s] ok" , p_forward_session->instance , p_forward_session->netaddr.ip , p_forward_session->netaddr.port , p_forward_session->user , p_forward_session->pass , p_forward_session->db );
	}
	
	/* 关闭epoll池 */
	close( p_env->epoll_fd );
	INFOLOG( "close epoll_fd" );
	
	return 0;
}

int worker( void *pv )
{
	struct MysqldaEnvironment	*p_env = (struct MysqldaEnvironment *) pv ;
	
	SetLogFile( "%s/log/mysqlda.log" , getenv("HOME") );
	SetLogLevel( LOGLEVEL_DEBUG );
	
	return _worker( p_env );
}

