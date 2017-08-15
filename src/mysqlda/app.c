#include "mysqlda_in.h"

int FormatHandshakeMessage( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session )
{
	int		len ;
	
	GenerateRandomDataWithoutNull( p_accepted_session->random_data , 20 );
	
	/* 初始化通讯缓冲区，跳过通讯头 */
	p_accepted_session->fill_len = 3 ;
	
	/* 序号 */
	p_accepted_session->comm_buffer[ p_accepted_session->fill_len ] = 0x00 ; p_accepted_session->fill_len++;
	
	/* 协议版本 */
	p_accepted_session->comm_buffer[ p_accepted_session->fill_len ] = 0x0A ; p_accepted_session->fill_len++;
	
	/* 协议文本串 */
	len = sprintf( p_accepted_session->comm_buffer+p_accepted_session->fill_len , "%s" , "5.5.52-MariaDB" ) ; p_accepted_session->fill_len += len+1 ;
	
	/* 连接ID */
	memcpy( p_accepted_session->comm_buffer+p_accepted_session->fill_len , "\x6A\x00\x00\x00" , 4 ); p_accepted_session->fill_len += 4 ;
	
	/* 加密串的前半部分 */
	memcpy( p_accepted_session->comm_buffer+p_accepted_session->fill_len , p_accepted_session->random_data , 8 ); p_accepted_session->fill_len += 8 ;
	
	/* 固定填充1个0x00 */
	p_accepted_session->comm_buffer[ p_accepted_session->fill_len ] = 0x00 ; p_accepted_session->fill_len++;
	
	/* 服务端属性的低16位 */
	memcpy( p_accepted_session->comm_buffer+p_accepted_session->fill_len , "\xFF\xF7" , 2 ); p_accepted_session->fill_len += 2 ;
	
	/* 字符集 */
	p_accepted_session->comm_buffer[ p_accepted_session->fill_len ] = 0x08 ; p_accepted_session->fill_len++;
	
	/* 服务器状态 */
	memcpy( p_accepted_session->comm_buffer+p_accepted_session->fill_len , "\x02\x00" , 2 ); p_accepted_session->fill_len += 2 ;
	
	/* 服务端属性的高16位 */
	memcpy( p_accepted_session->comm_buffer+p_accepted_session->fill_len , "\x0F\xA0" , 2 ); p_accepted_session->fill_len += 2 ;
	
	/* 固定填充1个0x00 */
	p_accepted_session->comm_buffer[ p_accepted_session->fill_len ] = 0x15 ; p_accepted_session->fill_len++;
	
	/* 固定填充10个0x00 */
	memset( p_accepted_session->comm_buffer+p_accepted_session->fill_len , 0x00 , 10 ); p_accepted_session->fill_len += 10 ;
	
	/* 加密串的后半部分 */
	memcpy( p_accepted_session->comm_buffer+p_accepted_session->fill_len , p_accepted_session->random_data+8 , 12 ); p_accepted_session->fill_len += 12 ;
	p_accepted_session->comm_buffer[ p_accepted_session->fill_len ] = 0x00 ; p_accepted_session->fill_len++;
	
	/* 未知后缀 */
	len = sprintf( p_accepted_session->comm_buffer+p_accepted_session->fill_len , "%s" , "mysql_native_password" ) ; p_accepted_session->fill_len += len+1 ;
	
	/* 最后 填充通讯头 */
	len = p_accepted_session->fill_len - 3 - 1 ;
	p_accepted_session->comm_body_len = len ;
	p_accepted_session->comm_buffer[0] = (len&0xFF) ; len >>= 8 ;
	p_accepted_session->comm_buffer[1] = (len&0xFF) ; len >>= 8 ;
	p_accepted_session->comm_buffer[2] = (len&0xFF) ; len >>= 8 ;
	p_accepted_session->process_len = 0 ;
	
	return 0;
}

static int CheckMysqlEncryptPassword( char *random_data , char *pass , char *enc_compare )
{
	unsigned char		pass_sha1[ 20 ] ;
	unsigned char		pass_sha1_sha1[ 20 ] ;
	unsigned char		random_data_and_pass_sha1_sha1[ 20 ] ;
	unsigned char		random_data_and_pass_sha1_sha1__sha1[ 20 ] ;
	unsigned char		enc_result[ 20 ] ;
	int			i ;
	
	DEBUGLOG( "--- CHECK MYSQL ENCRYPT PASSWORD --- BEGIN" );
	
	/*
	 * SHA1(password) XOR SHA1("20-bytes random data from server" <concat> SHA1(SHA1(password)))
	 */
	
	DEBUGHEXLOG( pass , strlen(pass) , "pass [%d]bytes" , strlen(pass) );
	
	SHA1( (unsigned char *)pass , strlen(pass) , pass_sha1 );
	DEBUGHEXLOG( (char*)pass_sha1 , 20 , "pass_sha1 [%d]bytes" , 20 );
	
	SHA1( (unsigned char *)pass_sha1 , 20 , pass_sha1_sha1 );
	DEBUGHEXLOG( (char*)pass_sha1_sha1 , 20 , "pass_sha1_sha1 [%d]bytes" , 20 );
	
	memcpy( random_data_and_pass_sha1_sha1 , random_data , 20 );
	memcpy( random_data_and_pass_sha1_sha1+20 , pass_sha1_sha1 , 20 );
	DEBUGHEXLOG( (char*)random_data_and_pass_sha1_sha1 , 40 , "random_data_and_pass_sha1_sha1 [%d]bytes" , 40 );
	
	SHA1( random_data_and_pass_sha1_sha1 , 40 , random_data_and_pass_sha1_sha1__sha1 );
	DEBUGHEXLOG( (char*)random_data_and_pass_sha1_sha1__sha1 , 20 , "random_data_and_pass_sha1_sha1__sha1 [%d]bytes" , 20 );
	
	for( i = 0 ; i < 20 ; i++ )
	{
		enc_result[i] = pass_sha1[i] ^ random_data_and_pass_sha1_sha1__sha1[i] ;
	}
	DEBUGHEXLOG( (char*)enc_result , 20 , "enc_result [%d]bytes" , 20 );
	
	DEBUGHEXLOG( (char*)enc_compare , 20 , "enc_compare [%d]bytes" , 20 );
	DEBUGLOG( "--- CHECK MYSQL ENCRYPT PASSWORD --- END" );
	if( memcmp( enc_result , enc_compare , 20 ) == 0 )
		return 0;
	else
		return 1;
}

int CheckAuthenticationMessage( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session )
{
	char		*p = NULL ;
	
	p = p_accepted_session->comm_buffer + 3 + 1 + 4 + 4 + 1 + 23 ;
	
	/* 用户名 */
	if( STRCMP( p , != , p_env->user ) )
	{
		ERRORLOG( "user[%s] is not matched with config[%s]" , p , p_env->user );
		return 1;
	}
	
	p += strlen(p) + 1 ;
	
	/* 密码串长度 */
	if( (*p) != 0x14 ) /* 20 */
	{
		ERRORLOG( "password length[%d] invalid" , (*p) );
		return 1;
	}
	
	p++;
	
	/* 密码串验证 */
	if( CheckMysqlEncryptPassword( p_accepted_session->random_data , p_env->pass , p ) )
	{
		ERRORLOG( "password not matched" );
		return 1;
	}
	
	p += 20 ;
	
	/* 数据库名 */
	if( STRCMP( p , != , p_env->db ) )
	{
		ERRORLOG( "db[%s] is not matched with config[%s]" , p , p_env->db );
		return 1;
	}
	
	return 0;
}

int FormatAuthResultFail( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session )
{
	int		len ;
	
	/* 初始化通讯缓冲区，跳过通讯头 */
	p_accepted_session->fill_len = 3 ;
	
	/* 序号 */
	p_accepted_session->comm_buffer[ p_accepted_session->fill_len ] = 0x02 ; p_accepted_session->fill_len++;
	
	/* 状态标识 */
	p_accepted_session->comm_buffer[ p_accepted_session->fill_len ] = 0xFF ; p_accepted_session->fill_len++;
	
	/* 错误码 */
	memcpy( p_accepted_session->comm_buffer+p_accepted_session->fill_len , "\x15\x04" , 2 ); p_accepted_session->fill_len += 2 ;
	
	/* 固定1个字节 */
	p_accepted_session->comm_buffer[ p_accepted_session->fill_len ] = 0x23 ; p_accepted_session->fill_len++;
	
	/* 固定5个字节 */
	memcpy( p_accepted_session->comm_buffer+p_accepted_session->fill_len , "\x32\x38\x30\x30\x30" , 5 ); p_accepted_session->fill_len += 5 ;
	
	/* 出错信息 */
	len = sprintf( p_accepted_session->comm_buffer+p_accepted_session->fill_len , "Access denied for user '%s' (using password: YES)" , p_env->user ) ; p_accepted_session->fill_len += len+1 ;
	
	/* 最后 填充通讯头 */
	len = p_accepted_session->fill_len - 3 - 1 ;
	p_accepted_session->comm_body_len = len ;
	p_accepted_session->comm_buffer[0] = (len&0xFF) ; len >>= 8 ;
	p_accepted_session->comm_buffer[1] = (len&0xFF) ; len >>= 8 ;
	p_accepted_session->comm_buffer[2] = (len&0xFF) ; len >>= 8 ;
	p_accepted_session->process_len = 0 ;
	
	return 0;
}

int FormatAuthResultOk( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session )
{
	int		len ;
	
	/* 初始化通讯缓冲区，跳过通讯头 */
	p_accepted_session->fill_len = 3 ;
	
	/* 序号 */
	p_accepted_session->comm_buffer[ p_accepted_session->fill_len ] = 0x02 ; p_accepted_session->fill_len++;
	
	/* 状态标识 */
	p_accepted_session->comm_buffer[ p_accepted_session->fill_len ] = 0x00 ; p_accepted_session->fill_len++;
	
	/* 影响行数 */
	p_accepted_session->comm_buffer[ p_accepted_session->fill_len ] = 0x00 ; p_accepted_session->fill_len++;
	
	/* LastInsertId */
	p_accepted_session->comm_buffer[ p_accepted_session->fill_len ] = 0x00 ; p_accepted_session->fill_len++;
	
	/* 状态 */
	memcpy( p_accepted_session->comm_buffer+p_accepted_session->fill_len , "\x02\x00" , 2 ); p_accepted_session->fill_len += 2 ;
	
	/* 消息 */
	memcpy( p_accepted_session->comm_buffer+p_accepted_session->fill_len , "\x00\x00" , 2 ); p_accepted_session->fill_len += 2 ;
	
	/* 最后 填充通讯头 */
	len = p_accepted_session->fill_len - 3 - 1 ;
	p_accepted_session->comm_body_len = len ;
	p_accepted_session->comm_buffer[0] = (len&0xFF) ; len >>= 8 ;
	p_accepted_session->comm_buffer[1] = (len&0xFF) ; len >>= 8 ;
	p_accepted_session->comm_buffer[2] = (len&0xFF) ; len >>= 8 ;
	p_accepted_session->process_len = 0 ;
	
	return 0;
}

int DatabaseSelectLibrary( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session )
{
	char			library[ MAXLEN_LIBRARY + 1 ] ;
	int			library_len ;
	struct ForwardSession	*p_forward_session = NULL ;
	unsigned long		hash_val = 0 ;
	struct ForwardInstance	*p_forward_instance = NULL ;
	struct ForwardServer	*p_first_forward_server = NULL ;
	struct ForwardLibrary	forward_library ;
	struct ForwardLibrary	*p_forward_library = NULL ;
	signed char		new_forward_session_flag ;
	
	int			fd ;
	char			*p_date_time_string = NULL ;
	
	int			len ;
	
	int			nret = 0 ;
	
	if( p_accepted_session->comm_body_len-1 > MAXLEN_LIBRARY )
	{
		ERRORLOG( "library[%.*s] too long" , p_accepted_session->comm_body_len-1 , p_accepted_session->comm_buffer+5 );
		return 1;
	}
	memset( library , 0x00 , sizeof(library) );
	sprintf( library , "%.*s" , p_accepted_session->comm_body_len-1 , p_accepted_session->comm_buffer+5 );
	library_len = strlen(library) ;
	
	if( p_accepted_session->p_pair_forward_session == NULL )
	{
		new_forward_session_flag = 1 ;
		
		p_forward_session = (struct ForwardSession *)malloc( sizeof(struct ForwardSession) ) ;
		if( p_forward_session == NULL )
		{
			ERRORLOG( "malloc failed , errno[%d]" , errno );
			return 1;
		}
		else
		{
			INFOLOG( "malloc ok , p_forward_session[%p]" , p_forward_session );
		}
		memset( p_forward_session , 0x00 , sizeof(struct ForwardSession) );
		
		p_accepted_session->p_pair_forward_session = p_forward_session ;
		
		p_forward_session->type = SESSIONTYPE_FORWARDSESSION ;
		p_accepted_session->p_pair_forward_session = p_forward_session ;
		p_forward_session->p_pair_accepted_session = p_accepted_session ;
	}
	else
	{
		new_forward_session_flag = 0 ;
		
		p_forward_session = p_accepted_session->p_pair_forward_session ;
	}
	
	memset( & forward_library , 0x00 , sizeof(struct ForwardLibrary) );
	strcpy( forward_library.library , library );
	p_forward_library = QueryForwardLibraryTreeNode( p_env , & forward_library ) ;
	if( p_forward_library == NULL )
	{
		hash_val = CalcHash( library , library_len ) % (p_env->total_power) ;
		INFOLOG( "library[%s] total_power[%d] hash_val[%d]" , library , p_env->total_power , hash_val );
		p_forward_instance = QueryForwardSerialRangeTreeNode( p_env , hash_val ) ;
		INFOLOG( "library[%s] p_forward_instance[%p][%s]" , library , p_forward_instance , (p_forward_instance?p_forward_instance->instance:"") );
	}
	else
	{
		p_forward_instance = p_forward_library->p_forward_instance ;
	}
	
	if( p_forward_session->p_forward_instance != p_forward_instance )
	{
		p_forward_session->p_forward_instance = p_forward_instance ;
		
		if( p_forward_session->mysql_connection )
		{
			DeleteForwardSessionEpoll( p_env , p_forward_session );
			INFOLOG( "[%s] #%d# mysql_close[%s][%d] ok" , p_forward_session->p_forward_instance->instance , p_forward_session->mysql_connection->net.fd , p_forward_session->p_forward_server->netaddr.ip , p_forward_session->p_forward_server->netaddr.port );
			mysql_close( p_forward_session->mysql_connection );
		}
		
		p_forward_session->mysql_connection = mysql_init( NULL ) ;
		if( p_forward_session->mysql_connection == NULL )
		{
			ERRORLOG( "mysql_init failed , errno[%d]" , errno );
			return 1;
		}
		
		p_forward_session->p_forward_server = lk_list_first_entry( & (p_forward_instance->forward_server_list) , struct ForwardServer , forward_server_listnode ) ;
		p_first_forward_server = p_forward_session->p_forward_server ;
		while(1)
		{
			INFOLOG( "[%s]mysql_real_connect[%s][%d][%s][%s][%s] connecting ..." , p_forward_session->p_forward_instance->instance , p_forward_session->p_forward_server->netaddr.ip , p_forward_session->p_forward_server->netaddr.port , p_env->user , p_env->pass , p_env->db );
			if( mysql_real_connect( p_forward_session->mysql_connection , p_forward_session->p_forward_server->netaddr.ip , p_env->user , p_env->pass , p_env->db , p_forward_session->p_forward_server->netaddr.port , NULL , 0 ) == NULL )
			{
				ERRORLOG( "[%s] mysql_real_connect[%s][%d][%s][%s][%s] failed , mysql_errno[%d][%s]" , p_forward_session->p_forward_instance->instance , p_forward_session->p_forward_server->netaddr.ip , p_forward_session->p_forward_server->netaddr.port , p_env->user , p_env->pass , p_env->db , mysql_errno(p_forward_session->mysql_connection) , mysql_error(p_forward_session->mysql_connection) );
			}
			else
			{
				INFOLOG( "[%s] #%d# mysql_real_connect[%s][%d][%s][%s][%s] connecting ok" , p_forward_session->p_forward_instance->instance , p_forward_session->mysql_connection->net.fd , p_forward_session->p_forward_server->netaddr.ip , p_forward_session->p_forward_server->netaddr.port , p_env->user , p_env->pass , p_env->db );
				break;
			}
			
			lk_list_move_tail( & (p_forward_session->p_forward_server->forward_server_listnode) , & (p_forward_instance->forward_server_list) );
			
			p_forward_session->p_forward_server = lk_list_first_entry( & (p_forward_instance->forward_server_list) , struct ForwardServer , forward_server_listnode ) ;
			if( p_forward_session->p_forward_server == p_first_forward_server )
			{
				ERRORLOG( "all mysql_real_connect failed" );
				return 1;
			}
		}
		
		if( new_forward_session_flag == 1 )
		{
			lk_list_add_tail( & (p_forward_session->forward_session_listnode) , & (p_forward_session->p_forward_server->forward_session_list) );
		}
		
		AddForwardSessionEpollInput( p_env , p_forward_session );
	}
	
	if( p_forward_library == NULL )
	{
		p_forward_library = (struct ForwardLibrary *)malloc( sizeof(struct ForwardLibrary) ) ;
		if( p_forward_library == NULL )
		{
			ERRORLOG( "malloc failed , errno[%d]" , errno );
			return 1;
		}
		memset( p_forward_library , 0x00 , sizeof(struct ForwardLibrary) );
		
		strcpy( p_forward_library->library , library );
		p_forward_library->p_forward_instance = p_forward_session->p_forward_instance ;
		
		nret = LinkForwardLibraryTreeNode( p_env , p_forward_library ) ;
		if( nret )
		{
			ERRORLOG( "LinkForwardLibraryTreeNode failed[%d]" , nret );
			free( p_forward_library );
			return 1;
		}
		else
		{
			INFOLOG( "LinkForwardLibraryTreeNode ok , library[%s] instance[%s]" , p_forward_library->library , p_forward_library->p_forward_instance->instance );
		}
		
		fd = open( p_env->save_filename , O_CREAT|O_WRONLY|O_APPEND , 00777 ) ;
		if( fd == -1 )
		{
			ERRORLOG( "open[%s] failed[%d]" , p_env->save_filename , nret );
			free( p_forward_library );
			return 1;
		}
		
		p_date_time_string = GetLogLastDateTimeStringPtr() ;
		write( fd , p_date_time_string , strlen(p_date_time_string) );
		write( fd , " " , 1 );
		write( fd , p_forward_library->library , strlen(p_forward_library->library) );
		write( fd , " " , 1 );
		write( fd , p_forward_library->p_forward_instance->instance , strlen(p_forward_library->p_forward_instance->instance) );
		write( fd , "\n" , 1 );
		
		close( fd );
		
		AddForwardInstanceTreeNodePower( p_env , p_forward_instance );
		
		p_forward_instance = NULL ;
		while(1)
		{
			p_forward_instance = TravelForwardSerialRangeTreeNode( p_env , p_forward_instance ) ;
			if( p_forward_instance == NULL )
				break;
			
			INFOLOG( "instance[%s] serial_range_begin[%lu] power[%lu]\n" , p_forward_instance->instance , p_forward_instance->serial_range_begin , p_forward_instance->power );
		}
		
		INFOLOG( "total_power[%ld]" , p_env->total_power );
	}
	
	/* 初始化通讯缓冲区，跳过通讯头 */
	p_accepted_session->fill_len = 3 ;
	
	/* 序号 */
	p_accepted_session->comm_buffer[ p_accepted_session->fill_len ] = 0x01 ; p_accepted_session->fill_len++;
	
	/* 存储索引 */
	len = sprintf( p_accepted_session->comm_buffer+p_accepted_session->fill_len , "%s" , p_forward_library->p_forward_instance->instance ) ; p_accepted_session->fill_len += len+1 ;
	
	/* 最后 填充通讯头 */
	len = p_accepted_session->fill_len - 3 - 1 ;
	p_accepted_session->comm_body_len = len ;
	p_accepted_session->comm_buffer[0] = (len&0xFF) ; len >>= 8 ;
	p_accepted_session->comm_buffer[1] = (len&0xFF) ; len >>= 8 ;
	p_accepted_session->comm_buffer[2] = (len&0xFF) ; len >>= 8 ;
	p_accepted_session->process_len = 0 ;
	
	return 0;
}

