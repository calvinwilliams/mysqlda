#ifndef _H_CTL_EPOLL_
#define _H_CTL_EPOLL_

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

#endif

