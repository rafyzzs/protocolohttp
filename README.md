# protocolohttp
Criação de um Cliente HTTP e Servidor HTTP para a matéria de Redes de Computadores, ministrada pelo Professor Flavio Luiz Schiavoni.

O projeto foi dividido em duas partes: meu_servidor, que serve arquivos de um diretório local e meu_navegador, que baixa arquivos de um site e salva eles localmente.

O arquivo padrão é o index.html. Dessa forma, se uma requisição é feita para um diretório que contem o index.html, ele é retornado automaticamente. Caso contrário, o servidor retorna uma página listando todos os arquivos e subpastas daquele diretório.

Como compilar

1. Clone o repositório: 
git clone [URL-DO-SEU-REPOSITORIO]
cd [NOME-DO-SEU-REPOSITORIO]

2. Execute o make na pasta raiz:
make

3. Isso vai gerar dois executáveis, o meu_servidor e o meu_navegador

4. Para limpar os arquivos compilados, execute: make clean

Como usar

São necessários dois terminais para a execução do cliente e do servidor

1. Crie um diretório para o servidor servir

2. Execute o servidor, passando o diretório raiz como argumento:
./meu_servidor meusite

3. Agora, é possível usar o meu_navegador para requisitar arquivos do seu servidor por meio do outro terminal:
./meu_navegador http://localhost:5050/

./meu_navegador http://localhost:5050/docs/