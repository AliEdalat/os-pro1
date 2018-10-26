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
    int i;
    if (len_a >= size && len_b >= size)
    {
        for (i = 0; i < size; ++i)
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
    write(1, "select :\n", 9);
    read(0, input, 4);
    if ('0' > input[0] || '9' < input[0] || '0' > input[2] || '9' < input[2] || input[1] != ' ' || input[3] != '\n'){
    	*state = 3;
    	write(1, "input is invalid!!\n", 19);
    	return;
    }
    char message[9] ={'s', 'e', 'l', ':', ' ', input[0], input[1], input[2], '\0'};
    send(sd, message, 9, 0);
    write(1, "select message has been sent!\n", 30);
    *state = 4;
}

int extract_server_port(char recv_string[255], int size){
	int i, j = 0;
    int state = 0;
    char port_string[255];
    for (i = 10; i < size; ++i)
    {
        port_string[j++] = recv_string[i];
    }
    port_string[j] = '\0';
    printf("port : %d\n", atoi(port_string));
    return atoi(port_string);	
}

void start_again(int map[][10], int* sock, int* state, char *argv[]){
	int heart_beat_socket;
	struct sockaddr_in serv_addr, heart_beat_address;
	fetch_map(map, "map.txt");
    if((heart_beat_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == 0)   
    {   
        perror("socket failed");   
        exit(EXIT_FAILURE);   
    }

    struct timeval tv;
	tv.tv_sec = 1;
    tv.tv_usec = 0;
    if (setsockopt(heart_beat_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
       printf("Couldn't set socket timeout\n");
       exit(EXIT_FAILURE);
    }

    memset(&heart_beat_address, '0', sizeof(heart_beat_address)); 
    heart_beat_address.sin_family = AF_INET;
    heart_beat_address.sin_addr.s_addr = INADDR_ANY;   
    heart_beat_address.sin_port = htons(atoi(argv[2]));

    if (bind(heart_beat_socket, (struct sockaddr *) &heart_beat_address, sizeof(heart_beat_address)) < 0){
        perror("bind() failed");   
        exit(EXIT_FAILURE);
    }

    int recvStringLen;
    char recvString[MAXRECVSTRING+1];
    /* Receive a single datagram from the server */
    if ((recvStringLen = recvfrom(heart_beat_socket, recvString, MAXRECVSTRING, 0, NULL, 0)) < 0){
        printf("recvfrom() broadcast port of server has been failed!!!\n");   
        //exit(EXIT_FAILURE);
        *state = 20;
        close(heart_beat_socket);
    }

    recvString[recvStringLen] = '\0';
    printf("Received: %s\n", recvString);    /* Print the received string */
    close(heart_beat_socket);

    int port = extract_server_port(recvString, recvStringLen);

    if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("\n Socket creation error \n"); 
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, '0', sizeof(serv_addr)); 
   
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port); 
 
   
    if (connect(*sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    { 
        printf("\nConnection Failed \n"); 
        *state = 20;
        return; 
    }

    printf("connect to %d\n", port);
}

void handle_game(int* state, int sd, int map[][10], char *argv[], int* sock){
	char buffer[1024] = {0};
	int valread;
	if (*state == 3) {
	    select_room(state, sd);
	} else if (*state == 4) {
	    valread = read(sd, buffer, 1024);
	    if (valread <= 0)
	    {
	    	*state = 0;
	    	write(1, "your friend is disconnected!!!\n", 31);
	    	start_again(map, sock, state, argv);
	    	return;
	    }
	    write(1, "received : ", 11);
        write(1, buffer, valread);
        write(1, "\n", 1);
	    char res_message[5] ={'r', 'e', 's', ':', ' '};
	    char win_message[9] ={'r', 'e', 's', ':', ' ', 'w', 'i', 'n', '\0'};
	    if (mystrcmp(buffer, win_message, valread, 9, 9)){
	        *state = 30;
	        write(1, "win\n", 4);
	    	return;
	    }
	    else if (mystrcmp(buffer, res_message, valread, 5, 5)){
	        int res = extract_result(buffer);
	        if (res)
	        	select_room(state, sd);
	        else
	            *state = 5;
	    }
	} else if (*state == 5) {
		write(1, "wait for your friend's selection...\n", 36);
	    valread = read(sd, buffer, 1024);
	    if (valread <= 0)
	    {
	    	*state = 0;
	    	write(1, "your friend is disconnected!!!\n", 31);
	    	start_again(map, sock, state, argv);
	    	return;
	    }
	    char sel_text[5] = {'s', 'e', 'l', ':', ' '};
	    char zero_message[7] ={'r', 'e', 's', ':', ' ', '0', '\0'};
	    char one_message[7] ={'r', 'e', 's', ':', ' ', '1', '\0'};
	    char win_message[9] ={'r', 'e', 's', ':', ' ', 'w', 'i', 'n', '\0'};
	    if (mystrcmp(buffer, sel_text, valread, 5, 5)){
	        int val = extract_sel(buffer, map);
	        if (is_map_clear(map)){
	            send(sd, win_message, 9, 0);
	            write(1, "lost\n", 5);
	            *state = 30;
	            write(1, "response message has been sent!\n", 32);
	            return;
	        }
	        else if (val){
	            send(sd, one_message, 7, 0);
	            *state = 5;
	            write(1, "response message has been sent!\n", 32);
	        }
	        else{
	            send(sd, zero_message, 7, 0);
	            write(1, "response message has been sent!\n", 32);
	            select_room(state, sd);
	        }
	    }
	}
}

void copy(char* recvString, char* buffer){
	int i = 0;
	while(recvString[i] != '\0'){
		buffer[i] = recvString[i];
		i++;
	}
	buffer[i] = '\0';
}

void create_server_message(int argc, char *argv[], char* message){
	strcat(message, argv[5]);
    strcat(message, " ");
    strcat(message, argv[6]);
    if (argc == 8)
    {
    	strcat(message, " ");
    	strcat(message, argv[7]);
    }
}

void handle_server_side_of_game(char* buffer, int valread, int* state, int sd, int map[][10]){
	char sel_text[5] = {'s', 'e', 'l', ':', ' '};
    if (mystrcmp(buffer, sel_text, valread, 5, 5) && *state == 10)
    	*state = 5;
    if (*state == 3) {
        select_room(state, sd);
    } else if (*state == 4) {
    	write(1, "received : ", 11);
        write(1, buffer, valread);
        write(1, "\n", 1);
        char res_message[5] ={'r', 'e', 's', ':', ' '};
        char win_message[9] ={'r', 'e', 's', ':', ' ', 'w', 'i', 'n', '\0'};
        if (mystrcmp(buffer, win_message, valread, 9, 9)){
            *state = 30;
	        return;
	    } else if (mystrcmp(buffer, res_message, valread, 5, 5)){
            int res = extract_result(buffer);
            if (res){
            	select_room(state, sd);
            }
            else
                *state = 5;
        }
    } else if (*state == 5) {
        char sel_text[5] = {'s', 'e', 'l', ':', ' '};
        char zero_message[7] ={'r', 'e', 's', ':', ' ', '0', '\0'};
        char one_message[7] ={'r', 'e', 's', ':', ' ', '1', '\0'};
        char win_message[9] ={'r', 'e', 's', ':', ' ', 'w', 'i', 'n', '\0'};
        if (mystrcmp(buffer, sel_text, valread, 5, 5)){
            int val = extract_sel(buffer, map);
            if (is_map_clear(map)){
                send(sd, win_message, 9, 0);
                write(1, "lost\n", 5);
                *state = 30;
                write(1, "response message has been sent!\n", 32);
	            return;
            }
            else if (val){
                send(sd, one_message, 7, 0);
                *state = 5;
                write(1, "response message has been sent!\n", 32);
            }
            else{
                send(sd, zero_message, 7, 0);
                write(1, "response message has been sent!\n", 32);
                select_room(state, sd);
            }
        }
    }	
}

int handle_response_of_client_request(int master_socket, int* client_socket, int* addrlen, fd_set* readfds,
	struct sockaddr_in* address, struct timeval* tv){
	int new_socket, max_sd, sd, activity;
	//clear the socket set  
    FD_ZERO(readfds);   
 
    //add master socket to set  
    FD_SET(master_socket, readfds);   
    max_sd = master_socket;   
         
    //add child sockets to set     
    //socket descriptor  
    sd = *client_socket;   
         
    //if valid socket descriptor then add to read list  
    if(sd > 0)   
        FD_SET(sd , readfds);   
         
    //highest file descriptor number, need it for the select function  
    if(sd > max_sd)   
        max_sd = sd;
 
    //wait for an activity on one of the sockets , timeout is NULL ,  
    //so wait indefinitely
    activity = select( max_sd + 1 , readfds , NULL , NULL , tv);   
   
    if ((activity < 0) && (errno!=EINTR))   
    {   
        write(2, "select error\n", 13);   
    }   
         
    //If something happened on the master socket ,  
    //then its an incoming connection  
    if (FD_ISSET(master_socket, readfds))   
    {   
        if ((new_socket = accept(master_socket,  
                (struct sockaddr *)address, (socklen_t*)addrlen))<0)   
        {   
            perror("accept");   
            exit(EXIT_FAILURE);   
        }   
         
        //inform user of socket number - used in send and receive commands  
        printf("New connection , socket fd is %d , ip is : %s , port : %d  \n" ,
			new_socket , inet_ntoa(address->sin_addr) , ntohs(address->sin_port));   
             
        //add new socket
        //if position is empty  
        if( *client_socket == 0 )   
        {   
            *client_socket = new_socket;   
            write(1, "Adding to list of sockets\n", 26);  
        }
        return 1;      
    }
    return 0;
}

void handle_client_to_server_connection(int* state, int* sock2, int* valread, int* client_peer, char* buffer,
	char* argv[], int argc, int sock, struct sockaddr_in* serv_addr, Partner* partner){
	if (*state == 0) {
		char message[11] = "127.0.0.1 ";
		create_server_message(argc, argv, message);    		
        if (send(sock, message, strlen(message), 0) < 0){
        	write(1, "server is down!!!\n", 18);
        	*state = 20;
        	return;
        }
        (*state)++;
        printf("send message :%s: to server!\n", message);
        return;
    } else if (*state == 1) {
    	write(1, "wait for server response...\n", 28);
        *valread = read(sock, buffer, 1024);
        if (*valread <= 0)
        {
        	write(1, "server is down!!!\n", 18);
        	*state = 20;
        	return;
        }
        char partner_text[10] = "partner: ";
        char paired_text[7] = "paired";
        if (mystrcmp(buffer, partner_text, *valread, 9, 9))
            (*state)++;
        else if (mystrcmp(buffer, paired_text, *valread, 6, 6))
        	(*state) = 10;
        return;
    } else if (*state == 2) {
        partner = extract_partner(buffer, *valread);
        char buffer[1024] = {0};

        if ((*sock2 = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	    { 
	        printf("\n Socket creation error \n"); 
	        return; 
	    }

	    memset(serv_addr, '0', sizeof(*serv_addr)); 
	   
	    serv_addr->sin_family = AF_INET; 
	    serv_addr->sin_addr.s_addr = INADDR_ANY;
	    serv_addr->sin_port = htons(atoi(partner->port)); 
	 
	   
	    if (connect(*sock2, (struct sockaddr *)serv_addr, sizeof(*serv_addr)) < 0) 
	    { 
	        printf("\nConnection Failed \n"); 
	        return; 
	    }
	    *client_peer = 1;
	    printf("connect to %d\n", atoi(partner->port));
        select_room(state, *sock2);
        return;
    }
}

int handle_server_status(char *argv[], int* sock, int* state, int* client_peer, int map[][10]){
	*state = 0;
	*client_peer = 0;
	start_again(map, sock, state, argv);
	if(*state == 0)
		return 1;
	return 0;
}

int main(int argc , char *argv[])   
{   
	START : ;
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
    
    if ((argc != 7 && argc != 8) || strcmp(argv[1], "--server-broadcast-port") != 0
    	|| strcmp(argv[3], "--client-broadcast-port") != 0)
    {
    	write(1, "usage: ./client --server-broadcast-port X --client-broadcast-port Y client_port client_name client_pname(optional)\n", 116);
    	return 1;
    }
    //a message
    char message[11] = "127.0.0.1 ";
    create_server_message(argc, argv, message);

    if((heart_beat_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == 0)   
    {   
        perror("socket failed");   
        exit(EXIT_FAILURE);   
    }

    struct timeval tv;
	tv.tv_sec = 1;
    tv.tv_usec = 0;
    if (setsockopt(heart_beat_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
       printf("Couldn't set socket timeout\n");
       return 0;
    }

    memset(&heart_beat_address, '0', sizeof(heart_beat_address)); 
    heart_beat_address.sin_family = AF_INET;
    heart_beat_address.sin_addr.s_addr = INADDR_ANY;   
    heart_beat_address.sin_port = htons(atoi(argv[2]));

    if (bind(heart_beat_socket, (struct sockaddr *) &heart_beat_address, sizeof(heart_beat_address)) < 0){
        perror("bind() failed");   
        exit(EXIT_FAILURE);
    }

    int recvStringLen;
    char recvString[MAXRECVSTRING+1];
    /* Receive a single datagram from the server */
    if ((recvStringLen = recvfrom(heart_beat_socket, recvString, MAXRECVSTRING, 0, NULL, 0)) < 0){
        printf("recvfrom() broadcast port of server has been failed!!!\n");   
        //exit(EXIT_FAILURE);
        state = 20;
        close(heart_beat_socket);
    }

    if (state == 0){
	    recvString[recvStringLen] = '\0';
	    printf("Received: %s\n", recvString);    /* Print the received string */
	    close(heart_beat_socket);

	    int port = extract_server_port(recvString, recvStringLen);

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
	}

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
    address.sin_port = htons(atoi(argv[5]));   

    //bind the socket to localhost port 8888  
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)   
    {   
        perror("bind failed");   
        exit(EXIT_FAILURE);   
    }   
    printf("Listener on port %s \n", argv[5]);   
         
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
    	handle_client_to_server_connection(&state, &sock2, &valread, &client_peer, buffer, argv, argc, sock, &serv_addr, partner);
    	if (state == 0 || state == 1 || state == 2)
    	{
    		continue;
    	}
        if (state == 20) {
        	int client_broadcast_socket;
        	struct sockaddr_in client_broadcast_address;
        	if( (client_broadcast_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == 0)   
		    {   
		        perror("socket failed");   
		        exit(EXIT_FAILURE);   
		    }

		    struct timeval tv;
			tv.tv_sec = 1;
		    tv.tv_usec = 0;
		    if (setsockopt(client_broadcast_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
		    {
		       printf("Couldn't set socket timeout\n");
		       return 0;
		    }

		    memset(&client_broadcast_address, '0', sizeof(client_broadcast_address)); 
		    client_broadcast_address.sin_family = AF_INET;
		    client_broadcast_address.sin_addr.s_addr = INADDR_ANY;   
		    client_broadcast_address.sin_port = htons(atoi(argv[4]));

		    if (bind(client_broadcast_socket, (struct sockaddr *) &client_broadcast_address,
		    		sizeof(client_broadcast_address)) < 0){
		        perror("bind() failed");   
		        exit(EXIT_FAILURE);
		    }

		    int recvStringLen;
		    char recvString[MAXRECVSTRING+1];
		    /* Receive a single datagram from the server */
		    if ((recvStringLen = recvfrom(client_broadcast_socket, recvString, MAXRECVSTRING, 0, NULL, 0)) < 0){
		        printf("recvfrom() broadcast port of client has been failed!!!\n");   
		        //exit(EXIT_FAILURE);
		        state = 21;
		        close(client_broadcast_socket);
		        continue;
		    }

		    recvString[recvStringLen] = '\0';
		    copy("partner: ", buffer);
		    strcat(buffer, recvString);
		    printf("%s\n", buffer);
		    valread = recvStringLen + 9;
		    close(client_broadcast_socket);
		    state = 2;
		    continue;
        } else if (state == 21){
			int client_broadcast_socket, opt_write = 1;
        	struct sockaddr_in client_broadcast_address;
        	struct timeval tv;
			tv.tv_sec = 1;
		    tv.tv_usec = 0;

        	if((client_broadcast_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == 0)   
		    {   
		        perror("socket failed");   
		        exit(EXIT_FAILURE);   
		    }

		    if( setsockopt(client_broadcast_socket, SOL_SOCKET, SO_BROADCAST, (char *)&opt_write,  
		          sizeof(opt_write)) < 0 )   
		    {   
		        perror("setsockopt");   
		        exit(EXIT_FAILURE);   
		    }


		    memset(&client_broadcast_address, '0', sizeof(client_broadcast_address)); 
		    client_broadcast_address.sin_family = AF_INET;
		    client_broadcast_address.sin_addr.s_addr = INADDR_ANY;   
		    client_broadcast_address.sin_port = htons(atoi(argv[4]));

		    char message[11] = "127.0.0.1 ";
		    strcat(message, argv[5]);
		    strcat(message, " ");
		    strcat(message, argv[6]);
		    if (argc == 8)
		    {
		    	strcat(message, " ");
		    	strcat(message, argv[7]);
		    }

		    while(1){
		    	if (handle_response_of_client_request(master_socket, &client_socket, &addrlen, &readfds, &address, &tv))
		    		break;
		    	// if (handle_server_status(argv ,&sock, &state, &client_peer, map))
		    	// 	break;
		  
	    		sendto(client_broadcast_socket, message, strlen(message), 0, (struct sockaddr*)&client_broadcast_address,
	    			sizeof(client_broadcast_address));   
	             
	            printf("client broadcast message : %s : sent successfully\n", message);
	            sleep(1);
	        }
            if (state == 0)
            {
            	continue;
            }
            client_peer = 0;
            state = 10;
            close(client_broadcast_socket);
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
	        if (state == 5 || state == 10)
	        	write(1, "wait for your friend's selection...\n", 36);
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
	                 
	            //add new socket
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
	            if (valread <= 0)
	            {
	            	write(1, "your friend is disconnected!!!\n", 31);
	            	state = 0;
	            	//close(master_socket);
	            	start_again(map, &sock, &state, argv);
	            	continue;
	            }
				handle_server_side_of_game(buffer, valread, &state, sd, map);
				if (state == 30)
	    			break;	                  
	        }
	    } else{
	    	handle_game(&state, sock2, map, argv, &sock);
	    	if (state == 30)
	    		break;
	    }
    }     
    return 0;   
}   


