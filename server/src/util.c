#include "../hd/server.h"

static char* handle_start_ip(char* start_ip)
{
	int count_dot = 3;	
	int count = 0;
		
	for (size_t index = 0; index < INET_ADDRSTRLEN; ++index)
	{
		if (count == count_dot) {start_ip[index] =  '\0'; break;}
		if (start_ip[index] == '.') {count++;}
	}
	
	return start_ip;
}



static int get_safe_line(char* dest, size_t dest_size)
{
	char temp_buffer[1024];
	if (fgets(temp_buffer, sizeof(temp_buffer), stdin) == NULL) {
		return 1;
	}

	size_t len = strlen(temp_buffer);
	if (len > 0 && temp_buffer[len - 1] != '\n')
	{
		int c;
		while ((c = getchar()) != '\n' && c != EOF);
	}

	while (len > 0 && (temp_buffer[len - 1] == '\n' || isspace(temp_buffer[len - 1])))
	{
		temp_buffer[len - 1] = '\0';
		len--;
	}

	char* start = temp_buffer;
	while (*start && isspace(*start))
	{
		start++;
	}

	if (strlen(start) == 0) return 0;

	snprintf(dest, dest_size, "%s", start);
	return 0;
}

void get_start_ip_for_local_check(struct local_check* lc, struct _server* server)
{
	while (true)
	{
		LOG("Enter network interface (e.g. wlp0s20f3): ");
		memset(server->interface, 0, IFNAMSIZ);

		if (get_safe_line(server->interface, IFNAMSIZ) != 0)
		{
			perror("Error reading input");
			exit(EXIT_FAILURE);
		}

		if (strlen(server->interface) == 0)
		{
			LOG("Empty input. Enter 'ip a' in console to see interfaces.\n");
			continue;
		}
		break;
	}

	while (true)
	{
		LOG("Determine the address range by network interface for scanning? [Y/N]: ");

		char choice_buf[16];
		if (get_safe_line(choice_buf, sizeof(choice_buf)) != 0)
		{
			exit(EXIT_FAILURE);
		}


		char choice = toupper(choice_buf[0]);

		if (choice == 'Y')
		{
			struct ifreq ifr_ip;
			struct ifreq ifr_mask;
			struct sockaddr_in lc_addr_for_ip;

			lc->lc_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
			if (lc->lc_sockfd < 0)
			{
				perror("failed create socket");
				exit(EXIT_FAILURE);
			}

			strncpy(ifr_ip.ifr_name, server->interface, IFNAMSIZ);
			strncpy(ifr_mask.ifr_name, server->interface, IFNAMSIZ);

			if (ioctl(lc->lc_sockfd, SIOCGIFADDR, &ifr_ip) < 0)
			{
				LOG("Error: Interface '%s' not found or no IP address.\n", server->interface);
				close(lc->lc_sockfd);
				continue;
			}

			lc_addr_for_ip = *(struct sockaddr_in*)&ifr_ip.ifr_addr;
			struct in_addr ip_addr;
			ip_addr.s_addr = lc_addr_for_ip.sin_addr.s_addr;

			if (ioctl(lc->lc_sockfd, SIOCGIFNETMASK, &ifr_mask) < 0)
			{
				perror("error ioctl for netmask");
				close(lc->lc_sockfd);
				exit(EXIT_FAILURE);
			}

			struct in_addr netmask;
			netmask.s_addr = ((struct sockaddr_in *)&ifr_mask.ifr_addr)->sin_addr.s_addr;

			struct in_addr network_addr;
			network_addr.s_addr = lc_addr_for_ip.sin_addr.s_addr & netmask.s_addr;

			strncpy(server->my_ip, inet_ntoa(ip_addr), sizeof(server->my_ip));
			strncpy(lc->start_ip, inet_ntoa(network_addr), INET_ADDRSTRLEN);

			handle_start_ip(lc->start_ip);

			close(lc->lc_sockfd);
			break;
		}
		else if (choice == 'N')
		{
			while (true)
			{
				LOG("Enter start ip address for scanning | Example: 192.168.1.0\n> ");

				if (get_safe_line(lc->start_ip, INET_ADDRSTRLEN) != 0)
				{
				exit(EXIT_FAILURE);
				}

				if (strlen(lc->start_ip) < 7)
				{
				LOG("Invalid IP length. Try again.\n");
				continue;
				}

				handle_start_ip(lc->start_ip);
				break;
			}
			break;
		}
		else
		{
			LOG("Error! Please enter Y or N\n");
			continue;
		}
	}

	LOG("range ip for scanning clients %s0-255\n", lc->start_ip);
}



time_t parse_time_string(const char* time_str)
{
    struct tm tm_struct;
    memset(&tm_struct, 0, sizeof(struct tm));
    if (strptime(time_str, TIME_FORMAT, &tm_struct) == NULL)
    {
        return (time_t)-1;
    }
    return mktime(&tm_struct);
}


void get_curr_time(char* cl_time)
{
	time_t raw_time;
	time(&raw_time);
	
	struct tm* timeinfo;
	timeinfo = localtime(&raw_time);

	strftime(cl_time, SIZE_TIME_BUFFER, "%Y-%m-%d %H:%M:%S", timeinfo);
}


unsigned short check_sum(void* b, int len)
{
	unsigned short *buffer 	= 	b;
    	unsigned int sum 	= 	0;
    	unsigned short result;

	for (sum = 0; len > 1; len -= 2)
	{
		sum += *buffer++;
	}

	if (len == 1)
	{
		sum += *(unsigned char*)buffer;
	}

	sum = (sum >> 16) + (sum + 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return result;

}



void set_nonblocking_mode(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
	{
		perror("fcntl get");
        	exit(EXIT_FAILURE);
	}
	
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void reset_terminal_mode(struct termios* orig_termios) 
{
    tcsetattr(STDIN_FILENO, TCSANOW, orig_termios);
}



void format_duration(time_t seconds, char* buff, size_t size)
{
	int hours = seconds / 3600;
	int minutes = (seconds % 3600) / 60;
	int secs = seconds % 60;
	int days = hours / 24;
	hours = hours % 24;

	if (days > 0)
		snprintf(buff, size, "%d days, %02d:%02d:%02d", days, hours, minutes, secs);
	else
		snprintf(buff, size, "%02d:%02d:%02d", hours, minutes, secs);
}


void format_timestamp(time_t t, char* buff, size_t size)
{
	struct tm* tm_info = localtime(&t);
	strftime(buff, size, "%Y-%m-%d %H:%M:%S", tm_info);
}















