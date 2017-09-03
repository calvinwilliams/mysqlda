#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "my_global.h"
#include "mysql.h"

/*
./mysqlda_test_connect "192.168.6.21" 13306 calvin calvin calvindb
*/

static void usage()
{
	printf( "USAGE : mysqlda_test_connect (ip) (port) (user) (pass) (database)\n" );
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
	
	if( argc != 1 + 5 )
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
	
	mysql_close( conn );
	printf( "mysql_close\n" );
	
	return 0;
}

