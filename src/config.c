#include "mysqlda_in.h"

int LoadConfig( struct MysqldaEnvironment *p_env )
{
	FILE				*fp = NULL ;
	char				file_buffer[ 4096 + 1 ] ;
	struct MysqlForwardClient	client ;
	struct MysqlForwardClient	*p_client = NULL ;
	int				serial_range_begin ;
	int				nret = 0 ;
	
	fp = fopen( p_env->config_filename , "r" ) ;
	if( fp == NULL )
	{
		printf( "*** ERROR : file[%s] not found\n" , p_env->config_filename );
		return -1;
	}
	
	serial_range_begin = 0 ;
	while( ! feof(fp) )
	{
		memset( file_buffer , 0x00 , sizeof(file_buffer) );
		if( fgets( file_buffer , sizeof(file_buffer) , fp ) == NULL )
			break;
		
		memset( & client , 0x00 , sizeof(struct MysqlForwardClient) );
		sscanf( file_buffer , "%s%s%d%s%s%s%ld%*[^\n]" , client.instance , client.ip , & (client.port) , client.user , client.pass , client.db , & (client.power) );
		if( client.pass[0] == '\0' )
		{
			printf( "*** ERROR : file buffer[%s] invalid\n" , file_buffer );
			fclose( fp );
			return -11;
		}
		
		if( client.power == 0 )
			client.power = 1 ;
		
		p_client = (struct MysqlForwardClient *)malloc( sizeof(struct MysqlForwardClient) ) ;
		if( p_client == NULL )
		{
			printf( "*** ERROR : malloc failed , errno[%d]\n" , errno );
			fclose( fp );
			return -12;
		}
		memcpy( p_client , & client , sizeof(struct MysqlForwardClient) );
		
		p_client->serial_range_begin = serial_range_begin ;
		
		nret = LinkMysqlForwardClientTreeNode( p_env , p_client );
		if( nret )
		{
			printf( "*** ERROR : LinkMysqlForwardClientTreeNode failed[%d] , errno[%d]\n" , nret , errno );
			fclose( fp );
			return -13;
		}
		
		serial_range_begin += client.power ;
		
		printf( "[%s] [%s][%d][%s][%s][%s] [%ld]\n" , p_client->instance , p_client->ip , p_client->port , p_client->user , p_client->pass , p_client->db , p_client->serial_range_begin );
	}
	
	fclose( fp );
	
	p_env->total_power = serial_range_begin ;
	if( p_env->total_power <= 0 )
	{
		printf( "*** ERROR : no mysql instance valid\n" );
		fclose( fp );
		return -13;
	}
	
	printf( "total_power[%ld]\n" , p_env->total_power );
	
	return 0;
}

