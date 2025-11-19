#include "../hd/server.h"

void handle_new_tcp_connection(struct _server* server, struct thread_data* thread_d)
{
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	while (true)
	{
		int new_client_fd = accept(server->tcp_listen_fd, (struct sockaddr *)&client_addr, &client_len);

		if (new_client_fd < 0)
		{
			if (errno == EWOULDBLOCK || errno == EAGAIN)
			{
				break;
			}
			perror("accept error");
			return;
		}

		set_nonblocking_mode(new_client_fd);

		struct epoll_event event;
		event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
		event.data.fd = new_client_fd;

		if (epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, new_client_fd, &event) == -1)
		{
			perror("epoll_ctl ADD error");
			close(new_client_fd);
			continue;
		}

		char client_ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

		pthread_mutex_lock(&thread_d->mutex_client_check);
		add_client_in_my_online_client_ips(server, thread_d, client_ip, new_client_fd);
		pthread_mutex_unlock(&thread_d->mutex_client_check);
	}
}


void handle_icmp_reply_async(struct _server* server, struct thread_data* thread_d)
{
	char recv_buffer[ICMP_PACKET_SIZE];
	struct sockaddr_in sender_addr;
	socklen_t addr_len = sizeof(sender_addr);
	ssize_t bytes_read;


	bytes_read = recvfrom(server->icmp_raw_sock, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr*)&sender_addr, &addr_len);

	if (bytes_read <= 0)
	{
		if (errno != EWOULDBLOCK && errno != EAGAIN)
		{
			perror("recvfrom ICMP error");
		}
		return;
	}

	char ip_addr[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &sender_addr.sin_addr, ip_addr, INET_ADDRSTRLEN);


	struct ip* ip_hdr = (struct ip*)recv_buffer;
	int ip_hdr_len = ip_hdr->ip_hl * 4;
	struct icmphdr* icmp_hdr = (struct icmphdr*)(recv_buffer + ip_hdr_len);


	if (icmp_hdr->type == ICMP_ECHOREPLY)
	{

		pthread_mutex_lock(&thread_d->mutex_client_check);
		add_active_ips(server, thread_d, ip_addr);
		pthread_mutex_unlock(&thread_d->mutex_client_check);
	}
}


int handle_tcp_data(struct _server* server, struct thread_data* thread_d, int fd)
{
	char buffer[BUFFER_ACCEPT_SIZE];
	ssize_t count;

	while (true)
	{

		count = recv(fd, buffer, sizeof(buffer) - 1, 0);

		if (count == -1)
		{

			if (errno == EWOULDBLOCK || errno == EAGAIN)
			{
				break;
			}

			handle_tcp_disconnection(server, thread_d, fd);
			return 0;
		}

		if (count == 0)
		{

			handle_tcp_disconnection(server, thread_d, fd);
			return 0;
		}


		buffer[count] = '\0';
		if (count > 0 && buffer[count-1] == '\n') buffer[count-1] = '\0';

		pthread_mutex_lock(&thread_d->mutex_client_check);
		struct client_info* cl_info = find_client_by_fd(server, fd);
		if (cl_info)
		{
			get_curr_time(cl_info->last_activity);
		}
		pthread_mutex_unlock(&thread_d->mutex_client_check);


		char response[256];
		memset(response, 0, sizeof(response));
		if (buffer[0] ==  '/')
		{
			if (strcmp(buffer, "/time") == 0)
			{
				time_t now = time(NULL);
				struct tm *t = localtime(&now);
				strftime(response, sizeof(response), "%Y-%m-%d %H:%M:%S\n", t);
				send(fd, response, strlen(response), 0);

				goto command_history;
			}
			else if (strcmp(buffer, "/stats") == 0)
			{
				pthread_mutex_lock(&thread_d->mutex_client_check);
				int current = server->my_online_client;
				int total = server->my_client_count;
				pthread_mutex_unlock(&thread_d->mutex_client_check);

				snprintf(response, sizeof(response), "Online: %d, Total History: %d\n", current, total);
				send(fd, response, strlen(response), 0);

				goto command_history;
			}

			else if (strcmp(buffer, "/shutdown") == 0)
			{
				const char* msg = "Server shutting down...\n";
				send(fd, msg, strlen(msg), 0);

				server->flags->stop_thread = 0;
				server->flags->stop_ot = 0;

				add_command_to_history(server, thread_d, buffer, fd);
				pthread_mutex_lock(&thread_d->mutex_client_check);
				dump_before_shutdown(server);
				pthread_mutex_unlock(&thread_d->mutex_client_check);

				return 13;
			}
			else
			{
				const char* buff = "command not support";
				send(fd, (void*)buff, strlen(buff), 0) ;
			}

command_history:
			add_command_to_history(server, thread_d, buffer, fd);
		}
		else
		{
			strncat(buffer, "\n", sizeof(buffer) - strlen(buffer) - 1);
			send(fd, buffer, strlen(buffer), 0);
		}
	}
	return 0;
}


void handle_tcp_disconnection(struct _server* server, struct thread_data* thread_d, int fd)
{
	if (epoll_ctl(server->epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1)
	{
		perror("epoll_ctl DEL error");
	}


	if (close(fd) == -1)
	{

		perror("close client fd error");
	}

	pthread_mutex_lock(&thread_d->mutex_client_check);
	remove_client_by_fd(server, thread_d, fd);
	pthread_mutex_unlock(&thread_d->mutex_client_check);

}
void handle_udp_data_async(struct _server* server, struct thread_data* thread_d)
{
	char buffer[BUFFER_ACCEPT_SIZE];
	struct sockaddr_in client_addr;
	socklen_t addr_len = sizeof(client_addr);
	ssize_t count;

	count = recvfrom(server->udp_sock_fd, buffer, sizeof(buffer) - 1, 0,
                     (struct sockaddr*)&client_addr, &addr_len);

	if (count > 0)
	{
		buffer[count] = '\0';
		char client_ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

		if (strcmp(buffer, "ready") == 0 || strcmp(buffer, "ready\n") == 0)
		{
			if (strcmp(client_ip, server->my_ip) == 0 || strcmp(client_ip, "127.0.0.1") == 0) {return;}
			pthread_mutex_lock(&thread_d->mutex_client_check);
			add_client_in_my_client_ips(server, thread_d, client_ip, 0);
			pthread_mutex_unlock(&thread_d->mutex_client_check);

		}



	}
	else if (count < 0)
	{

		if (errno != EWOULDBLOCK && errno != EAGAIN)
		{
			perror("recvfrom error in handle_udp_data_async");
		}
	}
}


void* client_command_handler(void* data)
{
	struct data_for_thread* dft = (struct data_for_thread*)data;
	struct _server* server = dft->server;

	struct epoll_event events[MAX_EVENTS];
	int nfds;

	if (server->epoll_fd < 0 || server->tcp_listen_fd < 0 || server->udp_sock_fd < 0)
	{
		LOG("Error: Server sockets or epoll not initialized.\n");
		return NULL;
	}

	while (dft->server->flags->stop_thread)
	{
		nfds = epoll_wait(server->epoll_fd, events, MAX_EVENTS, 100);

		if (nfds == -1)
		{
			if (errno == EINTR) continue;
			perror("epoll_wait error");
			break;
		}

		for (int n = 0; n < nfds; ++n)
		{
			int current_fd = events[n].data.fd;
			uint32_t current_events = events[n].events;

			if (current_fd == server->tcp_listen_fd)
			{
				if (current_events & EPOLLIN)
				{
					handle_new_tcp_connection(server, dft->thread_d);
				}
			}

			else if (current_fd == server->udp_sock_fd)
			{
				if (current_events & EPOLLIN)
				{
					handle_udp_data_async(server, dft->thread_d);
				}
			}

			else if (current_fd == server->icmp_raw_sock)
			{
				if (current_events & EPOLLIN)
				{
					handle_icmp_reply_async(server, dft->thread_d);
				}
			}


			else
			{
				if (current_events & EPOLLIN)
				{
					if (handle_tcp_data(server, dft->thread_d, current_fd) == 13) // call shutdown
					{
						all_clear(dft->server, dft->lc, dft->thread_d);
						exit(EXIT_SUCCESS);
					}

				}

				else if (current_events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP))
				{
					handle_tcp_disconnection(server, dft->thread_d, current_fd);
				}

			}
		}
    }

    return NULL;
}
