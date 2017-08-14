#include "mysqlda_in.h"

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
			DEBUGHEXLOG( recv_base , len , "recv #%d# [%d]bytes ok , comm_body_len[%d]" , p_accepted_session->netaddr.sock , len , p_accepted_session->comm_body_len );
			
			if( p_accepted_session->comm_buffer[4] == 0x01 )
			{
				INFOLOG( "mysql client close socket" );
				return 1;
			}
			else if( p_accepted_session->comm_buffer[4] == 0x79 )
			{
				INFOLOG( "select library[%.100s]" , p_accepted_session->comm_buffer+5 );
				if( p_accepted_session->status == SESSIONSTATUS_AFTER_SENDING_AUTH_OK_AND_BEFORE_RECEIVING_SELECT_LIBRARY )
					p_accepted_session->status = SESSIONSTATUS_AFTER_SENDING_SELECT_LIBRARY_AND_BEFORE_FORDWARD ;
				nret = DatabaseSelectLibrary( p_env , p_accepted_session ) ;
				if( nret )
					return 1;
				ModifyAcceptedSessionEpollOutput( p_env , p_accepted_session );
				return 0;
			}
		}
		else
		{
			DEBUGHEXLOG( recv_base , len , "recv #%d# [%d]bytes ok , comm_body_len[%d]" , p_accepted_session->netaddr.sock , len , p_accepted_session->comm_body_len );
		}
		
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
					p_accepted_session->status = SESSIONSTATUS_AFTER_SENDING_AUTH_OK_AND_BEFORE_RECEIVING_SELECT_LIBRARY ;
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
			if( UNLIKELY( p_accepted_session->status == SESSIONSTATUS_BEFORE_SENDING_HANDSHAKE ) )
			{
				p_accepted_session->fill_len = 0 ;
				p_accepted_session->process_len = 0 ;
				p_accepted_session->comm_body_len = 0 ;
				p_accepted_session->status = SESSIONSTATUS_AFTER_SENDING_HANDSHAKE_AND_BEFORE_RECEIVING_AUTHENTICATION ;
				ModifyAcceptedSessionEpollInput( p_env , p_accepted_session );
			}
			else if( UNLIKELY( p_accepted_session->status == SESSIONSTATUS_AFTER_SENDING_AUTH_FAIL_AND_BEFORE_FORWARDING ) )
			{
				INFOLOG( "need to close#%d#" , p_accepted_session->netaddr.sock );
				return 1;
			}
			else if( UNLIKELY( p_accepted_session->status == SESSIONSTATUS_AFTER_SENDING_AUTH_OK_AND_BEFORE_RECEIVING_SELECT_LIBRARY ) )
			{
				p_accepted_session->fill_len = 0 ;
				p_accepted_session->process_len = 0 ;
				p_accepted_session->comm_body_len = 0 ;
				p_accepted_session->status = SESSIONSTATUS_AFTER_SENDING_SELECT_LIBRARY_AND_BEFORE_FORDWARD ;
				ModifyAcceptedSessionEpollInput( p_env , p_accepted_session );
			}
			else if( UNLIKELY( p_accepted_session->status == SESSIONSTATUS_AFTER_SENDING_SELECT_LIBRARY_AND_BEFORE_FORDWARD ) )
			{
				p_accepted_session->fill_len = 0 ;
				p_accepted_session->process_len = 0 ;
				p_accepted_session->comm_body_len = 0 ;
				p_accepted_session->status = SESSIONSTATUS_FORWARDING ;
				ModifyAcceptedSessionEpollInput( p_env , p_accepted_session );
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
	INFOLOG( "close #%d#" , p_accepted_session->netaddr.sock );
	
	if( p_accepted_session->p_pair_forward_session )
	{
		if( p_forward_session->mysql_connection )
		{
			DeleteForwardSessionEpoll( p_env , p_forward_session );
			INFOLOG( "[%s] #%d# mysql_close[%s][%d] ok" , p_forward_session->p_forward_instance->instance , p_forward_session->mysql_connection->net.fd , p_forward_session->p_forward_server->netaddr.ip , p_forward_session->p_forward_server->netaddr.port );
			mysql_close( p_forward_session->mysql_connection );
			p_forward_session->mysql_connection = NULL ;
		}
		
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
	
	if( p_forward_session->mysql_connection )
	{
		DeleteForwardSessionEpoll( p_env , p_forward_session );
		INFOLOG( "[%s] #%d# mysql_close[%s][%d] ok" , p_forward_session->p_forward_instance->instance , p_forward_session->mysql_connection->net.fd , p_forward_session->p_forward_server->netaddr.ip , p_forward_session->p_forward_server->netaddr.port );
		mysql_close( p_forward_session->mysql_connection );
		p_forward_session->mysql_connection = NULL ;
	}
	
	if( p_forward_session->p_pair_accepted_session )
	{
		DeleteAcceptedSessionEpoll( p_env , p_accepted_session );
		INFOLOG( "close #%d#" , p_accepted_session->netaddr.sock );
		close( p_accepted_session->netaddr.sock );
		
		free( p_accepted_session->comm_buffer );
		free( p_accepted_session );
	}
	
	free( p_forward_session );
	
	return 0;
}

