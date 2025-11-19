#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>


#define PORT 8080
#define BUFFER_SIZE 256
#define COMMAND_BUFFER_SIZE 64
#define LOG(...) printf(__VA_ARGS__)


static void command_list(void);
static int enter_command(char* command, size_t max_len);
static void send_udp_ready(const char* server_ip, int port);


int main()
{
        char server_ip[INET_ADDRSTRLEN];
        int tcp_fd;
        struct sockaddr_in server_addr;


        LOG("Enter server IP address: ");
        if (fgets(server_ip, INET_ADDRSTRLEN, stdin) == NULL) {
                perror("Failed to read server IP");
                return EXIT_FAILURE;
        }

        server_ip[strcspn(server_ip, "\n")] = 0;
        if (strlen(server_ip) == 0) {
                LOG("No IP address entered. Exiting.\n");
                return EXIT_FAILURE;
        }


        send_udp_ready(server_ip, PORT);


        tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (tcp_fd < 0) {
                perror("Failed to create TCP socket");
                return EXIT_FAILURE;
        }


        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);

        if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
                LOG("Error: Invalid IP address (%s).\n", server_ip);
                close(tcp_fd);
                return EXIT_FAILURE;
        }


        if (connect(tcp_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                perror("Failed to connect to server via TCP");
                close(tcp_fd);
                return EXIT_FAILURE;
        }

        LOG("[TCP] Client successfully connected to server (%s:%d).\n", server_ip, PORT);


        char command_buffer[COMMAND_BUFFER_SIZE];
        char response_buffer[BUFFER_SIZE];
        char send_buffer[BUFFER_SIZE];

        while (1)
        {
                command_list();
                printf("> ");

                if (enter_command(command_buffer, sizeof(command_buffer)) != 0)
                {
                        LOG("Input error or EOF. Shutting down.\n");
                        break;
                }
                if (strlen(command_buffer) == 0) continue;


                snprintf(send_buffer, BUFFER_SIZE, "%s\n", command_buffer);
                if (send(tcp_fd, send_buffer, strlen(send_buffer), 0) < 0)
                {
                        perror("TCP send failed");
                        break;
                }


                ssize_t received_bytes = recv(tcp_fd, response_buffer, BUFFER_SIZE - 1, 0);
                if (received_bytes <= 0)
                {
                        if (received_bytes == 0) LOG("Server closed connection.\n");
                        else perror("TCP recv failed");
                        break;
                }

                response_buffer[received_bytes] = '\0';
                LOG("Server response: %s", response_buffer);

                if (strcmp(command_buffer, "/shutdown") == 0) break;
        }


        LOG("Shutting down client...\n");
        close(tcp_fd);
        return EXIT_SUCCESS;
}



static void send_udp_ready(const char* server_ip, int port)
{
        int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (udp_fd < 0)
        {
                LOG("[Warning] UDP socket creation for 'ready' failed.\n");
                return;
        }

        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);

        if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0)
        {
                LOG("[Warning] Invalid server IP for UDP.\n");
                close(udp_fd);
                return;
        }

        const char* msg = "ready";
        sendto(udp_fd, msg, strlen(msg), 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
        close(udp_fd);
        LOG("[UDP] 'ready' signal sent to %s:%d\n", server_ip, port);
}

static void command_list(void)
{
        LOG("\n--- Command List ---\n");
        LOG("/time\n");
        LOG("/stats\n");
        LOG("/shutdown\n");
        LOG("--------------------\n");
}

static int enter_command(char* command, size_t max_len)
{
        if (fgets(command, max_len, stdin) != NULL)
        {
                command[strcspn(command, "\n")] = 0;
                return 0;
        }
        return 1;
}
