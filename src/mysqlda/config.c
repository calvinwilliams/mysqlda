#include "mysqlda_in.h"

/* 创建缺省配置文件 */
int InitConfigFile( struct MysqldaEnvironment *p_env )
{
	mysqlda_conf			conf ;
	char				*file_buffer = NULL ;
	int				file_size ;
	int				nret = 0 ;
	
	/* 填充配置结构 */
	memset( & conf , 0x00 , sizeof(mysqlda_conf) );
	
	strcpy( conf.server.listen_ip , "127.0.0.1" );
	conf.server.listen_port = 3306 ;
	
	strcpy( conf.auth.user , "calvin" );
	strcpy( conf.auth.pass , "calvin" );
	strcpy( conf.auth.db , "calvindb" );
	
	conf.session_pool.unused_forward_session_timeout = 60 ;
	
	strcpy( conf.forwards[0].instance , "mysqlda1" );
	strcpy( conf.forwards[0].forward[0].ip , "127.0.0.1" );
	conf.forwards[0].forward[0].port = 13306 ;
	conf.forwards[0]._forward_count = 1 ;
	conf._forwards_count = 1 ;
	
	/* 转换成配置文本 */
	nret = DSCSERIALIZE_JSON_DUP_mysqlda_conf( & conf , "GB18030" , & file_buffer , NULL , & file_size ) ;
	if( nret )
	{
		printf( "*** ERROR : DSCSERIALIZE_JSON_DUP_mysqlda_conf not found\n" );
		return -1;
	}
	
	/* 写配置文件 */
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

/* 该服务端库增加一条转发规则，其它服务端库权重自增一，增加其它服务端库在下一次计算规则时增大选中概率 */
void IncreaseForwardInstanceTreeNodePower( struct MysqldaEnvironment *p_env , struct ForwardInstance *p_this_forward_instance )
{
	struct ForwardInstance	*p_forward_instance = NULL ;
	int			serial_range_begin ;
	
	serial_range_begin = 0 ;
	while(1)
	{
		p_forward_instance = TravelForwardSerialRangeTreeNode( p_env , p_forward_instance ) ;
		if( p_forward_instance == NULL )
			break;
		
		p_forward_instance->serial_range_begin = serial_range_begin ;
		if( p_forward_instance != p_this_forward_instance )
			p_forward_instance->power++;
		serial_range_begin += p_forward_instance->power ;
	}
	
	p_env->total_power = serial_range_begin ;
	
	return;
}

/* 装载配置文件 */
int LoadConfig( struct MysqldaEnvironment *p_env )
{
	char				*file_buffer = NULL ;
	int				file_size ;
	mysqlda_conf			*p_conf = NULL ;
	int				forwards_no ;
	struct ForwardInstance		*p_forward_instance = NULL ;
	int				serial_range_begin ;
	int				forward_no ;
	struct ForwardServer		*p_forward_server = NULL ;
	
	FILE				*fp = NULL ;
	struct ForwardLibrary		forward_library ;
	struct ForwardInstance		forward_instance ;
	struct ForwardLibrary		*p_forward_library = NULL ;
	
	char				etc_pathname[ 256 + 1 ] ;
	DIR				*dp = NULL ;
	struct dirent			*dtp = NULL ;
	char				mysqlda_str[ 64 + 1 ] ;
	char				correl_object_class[ MAXLEN_CORRELOBJECT_CLASS + 1 ] ;
	char				save_str[ 64 + 1 ] ;
	struct ForwardCorrelObjectClass	*p_forward_correl_object_class = NULL ;
	char				forward_correl_object_class_pathfilename[ 256 + 1 ] ;
	struct ForwardCorrelObject	forward_correl_object ;
	struct ForwardCorrelObject	*p_forward_correl_object = NULL ;
	
	int				nret = 0 ;
	
	/* 读入配置 */
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
	
	p_env->unused_forward_session_timeout = p_conf->session_pool.unused_forward_session_timeout ;
	
	if( p_conf->_forwards_count <= 0 )
	{
		ERRORLOG( "*** ERROR : forwards not found" );
		return -1;
	}
	
	/* 解读 服务端转发库 列表 */
	serial_range_begin = 0 ;
	for( forwards_no = 0 ; forwards_no < p_conf->_forwards_count ; forwards_no++ )
	{
		p_forward_instance = (struct ForwardInstance *)malloc( sizeof(struct ForwardInstance) ) ;
		if( p_forward_instance == NULL )
		{
			ERRORLOG( "*** ERROR : malloc failed , errno[%d]" , errno );
			return -1;
		}
		memset( p_forward_instance , 0x00 , sizeof(struct ForwardInstance) );
		
		strcpy( p_forward_instance->instance , p_conf->forwards[forwards_no].instance );
		p_forward_instance->power = 1 ;
		p_forward_instance->serial_range_begin = serial_range_begin ;
		
		if( p_conf->forwards[forwards_no]._forward_count <= 0 )
		{
			ERRORLOG( "*** ERROR : forward not found" );
			return -1;
		}
		
		/* 解读 服务端转发服务器信息 列表 */
		INIT_LK_LIST_HEAD( & (p_forward_instance->forward_server_list) );
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
			SETNETADDRESS( p_forward_server->netaddr );
			
			INIT_LK_LIST_HEAD( & (p_forward_server->forward_session_list) );
			INIT_LK_LIST_HEAD( & (p_forward_server->unused_forward_session_list) );
			
			lk_list_add_tail( & (p_forward_server->forward_server_listnode) , & (p_forward_instance->forward_server_list) );
		}
		
		/* 挂接到 服务端转发库 树 */
		nret = LinkForwardInstanceTreeNode( p_env , p_forward_instance );
		if( nret )
		{
			ERRORLOG( "*** ERROR : LinkForwardInstanceTreeNode failed[%d] , errno[%d]" , nret , errno );
			return -1;
		}
		
		nret = LinkForwardSerialRangeTreeNode( p_env , p_forward_instance );
		if( nret )
		{
			ERRORLOG( "*** ERROR : LinkForwardSerialRangeTreeNode failed[%d] , errno[%d]" , nret , errno );
			return -1;
		}
		
		serial_range_begin += p_forward_instance->power ;
	}
	
	p_env->total_power = serial_range_begin ;
	
	strcpy( p_env->listen_session.netaddr.ip , p_conf->server.listen_ip );
	p_env->listen_session.netaddr.port = p_conf->server.listen_port ;
	
	printf( "listen_ip[%s] listen_port[%d]\n" , p_env->listen_session.netaddr.ip , p_env->listen_session.netaddr.port );
	printf( "user[%s] pass[%s] db[%s]\n" , p_env->user , p_env->pass , p_env->db );
	
	free( p_conf );
	
	/* 装载服务端转发规则历史 持久化文件 */
	fp = fopen( p_env->save_filename , "r" ) ;
	if( fp )
	{
		while(1)
		{
			memset( & forward_library , 0x00 , sizeof(struct ForwardLibrary) );
			memset( & forward_instance , 0x00 , sizeof(struct ForwardInstance) );
			if( fscanf( fp , "%*s%*s%s%s\n" , forward_library.library , forward_instance.instance ) == EOF )
				break;
			if( forward_library.library[0] == '\0' || forward_instance.instance[0] == '\0' )
				break;
			
			p_forward_library = (struct ForwardLibrary *)malloc( sizeof(struct ForwardLibrary) ) ;
			if( p_forward_library == NULL )
			{
				ERRORLOG( "*** ERROR : malloc failed , errno[%d]" , errno );
				fclose( fp );
				return -1;
			}
			memcpy( p_forward_library , & forward_library , sizeof(struct ForwardLibrary) );
			
			p_forward_library->p_forward_instance = QueryForwardInstanceTreeNode( p_env , & forward_instance ) ;
			if( p_forward_library->p_forward_instance == NULL )
			{
				ERRORLOG( "*** ERROR : instance[%s] not found in %s" , forward_instance.instance , p_env->save_filename );
				fclose( fp );
				return -1;
			}
			
			nret = LinkForwardLibraryTreeNode( p_env , p_forward_library ) ;
			if( nret )
			{
				ERRORLOG( "*** ERROR : LinkForwardLibraryTreeNode[%s][%s] failed[%d]" , forward_library.library , forward_instance.instance , nret );
				fclose( fp );
				return -1;
			}
			
			IncreaseForwardInstanceTreeNodePower( p_env , p_forward_library->p_forward_instance );
		}
		
		fclose( fp );
	}
	
	/* 装载服务端转发关联规则历史 持久化文件 */
	memset( etc_pathname , 0x00 , sizeof(etc_pathname) );
	snprintf( etc_pathname , sizeof(etc_pathname)-1 , "%s/etc" , getenv("HOME") );
	dp = opendir( etc_pathname ) ;
	if( dp )
	{
		while( ( dtp = readdir(dp) ) )
		{
			if( strcmp(dtp->d_name,".") && strcmp(dtp->d_name,"..") )
			{
				memset( mysqlda_str , 0x00 , sizeof(mysqlda_str) );
				memset( correl_object_class , 0x00 , sizeof(correl_object_class) );
				memset( save_str , 0x00 , sizeof(save_str) );
				sscanf( dtp->d_name , "%[^.].%[^.].%[^.]" , mysqlda_str , correl_object_class , save_str );
				if( strcmp(mysqlda_str,"mysqlda") == 0 && strcmp(save_str,"save") == 0 )
				{
					p_forward_correl_object_class = (struct ForwardCorrelObjectClass *)malloc( sizeof(struct ForwardCorrelObjectClass) ) ;
					if( p_forward_correl_object_class == NULL )
					{
						ERRORLOG( "*** ERROR : malloc failed , errno[%d]" , errno );
						closedir( dp );
						return -1;
					}
					memset( p_forward_correl_object_class , 0x00 , sizeof(struct ForwardCorrelObjectClass) );
					
					strcpy( p_forward_correl_object_class->correl_object_class , correl_object_class );
					
					nret = LinkForwardCorrelObjectClassTreeNode( p_env , p_forward_correl_object_class ) ;
					if( nret )
					{
						ERRORLOG( "*** ERROR : LinkForwardCorrelObjectClassTreeNode failed[%d]" , nret );
						closedir( dp );
						return -1;
					}
					
					memset( forward_correl_object_class_pathfilename , 0x00 , sizeof(forward_correl_object_class_pathfilename) );
					snprintf( forward_correl_object_class_pathfilename , sizeof(forward_correl_object_class_pathfilename)-1 , "%s/etc/%s" , getenv("HOME") , dtp->d_name );
					fp = fopen( forward_correl_object_class_pathfilename , "r" ) ;
					if( fp == NULL )
					{
						ERRORLOG( "*** ERROR : fopen[%s] failed , errno[%d]" , forward_correl_object_class_pathfilename , errno );
						closedir( dp );
						return -1;
					}
					
					while(1)
					{
						memset( & forward_correl_object , 0x00 , sizeof(struct ForwardCorrelObject) );
						memset( & forward_library , 0x00 , sizeof(struct ForwardLibrary) );
						if( fscanf( fp , "%*s%*s%s%s\n" , forward_correl_object.correl_object , forward_library.library ) == EOF )
							break;
						if( forward_correl_object.correl_object[0] == '\0' || forward_library.library[0] == '\0' )
							break;
						
						p_forward_correl_object = (struct ForwardCorrelObject *)malloc( sizeof(struct ForwardCorrelObject) ) ;
						if( p_forward_correl_object == NULL )
						{
							ERRORLOG( "*** ERROR : malloc failed , errno[%d]" , errno );
							fclose( fp );
							closedir( dp );
							return -1;
						}
						memset( p_forward_correl_object , 0x00 , sizeof(struct ForwardCorrelObject) );
						
						strcpy( p_forward_correl_object->correl_object , forward_correl_object.correl_object );
						
						p_forward_correl_object->p_forward_library = QueryForwardLibraryTreeNode( p_env , & forward_library ) ;
						if( p_forward_correl_object->p_forward_library == NULL )
						{
							ERRORLOG( "*** ERROR : instance[%s] not found in %s" , forward_instance.instance , p_env->save_filename );
							fclose( fp );
							closedir( dp );
							return -1;
						}
						
						nret = LinkForwardCorrelObjectTreeNode( p_forward_correl_object_class , p_forward_correl_object ) ;
						if( nret )
						{
							ERRORLOG( "*** ERROR : LinkForwardCorrelObjectTreeNode[%s][%s] failed[%d]" , p_forward_correl_object_class->correl_object_class , p_forward_correl_object->correl_object , nret );
							fclose( fp );
							return -1;
						}
					}
					
					fclose( fp );
				}
			}
		}
		
		closedir( dp );
	}
	
	INFOLOG( "Load %s ok" , p_env->save_filename );
	
	/* 输出所有服务端转发库权重到日志 */
	p_forward_instance = NULL ;
	while(1)
	{
		p_forward_instance = TravelForwardSerialRangeTreeNode( p_env , p_forward_instance ) ;
		if( p_forward_instance == NULL )
			break;
		
		INFOLOG( "instance[%p][%s] serial_range_begin[%lu] power[%lu]" , p_forward_instance , p_forward_instance->instance , p_forward_instance->serial_range_begin , p_forward_instance->power );
		
		lk_list_for_each_entry( p_forward_server , & (p_forward_instance->forward_server_list) , struct ForwardServer , forward_server_listnode )
		{
			INFOLOG( "\tip[%s] port[%d]" , p_forward_server->netaddr.ip , p_forward_server->netaddr.port );
		}
	}
	
	INFOLOG( "total_power[%lu]" , p_env->total_power );
	
	return 0;
}

/* 重载配置文件 */
int ReloadConfig( struct MysqldaEnvironment *p_env )
{
	char			*file_buffer = NULL ;
	int			file_size ;
	mysqlda_conf		*p_conf = NULL ;
	
	struct ForwardInstance	*p_forward_instance = NULL ;
	struct ForwardServer	*p_forward_server = NULL ;
	int			forwards_no ;
	struct ForwardInstance	forward_instance ;
	int			forward_no ;
	struct ForwardSession	*p_forward_session = NULL ;
	struct ForwardSession	*p_next_forward_session = NULL ;
	
	
	int			nret = 0 ;
	
	/* 读入配置 */
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
	
	/* 解读 服务端转发库 列表 */
	for( forwards_no = 0 ; forwards_no < p_conf->_forwards_count ; forwards_no++ )
	{
		snprintf( forward_instance.instance , sizeof(forward_instance.instance)-1 , "%s" , p_conf->forwards[forwards_no].instance );
		p_forward_instance = QueryForwardInstanceTreeNode( p_env , & forward_instance ) ;
		if( p_forward_instance )
		{
			/* 删除不存在的服务端转发服务器 */
			lk_list_for_each_entry( p_forward_server , & (p_forward_instance->forward_server_list) , struct ForwardServer , forward_server_listnode )
			{
				for( forward_no = 0 ; forward_no < p_conf->forwards[forwards_no]._forward_count ; forward_no++ )
				{
					if( STRCMP( p_forward_server->netaddr.ip , == , p_conf->forwards[forwards_no].forward[forward_no].ip ) && p_forward_server->netaddr.port == p_conf->forwards[forwards_no].forward[forward_no].port )
						break;
				}
				if( forward_no > p_conf->forwards[forwards_no]._forward_count )
				{
					lk_list_for_each_entry_safe( p_forward_session , p_next_forward_session , & (p_forward_server->forward_session_list) , struct ForwardSession , forward_session_listnode )
					{
						OnClosingForwardSocket( p_env , p_forward_session );
					}
					
					lk_list_for_each_entry_safe( p_forward_session , p_next_forward_session , & (p_forward_server->unused_forward_session_list) , struct ForwardSession , forward_session_listnode )
					{
						OnClosingForwardSocket( p_env , p_forward_session );
					}
					
					INFOLOG( "reload remove ip[%s] port[%d] in instance[%p][%s]" , p_forward_server->netaddr.ip , p_forward_server->netaddr.port , p_forward_instance , p_forward_instance->instance );
				}
			}
			
			/* 新增服务端转发服务器 */
			for( forward_no = 0 ; forward_no < p_conf->forwards[forwards_no]._forward_count ; forward_no++ )
			{
				lk_list_for_each_entry( p_forward_server , & (p_forward_instance->forward_server_list) , struct ForwardServer , forward_server_listnode )
				{
					if( STRCMP( p_forward_server->netaddr.ip , == , p_conf->forwards[forwards_no].forward[forward_no].ip ) && p_forward_server->netaddr.port == p_conf->forwards[forwards_no].forward[forward_no].port )
						break;
				}
				if( & (p_forward_server->forward_server_listnode) == & (p_forward_instance->forward_server_list) )
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
					
					INIT_LK_LIST_HEAD( & (p_forward_server->forward_session_list) );
					INIT_LK_LIST_HEAD( & (p_forward_server->unused_forward_session_list) );
					
					lk_list_add_tail( & (p_forward_server->forward_server_listnode) , & (p_forward_instance->forward_server_list) );
					
					INFOLOG( "reload add ip[%s] port[%d] in instance[%p][%s]" , p_forward_server->netaddr.ip , p_forward_server->netaddr.port , p_forward_instance , p_forward_instance->instance );
				}
			}
		}
		else
		{
			/* 新增服务端转发库 */
			p_forward_instance = (struct ForwardInstance *)malloc( sizeof(struct ForwardInstance) ) ;
			if( p_forward_instance == NULL )
			{
				ERRORLOG( "*** ERROR : malloc failed , errno[%d]" , errno );
				return -1;
			}
			memset( p_forward_instance , 0x00 , sizeof(struct ForwardInstance) );
			
			strcpy( p_forward_instance->instance , p_conf->forwards[forwards_no].instance );
			p_forward_instance->power = p_env->total_power ;
			p_forward_instance->serial_range_begin = p_env->total_power ;
			
			if( p_conf->forwards[forwards_no]._forward_count <= 0 )
			{
				ERRORLOG( "*** ERROR : forward not found" );
				free( p_forward_instance );
				return -1;
			}
			
			/* 解读 服务端转发服务器信息 列表 */
			INIT_LK_LIST_HEAD( & (p_forward_instance->forward_server_list) );
			for( forward_no = 0 ; forward_no < p_conf->forwards[forwards_no]._forward_count ; forward_no++ )
			{
				p_forward_server = (struct ForwardServer *)malloc( sizeof(struct ForwardServer) ) ;
				if( p_forward_server == NULL )
				{
					ERRORLOG( "*** ERROR : malloc failed , errno[%d]" , errno );
					free( p_forward_instance );
					return -1;
				}
				memset( p_forward_server , 0x00 , sizeof(struct ForwardServer) );
				
				strcpy( p_forward_server->netaddr.ip , p_conf->forwards[forwards_no].forward[forward_no].ip );
				p_forward_server->netaddr.port = p_conf->forwards[forwards_no].forward[forward_no].port ;
				
				INIT_LK_LIST_HEAD( & (p_forward_server->forward_session_list) );
				INIT_LK_LIST_HEAD( & (p_forward_server->unused_forward_session_list) );
				
				lk_list_add_tail( & (p_forward_server->forward_server_listnode) , & (p_forward_instance->forward_server_list) );
				
				INFOLOG( "reload add instance[%s] ip[%s] port[%d]" , p_forward_instance->instance , p_forward_server->netaddr.ip , p_forward_server->netaddr.port );
			}
			
			/* 挂接到 服务端转发库 树 */
			nret = LinkForwardInstanceTreeNode( p_env , p_forward_instance );
			if( nret )
			{
				ERRORLOG( "*** ERROR : LinkForwardInstanceTreeNode failed[%d] , errno[%d]" , nret , errno );
				free( p_forward_instance );
				return -1;
			}
			
			nret = LinkForwardSerialRangeTreeNode( p_env , p_forward_instance );
			if( nret )
			{
				ERRORLOG( "*** ERROR : LinkForwardSerialRangeTreeNode failed[%d] , errno[%d]" , nret , errno );
				UnlinkForwardInstanceTreeNode( p_env , p_forward_instance );
				free( p_forward_instance );
				return -1;
			}
			
			p_env->total_power += p_env->total_power ;
		}
	}
	
	/* 输出所有服务端转发库权重到日志 */
	p_forward_instance = NULL ;
	while(1)
	{
		p_forward_instance = TravelForwardSerialRangeTreeNode( p_env , p_forward_instance ) ;
		if( p_forward_instance == NULL )
			break;
		
		INFOLOG( "p_forward_instance[%p][%s]" , p_forward_instance , p_forward_instance->instance );
		
		lk_list_for_each_entry( p_forward_server , & (p_forward_instance->forward_server_list) , struct ForwardServer , forward_server_listnode )
		{
			INFOLOG( "\tp_forward_server->ip[%s] ->port[%d]" , p_forward_server->netaddr.ip , p_forward_server->netaddr.port );
		}
	}
	
	return 0;
}

/* 卸载配置文件 */
void UnloadConfig( struct MysqldaEnvironment *p_env )
{
	struct ForwardInstance	*p_forward_instance = NULL ;
	struct ForwardInstance	*p_next_forward_instance = NULL ;
	struct ForwardServer	*p_forward_server = NULL ;
	struct ForwardServer	*p_next_forward_server = NULL ;
	
	while(1)
	{
		p_next_forward_instance = TravelForwardSerialRangeTreeNode( p_env , p_forward_instance ) ;
		if( p_forward_instance )
		{
			UnlinkForwardSerialRangeTreeNode( p_env , p_forward_instance );
			free( p_forward_instance );
		}
		if( p_next_forward_instance == NULL )
		{
			break;
		}
		
		p_forward_instance = p_next_forward_instance ;
		
		lk_list_for_each_entry_safe( p_forward_server , p_next_forward_server , & (p_forward_instance->forward_server_list) , struct ForwardServer , forward_server_listnode )
		{
			free( p_forward_server );
		}
	}
	
	/*
	DestroyForwardLibraryTree( p_env );
	*/
	
	return;
}

