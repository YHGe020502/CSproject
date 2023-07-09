#include "rpc.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#define NONBLOCKING

#define OPERATION_FIND 0
#define OPERATION_CALL 1
#define OPERATION_CLOSE 2

typedef struct rpc_function {
    int handle_id;
    char name[1010];
    rpc_handler handler;
    struct rpc_function *next;
} rpc_function;

struct rpc_server {
    /* Add variable(s) for server state */
    struct sockaddr_in6 server_addr;
    int server_fd;
    int socket_fd;
    rpc_function *handle_head;
    unsigned int handle_num;
};

rpc_server *rpc_init_server(int port) {
    rpc_server * server = malloc(sizeof(rpc_server));
    memset(server, 0, sizeof(rpc_server));
    // create ipv6 tcp socket
    server->server_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (server->server_fd < 0) {
        return NULL;
    }
    // set non-block
    fcntl(server->server_fd, F_SETFL, O_NONBLOCK);
    // set options
    int opt = 1;
    if (setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) < 0) {
        return NULL;
    }
    // bind socket
    memset(&server->server_addr, 0, sizeof(server->server_addr));
    server->server_addr.sin6_family = AF_INET6;
    server->server_addr.sin6_addr = in6addr_any;
    server->server_addr.sin6_port = htons(port);
    if (bind(server->server_fd, (struct sockaddr *)&server->server_addr, sizeof(server->server_addr)) < 0) {
        return NULL;
    }
    // listen
    if (listen(server->server_fd, 1024) < 0) {
        return NULL;
    }
    return server;
}

int rpc_register(rpc_server *srv, char *name, rpc_handler handler) {
    if (srv == NULL || name == NULL || handler == NULL) {
        return -1;
    }
    if (strlen(name) > 1000 || strlen(name) == 0) {
        return -1;
    }
    if (srv->handle_head == NULL) {
        // malloc and init, insert at head
        srv->handle_head = malloc(sizeof(rpc_function));
        memset(srv->handle_head, 0, sizeof(rpc_function));
        strcpy(srv->handle_head->name, name);
        srv->handle_head->handler = handler;
        srv->handle_head->handle_id = srv->handle_num;
        srv->handle_head->next = NULL;
        srv->handle_num++;
        return srv->handle_head->handle_id;
    } else {
        rpc_function *handle = srv->handle_head;
        while (handle->next != NULL) {
            if (strcmp(handle->name, name) == 0) {
                // already exist, update handler
                handle->handler = handler;
                return handle->handle_id;
            }
            handle = handle->next;
        }
        if (strcmp(handle->name, name) == 0) {
            // already exist, update handler
            handle->handler = handler;
            return handle->handle_id;
        }
        // malloc and init, insert at tail
        handle->next = malloc(sizeof(struct rpc_function));
        memset(handle->next, 0, sizeof(struct rpc_function));
        strcpy(handle->next->name, name);
        handle->next->handler = handler;
        handle->next->handle_id = srv->handle_num;
        handle->next->next = NULL;
        srv->handle_num++;
        return handle->next->handle_id;
    }
}

void run_server(rpc_server *srv) {
    while (1) {
        // first read lenth of data
        unsigned int length;
        read(srv->socket_fd, &length, sizeof(unsigned int));
        length = ntohl(length);
        // then read operation
        unsigned int operation;
        read(srv->socket_fd, &operation, sizeof(unsigned int));
        operation = ntohl(operation);
        if (operation == OPERATION_FIND) {
            // read name
            char name[1010];
            memset(name, 0, sizeof(name));
            read(srv->socket_fd, name, length);
            // find handle
            int find = 0;
            rpc_function *handle = srv->handle_head;
            while (handle != NULL) {
                if (strcmp(handle->name, name) == 0) {
                    find = 1;
                    break;
                }
                handle = handle->next;
            }
            // write handle id
            int handle_id = -1;
            if (find == 1) {
                handle_id = handle->handle_id;
            }
            handle_id = htonl(handle_id);
            write(srv->socket_fd, &handle_id, sizeof(int));
        } else if (operation == OPERATION_CALL) {
            // read handle id
            int handle_id;
            read(srv->socket_fd, &handle_id, sizeof(int));
            handle_id = ntohl(handle_id);
            // find handle
            rpc_function *handle = srv->handle_head;
            while (handle != NULL) {
                if (handle->handle_id == handle_id) {
                    break;
                }
                handle = handle->next;
            }
            // read data1
            uint32_t data1_high;
            uint32_t data1_low;
            read(srv->socket_fd, &data1_high, sizeof(uint32_t));
            read(srv->socket_fd, &data1_low, sizeof(uint32_t));
            data1_high = ntohl(data1_high);
            data1_low = ntohl(data1_low);
            unsigned long long data1 = data1_high;
            data1 = data1 << 32;
            data1 = data1 | data1_low;
            // read data2
            char *data2;
            if (length > 0) {
                data2 = malloc(length);
                read(srv->socket_fd, data2, length);
            } else {
                data2 = NULL;
            }
            // malloc and init a rpc data
            rpc_data *data = malloc(sizeof(rpc_data));
            data->data1 = data1;
            data->data2_len = length;
            data->data2 = data2;
            // call handler
            rpc_data *result = handle->handler(data);
            if (data2 != NULL) {
                free(data2);
            }
            free(data);
            if (result == NULL) {
                unsigned int data2_len = -1;
                data2_len = htonl(data2_len);
                write(srv->socket_fd, &data2_len, sizeof(unsigned int));
            } else if ((result->data2_len == 0 && result->data2 != NULL) || (result->data2_len > 0 && result->data2 == NULL)) {
                // send a wrong len to client
                result->data2_len = (unsigned int) -1;
                unsigned int data2_len = htonl(result->data2_len);
                write(srv->socket_fd, &data2_len, sizeof(unsigned int));
                if (result->data2 != NULL) {
                    free(result->data2);
                }
                free(result);
            } else {
                // write data2_len
                unsigned int data2_len = htonl(result->data2_len);
                write(srv->socket_fd, &data2_len, sizeof(unsigned int));
                // write data1, data1 is 64 bit, need to write 2 times
                data1 = result->data1;
                data1_high = data1 >> 32;
                data1_low = data1 & 0xffffffff;
                data1_high = htonl(data1_high);
                data1_low = htonl(data1_low);
                write(srv->socket_fd, &data1_high, sizeof(uint32_t));
                write(srv->socket_fd, &data1_low, sizeof(uint32_t));
                if (result->data2_len > 0) {
                    // write data2
                    write(srv->socket_fd, result->data2, result->data2_len);
                    free(result->data2);
                }
                free(result);
            }
        } else if (operation == OPERATION_CLOSE) {
            break;
        }
    }
}

int server_killed = 0;

void handler() {
    server_killed = 1;
}

void rpc_serve_all(rpc_server *srv) {
    server_killed = 0;
    int addrlen = sizeof(srv->server_addr);
    signal(SIGINT, handler);
    while (server_killed == 0) {
        if((srv->socket_fd = accept(srv->server_fd, (struct sockaddr *)&srv->server_addr, (socklen_t *)&addrlen)) < 0) {
            continue;
        }
        pid_t pid = fork();
        if (pid == 0) {
            // child process
            close(srv->server_fd);
            run_server(srv);
            while (srv->handle_head != NULL) {
                rpc_function *handle = srv->handle_head;
                srv->handle_head = srv->handle_head->next;
                free(handle);
            }
            close(srv->socket_fd);
            free(srv);
            exit(0);
        } else {
            // parent process
            close(srv->socket_fd);
        }
    }
    close(srv->server_fd);
    while (srv->handle_head != NULL) {
        rpc_function *handle = srv->handle_head;
        srv->handle_head = srv->handle_head->next;
        free(handle);
    }
    free(srv);
}

struct rpc_client {
    /* Add variable(s) for client state */
    int socket_fd;
};

struct rpc_handle {
    /* Add variable(s) for handle */
    int handle_id;
};

rpc_client *rpc_init_client(char *addr, int port) {
    rpc_client *client = malloc(sizeof(rpc_client));
    memset(client, 0, sizeof(rpc_client));
    struct sockaddr_in6 server_addr;
    // create ipv6 tcp socket
    client->socket_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (client->socket_fd < 0) {
        return NULL;
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(port);
    // convert ipv6 address
    if (inet_pton(AF_INET6, addr, &server_addr.sin6_addr) <= 0) {
        return NULL;
    }
    // connect
    if (connect(client->socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        return NULL;
    }
    return client;
}

rpc_handle *rpc_find(rpc_client *cl, char *name) {
    if (cl == NULL) {
        return NULL;
    }
    // write length
    unsigned int length = strlen(name);
    length = htonl(length);
    write(cl->socket_fd, &length, sizeof(unsigned int));
    // write operation = OPERATION_FIND
    unsigned int operation = OPERATION_FIND;
    operation = htonl(operation);
    write(cl->socket_fd, &operation, sizeof(unsigned int));
    // write name
    write(cl->socket_fd, name, strlen(name));
    // read handle id
    int handle_id;
    read(cl->socket_fd, &handle_id, sizeof(int));
    handle_id = ntohl(handle_id);
    if (handle_id == -1) {
        return NULL;
    }
    rpc_handle *handle = malloc(sizeof(rpc_handle));
    handle->handle_id = handle_id;
    return handle;
}

rpc_data *rpc_call(rpc_client *cl, rpc_handle *h, rpc_data *payload) {
    if (cl == NULL || h == NULL || payload == NULL) {
        return NULL;
    }
    // check valid data2
    if (payload->data2_len == 0 && payload->data2 != NULL) {
        return NULL;
    }
    if (payload->data2_len > 0 && payload->data2 == NULL) {
        return NULL;
    }
    // write lenth
    unsigned int length = payload->data2_len;
    length = htonl(length);
    write(cl->socket_fd, &length, sizeof(unsigned int));
    // write operation = OPERATION_CALL
    unsigned int operation = OPERATION_CALL;
    operation = htonl(operation);
    write(cl->socket_fd, &operation, sizeof(unsigned int));
    // write handle id
    unsigned int handle_id = htonl(h->handle_id);
    write(cl->socket_fd, &handle_id, sizeof(unsigned int));
    // write data1
    unsigned long long data1 = payload->data1;
    uint32_t data1_high = data1 >> 32;
    uint32_t data1_low = data1 & 0xffffffff;
    data1_high = htonl(data1_high);
    data1_low = htonl(data1_low);
    write(cl->socket_fd, &data1_high, sizeof(uint32_t));
    write(cl->socket_fd, &data1_low, sizeof(uint32_t));
    // write data2
    if (payload->data2_len > 0) {
        write(cl->socket_fd, payload->data2, payload->data2_len);
    }
    rpc_data *data = malloc(sizeof(rpc_data));
    // read lenth
    unsigned int data2_len;
    read(cl->socket_fd, &data2_len, sizeof(unsigned int));
    data2_len = ntohl(data2_len);
    if (data2_len == (unsigned int) -1) {
        return NULL;
    }
    data->data2_len = data2_len;
    // read data1, data1 is 64 bits
    read(cl->socket_fd, &data1_high, sizeof(uint32_t));
    read(cl->socket_fd, &data1_low, sizeof(uint32_t));
    data1_high = ntohl(data1_high);
    data1_low = ntohl(data1_low);
    data1 = data1_high;
    data1 = data1 << 32;
    data1 = data1 | data1_low;
    data->data1 = data1;
    data->data2 = NULL;
    // read data2
    if (data2_len > 0) {
        char *data2 = malloc(data2_len);
        read(cl->socket_fd, data2, data2_len);
        data->data2 = data2;
    }
    // check valid data2
    if (data2_len == 0 && data->data2 != NULL) {
        return NULL;
    }
    if (data2_len > 0 && data->data2 == NULL) {
        return NULL;
    }
    return data;
}

void rpc_close_client(rpc_client *cl) {
    if (cl == NULL) {
        return;
    }
    // write length = 0
    int length = 0;
    length = htonl(length);
    write(cl->socket_fd, &length, sizeof(int));
    // write operation = OPERATION_CLOSE
    int operation = OPERATION_CLOSE;
    operation = htonl(operation);
    write(cl->socket_fd, &operation, sizeof(int));
    close(cl->socket_fd);
    cl->socket_fd = -1;
    free(cl);
}

void rpc_data_free(rpc_data *data) {
    if (data == NULL) {
        return;
    }
    if (data->data2 != NULL) {
        free(data->data2);
    }
    free(data);
}
