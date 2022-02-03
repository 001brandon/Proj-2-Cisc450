/* tcp_ client.c */ 
/* Programmed by Adarsh Sethi */
/* Sept. 19, 2021 */     

#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset, memcpy, and strlen */
#include <netdb.h>          /* for struct hostent and gethostbyname */
#include <sys/socket.h>     /* for socket, connect, send, and recv */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */

#define STRING_SIZE 1024
#define CLIENT_PORT 45925
#define VISITOR_NAME "Dean-Brooks\0"

char buffer[88];
char message_recv[88];

struct client_message {
   unsigned short step_num;
   unsigned short client_port_num;
   unsigned short server_port_num;
   unsigned short server_secret_code;
   char payload[80];
} message, server_response;


int send_message(struct client_message message, char* buffer);

void interpret_server_message(char *buffer, struct client_message* server_response);

int check_travel_file(struct client_message* message, unsigned short server_port);

void update_travel_file(struct client_message* server_response, unsigned short serv_port);


int main(void) {

   int sock_client;  /* Socket used by client */

   struct sockaddr_in server_addr;  /* Internet address structure that
                                        stores server address */
   struct hostent * server_hp;      /* Structure to store server's IP
                                        address */
   char server_hostname[STRING_SIZE]="localhost"; /* Server's hostname */
   unsigned short server_port;  /* Port number used by server (remote port) */

   char receive_buffer[STRING_SIZE]; /* buffer to hold incoming messages */
   unsigned int msg_len;  /* length of message */                      
   int bytes_sent, bytes_recd; /* number of bytes sent or received */

   

   //read file

   for (unsigned short current_port = 48000; current_port < 49000; current_port++) {
      /* open a socket */

      if ((sock_client = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("Client: can't open stream socket");
        exit(1);
      }

      /* initialize server address information */
   
      if ((server_hp = gethostbyname(server_hostname)) == NULL) {
        perror("Client: invalid server hostname");
        close(sock_client);
        exit(1);
      }

      /* Clear server address structure and initialize with server address */
      
      memset(&server_addr, 0, sizeof(server_addr));
      server_addr.sin_family = AF_INET;
      memcpy((char *)&server_addr.sin_addr, server_hp->h_addr,
                                       server_hp->h_length);
      server_addr.sin_port = htons(current_port);

      /* connect to the server */
         
      if (connect(sock_client, (struct sockaddr *) &server_addr, 
                                       sizeof (server_addr)) < 0) {
         //perror("Client: can't connect to server");
         close(sock_client);
         //exit(1);
         continue;
      }
      else {
         
	 /* init values */
         
	 message.step_num = 0;
         message.client_port_num = CLIENT_PORT;
         message.server_port_num = 0;
         message.server_secret_code = 0;
         strcpy(message.payload,"*\0");

         /* check if current port has been met before, adjust step as necessary */
         
	 if (check_travel_file(&message, current_port) != 0) {
	    close(sock_client);
            continue;
	 }
         msg_len = send_message(message, buffer);

         /* send message */
         
	 bytes_sent = send(sock_client, &buffer, msg_len, 0);
         printf("Message sent:        %hu,   %hu,   %hu,   %hu,   %s\n",message.step_num,message.client_port_num,message.server_port_num, message.server_secret_code,message.payload);

         /* get response from server */
         bytes_recd = recv(sock_client, message_recv, 88, 0); //change to different buffer
         interpret_server_message(message_recv, &server_response);
         printf("Message received:    %hu,   %hu,   %hu,   %hu,   %s\n",server_response.step_num,server_response.client_port_num,server_response.server_port_num, server_response.server_secret_code,server_response.payload);
         
         /* edit travel file based on response */
         update_travel_file(&server_response, current_port);

         /* close the socket */
         close (sock_client);
      }
   }
}

int send_message(struct client_message message, char* buffer){
   unsigned short temp;
   temp=htons(message.step_num);
   memcpy(buffer,&temp,2);
   temp=htons(message.client_port_num);
   memcpy(buffer+2,&temp,2);
   temp=htons(message.server_port_num);
   memcpy(buffer+4,&temp,2);
   temp=htons(message.server_secret_code);
   memcpy(buffer+6,&temp,2);
   memcpy(buffer+8,&message.payload,strlen(VISITOR_NAME)+1);
   int msg_len = 9 + strlen(VISITOR_NAME);
   return msg_len;
}

void interpret_server_message(char *buffer,struct client_message* server_response) {
   unsigned short temp;
   memcpy(&temp, buffer, 2); //extract step num
   server_response->step_num = ntohs(temp);
   memcpy(&temp, buffer+2, 2); //extract client port
   server_response->client_port_num = ntohs(temp);
   memcpy(&temp, buffer+4, 2); //extract server port
   server_response->server_port_num = ntohs(temp);
   memcpy(&temp, buffer+6, 2); //extract secret code
   server_response->server_secret_code = ntohs(temp);
   memcpy(server_response->payload, buffer+8, 80);
   
}

int check_travel_file(struct client_message* message, unsigned short server_port) {
   //initialize vars
   char *line=NULL;
   size_t len=0;
   ssize_t read;
   unsigned short server_port_read = 0;
   char line_copy[100];
   //open file
   FILE *travel_file = fopen("./travel.txt", "r");
   if(travel_file==NULL){
      exit(EXIT_FAILURE);
   }
   //iterate through entries
   read = getline(&line,&len,travel_file);
   while (message->step_num == 0 && read > 1) {
      //read=getline(&line,&len,travel_file);
      strcpy(line_copy, line);
      strtok(line_copy, ",");
      server_port_read = (unsigned short) atoi(strtok(NULL, ","));
      if (server_port_read == server_port) {
         message->step_num = (unsigned short) (atoi(strtok(line, ","))) + 1;
         if (message->step_num == 3) { 
            message->step_num = 3;
            strcpy(message->payload,VISITOR_NAME);
         }
	 else if (message->step_num > 3) {
            return 1;
	 }
         message->server_port_num = (unsigned short) (atoi(strtok(NULL, ",")));
         message->server_secret_code = (unsigned short) (atoi(strtok(NULL, ",")));
         return 0;
      }
      read=getline(&line,&len,travel_file);
   }
   message->step_num = 1;
   fclose(travel_file);
   return 0;
}

void update_travel_file(struct client_message* server_response, unsigned short serv_port) {
   //initialize vars
   char *line=NULL;
   size_t len=0;
   ssize_t read;
   unsigned short server_port_read = 0;
   int server_port_known = 0;
   char buffer[100];
   char line_copy[100];

   //open old file (read)
   FILE *travel_file = fopen("./travel.txt", "r");
   if(travel_file==NULL){
      exit(EXIT_FAILURE);
   }

   //open new file (write)
   FILE *temp_file = fopen("./tempTravel.txt", "w");

   //iterate through all entries
   read=getline(&line,&len,travel_file);
   while (read > 1) {
      char line_copy[100];
      strcpy(line_copy, line);
      strtok(line_copy, ",");
      server_port_read = (unsigned short) atoi(strtok(NULL, ","));
      
      //if matches server_port, update vals
      if (server_port_read == serv_port) {
      //if (server_port_read == server_response->server_port_num) {
         //printf("Travel.txt: updated matching entry\n");
         server_port_known = 1;
         snprintf(buffer, sizeof(buffer), "%hu,%hu,%hu,%s\n", 
         server_response->step_num, 
         //server_response->server_port_num, 
         serv_port,
	 server_response->server_secret_code, 
         server_response->payload);
         fputs(buffer, temp_file);
      }
      //else, copy vals
      else {
         fputs(line, temp_file);
      }
      read=getline(&line,&len,travel_file);
   }
   //if server_port not in file, make new entry
   if (server_port_known == 0) {
      //printf("Travel.txt: added new entry\n");
      snprintf(buffer, sizeof(buffer), "%hu,%hu,%hu,%s\n", 
      server_response->step_num, 
      //server_response->server_port_num, 
      serv_port,
      server_response->server_secret_code, 
      server_response->payload);
      fputs(buffer, temp_file);
   }

   fclose(travel_file);
   fclose(temp_file);
   system("mv ./tempTravel.txt ./travel.txt");
   //system("cat ./travel.txt");
}

