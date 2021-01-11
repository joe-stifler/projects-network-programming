#include <socket.h>

#define LISTENQ 9
#define MAXLINE 4096
#define NUMCOMMANDS 4
#define MAXCOMMAND 10
#define MAXDATASIZE 100

#include<stdio.h>
#include <string>
#include<unistd.h> 
#include<sys/wait.h> 
#include<sys/types.h> 

void sig_chld(int signo) {
   int stat;
   pid_t pid;

   while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0);
}

int main (int argc, char **argv) {
   char commands[NUMCOMMANDS][MAXCOMMAND] = {"date", "pwd", "ls", END_COMMAND};

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
   sock::SocketAddr servaddr(AF_INET, atoi(argv[1])), clientaddr(0, 0);

   /*
      define que o servidor escutará requisições em 
      todas as interfaces de rede
   */
   servaddr.setInAddress((int) INADDR_ANY);

   /* cria um novo socket */
   int listenfd = sock::Socket(AF_INET, SOCK_STREAM, 0);

   /* atrela este socket a uma porta e interface de rede dada por 'servaddr' */
   sock::Bind(listenfd, &servaddr);

   /* faz com que o socket vire um socket passivo (escuta requisições) */
   sock::Listen(listenfd, LISTENQ);

   signal(SIGCHLD, sig_chld);

   int userId = 0;

   /* 
      Servidor entra em um loop infinito esperando por novas requisições dos clientes
   */
   while (true) {
      int pid;

      // sleep(12);
      
      /* quando uma requisição for recebida, servidor a aceita */
      int connfd = Accept(listenfd, &clientaddr);

      /* 
         faz com que o programa se divida em dois processos (pai e filho).
         O processo que entra no if é o filho 
      */
      if((pid = fork()) == 0) {
         char buf[MAXDATASIZE];
         
         /* obtem o horário do servidor */
         time_t ticks = time(NULL);

         /*
            escreve o horário do servidor no buffer 'buf', utilizando a formatação "%.24s\r\n"
         */
         snprintf(buf, sizeof(buf), "%.24s\r\n", ctime(&ticks));

         sock::Close(listenfd); /* processo filho fecha o socket de escuta listenfd */

         char   recvline[MAXLINE + 1];
         char *user_data = sock::sock_ntop((struct sockaddr *) &clientaddr.addr, sizeof(clientaddr.addr));

         // if (user_data != NULL) printf("Dados do socket do usuário remoto = %s\n", user_data);
         
         // sock::Write(connfd, buf, strlen(buf));

         /* Abrimos o arquivo que contém o output da execução do cliente */
         // FILE *fp = fopen(std::string("user_file_" + std::to_string(userId) + ".txt").c_str(), "w+");

         /* printa num arquivo o inicio da conexão com o cliente, assim com o horário em que a mesma ocorreu */
         // std::string message = "Início da conexão com cliente " + std::string(user_data) + ": " + std::string(buf);

         // /* Lemos estes bytes para o buffer recvline */
         // if (fwrite(message.c_str(), sizeof(char), message.size(), fp) != (size_t) message.size()) {
         //    perror("read error");
         //    exit(1);
         // }

         /* Itera sobre cada um dos commandos especificados no array de comandos */
         for (int i = 0; i < NUMCOMMANDS; ++i) {
            int numBytes = strlen(commands[i]);

            /* Escreve no socket do cliente o tamanho (em bytes) do commando */
            sock::Write(connfd, (char *) &numBytes, sizeof(int));
            
            /* Escreve no socket do cliente os bytes do comando */
            sock::Write(connfd, commands[i], strlen(commands[i]));

            /* Verifica se não é o último comando (EXIT), onde não é
               necessário realizar leitura vinda do cliente 
            */
            if (i + 1 != NUMCOMMANDS) {
               /* Lê os bytes que o cliente escreveu no socket */
               int numBytes = sock::ReadSocket(connfd, recvline, MAXLINE);

               /* Verfica se os dados do cliente (IP e porta) são válidos, assim como os bytes lidos */
               if (user_data != NULL && numBytes > 0) {
                  // printf("Resultado retornado pelo cliente %s para o comando `%s`:\n%s\n", user_data, commands[i], recvline);

                  // message = "***************************************\nResultado retornado pelo cliente " + std::string(user_data) + " para o comando: '" + std::string(commands[i]) + "'\n\n" + std::string(recvline);

                  // /* Lemos estes bytes para o buffer recvline */
                  // if (fwrite(message.c_str(), sizeof(char), message.size(), fp) != (size_t) message.size()) {
                  //    perror("read error");
                  //    exit(1);
                  // }
               }
               
               fflush(stdout);

               sleep(1); /* pausa a execução do programa por 4 segundos */
            }
         }

         sock::Close(connfd); /* fecha a conexão com o cliente */

         /* obtem o horário do servidor */
         ticks = time(NULL);

         /*
            escreve o horário do servidor no buffer 'buf', utilizando a formatação "%.24s\r\n"
         */
         snprintf(buf, sizeof(buf), "%.24s\r\n", ctime(&ticks));

         /* printa num arquivo o fim da conexão com o cliente, assim com o horário em que a mesma ocorreu */
         // message = "***************************************\nFim da conexão com cliente " + std::string(user_data) + ": " + std::string(buf);

         // /* Lemos estes bytes para o buffer recvline */
         // if (fwrite(message.c_str(), sizeof(char), message.size(), fp) != (size_t) message.size()) {
         //    perror("read error");
         //    exit(1);
         // }

         // close(fp);

         exit(0); /* processo filho termina */
      }

      /* printa os ids dos processos pai e filho */
      // printf("Child PID = %d (Parent PID = %d)\n", pid, getpid());

      sock::Close(connfd); /* fecha o socket que foi conectado com o cliente (aqui o processo pai executa) */

      ++userId; /* atualiza o número de usuários atendidos */
   }
   
   return 0;
}
