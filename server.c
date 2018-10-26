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
#define PORT 8880
#define HEART_BEAT_PORT 2222

typedef struct request Request;

struct request
{
	int mode;
	char ip[1025];
	char port[1025];
	char name[1025];
	char partner_name[1025];
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
				else if(state == 2)
					request_temp->name[j] = '\0';
				state++;
				j = 0;
			} else if(state == 0){
				request_temp->ip[j++] = buffer[i];
				request_temp->mode = 0;
			} else if(state == 1){
				request_temp->port[j++] = buffer[i];
				request_temp->mode = 0;
			} else if(state == 2){
				request_temp->name[j++] = buffer[i];
				request_temp->mode = 0;
			} else {
				request_temp->partner_name[j++] = buffer[i];
				request_temp->mode = 1;
			}
	}
	if (state == 2)
		request_temp->name[j] = '\0';
	else if (state == 3)
		request_temp->partner_name[j] = '\0';

	write(1, "ip : ", 5); write(1, request_temp->ip, strlen(request_temp->ip));write(1, "\n", 1);
	write(1, "port : ", 7); write(1, request_temp->port, strlen(request_temp->port));write(1, "\n", 1);
	write(1, "name : ", 7); write(1, request_temp->name, strlen(request_temp->name));write(1, "\n", 1);
	write(1, "pname : ", 8); write(1, request_temp->partner_name, strlen(request_temp->partner_name));write(1, "\n", 1);
	return request_temp;
}

void send_partner_info(Request* src, int dest){
    char buffer[1025] = {'p', 'a', 'r', 't', 'n', 'e', 'r', ':', ' '};
    int i, size = 9;

    for (i = 0; i < sizeof(src->ip); ++i)
    {
    	if (src->ip[i] == '\0')
    	{
    		break;
    	}
    	buffer[size] = src->ip[i];
    	size++;
    }

    buffer[size] = ' ';
    size++;

    for (i = 0; i < sizeof(src->port); ++i)
    {
    	if (src->port[i] == '\0')
    	{
    		break;
    	}
    	buffer[size] = src->port[i];
    	size++;
    }

    buffer[size] = ' ';
    size++;

    for (i = 0; i < sizeof(src->name); ++i)
    {
    	if (src->name[i] == '\0')
    	{
    		break;
    	}
    	buffer[size] = src->name[i];
    	size++;
    }
    buffer[size] = '\0';
    size++;
    
    send(dest, buffer, size, 0);
}

int pair_requests(Request** requests, int* client_socket, int i, int sd){
	int j;
	if (requests[i]->mode == 0){
	    for (j = 0; j < 30; ++j)
	    {
	    	if (requests[j] != NULL && requests[j]->mode == 1 && j != i && client_socket[j] != 0
	    		&& strcmp(requests[j]->partner_name, requests[i]->name) == 0)
	    	{
	    		send_partner_info(requests[j], sd);
	    		send(client_socket[j], "paired", 6, 0);
	    		requests[i] = NULL;
	    		requests[j] = NULL;
	    		client_socket[i] = 0;
	    		client_socket[j] = 0;
	    		return 1;
	    	}
	    }

	    for (j = 0; j < 30; ++j)
	    {
	    	if (requests[j] != NULL && requests[j]->mode == 0 && j != i && client_socket[j] != 0)
	    	{
	    		send_partner_info(requests[j], sd);
	    		send(client_socket[j], "paired", 6, 0);
	    		requests[i] = NULL;
	    		requests[j] = NULL;
	    		client_socket[i] = 0;
	    		client_socket[j] = 0;
	    		return 1;
	    	}
	    }
	} else {
		for (j = 0; j < 30; ++j)
	    {
	    	if (j != i && client_socket[j] != 0 && requests[j] != NULL && requests[j]->mode == 0
	    		&& strcmp(requests[j]->name, requests[i]->partner_name) == 0)
	    	{
	    		send_partner_info(requests[i], client_socket[j]);
	    		send(client_socket[i], "paired", 6, 0);
	    		requests[i] = NULL;
	    		requests[j] = NULL;
	    		client_socket[i] = 0;
	    		client_socket[j] = 0;
	    		return 1;
	    	}

	    	if (j != i && client_socket[j] != 0 && requests[j] != NULL && requests[j]->mode == 1
	    		&& strcmp(requests[j]->name, requests[i]->partner_name) == 0
	    		&& strcmp(requests[j]->partner_name, requests[i]->name) == 0)
	    	{
	    		send_partner_info(requests[i], client_socket[j]);
	    		send(client_socket[i], "paired", 6, 0);
	    		requests[i] = NULL;
	    		requests[j] = NULL;
	    		client_socket[i] = 0;
	    		client_socket[j] = 0;
	    		return 1;
	    	}
	    }
	}
	return 0;
}

void myreverse(char s[])
{
    int i, j;
    char c;
 
    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

void myitoa(int n, char s[])
{
    int i, sign;
 
    if ((sign = n) < 0)  /* record sign */
        n = -n;          /* make n positive */
    i = 0;
    do {       /* generate digits in reverse order */
        s[i++] = n % 10 + '0';   /* get next digit */
    } while ((n /= 10) > 0);     /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    myreverse(s);
}
     
int main(int argc , char *argv[])   
{   
    int opt = TRUE;
    int opt_write = TRUE;   
    int master_socket, heart_beat_socket, addrlen, write_addrlen, new_socket,write_new_socket,
    	client_socket[30], max_clients = 30, activity, write_activity, i, valread, sd;   
    int max_sd;   
    struct sockaddr_in address, heart_beat_address;   
         
    char buffer[1025];  //data buffer of 1K

    Request* requests[30] = {NULL};  
         
    //set of socket descriptors  
    fd_set readfds, writefds;   
    
    if (argc != 5 || strcmp(argv[1], "--server-broadcast-port") != 0
    	|| strcmp(argv[3], "--client-broadcast-port") != 0)
    {
    	write(1, "usage: ./server --server-broadcast-port X --client-broadcast-port Y\n", 69);
    	return 1;
    }
    //a message  
    char message[11] = "127.0.0.1 ";
    strcat(message, argv[4]);   
     
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

    if( (heart_beat_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == 0)   
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
     
    if( setsockopt(heart_beat_socket, SOL_SOCKET, SO_BROADCAST, (char *)&opt_write,  
          sizeof(opt_write)) < 0 )   
    {   
        perror("setsockopt");   
        exit(EXIT_FAILURE);   
    }

    //type of socket created  
    address.sin_family = AF_INET;   
    address.sin_addr.s_addr = INADDR_ANY;   
    address.sin_port = htons( atoi(argv[4]) );   
         
    memset(&heart_beat_address, '0', sizeof(heart_beat_address)); 
    heart_beat_address.sin_family = AF_INET;
    heart_beat_address.sin_addr.s_addr = INADDR_ANY;   
    heart_beat_address.sin_port = htons(atoi(argv[2]));
    //bind the socket to localhost port 
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)   
    {   
        perror("bind failed");   
        exit(EXIT_FAILURE);   
    }   

    write(1, "Listener on port ", 17);write(1, argv[2], strlen(argv[2]));write(1, "\n", 1);   
         
    //try to specify maximum of 3 pending connections for the master socket  
    if (listen(master_socket, 3) < 0)   
    {   
        perror("listen");   
        exit(EXIT_FAILURE);   
    }

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
	            write(2, "select error\n", 13);   
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
	            char snum[1024];
	            myitoa(new_socket, snum);
				write(1, "New connection , socket fd is ", 30);write(1, snum, strlen(snum));
				write(1, " , ip is : ", 11);write(1, inet_ntoa(address.sin_addr), strlen(inet_ntoa(address.sin_addr)));
				int connection_port = ntohs(address.sin_port);
				myitoa(connection_port, snum);
				write(1, " , port : ", 10);write(1, snum, strlen(snum));write(1, "\n", 1);
	            //add new socket to array of sockets  
	            for (i = 0; i < max_clients; i++)   
	            {   
	                //if position is empty  
	                if( client_socket[i] == 0 )   
	                {   
	                    client_socket[i] = new_socket;   
	                    write(1, "Adding to list of sockets\n", 26);   
	                         
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
	          			
	          			char snum[1024];
	                    write(1, "client disconnected , ip ", 25);write(1, inet_ntoa(address.sin_addr), strlen(inet_ntoa(address.sin_addr)));
	                    int connection_port_temp = ntohs(address.sin_port);
	                    myitoa(connection_port_temp, snum);
	                    write(1, " , port ", 8);write(1, snum, strlen(snum));write(1, "\n", 1);   
	                         
	                    //Close the socket and mark as 0 in list for reuse  
	                    close( sd );   
	                    client_socket[i] = 0;
	                    requests[i] = NULL;   
	                }   
	                     
	                //Echo back the message that came in  
	                else 
	                {   
	                    //set the string terminating NULL byte on the end  
	                    //of the data read  
	                    buffer[valread] = '\0';   
	                    //send(sd , buffer , strlen(buffer) , 0 );
	                    requests[i] = extract_request(buffer, valread);
	                    
	                    int status = pair_requests(requests, client_socket, i, sd);
	                    if (status == 1)
	                    	break;
	                }   
	            }   
	        }
	    }
	} else {
    	while(1){
    		sendto(heart_beat_socket, message, strlen(message), 0, (struct sockaddr*)&heart_beat_address,
    			sizeof(heart_beat_address));   
             
            write(1, "Heartbeat message sent successfully\n", 36);
            sleep(1);
        }
    }     
    return 0;   
}   


