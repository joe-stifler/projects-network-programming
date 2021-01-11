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

#define LISTENQ 10
#define MAXDATASIZE 100

int main (int argc, char **argv) {
   time_t ticks;
   int    listenfd, connfd;
   char   buf[MAXDATASIZE];
   struct sockaddr_in servaddr;

   /*
      Nós criamos um socket Internet (AF_INET) stream (SOCK_STREAM) 

      É um nome elegante para descrever um socket TCP. A syscall
      TCP retorna um descritor de arquivo (do linux),
      usado para identificar o socket criado ao longo do programa.

      Com o if, verificamos se o socket foi corretamente criado
   */
   if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("socket");
      exit(1);
   }

   /* 
      Setamos a estrutura inteira servaddr para 0

      bzero é similar ao memset, contudo mais fácil 
      de se lembrar, pois contém apenas doi parâmetros
   */
   bzero(&servaddr, sizeof(servaddr));

   /* 
      seta o endereço da família para o socket de internet (AF_INET)
   */
   servaddr.sin_family      = AF_INET;

   // servaddr.sin_addr.s_addr = htonl("192.168.0.16");

   // servaddr.sin_addr.s_addr = inet_addr("192.168.100.9");

   /*
      O endereço IP é especificado como INADDR_ANY,
      o que permite o servidor aceitar uma conexão de 
      um cliente a partir de qualquer interface de rede
      (considerando que o servidor possua múltiplas interfaces, 
      do contrário, escutará requisições sempre na mesma)
   */
   servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

   /* 
      Força o servidor a escolher automaticamente a porta
      no qual ficará escutando por requisições
   */
   servaddr.sin_port        = htons(0);

   /*
      Utilizamos o bind para atrelar o socket listenfd a uma interface e porta
      especificada por `servaddr`. Como acima utilizamos INADDR_ANY para o IP
      e htons(0) para a porta, sabemos que a porta será escolhida automaticamente
      conforme disponibilidade, e o socket estará atrelado a todas as interfaces
      (todos os IPs do host).
   */
   if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
      perror("bind");
      exit(1);
   }

   char server_local_ip[16];
   struct sockaddr_in server_local_addr;

   /* setamos a estrutura server_local_addr local_addr para 0 */
   bzero(&server_local_addr, sizeof(server_local_addr));

   /* tomamos a quantidade de bytes ocupados pela estrtura server_local_addr */
   socklen_t size_addr = sizeof(server_local_addr);

   /*
      utilizamos a função getsockname para povoar a estrutura 'server_local_addr'
      com base em informações locais do servidor (IP onde o listenfd está conectado,
      assim como a porta onde o listenfd está escutando requisições)
   */
   getsockname(listenfd, (struct sockaddr *) &server_local_addr, &size_addr);

   /*
      convertemos `server_local_addr.sin_addr` de um valor binário para uma
      string no padrão IPv4
   */
   inet_ntop(AF_INET, &server_local_addr.sin_addr, server_local_ip, sizeof(server_local_ip));

   /* 
      convertemos a ordem do inteiro representando a porta onde o servidor está escutando, 
      do padrão usado na rede (big-endian) para o padrão 
      usado no host (little-endian). ntohs --> n de network, 
      to = para, h de host, s de unsigned short integer
   */
   unsigned int userport = ntohs(server_local_addr.sin_port);

   printf("Servidor atrelado ao IP=%s e PORTA=%d\n", server_local_ip, (int) userport);
   printf("****************************\n");

   /*
      Com a syscall listen, o socket é convertido em um socket de escuta,
      no qual conexões de entrada vindas do cliente serão aceitas pelo kernel
      e serão posta numa fila até serem de fato aceitas pelo servidor através da 
      syscall accept. Usamos o LISTENQ para especificar o número máximo de conexões
      que o kernel irá enfileirar for este descritor de escuta (listenfd)

      verificamos com o if se ocorreu algum erro na execução da syscall listen()
   */
   if (listen(listenfd, LISTENQ) == -1) {
      perror("listen");
      exit(1);
   }

   /* 
      Servidor entra em um loop infinito esperando por novas requisições dos clientes
   */
   for ( ; ; ) {
      /*
         Uma conexão TCP usa o que chamamos de three-way handshake para estabelecer a
         conexão. Quando este handshake está completo, a syscall accept retorna
         e o valor retornado pela syscall é um novo descritor (connfd), chamado de
         descritor conectado. Este novo descritor é utilizado para comunicação com o cliente. 

         Caso o retorno do accept seja igual a -1, então houve algum erro com a tentativa
         de aceitar a conexão com o cliente.
      */
      if ((connfd = accept(listenfd, (struct sockaddr *) NULL, NULL)) == -1 ) {
         perror("accept");
         exit(1);
      }

      char userip[16];
      struct sockaddr_in user_addr;

      /* tomamos a quantidade de bytes ocupados pela estrtura user_addr */
      socklen_t size_addr = sizeof(user_addr);

      /* tomamos a quantidade de bytes ocupados pela estrtura user_addr */
      bzero(&user_addr, sizeof(user_addr));

      /*
         getpeername() retorna o endereço do par conectado ao socket connfd. 
         Tal valor retornado é armazenado na estrutura 'user_addr'. Assim,
         somos capazes de capturar o IP e a porta de requisição TCP feita
         pelo cliente.
      */
      getpeername(
         connfd,
         (struct sockaddr*) &user_addr,
         &size_addr
      );

      /*
         convertemos `user_addr.sin_addr` de um valor binário para uma
         string no padrão IPv4
      */
      inet_ntop(AF_INET, &user_addr.sin_addr, userip, sizeof(userip));

      /* 
         converte a ordem do inteiro representando a porta do cliente, 
         do padrão usado na rede (big-endian) para o padrão 
         usado no host (little-endian)
      */
      unsigned int user_port = ntohs(user_addr.sin_port);

      printf("(IP do cliente = %s, Porta do cliente =%d)\n", userip, (int) user_port);

      /* obtem o horário do servidor */
      ticks = time(NULL);
      
      /*
         escreve o horário do servidor no buffer 'buf', utilizando a formatação "%.24s\r\n"
      */
      snprintf(buf, sizeof(buf), "%.24s\r\n", ctime(&ticks));

      /*
         escreve o buffer `buf` no socket connfd conectado com o cliente, para que o
         possa receber o horário obtido pelo servidor
      */
      int ret = write(connfd, buf, strlen(buf));

      /* verifica se houve algum erro com a escrita do buffer no socket descriptor */
      if (!ret) {
         printf("Something went wrong\n");
          
         exit(1);
      }

      /* 
         Quando chamamos a syscall close(), começamos a sequência padrão para término da conexão TCP.
         Um FIN é enviado para cada direção e cada FIN é reconhecido pelos extremos (servidor e cliente)
         para que assim a conexão seja de fato terminada.
      */
      close(connfd);
   }
   
   return(0);
}
