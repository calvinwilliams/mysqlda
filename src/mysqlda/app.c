#include "mysqlda_in.h"

/* 填充转发端发给客户端的握手分组 */
int FormatHandshakeMessage( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session )
{
	char		*p = NULL ;
#if 0
	int		len ;
#endif
	
	GenerateRandomDataWithoutNull( p_accepted_session->random_data , 20 );
	
#if 0
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
#endif
	memcpy( p_accepted_session->comm_buffer , p_env->handshake_request_message , 4+p_env->handshake_request_message_length );
	p_accepted_session->comm_body_len = p_env->handshake_request_message_length ;
	p_accepted_session->fill_len = 3+1+p_accepted_session->comm_body_len ;
	
	p = p_accepted_session->comm_buffer + 3 + 1 + 1 ;
	p += strlen(p) + 1 ;
	p += 4 ;
	memcpy( p , p_accepted_session->random_data , 8 ); p += 8 ;
	p += 1 + 2 + 1 + 2 + 2 + 1 + 10 ;
	memcpy( p , p_accepted_session->random_data+8 , 12 );
	
	p_accepted_session->process_len = 0 ;
	
	return 0;
}

/* 密码串验证 */
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

/* 分解、检查客户端发给转发端的认证分组 */
int CheckAuthenticationMessage( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session )
{
	unsigned int	client_options ;
	char		*p = NULL ;
	
	client_options = MYSQL_OPTIONS_2(p_accepted_session->comm_buffer+3+1) ;
	
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
	
	if( (client_options&CLIENT_CONNECT_WITH_DB) == 0 )
		return 0;
	
	/* 数据库名 */
	if( STRCMP( p , != , p_env->db ) )
	{
		ERRORLOG( "db[%s] is not matched with config[%s]" , p , p_env->db );
		return 1;
	}
	
	return 0;
}

/* 填充转发端发给客户端的认证失败分组 */
int FormatAuthResultFail( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session , unsigned char serial_no , char *format , ... )
{
	va_list		valist ;
	int		len ;
	
	/* 初始化通讯缓冲区，跳过通讯头 */
	p_accepted_session->fill_len = 3 ;
	
	/* 序号 */
	p_accepted_session->comm_buffer[ p_accepted_session->fill_len ] = serial_no ; p_accepted_session->fill_len++;
	/* 状态标识 */
	p_accepted_session->comm_buffer[ p_accepted_session->fill_len ] = 0xFF ; p_accepted_session->fill_len++;
	/* 错误码 */
	memcpy( p_accepted_session->comm_buffer+p_accepted_session->fill_len , "\x15\x04" , 2 ); p_accepted_session->fill_len += 2 ;
	/* 固定1个字节 */
	p_accepted_session->comm_buffer[ p_accepted_session->fill_len ] = 0x23 ; p_accepted_session->fill_len++;
	/* 固定5个字节 */
	memcpy( p_accepted_session->comm_buffer+p_accepted_session->fill_len , "\x32\x38\x30\x30\x30" , 5 ); p_accepted_session->fill_len += 5 ;
	/* 出错信息 */
	va_start( valist , format );
	len = vsnprintf( p_accepted_session->comm_buffer+p_accepted_session->fill_len , p_accepted_session->comm_bufsize-1-p_accepted_session->fill_len , format , valist ) ; p_accepted_session->fill_len += len+1 ;
	va_end( valist );
	
	/* 最后 填充通讯头 */
	len = p_accepted_session->fill_len - 3 - 1 ;
	p_accepted_session->comm_body_len = len ;
	p_accepted_session->comm_buffer[0] = (len&0xFF) ; len >>= 8 ;
	p_accepted_session->comm_buffer[1] = (len&0xFF) ; len >>= 8 ;
	p_accepted_session->comm_buffer[2] = (len&0xFF) ; len >>= 8 ;
	p_accepted_session->process_len = 0 ;
	
	return 0;
}

/* 填充转发端发给客户端的认证成功分组 */
int FormatAuthResultOk( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session , unsigned char serial_no )
{
	int		len ;
	
	/* 初始化通讯缓冲区，跳过通讯头 */
	p_accepted_session->fill_len = 3 ;
	
	/* 序号 */
	p_accepted_session->comm_buffer[ p_accepted_session->fill_len ] = serial_no ; p_accepted_session->fill_len++;
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

/* 填充服务端的查询版本号分组 */
int FormatSelectVersionCommentResponse( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session )
{
	memcpy( p_accepted_session->comm_buffer , p_env->select_version_comment_response_message , 4+p_env->select_version_comment_response_message_length );
	p_accepted_session->fill_len = 4+p_env->select_version_comment_response_message_length ;
	memcpy( p_accepted_session->comm_buffer+p_accepted_session->fill_len , p_env->select_version_comment_response_message2 , 4+p_env->select_version_comment_response_message2_length );
	p_accepted_session->fill_len += 4+p_env->select_version_comment_response_message2_length ;
	memcpy( p_accepted_session->comm_buffer+p_accepted_session->fill_len , p_env->select_version_comment_response_message3 , 4+p_env->select_version_comment_response_message3_length );
	p_accepted_session->fill_len += 4+p_env->select_version_comment_response_message3_length ;
	memcpy( p_accepted_session->comm_buffer+p_accepted_session->fill_len , p_env->select_version_comment_response_message4 , 4+p_env->select_version_comment_response_message4_length );
	p_accepted_session->fill_len += 4+p_env->select_version_comment_response_message4_length ;
	memcpy( p_accepted_session->comm_buffer+p_accepted_session->fill_len , p_env->select_version_comment_response_message5 , 4+p_env->select_version_comment_response_message5_length );
	p_accepted_session->fill_len += 4+p_env->select_version_comment_response_message5_length ;
	
	p_accepted_session->comm_body_len = p_accepted_session->fill_len-3-1 ;
	p_accepted_session->process_len = 0 ;
	
	return 0;
}

/* 选择指定服务端转发库的服务器 */
static int SelectMysqlServer( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session , struct ForwardInstance *p_forward_instance , struct ForwardSession **pp_forward_session )
{
	struct ForwardServer	*p_forward_server = NULL ;
	struct ForwardSession	forward_session ;
	struct ForwardSession	*p_forward_session = NULL ;
	
	/* 遍历所有服务器 */
	lk_list_for_each_entry( p_forward_server , & (p_forward_instance->forward_server_list) , struct ForwardServer , forward_server_listnode )
	{
		if( ! lk_list_empty( & (p_forward_server->unused_forward_session_list) ) )
		{
			/* 如果有缓存会话，则复用之 */
			p_forward_session = lk_list_first_entry( & (p_forward_server->unused_forward_session_list) , struct ForwardSession , unused_forward_session_listnode ) ;
			lk_list_del_init( & (p_forward_session->unused_forward_session_listnode) );
			lk_list_add_tail( & (p_forward_session->forward_session_listnode) , & (p_forward_server->forward_session_list) );
			DEBUGLOG( "move p_forward_session[0x%X] from unused_forward_session_list to forward_session_list" , p_forward_session );
			
			AddForwardSessionEpollInput( p_env , p_forward_session );
			
			break;
		}
		else
		{
			/* 如果没有缓存会话，生成一个新会话 */
			memset( & forward_session , 0x00 , sizeof(struct ForwardSession) );
			forward_session.type = SESSIONTYPE_FORWARDSESSION ;
			forward_session.p_forward_instance = p_forward_instance ;
			forward_session.p_forward_server = p_forward_server ;
			forward_session.p_pair_accepted_session = p_accepted_session ;
			
			forward_session.mysql_connection = mysql_init( NULL ) ;
			if( forward_session.mysql_connection == NULL )
			{
				ERRORLOG( "mysql_init failed , errno[%d]" , errno );
				return 1;
			}
			
			INFOLOG( "[%s]mysql_real_connect[%s][%d][%s][%s][%s] connecting ..." , forward_session.p_forward_instance->instance , forward_session.p_forward_server->netaddr.ip , forward_session.p_forward_server->netaddr.port , p_env->user , p_env->pass , p_env->db );
			if( mysql_real_connect( forward_session.mysql_connection , forward_session.p_forward_server->netaddr.ip , p_env->user , p_env->pass , p_env->db , forward_session.p_forward_server->netaddr.port , NULL , 0 ) == NULL )
			{
				ERRORLOG( "[%s] mysql_real_connect[%s][%d][%s][%s][%s] failed , mysql_errno[%d][%s]" , forward_session.p_forward_instance->instance , forward_session.p_forward_server->netaddr.ip , forward_session.p_forward_server->netaddr.port , p_env->user , p_env->pass , p_env->db , mysql_errno(forward_session.mysql_connection) , mysql_error(forward_session.mysql_connection) );
			}
			else
			{
				INFOLOG( "[%s] #%d# mysql_real_connect[%s][%d][%s][%s][%s] connecting ok" , forward_session.p_forward_instance->instance , forward_session.mysql_connection->net.fd , forward_session.p_forward_server->netaddr.ip , forward_session.p_forward_server->netaddr.port , p_env->user , p_env->pass , p_env->db );
				
				p_forward_session = (struct ForwardSession *)malloc( sizeof(struct ForwardSession) ) ;
				if( p_forward_session == NULL )
				{
					ERRORLOG( "malloc failed , errno[%d]" , errno );
					return 1;
				}
				else
				{
					DEBUGLOG( "malloc ok , p_forward_session[%p]" , p_forward_session );
				}
				memcpy( p_forward_session , & forward_session , sizeof(struct ForwardSession) );
				p_accepted_session->p_pair_forward_session = p_forward_session ;
				
				lk_list_add_tail( & (p_forward_session->forward_session_listnode) , & (p_forward_server->forward_session_list) );
				
				AddForwardSessionEpollInput( p_env , p_forward_session );
				
				break;
			}
		}
	}
	if( & (p_forward_server->forward_session_list) == & (p_forward_instance->forward_server_list) )
	{
		ERRORLOG( "all mysql_real_connect failed" );
		return 1;
	}
	
	(*pp_forward_session) = p_forward_session ;
	return 0;
}

/* 查询 服务端转发库分组 */
int SelectDatabaseLibrary( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session , char *library , int library_len )
{
	struct ForwardLibrary	forward_library ;
	unsigned long		hash_val = 0 ;
	struct ForwardLibrary	*p_forward_library = NULL ;
	struct ForwardInstance	*p_forward_instance = NULL ;
	struct ForwardInstance	*p_travel_forward_instance = NULL ;
	
	int			fd ;
	char			*p_date_time_string = NULL ;
	
	int			nret = 0 ;
	
	/* 提取参数 */
	if( library_len > MAXLEN_LIBRARY )
	{
		ERRORLOG( "library[%.*s] too long" , library_len , library );
		return 1;
	}
	strcpy( forward_library.library , library );
	
	/* 查询服务端转发规则 */
	p_forward_library = QueryForwardLibraryTreeNode( p_env , & forward_library ) ;
	if( p_forward_library == NULL )
	{
		/* 历史中没有该规则，新建之 */
		hash_val = CalcHash( forward_library.library , library_len ) % (p_env->total_power) ;
		INFOLOG( "library[%s] total_power[%d] hash_val[%d]" , forward_library.library , p_env->total_power , hash_val );
		p_forward_instance = QueryForwardSerialRangeTreeNode( p_env , hash_val ) ;
		INFOLOG( "library[%s] p_forward_instance[%p][%s]" , forward_library.library , p_forward_instance , (p_forward_instance?p_forward_instance->instance:"") );
		
		p_forward_library = (struct ForwardLibrary *)malloc( sizeof(struct ForwardLibrary) ) ;
		if( p_forward_library == NULL )
		{
			ERRORLOG( "malloc failed , errno[%d]" , errno );
			return 1;
		}
		memset( p_forward_library , 0x00 , sizeof(struct ForwardLibrary) );
		
		strcpy( p_forward_library->library , forward_library.library );
		p_forward_library->p_forward_instance = p_forward_instance ;
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
		
		IncreaseForwardInstanceTreeNodePower( p_env , p_forward_library->p_forward_instance );
		
		/* 持久化规则到硬盘 */
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
		
		/* 输出服务端库权重到日志 */
		p_travel_forward_instance = NULL ;
		while(1)
		{
			p_travel_forward_instance = TravelForwardSerialRangeTreeNode( p_env , p_travel_forward_instance ) ;
			if( p_travel_forward_instance == NULL )
				break;
			
			INFOLOG( "instance[%s] serial_range_begin[%lu] power[%lu]" , p_travel_forward_instance->instance , p_travel_forward_instance->serial_range_begin , p_travel_forward_instance->power );
		}
		
		INFOLOG( "total_power[%ld]" , p_env->total_power );
	}
	else
	{
		p_forward_instance = p_forward_library->p_forward_instance ;
	}
	
	if( p_accepted_session->p_pair_forward_session == NULL )
	{
		/* 如果还没有服务端转发会话 与客户端连接会话 关联，直接挑选mysql服务器 */
		INFOLOG( "p_pair_forward_session null" );
		
		nret = SelectMysqlServer( p_env , p_accepted_session , p_forward_instance , & (p_accepted_session->p_pair_forward_session) ) ;
		if( nret )
		{
			ERRORLOG( "SelectMysqlServer failed" );
			return 1;
		}
	}
	else if( p_accepted_session->p_pair_forward_session->p_forward_instance != p_forward_instance )
	{
		/* 如果有服务端转发会话 与客户端连接会话 关联，且新老转发库不一致，则先把当前转发会话移到缓存中 */
		INFOLOG( "p_pair_forward_session not null" );
		
		lk_list_del_init( & (p_accepted_session->p_pair_forward_session->forward_session_listnode) );
		p_accepted_session->p_pair_forward_session->close_unused_forward_session_timestamp = time(NULL) + p_env->unused_forward_session_timeout ;
		lk_list_add_tail( & (p_accepted_session->p_pair_forward_session->unused_forward_session_listnode) , & (p_accepted_session->p_pair_forward_session->p_forward_server->unused_forward_session_list) );
		INFOLOG( "move p_forward_session[0x%X] [%d] from forward_session_list to unused_forward_session_list" , p_accepted_session->p_pair_forward_session , p_accepted_session->p_pair_forward_session->close_unused_forward_session_timestamp );
		
		DeleteForwardSessionEpoll( p_env , p_accepted_session->p_pair_forward_session );
		
		nret = SelectMysqlServer( p_env , p_accepted_session , p_forward_instance , & (p_accepted_session->p_pair_forward_session) ) ;
		if( nret )
		{
			ERRORLOG( "SelectMysqlServer failed" );
			return 1;
		}
	}
	
	return 0;
}

/* 设置 关联对象对应的服务端转发库 */
int SetDatabaseCorrelObject( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session , char *correl_object_class , int correl_object_class_len , char *correl_object , int correl_object_len , char *library , int library_len )
{
	struct ForwardCorrelObjectClass	forward_correl_object_class ;
	struct ForwardCorrelObjectClass	*p_forward_correl_object_class = NULL ;
	struct ForwardCorrelObject	forward_correl_object ;
	struct ForwardCorrelObject	*p_forward_correl_object = NULL ;
	struct ForwardLibrary		forward_library ;
	struct ForwardLibrary		*p_forward_library = NULL ;
	
	char				correl_object_class_save_pathfilename[ 256 + 1 ] ;
	int				fd ;
	char				*p_date_time_string = NULL ;
	
	int				nret = 0 ;
	
	/* 提取参数 */
	if( correl_object_class_len > MAXLEN_CORRELOBJECT_CLASS )
	{
		ERRORLOG( "correl_object_class[%.*s] too long" , correl_object_class_len , correl_object_class );
		return 1;
	}
	strcpy( forward_correl_object_class.correl_object_class , correl_object_class );
	
	if( correl_object_len > MAXLEN_CORRELOBJECT )
	{
		ERRORLOG( "correl_object[%.*s] too long" , correl_object_len , correl_object );
		return 1;
	}
	strcpy( forward_correl_object.correl_object , correl_object );
	
	if( library_len > MAXLEN_CORRELOBJECT )
	{
		ERRORLOG( "library[%.*s] too long" , library_len , library );
		return 1;
	}
	strcpy( forward_library.library , library );
	
	/* 查询服务端转发关联对象类树 */
	p_forward_correl_object_class = QueryForwardCorrelObjectClassTreeNode( p_env , & forward_correl_object_class ) ;
	if( p_forward_correl_object_class == NULL )
	{
		/* 创建服务端转发关联对象类 */
		p_forward_correl_object_class = (struct ForwardCorrelObjectClass *)malloc( sizeof(struct ForwardCorrelObjectClass) ) ;
		if( p_forward_correl_object_class == NULL )
		{
			ERRORLOG( "malloc failed , errno[%d]" , errno );
			return 1;
		}
		memset( p_forward_correl_object_class , 0x00 , sizeof(struct ForwardCorrelObjectClass) );
		strcpy( p_forward_correl_object_class->correl_object_class , correl_object_class );
		
		/* 挂接服务端转发关联对象类 */
		nret = LinkForwardCorrelObjectClassTreeNode( p_env , p_forward_correl_object_class ) ;
		if( nret )
		{
			ERRORLOG( "LinkForwardCorrelObjectClassTreeNode failed[%d]" , nret );
			return 1;
		}
		else
		{
			DEBUGLOG( "LinkForwardCorrelObjectClassTreeNode ok" );
		}
	}
	
	/* 查询服务端转发关联对象树 */
	p_forward_correl_object = QueryForwardCorrelObjectTreeNode( p_forward_correl_object_class , & forward_correl_object ) ;
	if( p_forward_correl_object == NULL )
	{
		/* 创建服务端转发关联对象 */
		p_forward_correl_object = (struct ForwardCorrelObject *)malloc( sizeof(struct ForwardCorrelObject) ) ;
		if( p_forward_correl_object == NULL )
		{
			ERRORLOG( "malloc failed , errno[%d]" , errno );
			return 1;
		}
		memset( p_forward_correl_object , 0x00 , sizeof(struct ForwardCorrelObject) );
		strcpy( p_forward_correl_object->correl_object , correl_object );
		
		/* 挂接服务端转发关联对象 */
		nret = LinkForwardCorrelObjectTreeNode( p_forward_correl_object_class , p_forward_correl_object ) ;
		if( nret )
		{
			ERRORLOG( "LinkForwardCorrelObjectTreeNode failed[%d]" , nret );
			return 1;
		}
		else
		{
			DEBUGLOG( "LinkForwardCorrelObjectTreeNode ok" );
		}
	}
	
	/* 查询服务端转发规则树 */
	p_forward_library = QueryForwardLibraryTreeNode( p_env , & forward_library ) ;
	if( p_forward_library == NULL )
	{
		ERRORLOG( "library[%.*s] not found" , library_len , library );
		return 1;
	}
	else
	{
		INFOLOG( "library[%.*s] found" , library_len , library );
		
		if( p_forward_correl_object->p_forward_library == NULL )
		{
			/* 持久化关联规则到硬盘 */
			memset( correl_object_class_save_pathfilename , 0x00 , sizeof(correl_object_class_save_pathfilename) );
			snprintf( correl_object_class_save_pathfilename , sizeof(correl_object_class_save_pathfilename)-1 , "mysqlda.%s.save" , p_forward_correl_object_class->correl_object_class );
			fd = open( correl_object_class_save_pathfilename , O_CREAT|O_WRONLY|O_APPEND , 00777 ) ;
			if( fd == -1 )
			{
				ERRORLOG( "open[%s] failed[%d]" , p_env->save_filename , nret );
				p_forward_library = NULL ;
			}
			else
			{
				p_date_time_string = GetLogLastDateTimeStringPtr() ;
				write( fd , p_date_time_string , strlen(p_date_time_string) );
				write( fd , " " , 1 );
				write( fd , p_forward_correl_object->correl_object , strlen(p_forward_correl_object->correl_object) );
				write( fd , " " , 1 );
				write( fd , p_forward_library->library , strlen(p_forward_library->library) );
				write( fd , "\n" , 1 );
				
				close( fd );
				
				p_forward_correl_object->p_forward_library = p_forward_library ;
			}
		}
		else
		{
			if( p_forward_correl_object->p_forward_library != p_forward_library )
				p_forward_library = NULL ;
		}
	}
	
	return 0;
}

/* 查询 关联对象对应的服务端转发库分组 */
int SelectDatabaseLibraryByCorrelObject( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session , char *correl_object_class , int correl_object_class_len , char *correl_object , int correl_object_len )
{
	struct ForwardCorrelObjectClass	forward_correl_object_class ;
	struct ForwardCorrelObjectClass	*p_forward_correl_object_class = NULL ;
	struct ForwardCorrelObject	forward_correl_object ;
	struct ForwardCorrelObject	*p_forward_correl_object = NULL ;
	
	/* 提取参数 */
	if( correl_object_class_len > MAXLEN_CORRELOBJECT_CLASS )
	{
		ERRORLOG( "correl_object_class[%.*s] too long" , correl_object_class_len , correl_object_class );
		return 1;
	}
	strcpy( forward_correl_object_class.correl_object_class , correl_object_class );
	
	if( correl_object_len > MAXLEN_CORRELOBJECT )
	{
		ERRORLOG( "correl_object[%.*s] too long" , correl_object_len , correl_object );
		return 1;
	}
	strcpy( forward_correl_object.correl_object , correl_object );
	
	/* 查询服务端转发关联对象类树 */
	p_forward_correl_object_class = QueryForwardCorrelObjectClassTreeNode( p_env , & forward_correl_object_class ) ;
	if( p_forward_correl_object_class )
	{
		/* 查询服务端转发关联对象树 */
		p_forward_correl_object = QueryForwardCorrelObjectTreeNode( p_forward_correl_object_class , & forward_correl_object ) ;
		if( p_forward_correl_object )
		{
			if( p_forward_correl_object->p_forward_library )
			{
				INFOLOG( "select library[%s]" , p_forward_correl_object->p_forward_library->library );
				return SelectDatabaseLibrary( p_env , p_accepted_session , p_forward_correl_object->p_forward_library->library , strlen(p_forward_correl_object->p_forward_library->library) );
			}
		}
	}
	
	return 1;
}

