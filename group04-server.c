/* tcpserver.c */
/* Programmed by Adarsh Sethi */
/* Sept. 19, 2021 */    

#include <ctype.h>          /* for toupper */
#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset */
#include <sys/socket.h>     /* for socket, bind, listen, accept */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */

#define STRING_SIZE 1024   

/* SERV_TCP_PORT is the port number on which the server listens for
   incoming requests from clients. You should change this to a different
   number to prevent conflicts with others in the class. */

#define SERV_TCP_PORT 48925
#define SERVER_SECRET_CODE 99
#define SERV_LOCATION "Smyrna-Delaware\0"

struct client_message {
   unsigned short step_num;
   unsigned short client_port_num;
   unsigned short server_port_num;
   unsigned short server_secret_code;
   char payload[80];
} clientMessage, serverResponse;


void interpret_client_message(char *buffer, struct client_message* clientMessage);
int create_response(char *buffer, unsigned short server_port, struct client_message clientMessage);
void copyToServerResponse(struct client_message *serverResponse, char *buffer);
void update_visitor_file(struct client_message* clientMessage);

int main(void) {

   int sock_server;  /* Socket on which server listens to clients */
   int sock_connection;  /* Socket on which server exchanges data with client */

   struct sockaddr_in server_addr;  /* Internet address structure that
                                        stores server address */
   unsigned int server_addr_len;  /* Length of server address structure */
   unsigned short server_port;

   struct sockaddr_in client_addr;  /* Internet address structure that
                                        stores client address */
   unsigned int client_addr_len;  /* Length of client address structure */

   char receive_buffer[88];            /* buffer to hold incoming messages */
   char message_buffer[88];
   unsigned int msg_len; /*length of message*/
   int bytes_sent, bytes_recd; /* number of bytes sent or received */
   unsigned int i;  /* temporary loop variable */

   /* open a socket */

   if ((sock_server = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
      perror("Server: can't open stream socket");
      exit(1);                                                
   }

   /* initialize server address information */
    
   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = htonl (INADDR_ANY);  /* This allows choice of
                                        any host interface, if more than one
                                        are present */ 
   server_port = SERV_TCP_PORT; /* Server will listen on this port */
   server_addr.sin_port = htons(SERV_TCP_PORT);

   /* bind the socket to the local server port */

   if (bind(sock_server, (struct sockaddr *) &server_addr,
                                    sizeof (server_addr)) < 0) {
      perror("Server: can't bind to local address");
      close(sock_server);
      exit(1);
   }                     

   /* listen for incoming requests from clients */

   if (listen(sock_server, 50) < 0) {    /* 50 is the max number of pending */
      perror("Server: error on listen"); /* requests that will be queued */
      close(sock_server);
      exit(1);
   }
   printf("I am here to listen ... on port %hu\n\n", SERV_TCP_PORT);
  
   client_addr_len = sizeof (client_addr);

   /* wait for incoming connection requests in an indefinite loop */

   for (;;) {

      sock_connection = accept(sock_server, (struct sockaddr *) &client_addr, 
                                         &client_addr_len);
                     /* The accept function blocks the server until a
                        connection request comes from a client */
      if (sock_connection < 0) {
         perror("Server: accept() error\n"); 
         close(sock_server);
         exit(1);
      }
 
      /* receive the message */

      bytes_recd = recv(sock_connection, receive_buffer, 88, 0);
      
      if (bytes_recd > 0) {
         interpret_client_message(receive_buffer, &clientMessage);
         update_visitor_file(&clientMessage);
         printf("Message received:    %hu,   %hu,   %hu,   %hu,   %s\n",
         clientMessage.step_num,clientMessage.client_port_num,clientMessage.server_port_num,
         clientMessage.server_secret_code, clientMessage.payload);

         /* prepare the message to send */

         msg_len = create_response(message_buffer,server_port,clientMessage); 
         copyToServerResponse(&serverResponse, message_buffer);
      
         printf("Message sent:    %hu,   %hu,   %hu,   %hu,   %s\n",
         serverResponse.step_num,serverResponse.client_port_num,serverResponse.server_port_num,
         serverResponse.server_secret_code, serverResponse.payload);

         /* send message */
      
         bytes_sent = send(sock_connection, message_buffer, msg_len, 0);
      }
      }

      /* close the socket */

      close(sock_connection);
   } 

void copyToServerResponse(struct client_message *serverResponse, char *buffer) {
   unsigned short temp;
   memcpy(&temp, buffer, 2); //extract step num
   serverResponse->step_num = ntohs(temp);
   memcpy(&temp, buffer+2, 2); //extract client port
   serverResponse->client_port_num = ntohs(temp);
   memcpy(&temp, buffer+4, 2); //extract server port
   serverResponse->server_port_num = ntohs(temp);
   memcpy(&temp, buffer+6, 2); //extract secret code
   serverResponse->server_secret_code = ntohs(temp);
   memcpy(serverResponse->payload, buffer+8, strlen(SERV_LOCATION)+1);
}

void interpret_client_message(char *buffer, struct client_message* clientMessage) {
   unsigned short temp;
   memcpy(&temp, buffer, 2); //extract step num
   clientMessage->step_num = ntohs(temp);
   memcpy(&temp, buffer+2, 2); //extract client port
   clientMessage->client_port_num = ntohs(temp);
   memcpy(&temp, buffer+4, 2); //extract server port
   clientMessage->server_port_num = ntohs(temp);
   memcpy(&temp, buffer+6, 2); //extract secret code
   clientMessage->server_secret_code = ntohs(temp);
   memcpy(clientMessage->payload, buffer+8, 80);
}

int create_response(char *buffer, unsigned short server_port, struct client_message clientMessage){
   
   //base message (step one)
   unsigned short temp;
   temp=htons(clientMessage.step_num);
   memcpy(buffer,&temp,2);
   temp=htons(clientMessage.client_port_num);
   memcpy(buffer+2,&temp,2);
   temp=htons(server_port);
   memcpy(buffer+4,&temp,2);
   temp=htons(0);
   memcpy(buffer+6,&temp,2);
   memcpy(buffer+8,"*\0",2);

   //add secret code if step 2
   if (clientMessage.step_num >= 2 && server_port == clientMessage.server_port_num) {
      temp=htons(SERVER_SECRET_CODE);
      memcpy(buffer+6,&temp,2);
   }
   //add location if step 3
   if (clientMessage.step_num == 3 && clientMessage.server_secret_code == SERVER_SECRET_CODE) {
      //memcpy(buffer+8,"Location\0",80);
      memcpy(buffer+8, SERV_LOCATION, 80);
   }
   int msg_len = 9 + strlen(SERV_LOCATION); // header = 8 + \0 = 1 + length of data string
   return msg_len;
  
}



void update_visitor_file(struct client_message* clientMessage) {
   //initialize vars
   char *line=NULL;
   size_t len=0;
   ssize_t read;
   unsigned short client_port_read = 0;
   int client_port_known = 0;
   char buffer[100];
   char line_copy[100];

   //open old file (read)
   FILE *visitor_file = fopen("./visitors.txt", "r");
   if(visitor_file==NULL){
      exit(EXIT_FAILURE);
   }

   //open new file (write)
   FILE *temp_file = fopen("./tempVisitors.txt", "w");

   //iterate through all entries
   read=getline(&line,&len,visitor_file);
   while (read > 1) {
      char line_copy[100];
      strcpy(line_copy, line);
      strtok(line_copy, ",");
      client_port_read = (unsigned short) atoi(strtok(NULL, ","));
      
      //if matches client_port, update vals
      if (client_port_read == clientMessage->client_port_num) {
         printf("Visitors.txt: updated matching entry\n");
         client_port_known = 1;
         snprintf(buffer, sizeof(buffer), "%hu,%hu,%s\n", 
         clientMessage->step_num, 
         clientMessage->client_port_num, 
         clientMessage->payload);
         fputs(buffer, temp_file);
      }
      //else, copy vals
      else {
         fputs(line, temp_file);
      }
      read=getline(&line,&len,visitor_file);
   }
   //if server_port not in file, make new entry
   if (client_port_known == 0) {
      printf("Visitors.txt: added new entry\n");
      snprintf(buffer, sizeof(buffer), "%hu,%hu,%s\n", 
      clientMessage->step_num, 
      clientMessage->client_port_num, 
      clientMessage->payload);
      fputs(buffer, temp_file);
   }

   fclose(visitor_file);
   fclose(temp_file);
   system("mv ./tempVisitors.txt ./visitors.txt");
   system("cat ./visitors.txt");
}


