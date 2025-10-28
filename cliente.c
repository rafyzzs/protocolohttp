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

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s http://host:port/caminho/arquivo\n", argv[0]);
        exit(1);
    }

    char *host, *path, *filename;
    int port, sockfd;

    // 1. Analisar a URL
    parse_url(argv[1], &host, &port, &path);
    printf("Host: %s, Porta: %d, Caminho: %s\n", host, port, path);

    // 2. Resolver o Host
    struct addrinfo *addr = resolve_host(host, port);

    // 3. Conectar ao Servidor
    sockfd = connect_to_server(addr);
    printf("Conectado ao servidor.\n");

    // 4. Montar e Enviar a Requisição HTTP
    char request_buffer[BUF_SIZE];
    sprintf(request_buffer, "GET %s HTTP/1.1\r\n"
                            "Host: %s\r\n"
                            "Connection: close\r\n"
                            "\r\n", 
                            path, host);
    
    if (send(sockfd, request_buffer, strlen(request_buffer), 0) < 0) {
        error_exit("Erro ao enviar requisição");
    }

    // 5. Receber a Resposta e Salvar o Arquivo
    char response_buffer[BUF_SIZE];
    ssize_t bytes_received;

    // Determina o nome do arquivo de saída
    filename = get_filename_from_path(path);
    FILE *file = fopen(filename, "wb"); // "wb" -> Write Binary
    if (file == NULL) {
        error_exit("Erro ao criar arquivo de saída");
    }

    printf("Salvando em: %s\n", filename);

    // Flag para saber se já encontramos o fim dos cabeçalhos
    int headers_ended = 0;
    char *body_start = NULL;

    while ((bytes_received = recv(sockfd, response_buffer, BUF_SIZE - 1, 0)) > 0) {
        response_buffer[bytes_received] = '\0'; // Termina a string

        if (!headers_ended) {
            // Verifica o status (simplificado)
            if (strstr(response_buffer, "HTTP/1.1 200 OK") == NULL) {
                if (strstr(response_buffer, "HTTP/1.1 404 Not Found") != NULL) {
                    fprintf(stderr, "Erro: Servidor retornou 404 Not Found\n");
                } else {
                    fprintf(stderr, "Erro: Resposta do servidor não foi 200 OK\n");
                    // Imprime os primeiros 100 char da resposta para debug
                    fprintf(stderr, "%.100s...\n", response_buffer);
                }
                fclose(file);
                remove(filename); // Remove o arquivo vazio
                exit(1);
            }

            // Encontra o fim dos cabeçalhos ("\r\n\r\n")
            body_start = strstr(response_buffer, "\r\n\r\n");
            
            if (body_start) {
                headers_ended = 1;
                body_start += 4; // Pula os 4 caracteres de "\r\n\r\n"
                
                // Calcula o tamanho do corpo *neste primeiro buffer*
                size_t body_chunk_size = bytes_received - (body_start - response_buffer);
                fwrite(body_start, 1, body_chunk_size, file);
            }
            // Se não encontrou, assume que os headers são maiores que o buffer
            // (para este projeto, assumimos que cabeçalhos cabem em 1 buffer)
        } else {
            // Já pulamos os cabeçalhos, apenas escreve os dados recebidos
            fwrite(response_buffer, 1, bytes_received, file);
        }
    }

    if (bytes_received < 0) {
        perror("Erro ao receber dados");
    }

    // 6. Limpeza
    fclose(file);
    close(sockfd);
    free(host);
    free(path);
    free(filename);
    freeaddrinfo(addr);

    printf("Download concluído.\n");
    return 0;
}