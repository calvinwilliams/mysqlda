/*
 * rbtree_tpl
 * author	: calvin
 * email	: calvinwilliams@163.com
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#include "mysqlda_in.h"

#include "rbtree_tpl.h"

LINK_RBTREENODE_INT( LinkForwardPowerTreeNode , struct MysqldaEnvironment , forward_power_rbtree , struct ForwardPower , forward_power_rbnode , serial_range_begin )
UNLINK_RBTREENODE( UnlinkForwardPowerTreeNode , struct MysqldaEnvironment , forward_power_rbtree , struct ForwardPower , forward_power_rbnode )
DESTROY_RBTREE( DestroyTcpdaemonAcceptedPowerTree , struct MysqldaEnvironment , forward_power_rbtree , struct ForwardPower , forward_power_rbnode , NULL )
struct ForwardPower *QueryForwardPowerRangeTreeNode( struct MysqldaEnvironment *p_env , unsigned long serial_no )
{
	struct rb_node		*p_node = p_env->forward_power_rbtree.rb_node ;
	struct ForwardPower	*p = NULL ;
	
	while( p_node )
	{
		p = container_of( p_node , struct ForwardPower , forward_power_rbnode ) ;
		
		if( serial_no < p->serial_range_begin )
			p_node = p_node->rb_left ;
		else if( serial_no >= p->serial_range_begin+p->power )
			p_node = p_node->rb_right ;
		else
			return p;
	}
	
	return NULL;
}
TRAVEL_RBTREENODE( TravelForwardPowerTreeNode , struct MysqldaEnvironment , forward_power_rbtree , struct ForwardPower , forward_power_rbnode )

