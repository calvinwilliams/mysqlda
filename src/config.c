#include "mysqlda_in.h"

int LoadConfig( struct MysqldaEnvironment *p_env )
{
	FILE				*fp = NULL ;
	char				file_buffer[ 4096 + 1 ] ;
	struct MysqlClientForward	forward ;
	struct MysqlClientForward	*p_forward = NULL ;
	
	fp = fopen( p_env->config_filename , "r" ) ;
	if( fp == NULL )
	{
		printf( "*** ERROR : file[%s] not found\n" , p_env->config_filename );
		return -1;
	}
	
	while( ! feof(fp) )
	{
		memset( file_buffer , 0x00 , sizeof(file_buffer) );
		if( fgets( file_buffer , sizeof(file_buffer) , fp ) == NULL )
			break;
		
		memset( & forward , 0x00 , sizeof(struct MysqlClientForward) );
		sscanf( file_buffer , "%s%d%s%s%*[^\n]" , forward.ip , & (forward.port) , forward.user , forward.pass );
		if( forward.pass[0] == '\0' )
		{
			printf( "*** ERROR : file buffer[%s] invalid\n" , file_buffer );
			fclose( fp );
			return -2;
		}
		
		p_forward = (struct MysqlClientForward *)malloc( sizeof(struct MysqlClientForward) ) ;
		if( p_forward == NULL )
		{
			printf( "*** ERROR : malloc failed , errno[%d]\n" , errno );
			fclose( fp );
			return -3;
		}
		memcpy( p_forward , & forward , sizeof(struct MysqlClientForward) );
		
		list_add_tail( p_forward , & (p_env->forward_list) );
	}
	
	fclose( fp );
	
	
	return 0;
}

