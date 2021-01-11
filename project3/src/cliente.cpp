#include <socket.h>

#define MAXLINE 4096

int main(int argc, char **argv) {
   char   recvline[MAXLINE + 1];

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

   /* 
      converte informações de porta e ip do servidor que estavam no padrão da rede (big-endian),
      para o padrão local (little-endian) 
    */
   char *server_data = sock::sock_ntop((struct sockaddr *) &servaddr.addr, sizeof(servaddr.addr));

   /* printa informações de ip e porta do servidor */
   if (server_data != NULL) printf("Dados do socket do servidor remoto = %s\n", server_data);

   /* busca informações de porta e ip do usuário local (cliente) */
   sock::SocketAddr clientaddr = sock::getLocalInfo(sockfd);

   /*
      converte informações de porta e ip do cliente que estavam no padrão
      da rede (big-endian), para o padrão local (little-endian) 
   */
   char *user_data = sock::sock_ntop((struct sockaddr *) &clientaddr.addr, sizeof(clientaddr.addr));

   /* printa os dados de IP e porta do usuário local (cliente) */
   if (user_data != NULL) printf("Dados do socket do usuário local = %s\n", user_data);

   /* fica recebendo continuamente comandos do servidor, 
      até que a string 'exit' seja recebida */
   sock::FetchCommands(sockfd, recvline, MAXLINE);

   /* fecha a conexão com o servidor */
   sock::Close(sockfd);

   return 0;
}
