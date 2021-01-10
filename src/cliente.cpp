#include <map>
#include <string>
#include <stdlib.h>
#include <iostream>
#include <socket.h>

#define MAXLINE 1000

enum PlayerId : char {
    NoPlayer = ' ',
    Player1 = 'X',
    Player2 = 'O'
};

void getIpPort(std::string &address, std::string &ip, int &port) {
    std::string s(address);
    std::string token = s.substr(0, s.find(":"));

    ip = token.c_str();

    token = s.substr(s.find(":") + 1, s.size());

    port = std::atoi(token.c_str());
}

void treat_line(char sendline[], std::string delimiter, int &line, int &column) {
    std::string s(sendline);
    std::string token = s.substr(0, s.find(delimiter));

    line = std::atoi(token.c_str()) - 1;

    token = s.substr(s.find(delimiter) + 1, s.size());

    column = std::atoi(token.c_str()) - 1;
}

void read_input(char board[], char sendline[], PlayerId player) {
    int line, column;
    bool valid = false;

    while (!valid) {
        fflush(stdin); 

        printf("Sua vez (seu símbolo é '%c'). Dê as coordenadas (linha, coluna) no intervalo [1, 3]: ", player);

        while (fgets(sendline, MAXLINE, stdin) == NULL);

        treat_line(sendline, " ", line, column);

        if (column + line * 3 >= 9 || column + line * 3 < 0)
            printf("[Células inválidas] linha = %d, coluna = %d. Escolha uma linha e uma coluna entre 1 e 3\n", line + 1, column + 1);
        else {
            if (board[column + line * 3] != ' ') printf("[Células inválidas] linha = %d, coluna = %d. Escolha uma célula vazia.\n", line + 1, column + 1);
            else {
                valid = true;
                board[column + line * 3] = player;
            }
        }
    };
}

/*
  this function is responsible for
  printing the board on the screen
*/
void update_screen(char *board) {
    printf(" --- --- ---\n");

    for (int i = 0; i < 9; ++i) {
        printf("| %c ", board[i]);
        if (i % 3 == 2) {
            printf("|\n");
            printf(" --- --- ---\n");
        }
    }
}

char test_board(char *board) {
    for (int i = 0; i < 3; ++i) {
        /* horizontal test: line i */
        if (board[0 + 3 * i] == board[1 + 3 * i] && board[1 + 3 * i] == board[2 + 3 * i]) return board[2 + 3 * i];

        /* vertical test: column i */
        if (board[i + 0 * 3] == board[i + 1 * 3] && board[i + 1 * 3] == board[i + 2 * 3]) return board[i + 2 * 3];
    }

    /* diagonal test: column i */
    if (board[0 + 0 * 3] == board[1 + 1 * 3] && board[1 + 1 * 3] == board[2 + 2 * 3]) return board[2 + 2 * 3];

    /* antidiagonal test: column i */
    if (board[2 + 0 * 3] == board[1 + 1 * 3] && board[1 + 1 * 3] == board[0 + 2 * 3]) return board[0 + 2 * 3];

    return PlayerId::NoPlayer;
}

char play_game(int sockfd, sock::SocketAddr ppeeraddr, PlayerId player) {
    printf("\033[2J\033[1;1H");

    char winner;
    int line, column;
    char board[9], sendline[MAXLINE];

    for (int i = 0; i < 9; ++i) board[i] = PlayerId::NoPlayer;

    update_screen(board);

    fflush(stdin);

    if (player == PlayerId::Player1) {
        for (int i = 0; i < 4; ++i) {
            read_input(board, sendline, player);

            update_screen(board);

            sock::Sendto(sockfd, sendline, &ppeeraddr);

            if ((winner = test_board(board)) && winner != ' ') break;
            
            sock::Recvfrom(sockfd, sendline, MAX_LINE, &ppeeraddr);

            treat_line(sendline, " ", line, column);

            board[column + line * 3] = PlayerId::Player2;

            update_screen(board);

            if ((winner = test_board(board)) && winner != ' ') break;
        }
    } else {
        for (int i = 0; i < 4; ++i) {
            sock::Recvfrom(sockfd, sendline, MAX_LINE, &ppeeraddr);

            treat_line(sendline, " ", line, column);

            board[column + line * 3] = PlayerId::Player1;

            update_screen(board);

            if ((winner = test_board(board)) && winner != ' ') break;

            read_input(board, sendline, player);

            sock::Sendto(sockfd, sendline, &ppeeraddr);
        
            update_screen(board);

            if ((winner = test_board(board)) && winner != ' ') break;
        }
    }
    
    return winner;
}

void verify_input(int argc, char **argv) {
    /* 
        Verificamos se o usuário passou o número correto de parâmetros
    */
    if (argc != 3) {
        char   error[MAXLINE + 1];

        strcpy(error, "uso: ");
        strcat(error, argv[0]);
        strcat(error, " <IPaddress>");
        strcat(error, " <Port>");
        perror(error);
        exit(1);
    }
}

void printListOfClients(std::map<int, std::string> &clients, std::set<int> &playing, std::map<int, int> &scores, int myId) {
    printf("\033[2J\033[1;1H");
    printf("**********************************\n");
    printf("*       Minhas credenciais:      *\n");
    printf("**********************************\n");
    auto cli = clients.find(myId);
    bool available = true;
    int score = scores[cli->first];

    if (playing.find(cli->first) != playing.end()) available = false;

    printf("* Meu usuario:                   *\n*    - id: %s  *\n*    - score: %d                  *\n*    - status: ", cli->second.c_str(), score);

    if (available) printf("disponível\n");
    else printf("ocupado\n");

    printf("**********************************\n");
    printf("\n\n");

    printf("**********************************\n");
    printf("*       Lista de clientes:       *\n");
    printf("**********************************\n");

    if (clients.size() > 0) {
        for (auto &cli : clients) {
            if (myId != cli.first) {
                bool available = true;
                int score = scores[cli.first];

                if (playing.find(cli.first) != playing.end()) available = false;

                printf("* Cliente %d:                     *\n*    - id: %s  *\n*    - score: %d                  *\n*    - status: ", cli.first, cli.second.c_str(), score);

                if (available) printf("disponível\n");
                else printf("ocupado\n");
                printf("**********************************\n");
            }
        }
    } else {
        printf("*              Vazia             *\n");
    }

    printf("\nEscolha o cliente ('enter' para atualizar lista): ");
    fflush(stdout);
}

int main(int argc, char **argv) {
    char *buf = new char[MAX_LINE];
    char *recvline = new char[MAX_LINE];

    verify_input(argc, argv);

    /*
        constrói o socket de endereço, definindo conexão do 
        tipo internet (AF_INET), assim como a porta de conexão
        com o servidor
    */
    sock::SocketAddr servaddr(AF_INET, (char *) argv[1], atoi(argv[2]));

    /* cria um novo socket para realizar as requisições */
    int serverfd = sock::Socket(AF_INET, SOCK_STREAM, 0);

    /* conecta o socket criado ao servidor */
    sock::Connect(serverfd, &servaddr);

    int counter = 0;
    int local_port = 0; 
    char local_ip[16] = {0};

    sock::printIpPort(serverfd, local_ip, local_port);

    /*
        constrói o socket de endereço, definindo conexão do 
        tipo internet (AF_INET), assim como a porta e o endereço
        de conexão do servidor
    */
    sock::SocketAddr cliaddr(AF_INET, local_ip, local_port);

    /* cria um novo socket para realizar as requisições */
    int peerfd = sock::Socket(AF_INET, SOCK_DGRAM, 0);

    sock::Bind(peerfd, &cliaddr);

    int myId = -1;
    std::set<int> playing;
    std::map<int, int> scores;
    std::map<int, std::string> clients;

    fd_set rset;

    FD_ZERO(&rset); /* limpas os bits de rset */
    
    std::cout << "\033[2J\033[1;1H";

    sock::MessageStatus msgStatus = sock::UpdateList;

    sock::Write(serverfd, (char *) &msgStatus, sizeof(msgStatus));

    int score = 0;
    std::string aceite;
    char winner = PlayerId::NoPlayer;
    int n, idCli, randNum, peerport;
    std::string address, peerip;

    while (true) {
        FD_SET(fileno(stdin), &rset); /* seta o bit de rset referente a posição 'fileno(fp)' */

        FD_SET(serverfd, &rset); /* seta o bit de rset referente a posição 'sockfd' */

        int maxfdp1 = std::max(fileno(stdin), serverfd) + 1;

        /* executa a função select enquanto ou o socket 'sockfd' ou o stdin não estiverem prontos para leitura */
        int nready = sock::Select(maxfdp1, &rset);

        if (nready > 0) {
            /* Verifica se o socket 'sockfd' está pronto para ser lido */
            if (FD_ISSET(serverfd, &rset)) {
                if ((n = sock::Read(serverfd, (char *) &msgStatus, sizeof(sock::MessageStatus))) == 0) {
                    break;
                }

                switch (msgStatus) {
                    case sock::NewGameMsg:
                        sock::readNewGameMsg(serverfd, idCli);

                        printf("\033[2J\033[1;1H");
                        printf("************************************************************\n");
                        printf("* Convite de jogo pelo cliente: %d\n", idCli);

                        printf("* Voce aceita o convite? ('S' ou 'N'): ");

                        aceite = "";
                        std::getline (std::cin, aceite);

                        if (aceite == "S") {
                            sock::writeAcceptMsg2(serverfd, idCli);
                        } else {
                            sock::writeDenyMsg2(serverfd, idCli);
                            printListOfClients(clients, playing, scores, myId);
                        }

                        break;

                    case sock::AcceptMsg:
                        sock::readAcceptMsg(serverfd, recvline, address, randNum);

                        getIpPort(address, peerip, peerport);

                        printf("Starting game with: %s -- %d (rand num = %d)\n", peerip.c_str(), peerport, randNum);

                        score = 0;

                        winner = PlayerId::NoPlayer;

                        if (randNum == 0) {
                            winner = play_game(peerfd, sock::SocketAddr(AF_INET, peerip.c_str(), peerport), PlayerId::Player1);

                            printf("\033[2J\033[1;1H");
                            printf("************************************************************\n");

                            if (winner == PlayerId::Player1) {
                                score += 1;
                                printf("*              Parabéns!!! Você venceu o jogo!             *\n");
                            } else if (winner == PlayerId::Player2) {
                                printf("*              Desculpa!!! Você perdeu o jogo!             *\n");
                            } else {
                                printf("*                Mehhhhhh!!! O jogo empatou!               *\n");
                            }
                        } else {
                            winner = play_game(peerfd, sock::SocketAddr(AF_INET, peerip.c_str(), peerport), PlayerId::Player2);


                            printf("\033[2J\033[1;1H");
                            printf("************************************************************\n");

                            if (winner == PlayerId::Player2) {
                                score += 1;
                                printf("*              Parabéns!!! Você venceu o jogo!             *\n");
                            } else if (winner == PlayerId::Player1) {
                                printf("*              Desculpa!!! Você perdeu o jogo!             *\n");
                            } else {
                                printf("*                Mehhhhhh!!! O jogo empatou!               *\n");
                            }
                        }

                        printf("* Tecle enter para voltar a lista de clientes disponiveis. *\n");
                        printf("************************************************************\n\n\n");

                        msgStatus = sock::FinishGame;

                        sock::Write(serverfd, (char *) &msgStatus, sizeof(msgStatus));

                        sock::Write(serverfd, (char *) &score, sizeof(score));

                        if (fgets (recvline, MAX_LINE, stdin) >= 0) {
                            printListOfClients(clients, playing, scores, myId);
                        }

                        msgStatus = sock::UpdateList;

                        sock::Write(serverfd, (char *) &msgStatus, sizeof(msgStatus));

                        break;

                    case sock::DenyMsg:
                        printf("\033[2J\033[1;1H");
                        printf("************************************************************\n");
                        printf("*          Voce foi rejeitado pelo outro jogador.          *\n");
                        printf("* Tecle enter para voltar a lista de clientes disponiveis. *\n");
                        printf("************************************************************\n\n\n");

                        if (fgets (recvline, MAX_LINE, stdin) >= 0) {
                            printListOfClients(clients, playing, scores, myId);
                        }

                        break;

                    case sock::UpdateList:
                        sock::readListOfClients(serverfd, recvline, clients, playing, scores, myId);

                        printListOfClients(clients, playing, scores, myId);

                        break;

                    case sock::FinishGame:
                        break;
                }
            }

            /* Verifica se o file descriptor da entrada padrão do programa (stdin) está 
                preparado para ter seu conteúdo lido */
            if (FD_ISSET(fileno(stdin), &rset)) {
                /* lê o conteúdo da entrada padrão (stdin) e verifica se todo o arquivo de entrada foi lido */
                int n = sock::Read(fileno(stdin), buf + counter, 1);

                counter += n; /* atualiza o número de caracters contido na linha */

                /* verifica se uma linha já foi lida por completo, para poder ser enviada para o servidor */
                if (buf[counter - 1] == '\n' || buf[counter - 1] == '\t') {
                    buf[counter] = '\0';

                    // send to server --> NewGame
                    int peerId = atoi(buf);
                    bool clientFound = false;

                    for (auto &cli : clients) {
                        if (cli.first == peerId && cli.first != myId) {
                            clientFound = true;
                            break;
                        }
                    }
    
                    if (clientFound) {
                        printf("\033[2J\033[1;1H");
                        printf("************************************************************\n");
                        printf("*                Voce escolheu o cliente: %d                *\n", peerId);
                        printf("*          Agora espere a resposta do outro cliente        *\n");
                        printf("************************************************************\n");

                        sock::writeNewGameMsg(serverfd, peerId);
                    } else if (peerId != 0) {
                        printf("\033[2J\033[1;1H");
                        printf("************************************************************\n");
                        printf("*            Voce escolheu um cliente invalido.            *\n");
                        printf("* Tecle enter para voltar a lista de clientes disponiveis. *\n");
                        printf("************************************************************\n\n\n");

                        if (fgets (recvline, MAX_LINE, stdin) >= 0) {
                            printListOfClients(clients, playing, scores, myId);
                        }
                        
                        sock::MessageStatus msgStatus = sock::UpdateList;

                        sock::Write(serverfd, (char *) &msgStatus, sizeof(msgStatus));
                    } else {
                        sock::MessageStatus msgStatus = sock::UpdateList;

                        sock::Write(serverfd, (char *) &msgStatus, sizeof(msgStatus));
                    }

                    counter = 0; /* zera o contador de caracters da linha */
                }
            }
        }
    }

    sock::Close(serverfd);

    return 0;
}
