
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

#define MAX_LINE 4096

namespace sock {
    class SocketAddr {
        public:
            struct sockaddr_in addr;

            SocketAddr(int family, int port) {
                /* 
                    Setamos a estrutura inteira addr para 0

                    bzero é similar ao memset, contudo mais fácil 
                    de se lembrar, pois contém apenas doi parâmetros
                */
                bzero(&addr, sizeof(addr));

                setInFamily(family);

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

            void setInAddress(char *_addr_str) {
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
        
        recvline[n] = 0;

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

    SocketAddr getLocalInfo(int sockfd) {
        SocketAddr localAddr(0, 0);

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
}

#endif