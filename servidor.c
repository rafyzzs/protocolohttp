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

char *base_dir; 

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

    struct stat s;
    if (stat(filepath, &s) != 0) {
        send_error(client_fd, "500 Internal Server Error", "Não foi possível ler o arquivo.");
        fclose(file);
        return;
    }
    long file_size = s.st_size;
    const char *mime_type = get_mime_type(filepath);

    send_header(client_fd, "200 OK", mime_type, file_size);

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

void send_dir_listing(int client_fd, const char *dir_path, const char *http_path) {
    DIR *d = opendir(dir_path);
    if (!d) {
        send_error(client_fd, "500 Internal Server Error", "Não foi possível ler o diretório.");
        return;
    }

    char body[BUF_SIZE * 4]; // 32KB
    char entry_path[MAX_PATH];
    
    // HTML
    sprintf(body, "<html><head><title>Index of %s</title></head>"
                  "<body><h1>Index of %s</h1><ul>",
                  http_path, http_path);

    struct dirent *dir_entry;
    while ((dir_entry = readdir(d)) != NULL) {
        if (strcmp(dir_entry->d_name, ".") == 0) continue; 

        char link[MAX_PATH];
        if (strcmp(http_path, "/") == 0) {
            sprintf(link, "%s", dir_entry->d_name);
        } else {
            sprintf(link, "%s/%s", http_path, dir_entry->d_name);
        }

        sprintf(entry_path, "%s/%s", dir_path, dir_entry->d_name);
        struct stat s;
        char *display_name = strdup(dir_entry->d_name);
        if (stat(entry_path, &s) == 0 && S_ISDIR(s.st_mode)) {
            strcat(display_name, "/");
        }

        sprintf(body + strlen(body), "<li><a href=\"%s\">%s</a></li>",
                link, display_name);
        
        free(display_name);
    }
    closedir(d);

    // Fim do HTML
    strcat(body, "</ul></body></html>");

    // Envia a listagem
    send_header(client_fd, "200 OK", "text/html", strlen(body));
    send(client_fd, body, strlen(body), 0);
}

void handle_connection(int client_fd) {
    char buffer[BUF_SIZE];
    
    // Recebe a requisição (simplificado, assume que cabe em 1 buffer)
    ssize_t bytes_received = recv(client_fd, buffer, BUF_SIZE - 1, 0);
    if (bytes_received <= 0) {
        return; // Erro ou cliente desconectou
    }
    buffer[bytes_received] = '\0';

    // Analisa a requisição (simplificado)
    char method[16], http_path[MAX_PATH];
    if (sscanf(buffer, "%s %s", method, http_path) < 2) {
        send_error(client_fd, "400 Bad Request", "Requisição mal formatada.");
        return;
    }

    // Só aceitamos GET
    if (strcmp(method, "GET") != 0) {
        send_error(client_fd, "501 Not Implemented", "Método não implementado.");
        return;
    }

    // Segurança básica: impede "directory traversal"
    if (strstr(http_path, "..")) {
        send_error(client_fd, "403 Forbidden", "Acesso proibido.");
        return;
    }
    
    // Monta o caminho completo no sistema de arquivos
    char full_path[MAX_PATH];
    sprintf(full_path, "%s%s", base_dir, http_path);

    // Analisa o que é o caminho (arquivo, diretório ou não existe)
    struct stat s;
    if (stat(full_path, &s) != 0) {
        // Não existe
        send_error(client_fd, "404 Not Found", "Arquivo ou diretório não encontrado.");
    } else if (S_ISDIR(s.st_mode)) {
        // É um diretório
        
        // Verifica se /index.html existe dentro dele
        char index_path[MAX_PATH];
        sprintf(index_path, "%s/index.html", full_path);
        
        if (stat(index_path, &s) == 0 && S_ISREG(s.st_mode)) {
            // index.html existe, envia ele
            send_file(client_fd, index_path);
        } else {
            // index.html não existe, envia listagem do diretório
            send_dir_listing(client_fd, full_path, http_path);
        }
    } else if (S_ISREG(s.st_mode)) {
        // É um arquivo regular, envia
        send_file(client_fd, full_path);
    } else {
        // Outra coisa (link simbólico, socket, etc.)
        send_error(client_fd, "403 Forbidden", "Tipo de arquivo não suportado.");
    }
}