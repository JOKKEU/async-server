#include "../hd/server.h"


void dump_before_shutdown(struct _server* server)
{
	char path[256];
	const char* home = getenv("HOME");
	if (!home) home = ".";
	snprintf(path, sizeof(path), "%s/.server", home);
	mkdir(path, 0700);
	snprintf(path, sizeof(path), "%s/.server/server_dump.txt", home);

	FILE* fp = fopen(path, "w");
	if (!fp)
	{
		perror("Failed to open dump file");
		return;
	}

	char t_start[64], t_end[64], t_uptime[64];
	time_t now = time(NULL);

	format_timestamp(server->start_time, t_start, sizeof(t_start));
	format_timestamp(now, t_end, sizeof(t_end));
	format_duration(now - server->start_time, t_uptime, sizeof(t_uptime));

	fprintf(fp, "================ SERVER DUMP ================\n");
	fprintf(fp, "Start Time:  %s\n", t_start);
	fprintf(fp, "End Time:    %s\n", t_end);
	fprintf(fp, "Uptime:      %s\n", t_uptime);
	fprintf(fp, "=============================================\n\n");

	fprintf(fp, "--- ONLINE CLIENTS (Total: %zu) ---\n", server->my_online_client);

	for (size_t i = 0; i < server->max_my_online_client; ++i)
	{
		struct client_info* cl = server->my_online_client_ips[i];
		if (cl != NULL)
		{
			fprintf(fp, "[Index: %zu] IP: %-15s | FD: %d | Last Active: %s\n", i, cl->client_ip, cl->client_fd, cl->last_activity);
		}
	}
	fprintf(fp, "\n");
	fprintf(fp, "--- ALL CLIENTS HISTORY (Total: %zu) ---\n", server->my_client_count);
	for (size_t i = 0; i < server->max_my_client; ++i)
	{
		struct client_info* cl = server->my_client_ips[i];
		if (cl != NULL)
		{
			fprintf(fp, "[Index: %zu] IP: %-15s | FD: %d | Last Active: %s\n", i, cl->client_ip, cl->client_fd, cl->last_activity);
		}
	}
	fprintf(fp, "\n");

	if (server->command)
	{
		fprintf(fp, "--- COMMAND HISTORY (Count: %zu) ---\n", server->command->count_cmd);
		for (size_t i = 0; i < server->command->count_cmd; ++i)
		{
			if (server->command->history_cmd[i])
			{
				fprintf(fp, "%zu: %s\n", i + 1, server->command->history_cmd[i]);
			}
		}
	}
	else
	{
		fprintf(fp, "--- COMMAND HISTORY IS NULL ---\n");
	}


	fclose(fp);
	LOG("Dump saved successfully to %s\n", path);
}
