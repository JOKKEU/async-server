#ifndef __SERVER__
#define __SERVER__


#define _GNU_SOURCE

#include "gen.h"

#include <pthread.h>
#include <sched.h>

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/epoll.h>

#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h> 

#include <linux/in.h>
#include <netinet/ip_icmp.h>

#include <arpa/inet.h>
#include <asm-generic/param.h>

#define true 				1

#define MAX_PACKET_SIZE			1024
#define ICMP_PACKET_SIZE 		512
#define LOG(...) 			fprintf(stdout, "" __VA_ARGS__)


#define CLIENTS			 	5


#ifndef PORT

	#define PORT 			8080
	
#endif // PORT



#define _CLEAR 			\
				\
	if (server)		\
	{			\
		free(server); 	\
	}			\


// --- ЦВЕТА ДЛЯ КОНСОЛИ ---
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"


#ifndef PING_BUFFER_SIZE
	#define PING_BUFFER_SIZE 	64
#endif // PING_BUFFER_SIZE


#ifndef BUFFER_ACCEPT_SIZE

	#define BUFFER_ACCEPT_SIZE 4096

#endif // BUFFER_ACCEPT_SIZE

#define SIZE_TIME_BUFFER 		64

#define MAX_EVENTS			64
#define TIME_FORMAT 			"%Y-%m-%d %H:%M:%S"

struct _flags
{
	int 			stop_thread;
	int			stop_ot;
};


struct local_check
{
	struct icmp* 		lc_packet;
	struct sockaddr_in* 	lc_addr;
	
	struct timeval	 	timeout;
	
	
	char*			start_ip;
	int 			lc_sockfd;
	char			buffer[PING_BUFFER_SIZE];
	
	
	void			(*cleanup_lc)(struct local_check* lc);
};


struct socket_param
{
	int 			domain;
	int 			type;
	int 			protocol;
	
	
	int 			backlog;
	int 			opt;
	
};

struct client_info
{
	char			client_ip[INET_ADDRSTRLEN];
	int 			client_fd;
	char 			last_activity[SIZE_TIME_BUFFER];
	bool			dead;
};

struct active_ip
{
	char			ip[INET_ADDRSTRLEN];
	time_t			last_active;
};

struct command
{
	size_t count_cmd;
	size_t history_size;
	char** history_cmd;
};



struct _server
{
	char			my_ip[INET_ADDRSTRLEN];
	char			interface[IFNAMSIZ];
	char*			buffer_for_accept;

	size_t 			all_online_ips;
	size_t 			max_online_ips;
	struct active_ip**	all_active_ips;

	size_t 			max_my_online_client;
	size_t 			my_online_client;
	struct client_info** 	my_online_client_ips;

	size_t 			max_my_client;
	size_t 			my_client_count;
	struct client_info** 	my_client_ips;


	time_t			start_time;
	size_t 			start_device_count;
	unsigned short		port;

	int 			icmp_raw_sock;

	int              	epoll_fd;
	int             	tcp_listen_fd;
	int              	udp_sock_fd;


	struct socket_param* 	sock_param;
	struct _flags*		flags;

	struct command*		command;
	
	void (*cleanup_server)(struct _server*);
};



struct thread_data
{
	
	pthread_t		console_handler;
	pthread_mutex_t		console_handler_mutex;
	void*			(*console_func_handler)(void*);
	
	pthread_mutex_t		lp_mutex;
	
	pthread_t 		thread_client_check;
	pthread_mutex_t 	mutex_client_check;
	pthread_mutex_t 	mutex_client_check_inner;
	void*			(*client_handler_func)(void*);


	pthread_t 		client_command;
	void*			(*client_command_handler)(void*);


	pthread_t 		broadcast;
	void*			(*broadcast_handler)(void*);

};	

struct data_for_thread
{
		
	struct _server* 	server;
	struct local_check* 	lc;	
	struct thread_data* 	thread_d;
};



extern int   server(void);
extern void  get_start_ip_for_local_check(struct local_check* lc, struct _server* server);

extern void  start_init_server(struct _server* server);
extern void  init_local_check(struct local_check* lc);
extern void send_local_net_ping(struct _server* server, struct local_check* lc, struct thread_data* thread_d, char* ip_addr);

extern void all_clear(struct _server* server, struct local_check* lc, struct thread_data* thread_d);
extern void* operation_clients(void* data);
extern void* console_handler_server(void* data);

extern void set_nonblocking_mode(int fd);
extern void reset_terminal_mode(struct termios* orig_termios);

extern void* client_command_handler(void* data);
extern void* broadcast_handler(void*);
extern void format_duration(time_t seconds, char* buff, size_t size);
extern void format_timestamp(time_t t, char* buff, size_t size);

// callback epoll
extern void handle_new_tcp_connection(struct _server* server, struct thread_data* thread_d);
extern void handle_icmp_reply_async(struct _server* server, struct thread_data* thread_d);

extern int handle_tcp_data(struct _server* server, struct thread_data* thread_d, int fd);
extern void  handle_tcp_disconnection(struct _server* server, struct thread_data* thread_d, int fd);
extern void handle_udp_data_async(struct _server* server, struct thread_data* thread_d);
extern unsigned short check_sum(void* b, int len);
extern time_t parse_time_string(const char* time_str);
extern void get_curr_time(char* cl_time);

// storage ops
extern void all_online_ips_print(struct _server* server, struct thread_data* thread_d);
extern void my_clients_print(struct _server* server, struct thread_data* thread_d);
extern int check_ip_in_arr(struct _server* server, char* ip_addr);
extern void remove_active_ips(struct _server* server, struct thread_data* thread_d, char* ip_addr);
extern void remove_my_client_ips(struct _server* server, struct thread_data* thread_d, char* ip_addr);
extern void remove_my_online_client_ips(struct _server* server, struct thread_data* thread_d, char* ip_addr);
extern void my_online_clients_print(struct _server* server, struct thread_data* thread_d);
extern void add_active_ips(struct _server* server, struct thread_data* thread_d, char* ip_addr);
extern struct client_info* find_client_by_fd(struct _server* server, int fd);
extern void remove_client_by_fd(struct _server* server, struct thread_data* thread_d, int fd);
extern void add_client_in_my_online_client_ips(struct _server* server, struct thread_data* thread_d, char* ip_addr, int client_fd);
extern void add_client_in_my_client_ips(struct _server* server, struct thread_data* thread_d, char* ip_addr, int client_fd);
extern void all_client_ips_realloc(struct _server* server, struct thread_data* thread_d);
extern void my_online_client_ips_realloc(struct _server* server, struct thread_data* thread_d);
extern void my_client_ips_realloc(struct _server* server, struct thread_data* thread_d);

// histoty ops
extern void add_command_to_history(struct _server* server, struct thread_data* thread_d, char* buffer, int client_fd);
extern void print_command_history(struct command* command);


// dump
extern void dump_before_shutdown(struct _server* server);



#endif // __SERVER__












