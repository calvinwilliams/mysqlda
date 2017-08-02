#include "mysqlda_in.h"

static void usage()
{
	printf( "USAGE : mysqlda -f (config_filename) --no-daemon -a [ init | start ]\n" );
	return;
}

int main( int argc , char *argv[] )
{
	struct MysqldaEnvironment	env , *p_env = & env ;
	int				i ;
	int				nret = 0 ;
	
	memset( p_env , 0x00 , sizeof(struct MysqldaEnvironment) );
	
	if( argc == 1 )
	{
		usage();
		exit(7);
	}
	
	for( i = 0 ; i < argc ; i++ )
	{
		if( strcmp( argv[i] , "-f" ) == 0 && i + 1 < argc )
		{
			p_env->config_filename = argv[++i] ;
		}
		else if( strcmp( argv[i] , "--no-daemon" ) == 0 )
		{
			p_env->no_daemon_flag = 1 ;
		}
		else if( strcmp( argv[i] , "-a" ) == 0 && i + 1 < argc )
		{
			p_env->action = argv[++i] ;
		}
	}
	if( p_env->config_filename == NULL || p_env->action == NULL )
	{
		usage();
		exit(7);
	}
	
	if( STRCMP( p_env->action , == , "init" ) )
	{
		return -InitConfigFile( p_env );
	}
	else if( STRCMP( p_env->action , == , "start" ) )
	{
		nret = LoadConfig( p_env ) ;
		if( nret )
			return -nret;
		
		if( p_env->no_daemon_flag )
		{
			return -worker( p_env );
		}
		else
		{
			return -BindDaemonServer( & worker , (void*)p_env , 1 );
		}
	}
}

