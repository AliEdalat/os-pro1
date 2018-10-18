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
#include <fcntl.h> 
#include <sys/ioctl.h> 
     
#define TRUE   1  
#define FALSE  0  
#define PORT 8880
#define HEART_BEAT_PORT 2222
#define MAXRECVSTRING 255 

typedef struct partner Partner;

struct partner
{
    char ip[1025];
    char port[1025];
    char name[1025];
};

int mystrcmp(char* a, char* b, int len_a, int len_b, int size){
    if (len_a >= size && len_b >= size)
    {
        for (int i = 0; i < size; ++i)
        {
            if (a[i] != b[i])
            {
                return 0;
            }
        }
        return 1;
    } else {
        return 0;
    }
}



int extract_result(char* buffer){
    int i = 5;
    if (buffer[i] == '0')
        return 0;
    else
        return 1;
}

int extract_sel(char* buffer, int map[][10])
{
    int i = buffer[5] - '0';
    int j = buffer[7] - '0';
    int val = map[i][j];
    printf("val %d, i %d, j %d\n", val, i, j);
    map[i][j] = 0;
    return val;
}

int is_map_clear(int map[][10]){
    int i, j;
    for (i = 0; i < 10; ++i)
    {
        for (j = 0; j < 10; ++j)
        {
            if (map[i][j] == 1)
            {
                return 0;
            }
        }
    }
    return 1;
}

Partner* extract_partner(char * buffer, int size){
    int i, j = 0;
    int state = 0;
    Partner* partner_temp = (Partner*)malloc(sizeof(Partner));
    for (i = 9; i < size; ++i)
    {
            if (buffer[i] == ' ')
            {
                if(state == 0)
                    partner_temp->ip[j] = '\0';
                else if(state == 1)
                    partner_temp->port[j] = '\0';
                else
                    partner_temp->name[j] = '\0';
                state++;
                j = 0;
            } else if(state == 0){
                partner_temp->ip[j++] = buffer[i];
            } else if(state == 1){
                partner_temp->port[j++] = buffer[i];
            } else{
                partner_temp->name[j++] = buffer[i];
            }
    }
    printf("ip : %s\nport : %s\nname : %s\n", partner_temp->ip, partner_temp->port, partner_temp->name);
    return partner_temp;
}

void fetch_map(int map[][10], char* file_name){
    int i, j;
    int fd = open(file_name, O_RDONLY);
    for (i = 0; i < 10; ++i)
    {
    	int size = 0;
        for (j = 0; j < 20; ++j)
        {
        	char temp;
            read(fd, &temp, 1);
            if (temp == ' ')
             	continue;
            else if(temp == '\n')
             	break;
            else if (temp == 0)
            	break;
            else
             	map[i][size++] = temp - '0';
        }
    }
}

void select_room(int* state, int sd){	
    char input[4];
    // printf("i%d\n", map[2][2]);
    write(1, "select :\n", 9);
    
    read(0, input, 4);
    printf("input : %s\n", input);
    char message[9] ={'s', 'e', 'l', ':', ' ', input[0], input[1], input[2], '\0'};
    send(sd, message, 9, 0);
    (*state)++;
}

void handle_game(int* state, int sd, int map[][10]){
	char buffer[1024] = {0};
	int valread;
	if (*state == 3) {
	    select_room(state, sd);
	} else if (*state == 4) {
	    // char buffer[1024] = {0};
	    valread = read(sd, buffer, 1024); 
	    printf("%s\n", buffer);
	    char res_message[5] ={'r', 'e', 's', ':', ' '};
	    char win_message[9] ={'r', 'e', 's', ':', ' ', 'w', 'i', 'n', '\0'};
	    if (mystrcmp(buffer, win_message, valread, 9, 9))
	        return;
	    else if (mystrcmp(buffer, res_message, valread, 5, 5)){
	        int res = extract_result(buffer);
	        if (res){
	            *state = 3;
	        	select_room(state, sd);
	        }
	        else
	            *state = 5;
	    }
	} else if (*state == 5) {
	    //char buffer[1024] = {0};
	    valread = read(sd, buffer, 1024); 
	    printf("%s\n",buffer );
	    char sel_text[5] = {'s', 'e', 'l', ':', ' '};
	    char zero_message[7] ={'r', 'e', 's', ':', ' ', '0', '\0'};
	    char one_message[7] ={'r', 'e', 's', ':', ' ', '1', '\0'};
	    char win_message[9] ={'r', 'e', 's', ':', ' ', 'w', 'i', 'n', '\0'};
	    if (mystrcmp(buffer, sel_text, valread, 5, 5)){
	        int val = extract_sel(buffer, map);
	        // printf("val %d\n", val);
	        if (is_map_clear(map)){
	            send(sd, win_message, 9, 0);
	            write(1, "lost\n", 5);
	            return;
	        }
	        else if (val){
	            send(sd, one_message, 7, 0);
	            *state = 5;
	        }
	        else{
	            send(sd, zero_message, 7, 0);
	            *state = 3;
	            select_room(state, sd);
	        }
	    }
	}
}

int extract_server_port(char* recv_string){
	int i, j = 0;
    int state = 0;
    char port_string[250];
    for (i = 0; i < 255; ++i)
    {
            if (recv_string[i] == ' ')
            {
                state++;
                j = 0;
            }

            if (state == 1)
            {
            	port_string[j++] = recv_string[i];
            }
    }
    port_string[j] = '\0';
    printf("port : %d\n", atoi(port_string));
    return atoi(port_string);	
}
 
int main(int argc , char *argv[])   
{   
	int map[10][10];
    int opt = TRUE;
    int opt_write = TRUE;   
    int state = 0, master_socket, client_peer = 0, sock , sock2 , heart_beat_socket , addrlen , write_addrlen , new_socket , write_new_socket , client_socket = 0,  
          max_clients = 1 , activity , write_activity , i , valread , sd;   
    int max_sd;   
    struct sockaddr_in address, serv_addr, heart_beat_address;   
         
    char buffer[1025];  //data buffer of 1K  
         
    //set of socket descriptors  
    fd_set readfds;
    Partner* partner;
    fetch_map(map, "map.txt");
         
    //a message  
    char name[50] = {0};
    char pname[50] = {0};
    char message[11] = "127.0.0.1 ";
    strcat(message, argv[1]);
    strcat(message, " ");
    strcat(message, argv[2]);
    if (argc == 4)
    {
    	strcat(message, " ");
    	strcat(message, argv[3]);
    }

    if( (heart_beat_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == 0)   
    {   
        perror("socket failed");   
        exit(EXIT_FAILURE);   
    }

    // unsigned long nonblocking = 1;

    // if (ioctl(heart_beat_socket, FIONBIO, &nonblocking) != 0){
    //     perror("ioctlsocket() failed");
    //     exit(EXIT_FAILURE);
    // }

    // fcntl(heart_beat_socket, F_SETFL, fcntl(heart_beat_socket, F_GETFL, 0) | O_NONBLOCK);

    if( setsockopt(heart_beat_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt_write,  
          sizeof(opt_write)) < 0 )   
    {   
        perror("setsockopt");   
        exit(EXIT_FAILURE);   
    }

    memset(&heart_beat_address, '0', sizeof(heart_beat_address)); 
    heart_beat_address.sin_family = AF_INET;
    heart_beat_address.sin_addr.s_addr = INADDR_ANY;   
    heart_beat_address.sin_port = htons( HEART_BEAT_PORT );

    if (bind(heart_beat_socket, (struct sockaddr *) &heart_beat_address, sizeof(heart_beat_address)) < 0){
        perror("bind() failed");   
        exit(EXIT_FAILURE);
    }

    int recvStringLen;
    char recvString[MAXRECVSTRING+1];
    /* Receive a single datagram from the server */
    if ((recvStringLen = recvfrom(heart_beat_socket, recvString, MAXRECVSTRING, 0, NULL, 0)) < 0){
        perror("recvfrom() failed");   
        exit(EXIT_FAILURE);
    }

    recvString[recvStringLen] = '\0';
    printf("Received: %s\n", recvString);    /* Print the received string */
    close(heart_beat_socket);

    int port = extract_server_port(recvString);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("\n Socket creation error \n"); 
        return -1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr)); 
   
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port); 
 
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    { 
        printf("\nConnection Failed \n"); 
        return -1; 
    }

    printf("connect to %d\n", port);

    //create a master socket  
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)   
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
     
    //type of socket created  
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;   
    address.sin_port = htons(atoi(argv[1]));   

    //bind the socket to localhost port 8888  
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)   
    {   
        perror("bind failed");   
        exit(EXIT_FAILURE);   
    }   
    printf("Listener on port %s \n", argv[1]);   
         
    //try to specify maximum of 3 pending connections for the master socket  
    if (listen(master_socket, 3) < 0)   
    {   
        perror("listen");   
        exit(EXIT_FAILURE);   
    }

    //accept the incoming connection  
    addrlen = sizeof(address);
    puts("Waiting for connections ...");   
    
    while(TRUE)   
    {
    	printf("state : %d\n", state);
    	if (state == 0) {
            send(sock, message, strlen(message), 0);
            state++;
            printf("send message :%s: to server!\n", message);
            continue;
        } else if (state == 1) {
            valread = read(sock, buffer, 1024);
            printf("%s\n",buffer );
            char partner_text[10] = "partner: ";
            char paired_text[7] = "paired";
            if (mystrcmp(buffer, partner_text, valread, 9, 9))
                state++;
            else if (mystrcmp(buffer, paired_text, valread, 6, 6))
            	state = 10;
            // else if (mystrcmp(buffer, sel_text, 1024, 5, 5))
            //     state = 5;
            continue;
        } else if (state == 2) {
            partner = extract_partner(buffer, valread);
            char buffer[1024] = {0};
            close(sock);
            if ((sock2 = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
		    { 
		        printf("\n Socket creation error \n"); 
		        return -1; 
		    }

		    memset(&serv_addr, '0', sizeof(serv_addr)); 
		   
		    serv_addr.sin_family = AF_INET; 
		    serv_addr.sin_addr.s_addr = INADDR_ANY;
		    serv_addr.sin_port = htons(atoi(partner->port)); 
		 
		   
		    if (connect(sock2, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
		    { 
		        printf("\nConnection Failed \n"); 
		        return -1; 
		    }
		    client_peer = 1;
		    printf("connect to %d\n", atoi(partner->port));
            char input[4];
            write(1, "select :\n", 9);
            read(0, input, 4);
            char message[9] ={'s', 'e', 'l', ':', ' ', input[0], input[1], input[2], '\0'};
            send(sock2, message, 9, 0);
            state = 4;
            continue;
        }
        if (client_peer == 0){
	        //clear the socket set  
	        FD_ZERO(&readfds);   
	     
	        //add master socket to set  
	        FD_SET(master_socket, &readfds);   
	        max_sd = master_socket;   
	             
	        //add child sockets to set     
	        //socket descriptor  
	        sd = client_socket;   
	             
	        //if valid socket descriptor then add to read list  
	        if(sd > 0)   
	            FD_SET( sd , &readfds);   
	             
	        //highest file descriptor number, need it for the select function  
	        if(sd > max_sd)   
	            max_sd = sd;
	     
	        //wait for an activity on one of the sockets , timeout is NULL ,  
	        //so wait indefinitely
	        printf("befor select ...\n");
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
	                 
	            puts("Welcome message sent successfully");   
	                 
	            //add new socket to array of sockets  
	               
	            //if position is empty  
	            if( client_socket == 0 )   
	            {   
	                client_socket = new_socket;   
	                printf("Adding to list of sockets as %d\n" , i);  
	            }      
	        }   
	             
	        //else its some IO operation on some other socket   
	        sd = client_socket;   
	             
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
	                client_socket = 0;   
	            }
	            char sel_text[5] = {'s', 'e', 'l', ':', ' '};
	            printf("%s\n", buffer); 
	            if (mystrcmp(buffer, sel_text, valread, 5, 5) && state == 10)
	            	state = 5;
	            if (state == 3) {
		            char input[4];
		            printf("i%d\n", map[2][2]);
	    			write(1, "select :\n", 9);
	    
	    			read(0, input, 4);
	    			printf("input : %s\n", input);
		            char message[9] ={'s', 'e', 'l', ':', ' ', input[0], input[1], input[2], '\0'};
		            send(sd, message, 9, 0);
		            state++;
		        } else if (state == 4) {
		            // char buffer[1024] = {0};
		            // valread = read( sock, buffer, 1024); 
		            printf("%s\n", buffer);
		            char res_message[5] ={'r', 'e', 's', ':', ' '};
		            char win_message[9] ={'r', 'e', 's', ':', ' ', 'w', 'i', 'n', '\0'};
		            if (mystrcmp(buffer, win_message, valread, 9, 9))
		                break;
		            else if (mystrcmp(buffer, res_message, valread, 5, 5)){
		                int res = extract_result(buffer);
		                if (res){
		                    state = 3;
		                	select_room(&state, sd);
		                }
		                else
		                    state = 5;
		            }
		        } else if (state == 5) {
		            //char buffer[1024] = {0};
		            // valread = read( sock , buffer, 1024); 
		            printf("%s\n",buffer );
		            char sel_text[5] = {'s', 'e', 'l', ':', ' '};
		            char zero_message[7] ={'r', 'e', 's', ':', ' ', '0', '\0'};
		            char one_message[7] ={'r', 'e', 's', ':', ' ', '1', '\0'};
		            char win_message[9] ={'r', 'e', 's', ':', ' ', 'w', 'i', 'n', '\0'};
		            if (mystrcmp(buffer, sel_text, valread, 5, 5)){
		                int val = extract_sel(buffer, map);
		                if (is_map_clear(map)){
		                    send(sd, win_message, 9, 0);
		                    write(1, "lost\n", 5);
		                    break;
		                }
		                else if (val){
		                    send(sd, one_message, 7, 0);
		                    state = 5;
		                }
		                else{
		                    send(sd, zero_message, 7, 0);
		                    state = 3;
		                    select_room(&state, sd);
		                }
		            }
		        }      
	        }
	    } else
	    	handle_game(&state, sock2, map);
    }     
    return 0;   
}   


