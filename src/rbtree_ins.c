/*
 * rbtree_tpl
 * author	: calvin
 * email	: calvinwilliams@163.com
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#include "mysqlda_in.h"

#include "rbtree_tpl.h"

LINK_RBTREENODE_INT( LinkMysqlForwardClientTreeNode , struct MysqldaEnvironment , client_rbtree , struct MysqlForwardClient , client_rbnode , serial_range_begin )
UNLINK_RBTREENODE( UnlinkMysqlForwardClientTreeNode , struct MysqldaEnvironment , client_rbtree , struct MysqlForwardClient , client_rbnode )
DESTROY_RBTREE( DestroyTcpdaemonAcceptedSessionTree , struct MysqldaEnvironment , client_rbtree , struct MysqlForwardClient , client_rbnode , NULL )
struct MysqlForwardClient *QueryMysqlForwardClientRangeTreeNode( struct MysqldaEnvironment *p_env , unsigned long serial_no )
{
	struct rb_node			*p_node = p_env->client_rbtree.rb_node ;
	struct MysqlForwardClient	*p = NULL ;
	
	while( p_node )
	{
		p = container_of( p_node , struct MysqlForwardClient , client_rbnode ) ;
		
		if( serial_no < p->serial_range_begin )
			p_node = p_node->rb_left ;
		else if( serial_no >= p->serial_range_begin+p->power )
			p_node = p_node->rb_right ;
		else
			return p;
	}
	
	return NULL;
}
TRAVEL_RBTREENODE( TravelMysqlForwardClientTreeNode , struct MysqldaEnvironment , client_rbtree , struct MysqlForwardClient , client_rbnode )

