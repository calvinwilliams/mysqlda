#include "mysqlda_api.h"

static int mysql_request( MYSQL *mysql , char *comm_buffer , int comm_bufsize , int *p_need_send_len_or_received_len )
{
	int		need_send_len ;
	int		len ;
	int		sended_len ;
	int		comm_body_len ;
	int		received_len ;
	
	/* 发送请求报文 */
	need_send_len = (*p_need_send_len_or_received_len) ;
	for( sended_len = 0 ; need_send_len ; )
	{
		len = send( mysql->net.fd , comm_buffer+sended_len , need_send_len , 0 ) ;
		if( len == -1 )
			return -11;
		
		sended_len += len ;
		need_send_len -= len ;
	}
	
	/* 接收响应报文 */
	memset( comm_buffer , 0x00 , comm_bufsize );
	received_len = 0 ;
	comm_body_len = 0 ;
	while(1)
	{
		if( comm_body_len == 0 )
		{
			len = recv( mysql->net.fd , comm_buffer+received_len , 3-received_len , 0 ) ;
			if( len == -1 )
				return -21;
			else if( len == 0 )
				return -1;
			
			received_len += len ;
			if( received_len == 3 )
				comm_body_len = comm_buffer[0]+comm_buffer[1]*0xFF+comm_buffer[2]*0xFF*0xFF ;
		}
		else
		{
			len = recv( mysql->net.fd , comm_buffer+received_len , 3+1+comm_body_len-received_len , 0 ) ;
			if( len == -1 )
				return -22;
			else if( len == 0 )
				return -1;
			
			received_len += len ;
			if( received_len == 3+1+comm_body_len )
				break;
		}
	}
	
	(*p_need_send_len_or_received_len) = received_len ;
	
	return 0;
}

int STDCALL mysql_select_library( MYSQL *mysql , const char *library , char *instance , int instance_bufsize )
{
	char		comm_buffer[ 4096 + 1 ] ;
	int		comm_buflen ;
	int		len ;
	
	int		nret = 0 ;
	
	/* 组织请求报文 */
	memset( comm_buffer , 0x00 , sizeof(comm_buffer) );
	comm_buflen = 0 ;
	
	comm_buflen += snprintf( comm_buffer , sizeof(comm_buffer)-1 , "1230\x79%s" , library ) + 1 ;
	
	len = comm_buflen - 3 - 1 ;
	comm_buffer[0] = (len&0xFF) ; len >>= 8 ;
	comm_buffer[1] = (len&0xFF) ; len >>= 8 ;
	comm_buffer[2] = (len&0xFF) ; len >>= 8 ;
	comm_buffer[3] = 0x00 ;
	
	/* 发起mysql通讯请求 */
	nret = mysql_request( mysql , comm_buffer , sizeof(comm_buffer) , & comm_buflen ) ;
	if( nret )
		return nret;
	
	/* 解析响应报文 */
	if( instance )
	{
		snprintf( instance , instance_bufsize , "%.*s" , comm_buflen-3-1-1 , comm_buffer+3+1+1 );
	}
	
	return (comm_buffer[4]?1:0);
}

int STDCALL mysql_set_correl_object( MYSQL *mysql , const char *correl_object_class , const char *correl_object , const char *library , char *instance , int instance_bufsize )
{
	char		comm_buffer[ 4096 + 1 ] ;
	int		comm_buflen ;
	int		len ;
	
	int		nret = 0 ;
	
	/* 组织请求报文 */
	memset( comm_buffer , 0x00 , sizeof(comm_buffer) );
	comm_buflen = 0 ;
	
	comm_buflen += snprintf( comm_buffer , sizeof(comm_buffer)-1 , "1230\x80%s" , correl_object_class ) + 1 ;
	comm_buflen += snprintf( comm_buffer+comm_buflen , sizeof(comm_buffer)-1-comm_buflen , "%s" , correl_object ) + 1 ;
	comm_buflen += snprintf( comm_buffer+comm_buflen , sizeof(comm_buffer)-1-comm_buflen , "%s" , library ) + 1 ;
	
	len = comm_buflen - 3 - 1 ;
	comm_buffer[0] = (len&0xFF) ; len >>= 8 ;
	comm_buffer[1] = (len&0xFF) ; len >>= 8 ;
	comm_buffer[2] = (len&0xFF) ; len >>= 8 ;
	comm_buffer[3] = 0x00 ;
	
	/* 发起mysql通讯请求 */
	nret = mysql_request( mysql , comm_buffer , sizeof(comm_buffer) , & comm_buflen ) ;
	if( nret )
		return nret;
	
	/* 解析响应报文 */
	if( instance )
	{
		snprintf( instance , instance_bufsize , "%.*s" , comm_buflen-3-1-1 , comm_buffer+3+1+1 );
	}
	
	return (comm_buffer[4]?1:0);
}

int STDCALL mysql_select_library_by_correl_object( MYSQL *mysql , const char *correl_object_class , const char *correl_object , char *instance , int instance_bufsize )
{
	char		comm_buffer[ 4096 + 1 ] ;
	int		comm_buflen ;
	int		len ;
	
	int		nret = 0 ;
	
	/* 组织请求报文 */
	memset( comm_buffer , 0x00 , sizeof(comm_buffer) );
	comm_buflen = 0 ;
	
	comm_buflen += snprintf( comm_buffer , sizeof(comm_buffer)-1 , "1230\x81%s" , correl_object_class ) + 1 ;
	comm_buflen += snprintf( comm_buffer+comm_buflen , sizeof(comm_buffer)-1-comm_buflen , "%s" , correl_object ) + 1 ;
	
	len = comm_buflen - 3 - 1 ;
	comm_buffer[0] = (len&0xFF) ; len >>= 8 ;
	comm_buffer[1] = (len&0xFF) ; len >>= 8 ;
	comm_buffer[2] = (len&0xFF) ; len >>= 8 ;
	comm_buffer[3] = 0x00 ;
	
	/* 发起mysql通讯请求 */
	nret = mysql_request( mysql , comm_buffer , sizeof(comm_buffer) , & comm_buflen ) ;
	if( nret )
		return nret;
	
	/* 解析响应报文 */
	if( instance )
	{
		snprintf( instance , instance_bufsize , "%.*s" , comm_buflen-3-1-1 , comm_buffer+3+1+1 );
	}
	
	return (comm_buffer[4]?1:0);
}

