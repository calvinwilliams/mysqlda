#include "mysqlda_in.h"

static int _worker( struct MysqldaEnvironment *p_env )
{
	struct MysqlForwardClient	*p_client = NULL ;
	
	while(1)
	{
		p_client = TravelMysqlForwardClientTreeNode( p_env , p_client ) ;
		if( p_client == NULL )
			break;
		
		p_client->mysql_connection = mysql_init( NULL ) ;
		if( p_client->mysql_connection == NULL )
		{
			ERRORLOG( "mysql_init failed , errno[%d]" , errno );
			return -1;
		}
		
		INFOLOG( "[%s]mysql_real_connect[%s][%d][%s][%s][%s] connecting ..." , p_client->instance , p_client->ip , p_client->port , p_client->user , p_client->pass , p_client->db );
		if( mysql_real_connect( p_client->mysql_connection , p_client->ip , p_client->user , p_client->pass , p_client->db , p_client->port , NULL , 0 ) == NULL )
		{
			ERRORLOG( "[%s]mysql_real_connect[%s][%d][%s][%s][%s] failed , mysql_errno[%d][%s]" , p_client->instance , p_client->ip , p_client->port , p_client->user , p_client->pass , p_client->db , mysql_errno(p_client->mysql_connection) , mysql_error(p_client->mysql_connection) );
			return -2;
		}
		else
		{
			INFOLOG( "[%s]mysql_real_connect[%s][%d][%s][%s][%s] connecting ok" , p_client->instance , p_client->ip , p_client->port , p_client->user , p_client->pass , p_client->db );
		}
	}
	
	
	
	
	
	
	
	while(1)
	{
		p_client = TravelMysqlForwardClientTreeNode( p_env , p_client ) ;
		if( p_client == NULL )
			break;
		
		mysql_close( p_client->mysql_connection );
		INFOLOG( "[%s]mysql_close[%s][%d][%s][%s][%s] ok" , p_client->instance , p_client->ip , p_client->port , p_client->user , p_client->pass , p_client->db );
	}
	
	return 0;
}

int worker( void *pv )
{
	struct MysqldaEnvironment	*p_env = (struct MysqldaEnvironment *) pv ;
	
	SetLogFile( "%s/log/mysqlda.log" , getenv("HOME") );
	SetLogLevel( LOGLEVEL_DEBUG );
	
	return _worker( p_env );
}

