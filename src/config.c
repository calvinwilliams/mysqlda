#include "mysqlda_in.h"

int InitConfigFile( struct MysqldaEnvironment *p_env )
{
	mysqlda_conf			conf ;
	char				*file_buffer = NULL ;
	int				file_size ;
	int				nret = 0 ;
	
	memset( & conf , 0x00 , sizeof(mysqlda_conf) );
	
	strcpy( conf.forward[0].instance , "mysqlda1" );
	strcpy( conf.forward[0].ip , "localhost" );
	conf.forward[0].port = 13306 ;
	strcpy( conf.forward[0].user , "calvin" );
	strcpy( conf.forward[0].pass , "calvin" );
	strcpy( conf.forward[0].db , "calvindb" );
	conf.forward[0].power = 1 ;
	conf._forward_count = 1 ;
	
	strcpy( conf.server.listen_ip , "127.0.0.1" );
	conf.server.listen_port = 3306 ;
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
	struct ForwardSession	*p_forward_session = NULL ;
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
	
	serial_range_begin = 0 ;
	for( forward_no = 0 ; forward_no < conf._forward_count ; forward_no++ )
	{
		p_forward_session = (struct ForwardSession *)malloc( sizeof(struct ForwardSession) ) ;
		if( p_forward_session == NULL )
		{
			printf( "*** ERROR : malloc failed , errno[%d]\n" , errno );
			return -12;
		}
		memset( p_forward_session , 0x00 , sizeof(struct ForwardSession) );
		
		p_forward_session->type = SESSIONTYPE_FORWARDSESSION ;
		strcpy( p_forward_session->instance , conf.forward[forward_no].instance );
		strcpy( p_forward_session->netaddr.ip , conf.forward[forward_no].ip );
		p_forward_session->netaddr.port = conf.forward[forward_no].port ;
		strcpy( p_forward_session->user , conf.forward[forward_no].user );
		strcpy( p_forward_session->pass , conf.forward[forward_no].pass );
		strcpy( p_forward_session->db , conf.forward[forward_no].db );
		p_forward_session->power = conf.forward[forward_no].power ;
		if( p_forward_session->power == 0 )
			p_forward_session->power = 1 ;
		
		p_forward_session->serial_range_begin = serial_range_begin ;
		
		nret = LinkForwardSessionTreeNode( p_env , p_forward_session );
		if( nret )
		{
			printf( "*** ERROR : LinkForwardSessionTreeNode failed[%d] , errno[%d]\n" , nret , errno );
			return -13;
		}
		
		serial_range_begin += p_forward_session->power ;
		
		printf( "[%s] [%s][%d][%s][%s][%s][%u] [%ld]\n" , p_forward_session->instance , p_forward_session->netaddr.ip , p_forward_session->netaddr.port , p_forward_session->user , p_forward_session->pass , p_forward_session->db , p_forward_session->power , p_forward_session->serial_range_begin );
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
	
	return 0;
}

