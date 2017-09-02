#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "my_global.h"
#include "mysql.h"

static void usage()
{
	printf( "USAGE : mysqlda_test_press ip port begin_seqno end_seqno\n" );
	return;
}

int main( int argc , char *argv[] )
{
	MYSQL		*conn = NULL ;
	MYSQL_RES	*result = NULL ;
	char		*ip = NULL ;
	int		port ;
	int		begin_seqno ;
	int		end_seqno ;
	int		seqno ;
	char		seqno_buffer[ 20 + 1 ] ;
	char		sql[ 4096 + 1 ] ;
	
	int		nret = 0 ;
	
	if( argc != 1 + 4 )
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
	port = atoi(argv[2]) ;
	if( mysql_real_connect( conn , ip , "calvin" , "calvin" , "calvindb" , port , NULL , 0 ) == NULL )
	{
		printf( "mysql_real_connect failed , mysql_errno[%d][%s]\n" , mysql_errno(conn) , mysql_error(conn) );
		return 1;
	}
	else
	{
		printf( "mysql_real_connect ok\n" );
	}
	
	memset( seqno_buffer , 0x00 , sizeof(seqno_buffer) );
	begin_seqno = atoi(argv[3]) ;
	end_seqno = atoi(argv[4]) ;
	for( seqno = begin_seqno ; seqno <= end_seqno ; seqno++ )
	{
		if( port == 13306 )
		{
			memset( sql , 0x00 , sizeof(sql) );
			snprintf( sql , sizeof(sql) , "select library %d" , seqno );
			nret = mysql_query( conn , sql ) ;
			if( nret )
			{
				printf( "mysql_query[%s] failed , mysql_errno[%d][%s]\n" , sql , mysql_errno(conn) , mysql_error(conn) );
				mysql_close( conn );
				return 1;
			}
			else
			{
				if( seqno == begin_seqno )
					printf( "mysql_query[%s] ok\n" , sql );
			}
		}
		
		memset( sql , 0x00 , sizeof(sql) );
		snprintf( sql , sizeof(sql) , "insert into test_table value( '%d' , '%d' )" , seqno , seqno );
		nret = mysql_query( conn , sql ) ;
		if( nret )
		{
			printf( "mysql_query[%s] failed , mysql_errno[%d][%s]\n" , sql , mysql_errno(conn) , mysql_error(conn) );
			mysql_close( conn );
			return 1;
		}
		else
		{
			if( seqno == begin_seqno )
				printf( "mysql_query[%s] ok\n" , sql );
		}
		
		memset( sql , 0x00 , sizeof(sql) );
		snprintf( sql , sizeof(sql) , "select * from test_table where name='%d'" , seqno );
		nret = mysql_query( conn , sql ) ;
		if( nret )
		{
			printf( "mysql_query[%s] failed , mysql_errno[%d][%s]\n" , sql , mysql_errno(conn) , mysql_error(conn) );
			mysql_close( conn );
			return 1;
		}
		else
		{
			if( seqno == begin_seqno )
				printf( "mysql_query[%s] ok\n" , sql );
		}
		
		result = mysql_store_result( conn );
		mysql_free_result( result );
		
		memset( sql , 0x00 , sizeof(sql) );
		snprintf( sql , sizeof(sql) , "update test_table set value='%d_%d' where name='%d'" , seqno , seqno , seqno );
		nret = mysql_query( conn , sql ) ;
		if( nret )
		{
			printf( "mysql_query[%s] failed , mysql_errno[%d][%s]\n" , sql , mysql_errno(conn) , mysql_error(conn) );
			mysql_close( conn );
			return 1;
		}
		else
		{
			if( seqno == begin_seqno )
				printf( "mysql_query[%s] ok\n" , sql );
		}
		
		memset( sql , 0x00 , sizeof(sql) );
		snprintf( sql , sizeof(sql) , "delete from test_table where name='%d'" , seqno );
		nret = mysql_query( conn , sql ) ;
		if( nret )
		{
			printf( "mysql_query[%s] failed , mysql_errno[%d][%s]\n" , sql , mysql_errno(conn) , mysql_error(conn) );
			mysql_close( conn );
			return 1;
		}
		else
		{
			if( seqno == begin_seqno )
				printf( "mysql_query[%s] ok\n" , sql );
		}
	}
	
	mysql_close( conn );
	printf( "mysql_close\n" );
	
	return 0;
}

