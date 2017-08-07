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
	char				config_pathfilename[ 256 + 1 ] ;
	char				save_pathfilename[ 256 + 1 ] ;
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
		else if( strcmp( argv[i] , "-s" ) == 0 && i + 1 < argc )
		{
			p_env->save_filename = argv[++i] ;
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
	if( p_env->action == NULL )
	{
		usage();
		exit(7);
	}
	
	if( p_env->config_filename == NULL )
	{
		memset( config_pathfilename , 0x00 , sizeof(config_pathfilename) );
		snprintf( config_pathfilename , sizeof(config_pathfilename)-1 , "%s/etc/mysqlda.conf" , getenv("HOME") );
		
		p_env->config_filename = config_pathfilename ;
	}
	
	if( p_env->save_filename == NULL )
	{
		memset( save_pathfilename , 0x00 , sizeof(save_pathfilename) );
		snprintf( save_pathfilename , sizeof(save_pathfilename)-1 , "%s/etc/mysqlda.save" , getenv("HOME") );
		
		p_env->save_filename = save_pathfilename ;
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

