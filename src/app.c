#include "mysqlda_in.h"

int FormatHandshakeMessage( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session )
{
	int		len ;
	
	GenerateRandomData( p_accepted_session->random_data , 20 );
	
	/* 初始化通讯缓冲区，跳过通讯头 */
	p_accepted_session->fill_len = 3 ;
	p_accepted_session->process_len = 0 ;
	
	/* 序号 */
	p_accepted_session->comm_buffer[ p_accepted_session->fill_len ] = 0x00 ; p_accepted_session->fill_len++;
	
	/* 协议版本 */
	p_accepted_session->comm_buffer[ p_accepted_session->fill_len ] = 0x0A ; p_accepted_session->fill_len++;
	
	/* 协议文本串 */
	len = sprintf( p_accepted_session->comm_buffer+p_accepted_session->fill_len , "%s" , "5.5.52-MariaDB" ) ; p_accepted_session->fill_len += len+1 ;
	
	/* 连接ID */
	memcpy( p_accepted_session->comm_buffer+p_accepted_session->fill_len , "\x10\x00\x00\x00" , 4 ); p_accepted_session->fill_len += 4 ;
	
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
	p_accepted_session->comm_buffer[0] = (len&0xFF) ; len >>= 8 ;
	p_accepted_session->comm_buffer[1] = (len&0xFF) ; len >>= 8 ;
	p_accepted_session->comm_buffer[2] = (len&0xFF) ; len >>= 8 ;
	
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
	p_accepted_session->process_len = 0 ;
	
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
	p_accepted_session->comm_buffer[0] = (len&0xFF) ; len >>= 8 ;
	p_accepted_session->comm_buffer[1] = (len&0xFF) ; len >>= 8 ;
	p_accepted_session->comm_buffer[2] = (len&0xFF) ; len >>= 8 ;
	
	return 0;
}

int FormatAuthResultOk( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session )
{
	int		len ;
	
	/* 初始化通讯缓冲区，跳过通讯头 */
	p_accepted_session->fill_len = 3 ;
	p_accepted_session->process_len = 0 ;
	
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
	p_accepted_session->comm_buffer[0] = (len&0xFF) ; len >>= 8 ;
	p_accepted_session->comm_buffer[1] = (len&0xFF) ; len >>= 8 ;
	p_accepted_session->comm_buffer[2] = (len&0xFF) ; len >>= 8 ;
	
	return 0;
}

