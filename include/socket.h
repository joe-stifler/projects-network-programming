
#ifndef SOCKET_H
#define SOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include <set>
#include <map>
#include <string>

#define MAX_LINE 1000

namespace sock {
    enum MessageStatus : char {
        NewGameMsg,
        AcceptMsg,
        DenyMsg,
        UpdateList,
        FinishGame
    };

    class SocketAddr {
        public:
            struct sockaddr_in addr;

            SocketAddr(int family, const char *address, int port) {
                /* 
                    Setamos a estrutura inteira addr para 0

                    bzero é similar ao memset, contudo mais fácil 
                    de se lembrar, pois contém apenas doi parâmetros
                */
                bzero(&addr, sizeof(addr));

                setInFamily(family);

                setInAddress(address);

                setPort(port);
            }

            SocketAddr(int family, int address, int port) {
                /* 
                    Setamos a estrutura inteira addr para 0

                    bzero é similar ao memset, contudo mais fácil 
                    de se lembrar, pois contém apenas doi parâmetros
                */
                bzero(&addr, sizeof(addr));

                setInFamily(family);

                setInAddress(address);

                setPort(port);
            }

            void setInFamily(int family) {
                /* 
                    seta o endereço da família para o socket de internet (AF_INET)
                */
                addr.sin_family      = family;
            }

            void setInAddress(int _addr) {
                // addr.sin_addr.s_addr = htonl("192.168.0.16");

                // addr.sin_addr.s_addr = inet_addr("192.168.100.9");

                /*
                    O endereço IP é especificado como INADDR_ANY,
                    o que permite o servidor aceitar uma conexão de 
                    um cliente a partir de qualquer interface de rede
                    (considerando que o servidor possua múltiplas interfaces, 
                    do contrário, escutará requisições sempre na mesma)
                */
                addr.sin_addr.s_addr = htonl(_addr);
            }

            void setInAddress(const char *_addr_str) {
                /* 
                    a função inet_pton (presentation to numeric) é usada para converter
                    o IP do servidor passado pelo usuário no formato de string IPV4, 
                    para o formato binário esperado pela rede.

                    Caso o retorno de inet_pton seja menor ou igual a zero, então
                    houve erro na conversão
                */
                if (inet_pton(addr.sin_family, _addr_str, &addr.sin_addr) <= 0) {
                    perror("inet_pton error");
                    exit(1);
                }
            }

            void setPort(int port) {
                /* 
                    Força o servidor a escolher automaticamente a porta
                    no qual ficará escutando por requisições
                */
                addr.sin_port        = htons(port);
            }
    };

    int Socket(int family, int type, int flags) {
        int sockfd;
        /*
            Nós criamos um socket Internet (AF_INET) stream (SOCK_STREAM) 

            É um nome elegante para descrever um socket TCP. A syscall
            TCP retorna um descritor de arquivo (do linux),
            usado para identificar o socket criado ao longo do programa.

            Com o if, verificamos se o socket foi corretamente criado
        */
        if ((sockfd = socket(family, type, flags)) == -1) {
            perror("socket error");

            exit(1);
        }

        return sockfd;
    }

    void Bind(int sockfd, SocketAddr *sAddr) {
        if (sAddr == NULL) {
            perror("bind null ptr error");
            exit(1);
        }

        /*
            Utilizamos o bind para atrelar o socket listenfd a uma interface e porta
            especificada por `addr`. Como acima utilizamos INADDR_ANY para o IP
            e htons(0) para a porta, sabemos que a porta será escolhida automaticamente
            conforme disponibilidade, e o socket estará atrelado a todas as interfaces
            (todos os IPs do host).
        */
        if (bind(sockfd, (struct sockaddr *) &sAddr->addr, sizeof(sAddr->addr)) == -1) {
            perror("bind error");
            exit(1);
        }
    }

    void Listen(int sockfd, int backlog) {
           /*
            Com a syscall listen, o socket é convertido em um socket de escuta,
            no qual conexões de entrada vindas do cliente serão aceitas pelo kernel
            e serão posta numa fila até serem de fato aceitas pelo servidor através da 
            syscall accept. Usamos o LISTENQ para especificar o número máximo de conexões
            que o kernel irá enfileirar for este descritor de escuta (listenfd)

            verificamos com o if se ocorreu algum erro na execução da syscall listen()
        */
        if (listen(sockfd, backlog) == -1) {
            perror("listen");
            exit(1);
        }
    }

    int Accept(int sockfd, SocketAddr *sockAddr) {
        int connfd;
        socklen_t addrlen =  (sockAddr != NULL) ? sizeof(sockAddr->addr) : 0;
        struct sockaddr *sockAddrAux = (sockAddr != NULL) ? (struct sockaddr *) &sockAddr->addr : NULL;

        /*
            Uma conexão TCP usa o que chamamos de three-way handshake para estabelecer a
            conexão. Quando este handshake está completo, a syscall accept retorna
            e o valor retornado pela syscall é um novo descritor (connfd), chamado de
            descritor conectado. Este novo descritor é utilizado para comunicação com o cliente. 

            Caso o retorno do accept seja igual a -1, então houve algum erro com a tentativa
            de aceitar a conexão com o cliente.
        */
        if ((connfd = accept(sockfd, sockAddrAux, &addrlen)) == -1 ) {
            perror("accept");
            exit(1);
        }

        return connfd;
    }

    void Connect(int sockfd, SocketAddr *sockAddr) {
        if (sockAddr == NULL) {
            perror("connect null ptr error");
            exit(1);
        }

        /*
            A função connect quando aplicada a um socket TCP, estabelece
            uma conexão TCP com o servidor especificador pela estrutura servaddr

            caso conecte retorne um valor menor que zero, então a conexão não foi sucedida
        */
        if (connect(sockfd, (struct sockaddr *) &sockAddr->addr, sizeof(sockAddr->addr)) < 0) {
            perror("connect error");
            exit(1);
        }
    }

    void Write(int sockfd, char *buf, int sizebuf) {
        /*
            escreve o buffer `buf` no socket connfd conectado com o cliente, para que o
            possa receber o horário obtido pelo servidor
        */

        /* verifica se houve algum erro com a escrita do buffer no socket descriptor */
        if (!write(sockfd, buf, sizebuf)) {
            perror("Something went wrong");
            exit(1);
        }
    }

    int Read(int sockfd, char *recvline, int maxline) {
        int n;

        if ((n = read(sockfd, recvline, maxline)) < 0) {
            perror("read error");
            exit(1);
        }
        
        // recvline[n] = 0;

        return n;
    }

    void Close(int sockfd) {
      /* 
         Quando chamamos a syscall close(), começamos a sequência padrão para término da conexão TCP.
         Um FIN é enviado para cada direção e cada FIN é reconhecido pelos extremos (servidor e cliente)
         para que assim a conexão seja de fato terminada.
      */
      close(sockfd);
    }

    char *sock_ntop(const struct sockaddr *sa, socklen_t salen) {
        char portstr[8];
        static char str[128];

        /* verifica se o endereço de socket é do tipo internet (AF_INET) */
        switch(sa->sa_family){
            case AF_INET: {
                struct sockaddr_in *sin = (struct sockaddr_in*) sa;

                /*
                   converte o endereço ip do padrão da rede (big-endian), para o padrão local
                   (little-endian) e no formato de string
                */
                if (inet_ntop(AF_INET, &sin->sin_addr,str, sizeof(str)) == NULL) return NULL;

                if ( ntohs(sin->sin_port) != 0) {
                    /* 
                       converte a porta definida pelo socket do padrão da rede (big-endia),
                       para o padrão local (little-endian)
                    */
                    snprintf(portstr, sizeof(portstr), ":%d",ntohs(sin->sin_port) );

                    /* concatena o ip com a porta numa só string */
                    strcat(str, portstr);

                    return (str);
                }
            }
        }

        return NULL;
    }

    void printIpPort(int sockfd, char *local_ip, int &local_port) {
        /* 16 bytes para IPv4 */
        struct sockaddr_in local_addr;

        /* setamos a estrutura inteira local_addr para 0 */
        bzero(&local_addr, sizeof(local_addr));

        /* tomamos a quantidade de bytes ocupados pela estrtura local_addr */
        socklen_t size_addr = sizeof(local_addr);

        /*
            utilizamos a função getsockname para povoar a estrutura 'local_addr'
            com base em informações locais do host (IP onde o sockfd está conectado,
            assim como a porta onde o sockfd está fazendo a requisição para o servidor)
        */
        getsockname(sockfd, (struct sockaddr *) &local_addr, &size_addr);

        /*
            convertemos `local_addr.sin_addr` de um valor binário para uma
            string no padrão IPv4
        */
        inet_ntop(AF_INET, &local_addr.sin_addr, local_ip, 16 * sizeof(char));

        /* 
            converte a ordem do inteiro representando a porta local do cliente, 
            do padrão usado na rede (big-endian) para o padrão 
            usado no host (little-endian). ntohs--> n de network, 
            to = para, h de host, s de unsigned short integer
        */
        local_port = (int) ntohs(local_addr.sin_port);

        printf("IP do usuário local = %s\nPorta do usuário local = %d\n", local_ip, (int) local_port);
    }

    SocketAddr getLocalInfo(int sockfd) {
        SocketAddr localAddr(0, 0, 0);

        /* tomamos a quantidade de bytes ocupados pela estrtura local_addr */
        socklen_t size_addr = sizeof(localAddr.addr);

        /*
            utilizamos a função getsockname para povoar a estrutura 'local_addr'
            com base em informações locais do host (IP onde o sockfd está conectado,
            assim como a porta onde o sockfd está fazendo a requisição para o servidor)
        */
        getsockname(sockfd, (struct sockaddr *) &localAddr.addr, &size_addr);

        return localAddr;
    }

    int Select(int maxfdp1, fd_set *rset) {
        int n;
        
        if ((n = select(maxfdp1, rset, NULL, NULL, NULL)) < 0) {
            perror("select error");
            exit(1);
        }

        return n;
    }

    int Recvfrom(int sockfd, char msg[], int maxlen, SocketAddr *sockAddr) {
        int n;
        socklen_t addrlen =  (sockAddr != NULL) ? sizeof(sockAddr->addr) : 0;
        struct sockaddr *sockAddrAux = (sockAddr != NULL) ? (struct sockaddr *) &sockAddr->addr : NULL;

        if ((n = recvfrom(sockfd, msg, maxlen, 0, sockAddrAux, &addrlen)) < 0) {
            perror("recvfrom error");
            exit(1);
        }

        msg[n] = '\0';

        return n;
    }

    void Sendto(int sockfd, char msg[], SocketAddr *sockAddr) {
        socklen_t addrlen =  (sockAddr != NULL) ? sizeof(sockAddr->addr) : 0;
        struct sockaddr *sockAddrAux = (sockAddr != NULL) ? (struct sockaddr *) &sockAddr->addr : NULL;

        if (sendto(sockfd, msg, strlen(msg), 0, sockAddrAux, addrlen) < 0) {
            perror("sendto error");
            exit(1);
        }
    }

    void writeDenyMsg(int sockfd) {
        if (sockfd >= 0) {
            auto status = sock::DenyMsg;

            // send message (with address of peer) to client to start game
            sock::Write(sockfd, (char *) &status, sizeof(status));
        }
    }

    void writeDenyMsg2(int sockfd, int idCli) {
        if (sockfd >= 0) {
            auto status = sock::DenyMsg;
            
            // send message to peer (to start a new game)
            sock::Write(sockfd, (char *) &status, sizeof(status));

            // send to peer the id of client i
            sock::Write(sockfd, (char *) &idCli, sizeof(idCli));
        }
    }

    void writeNewGameMsg(int sockfd, int idCli) {
        if (sockfd >= 0) {
            auto status = sock::NewGameMsg;
            
            // send message to peer (to start a new game)
            sock::Write(sockfd, (char *) &status, sizeof(status));

            // send to peer the id of client i
            sock::Write(sockfd, (char *) &idCli, sizeof(idCli));
        }
    }

    void readNewGameMsg(int sockfd, int &idCli) {
        if (sockfd >= 0) {
            // send to peer the id of client i
            sock::Read(sockfd, (char *) &idCli, sizeof(idCli));
        }
    }

    void writeAcceptMsg(int sockfd, std::string address, int randNum) {
        if (sockfd >= 0) {
            auto status = sock::AcceptMsg;

            // send message (with address of peer) to client to start game
            sock::Write(sockfd, (char *) &status, sizeof(status));

            sock::Write(sockfd, (char *) &randNum, sizeof(randNum));

            auto len = (int) address.size();

            sock::Write(sockfd, (char *) &len, sizeof(len));

            sock::Write(sockfd, (char *) address.c_str(), len * sizeof(char));
        }
    }

    void writeAcceptMsg2(int sockfd, int idCli) {
        if (sockfd >= 0) {
            auto status = sock::AcceptMsg;
            
            // send message to peer (to start a new game)
            sock::Write(sockfd, (char *) &status, sizeof(status));

            // send to peer the id of client i
            sock::Write(sockfd, (char *) &idCli, sizeof(idCli));
        }
    }

    void readAcceptMsg(int sockfd, char *recvline, std::string &address, int &randNum) {
        if (sockfd >= 0) {
            sock::Read(sockfd, (char *) &randNum, sizeof(randNum));
            
            int len;

            sock::Read(sockfd, (char *) &len, sizeof(len));

            sock::Read(sockfd, (char *) recvline, len * sizeof(char));

            recvline[len] = '\0';

            address = std::string(recvline);
        }
    }

    void writeListOfClients(int sockfd, int idCli, std::map<int, std::string> &clients, std::set<int> &playing, std::map<int, int> &scores) {
        if (sockfd >= 0) {
            auto status = sock::UpdateList;

            sock::Write(sockfd, (char *) &status, sizeof(status));

            int num_clis = (int) clients.size();

            sock::Write(sockfd, (char *) &idCli, sizeof(idCli));

            sock::Write(sockfd, (char *) &num_clis, sizeof(num_clis));

            for (auto &cli : clients) {
                sock::Write(sockfd, (char *) &cli.first, sizeof(int));

                int score = scores[cli.first];

                sock::Write(sockfd, (char *) &score, sizeof(int));

                bool available = true;

                if (playing.find(cli.first) != playing.end()) available = false;

                sock::Write(sockfd, (char *) &available, sizeof(available));

                int len = (int) cli.second.size();

                sock::Write(sockfd, (char *) &len, sizeof(len));

                sock::Write(sockfd, (char *) cli.second.c_str(), len * sizeof(char));
            }
        }
    }

    void readListOfClients(int sockfd, char *recvline, std::map<int, std::string> &clients, std::set<int> &playing, std::map<int, int> &scores, int &myId) {
        int num_clis;

        sock::Read(sockfd, (char *) &myId, sizeof(myId));

        sock::Read(sockfd, (char *) &num_clis, sizeof(num_clis));

        scores.clear();
        playing.clear();
        clients.clear();

        for (int i = 0; i < num_clis; ++i) {
            int cli_id;

            sock::Read(sockfd, (char *) &cli_id, sizeof(int));

            int score;

            sock::Read(sockfd, (char *) &score, sizeof(int));

            scores[cli_id] = score;

            bool available;

            sock::Read(sockfd, (char *) &available, sizeof(available));

            if (!available) playing.insert(cli_id);

            int len;

            sock::Read(sockfd, (char *) &len, sizeof(len));

            sock::Read(sockfd, (char *) recvline, len * sizeof(char));

            recvline[len] = '\0';

            clients[cli_id] = std::string(recvline);
        }
    }
}

#endif