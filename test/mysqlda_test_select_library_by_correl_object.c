#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "my_global.h"
#include "mysql.h"

/*
./mysqlda_test_select_library_by_correl_object "192.168.6.21" 13306 calvin calvin calvindb card_no 330001
*/

static void usage()
{
	printf( "USAGE : mysqlda_test_select_library_by_correl_object (ip) (port) (user) (pass) (database) (correl_object_class) (correl_object)\n" );
	return;
}

int main( int argc , char *argv[] )
{
	MYSQL		*conn = NULL ;
	char		*ip = NULL ;
	unsigned int	port ;
	char		*user = NULL ;
	char		*pass = NULL ;
	char		*database = NULL ;
	
	char		*correl_object_class = NULL ;
	char		*correl_object = NULL ;
	char		sql[ 4096 + 1 ] ;
	
	int		nret = 0 ;
	
	if( argc != 1 + 7 )
	{
		usage();
		exit(7);
	}
	
	printf( "mysql_get_client_info[%s]\n" , mysql_get_client_info() );
	
	conn = mysql_init(NULL) ;
	if( conn == NULL )
	{
		printf( "mysql_init failed\n" );
		return 1;
	}
	
	ip = argv[1] ;
	port = (unsigned int)atoi(argv[2]) ;
	user = argv[3] ;
	pass = argv[4] ;
	database = argv[5] ;
	if( mysql_real_connect( conn , ip , user , pass , database , port , NULL , 0 ) == NULL )
	{
		printf( "mysql_real_connect failed , mysql_errno[%d][%s]\n" , mysql_errno(conn) , mysql_error(conn) );
		return 1;
	}
	else
	{
		printf( "mysql_real_connect ok\n" );
	}
	
	correl_object_class = argv[6] ;
	correl_object = argv[7] ;
	
	memset( sql , 0x00 , sizeof(sql) );
	snprintf( sql , sizeof(sql) , "select library_by_correl_object %s %s" , correl_object_class , correl_object );
	nret = mysql_query( conn , sql ) ;
	if( nret )
	{
		printf( "mysql_query failed , mysql_errno[%d][%s]\n" , mysql_errno(conn) , mysql_error(conn) );
		mysql_close( conn );
		return 1;
	}
	else
	{
		printf( "mysql_query ok\n" );
	}
	
	mysql_close( conn );
	printf( "mysql_close\n" );
	
	return 0;
}

