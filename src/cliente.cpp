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

        printf("Your time (the symbol %c). Give the coordinates (line, column) in range [1, 3]: ", player);

        while (fgets(sendline, MAXLINE, stdin) == NULL);

        treat_line(sendline, " ", line, column);

        if (column + line * 3 >= 9 || column + line * 3 < 0)
            printf("[Invalid cells] line = %d, column = %d. Choose a line and column between 1 and 3\n", line + 1, column + 1);
        else {
            if (board[column + line * 3] != ' ') printf("[Invalid cells] line = %d, column = %d. Choose an empty cell.\n", line + 1, column + 1);
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

char play_game(int sockfd, sock::SocketAddr &ppeeraddr, PlayerId player) {
    char winner;
    int line, column;
    char board[9], sendline[MAXLINE];

    for (int i = 0; i < 9; ++i) board[i] = PlayerId::NoPlayer;

    update_screen(board);

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

    switch (winner) {
        case PlayerId::Player1:
        case PlayerId::Player2:
            if (winner == player) printf("You (%c) won the match.\n", player);
            else printf("You (%c) lost the match\n", player);
            
            break;
        
        default:
            printf("No winner. It is a draw.");
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

int main(int argc, char **argv) {
    verify_input(argc, argv);

    sock::SocketAddr cliaddr(AF_INET, (int) INADDR_ANY, 0);

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

    sock::printIpPort(serverfd);

    int num_clis;
    char buf[MAX_LINE];
    std::map<int, std::string> clients;

    sock::Read(serverfd, (char *) &num_clis, sizeof(num_clis));

    for (int i = 0; i < num_clis; ++i) {
        int cli_id;

        sock::Read(serverfd, (char *) &cli_id, sizeof(int));

        int len;

        sock::Read(serverfd, (char *) &len, sizeof(len));

        sock::Read(serverfd, (char *) buf, len * sizeof(char));

        buf[len] = '\0';

        clients[cli_id] = std::string(buf);

        printf("Client %d: %s\n", cli_id, buf);
    }

    while (true);

    sock::Close(serverfd);

    // /* 
    //     constrói o socket de endereço, definindo conexão do 
    //     tipo internet (AF_INET), assim como a porta e o endereço
    //     de conexão do servidor
    // */
    // sock::SocketAddr ppeeraddr(AF_INET, (char *) argv[1], atoi(argv[2]));

    // /* cria um novo socket para realizar as requisições */
    // int peerfd = sock::Socket(AF_INET, SOCK_DGRAM, 0);

    // sock::Bind(peerfd, &cliaddr);

    // if (atoi(argv[3]) == 0) {
    //     char winner = play_game(peerfd, ppeeraddr, PlayerId::Player1);
    // } else {
    //     char winner = play_game(peerfd, ppeeraddr, PlayerId::Player2);
    // }

    return 0;
}
