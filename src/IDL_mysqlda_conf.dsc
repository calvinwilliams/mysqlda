STRUCT	mysqlda_conf
{
	STRUCT	forward	ARRAY	100
	{
		STRING 20	instance
		STRING 20	ip
		INT 4		port
		STRING 40	user
		STRING 40	pass
		STRING 40	db
		UINT 4		power
	}
	
	STRUCT	server
	{
		STRING 20	listen_ip
		INT 4		listen_port
	}
	
	
}

