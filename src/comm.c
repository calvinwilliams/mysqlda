#include "mysqlda_in.h"

int ModifyEpollInput( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session )
{
	struct epoll_event	event ;
	int			nret = 0 ;
	
	memset( & event , 0x00 , sizeof(struct epoll_event) );
	event.events = EPOLLIN | EPOLLERR ;
	event.data.ptr = p_accepted_session ;
	nret = epoll_ctl( p_env->epoll_fd , EPOLL_CTL_ADD , p_accepted_session->netaddr.sock , & event ) ;
	if( nret == -1 )
	{
		ERRORLOG( "epoll_ctl[%d] modify[%d] EPOLLIN failed , errno[%d]" , p_env->epoll_fd , p_accepted_session->netaddr.sock , errno );
		return 1;
	}
	else
	{
		DEBUGLOG( "epoll_ctl[%d] modify[%d] EPOLLIN ok" , p_env->epoll_fd , p_accepted_session->netaddr.sock );
	}
	
	return 0;
}

int ModifyEpollOutput( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session )
{
	struct epoll_event	event ;
	int			nret = 0 ;
	
	memset( & event , 0x00 , sizeof(struct epoll_event) );
	event.events = EPOLLOUT | EPOLLERR ;
	event.data.ptr = p_accepted_session ;
	nret = epoll_ctl( p_env->epoll_fd , EPOLL_CTL_ADD , p_accepted_session->netaddr.sock , & event ) ;
	if( nret == -1 )
	{
		ERRORLOG( "epoll_ctl[%d] modify[%d] EPOLLOUT failed , errno[%d]" , p_env->epoll_fd , p_accepted_session->netaddr.sock , errno );
		return 1;
	}
	else
	{
		DEBUGLOG( "epoll_ctl[%d] modify[%d] EPOLLOUT ok" , p_env->epoll_fd , p_accepted_session->netaddr.sock );
	}
	
	return 0;
}

int OnAcceptingSocket( struct MysqldaEnvironment *p_env , struct ListenSession *p_listen_session )
{
	struct AcceptedSession	accepted_session ;
	socklen_t		accept_addr_len ;
	struct AcceptedSession	*p_accepted_session = NULL ;
	
	struct epoll_event	event ;
	
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
		
		p_accepted_session->type = SESSIONSTATUS_BEFORE_SENDING_HANDSHAKE ;
		
		/* 组织握手报文 */
		FormatHandshakeMessage( p_env , p_accepted_session );
		
		/* 加入新套接字可读事件到epoll */
		memset( & event , 0x00 , sizeof(struct epoll_event) );
		event.events = EPOLLOUT | EPOLLERR ;
		event.data.ptr = p_accepted_session ;
		nret = epoll_ctl( p_env->epoll_fd , EPOLL_CTL_ADD , p_accepted_session->netaddr.sock , & event ) ;
		if( nret == -1 )
		{
			ERRORLOG( "epoll_ctl[%d] add[%d] failed , errno[%d]" , p_env->epoll_fd , p_accepted_session->netaddr.sock , errno );
			close( p_accepted_session->netaddr.sock );
			free( p_accepted_session );
			return 1;
		}
		else
		{
			DEBUGLOG( "epoll_ctl[%d] add[%d] ok" , p_env->epoll_fd , p_accepted_session->netaddr.sock );
		}
	}
	
	return 0;
}

int OnSendingAcceptedSocket( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session )
{
	char		*send_base = NULL ;
	int		send_len ;
	int		len ;
	
	send_base = p_accepted_session->comm_buffer + p_accepted_session->process_len ;
	send_len = p_accepted_session->fill_len - p_accepted_session->process_len ;
	len = send( p_accepted_session->netaddr.sock , send_base , send_len , 0 ) ;
	if( len == -1 )
	{
		ERRORLOG( "send[%d] failed , errno[%d]" , p_accepted_session->netaddr.sock , errno );
		return 1;
	}
	else
	{
		DEBUGHEXLOG( send_base , len , "send[%d] ok" , p_accepted_session->netaddr.sock );
		p_accepted_session->process_len += len ;
		
		if( p_accepted_session->process_len == p_accepted_session->fill_len )
		{
			if( UNLIKELY( p_accepted_session->status == SESSIONSTATUS_BEFORE_SENDING_HANDSHAKE ) )
			{
				p_accepted_session->status = SESSIONSTATUS_AFTER_SENDING_HANDSHAKE_AND_BEFORE_RECEIVING_AUTHENTICATION ;
				p_accepted_session->fill_len = 0 ;
				p_accepted_session->process_len = 0 ;
				ModifyEpollInput( p_env , p_accepted_session );
			}
			else if( UNLIKELY( p_accepted_session->status == SESSIONSTATUS_AFTER_SENDING_AUTH_FAIL_AND_BEFORE_FORWARDING ) )
			{
				INFOLOG( "need to close[%d]" , p_accepted_session->netaddr.sock );
				return 1;
			}
			else if( UNLIKELY( p_accepted_session->status == SESSIONSTATUS_AFTER_SENDING_AUTH_OK_AND_BEFORE_FORWARDING ) )
			{
				p_accepted_session->status = SESSIONSTATUS_FORWARDING ;
				p_accepted_session->fill_len = 0 ;
				p_accepted_session->process_len = 0 ;
				ModifyEpollInput( p_env , p_accepted_session );
			}
		}
	}
	
	return 0;
}

int OnReceivingAcceptedSocket( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session )
{
	char		*recv_base = NULL ;
	int		recv_len ;
	int		len ;
	int		nret = 0 ;
	
	recv_base = p_accepted_session->comm_buffer + p_accepted_session->fill_len ;
	recv_len = sizeof(p_accepted_session->comm_buffer) - p_accepted_session->fill_len ;
	len = recv( p_accepted_session->netaddr.sock , recv_base , recv_len , 0 ) ;
	if( len == 0 )
	{
		INFOLOG( "recv[%d] return close" , p_accepted_session->netaddr.sock );
		return 1;
	}
	else if( len < 0 )
	{
		ERRORLOG( "recv[%d] failed[%d]" , p_accepted_session->netaddr.sock , len );
		return 1;
	}
	else
	{
		DEBUGHEXLOG( recv_base , len , "recv[%d] ok" , p_accepted_session->netaddr.sock );
		p_accepted_session->fill_len += len ;
		
		if( UNLIKELY( p_accepted_session->status == SESSIONSTATUS_AFTER_SENDING_HANDSHAKE_AND_BEFORE_RECEIVING_AUTHENTICATION ) )
		{
			if( p_accepted_session->fill_len > 3 && p_accepted_session->msg_len == 0 )
			{
				p_accepted_session->msg_len = p_accepted_session->comm_buffer[0] + p_accepted_session->comm_buffer[0]*0xFF + p_accepted_session->comm_buffer[0]*0xFF*0xFF ;
			}
			
			if( p_accepted_session->fill_len == 3 + p_accepted_session->msg_len )
			{
				nret = CheckAuthenticationMessage( p_env , p_accepted_session ) ;
				if( nret )
				{
					ERRORLOG( "CheckAuthenticationMessage failed[%d]" , nret );
					p_accepted_session->status = SESSIONSTATUS_AFTER_SENDING_AUTH_FAIL_AND_BEFORE_FORWARDING ;
					FormatAuthResultFail( p_env , p_accepted_session );
					ModifyEpollOutput( p_env , p_accepted_session );
				}
				else
				{
					INFOLOG( "CheckAuthenticationMessage ok" );
					p_accepted_session->status = SESSIONSTATUS_AFTER_SENDING_AUTH_OK_AND_BEFORE_FORWARDING ;
					FormatAuthResultOk( p_env , p_accepted_session );
					ModifyEpollOutput( p_env , p_accepted_session );
				}
			}
		}
	}
	
	return 0;
}

int OnClosingAcceptedSocket( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session )
{
	close( p_accepted_session->netaddr.sock );
	INFOLOG( "close[%d]" , p_accepted_session->netaddr.sock );
	free( p_accepted_session );
	return 0;
}

int OnReceivingForwardSocket( struct MysqldaEnvironment *p_env , struct ForwardSession *p_forward_session )
{
	return 0;
}

int OnSendingForwardSocket( struct MysqldaEnvironment *p_env , struct ForwardSession *p_forward_session )
{
	return 0;
}

int OnClosingForwardSocket( struct MysqldaEnvironment *p_env , struct ForwardSession *p_forward_session )
{
	return 0;
}

