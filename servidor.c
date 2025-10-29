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
        send_error(client_fd, "404 Not Found", "Arquivo nao encontrado.");
        return;
    }

    struct stat s;
    if (stat(filepath, &s) != 0) {
        send_error(client_fd, "500 Internal Server Error", "Nao foi possível ler o arquivo.");
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
    
    strcpy(body, "<html><head>");
    
    sprintf(body + strlen(body), "<title>Index of %s</title>", http_path);

    // Bloco de Estilo (CSS) Embutido
    strcat(body, "<style>\n"
                 "  body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; \n"
                 "         background-color: #f8f9fa; color: #212529; margin: 2em; }\n"
                 "  .container { max-width: 800px; margin: 0 auto; padding: 20px; background-color: #fff; \n"
                 "                 border-radius: 8px; box-shadow: 0 4px 12px rgba(0,0,0,0.05); }\n"
                 "  h1 { border-bottom: 2px solid #dee2e6; padding-bottom: 10px; color: #495057; }\n"
                 "  ul { list-style-type: none; padding: 0; }\n"
                 "  li { background-color: #fff; margin: 8px 0; border: 1px solid #e9ecef; border-radius: 5px; \n"
                 "         transition: background-color 0.2s, box-shadow 0.2s; }\n"
                 "  li a { display: block; padding: 12px 15px; text-decoration: none; color: #007bff; \n"
                 "         font-weight: 500; }\n"
                 "  li a:hover { color: #0056b3; }\n"
                 "  li:hover { background-color: #fdfdfe; box-shadow: 0 2px 5px rgba(0,0,0,0.08); }\n"
                 "  footer { margin-top: 20px; font-size: 0.9em; color: #868e96; text-align: center; }\n"
                 "</style>");

    sprintf(body + strlen(body), "</head><body><div class=\"container\"><h1>Index of %s</h1><ul>", http_path);


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

    strcat(body, "</ul><footer>Servidor HTTP em C</footer></div></body></html>");

    send_header(client_fd, "200 OK", "text/html", strlen(body));
    send(client_fd, body, strlen(body), 0);
}

void handle_connection(int client_fd) {
    char buffer[BUF_SIZE];
    
    ssize_t bytes_received = recv(client_fd, buffer, BUF_SIZE - 1, 0);
    if (bytes_received <= 0) {
        return; 
    }
    buffer[bytes_received] = '\0';

    char method[16], http_path[MAX_PATH];
    if (sscanf(buffer, "%s %s", method, http_path) < 2) {
        send_error(client_fd, "400 Bad Request", "Requisição mal formatada.");
        return;
    }

    if (strcmp(method, "GET") != 0) {
        send_error(client_fd, "501 Not Implemented", "Método não implementado.");
        return;
    }

    if (strstr(http_path, "..")) {
        send_error(client_fd, "403 Forbidden", "Acesso proibido.");
        return;
    }
    
    char full_path[MAX_PATH];
    sprintf(full_path, "%s%s", base_dir, http_path);

    struct stat s;
    if (stat(full_path, &s) != 0) {

        send_error(client_fd, "404 Not Found", "Arquivo ou diretório não encontrado.");
    } else if (S_ISDIR(s.st_mode)) {
        
        char index_path[MAX_PATH];
        sprintf(index_path, "%s/index.html", full_path);
        
        if (stat(index_path, &s) == 0 && S_ISREG(s.st_mode)) {
        
            send_file(client_fd, index_path);
        } else {
            
            send_dir_listing(client_fd, full_path, http_path);
        }
    } else if (S_ISREG(s.st_mode)) {
        
        send_file(client_fd, full_path);
    } else {
        
        send_error(client_fd, "403 Forbidden", "Tipo de arquivo não suportado.");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <diretorio_raiz>\n", argv[0]);
        exit(1);
    }
    base_dir = argv[1];

    int listen_fd, client_fd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t cli_len = sizeof(cli_addr);

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        error_exit("Erro ao criar socket");
    }

    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        error_exit("Erro ao configurar setsockopt");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY; 
    serv_addr.sin_port = htons(PORT);       

    if (bind(listen_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        error_exit("Erro no bind");
    }

    if (listen(listen_fd, 5) < 0) { 
        error_exit("Erro no listen");
    }

    printf("Servidor HTTP escutando na porta %d, servindo o diretório '%s'\n", PORT, base_dir);
    printf("Acesse: http://localhost:%d\n", PORT);

    while (1) {
        client_fd = accept(listen_fd, (struct sockaddr*)&cli_addr, &cli_len);
        if (client_fd < 0) {
            perror("Erro no accept");
            continue; 
        }
        
        printf("Cliente conectado.\n");
        
        handle_connection(client_fd);

        close(client_fd);
        printf("Cliente desconectado.\n");
    }

    close(listen_fd);
    return 0;
}