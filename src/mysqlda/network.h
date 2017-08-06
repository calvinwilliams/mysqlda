#ifndef _H_NETWORK_
#define _H_NETWORK_

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

#endif

