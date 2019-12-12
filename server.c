#include <stdio.h>          
#include <strings.h>
#include <string.h>         
#include <unistd.h>          
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <pthread.h> 
#include <stdlib.h>

#define PORT 1234    
#define BACKLOG 1 
#define Max 5
#define MAXSIZE 1024

struct userinfo {
    char id[100];
    int playwith;
};

int fdt[Max]={0};
char mes[1024];
int SendToClient(int fd,char* buf,int Size);
struct userinfo users[100];
int find_fd(char *name);
int win_dis[8][3] = {{0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6}};


void message_handler(char *mes, int sender)
{
    int instruction = 0;
    sscanf (mes,"%d",&instruction);
    switch (instruction) {
        case 1: {
            char name[100];
            sscanf (mes,"1 %s",name);
            strncpy (users[sender].id,name,100);
            send(sender,"1",1,0);
            printf("[1]加入使用者:%s\n",name);

            break;
        }
        case 2:{
            char buf[MAXSIZE],tmp[100];
            int p = sprintf(buf,"2 ");
            for (int i=0;i<100;i++)
                if (strcmp(users[i].id,"")!=0){
                    sscanf(users[i].id,"%s",tmp);
                    p = sprintf(buf+p,"%s ", tmp) + p;
                }
            printf("[2]顯示:%s\n",buf);
            send(sender,buf,strlen(buf),0);
            
            break;
        }
        case 3:{
            char a[100],b[100];
            char buf[MAXSIZE];
            sscanf (mes,"3 %s %s",a,b);
            int b_fd = find_fd(b);
            sprintf(buf, "4 %s 邀請您加入遊戲. 是否接受?\n", a);
            send(b_fd, buf, strlen(buf), 0);
            printf("[3]:[%s] 接受 [%s].\n",a,b);

            break;
        }
        case 5:{
            int state;
            char inviter[100];
            sscanf(mes, "5 %d %s", &state, inviter);
            if (state==1){
                send(sender, "6\n", 2, 0);
                send(find_fd(inviter), "6\n", 2, 0);
                int fd = find_fd(inviter);
                users[sender].playwith = fd;
                users[fd].playwith = sender;
                printf("[5]:開始新的一局.\n");
            }
            break;
        }
        case 7:{
            int board[9];
            char state[100];
            char buf[MAXSIZE];
            sscanf(mes, "7  %d %d %d %d %d %d %d %d %d",&board[0],&board[1],&board[2],&board[3],&board[4],&board[5],&board[6],&board[7],&board[8]);
            
            for (int i=0;i<100;i++)
                state[i] = '\0';
            memset(buf,'\0',MAXSIZE);
            memset(state,'\0',sizeof(state));
            strcat(state, users[sender].id);
            
            for (int i = 0; i < 8;i++)  {
                if (board[win_dis[i][0]]==board[win_dis[i][1]] && board[win_dis[i][1]]==board[win_dis[i][2]]) {
                    if (board[win_dis[i][0]]!=0) {
                    strcat(state, "獲勝!\n");
                    sprintf (buf,"8  %d %d %d %d %d %d %d %d %d %s\n",board[0],board[1],board[2],board[3],board[4],board[5],board[6],board[7],board[8],state);
                    printf ("[7]:%s",buf);
                    send(sender,buf,sizeof(buf),0);
                    send(users[sender].playwith,buf,sizeof(buf),0);
                    return;
                    }
                }
            }
            memset(buf,'\0',MAXSIZE);
            memset(state,'\0',sizeof(state));
            
            for (int i = 0; i < 9;i++) {
                if (i==8) {
                    strcat(state, "平局!\n");
                    sprintf (buf,"8  %d %d %d %d %d %d %d %d %d %s\n",board[0],board[1],board[2],board[3],board[4],board[5],board[6],board[7],board[8],state);
                    printf ("[7]:%s",buf);
                    send(sender,buf,sizeof(buf),0);
                    send(users[sender].playwith,buf,sizeof(buf),0);
                    return;
                }
                if (board[i]== 0)
                    break;
            }

            memset(buf,'\0',MAXSIZE);
            memset(state,'\0',sizeof(state));
            strcat(state, users[users[sender].playwith].id);
            strcat(state, "換你了!\n");
            sprintf (buf,"8  %d %d %d %d %d %d %d %d %d %s\n",board[0],board[1],board[2],board[3],board[4],board[5],board[6],board[7],board[8],state);
            printf ("[7]:%s",buf);
            send(sender,buf,sizeof(buf),0);
            send(users[sender].playwith,buf,sizeof(buf),0);
            break;
        }
    }
}

void *pthread_service(void* sfd){
    int fd=*(int *)sfd;
    while(1){
        int numbytes;
        int i;
        numbytes=recv(fd,mes,MAXSIZE,0);
        printf ("[Get]:%s",mes);

        if(numbytes<=0){
            for(i=0;i<Max;i++){
                if(fd==fdt[i]){
                    fdt[i]=0;               
                }
            }
            memset(users[fd].id,'\0',sizeof(users[fd].id));
            users[fd].playwith = -1;
            break;
        }

        message_handler(mes,fd);
        bzero(mes,MAXSIZE);
    }
    close(fd);
}

int  main()  
{ 
    int listenfd, connectfd;    
    struct sockaddr_in server; 
    struct sockaddr_in client;      
    int sin_size; 
    sin_size=sizeof(struct sockaddr_in); 
    int number=0;
    int fd;
    
    for (int i=0;i<100;i++){
        for (int j=0;j<100;j++)
            users[i].id[j] ='\0';
        users[i].playwith = -1;
    }

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {   
        perror("Creating socket failed.");
        exit(1);
    }


    int opt = SO_REUSEADDR;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bzero(&server,sizeof(server));  

    server.sin_family=AF_INET; 
    server.sin_port=htons(PORT); 
    server.sin_addr.s_addr = htonl (INADDR_ANY); 

    if (bind(listenfd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) { 
        perror("Bind error.");
        exit(1); 
    }   

    if(listen(listenfd,BACKLOG) == -1)  {  
        perror("listen() error\n"); 
        exit(1); 
        } 
    printf("Waiting for client....\n");

    while(1)    {
        if ((fd = accept(listenfd,(struct sockaddr *)&client,&sin_size))==-1) {
        perror("accept() error\n"); 
        exit(1); 
        }

        if(number>=Max){
            printf("no more client is allowed\n");
            close(fd);
        }

        int i;
        for(i=0;i<Max;i++){
            if(fdt[i]==0){
                fdt[i]=fd;
                break;
            }
        }
        pthread_t tid;
        pthread_create(&tid,NULL,(void*)pthread_service,&fd);
        number=number+1;
    }
    close(listenfd);            
}

int find_fd (char *name) {
    for (int i=0;i<100;i++)
        if (strcmp(name,users[i].id)==0)
            return i;
    return -1;
}
