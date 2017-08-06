#include "mysqlda_in.h"

int InitConfigFile( struct MysqldaEnvironment *p_env )
{
	mysqlda_conf			conf ;
	char				*file_buffer = NULL ;
	int				file_size ;
	int				nret = 0 ;
	
	memset( & conf , 0x00 , sizeof(mysqlda_conf) );
	
	strcpy( conf.server.listen_ip , "127.0.0.1" );
	conf.server.listen_port = 3306 ;
	
	strcpy( conf.auth.user , "calvin" );
	strcpy( conf.auth.pass , "calvin" );
	strcpy( conf.auth.db , "calvindb" );
	
	strcpy( conf.forward[0].instance , "mysqlda1" );
	strcpy( conf.forward[0].ip , "localhost" );
	conf.forward[0].port = 13306 ;
	conf.forward[0].power = 1 ;
	conf._forward_count = 1 ;
	
	nret = DSCSERIALIZE_JSON_DUP_mysqlda_conf( & conf , "GB18030" , & file_buffer , NULL , & file_size ) ;
	if( nret )
	{
		printf( "*** ERROR : DSCSERIALIZE_JSON_DUP_mysqlda_conf not found\n" );
		return -1;
	}
	
	nret = WriteEntireFile( p_env->config_filename , file_buffer , file_size ) ;
	if( nret )
	{
		printf( "*** ERROR : WriteEntireFile[%s] failed[%d] , errno[%d]\n" , p_env->config_filename , nret , errno );
		free( file_buffer );
		return -1;
	}
	
	free( file_buffer );
	
	return 0;
}

int LoadConfig( struct MysqldaEnvironment *p_env )
{
	char			*file_buffer = NULL ;
	int			file_size ;
	mysqlda_conf		conf ;
	int			forward_no ;
	struct ForwardPower	*p_forward_power = NULL ;
	int			serial_range_begin ;
	int			nret = 0 ;
	
	file_buffer = StrdupEntireFile( p_env->config_filename , & file_size ) ;
	if( file_buffer == NULL )
	{
		printf( "*** ERROR : file[%s] not found , errno[%d]\n" , p_env->config_filename , errno );
		return -1;
	}
	
	memset( & conf , 0x00 , sizeof(mysqlda_conf) );
	nret = DSCDESERIALIZE_JSON_mysqlda_conf( "GB18030" , file_buffer , & file_size , & conf ) ;
	free( file_buffer );
	if( nret )
	{
		printf( "*** ERROR : DSCDESERIALIZE_JSON_mysqlda_conf[%s] failed[%d]\n" , p_env->config_filename , nret );
		return -1;
	}
	
	strcpy( p_env->user , conf.auth.user );
	strcpy( p_env->pass , conf.auth.pass );
	strcpy( p_env->db , conf.auth.db );
	
	serial_range_begin = 0 ;
	for( forward_no = 0 ; forward_no < conf._forward_count ; forward_no++ )
	{
		p_forward_power = (struct ForwardPower *)malloc( sizeof(struct ForwardPower) ) ;
		if( p_forward_power == NULL )
		{
			printf( "*** ERROR : malloc failed , errno[%d]\n" , errno );
			return -12;
		}
		memset( p_forward_power , 0x00 , sizeof(struct ForwardPower) );
		
		strcpy( p_forward_power->instance , conf.forward[forward_no].instance );
		strcpy( p_forward_power->netaddr.ip , conf.forward[forward_no].ip );
		p_forward_power->netaddr.port = conf.forward[forward_no].port ;
		p_forward_power->power = conf.forward[forward_no].power ;
		if( p_forward_power->power == 0 )
			p_forward_power->power = 1 ;
		
		p_forward_power->serial_range_begin = serial_range_begin ;
		
		nret = LinkForwardInstanceTreeNode( p_env , p_forward_power );
		if( nret )
		{
			printf( "*** ERROR : LinkForwardInstanceTreeNode failed[%d] , errno[%d]\n" , nret , errno );
			return -13;
		}
		
		nret = LinkForwardSerialRangeTreeNode( p_env , p_forward_power );
		if( nret )
		{
			printf( "*** ERROR : LinkForwardSerialRangeTreeNode failed[%d] , errno[%d]\n" , nret , errno );
			return -14;
		}
		
		serial_range_begin += p_forward_power->power ;
		
		printf( "[%s] [%s][%d][%u] [%ld]\n" , p_forward_power->instance , p_forward_power->netaddr.ip , p_forward_power->netaddr.port , p_forward_power->power , p_forward_power->serial_range_begin );
	}
	
	p_env->total_power = serial_range_begin ;
	if( p_env->total_power <= 0 )
	{
		printf( "*** ERROR : no mysql instance valid\n" );
		return -13;
	}
	
	printf( "total_power[%ld]\n" , p_env->total_power );
	
	strcpy( p_env->listen_session.netaddr.ip , conf.server.listen_ip );
	p_env->listen_session.netaddr.port = conf.server.listen_port ;
	
	printf( "listen_ip[%s] listen_port[%d]\n" , p_env->listen_session.netaddr.ip , p_env->listen_session.netaddr.port );
	printf( "user[%s] pass[%s] db[%s]\n" , p_env->user , p_env->pass , p_env->db );
	
	return 0;
}

