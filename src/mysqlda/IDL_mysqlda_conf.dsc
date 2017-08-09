STRUCT	mysqlda_conf
{
	STRUCT	server
	{
		STRING 20	listen_ip
		INT 4		listen_port
	}
	
	STRUCT	auth
	{
		STRING 40	user
		STRING 40	pass
		STRING 40	db
	}
	
	STRUCT	forwards	ARRAY	1000
	{
		STRING 20	instance
		UINT 4		power
		
		STRUCT	forward	ARRAY	8
		{
			STRING 20	ip
			INT 4		port
		}
	}
}

