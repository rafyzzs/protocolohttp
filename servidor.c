#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/stat.h>   
#include <dirent.h>     
#include <errno.h>      

#define PORT 5050       // Definindo a porta
#define BUF_SIZE 8192   // 8KB
#define MAX_PATH 4096

char *base_dir; // Diretório raiz que o servidor irá servir

void error_exit(const char *msg) {
    perror(msg);
    exit(1);
}

const char* get_mime_type(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return "application/octet-stream";
    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".jpg") == 0) return "image/jpeg";
    if (strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".pdf") == 0) return "application/pdf";
    if (strcmp(ext, ".txt") == 0) return "text/plain";
    return "application/octet-stream";
}

void send_header(int client_fd, const char *status, const char *content_type, long content_length) {
    char header_buffer[1024];
    sprintf(header_buffer, "HTTP/1.1 %s\r\n"
                           "Content-Type: %s\r\n"
                           "Content-Length: %ld\r\n"
                           "Connection: close\r\n"
                           "\r\n",
                           status, content_type, content_length);
    send(client_fd, header_buffer, strlen(header_buffer), 0);
}

void send_error(int client_fd, const char *status, const char *message) {
    char body[512];
    sprintf(body, "<html><body><h1>%s</h1><p>%s</p></body></html>", status, message);
    send_header(client_fd, status, "text/html", strlen(body));
    send(client_fd, body, strlen(body), 0);
}

void send_file(int client_fd, const char *filepath) {
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        send_error(client_fd, "404 Not Found", "Arquivo não encontrado.");
        return;
    }

    // Pega o tamanho do arquivo
    struct stat s;
    if (stat(filepath, &s) != 0) {
        send_error(client_fd, "500 Internal Server Error", "Não foi possível ler o arquivo.");
        fclose(file);
        return;
    }
    long file_size = s.st_size;
    const char *mime_type = get_mime_type(filepath);

    // Envia o cabeçalho 200 OK
    send_header(client_fd, "200 OK", mime_type, file_size);

    // Envia o corpo (arquivo) em pedaços (chunks)
    char buffer[BUF_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUF_SIZE, file)) > 0) {
        if (send(client_fd, buffer, bytes_read, 0) < 0) {
            // Cliente desconectou
            break;
        }
    }

    fclose(file);
}