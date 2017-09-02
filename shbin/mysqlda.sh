#!/bin/bash

usage()
{
	echo "USAGE : mysqlda.sh [ status | start | stop | kill | restart | reload ]"
}

if [ $# -eq 0 ] ; then
	usage
	exit 9
fi

ACTION=$1
shift

case $ACTION in
	status)
		ps -f -u $USER | grep "mysqlda -a start" | grep -v grep | awk '{if($3=="1")print $0}'
		ps -f -u $USER | grep "mysqlda -a start" | grep -v grep | awk '{if($3!="1")print $0}'
		;;
	start)
		if [ x"$MYSQLDA_LOGDIR" != x"" ] ; then
			if [ ! -d $MYSQLDA_LOGDIR ] ; then
				mkdir -p $MYSQLDA_LOGDIR
			fi
		else
			if [ ! -d $HOME/log ] ; then
				mkdir -p $HOME/log 
			fi
		fi
		
		PID=`ps -f -u $USER | grep "mysqlda -a start" | grep -v grep | awk '{if($3=="1")print $2}'`
		if [ x"$PID" != x"" ] ; then
			echo "*** ERROR : mysqlda existed"
			exit 1
		fi
		mysqlda -a start $*
		NRET=$?
		if [ $NRET -ne 0 ] ; then
			echo "*** ERROR : mysqlda start error[$NRET]"
			exit 1
		fi
		while [ 1 ] ; do
			sleep 1
			PID=`ps -f -u $USER | grep "mysqlda -a start" | grep -v grep | awk '{if($3=="1")print $2}'`
			if [ x"$PID" != x"" ] ; then
				break
			fi
		done
		echo "mysqlda start ok"
		mysqlda.sh status
		;;
	stop)
		mysqlda.sh status
		if [ $? -ne 0 ] ; then
			exit 1
		fi
		PID=`ps -f -u $USER | grep "mysqlda -a start" | grep -v grep | awk '{if($3=="1")print $2}'`
		if [ x"$PID" = x"" ] ; then
			echo "*** ERROR : mysqlda not existed"
			exit 1
		fi
		kill $PID
		while [ 1 ] ; do
			sleep 1
			PID=`ps -f -u $USER | grep "mysqlda -a start" | grep -v grep | awk '{if($3=="1")print $2}'`
			if [ x"$PID" = x"" ] ; then
				break
			fi
		done
		echo "mysqlda end ok"
		;;
	kill)
		mysqlda.sh status
		ps -f -u $USER | grep "mysqlda -a start" | grep -v grep | awk '{print $2}' | while read PID ; do
			kill -9 $PID
		done
		;;
	restart)
		mysqlda.sh stop
		sleep 1
		mysqlda.sh start $*
		;;
	reload)
		mysqlda.sh status
		if [ $? -ne 0 ] ; then
			exit 1
		fi
		PID=`ps -f -u $USER | grep "mysqlda -a start" | grep -v grep | awk '{if($3=="1")print $2}'`
		if [ x"$PID" = x"" ] ; then
			echo "*** ERROR : mysqlda not existed"
			exit 1
		fi
		kill -USR1 $PID
		echo "mysqlda reload ok"
		;;
	*)
		usage
		;;
esac

