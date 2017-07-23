#include "mysqlda_in.h"

int OnAcceptingSocket( struct MysqldaEnvironment *p_env , struct ListenSession *p_listen_session )
{
	return 0;
}

int OnReceivingAcceptedSocket( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session )
{
	return 0;
}

int OnSendingAcceptedSocket( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session )
{
	return 0;
}

int OnClosingAcceptedSocket( struct MysqldaEnvironment *p_env , struct AcceptedSession *p_accepted_session )
{
	return 0;
}

int OnReceivingForwardSocket( struct MysqldaEnvironment *p_env , struct ForwardSession *p_forward_session )
{
	return 0;
}

int OnSendingForwardSocket( struct MysqldaEnvironment *p_env , struct ForwardSession *p_forward_session )
{
	return 0;
}

int OnClosingForwardSocket( struct MysqldaEnvironment *p_env , struct ForwardSession *p_forward_session )
{
	return 0;
}

