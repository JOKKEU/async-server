#include "../hd/server.h"

void* console_handler_server(void* data)
{
	struct data_for_thread* dft = (struct data_for_thread*)data;
	struct termios orig_termios, new_termios;


	tcgetattr(STDIN_FILENO, &orig_termios);
	new_termios = orig_termios;
	set_nonblocking_mode(STDIN_FILENO);
	new_termios.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

	char operation[256];
	size_t index = 0;


	printf(ANSI_COLOR_CYAN "\n=== Admin Console Ready ===\n" ANSI_COLOR_RESET);
	printf("Type 'help' for commands.\n");


	printf(ANSI_COLOR_YELLOW "ADMIN > " ANSI_COLOR_RESET);
	fflush(stdout);

	while (true)
	{

		if (!dft->server->flags->stop_ot) break;
		char c;
		int n = read(STDIN_FILENO, &c, 1);

		if (n == -1)
		{
			if (errno == EAGAIN)
			{
				usleep(50000);
				continue;
			}
			perror("read error");
			break;
		}

		if (c == '\n')
		{
			operation[index] = '\0';
			printf("\n");
			index = 0;

			if (strlen(operation) == 0) {}
			else if (strcmp(operation, "help") == 0)
			{
				printf(ANSI_COLOR_CYAN "--- Available Commands ---\n" ANSI_COLOR_RESET);
				printf(" [1] Online clients\n");
				printf(" [2] History of clients\n");
				printf(" [3] Command history\n");
				printf(" [4] Active IPs (scan results)\n");
				printf(" [5] Server uptime\n");
				printf(" [clear] Clear screen\n");
				printf(" [exit] Stop server\n");
			}
			else if (strcmp(operation, "1") == 0)
			{
				pthread_mutex_lock(&dft->thread_d->mutex_client_check);
				my_online_clients_print(dft->server, dft->thread_d);
				pthread_mutex_unlock(&dft->thread_d->mutex_client_check);
			}
			else if (strcmp(operation, "2") == 0)
			{
				pthread_mutex_lock(&dft->thread_d->mutex_client_check);
				my_clients_print(dft->server, dft->thread_d);
				pthread_mutex_unlock(&dft->thread_d->mutex_client_check);
			}
			else if (strcmp(operation, "3") == 0)
			{
				pthread_mutex_lock(&dft->thread_d->console_handler_mutex);
				print_command_history(dft->server->command);
				pthread_mutex_unlock(&dft->thread_d->console_handler_mutex);
			}
			else if (strcmp(operation, "4") == 0)
			{
				pthread_mutex_lock(&dft->thread_d->mutex_client_check_inner);
				all_online_ips_print(dft->server, dft->thread_d);
				pthread_mutex_unlock(&dft->thread_d->mutex_client_check_inner);
			}
			else if (strcmp(operation, "5") == 0)
			{
				time_t time_now = time(NULL);
				time_t alive_time = time_now - dft->server->start_time;

				int days = alive_time / 86400;
				int hours = (alive_time % 86400) / 3600;
				int minutes = (alive_time % 3600) / 60;
				int seconds = alive_time % 60;

				printf("Server uptime: %d days, %02d:%02d:%02d\n", days, hours, minutes, seconds);
			}
			else if (strcmp(operation, "clear") == 0)
			{
				system("clear");
			}
			else if (strcmp(operation, "exit") == 0)
			{
				printf(ANSI_COLOR_RED "Stopping server...\n" ANSI_COLOR_RESET);
				all_clear(dft->server, dft->lc, dft->thread_d);
				exit(EXIT_SUCCESS);
				break;
			}
			else
			{
				printf(ANSI_COLOR_RED "Unknown command: %s\n" ANSI_COLOR_RESET, operation);
			}

			printf(ANSI_COLOR_YELLOW "ADMIN > " ANSI_COLOR_RESET);
			fflush(stdout);
		}
		else if (c == 127 || c == 8)
		{
			if (index > 0) {
				index--;
				printf("\b \b");
				fflush(stdout);
			}
		}
		else
		{
			if (index < sizeof(operation) - 1 && isprint(c)) {
				operation[index++] = c;
				putchar(c);
				fflush(stdout);
			}
		}
	}

	reset_terminal_mode(&orig_termios);
	return NULL;
}
