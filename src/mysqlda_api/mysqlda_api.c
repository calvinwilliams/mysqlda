#include "mysqlda_api.h"

int STDCALL mysql_select_library( MYSQL *mysql , const char *library , int *p_index )
{
	char		comm_buffer[ 4096 + 1 ] ;
	int		need_send_len ;
	int		len ;
	int		sended_len ;
	int		comm_body_len ;
	int		received_len ;
	uint32_t	*p = NULL ;
	
	/* 组织请求报文 */
	memset( comm_buffer , 0x00 , sizeof(comm_buffer) );
	len = snprintf( comm_buffer , sizeof(comm_buffer)-1 , "1230\x79%s" , library ) ;
	need_send_len = len+1 ;
	comm_buffer[0] = (len&0xFF) ; len >>= 8 ;
	comm_buffer[1] = (len&0xFF) ; len >>= 8 ;
	comm_buffer[2] = (len&0xFF) ; len >>= 8 ;
	comm_buffer[3] = 0x00 ;
	
	/* 发送请求报文 */
	for( sended_len = 0 ; need_send_len ; )
	{
		len = send( mysql->net.fd , comm_buffer+sended_len , need_send_len , 0 ) ;
		if( len == -1 )
			return -1;
		
		sended_len += len ;
		need_send_len -= len ;
	}
	
	/* 接收响应报文 */
	memset( comm_buffer , 0x00 , sizeof(comm_buffer) );
	comm_body_len = 0 ;
	received_len = 0 ;
	while(1)
	{
		if( comm_body_len == 0 )
		{
			len = recv( mysql->net.fd , comm_buffer+received_len , 3-received_len , 0 ) ;
			if( len == -1 )
				return -1;
			else if( len == 0 )
				return 1;
			
			received_len += len ;
			if( received_len == 3 )
				comm_body_len = comm_buffer[0]+comm_buffer[1]*0xFF+comm_buffer[2]*0xFF*0xFF ;
		}
		else
		{
			len = recv( mysql->net.fd , comm_buffer+received_len , 3+1+comm_body_len-received_len , 0 ) ;
			if( len == -1 )
				return -1;
			else if( len == 0 )
				return 1;
			
			received_len += len ;
			if( received_len == 3+1+comm_body_len )
				break;
		}
	}
	
	/* 解析响应报文 */
	p = (uint32_t*)(comm_buffer+3) ;
	(*p_index) = (int)( ntohl(*p) ) ;
	
	return 0;
}

