#include <string>
#include <iostream>
#include <socket.h>

#define MAXLINE 100000

void str_cli(FILE *fp, int sockfd) {
    int maxfdp1, stdineof;
    fd_set rset;
    int n, counter = 0;
    char *buf = new char[MAX_LINE];
    char *recvline = new char[MAX_LINE];

    stdineof = 0;


    FD_ZERO(&rset); /* limpas os bits de rset */

    for (; ;) {
        if (stdineof == 0) {
            FD_SET(fileno(fp), &rset); /* seta o bit de rset referente a posição 'fileno(fp)' */
        }

        FD_SET(sockfd, &rset); /* seta o bit de rset referente a posição 'sockfd' */

        maxfdp1 = std::max(fileno(fp), sockfd) + 1;

        /* executa a função select enquanto ou o socket 'sockfd' ou o stdin não estiverem prontos para leitura */
        int nready = sock::Select(maxfdp1, &rset);

        if (nready > 0) {
            /* Verifica se o socket 'sockfd' está pronto para ser lido */
            if (FD_ISSET(sockfd, &rset)) {
                /* Lê o conteúdo do socket, e verifica se a conexão foi fechada */
                if ((n = sock::Read(sockfd, recvline, MAX_LINE)) == 0) {
                    /* verifica se a entrada foi toda lida. em caso positivo, 
                        sai do loop para que o programa seja finalizado. em caso 
                        negativo, houve algum erro no servidor, uma vez que o mesmo 
                        fechou a conexão antes de receber todo o conteúdo do cliente */
                    if (stdineof == 1) break;
                    else {
                        perror("str_cli: server terminated prematurely");

                        exit(1);
                    }
                }

                /* escreve o conteúdo ecoado pelo servido na saída padrão do programa (stdout) */
                sock::Write(fileno(stdout), recvline, n);
            }

            /* Verifica se o file descriptor da entrada padrão do programa (stdin) está 
                preparado para ter seu conteúdo lido */
            if (FD_ISSET(fileno(fp), &rset)) {
                /* lê o conteúdo da entrada padrão (stdin) e verifica se todo o arquivo de entrada foi lido */
                if ((n = sock::Read(fileno(fp), buf + counter, 1)) == 0) {
                    /* em caso de o arquivo de entrada ter sido todo lido, envia para o 
                        servidor o conteúdo final armazenado no buffer 'buf', fecha a conexão com o 
                        socket (servidor) e continua para a próxima iteração do loop. */
                    if (counter != 0) {
                        sock::Write(sockfd, buf, counter);
                    }

                    stdineof = 1;
                    shutdown(sockfd, SHUT_WR); /* envia um FIN para o servidor */

                    FD_CLR(fileno(fp), &rset); /* limpa (zera) o conteúdo de rset */

                    continue;
                }

                counter += n; /* atualiza o número de caracters contido na linha */

                /* verifica se uma linha já foi lida por completo, para poder ser enviada para o servidor */
                if (buf[counter - 1] == '\n' || buf[counter - 1] == '\t') {
                    buf[counter] = '\0';

                    sock::Write(sockfd, buf, counter); /* envia a linha de texto para o servidor */

                    counter = 0; /* zera o contador de caracters da linha */

                    buf[counter] = '\0';
                }
            }
        }
    }

    delete buf;
    delete recvline;
}

int main(int argc, char **argv) {
    /* 
        Verificamos se o usuário passou o número correto de parâmetros
    */
    if (argc != 3) {
        char   error[MAXLINE + 1];

        strcpy(error,"uso: ");
        strcat(error,argv[0]);
        strcat(error," <IPaddress>");
        strcat(error," <Port>");
        perror(error);
        exit(1);
    }

    /* 
        constrói o socket de endereço, definindo conexão do 
        tipo internet (AF_INET), assim como a porta de conexão
        com o servidor
    */
    sock::SocketAddr servaddr(AF_INET, atoi(argv[2]));

    /* seta o endereço ip do servidor */
    servaddr.setInAddress((char *) argv[1]);

    /* cria um novo socket para realizar as requisições */
    int sockfd = sock::Socket(AF_INET, SOCK_STREAM, 0);

    /* conecta o socket criado ao servidor */
    sock::Connect(sockfd, &servaddr);

    str_cli(stdin, sockfd);

    /* fecha a conexão com o servidor */
    sock::Close(sockfd);

    return 0;
}
