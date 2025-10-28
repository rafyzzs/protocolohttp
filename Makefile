# Compilador
CC = gcc
# Flags de compilação: -Wall (todos os warnings) é seu amigo!
CFLAGS = -Wall -g

# Alvos (o que queremos construir)
all: meu_navegador meu_servidor

# Como construir o cliente
meu_navegador: cliente.c
	$(CC) $(CFLAGS) -o meu_navegador cliente.c

# Como construir o servidor
meu_servidor: servidor.c
	$(CC) $(CFLAGS) -o meu_servidor servidor.c

# Comando 'make clean' para limpar os arquivos compilados
clean:
	rm -f meu_navegador meu_servidor