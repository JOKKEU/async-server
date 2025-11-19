#include "../hd/server.h"

static void history_buff_realloc(struct command* command)
{
	size_t new_size = command->history_size + 10;
	char** new_history_cmd = (char**)realloc(command->history_cmd, new_size * sizeof(char*));

	if (!new_history_cmd)
	{
		perror("realloc history_cmd failed");
		exit(EXIT_FAILURE);
	}

	for (size_t i = command->history_size; i < new_size; ++i)
	{
		new_history_cmd[i] = NULL;
	}

	command->history_cmd = new_history_cmd;
	command->history_size = new_size;
}

void add_command_to_history(struct _server* server, struct thread_data* thread_d, char* buffer, int client_fd)
{
	pthread_mutex_lock(&thread_d->mutex_client_check);
	struct client_info* cl_info = find_client_by_fd(server, client_fd);
	pthread_mutex_unlock(&thread_d->mutex_client_check);

	if (!cl_info) {return;}

	time_t now = time(NULL);
	char curr_time[32];
	struct tm *t = localtime(&now);
	strftime(curr_time, sizeof(curr_time), "%Y-%m-%d %H:%M:%S", t);

	size_t required_len =
        strlen("TIME: ") + strlen(curr_time) +
        strlen(" | SocketFD: ") + 10 +
        strlen(" | ClientIP: ") + INET_ADDRSTRLEN +
        strlen(" | command: ") + strlen(buffer) + 2;

	char* full_command = (char*)malloc(required_len);
	if (!full_command)
	{
		perror("malloc full_command failed");
		return;
	}
	snprintf(full_command, required_len,
             "TIME: %s | SocketFD: %d | ClientIP: %s | command: %s\n",
             curr_time, client_fd, cl_info->client_ip, buffer);

	pthread_mutex_lock(&thread_d->mutex_client_check);
	if (server->command->count_cmd >= server->command->history_size)
	{
		history_buff_realloc(server->command);
	}
	size_t index = server->command->count_cmd;
	server->command->history_cmd[index] = full_command;
	server->command->count_cmd++;
	pthread_mutex_unlock(&thread_d->mutex_client_check);
}


void print_command_history(struct command* command)
{
	LOG("--- COMMAND HISTORY  (Total: %zu) ---\n", command->count_cmd);
	if (command->count_cmd == 0) {return;}
	for (size_t index = 0; index < command->count_cmd; ++index){LOG("%s\n", command->history_cmd[index]);}
	return;
}


