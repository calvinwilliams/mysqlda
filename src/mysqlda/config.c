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
	
	strcpy( conf.forwards[0].instance , "mysqlda1" );
	strcpy( conf.forwards[0].forward[0].ip , "localhost" );
	conf.forwards[0].forward[0].port = 13306 ;
	conf.forwards[0]._forward_count = 1 ;
	conf._forwards_count = 1 ;
	
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

void AddForwardPowerTreeNodePower( struct MysqldaEnvironment *p_env , struct ForwardPower *p_this_forward_power )
{
	struct ForwardPower	*p_forward_power = NULL ;
	int			serial_range_begin ;
	
	serial_range_begin = 0 ;
	while(1)
	{
		p_forward_power = TravelForwardSerialRangeTreeNode( p_env , p_forward_power ) ;
		if( p_forward_power == NULL )
			break;
		
		p_forward_power->serial_range_begin = serial_range_begin ;
		if( p_forward_power != p_this_forward_power )
			p_forward_power->power++;
		serial_range_begin += p_forward_power->power ;
	}
	
	p_env->total_power = serial_range_begin ;
	
	return;
}

int LoadConfig( struct MysqldaEnvironment *p_env )
{
	char			*file_buffer = NULL ;
	int			file_size ;
	mysqlda_conf		*p_conf = NULL ;
	int			forwards_no ;
	struct ForwardPower	*p_forward_power = NULL ;
	int			serial_range_begin ;
	int			forward_no ;
	struct ForwardServer	*p_forward_server = NULL ;
	
	FILE			*fp = NULL ;
	struct ForwardLibrary	forward_library ;
	struct ForwardPower	forward_power ;
	struct ForwardLibrary	*p_forward_library = NULL ;
	
	int			nret = 0 ;
	
	file_buffer = StrdupEntireFile( p_env->config_filename , & file_size ) ;
	if( file_buffer == NULL )
	{
		ERRORLOG( "*** ERROR : config file[%s] not found , errno[%d]" , p_env->config_filename , errno );
		return -1;
	}
	
	p_conf = (mysqlda_conf *)malloc( sizeof(mysqlda_conf) ) ;
	if( p_conf == NULL )
	{
		ERRORLOG( "*** ERROR : malloc failed , errno[%d]" , errno );
		return -1;
	}
	
	memset( p_conf , 0x00 , sizeof(mysqlda_conf) );
	nret = DSCDESERIALIZE_JSON_mysqlda_conf( "GB18030" , file_buffer , & file_size , p_conf ) ;
	free( file_buffer );
	if( nret )
	{
		ERRORLOG( "*** ERROR : DSCDESERIALIZE_JSON_mysqlda_conf[%s] failed[%d]" , p_env->config_filename , nret );
		return -1;
	}
	
	strcpy( p_env->user , p_conf->auth.user );
	strcpy( p_env->pass , p_conf->auth.pass );
	strcpy( p_env->db , p_conf->auth.db );
	
	if( p_conf->_forwards_count <= 0 )
	{
		ERRORLOG( "*** ERROR : forwards not found" );
		return -1;
	}
	
	serial_range_begin = 0 ;
	for( forwards_no = 0 ; forwards_no < p_conf->_forwards_count ; forwards_no++ )
	{
		p_forward_power = (struct ForwardPower *)malloc( sizeof(struct ForwardPower) ) ;
		if( p_forward_power == NULL )
		{
			ERRORLOG( "*** ERROR : malloc failed , errno[%d]" , errno );
			return -1;
		}
		memset( p_forward_power , 0x00 , sizeof(struct ForwardPower) );
		
		strcpy( p_forward_power->instance , p_conf->forwards[forwards_no].instance );
		p_forward_power->power = 1 ;
		p_forward_power->serial_range_begin = serial_range_begin ;
		
		if( p_conf->forwards[forwards_no]._forward_count <= 0 )
		{
			ERRORLOG( "*** ERROR : forward not found" );
			return -1;
		}
		
		INIT_LK_LIST_HEAD( & (p_forward_power->forward_server_list) );
		for( forward_no = 0 ; forward_no < p_conf->forwards[forwards_no]._forward_count ; forward_no++ )
		{
			p_forward_server = (struct ForwardServer *)malloc( sizeof(struct ForwardServer) ) ;
			if( p_forward_server == NULL )
			{
				ERRORLOG( "*** ERROR : malloc failed , errno[%d]" , errno );
				return -1;
			}
			memset( p_forward_server , 0x00 , sizeof(struct ForwardServer) );
			
			strcpy( p_forward_server->netaddr.ip , p_conf->forwards[forwards_no].forward[forward_no].ip );
			p_forward_server->netaddr.port = p_conf->forwards[forwards_no].forward[forward_no].port ;
			
			lk_list_add_tail( & (p_forward_server->forward_server_listnode) , & (p_forward_power->forward_server_list) );
		}
		
		nret = LinkForwardInstanceTreeNode( p_env , p_forward_power );
		if( nret )
		{
			ERRORLOG( "*** ERROR : LinkForwardInstanceTreeNode failed[%d] , errno[%d]" , nret , errno );
			return -1;
		}
		
		nret = LinkForwardSerialRangeTreeNode( p_env , p_forward_power );
		if( nret )
		{
			ERRORLOG( "*** ERROR : LinkForwardSerialRangeTreeNode failed[%d] , errno[%d]" , nret , errno );
			return -1;
		}
		
		serial_range_begin += p_forward_power->power ;
	}
	
	p_env->total_power = serial_range_begin ;
	
	strcpy( p_env->listen_session.netaddr.ip , p_conf->server.listen_ip );
	p_env->listen_session.netaddr.port = p_conf->server.listen_port ;
	
	printf( "listen_ip[%s] listen_port[%d]\n" , p_env->listen_session.netaddr.ip , p_env->listen_session.netaddr.port );
	printf( "user[%s] pass[%s] db[%s]\n" , p_env->user , p_env->pass , p_env->db );
	
	free( p_conf );
	
	fp = fopen( p_env->save_filename , "r" ) ;
	if( fp )
	{
		while(1)
		{
			memset( & forward_library , 0x00 , sizeof(struct ForwardLibrary) );
			memset( & forward_power , 0x00 , sizeof(struct ForwardPower) );
			if( fscanf( fp , "%*s%*s%s%s\n" , forward_library.library , forward_power.instance ) == EOF )
				break;
			if( forward_library.library[0] == '\0' || forward_power.instance[0] == '\0' )
				break;
			
			p_forward_library = (struct ForwardLibrary *)malloc( sizeof(struct ForwardLibrary) ) ;
			if( p_forward_library == NULL )
			{
				ERRORLOG( "*** ERROR : malloc failed , errno[%d]" , errno );
				fclose( fp );
				return -1;
			}
			memcpy( p_forward_library , & forward_library , sizeof(struct ForwardLibrary) );
			
			p_forward_library->p_forward_power = QueryForwardInstanceTreeNode( p_env , & forward_power ) ;
			if( p_forward_library->p_forward_power == NULL )
			{
				ERRORLOG( "*** ERROR : instance[%s] not found in %s" , forward_power.instance , p_env->save_filename );
				fclose( fp );
				return -1;
			}
			
			nret = LinkForwardLibraryTreeNode( p_env , p_forward_library ) ;
			if( nret )
			{
				ERRORLOG( "*** ERROR : LinkForwardLibraryTreeNode[%s][%s] failed[%d]" , forward_library.library , forward_power.instance , nret );
				fclose( fp );
				return -1;
			}
			
			AddForwardPowerTreeNodePower( p_env , p_forward_library->p_forward_power );
		}
		
		fclose( fp );
	}
	
	INFOLOG( "Load %s ok" , p_env->save_filename );
	
	p_forward_power = NULL ;
	while(1)
	{
		p_forward_power = TravelForwardSerialRangeTreeNode( p_env , p_forward_power ) ;
		if( p_forward_power == NULL )
			break;
		
		INFOLOG( "instance[%s] serial_range_begin[%lu] power[%lu]" , p_forward_power->instance , p_forward_power->serial_range_begin , p_forward_power->power );
		
		lk_list_for_each_entry( p_forward_server , & (p_forward_power->forward_server_list) , struct ForwardServer , forward_server_listnode )
		{
			INFOLOG( "\tip[%s] port[%d]" , p_forward_server->netaddr.ip , p_forward_server->netaddr.port );
		}
	}
	
	INFOLOG( "total_power[%lu]" , p_env->total_power );
	
	return 0;
}

void UnloadConfig( struct MysqldaEnvironment *p_env )
{
	struct ForwardPower	*p_forward_power = NULL ;
	struct ForwardPower	*p_next_forward_power = NULL ;
	struct ForwardServer	*p_forward_server = NULL ;
	struct ForwardServer	*p_next_forward_server = NULL ;
	
	while(1)
	{
		p_next_forward_power = TravelForwardSerialRangeTreeNode( p_env , p_forward_power ) ;
		if( p_forward_power )
		{
			UnlinkForwardSerialRangeTreeNode( p_env , p_forward_power );
			free( p_forward_power );
		}
		if( p_next_forward_power == NULL )
		{
			break;
		}
		
		p_forward_power = p_next_forward_power ;
		
		lk_list_for_each_entry_safe( p_forward_server , p_next_forward_server , & (p_forward_power->forward_server_list) , struct ForwardServer , forward_server_listnode )
		{
			free( p_forward_server );
		}
	}
	
	DestroyForwardLibraryTree( p_env );
	
	return;
}

