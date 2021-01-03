#include <socket.h>

#define LISTENQ 9
#define MAXLINE 4096
#define NUMCOMMANDS 4
#define MAXCOMMAND 10
#define MAXDATASIZE 100

#include <set>
#include <map>
#include <stdio.h>

int main (int argc, char **argv) {
    /* 
       Verificamos se o usuário passou o número correto de parâmetros
    */
    if (argc != 2) {
       char   error[100];

       strcpy(error,"uso: ");
       strcat(error,argv[0]);
       strcat(error," <Port>");
       perror(error);

       exit(1);
    }

    /* 
       constrói o socket de endereço, definindo conexão do 
       tipo internet (AF_INET), assim como a porta de conexão
       com na qual o servidor ficará escutando 
       (passada pelo usuário em argv[1])
    */
    sock::SocketAddr servaddr(AF_INET, (int) INADDR_ANY, atoi(argv[1])), clientaddr(0, 0, 0);

    /* cria um novo socket */
    int listenfd = sock::Socket(AF_INET, SOCK_STREAM, 0);

    /* atrela este socket a uma porta e interface de rede dada por 'servaddr' */
    sock::Bind(listenfd, &servaddr);

    /* faz com que o socket vire um socket passivo (escuta requisições) */
    sock::Listen(listenfd, LISTENQ);

    int maxi = -1;
    fd_set rset, allset;
    int maxfd = listenfd;
    int nready, client[FD_SETSIZE];

    std::set<int> playing;
    std::map<int, std::string> clients;

    for (int i = 0; i < FD_SETSIZE; ++i) client[i] = -1; /* inicializa todos os índices do vetor de clientes como estando inativos */

    FD_ZERO(&allset); /* zera todos os bits de allset */

    FD_SET(listenfd, &allset); /* seta o bit listenfd de allset */

    /* 
       Servidor entra em um loop infinito esperando por novas requisições dos clientes
    */
    for ( ; ; ) {
        rset = allset; /* atribuição da estrutura */
        nready = sock::Select(maxfd + 1, &rset);

        if (FD_ISSET(listenfd, &rset)) { /* nova conexão de cliente */
            int i;
            /* quando uma requisição for recebida, servidor a aceita */
            int connfd = Accept(listenfd, &clientaddr);      

            /* informa que o file descriptor na posição i agora está ativo (e relacionado com o cliente i) */
            for (i = 0; i < FD_SETSIZE; ++i) {
                if (client[i] < 0) {
                    client[i] = connfd; /* salva descritor */
                    break;
                }
            }

            /* verifica se o número máximo de conexões foi atingido. */
            if (i == FD_SETSIZE) {
                perror("too many clients");

                exit(1);
            }

            /* seta o bit connfd na variável allset */
            FD_SET(connfd, &allset);

            /* verifica o índice do maior file descriptor aberto e o atualiza em maxfd */
            if (connfd > maxfd) maxfd = connfd;

            /* verifica o indíce do maior cliente aberto e o atualiza em maxi */
            if (i > maxi) maxi = i;

            /* pega informações do socket do cliente */
            char *user_data = sock::sock_ntop((struct sockaddr *) &clientaddr.addr, sizeof(clientaddr.addr));

            printf("Client: %s\n", user_data);

            clients[i] = std::string(user_data);

            if (--nready <= 0) continue;    /* não há mais descritores prontos para leitura. continua para a próxima iteração do loop. */
        }

        /* itera sobre todos os descritores abertos (igual ao numero total de clientes ativos)
            e busca o descritor do cliente que possui algum conteúdo a ser lido */
        for (int idCli = 0; idCli <= maxi; ++idCli) {
            int n, sockfdcli;

            /* verifica se o descritor i está ativo */
            if ((sockfdcli = client[idCli]) < 0) continue;

            /* se estiver ativo, verifica se possui algum conteúdo pronto para ser lido */
            if (FD_ISSET(sockfdcli, &rset)) {
                sock::MessageStatus msgStatus;

                /* Lê o conteúdo do sockfd */
                if ((n = sock::Read(sockfdcli, (char *) &msgStatus, sizeof(sock::MessageStatus))) == 0) {
                    /* caso nenhum caracter seja lido, então o cliente fechou a conexão (FIN enviado). então o servidor
                    também fecha a conexão (envia FIN). */
                    
                    FD_CLR(sockfdcli, &allset); /* limpa os bits de allset */

                    clients.erase(idCli);
                    playing.erase(idCli);

                    client[idCli] = -1; /* informa que o cliente i não está mais ativo */                    
                } else {
                    int idPeer;

                    switch (msgStatus) {
                        case sock::NewGameMsg:
                            sock::Read(sockfdcli, (char *) &idPeer, sizeof(idPeer));

                            // verify if peer exists and is available (is not playing already)
                            if (client[idPeer] > 0 && playing.find(idPeer) == playing.end() && playing.find(idCli) == playing.end()) {
                                sock::writeNewGameMsg(client[idPeer], idCli);
                            } else { // the peer is already playing or does not exist
                                // send message to client denying game
                                sock::writeDenyMsg(sockfdcli);
                            }

                            break;
                        case sock::AcceptMsg:
                            sock::Read(sockfdcli, (char *) &idPeer, sizeof(idPeer));

                            // verify if both exists
                            if (client[idPeer] >= 0) {
                                // verify if both are available (are not playing)
                                if (playing.find(idCli) == playing.end() && playing.find(idPeer) == playing.end()) {
                                    auto itCli = clients.find(idCli);
                                    auto itPeer = clients.find(idPeer);

                                    // put both into playing list
                                    playing.insert(idCli);
                                    playing.insert(idPeer);

                                    // send message (with address of peer) to client to start game
                                    sock::writeAcceptMsg(sockfdcli, itPeer->second);

                                    // send message (with address of client) to peer to start game
                                    sock::writeAcceptMsg(client[idPeer], itCli->second);
                                }
                            }
                            
                            break;
                        case sock::DenyMsg:
                            sock::Read(sockfdcli, (char *) &idPeer, sizeof(idPeer));

                            // send message to peer to deny game
                            sock::writeDenyMsg(client[idPeer]);

                            break;
                        
                        case sock::UpdateList:
                            sock::writeListOfClients(sockfdcli, idCli, clients);

                            break;

                        case sock::FinishGame:
                            playing.erase(idCli);

                            break;
                    }
                }

                if (--nready <= 0) break;   /* não há mais descritores prontos para leitura. para loop então */
            }
        }

        fflush(stdout);
    }
   
    return(0);
}