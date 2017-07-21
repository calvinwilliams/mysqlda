#include "mysqlda_in.h"

static int _mysqlda( struct MysqldaEnvironment *p_env )
{
	return 0;
}

int mysqlda( void *pv )
{
	struct MysqldaEnvironment	*p_env = (struct MysqldaEnvironment *) pv ;
	
	SetLogFile( "%s/log/mysqlda.log" );
	SetLogLevel( LOGLEVEL_DEBUG );
	
	return _mysqlda( p_env );
}

