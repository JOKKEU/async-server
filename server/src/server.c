#include "../hd/server.h"

int server(void)
{
    	struct _server* server = (struct _server*)malloc(sizeof(struct _server));
    	if (!server)
    	{
		perror("failed to allocate server");
		return EXIT_FAILURE;
   	}
    	start_init_server(server);
    	

    	server->udp_sock_fd = socket(server->sock_param->domain, server->sock_param->type, server->sock_param->protocol);
    	if (server->udp_sock_fd < 0)
   	{
		perror("failed to create socket");
		_CLEAR
		return EXIT_FAILURE;
    	}

	int opt = 1;
    	if (setsockopt (server->udp_sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		_CLEAR
		return EXIT_FAILURE;
	}


	struct local_check* lc = (struct local_check*)malloc(sizeof (struct local_check));
	if (!lc)
	{
		perror("failed to allocate lc");
		_CLEAR
		return EXIT_FAILURE;
	}
	init_local_check(lc);

    	
    	struct thread_data* thread_d = (struct thread_data*)malloc(sizeof (struct thread_data));
	if (!thread_d)
	{
		perror("failed to allocate thread_data");
		_CLEAR
		return EXIT_FAILURE;
	}
	
	struct data_for_thread dft;
	dft.server = server;
	dft.lc = lc;
	dft.thread_d = thread_d;
	
	server->icmp_raw_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (server->icmp_raw_sock < 0)
	{
		perror("socket RAW ICMP failed");
		_CLEAR
		exit(EXIT_FAILURE);
	}

	set_nonblocking_mode(server->icmp_raw_sock);

	struct epoll_event event;
	event.data.fd = server->icmp_raw_sock;
	event.events = EPOLLIN;


	if (epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, server->icmp_raw_sock, &event) == -1)
	{
		perror("epoll_ctl ICMP ADD error");
		_CLEAR
		exit(EXIT_FAILURE);
	}

	set_nonblocking_mode(server->tcp_listen_fd);

	event.data.fd = server->tcp_listen_fd;
	event.events = EPOLLIN | EPOLLET;
	if (epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, server->tcp_listen_fd, &event) == -1)
	{
		perror("epoll_ctl TCP ADD error");
		_CLEAR
		exit(EXIT_FAILURE);
	}


	set_nonblocking_mode(server->udp_sock_fd);

	event.data.fd = server->udp_sock_fd;
	event.events = EPOLLIN | EPOLLET;
	if (epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, server->udp_sock_fd, &event) == -1)
	{
		perror("epoll_ctl UDP ADD error");
		_CLEAR
		exit(EXIT_FAILURE);
	}
	LOG("\n");

	pthread_mutex_init(&thread_d->mutex_client_check, NULL);
	pthread_mutex_init(&thread_d->mutex_client_check_inner, NULL);
	thread_d->client_handler_func = operation_clients;
	
	pthread_mutex_init(&thread_d->lp_mutex, NULL);
	thread_d->client_command_handler = client_command_handler;

	
	pthread_mutex_init(&thread_d->console_handler_mutex, NULL);
	thread_d->console_func_handler = console_handler_server;

	thread_d->broadcast_handler = broadcast_handler;

	get_start_ip_for_local_check(lc, server);
	struct termios orig_termios, new_termios;

	tcgetattr(STDIN_FILENO, &orig_termios);
    	new_termios = orig_termios;

    	set_nonblocking_mode(STDIN_FILENO);

    	new_termios.c_lflag &= ~ICANON;
    	tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);


	
	if (pthread_create(&thread_d->thread_client_check, NULL, thread_d->client_handler_func, &dft) != 0)
    	{
    		perror("failed thread_client_check create");
		_CLEAR
		return EXIT_FAILURE;
    	}
	LOG("Thread: client checker started!\n");
    	if (pthread_create(&thread_d->client_command, NULL, thread_d->client_command_handler, &dft) != 0)
    	{
    		perror("failed client_command create");
		_CLEAR
		return EXIT_FAILURE;
    	}
    	
	LOG("Thread: client command hadnler started!\n");


	if (pthread_create(&thread_d->broadcast, NULL, thread_d->broadcast_handler, &dft) != 0)
    	{
    		perror("failed broadcast create");
		_CLEAR
		return EXIT_FAILURE;
    	}
	LOG("Thread: broadcast started!\n");
    	if (pthread_create(&thread_d->console_handler, NULL, thread_d->console_func_handler, &dft) != 0)
    	{
    		perror("failed clinet_command create");
		_CLEAR
		return EXIT_FAILURE;
    	}

    	LOG("Thread: operation hadnler started!\n");
    	LOG("Server started and listening on port %hu\n", server->port);
    	
    	
   
	pthread_join(thread_d->thread_client_check, NULL);
	pthread_mutex_destroy(&thread_d->mutex_client_check);
	pthread_mutex_destroy(&thread_d->mutex_client_check_inner);
	
	pthread_join(thread_d->console_handler, NULL);
	pthread_mutex_destroy(&thread_d->console_handler_mutex);


	pthread_join(thread_d->client_command, NULL);
	
	all_clear(server, lc, thread_d);
	_CLEAR
    	return EXIT_SUCCESS;
}




