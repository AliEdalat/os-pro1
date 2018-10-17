#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h>
#include <fcntl.h>
#define PORT 8884

typedef struct partner Partner;

struct partner
{
    char ip[1025];
    char port[1025];
    char name[1025];
};

int mystrcmp(char* a, char* b, int len_a, int len_b, int size){
    printf("ffff\n");
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
        for (j = 0; j < 10; ++j)
        {
            read(fd, &(map[i][j]), 1);
        }
    }
}
   
int main(int argc, char const *argv[]) 
{ 
    int map[10][10];
    struct sockaddr_in address; 
    int sock = 0, valread, state = 0; 
    struct sockaddr_in serv_addr; 
    char *hello = "127.0.0.1 1234 ali"; 
    char buffer[1024] = {0};
    Partner* partner;
    fetch_map(map, "map.txt");
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("\n Socket creation error \n"); 
        return -1; 
    } 
   
    memset(&serv_addr, '0', sizeof(serv_addr)); 
   
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(atoi(argv[1])); 
 
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    { 
        printf("\nConnection Failed \n"); 
        return -1; 
    }

    while(1){
        if (state == 0) {
            send(sock, hello, strlen(hello), 0);
            state++;
        } else if (state == 1) {
            valread = read( sock , buffer, 1024); 
            printf("%s\n",buffer );
            char partner_text[10] = "partner: ";
            char sel_text[6] = "sel: ";
            if (mystrcmp(buffer, partner_text, 1024, 9, 9))
                state++;
            else if (mystrcmp(buffer, sel_text, 1024, 5, 5))
                state = 5;
        } else if (state == 2) {
            printf("herererererer\n");
            partner = extract_partner(buffer, sizeof(buffer));
            char buffer[1024] = {0};
            memset(&serv_addr, '0', sizeof(serv_addr)); 
   
            serv_addr.sin_family = AF_INET; 
            serv_addr.sin_addr.s_addr = INADDR_ANY;
            serv_addr.sin_port = atoi(partner->port); 
         
           
            if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
            { 
                printf("\nConnection Failed \n"); 
                return -1; 
            }

            state++;
        } else if (state == 3) {
            char input[3];
            write(1, "select :\n", 9);
            read(0, input, 3);
            char message[9] ={'s', 'e', 'l', ':', ' ', input[0], input[1], input[2], '\0'};
            send(sock, message, 9, 0);
            state++;
        } else if (state == 4) {
            char buffer[1024] = {0};
            valread = read( sock, buffer, 1024); 
            printf("%s\n", buffer);
            char res_message[5] ={'r', 'e', 's', ':', ' '};
            char win_message[9] ={'r', 'e', 's', ':', ' ', 'w', 'i', 'n', '\0'};
            if (mystrcmp(buffer, win_message, 1024, 9, 9))
                break;
            else if (mystrcmp(buffer, res_message, 1024, 5, 5)){
                int res = extract_result(buffer);
                if (res)
                    state = 3;
                else
                    state = 5;
            }
        } else if (state == 5) {
            char buffer[1024] = {0};
            valread = read( sock , buffer, 1024); 
            printf("%s\n",buffer );
            char sel_text[5] = {'s', 'e', 'l', ':', ' '};
            char zero_message[7] ={'r', 'e', 's', ':', ' ', '0', '\0'};
            char one_message[7] ={'r', 'e', 's', ':', ' ', '1', '\0'};
            char win_message[9] ={'r', 'e', 's', ':', ' ', 'w', 'i', 'n', '\0'};
            if (mystrcmp(buffer, sel_text, 1024, 5, 5)){
                int val = extract_sel(buffer, map);
                if (is_map_clear(map)){
                    send(sock, win_message, 9, 0);
                    write(1, "lost\n", 5);
                    break;
                }
                else if (val){
                    send(sock, one_message, 7, 0);
                    state = 5;
                }
                else{
                    send(sock, one_message, 7, 0);
                    state = 3;
                }
            }
        }
    }
    return 0; 
} 

