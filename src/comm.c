#include "mysqlda_in.h"

#define AddAcceptedSessionEpollOutput(_p_env_,_p_accepted_session_) \
	{ \
		struct epoll_event	event ; \
		\
		memset( & event , 0x00 , sizeof(struct epoll_event) ); \
		event.events = EPOLLOUT | EPOLLERR ; \
		event.data.ptr = (_p_accepted_session_) ; \
		nret = epoll_ctl( (_p_env_)->epoll_fd , EPOLL_CTL_ADD , (_p_accepted_session_)->netaddr.sock , & event ) ; \
		if( nret == -1 ) \
		{ \
			ERRORLOG( "epoll_ctl #%d# add #%d# accepted session EPOLLOUT failed , errno[%d]" , (_p_env_)->epoll_fd , (_p_accepted_session_)->netaddr.sock , errno ); \
			return -1; \
		} \
		else \
		{ \
			DEBUGLOG( "epoll_ctl #%d# add #%d# accepted session EPOLLOUT ok" , (_p_env_)->epoll_fd , (_p_accepted_session_)->netaddr.sock ); \
		} \
	} \

#define ModifyAcceptedSessionEpollInput(_p_env_,_p_accepted_session_) \
	{ \
		struct epoll_event	event ; \
		\
		memset( & event , 0x00 , sizeof(struct epoll_event) ); \
		event.events = EPOLLIN | EPOLLERR ; \
		event.data.ptr = (_p_accepted_session_) ; \
		nret = epoll_ctl( (_p_env_)->epoll_fd , EPOLL_CTL_MOD , (_p_accepted_session_)->netaddr.sock , & event ) ; \
		if( nret == -1 ) \
		{ \
			ERRORLOG( "epoll_ctl #%d# modify #%d# accepted session EPOLLIN failed , errno[%d]" , (_p_env_)->epoll_fd , (_p_accepted_session_)->netaddr.sock , errno ); \
			return -1; \
		} \
		else \
		{ \
			DEBUGLOG( "epoll_ctl #%d# modify #%d# accepted session EPOLLIN ok" , (_p_env_)->epoll_fd , (_p_accepted_session_)->netaddr.sock ); \
		} \
	} \

#define ModifyAcceptedSessionEpollOutput(_p_env_,_p_accepted_session_) \
	{ \
		struct epoll_event	event ; \
		\
		memset( & event , 0x00 , sizeof(struct epoll_event) ); \
		event.events = EPOLLOUT | EPOLLERR ; \
		event.data.ptr = (_p_accepted_session_) ; \
		nret = epoll_ctl( (_p_env_)->epoll_fd , EPOLL_CTL_MOD , (_p_accepted_session_)->netaddr.sock , & event ) ; \
		if( nret == -1 ) \
		{ \
			ERRORLOG( "epoll_ctl #%d# modify #%d# accepted session EPOLLOUT failed , errno[%d]" , (_p_env_)->epoll_fd , (_p_accepted_session_)->netaddr.sock , errno ); \
			return -1; \
		} \
		else \
		{ \
			DEBUGLOG( "epoll_ctl #%d# modify #%d# accepted session EPOLLOUT ok" , (_p_env_)->epoll_fd , (_p_accepted_session_)->netaddr.sock ); \
		} \
	} \

#define ModifyAcceptedSessionEpollError(_p_env_,_p_accepted_session_) \
	{ \
		struct epoll_event	event ; \
		\
		memset( & event , 0x00 , sizeof(struct epoll_event) ); \
		event.events = EPOLLERR ; \
		event.data.ptr = (_p_accepted_session_) ; \
		nret = epoll_ctl( (_p_env_)->epoll_fd , EPOLL_CTL_MOD , (_p_accepted_session_)->netaddr.sock , & event ) ; \
		if( nret == -1 ) \
		{ \
			ERRORLOG( "epoll_ctl #%d# modify #%d# accepted session EPOLLERR failed , errno[%d]" , (_p_env_)->epoll_fd , (_p_accepted_session_)->netaddr.sock , errno ); \
			return -1; \
		} \
		else \
		{ \
			DEBUGLOG( "epoll_ctl #%d# modify #%d# accepted session EPOLLERR ok" , (_p_env_)->epoll_fd , (_p_accepted_session_)->netaddr.sock ); \
		} \
	} \

#define DeleteAcceptedSessionEpoll(_p_env_,_p_accepted_session_) \
	{ \
		nret = epoll_ctl( (_p_env_)->epoll_fd , EPOLL_CTL_DEL , (_p_accepted_session_)->netaddr.sock , NULL ) ; \
		if( nret == -1 ) \
		{ \
			ERRORLOG( "epoll_ctl #%d# delete #%d# accpeted session failed , errno[%d]" , (_p_env_)->epoll_fd , (_p_accepted_session_)->netaddr.sock , errno ); \
			return -1; \
		} \
		else \
		{ \
			DEBUGLOG( "epoll_ctl #%d# delete #%d# accpeted session ok" , (_p_env_)->epoll_fd , (_p_accepted_session_)->netaddr.sock ); \
		} \
	} \

#define AddForwardSessionEpollInput(_p_env_,_p_forward_session_) \
	{ \
		struct epoll_event	event ; \
		\
		memset( & event , 0x00 , sizeof(struct epoll_event) ); \
		event.events = EPOLLIN | EPOLLERR ; \
		event.data.ptr = (_p_forward_session_) ; \
		nret = epoll_ctl( (_p_env_)->epoll_fd , EPOLL_CTL_ADD , (_p_forward_session_)->mysql_connection->net.fd , & event ) ; \
		if( nret == -1 ) \
		{ \
			ERRORLOG( "epoll_ctl #%d# add #%d# forward session EPOLLIN failed , errno[%d]" , (_p_env_)->epoll_fd , (_p_forward_session_)->mysql_connection->net.fd , errno ); \
			return -1; \
		} \
		else \
		{ \
			DEBUGLOG( "epoll_ctl #%d# add #%d# forward session EPOLLIN ok" , (_p_env_)->epoll_fd , (_p_forward_session_)->mysql_connection->net.fd ); \
		} \
	} \

#define ModifyForwardSessionEpollInput(_p_env_,_p_forward_session_) \
	{ \
		struct epoll_event	event ; \
		\
		memset( & event , 0x00 , sizeof(struct epoll_event) ); \
		event.events = EPOLLIN | EPOLLERR ; \
		event.data.ptr = (_p_forward_session_) ; \
		nret = epoll_ctl( (_p_env_)->epoll_fd , EPOLL_CTL_MOD , (_p_forward_session_)->mysql_connection->net.fd , & event ) ; \
		if( nret == -1 ) \
		{ \
			ERRORLOG( "epoll_ctl #%d# modify #%d# forward session EPOLLIN failed , errno[%d]" , (_p_env_)->epoll_fd , (_p_forward_session_)->mysql_connection->net.fd , errno ); \
			return -1; \
		} \
		else \
		{ \
			DEBUGLOG( "epoll_ctl #%d# modify #%d# forward session EPOLLIN ok" , (_p_env_)->epoll_fd , (_p_forward_session_)->mysql_connection->net.fd ); \
		} \
	}

#define ModifyForwardSessionEpollOutput(_p_env_,_p_forward_session_) \
	{ \
		struct epoll_event	event ; \
		\
		memset( & event , 0x00 , sizeof(struct epoll_event) ); \
		event.events = EPOLLOUT | EPOLLERR ; \
		event.data.ptr = (_p_forward_session_) ; \
		nret = epoll_ctl( (_p_env_)->epoll_fd , EPOLL_CTL_MOD , (_p_forward_session_)->mysql_connection->net.fd , & event ) ; \
		if( nret == -1 ) \
		{ \
			ERRORLOG( "epoll_ctl #%d# modify #%d# forward session EPOLLOUT failed , errno[%d]" , (_p_env_)->epoll_fd , (_p_forward_session_)->mysql_connection->net.fd , errno ); \
			return -1; \
		} \
		else \
		{ \
			DEBUGLOG( "epoll_ctl #%d# modify #%d# forward session EPOLLOUT ok" , (_p_env_)->epoll_fd , (_p_forward_session_)->mysql_connection->net.fd ); \
		} \
	} \

#define ModifyForwardSessionEpollError(_p_env_,_p_forward_session_) \
	{ \
		struct epoll_event	event ; \
		\
		memset( & event , 0x00 , sizeof(struct epoll_event) ); \
		event.events = EPOLLERR ; \
		event.data.ptr = (_p_forward_session_) ; \
		nret = epoll_ctl( (_p_env_)->epoll_fd , EPOLL_CTL_MOD , (_p_forward_session_)->mysql_connection->net.fd , & event ) ; \
		if( nret == -1 ) \
		{ \
			ERRORLOG( "epoll_ctl #%d# modify #%d# forward session EPOLLERR failed , errno[%d]" , (_p_env_)->epoll_fd , (_p_forward_session_)->mysql_connection->net.fd , errno ); \
			return -1; \
		} \
		else \
		{ \
			DEBUGLOG( "epoll_ctl #%d# modify #%d# forward session EPOLLERR ok" , (_p_env_)->epoll_fd , (_p_forward_session_)->mysql_connection->net.fd ); \
		} \
	} \

#define DeleteForwardSessionEpoll(_p_env_,_p_forward_session_) \
	{ \
		nret = epoll_ctl( (_p_env_)->epoll_fd , EPOLL_CTL_DEL , (_p_forward_session_)->mysql_connection->net.fd , NULL ) ; \
		if( nret == -1 ) \
		{ \
			ERRORLOG( "epoll_ctl #%d# delete #%d# forward session failed , errno[%d]" , (_p_env_)->epoll_fd , (_p_forward_session_)->mysql_connection->net.fd , errno ); \
			return -1; \
		} \
		else \
		{ \
			DEBUGLOG( "epoll_ctl #%d# delete #%d# forward session ok" , (_p_env_)->epoll_fd , (_p_forward_session_)->mysql_connection->net.fd ); \
		} \
	} \

int OnAcceptingSocket( struct MysqldaEnvironment *p_env , struct ListenSession *p_listen_session )
{
	struct AcceptedSession	accepted_session ;
	socklen_t		accept_addr_len ;
	struct AcceptedSession	*p_accepted_session = NULL ;
	
	int			nret = 0 ;
	
	while(1)
	{
		/* 接受新连接 */
		memset( & accepted_session , 0x00 , sizeof(struct AcceptedSession) );
		
		accept_addr_len = sizeof(struct sockaddr) ;
		accepted_session.netaddr.sock = accept( p_listen_session->netaddr.sock , (struct sockaddr *) & (accepted_session.netaddr.addr) , & accept_addr_len ) ;
		if( accepted_session.netaddr.sock == -1 )
		{
			if( errno == EAGAIN )
				break;
			ERRORLOG( "accept failed , errno[%d]" , errno );
			return 1;
		}
		
		/* 分配内存以存放 客户端连接会话 */
		p_accepted_session = (struct AcceptedSession *)malloc( sizeof(struct AcceptedSession) ) ;
		if( p_accepted_session == NULL )
		{
			ERRORLOG( "malloc failed , errno[%d]" , errno );
			return 1;
		}
		memcpy( p_accepted_session , & accepted_session , sizeof(struct AcceptedSession) );
		
		/* 设置通讯选项 */
		SetHttpNonblock( p_accepted_session->netaddr.sock );
		SetHttpNodelay( p_accepted_session->netaddr.sock , 1 );
		
		GETNETADDRESS( p_accepted_session->netaddr )
		
		p_accepted_session->type = SESSIONTYPE_ACCEPTEDSESSION ;
		p_accepted_session->status = SESSIONSTATUS_BEFORE_SENDING_HANDSHAKE ;
		
		p_accepted_session->comm_buffer = (char*)malloc( MAXLEN_ACCEPTED_SESSION_COMM_BUFFER ) ;
		if( p_accepted_session->comm_buffer == NULL )
		{
			ERRORLOG( "malloc failed , errno[%d]" , errno );
			return 1;
		}
		memset( p_accepted_session->comm_buffer , 0x00 , MAXLEN_ACCEPTED_SESSION_COMM_BUFFER );
		
		/* 组织握手报文 */
		FormatHandshakeMessage( p_env , p_accepted_session );
		
		/* 加入新套接字可读事件到epoll */
		AddAcceptedSessionEpollOutput( p_env , p_accepted_session );
		if( nret )
		{
			close( p_accepted_session->netaddr.sock );
			free( p_accepted_session );
		}
	}
	
	return 0;
}

int OnReceivingAcceptedSocket( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session )
{
	struct ForwardSession	*p_forward_session = p_accepted_session->p_pair_forward_session ;
	char			*recv_base = NULL ;
	int			recv_len ;
	int			len ;
	
	int			nret = 0 ;
	
	recv_base = p_accepted_session->comm_buffer + p_accepted_session->fill_len ;
	recv_len = MAXLEN_ACCEPTED_SESSION_COMM_BUFFER - p_accepted_session->fill_len ;
	len = recv( p_accepted_session->netaddr.sock , recv_base , recv_len , 0 ) ;
	if( len == 0 )
	{
		INFOLOG( "recv #%d# return close" , p_accepted_session->netaddr.sock );
		return 1;
	}
	else if( len < 0 )
	{
		ERRORLOG( "recv #%d# failed[%d]" , p_accepted_session->netaddr.sock , len );
		return 1;
	}
	else
	{
		p_accepted_session->fill_len += len ;
		p_accepted_session->process_len = 0 ;
		
		if( p_accepted_session->fill_len > 3 && p_accepted_session->comm_body_len == 0 )
		{
			p_accepted_session->comm_body_len = MYSQL_COMMLEN(p_accepted_session->comm_buffer) ;
			
			if( p_accepted_session->comm_body_len == 1 && p_accepted_session->comm_buffer[4] == 0x01 )
			{
				INFOLOG( "mysql client close socket" );
				return 1;
			}
		}
		
		DEBUGHEXLOG( recv_base , len , "recv #%d# [%d]bytes ok , comm_body_len[%d]" , p_accepted_session->netaddr.sock , len , p_accepted_session->comm_body_len );
		
		if( p_accepted_session->comm_body_len > 0 && p_accepted_session->fill_len >= 3+1+p_accepted_session->comm_body_len )
		{
			DEBUGLOG( "recv #%d# done , comm_body_len[%d]" , p_accepted_session->netaddr.sock , p_accepted_session->comm_body_len );
			
			if( UNLIKELY( p_accepted_session->status == SESSIONSTATUS_AFTER_SENDING_HANDSHAKE_AND_BEFORE_RECEIVING_AUTHENTICATION ) )
			{
				nret = CheckAuthenticationMessage( p_env , p_accepted_session ) ;
				if( nret )
				{
					ERRORLOG( "CheckAuthenticationMessage failed[%d]" , nret );
					p_accepted_session->status = SESSIONSTATUS_AFTER_SENDING_AUTH_FAIL_AND_BEFORE_FORWARDING ;
					FormatAuthResultFail( p_env , p_accepted_session );
					ModifyAcceptedSessionEpollOutput( p_env , p_accepted_session );
				}
				else
				{
					INFOLOG( "CheckAuthenticationMessage ok" );
					p_accepted_session->status = SESSIONSTATUS_AFTER_SENDING_AUTH_OK_AND_BEFORE_FORWARDING ;
					FormatAuthResultOk( p_env , p_accepted_session );
					ModifyAcceptedSessionEpollOutput( p_env , p_accepted_session );
				}
			}
			else if( LIKELY( p_accepted_session->status == SESSIONSTATUS_FORWARDING ) )
			{
				ModifyAcceptedSessionEpollError( p_env , p_accepted_session );
				ModifyForwardSessionEpollOutput( p_env , p_forward_session );
			}
		}
	}
	
	return 0;
}

int OnSendingAcceptedSocket( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session )
{
	struct ForwardSession	*p_forward_session = p_accepted_session->p_pair_forward_session ;
	char			*send_base = NULL ;
	int			send_len ;
	int			len ;
	
	int			nret = 0 ;
	
_GOTO_SENDING_AGAIN :
	
	send_base = p_accepted_session->comm_buffer + p_accepted_session->process_len ;
	send_len = 3+1+p_accepted_session->comm_body_len - p_accepted_session->process_len ;
	len = send( p_accepted_session->netaddr.sock , send_base , send_len , 0 ) ;
	if( len == -1 )
	{
		ERRORLOG( "send #%d# failed , errno[%d]" , p_accepted_session->netaddr.sock , errno );
		return 1;
	}
	else
	{
		p_accepted_session->process_len += len ;
		DEBUGHEXLOG( send_base , len , "send #%d# [%d]bytes ok" , p_accepted_session->netaddr.sock , len );
		
		if( 3+1+p_accepted_session->comm_body_len == p_accepted_session->process_len )
		{
			p_accepted_session->fill_len = 0 ;
			p_accepted_session->process_len = 0 ;
			p_accepted_session->comm_body_len = 0 ;
			
			if( UNLIKELY( p_accepted_session->status == SESSIONSTATUS_BEFORE_SENDING_HANDSHAKE ) )
			{
				p_accepted_session->status = SESSIONSTATUS_AFTER_SENDING_HANDSHAKE_AND_BEFORE_RECEIVING_AUTHENTICATION ;
				ModifyAcceptedSessionEpollInput( p_env , p_accepted_session );
			}
			else if( UNLIKELY( p_accepted_session->status == SESSIONSTATUS_AFTER_SENDING_AUTH_FAIL_AND_BEFORE_FORWARDING ) )
			{
				INFOLOG( "need to close#%d#" , p_accepted_session->netaddr.sock );
				return 1;
			}
			else if( UNLIKELY( p_accepted_session->status == SESSIONSTATUS_AFTER_SENDING_AUTH_OK_AND_BEFORE_FORWARDING ) )
			{
				p_accepted_session->status = SESSIONSTATUS_FORWARDING ;
				
				p_forward_session = (struct ForwardSession *)malloc( sizeof(struct ForwardSession) ) ;
				if( p_forward_session == NULL )
				{
					ERRORLOG( "malloc failed , errno[%d]" , errno );
					return 1;
				}
				memset( p_forward_session , 0x00 , sizeof(struct ForwardSession) );
				
				p_forward_session->type = SESSIONTYPE_FORWARDSESSION ;
				p_forward_session->p_forward_power = QueryForwardPowerRangeTreeNode( p_env , 0 ) ;
				p_forward_session->mysql_connection = mysql_init( NULL ) ;
				if( p_forward_session->mysql_connection == NULL )
				{
					ERRORLOG( "mysql_init failed , errno[%d]" , errno );
					return 1;
				}
				
				INFOLOG( "[%s]mysql_real_connect[%s][%d][%s][%s][%s] connecting ..." , p_forward_session->p_forward_power->instance , p_forward_session->p_forward_power->netaddr.ip , p_forward_session->p_forward_power->netaddr.port , p_env->user , p_env->pass , p_env->db );
				if( mysql_real_connect( p_forward_session->mysql_connection , p_forward_session->p_forward_power->netaddr.ip , p_env->user , p_env->pass , p_env->db , p_forward_session->p_forward_power->netaddr.port , NULL , 0 ) == NULL )
				{
					ERRORLOG( "[%s]mysql_real_connect[%s][%d][%s][%s][%s] failed , mysql_errno[%d][%s]" , p_forward_session->p_forward_power->instance , p_forward_session->p_forward_power->netaddr.ip , p_forward_session->p_forward_power->netaddr.port , p_env->user , p_env->pass , p_env->db , mysql_errno(p_forward_session->mysql_connection) , mysql_error(p_forward_session->mysql_connection) );
					return 1;
				}
				else
				{
					INFOLOG( "[%s]mysql_real_connect[%s][%d][%s][%s][%s] connecting ok" , p_forward_session->p_forward_power->instance , p_forward_session->p_forward_power->netaddr.ip , p_forward_session->p_forward_power->netaddr.port , p_env->user , p_env->pass , p_env->db );
				}
				
				p_accepted_session->p_pair_forward_session = p_forward_session ;
				p_forward_session->p_pair_accepted_session = p_accepted_session ;
				
				ModifyAcceptedSessionEpollInput( p_env , p_accepted_session );
				AddForwardSessionEpollInput( p_env , p_forward_session );
			}
			else if( LIKELY( p_accepted_session->status == SESSIONSTATUS_FORWARDING ) )
			{
				if( p_accepted_session->process_len == p_accepted_session->fill_len )
				{
					p_accepted_session->fill_len = 0 ;
					p_accepted_session->process_len = 0 ;
					p_accepted_session->comm_body_len = 0 ;
					ModifyAcceptedSessionEpollInput( p_env , p_accepted_session );
					ModifyForwardSessionEpollInput( p_env , p_forward_session );
				}
				else
				{
					memmove( p_accepted_session->comm_buffer , p_accepted_session->comm_buffer+p_accepted_session->process_len , p_accepted_session->fill_len-p_accepted_session->process_len );
					p_accepted_session->fill_len -= p_accepted_session->process_len ;
					p_accepted_session->process_len = 0 ;
					p_accepted_session->comm_body_len = MYSQL_COMMLEN(p_accepted_session->comm_buffer) ;
					goto _GOTO_SENDING_AGAIN;
				}
			}
		}
	}
	
	return 0;
}

int OnClosingAcceptedSocket( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session )
{
	struct ForwardSession	*p_forward_session = p_accepted_session->p_pair_forward_session ;
	
	int			nret = 0 ;
	
	DeleteAcceptedSessionEpoll( p_env , p_accepted_session );
	close( p_accepted_session->netaddr.sock );
	INFOLOG( "close#%d#" , p_accepted_session->netaddr.sock );
	
	if( p_accepted_session->p_pair_forward_session )
	{
		DeleteForwardSessionEpoll( p_env , p_forward_session );
		mysql_close( p_forward_session->mysql_connection );
		INFOLOG( "[%s]#%d#mysql_close[%s][%d] ok" , p_forward_session->p_forward_power->instance , p_forward_session->mysql_connection->net.fd , p_forward_session->p_forward_power->netaddr.ip , p_forward_session->p_forward_power->netaddr.port );
		
		free( p_forward_session );
	}
	
	free( p_accepted_session->comm_buffer );
	free( p_accepted_session );
	
	return 0;
}

int OnReceivingForwardSocket( struct MysqldaEnvironment *p_env , struct ForwardSession *p_forward_session )
{
	struct AcceptedSession	*p_accepted_session = p_forward_session->p_pair_accepted_session ;
	char			*recv_base = NULL ;
	int			recv_len ;
	int			len ;
	
	int			nret = 0 ;
	
	recv_base = p_accepted_session->comm_buffer + p_accepted_session->fill_len ;
	recv_len = MAXLEN_ACCEPTED_SESSION_COMM_BUFFER - p_accepted_session->fill_len ;
	len = recv( p_forward_session->mysql_connection->net.fd , recv_base , recv_len , 0 ) ;
	if( len == 0 )
	{
		INFOLOG( "recv #%d# return close" , p_accepted_session->netaddr.sock );
		return 1;
	}
	else if( len < 0 )
	{
		ERRORLOG( "recv #%d# failed[%d]" , p_accepted_session->netaddr.sock , len );
		return 1;
	}
	else
	{
		p_accepted_session->fill_len += len ;
		p_accepted_session->process_len = 0 ;
		
		if( p_accepted_session->fill_len > 3 && p_accepted_session->comm_body_len == 0 )
		{
			p_accepted_session->comm_body_len = MYSQL_COMMLEN(p_accepted_session->comm_buffer) ;
		}
		
		DEBUGHEXLOG( recv_base , len , "recv #%d# [%d]bytes ok , comm_body_len[%d]" , p_accepted_session->netaddr.sock , len , p_accepted_session->comm_body_len );
		
		if( p_accepted_session->comm_body_len > 0 && p_accepted_session->fill_len >= 3+1+p_accepted_session->comm_body_len )
		{
			DEBUGLOG( "recv #%d# done , comm_body_len[%d]" , p_accepted_session->netaddr.sock , p_accepted_session->comm_body_len );
			
			ModifyAcceptedSessionEpollOutput( p_env , p_accepted_session );
			ModifyForwardSessionEpollError( p_env , p_forward_session );
		}
	}
	
	return 0;
}

int OnSendingForwardSocket( struct MysqldaEnvironment *p_env , struct ForwardSession *p_forward_session )
{
	struct AcceptedSession	*p_accepted_session = p_forward_session->p_pair_accepted_session ;
	char			*send_base = NULL ;
	int			send_len ;
	int			len ;
	
	int			nret = 0 ;
	
_GOTO_SENDING_AGAIN :
	
	send_base = p_accepted_session->comm_buffer + p_accepted_session->process_len ;
	send_len = 3+1+p_accepted_session->comm_body_len - p_accepted_session->process_len ;
	len = send( p_forward_session->mysql_connection->net.fd , send_base , send_len , 0 ) ;
	if( len == -1 )
	{
		ERRORLOG( "send #%d# failed , errno[%d]" , p_accepted_session->netaddr.sock , errno );
		return 1;
	}
	else
	{
		p_accepted_session->process_len += len ;
		DEBUGHEXLOG( send_base , len , "send #%d# [%d]bytes ok" , p_accepted_session->netaddr.sock , len );
		
		if( p_accepted_session->process_len == p_accepted_session->fill_len )
		{
			p_accepted_session->fill_len = 0 ;
			p_accepted_session->process_len = 0 ;
			p_accepted_session->comm_body_len = 0 ;
			ModifyForwardSessionEpollInput( p_env , p_forward_session );
		}
		else
		{
			memmove( p_accepted_session->comm_buffer , p_accepted_session->comm_buffer+p_accepted_session->process_len , p_accepted_session->fill_len-p_accepted_session->process_len );
			p_accepted_session->fill_len -= p_accepted_session->process_len ;
			p_accepted_session->process_len = 0 ;
			p_accepted_session->comm_body_len = MYSQL_COMMLEN(p_accepted_session->comm_buffer) ;
			goto _GOTO_SENDING_AGAIN;
		}
	}
	
	return 0;
}

int OnClosingForwardSocket( struct MysqldaEnvironment *p_env , struct ForwardSession *p_forward_session )
{
	struct AcceptedSession	*p_accepted_session = p_forward_session->p_pair_accepted_session ;
	
	int			nret = 0 ;
	
	DeleteForwardSessionEpoll( p_env , p_forward_session );
	mysql_close( p_forward_session->mysql_connection );
	INFOLOG( "[%s]#%d#mysql_close[%s][%d] ok" , p_forward_session->p_forward_power->instance , p_forward_session->mysql_connection->net.fd , p_forward_session->p_forward_power->netaddr.ip , p_forward_session->p_forward_power->netaddr.port );
	
	if( p_forward_session->p_pair_accepted_session )
	{
		DeleteAcceptedSessionEpoll( p_env , p_accepted_session );
		INFOLOG( "close[%d]" , p_accepted_session->netaddr.sock );
		close( p_accepted_session->netaddr.sock );
		
		free( p_accepted_session->comm_buffer );
		free( p_accepted_session );
	}
	
	free( p_forward_session );
	
	return 0;
}

