# 此文件由makeobj.sh自动生成
############################################################
# 项目名 : 
# 模块名 : 
# 备  注 : 
############################################################

###### 源文件配置区
#@ c_FILE
c_FILE		=	\
			mysqlda_test_connect.c \
			mysqlda_test_select_library.c \
			mysqlda_test_insert.c \

###### 目标文件、安装目录配置区
include makeinstall
BIN		=	mysqlda_test_connect \
			mysqlda_test_select_library \

#			mysqlda_test_insert \

###### 编译选项
CFLAGS		=	$(_CFLAGS) \
			-std=gnu99 \
			-I$(HOME)/include/mysqlda_api \
			-I/usr/include/mysql \

###### 链接选项
LFLAGS		=	$(_LFLAGS) \
			-L$(HOME)/lib \
			-lmysqlda_api \
			-L/usr/lib64/mysql \
			-lmysqlclient \

###### 额外宏定义区
CLEAN_ADDITION	=

###### 加载mktpl模板库
#@ make_all
#@ make_clean
#@ make_install
#@ make_install_BININST
#@ make_install_DFTHDERINST
#@ make_uninstall
#@ make_uninstall_BININST
#@ make_uninstall_DFTHDERINST
include $(MKTPLDIR)/makeobj_$(MKTPLOS).inc

###### 目标文件依赖关系
mysqlda_test_connect	:	mysqlda_test_connect.o
	$(CC) -o $@ mysqlda_test_connect.o $(LFLAGS)

mysqlda_test_select_library	:	mysqlda_test_select_library.o
	$(CC) -o $@ mysqlda_test_select_library.o $(LFLAGS)

