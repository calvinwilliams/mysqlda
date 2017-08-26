#include "mysqlda_in.h"

/* 非堵塞接受客户端连接 */
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

/* 非堵塞接收客户端连接会话 */
int OnReceivingAcceptedSocket( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session )
{
	struct ForwardSession	*p_forward_session = p_accepted_session->p_pair_forward_session ;
	char			*recv_base = NULL ;
	int			recv_len ;
	int			len ;
	
	int			nret = 0 ;
	
	/* 收一把客户端连接会话 */
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
		
		/* 如果未计算出mysql协议体长度，则计算之 */
		if( p_accepted_session->fill_len >= 4 && p_accepted_session->comm_body_len == 0 )
		{
			p_accepted_session->comm_body_len = MYSQL_COMMLEN(p_accepted_session->comm_buffer) ;
			DEBUGHEXLOG( recv_base , len , "recv #%d# [%d]bytes ok , comm_body_len[%d]" , p_accepted_session->netaddr.sock , len , p_accepted_session->comm_body_len );
			
			if( p_accepted_session->comm_buffer[4] == 0x01 )
			{
				INFOLOG( "mysql client close socket" );
				return 1;
			}
			else if( (unsigned char)(p_accepted_session->comm_buffer[4]) == 0x79 )
			{
				char		*library = NULL ;
				int		library_len ;
				
				if( p_accepted_session->status == SESSIONSTATUS_AFTER_SENDING_AUTH_OK_AND_BEFORE_RECEIVING_SELECT_LIBRARY )
					p_accepted_session->status = SESSIONSTATUS_AFTER_SENDING_SELECT_LIBRARY_AND_BEFORE_FORDWARD ;
				
				library = p_accepted_session->comm_buffer + 5 ;
				library_len = p_accepted_session->comm_body_len - 1 ;
				INFOLOG( "select library[%.*s]" , library_len , library );
				
				nret = SelectDatabaseLibrary( p_env , p_accepted_session , library , library_len ) ;
				if( nret )
				{
					ERRORLOG( "SelectDatabaseLibrary failed[%d]" , nret );
					return 1;
				}
				else
				{
					DEBUGLOG( "SelectDatabaseLibrary ok" );
				}
				
				ModifyAcceptedSessionEpollOutput( p_env , p_accepted_session );
				
				return 0;
			}
			else if( (unsigned char)(p_accepted_session->comm_buffer[4]) == 0x80 )
			{
				char		*correl_object_class = NULL ;
				int		correl_object_class_len ;
				char		*correl_object = NULL ;
				int		correl_object_len ;
				char		*library = NULL ;
				int		library_len ;
				
				correl_object_class = p_accepted_session->comm_buffer + 5 ;
				correl_object_class_len = strlen(correl_object_class) ;
				INFOLOG( "select correl_object_class[%.*s]" , correl_object_class_len , correl_object_class );
				
				correl_object = correl_object_class + correl_object_class_len + 1 ;
				correl_object_len = strlen(correl_object) ;
				INFOLOG( "select correl_object[%.*s]" , correl_object_len , correl_object );
				
				library = correl_object + correl_object_len + 1 ;
				library_len = strlen(library) ;
				INFOLOG( "select library[%.*s]" , library_len , library );
				
				if( correl_object_class_len+1+correl_object_len+1+library_len+1 > p_accepted_session->comm_body_len-1 )
					return 1;
				
				nret = SetDatabaseCorrelObject( p_env , p_accepted_session , correl_object_class , correl_object_class_len , correl_object , correl_object_len , library , library_len ) ;
				if( nret )
				{
					ERRORLOG( "SetDatabaseCorrelObject failed[%d]" , nret );
					return 1;
				}
				else
				{
					DEBUGLOG( "SetDatabaseCorrelObject ok" );
				}
				
				ModifyAcceptedSessionEpollOutput( p_env , p_accepted_session );
				
				return 0;
			}
			else if( (unsigned char)(p_accepted_session->comm_buffer[4]) == 0x81 )
			{
				char		*correl_object_class = NULL ;
				int		correl_object_class_len ;
				char		*correl_object = NULL ;
				int		correl_object_len ;
				
				INFOLOG( "select library[%.100s]" , p_accepted_session->comm_buffer+5 );
				if( p_accepted_session->status == SESSIONSTATUS_AFTER_SENDING_AUTH_OK_AND_BEFORE_RECEIVING_SELECT_LIBRARY )
					p_accepted_session->status = SESSIONSTATUS_AFTER_SENDING_SELECT_LIBRARY_AND_BEFORE_FORDWARD ;
				
				correl_object_class = p_accepted_session->comm_buffer + 5 ;
				correl_object_class_len = strlen(correl_object_class) ;
				INFOLOG( "select correl_object_class[%.*s]" , correl_object_class_len , correl_object_class );
				
				correl_object = correl_object_class + correl_object_class_len + 1 ;
				correl_object_len = strlen(correl_object) ;
				INFOLOG( "select correl_object[%.*s]" , correl_object_len , correl_object );
				
				if( correl_object_class_len+1+correl_object_len+1 > p_accepted_session->comm_body_len-1 )
					return 1;
				
				nret = SelectDatabaseLibraryByCorrelObject( p_env , p_accepted_session , correl_object_class , correl_object_class_len , correl_object , correl_object_len ) ;
				if( nret )
				{
					ERRORLOG( "SelectDatabaseLibraryByCorrelObject failed[%d]" , nret );
					return 1;
				}
				else
				{
					DEBUGLOG( "SelectDatabaseLibraryByCorrelObject ok" );
				}
				
				ModifyAcceptedSessionEpollOutput( p_env , p_accepted_session );
				
				return 0;
			}
		}
		else
		{
			DEBUGHEXLOG( recv_base , len , "recv #%d# [%d]bytes ok , comm_body_len[%d]" , p_accepted_session->netaddr.sock , len , p_accepted_session->comm_body_len );
		}
		
		/* 检查是否收满完整的mysql协议包 */
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

/* 非堵塞发送客户端连接会话 */
int OnSendingAcceptedSocket( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session )
{
	struct ForwardSession	*p_forward_session = p_accepted_session->p_pair_forward_session ;
	char			*send_base = NULL ;
	int			send_len ;
	int			len ;
	
_GOTO_SENDING_AGAIN :
	
	/* 发一把客户端连接会话 */
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
		
		/* 检查是否发完mysql协议包 */
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

/* 关闭客户端连接会话 */
int OnClosingAcceptedSocket( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session )
{
	struct ForwardSession	*p_forward_session = p_accepted_session->p_pair_forward_session ;
	
	/* 清理客户端连接会话 */
	DeleteAcceptedSessionEpoll( p_env , p_accepted_session );
	close( p_accepted_session->netaddr.sock );
	INFOLOG( "close #%d#" , p_accepted_session->netaddr.sock );
	
	if( p_accepted_session->p_pair_forward_session )
	{
		/* 清理服务端转发会话 */
		if( p_forward_session->mysql_connection )
		{
			/* 移动会话到缓存池 */
			DeleteForwardSessionEpoll( p_env , p_forward_session );
			lk_list_del_init( & (p_accepted_session->p_pair_forward_session->forward_session_listnode) );
			p_accepted_session->p_pair_forward_session->close_unused_forward_session_timestamp = time(NULL) + p_env->unused_forward_session_timeout ;
			lk_list_add_tail( & (p_accepted_session->p_pair_forward_session->unused_forward_session_listnode) , & (p_accepted_session->p_pair_forward_session->p_forward_server->unused_forward_session_list) );
			INFOLOG( "move p_forward_session[0x%X] [%d] from forward_session_list to unused_forward_session_list" , p_accepted_session->p_pair_forward_session , p_accepted_session->p_pair_forward_session->close_unused_forward_session_timestamp );
		}
		else
		{
			free( p_forward_session );
		}
	}
	
	free( p_accepted_session->comm_buffer );
	free( p_accepted_session );
	
	return 0;
}

/* 非堵塞接收服务端转发会话 */
int OnReceivingForwardSocket( struct MysqldaEnvironment *p_env , struct ForwardSession *p_forward_session )
{
	struct AcceptedSession	*p_accepted_session = p_forward_session->p_pair_accepted_session ;
	char			*recv_base = NULL ;
	int			recv_len ;
	int			len ;
	
	/* 收一把服务端转发会话 */
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
		
		/* 如果未计算出mysql协议体长度，则计算之 */
		if( p_accepted_session->fill_len > 3 && p_accepted_session->comm_body_len == 0 )
		{
			p_accepted_session->comm_body_len = MYSQL_COMMLEN(p_accepted_session->comm_buffer) ;
		}
		
		DEBUGHEXLOG( recv_base , len , "recv #%d# [%d]bytes ok , comm_body_len[%d]" , p_accepted_session->netaddr.sock , len , p_accepted_session->comm_body_len );
		
		/* 检查是否收满完整的mysql协议包 */
		if( p_accepted_session->comm_body_len > 0 && p_accepted_session->fill_len >= 3+1+p_accepted_session->comm_body_len )
		{
			DEBUGLOG( "recv #%d# done , comm_body_len[%d]" , p_accepted_session->netaddr.sock , p_accepted_session->comm_body_len );
			
			ModifyAcceptedSessionEpollOutput( p_env , p_accepted_session );
			ModifyForwardSessionEpollError( p_env , p_forward_session );
		}
	}
	
	return 0;
}

/* 非堵塞发送服务端转发会话 */
int OnSendingForwardSocket( struct MysqldaEnvironment *p_env , struct ForwardSession *p_forward_session )
{
	struct AcceptedSession	*p_accepted_session = p_forward_session->p_pair_accepted_session ;
	char			*send_base = NULL ;
	int			send_len ;
	int			len ;
	
_GOTO_SENDING_AGAIN :
	
	/* 发一把服务端转发会话 */
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
		
		/* 检查是否发完mysql协议包 */
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

/* 关闭服务端转发会话 */
int OnClosingForwardSocket( struct MysqldaEnvironment *p_env , struct ForwardSession *p_forward_session )
{
	struct AcceptedSession	*p_accepted_session = p_forward_session->p_pair_accepted_session ;
	
	/* 清理客户端连接会话 */
	if( p_forward_session->p_pair_accepted_session )
	{
		DeleteAcceptedSessionEpoll( p_env , p_accepted_session );
		INFOLOG( "close #%d#" , p_accepted_session->netaddr.sock );
		close( p_accepted_session->netaddr.sock );
		
		free( p_accepted_session->comm_buffer );
		free( p_accepted_session );
	}
	
	/* 清理服务端转发会话 */
	if( p_forward_session->mysql_connection )
	{
		/* 移动会话到缓存池 */
		DeleteForwardSessionEpoll( p_env , p_forward_session );
		lk_list_del_init( & (p_accepted_session->p_pair_forward_session->forward_session_listnode) );
		p_accepted_session->p_pair_forward_session->close_unused_forward_session_timestamp = time(NULL) + p_env->unused_forward_session_timeout ;
		lk_list_add_tail( & (p_accepted_session->p_pair_forward_session->unused_forward_session_listnode) , & (p_accepted_session->p_pair_forward_session->p_forward_server->unused_forward_session_list) );
		INFOLOG( "move p_forward_session[0x%X] [%d] from forward_session_list to unused_forward_session_list" , p_accepted_session->p_pair_forward_session , p_accepted_session->p_pair_forward_session->close_unused_forward_session_timestamp );
	}
	else
	{
		free( p_forward_session );
	}
	
	return 0;
}

