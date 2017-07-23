/*
 * rbtree_tpl
 * author	: calvin
 * email	: calvinwilliams@163.com
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#include "mysqlda_in.h"

#include "rbtree_tpl.h"

LINK_RBTREENODE_INT( LinkForwardSessionTreeNode , struct MysqldaEnvironment , forward_rbtree , struct ForwardSession , forward_rbnode , serial_range_begin )
UNLINK_RBTREENODE( UnlinkForwardSessionTreeNode , struct MysqldaEnvironment , forward_rbtree , struct ForwardSession , forward_rbnode )
DESTROY_RBTREE( DestroyTcpdaemonAcceptedSessionTree , struct MysqldaEnvironment , forward_rbtree , struct ForwardSession , forward_rbnode , NULL )
struct ForwardSession *QueryForwardSessionRangeTreeNode( struct MysqldaEnvironment *p_env , unsigned long serial_no )
{
	struct rb_node		*p_node = p_env->forward_rbtree.rb_node ;
	struct ForwardSession	*p = NULL ;
	
	while( p_node )
	{
		p = container_of( p_node , struct ForwardSession , forward_rbnode ) ;
		
		if( serial_no < p->serial_range_begin )
			p_node = p_node->rb_left ;
		else if( serial_no >= p->serial_range_begin+p->power )
			p_node = p_node->rb_right ;
		else
			return p;
	}
	
	return NULL;
}
TRAVEL_RBTREENODE( TravelForwardSessionTreeNode , struct MysqldaEnvironment , forward_rbtree , struct ForwardSession , forward_rbnode )

