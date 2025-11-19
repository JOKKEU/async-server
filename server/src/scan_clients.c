#include "../hd/server.h"


static void range_ping(size_t* index, size_t max_index, struct data_for_thread* dft)
{
	for (; *index < max_index; ++(*index))
	{
		char ip_for_check[32];
		char index_to_str[16];
		snprintf(index_to_str, sizeof(index_to_str), "%zu", *index);
		snprintf(ip_for_check, INET_ADDRSTRLEN, "%s%s", dft->lc->start_ip, index_to_str);
		if (strcmp(ip_for_check, dft->server->my_ip) == 0) {continue;}
		send_local_net_ping(dft->server, dft->lc, dft->thread_d, ip_for_check);
	}
}


void* operation_clients(void* data)
{
	struct data_for_thread* dft = (struct data_for_thread*)data;
	struct termios orig_termios, new_termios;
	tcgetattr(STDIN_FILENO, &orig_termios);
    	new_termios = orig_termios;

    	set_nonblocking_mode(STDIN_FILENO);

    	new_termios.c_lflag &= ~ICANON;
    	tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

	while (dft->server->flags->stop_thread)
	{
		time_t current_time = time(NULL);
		pthread_mutex_lock(&dft->thread_d->mutex_client_check);
		for (size_t index = 0; index < dft->server->max_my_online_client; ++index)
		{
			struct client_info* cl_info = dft->server->my_online_client_ips[index];

			if (cl_info != NULL)
			{
				time_t client_time = parse_time_string(cl_info->last_activity);
				if (client_time != (time_t)-1 && (current_time - client_time > 15))
				{

					if (cl_info->client_fd != -1)
					{
						epoll_ctl(dft->server->epoll_fd, EPOLL_CTL_DEL, cl_info->client_fd, NULL);
						close(cl_info->client_fd);
					}


					remove_my_online_client_ips(dft->server, dft->thread_d, cl_info->client_ip);

				}
			}
		}

		const int DISCOVERY_TIMEOUT = 60;
		for (size_t index = 0; index < dft->server->max_online_ips; ++index)
		{
			struct active_ip* ip_info = dft->server->all_active_ips[index];

			if (ip_info == NULL)
			{
				continue;
			}

			time_t ip_time = ip_info->last_active;
			if ((current_time - ip_time) > DISCOVERY_TIMEOUT)
			{
				free(ip_info);
				dft->server->all_active_ips[index] = NULL;
				dft->server->all_online_ips--;
			}
		}




		pthread_mutex_unlock(&dft->thread_d->mutex_client_check);
		size_t index = 0;
		range_ping(&index, 51, dft);
		usleep(4000000);
		range_ping(&index, 102, dft);
		usleep(4000000);
		range_ping(&index, 153, dft);
		usleep(4000000);
		range_ping(&index, 204, dft);
		usleep(4000000);
		range_ping(&index, 255, dft);
		usleep(4000000);

	}

	return NULL;
}




void send_local_net_ping(struct _server* server, struct local_check* lc, struct thread_data* thread_d, char* ip_addr)
{
	pthread_mutex_lock(&thread_d->lp_mutex);

	memset(lc->lc_addr, 0, sizeof(struct sockaddr_in));
	lc->lc_addr->sin_family = AF_INET;

	if (inet_pton(AF_INET, ip_addr, &lc->lc_addr->sin_addr) <= 0)
	{
		LOG("send_local_net_ping: Invalid IP address: %s\n", ip_addr);
		goto cleanup;
	}

	memset(lc->lc_packet, 0, sizeof(struct icmp));
	lc->lc_packet->icmp_type = ICMP_ECHO;
	lc->lc_packet->icmp_code = 0;
	lc->lc_packet->icmp_id   = getpid() & 0xFFFF;
	lc->lc_packet->icmp_seq  = 1;
	lc->lc_packet->icmp_cksum = 0;
	lc->lc_packet->icmp_cksum = check_sum((void*)lc->lc_packet, sizeof(struct icmp));

	if (sendto(server->icmp_raw_sock,
		lc->lc_packet, sizeof(struct icmp), 0,
		(struct sockaddr*)lc->lc_addr, sizeof(struct sockaddr_in)) < 0)
	{
		goto cleanup;
	}

cleanup:
	pthread_mutex_unlock(&thread_d->lp_mutex);

}



void* broadcast_handler(void* data)
{
	struct data_for_thread* dft = (struct data_for_thread*)data;
	int socket_broadcast;

	socket_broadcast = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_broadcast < 0)
	{

		perror("broadcast socket create failed");
		return NULL;
	}


	int broadcast_enable = 1;
	if (setsockopt(socket_broadcast, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0)
	{
		perror("setsockopt(SO_BROADCAST) failed");
		close(socket_broadcast);
		return NULL;
	}


	struct sockaddr_in broadcast_addr;
	memset(&broadcast_addr, 0, sizeof(struct sockaddr_in));
	broadcast_addr.sin_family = AF_INET;
	broadcast_addr.sin_port = htons(dft->server->port);
	broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	const char* send_msg = "ready";
	size_t msg_len = strlen(send_msg);
	while (true)
	{
		if (sendto(socket_broadcast,
                   send_msg,
                   msg_len,
                   0,
                   (struct sockaddr*)&broadcast_addr,
                   sizeof(broadcast_addr)
                   ) < 0)
		{
			perror("broadcast sendto failed");
		}
		sleep(3);
	}

	close(socket_broadcast);
	return NULL;
}

