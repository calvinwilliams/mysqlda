# 此文件由makeobj.sh自动生成
############################################################
# 项目名 : 
# 模块名 : 
# 备  注 : 
############################################################

###### 源文件配置区
#@ c_FILE
c_FILE		=	\
			lk_list.c \
			rbtree.c \
			LOGC.c \
			fasterjson.c \
			util.c \
			rbtree_ins.c \
			IDL_mysqlda_conf.dsc.c \
			main.c \
			config.c \
			worker.c \
			comm.c \
			app.c \

###### 目标文件、安装目录配置区
include makeinstall
BIN		=	mysqlda
BININST		=	$(_BININST)
HDER		= \

HDERINST	=	$(_HDERINST)

###### 编译选项
CFLAGS		=	$(_CFLAGS) \
			-std=gnu99 \
			-I/usr/include/mysql \

###### 链接选项
LFLAGS		=	$(_LFLAGS) \
			-L/usr/lib64/mysql \
			-lmysqlclient \
			-lcrypto \

###### 额外宏定义区
CLEAN_ADDITION	=

###### 加载mktpl模板库
#@ make_all
#@ make_clean
#@ make_install
#@ make_install_BININST
#@ make_install_HDERINST
#@ make_install_DFTHDERINST
#@ make_uninstall
#@ make_uninstall_BININST
#@ make_uninstall_HDERINST
#@ make_uninstall_DFTHDERINST
include $(MKTPLDIR)/makeobj_$(MKTPLOS).inc

###### 目标文件依赖关系
mysqlda		:	$(c_FILE_o)
	$(CC) -o $@ $(c_FILE_o) $(LFLAGS)

