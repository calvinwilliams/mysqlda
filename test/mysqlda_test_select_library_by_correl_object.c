#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "my_global.h"
#include "mysql.h"
#include "mysqlda_api.h"

static void usage()
{
	printf( "USAGE : mysqlda_test_select_library_by_correl_object correl_object_class correl_object\n" );
	return;
}

int main( int argc , char *argv[] )
{
	MYSQL		*conn = NULL ;
	char		*correl_object_class = NULL ;
	char		*correl_object = NULL ;
	char		instance[ 20 + 1 ] ;
	int		nret = 0 ;
	
	if( argc != 1 + 2 )
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
	
	if( mysql_real_connect( conn , "192.168.6.21" , "calvin" , "calvin" , "calvindb" , 3306 , NULL , 0 ) == NULL )
	{
		printf( "mysql_real_connect failed , mysql_errno[%d][%s]\n" , mysql_errno(conn) , mysql_error(conn) );
		return 1;
	}
	else
	{
		printf( "mysql_real_connect ok\n" );
	}
	
	correl_object_class = argv[1] ;
	correl_object = argv[2] ;
	memset( instance , 0x00 , sizeof(instance) );
	nret = mysql_select_library_by_correl_object( conn , correl_object_class , correl_object , instance , sizeof(instance) ) ;
	if( nret )
	{
		printf( "mysql_select_library_by_correl_object failed , mysql_errno[%d][%s]\n" , mysql_errno(conn) , mysql_error(conn) );
		mysql_close( conn );
		return 1;
	}
	else
	{
		printf( "mysql_select_library_by_correl_object ok , instance[%s]\n" , instance );
	}
	
	mysql_close( conn );
	printf( "mysql_close\n" );
	
	return 0;
}

