#include "../hd/server.h"


static void cleanup_lc(struct local_check* lc);
static void cleanup_server(struct _server* server);


void start_init_server(struct _server* server)
{

	server->start_time = time(NULL);

	server->start_device_count = 	CLIENTS;
	server->max_online_ips = 	CLIENTS;
	server->max_my_online_client =	CLIENTS;
	server->max_my_client =		CLIENTS;

	server->all_active_ips = (struct active_ip**)malloc(sizeof (struct active_ip*) * server->max_online_ips);
	if (!server->all_active_ips)
	{
		perror("failed alloc all_active_ips");
		exit(EXIT_FAILURE);
	}

	for (size_t index = 0; index < server->max_online_ips; ++index)
	{

		server->all_active_ips[index] = (struct active_ip*)malloc(sizeof (struct active_ip) * INET_ADDRSTRLEN);
		if (!server->all_active_ips[index])
		{
			perror("online users[i] alloc error");
			for (size_t j = 0; j < index; ++j)
			{
				free(server->all_active_ips[j]);
			}
			
			free(server->all_active_ips);
			exit(EXIT_FAILURE);
		}

		server->all_active_ips[index] = NULL;
	}
	
	server->my_online_client_ips = (struct client_info**)malloc(sizeof (struct client_info*) * server->max_my_online_client);
	if (!server->my_online_client_ips)
	{
		perror("failed alloc my_online_client_ips");
		exit(EXIT_FAILURE);
	}

	for (size_t index = 0; index < server->max_my_online_client; ++index)
	{

		server->my_online_client_ips[index] = (struct client_info*)malloc(sizeof (struct client_info));
		if (!server->my_online_client_ips[index])
		{
			perror("online users[i] alloc error");
			for (size_t j = 0; j < index; ++j)
			{
				free(server->my_online_client_ips[j]);
			}
			
			free(server->my_online_client_ips);
			exit(EXIT_FAILURE);
		}

		memset(server->my_online_client_ips[index], 0, sizeof(struct client_info));
		server->my_online_client_ips[index] = NULL;

	}

	server->my_client_ips = (struct client_info**)malloc(sizeof (struct client_info*) * server->max_my_client);
	if (!server->my_client_ips)
	{
		perror("failed alloc my_client_ips");
		exit(EXIT_FAILURE);
	}

	for (size_t index = 0; index < server->max_my_client; ++index)
	{

		server->my_client_ips[index] = (struct client_info*)malloc(sizeof (struct client_info));
		if (!server->my_client_ips[index])
		{
			perror("online users[i] alloc error");
			for (size_t j = 0; j < index; ++j)
			{
				free(server->my_client_ips[j]);
			}

			free(server->my_client_ips);
			exit(EXIT_FAILURE);
		}

		memset(server->my_client_ips[index], 0, sizeof(struct client_info));
		server->my_client_ips[index] = NULL;
	}

	server->port = 			PORT;
	server->cleanup_server =	cleanup_server;
	
	
	
	server->sock_param = (struct socket_param*)malloc(sizeof (struct socket_param));
	if (!server->sock_param)
	{
		perror("failed alloc all_client_ips");
		exit(EXIT_FAILURE);
	}

	server->command = (struct command*)malloc(sizeof(struct command));
	if (!server->command)
	{
		perror("failed alloc command");
		exit(EXIT_FAILURE);
	}
	server->command->count_cmd = 0;
	server->command->history_size = 10;
	server->command->history_cmd = (char**)malloc(sizeof(char*) * server->command->history_size);
	if (!server->command->history_cmd) {perror("failed alloc command"); exit(EXIT_FAILURE);}
	for (size_t i = 0; i < server->command->history_size; ++i)
	{
		server->command->history_cmd[i] = NULL;
	}

	server->flags = (struct _flags*)malloc(sizeof (struct _flags));
	if (!server->flags)
	{
		perror("failed alloc flags");
		exit(EXIT_FAILURE);
	}
	
	server->buffer_for_accept = (char*)malloc(sizeof(char) * BUFFER_ACCEPT_SIZE);
	if (!server->buffer_for_accept)
	{
		perror("failed alloc buffer_for_accept");
		exit(EXIT_FAILURE);
	}
	
	server->flags->stop_thread = 	true;
	server->flags->stop_ot = 	true;
	server->sock_param->domain = 	AF_INET;
	server->sock_param->type = 	SOCK_DGRAM;
	server->sock_param->protocol =	0;
	server->sock_param->opt = 	1;
	server->sock_param->backlog =	server->start_device_count;
	server->all_online_ips = 	0;
	server->my_client_count =	0;
	server->my_online_client =	0;


	server->epoll_fd = epoll_create1(0);
	if (server->epoll_fd == -1)
	{
		perror("epoll_create1 failed");
		exit(EXIT_FAILURE);
	}

	server->tcp_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server->tcp_listen_fd < 0)
	{
		perror("socket TCP failed");
		exit(EXIT_FAILURE);
	}

	int opt = 1;
	if (setsockopt(server->tcp_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		perror("setsockopt TCP");
		close(server->tcp_listen_fd);
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in tcp_addr;
	memset(&tcp_addr, 0, sizeof(tcp_addr));
	tcp_addr.sin_family = AF_INET;
	tcp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	tcp_addr.sin_port = htons(PORT);

	if (bind(server->tcp_listen_fd, (struct sockaddr*)&tcp_addr, sizeof(tcp_addr)) < 0)
	{
		perror("bind TCP failed");
		close(server->tcp_listen_fd);
		exit(EXIT_FAILURE);
	}

	if (listen(server->tcp_listen_fd, server->sock_param->backlog) < 0)
	{
		perror("listen TCP failed");
		close(server->tcp_listen_fd);
		exit(EXIT_FAILURE);
	}

}




void init_local_check(struct local_check* lc)
{
	lc->lc_packet = (struct icmp*)malloc(sizeof (struct icmp));
	if (!lc->lc_packet)
	{
		perror("lc_packet alloc error");
		exit(EXIT_FAILURE);
	}
	
	lc->lc_addr = (struct sockaddr_in*)malloc(sizeof (struct sockaddr_in));
	if (!lc->lc_addr)
	{
		perror("lc_addr alloc error");
		exit(EXIT_FAILURE);
	}
	
	lc->start_ip = (char*)malloc(INET_ADDRSTRLEN + 1);
    	if (!lc->start_ip)
    	{
    		perror("error start_ip");
		exit(EXIT_FAILURE);
    	}
	
	lc->cleanup_lc = 		cleanup_lc;
	lc->lc_sockfd = 		0;
	
	
}



static void cleanup_lc(struct local_check* lc)
{	
	if (lc)
	{
		
		if (lc->lc_packet)
		{
			free(lc->lc_packet);
		}
		
		if (lc->lc_addr)
		{
			free(lc->lc_addr);
		}

	}
	
}



static void cleanup_server(struct _server* server)
{
	if (server)
	{
		if (server->all_active_ips)
		{
			for (size_t index = 0; index < server->max_online_ips; ++index)
			{
				if (!server->all_active_ips[index]) {free(server->all_active_ips[index]);}
			}
			free(server->all_active_ips);
		}
		
		if (server->sock_param)
		{
			free(server->sock_param);
		}
		
		if (server->my_client_ips)
		{
			for (size_t index = 0; index < server->max_my_client; ++index)
			{
				if (!server->my_client_ips[index]) {free(server->my_client_ips[index]);}
			}
			free(server->my_client_ips);
		}

		if (server->my_online_client_ips)
		{
			for (size_t index = 0; index < server->max_my_online_client; ++index)
			{
				if (!server->my_online_client_ips[index]) {free(server->my_online_client_ips[index]);}
			}
			free(server->my_online_client_ips);
		}
		if (server->command)
		{
			if (server->command->history_cmd)
			{
				free(server->command->history_cmd);
			}
			free(server->command);
		}


		close(server->epoll_fd);
		close(server->icmp_raw_sock);
		close(server->tcp_listen_fd);
		close(server->udp_sock_fd);

	}

}


void all_clear(struct _server* server, struct local_check* lc, struct thread_data* thread_d)
{
	if (thread_d)
	{
		free(thread_d);
	}
	
	if (lc)
	{
		lc->cleanup_lc(lc);
	}
	
    		
    	if (server)
    	{
    		server->cleanup_server(server);  
    	}  
	
}


