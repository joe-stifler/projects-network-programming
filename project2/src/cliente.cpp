#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXLINE 4096

#define MY_ADDR "192.168.100.11"

int main(int argc, char **argv) {
   int    sockfd, n;
   char   recvline[MAXLINE + 1];
   char   error[MAXLINE + 1];
   struct sockaddr_in servaddr;

   /* 
      Verificamos se o usuário passou o número correto de parâmetros
   */
   if (argc != 3) {
      strcpy(error,"uso: ");
      strcat(error,argv[0]);
      strcat(error," <IPaddress>");
      strcat(error," <Port>");
      perror(error);
      exit(1);
   }
   
   /*
      Nós criamos um socket Internet (AF_INET) stream (SOCK_STREAM) 

      É um nome elegante para descrever um socket TCP. A syscall
      TCP retorna um descritor de arquivo (do linux),
      usado para identificar o socket criado ao longo do programa.

      Com o if, verificamos se o socket foi corretamente criado
   */
   if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      perror("socket error");
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
   servaddr.sin_family = AF_INET;

   /*
      seta o número da porta que o servidor está escutando requisições

      a função htons (host to network short) é utilizada
      para converter o inteiro passado pelo usuário 
      representando a porta que o servidor está escutando,
      para o padrão de binário esperado pela rede (big-endian)
   */
   servaddr.sin_port   = htons(atoi(argv[2]));

   /* 
      a função inet_pton (presentation to numeric) é usada para converter
      o IP do servidor passado pelo usuário no formato de string IPV4, 
      para o formato binário esperado pela rede.

      Caso o retorno de inet_pton seja menor ou igual a zero, então
      houve erro na conversão
   */
   if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
      perror("inet_pton error");
      exit(1);
   }

   /*
      A função connect quando aplicada a um socket TCP, estabelece
      uma conexão TCP com o servidor especificador pela estrutura servaddr

      caso conecte retorne um valor menor que zero, então a conexão não foi sucedida
   */
   if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
      perror("connect error");
      exit(1);
   }
   
   /* 16 bytes para IPv4 */
   char local_ip[16];
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
   inet_ntop(AF_INET, &local_addr.sin_addr, local_ip, sizeof(local_ip));

   /* 
      converte a ordem do inteiro representando a porta local do cliente, 
      do padrão usado na rede (big-endian) para o padrão 
      usado no host (little-endian). ntohs--> n de network, 
      to = para, h de host, s de unsigned short integer
   */
   unsigned int local_port = (unsigned int) ntohs(local_addr.sin_port);

   printf("IP do usuário local = %s\nPorta do usuário local = %d\n", local_ip, (int) local_port);

   /*
      Devemos ser cuidados ao usar TCP pois é um protocol baseado em
      fluxo de bytes, sem registro de fim. A resposta do servidor é normalmente
      uma string com 26 bytes no formato "Mon May 26 20 : 58 : 40 2003\r\n".

      O while abaixo é necessário uma vez que não podemos garantir que o segmento
      TCP obtido pela syscall read() pegará todo o dado enviado pelo servidor.
      Os 26 bytes podem ser pegos de forma completa somente após múltiplas 
      chamadas ao read().

      O loop apenas para quando o servidor fecha a conexão ou um valor menor do que
      zero é obtido (neste caso representando um erro).
   */
   while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
      recvline[n] = 0; /* definimos o fim da string */

      /*
         printamos a string contida em recvline no stdout do cliente
      
         também verificamos se algum erro ocorreu
      */
      if (fputs(recvline, stdout) == EOF) {
         perror("fputs error");
         exit(1);
      }
   }

   /* verificamos se algum erro foi retornado da leitura dos dados do socket descriptor */
   if (n < 0) {
      perror("read error");
      exit(1);
   }

   /* 
      linux sempre fecha todos os descriptors abertos quando o processo termina
   */
   exit(0);
}
