/*
 * rbtree_tpl
 * author	: calvin
 * email	: calvinwilliams@163.com
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#include "mysqlda_in.h"

#include "rbtree_tpl.h"

LINK_RBTREENODE_STRING( LinkForwardInstanceTreeNode , struct MysqldaEnvironment , forward_instance_rbtree , struct ForwardInstance , forward_instance_rbnode , instance )
QUERY_RBTREENODE_STRING( QueryForwardInstanceTreeNode , struct MysqldaEnvironment , forward_instance_rbtree , struct ForwardInstance , forward_instance_rbnode , instance )
TRAVEL_RBTREENODE( TravelForwardInstanceTreeNode , struct MysqldaEnvironment , forward_instance_rbtree , struct ForwardInstance , forward_instance_rbnode )
UNLINK_RBTREENODE( UnlinkForwardInstanceTreeNode , struct MysqldaEnvironment , forward_instance_rbtree , struct ForwardInstance , forward_instance_rbnode )
DESTROY_RBTREE( DestroyForwardInstanceTree , struct MysqldaEnvironment , forward_instance_rbtree , struct ForwardInstance , forward_instance_rbnode , NULL )

LINK_RBTREENODE_INT( LinkForwardSerialRangeTreeNode , struct MysqldaEnvironment , forward_serial_range_rbtree , struct ForwardInstance , forward_serial_range_rbnode , serial_range_begin )
struct ForwardInstance *QueryForwardSerialRangeTreeNode( struct MysqldaEnvironment *p_env , unsigned long serial_no )
{
	struct rb_node		*p_node = p_env->forward_serial_range_rbtree.rb_node ;
	struct ForwardInstance	*p = NULL ;
	
	while( p_node )
	{
		p = container_of( p_node , struct ForwardInstance , forward_serial_range_rbnode ) ;
		
		if( serial_no < p->serial_range_begin )
			p_node = p_node->rb_left ;
		else if( serial_no >= p->serial_range_begin+p->power )
			p_node = p_node->rb_right ;
		else
			return p;
	}
	
	return NULL;
}
TRAVEL_RBTREENODE( TravelForwardSerialRangeTreeNode , struct MysqldaEnvironment , forward_serial_range_rbtree , struct ForwardInstance , forward_serial_range_rbnode )
UNLINK_RBTREENODE( UnlinkForwardSerialRangeTreeNode , struct MysqldaEnvironment , forward_serial_range_rbtree , struct ForwardInstance , forward_serial_range_rbnode )
DESTROY_RBTREE( DestroyForwardSerialRangeTree , struct MysqldaEnvironment , forward_serial_range_rbtree , struct ForwardInstance , forward_serial_range_rbnode , NULL )

LINK_RBTREENODE_STRING( LinkForwardLibraryTreeNode , struct MysqldaEnvironment , forward_library_rbtree , struct ForwardLibrary , forward_library_rbnode , library )
QUERY_RBTREENODE_STRING( QueryForwardLibraryTreeNode , struct MysqldaEnvironment , forward_library_rbtree , struct ForwardLibrary , forward_library_rbnode , library )
UNLINK_RBTREENODE( UnlinkForwardLibraryTreeNode , struct MysqldaEnvironment , forward_library_rbtree , struct ForwardLibrary , forward_library_rbnode )
DESTROY_RBTREE( DestroyForwardLibraryTree , struct MysqldaEnvironment , forward_library_rbtree , struct ForwardLibrary , forward_library_rbnode , FREE_RBTREENODEENTRY_DIRECTLY )

LINK_RBTREENODE_STRING( LinkForwardCorrelObjectClassTreeNode , struct MysqldaEnvironment , forward_correl_object_class_rbtree , struct ForwardCorrelObjectClass , forward_correl_object_class_rbnode , correl_object_class )
QUERY_RBTREENODE_STRING( QueryForwardCorrelObjectClassTreeNode , struct MysqldaEnvironment , forward_correl_object_class_rbtree , struct ForwardCorrelObjectClass , forward_correl_object_class_rbnode , correl_object_class )
UNLINK_RBTREENODE( UnlinkForwardCorrelObjectClassTreeNode , struct MysqldaEnvironment , forward_correl_object_class_rbtree , struct ForwardCorrelObjectClass , forward_correl_object_class_rbnode )
DESTROY_RBTREE( DestroyForwardCorrelObjectClassTree , struct MysqldaEnvironment , forward_correl_object_class_rbtree , struct ForwardCorrelObjectClass , forward_correl_object_class_rbnode , FREE_RBTREENODEENTRY_DIRECTLY )

LINK_RBTREENODE_STRING( LinkForwardCorrelObjectTreeNode , struct ForwardCorrelObjectClass , forward_correl_object_rbtree , struct ForwardCorrelObject , forward_correl_object_rbnode , correl_object )
QUERY_RBTREENODE_STRING( QueryForwardCorrelObjectTreeNode , struct ForwardCorrelObjectClass , forward_correl_object_rbtree , struct ForwardCorrelObject , forward_correl_object_rbnode , correl_object )
UNLINK_RBTREENODE( UnlinkForwardCorrelObjectTreeNode , struct ForwardCorrelObjectClass , forward_correl_object_rbtree , struct ForwardCorrelObject , forward_correl_object_rbnode )
DESTROY_RBTREE( DestroyForwardCorrelObjectTree , struct ForwardCorrelObjectClass , forward_correl_object_rbtree , struct ForwardCorrelObject , forward_correl_object_rbnode , FREE_RBTREENODEENTRY_DIRECTLY )

