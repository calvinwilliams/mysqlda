#include "mysqlda_in.h"

static void usage()
{
	printf( "USAGE : mysqlda -f (config_filename) --no-daemon -a [ init | start ]\n" );
	printf( "                                     [ --loglevel-debug | --loglevel-info | --loglevel-notice\n" );
	printf( "                                     | --loglevel-warn | --loglevel-error | --loglevel-fatal ]\n" );
	printf( "                -v\n" );
	return;
}

static void version()
{
	printf( "mysqlda v0.0.7.0\n" );
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
	
	/* 不带参数执行则显示命令行语法 */
	if( argc == 1 )
	{
		usage();
		exit(7);
	}
	
	/* 解析命令行参数 */
	p_env->log_level = LOGLEVEL_NOTICE ;
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
		else if( strcmp( argv[i] , "--loglevel-debug" ) == 0 )
		{
			p_env->log_level = LOGLEVEL_DEBUG ;
		}
		else if( strcmp( argv[i] , "--loglevel-info" ) == 0 )
		{
			p_env->log_level = LOGLEVEL_INFO ;
		}
		else if( strcmp( argv[i] , "--loglevel-notice" ) == 0 )
		{
			p_env->log_level = LOGLEVEL_NOTICE ;
		}
		else if( strcmp( argv[i] , "--loglevel-warn" ) == 0 )
		{
			p_env->log_level = LOGLEVEL_WARN ;
		}
		else if( strcmp( argv[i] , "--loglevel-error" ) == 0 )
		{
			p_env->log_level = LOGLEVEL_ERROR ;
		}
		else if( strcmp( argv[i] , "--loglevel-fatal" ) == 0 )
		{
			p_env->log_level = LOGLEVEL_FATAL ;
		}
		else if( strcmp( argv[i] , "-a" ) == 0 && i + 1 < argc )
		{
			p_env->action = argv[++i] ;
		}
		else if( strcmp( argv[i] , "-v" ) == 0 )
		{
			version();
			exit(0);
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
	
	/* 执行行为 */
	if( STRCMP( p_env->action , == , "init" ) )
	{
		/* 创建缺省配置文件 */
		return -InitConfigFile( p_env );
	}
	else if( STRCMP( p_env->action , == , "start" ) )
	{
		if( p_env->no_daemon_flag )
		{
			/* 非守护进程方式启动 */
			nret = monitor( p_env ) ;
		}
		else
		{
			/* 守护进程方式启动 */
			nret = BindDaemonServer( & monitor , (void*)p_env , 1 ) ;
		}
		
		return -nret;
	}
}

