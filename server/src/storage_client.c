#include "../hd/server.h"


void all_online_ips_print(struct _server* server, struct thread_data* thread_d)
{
	LOG("--- ALL CLIENTS HISTORY (Total: %zu) ---\n", server->all_online_ips);
	(void)thread_d;
	if (server->all_online_ips == 0) {return;}
	for(size_t index = 0; index < server->max_online_ips; ++index)
	{
		if (!server->all_active_ips[index]) {continue;}
		char last_response[32];
		format_timestamp(server->all_active_ips[index]->last_active, last_response, 32);
		LOG("[%zu]IP: %s | last response: %s\n", index, server->all_active_ips[index]->ip, last_response);
	}
	return;
}


void my_online_clients_print(struct _server* server, struct thread_data* thread_d)
{
	(void)thread_d;
	LOG("--- ONLINE CLIENTS (Total: %zu) ---\n", server->my_online_client);
	if (server->my_online_client == 0) {return;}
	for(size_t index = 0; index < server->max_my_online_client; ++index)
	{
		if (server->my_online_client_ips[index] == NULL) {continue;}
		struct client_info* cl_info = server->my_online_client_ips[index];

		LOG("[fd: %d] my client on: %s ip\tlast activity: %s\n",
		cl_info->client_fd,
		cl_info->client_ip,
		cl_info->last_activity);
	}
}


void my_clients_print(struct _server* server, struct thread_data* thread_d)
{
	LOG("--- ALL CLIENTS (Total: %zu) ---\n", server->my_client_count);
	(void)thread_d;
	if (server->my_client_count == 0) { return;}
	for(size_t index = 0; index < server->max_my_client; ++index)
	{
		if (server->my_client_ips[index] == NULL) {continue;}
		struct client_info* cl_info = server->my_client_ips[index];

		LOG("[fd: %d] my client on: %s ip\ttime to connect: %s\t alive: %s\n",
		cl_info->client_fd,
		cl_info->client_ip,
		cl_info->last_activity, (cl_info->dead) ? "dead": "active");
	}
}

int check_ip_in_arr(struct _server* server, char* ip_addr)
{
	for (size_t index = 0; index < server->max_online_ips; ++index)
	{
		if (server->all_active_ips[index] == NULL) { continue;}
		if (strcmp(server->all_active_ips[index]->ip, ip_addr) == 0){return EXIT_SUCCESS;}
	}

	return EXIT_FAILURE;
}



void remove_active_ips(struct _server* server, struct thread_data* thread_d, char* ip_addr)
{
	pthread_mutex_lock(&thread_d->mutex_client_check_inner);
	for (size_t index = 0; index < server->max_online_ips; ++index)
	{
		struct active_ip* current = server->all_active_ips[index];
		if (current != NULL && strcmp(current->ip, ip_addr) == 0)
		{
			free(current);

			server->all_active_ips[index] = NULL;
			server->all_online_ips--;
			break;
		}
	}

	pthread_mutex_unlock(&thread_d->mutex_client_check_inner);
}


void remove_my_online_client_ips(struct _server* server, struct thread_data* thread_d, char* ip_addr)
{
	(void)thread_d;
	for (size_t index = 0; index < server->max_my_client; ++index)
	{
		if (server->my_client_ips[index] == NULL) continue;
		if(strcmp(server->my_client_ips[index]->client_ip, ip_addr) == 0)
		{
			server->my_client_ips[index]->dead = true;
		}
	}


	for (size_t index = 0; index < server->max_my_online_client; ++index)
	{
		if (server->my_online_client_ips[index] == NULL) continue;
		struct client_info* cl_info = server->my_online_client_ips[index];
		if(strcmp(cl_info->client_ip, ip_addr) == 0)
		{
			free(cl_info);
			server->my_online_client_ips[index] = NULL;
			server->my_online_client--;
			return;
		}
	}
}




void add_active_ips(struct _server* server, struct thread_data* thread_d, char* ip_addr)
{
	pthread_mutex_lock(&thread_d->mutex_client_check_inner);
	for (size_t index = 0; index < server->max_online_ips; ++index)
	{
		struct active_ip* current = server->all_active_ips[index];
		if (current != NULL && strcmp(current->ip, ip_addr) == 0)
		{
			current->last_active = time(NULL);
			pthread_mutex_unlock(&thread_d->mutex_client_check_inner);
			return;
		}
	}

	if (server->all_online_ips >= server->max_online_ips)
	{
		all_client_ips_realloc(server, thread_d);

		if (server->all_online_ips >= server->max_online_ips)
		{
			LOG("Failed append %s, list overflow, realloc don`t help.\n", ip_addr);
			pthread_mutex_unlock(&thread_d->mutex_client_check_inner);
			return;
		}
	}

	for (size_t index = 0; index < server->max_online_ips; ++index)
	{
		if (server->all_active_ips[index] == NULL)
		{
		struct active_ip* new_ip = malloc(sizeof(struct active_ip));
		if (new_ip == NULL)
		{
			perror("malloc active_ip");
			pthread_mutex_unlock(&thread_d->mutex_client_check_inner);
			return;
		}

		strncpy(new_ip->ip, ip_addr, INET_ADDRSTRLEN);
		new_ip->ip[INET_ADDRSTRLEN - 1] = '\0';
		new_ip->last_active = time(NULL);

		server->all_active_ips[index] = new_ip;
		server->all_online_ips++;

		pthread_mutex_unlock(&thread_d->mutex_client_check_inner);
		return;
		}
	}

	pthread_mutex_unlock(&thread_d->mutex_client_check_inner);
}

struct client_info* find_client_by_fd(struct _server* server, int fd)
{
	for (size_t index = 0; index < server->max_my_online_client; ++index)
	{
		if (server->my_online_client_ips[index] != NULL &&
		server->my_online_client_ips[index]->client_fd == fd)
		{
			return server->my_online_client_ips[index];
		}
	}
	return NULL;
}

void remove_client_by_fd(struct _server* server, struct thread_data* thread_d, int fd)
{
	for (size_t index = 0; index < server->max_my_online_client; ++index)
	{
		struct client_info* cl_info = server->my_online_client_ips[index];

		if (cl_info != NULL && cl_info->client_fd == fd)
		{
			char ip_to_remove[INET_ADDRSTRLEN];
			strncpy(ip_to_remove, cl_info->client_ip, INET_ADDRSTRLEN);
			remove_my_online_client_ips(server, thread_d, ip_to_remove);
			return;
		}
	}
}

void add_client_in_my_online_client_ips(struct _server* server, struct thread_data* thread_d, char* ip_addr, int client_fd)
{
	(void)thread_d;
	add_client_in_my_client_ips(server, thread_d, ip_addr, client_fd);

	for (size_t index = 0; index < server->max_my_online_client; ++index)
	{
		if (server->my_online_client_ips[index] != NULL && strcmp(server->my_online_client_ips[index]->client_ip, ip_addr) == 0)
		{
			server->my_online_client_ips[index]->client_fd = client_fd;
			get_curr_time(server->my_online_client_ips[index]->last_activity);
			return;
		}
	}

	if (server->my_online_client >= server->max_my_online_client)
	{
		my_online_client_ips_realloc(server, thread_d);
	}

	for (size_t index = 0; index < server->max_my_online_client; ++index)
	{
		if (server->my_online_client_ips[index] == NULL)
		{
			struct client_info* new_client = (struct client_info*)malloc(sizeof(struct client_info));
			if (!new_client) {exit(EXIT_FAILURE);}

			new_client->client_fd = client_fd;
			strncpy(new_client->client_ip, ip_addr, INET_ADDRSTRLEN);
			get_curr_time(new_client->last_activity);

			server->my_online_client_ips[index] = new_client;
			server->my_online_client++;
		return;
		}
	}
}


void add_client_in_my_client_ips(struct _server* server, struct thread_data* thread_d, char* ip_addr, int client_fd)
{
	(void)thread_d;
	for (size_t index = 0; index < server->max_my_client; ++index)
	{
		if (server->my_client_ips[index] != NULL && strcmp(server->my_client_ips[index]->client_ip, ip_addr) == 0)
		{
			server->my_client_ips[index]->client_fd = client_fd;
			server->my_client_ips[index]->dead = false;
			get_curr_time(server->my_client_ips[index]->last_activity);
			return;
		}
	}

	if (server->my_client_count >= server->max_my_client)
	{
		my_client_ips_realloc(server, thread_d);
	}


	for (size_t index = 0; index < server->max_my_client; ++index)
	{
		if (server->my_client_ips[index] == NULL)
		{
			struct client_info* new_client = (struct client_info*)malloc(sizeof(struct client_info));
			if (!new_client) {exit(EXIT_FAILURE);}
			new_client->client_fd = client_fd;
			strncpy(new_client->client_ip, ip_addr, INET_ADDRSTRLEN);
			get_curr_time(new_client->last_activity);
			new_client->dead = false;
			server->my_client_ips[index] = new_client;
			server->my_client_count++;
			return;
		}
	}
}


void all_client_ips_realloc(struct _server* server, struct thread_data* thread_d)
{
	(void)thread_d;
	size_t new_size_count = server->max_online_ips + 16;
	size_t new_size_bytes = new_size_count * sizeof(struct active_ip*);
	struct active_ip** temp_ptr = realloc(server->all_active_ips, new_size_bytes);

	if (temp_ptr == NULL)
	{
		perror("error realloc all_active_ips");
		return;
	}

	server->all_active_ips = temp_ptr;
	for (size_t index = server->max_online_ips; index < new_size_count; ++index)
	{
		server->all_active_ips[index] = NULL;
	}
	server->max_online_ips = new_size_count;
}


void my_online_client_ips_realloc(struct _server* server, struct thread_data* thread_d)
{
	(void)thread_d;
	size_t new_size = server->max_my_online_client + 5;
	struct client_info** temp_max_clients = (struct client_info**) malloc(sizeof(struct client_info*) * new_size);

	if (!temp_max_clients)
	{
		perror("error realloc my_client_ips (pointers)");
		exit(EXIT_FAILURE);
	}
	for (size_t index = 0; index < server->max_my_online_client; ++index)
	{
		temp_max_clients[index] = server->my_online_client_ips[index];
	}


	for (size_t index = server->max_my_online_client; index < new_size; ++index)
	{
		temp_max_clients[index] = NULL;
	}

	free(server->my_online_client_ips);

	server->my_online_client_ips = temp_max_clients;
	server->max_my_online_client = new_size;
}

void my_client_ips_realloc(struct _server* server, struct thread_data* thread_d)
{
	(void)thread_d;
	size_t new_size = server->max_my_client + 5;
	struct client_info** temp_max_clients = (struct client_info**) malloc(sizeof(struct client_info*) * new_size);

	if (!temp_max_clients)
	{
		perror("error realloc my_client_ips (pointers)");
		exit(EXIT_FAILURE);
	}
	for (size_t index = 0; index < server->max_my_client; ++index)
	{
		temp_max_clients[index] = server->my_client_ips[index];
	}


	for (size_t index = server->max_my_client; index < new_size; ++index)
	{
		temp_max_clients[index] = NULL;
	}

	free(server->my_client_ips);

	server->my_client_ips = temp_max_clients;
	server->max_my_client = new_size;
}
