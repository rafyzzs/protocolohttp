protocolohttp
Projeto de um Cliente HTTP (meu_navegador) e um Servidor HTTP (meu_servidor) desenvolvido para a disciplina de Redes de Computadores, ministrada pelo Professor Flavio Luiz Schiavoni.

📝 Descrição
O projeto foi dividido em duas partes principais:

meu_servidor: Um servidor web que serve arquivos de um diretório local.

meu_navegador: Um cliente HTTP (navegador de modo texto) que baixa arquivos de uma URL e os salva localmente.

Funcionalidades do Servidor

index.html Padrão: Se uma requisição é feita para um diretório que contém um arquivo index.html, esse arquivo é retornado automaticamente.

Listagem de Diretório: Caso o diretório não possua um index.html, o servidor gera e retorna dinamicamente uma página HTML que lista todos os arquivos e subpastas daquele diretório.

⚙️ Como Compilar
Este projeto utiliza make para simplificar a compilação.

Clone o repositório:

Bash

git clone [URL-DO-SEU-REPOSITORIO]
cd [NOME-DO-SEU-REPOSITORIO]
Execute o make na pasta raiz:

Bash

make
Isso irá gerar dois executáveis: meu_servidor e meu_navegador.

Para limpar os arquivos compilados, execute:

Bash

make clean
🏁 Como Usar
São necessários dois terminais para a execução do cliente e do servidor.

1. Iniciar o Servidor (Terminal 1)
Primeiro, crie um diretório para o servidor servir (ex: meusite) e popule-o com alguns arquivos.

Bash

# Crie um diretório para servir
mkdir meusite
echo "<h1>Página Principal</h1>" > meusite/index.html
mkdir meusite/docs
echo "Página de documentos" > meusite/docs/documento.txt
Execute o servidor, passando o diretório raiz como argumento:

Bash

./meu_servidor meusite
O servidor estará rodando em http://localhost:5050.

2. Usar o Cliente (Terminal 2)
Agora, é possível usar o meu_navegador para requisitar arquivos do seu servidor por meio do outro terminal:

Bash

# 1. Baixar o index.html (da raiz)
./meu_navegador http://localhost:5050/

# 2. Baixar a listagem do diretório 'docs'
./meu_navegador http://localhost:5050/docs/

# 3. Baixar um arquivo específico
./meu_navegador http://localhost:5050/docs/documento.txt
Você também pode testar o servidor acessando http://localhost:5050 no seu navegador web.