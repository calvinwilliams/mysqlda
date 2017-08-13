#include "mysqlda_in.h"

static int	g_exit_flag = 0 ;

static void SignalProcess( int sig_no )
{
	if( sig_no == SIGTERM || sig_no == SIGINT )
	{
		g_exit_flag = 1 ;
	}
	
	return;
}

static int _monitor( struct MysqldaEnvironment *p_env )
{
	struct sigaction	act ;
	
	pid_t			pid ;
	int			status ;
	
	int			nret = 0 ;
	
	/* ÉèÖÃÐÅºÅµÆ */
	act.sa_handler = SignalProcess ;
	sigemptyset( & (act.sa_mask) );
	act.sa_flags = 0 ;
	sigaction( SIGTERM , & act , NULL );
	sigaction( SIGINT , & act , NULL );
	signal( SIGCLD , SIG_DFL );
	signal( SIGCHLD , SIG_DFL );
	signal( SIGPIPE , SIG_IGN );
	signal( SIGHUP , SIG_IGN );
	
	while( ! g_exit_flag )
	{
_GOTO_RETRY_PIPE :
		nret = pipe( p_env->alive_pipe_session.alive_pipe ) ;
		if( nret == -1 )
		{
			if( errno == EINTR )
				goto _GOTO_RETRY_PIPE;
			FATALLOG( "pipe failed , errno[%d]" , errno );
			return -1;
		}
		
_GOTO_RETRY_FORK :
		pid = fork() ;
		if( pid == -1 )
		{
			if( errno == EINTR )
				goto _GOTO_RETRY_FORK;
			FATALLOG( "fork failed , errno[%d]" , errno );
			return -1;
		}
		else if( pid == 0 )
		{
			INFOLOG( "[%d]fork[%d] ok" , getppid() , getpid() );
			close( p_env->alive_pipe_session.alive_pipe[1] );
			exit( worker((void*)p_env) );
		}
		else
		{
			INFOLOG( "[%d]fork[%d] ok" , getpid() , pid );
			close( p_env->alive_pipe_session.alive_pipe[0] );
		}
		
_GOTO_RETRY_WAITPID :
		pid = waitpid( pid , & status , 0 ) ;
		if( pid == -1 ) 
		{
			if( errno == EINTR )
			{
				if( g_exit_flag == 1 )
					close( p_env->alive_pipe_session.alive_pipe[1] );
				goto _GOTO_RETRY_WAITPID;
			}
			FATALLOG( "waitpid failed , errno[%d]" , errno );
			return -1;
		}
		else
		{
			INFOLOG( "waitpid[%d] ok" , pid );
			if( g_exit_flag == 0 )
				close( p_env->alive_pipe_session.alive_pipe[1] );
		}
		
		sleep(1);
	}
	
	return 0;
}

int monitor( void *pv )
{
	struct MysqldaEnvironment	*p_env = (struct MysqldaEnvironment *) pv ;
	
	int				nret = 0 ;
	
	SetLogFile( "%s/log/mysqlda.log" , getenv("HOME") );
	SetLogLevel( LOGLEVEL_DEBUG );
	
	nret = _monitor( p_env ) ;
	INFOLOG( "monitor exit ..." );
	return -nret;
	
}

