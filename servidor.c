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