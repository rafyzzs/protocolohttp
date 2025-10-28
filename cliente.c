#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>  
#include <netdb.h>      
#include <errno.h>      
#include <libgen.h>    

#define BUF_SIZE 8192       // 8 KB

void error_exit(const char *msg) {
    perror(msg);
    exit(1);
}

void parse_url(char *url, char **host, int *port, char **path) {
    char *url_copy = strdup(url); 
    char *host_port_path;
    
    if (strncmp(url_copy, "http://", 7) == 0) {
        host_port_path = url_copy + 7;
    } else {
        host_port_path = url_copy;
    }

    char *path_start = strchr(host_port_path, '/');
    if (path_start) {
        *path = strdup(path_start); 
        *path_start = '\0'; 
    } else {
        *path = strdup("/"); 
    }

    char *port_start = strchr(host_port_path, ':');
    if (port_start) {
        *port_start = '\0'; 
        *host = strdup(host_port_path);
        *port = atoi(port_start + 1); 
    } else {
        *host = strdup(host_port_path);
        *port = 80; 
    }
    
    if (strcmp(*host, "localhost") == 0) {
        free(*host);
        *host = strdup("127.0.0.1");
    }

    free(url_copy);
}

struct addrinfo* resolve_host(const char *host, int port) {
    struct addrinfo hints, *res;
    char port_str[6]; 
    sprintf(port_str, "%d", port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       
    hints.ai_socktype = SOCK_STREAM; 

    int status;
    if ((status = getaddrinfo(host, port_str, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }

    return res;
}

int connect_to_server(struct addrinfo *addr) {
    int sockfd;
    
    // Tenta criar o socket
    sockfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if (sockfd < 0) {
        error_exit("Erro ao criar socket");
    }

    // Tenta conectar
    if (connect(sockfd, addr->ai_addr, addr->ai_addrlen) < 0) {
        error_exit("Erro ao conectar ao servidor");
    }

    return sockfd;
}

char* get_filename_from_path(const char *path) {
    if (strcmp(path, "/") == 0) {
        return strdup("index.html");
    }

    char *path_copy = strdup(path);
    char *filename = basename(path_copy);
    char *result = strdup(filename);
    free(path_copy);
    return result;
}