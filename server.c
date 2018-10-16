#include <stdio.h>  
#include <string.h>   //strlen  
#include <stdlib.h>  
#include <errno.h>  
#include <unistd.h>   //close  
#include <arpa/inet.h>    //close  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros  
     
#define TRUE   1  
#define FALSE  0  
#define PORT 8884
#define HEART_BEAT_PORT 1117  

typedef struct request Request;

struct request
{
	char ip[1025];
	char port[1025];
	char name[1025];
};

Request* extract_request(char * buffer, int size){
	int i, j = 0;
	int state = 0;
	Request* request_temp = (Request*)malloc(sizeof(Request));
	for (i = 0; i < size; ++i)
	{
			if (buffer[i] == ' ')
			{
				if(state == 0)
					request_temp->ip[j] = '\0';
				else if(state == 1)
					request_temp->port[j] = '\0';
				else
					request_temp->name[j] = '\0';
				state++;
				j = 0;
			} else if(state == 0){
				request_temp->ip[j++] = buffer[i];
			} else if(state == 1){
				request_temp->port[j++] = buffer[i];
			} else{
				request_temp->name[j++] = buffer[i];
			}
	}
	printf("ip : %s\nport : %s\nname : %s\n", request_temp->ip, request_temp->port, request_temp->name);
	return request_temp;
}

void send_partner_info(Request* src, Request* dest){
	int sockfd, newsockfd, portno, clilen;
    char buffer[1025];
    struct sockaddr_in serv_addr, cli_addr;
    int n, i, size = 0;

    for (i = 0; i < sizeof(src->ip); ++i)
    {
    	if (src->ip[i] == '\0')
    	{
    		break;
    	}
    	buffer[i] = src->ip[i];
    	size++;
    }

    buffer[i] = ' ';
    size++;

    for (i = 0; i < sizeof(src->port); ++i)
    {
    	if (src->port[i] == '\0')
    	{
    		break;
    	}
    	buffer[i] = src->port[i];
    	size++;
    }

    buffer[i] = ' ';
    size++;

    for (i = 0; i < sizeof(src->name); ++i)
    {
    	if (src->name[i] == '\0')
    	{
    		break;
    	}
    	buffer[i] = src->name[i];
    	size++;
    }
    size++;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
       error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = (char*)(dest->port);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0) 
             error("ERROR on binding");
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) 
         error("ERROR on accept");
    n = write(newsockfd, buffer, size);
    if (n < 0) error("ERROR writing to socket");
    printf("%s to %s \n", buffer, dest->port);
}
     
int main(int argc , char *argv[])   
{   
    int opt = TRUE;
    int opt_write = TRUE;   
    int master_socket , heart_beat_socket , addrlen , write_addrlen , new_socket , write_new_socket , client_socket[30] ,  
          max_clients = 30 , activity , write_activity , i , valread , sd;   
    int max_sd;   
    struct sockaddr_in address, heart_beat_address;   
         
    char buffer[1025];  //data buffer of 1K

    Request* requests[30] = {NULL};  
         
    //set of socket descriptors  
    fd_set readfds, writefds;   
         
    //a message  
    char message[20] = "ECHO Daemon v1.0 \r\n";   
     
    //initialise all client_socket[] to 0 so not checked  
    for (i = 0; i < max_clients; i++)   
    {   
        client_socket[i] = 0;   
    }   
         
    //create a master socket  
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)   
    {   
        perror("socket failed");   
        exit(EXIT_FAILURE);   
    }

    if( (heart_beat_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)   
    {   
        perror("socket failed");   
        exit(EXIT_FAILURE);   
    }   
     
    //set master socket to allow multiple connections ,  
    //this is just a good habit, it will work without this  
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,  
          sizeof(opt)) < 0 )   
    {   
        perror("setsockopt");   
        exit(EXIT_FAILURE);   
    }   
     
    // if( setsockopt(heart_beat_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt_write,  
    //       sizeof(opt_write)) < 0 )   
    // {   
    //     perror("setsockopt");   
    //     exit(EXIT_FAILURE);   
    // }

    //type of socket created  
    address.sin_family = AF_INET;   
    address.sin_addr.s_addr = INADDR_ANY;   
    address.sin_port = htons( PORT );   
         
    heart_beat_address.sin_family = AF_INET;
    heart_beat_address.sin_addr.s_addr = INADDR_ANY;   
    heart_beat_address.sin_port = htons( HEART_BEAT_PORT );
    //bind the socket to localhost port 8888  
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)   
    {   
        perror("bind failed");   
        exit(EXIT_FAILURE);   
    }   
    printf("Listener on port %d \n", PORT);   
         
    //try to specify maximum of 3 pending connections for the master socket  
    if (listen(master_socket, 3) < 0)   
    {   
        perror("listen");   
        exit(EXIT_FAILURE);   
    }   
    
    if (bind(heart_beat_socket, (struct sockaddr *)&heart_beat_address, sizeof(heart_beat_address))<0)   
    {   
        perror("bind failed");   
        exit(EXIT_FAILURE);   
    }
    // if (listen(heart_beat_socket, 3) < 0)   
    // {   
    //     perror("listen");   
    //     exit(EXIT_FAILURE);   
    // }   
    printf("Listener on port %d \n", HEART_BEAT_PORT);

    //accept the incoming connection  
    addrlen = sizeof(address);
    write_addrlen = sizeof(heart_beat_address);   
    puts("Waiting for connections ...");   
    
    if(fork() != 0){    
	    while(TRUE)   
	    {   
	        //clear the socket set  
	        FD_ZERO(&readfds);   
	     
	        //add master socket to set  
	        FD_SET(master_socket, &readfds);   
	        max_sd = master_socket;   
	             
	        //add child sockets to set  
	        for ( i = 0 ; i < max_clients ; i++)   
	        {   
	            //socket descriptor  
	            sd = client_socket[i];   
	                 
	            //if valid socket descriptor then add to read list  
	            if(sd > 0)   
	                FD_SET( sd , &readfds);   
	                 
	            //highest file descriptor number, need it for the select function  
	            if(sd > max_sd)   
	                max_sd = sd;   
	        }   
	     
	        //wait for an activity on one of the sockets , timeout is NULL ,  
	        //so wait indefinitely
	          
	        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);   
	       
	        if ((activity < 0) && (errno!=EINTR))   
	        {   
	            printf("select error");   
	        }   
	             
	        //If something happened on the master socket ,  
	        //then its an incoming connection  
	        if (FD_ISSET(master_socket, &readfds))   
	        {   
	            if ((new_socket = accept(master_socket,  
	                    (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)   
	            {   
	                perror("accept");   
	                exit(EXIT_FAILURE);   
	            }   
	             
	            //inform user of socket number - used in send and receive commands  
	            printf("New connection , socket fd is %d , ip is : %s , port : %d  \n" ,
					new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));   
	           
	            //send new connection greeting message  
	            if( send(new_socket, message, strlen(message), 0) != strlen(message) )   
	            {   
	                perror("send");   
	            }   
	                 
	            puts("Welcome message sent successfully");   
	                 
	            //add new socket to array of sockets  
	            for (i = 0; i < max_clients; i++)   
	            {   
	                //if position is empty  
	                if( client_socket[i] == 0 )   
	                {   
	                    client_socket[i] = new_socket;   
	                    printf("Adding to list of sockets as %d\n" , i);   
	                         
	                    break;   
	                }   
	            }   
	        }   
	             
	        //else its some IO operation on some other socket 
	        for (i = 0; i < max_clients; i++)   
	        {   
	            sd = client_socket[i];   
	                 
	            if (FD_ISSET( sd , &readfds))   
	            {   
	                //Check if it was for closing , and also read the  
	                //incoming message  
	                if ((valread = read( sd , buffer, 1024)) == 0)   
	                {   
	                    //Somebody disconnected , get his details and print  
	                    getpeername(sd , (struct sockaddr*)&address, (socklen_t*)&addrlen);   
	          
	          			printf("Host disconnected , ip %s , port %d \n" ,  
	                          inet_ntoa(address.sin_addr) , ntohs(address.sin_port));   
	                         
	                    //Close the socket and mark as 0 in list for reuse  
	                    close( sd );   
	                    client_socket[i] = 0;   
	                }   
	                     
	                //Echo back the message that came in  
	                else 
	                {   
	                    //set the string terminating NULL byte on the end  
	                    //of the data read  
	                    buffer[valread] = '\0';   
	                    //send(sd , buffer , strlen(buffer) , 0 );
	                    requests[i] = extract_request(buffer, valread);
	                    int j;
	                    for (j = 0; j < 30; ++j)
	                    {
	                    	if (requests[j] != NULL && j != i)
	                    	{
	                    		printf("hhhhhhhhhhhhhh\n");
	                    		send_partner_info(requests[j], requests[i]);
	                    		requests[i] = NULL;
	                    		requests[j] = NULL;
	                    		break;
	                    	}
	                    }
	                }   
	            }   
	        }
	    }
	} else {
    	// while(1){
    	// 	int socket = accept(heart_beat_socket, (struct sockaddr*)NULL, NULL);
    	// 	// // time_t ticks = time(NULL);
	    // 	// printf("wefwegreayuwgewygewyuhekjtgeruwwut4uie\n");  
     //  //       //send new connection greeting message  
     //        if( write(socket, message, strlen(message)) != strlen(message) )   
     //        {   
     //            perror("send");   
     //        }   
             
     //        printf("Heartbeat message sent successfully\n");
     //        sleep(1);
     //    }
    }     
    return 0;   
}   


