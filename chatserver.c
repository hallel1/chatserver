
#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <ctype.h>


#define TRUE   1
#define NUM_ARGC 2
#define PORT_INDEX 1
#define BUFLEN_ASSUMPTION 4096
#define QLEN 5//maximum number of client connections
#define MAX_NUM_SD_DIGITS 10
#define LEN_START_MSG 7;//"guest<sd>: " - 7 chars without sd


//list:
#define slist_head(l) l->head
#define slist_tail(l) l->tail
#define slist_size(l) l->size
#define slist_next(n) n->next
#define slist_data(n) n->data

struct slist_node
{
    int data; // Pointer to data of this node
    struct slist_node *next; // Pointer to next node on list
};
typedef struct slist_node slist_node_t;

struct slist
{
    slist_node_t *head; // Pointer to head of list
    slist_node_t *tail; // Pointer to tail of list
    unsigned int size; // The number of elements in the list
};

typedef struct slist slist_t;


//fuctions of list
slist_node_t* createNode(int data, slist_node_t* next);
void slist_init(slist_t *);
void slist_destroy(slist_t *);
int slist_pop_first(slist_t *);
int slist_append(slist_t *,int);
void pop_by_sd(slist_t * l,int sd);
//other function:
void errorInput(char *msg);
void errorMsg(char *msg);
int stringToInt(char *str);

typedef void (*sighandler_t)(int);
sighandler_t signal(int,sighandler_t);
void handler (int signum);

slist_t *clientList;//global
int master_socket;

int main(int argc , char *argv[]) {


    int  addrlen , new_socket ,checkSelect , nRead,nReadLine , sd,indexRead,flag;
    int max_sd;
    struct sockaddr_in serv_addr;

    char *newLine;
    char bufferTmp[BUFLEN_ASSUMPTION];
    char buffer[BUFLEN_ASSUMPTION];

    int buferToSendLen=MAX_NUM_SD_DIGITS+LEN_START_MSG;
    buferToSendLen+=BUFLEN_ASSUMPTION;
    char bufferToSend[buferToSendLen];
    char bufferSD[MAX_NUM_SD_DIGITS];
    int actuallyLen;
    strcpy(buffer,"\0");
    strcpy(bufferTmp,"\0");
    strcpy(bufferToSend,"\0");
    strcpy(bufferSD,"\0");


    if (argc != NUM_ARGC) {
        errorInput("Usage: ./<name file> <port>\n");
    }

  
    fd_set readfds;
    fd_set writefds;


    
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)
    {
        errorMsg("socket");
    }


    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons((uint16_t) stringToInt(argv[PORT_INDEX]));

   
    if (bind(master_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0){
        errorMsg("bind");
    }

    if (listen(master_socket, QLEN) < 0){
        errorMsg("listen");
    }

    
    addrlen = sizeof(serv_addr);

    clientList =(slist_t*)malloc(sizeof(slist_t));
    if(clientList==NULL){
        return -1;
    }
    slist_init(clientList);
    signal(SIGINT,handler);



    while(TRUE){

        FD_ZERO(&readfds);
        FD_ZERO(&writefds);

        FD_SET(master_socket, &readfds);
        FD_SET(master_socket, &writefds);
        max_sd = master_socket;


        slist_node_t * nodeTemp= slist_head(clientList);//temp pointer for list
        slist_node_t * nodeTemp2;//temp pointer for list
        slist_node_t * nodeTempClose;//temp pointer for list

        while(nodeTemp!=NULL){

            //socket descriptor
            sd = slist_data(nodeTemp);

            if(sd > 0){
                FD_SET( sd , &readfds);
                FD_SET( sd , &writefds);
            }


            if(sd > max_sd){
                max_sd = sd;
            }
            nodeTemp=slist_next(nodeTemp);
        }


        checkSelect = select( max_sd + 1 , &readfds , &writefds , NULL , NULL);
        if ((checkSelect < 0) && (errno!=EINTR)){
            free(clientList);
            errorMsg("select");
        }

        if (FD_ISSET(master_socket, &readfds))
        {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&serv_addr, (socklen_t*)&addrlen))<0)
            {
                free(clientList);
                perror("accept");
                exit(EXIT_FAILURE);
            }

            slist_append(clientList,new_socket);

        }


        nodeTemp= slist_head(clientList);//temp pointer for list

        while(nodeTemp!=NULL){

            sd = slist_data(nodeTemp);
            flag=0;
            if (FD_ISSET( sd , &readfds) )
            {

                if((nRead = (int) recv(sd, bufferTmp, BUFLEN_ASSUMPTION - 1, MSG_PEEK)) < 0){
                    free(clientList);
                    errorMsg("recv");
                }
                if(nRead==0){
                    nodeTempClose=slist_next(nodeTemp);
                    close( sd );
                    pop_by_sd(clientList,sd);//remove this node from the list
                    flag=1;
                    nodeTemp=nodeTempClose;
                }
                else
                {

                    printf("Server is ready to read from socket %d\r\n", sd);
                    newLine=strstr(bufferTmp,"\r\n");
                    if(newLine!=NULL){
                        indexRead= ((int)(newLine-bufferTmp));
                        indexRead+=2;
                        nReadLine= (int) recv(sd , buffer, (size_t) indexRead, 0);
                    }
                    else{
                        nReadLine= (int) recv(sd , buffer, BUFLEN_ASSUMPTION - 1, 0);
                    }


                    bzero(bufferSD,MAX_NUM_SD_DIGITS);

                    strcat(bufferToSend,"guest");

                    sprintf(bufferSD,"%d",sd);

                    strcat(bufferToSend,bufferSD);
                    strcat(bufferToSend,": ");

                    strncat(bufferToSend, buffer, (size_t) nReadLine);
                    bufferToSend[strlen(bufferToSend)]='\0';



                    nodeTemp2= slist_head(clientList);//temp pointer for list

                    while(nodeTemp2!=NULL) {
                        sd = slist_data(nodeTemp2);
                        actuallyLen = (int) strlen(bufferToSend);
                        if (FD_ISSET(sd, &writefds) && (actuallyLen != 0) )
                        {
                            printf("Server is ready to write to socket %d\r\n", sd);
                            write(sd, bufferToSend, strlen(bufferToSend));
                        }
                        nodeTemp2 = slist_next(nodeTemp2);

                    }
                    bzero(buffer,BUFLEN_ASSUMPTION);
                    bzero(bufferTmp,BUFLEN_ASSUMPTION);
                    bzero(bufferToSend,strlen(bufferToSend));
                    strcpy(bufferToSend,"");
                }
            }

            if(flag==0)
                nodeTemp=slist_next(nodeTemp);

        }

    }
    return 0;
}
/****************************************************
***********f u n c t i o n   a r e a*****************
*****************************************************/
void errorMsg(char *msg)  {  //exit with messege

    perror(msg);
    exit(1);

}
//-----------------------------------------------------
void errorInput(char *msg) { //input error

    fprintf(stderr, "%s", msg );
    exit(1);
}
//-----------------------------------------------------
int stringToInt(char *str){//check if the input str is a integer - if yes return the number else call errorInput
    int i=0;

    for(;i<strlen(str);i++){

        if( !(isdigit(str[i])) ){
            errorInput("wrong input\n");
        }

    }
    return atoi(str);


}
//-----------------------------------------------------
void handler (int signum){//catch the signal: ^c in order to release the list

    slist_destroy(clientList);
    
    
    free(clientList);
    close(master_socket);
    exit(EXIT_SUCCESS);


}
//-----------------------------------------------------

slist_node_t* createNode(int data, slist_node_t* next){//create node and insrert his values

    slist_node_t* node = (slist_node_t *)malloc(sizeof(slist_node_t));
    if(node!=NULL){
        slist_data(node)= data;
        slist_next(node)=next;
    }
    return node;
}

//-----------------------------------------------------------------------
/** Initialize a single linked list
	\param list - the list to initialize */
void slist_init(slist_t * l){
    if(l==NULL)
        return;
    slist_head(l)=NULL;
    slist_tail(l)=NULL;
    slist_size(l)=0;
}
//-----------------------------------------------------------------------

/** Destroy and de-allocate the memory hold by a list
	\param list - a pointer to an existing list
	\param dealloc flag that indicates whether stored data should also be de-allocated */

void slist_destroy(slist_t * l){


    if(l==NULL){
        return;
    }

    while(slist_head(l)!=NULL){
    	close(slist_data(slist_head(l)));
        slist_pop_first(l);

    }


}
//-----------------------------------------------------------------------
/** Append data to list (add as last node of the list)
	\param list - a pointer to a list
	\param data - the data to place in the list
	\return 0 on success, or -1 on failure */
int slist_append(slist_t * l,int data){
    if(l==NULL)
        return -1;
    slist_node_t* newNode= createNode(data, NULL);
    if(newNode==NULL)
        return -1;
    slist_node_t * nodeTemp;

    if(slist_size(l)==0){
        slist_head(l)=newNode;
        slist_tail(l)=newNode;
    }else{
        nodeTemp = slist_tail(l);//pointer to the tail of the list
        slist_next(nodeTemp)=newNode;
        slist_tail(l)=slist_next(nodeTemp);

    }

    nodeTemp=NULL;
    slist_size(l)++;//increase list size

    return 0;


}

//-----------------------------------------------------------------------
/** Pop the first element in the list
	\param list - a pointer to a list
	\return a pointer to the data of the element, or -1 if the list is empty */
int slist_pop_first(slist_t * l){

    if(l==NULL || slist_head(l)==NULL)
        return -1;
    slist_node_t * nodeTemp= slist_head(l);
    int data = slist_data(nodeTemp);
    slist_head(l)=slist_next(slist_head(l));
    free(nodeTemp);
    
    if(slist_head(l)==NULL)
        slist_tail(l)=NULL;
    slist_size(l)--;
    return data;
}
//-----------------------------------------------------------------------

/** Pop element by sd from the list**/
void pop_by_sd(slist_t * l,int sd){


    if(l==NULL || slist_head(l)==NULL)
        return;
    slist_node_t * nodeTemp= slist_head(l);
    slist_node_t * curNode= slist_head(l);

    if(slist_data(nodeTemp)==sd){//if need to free the first node

        slist_pop_first(l);
        return;
    }

    while(slist_next(nodeTemp)!=NULL ){
        if( slist_data(slist_next(nodeTemp))==sd){

            if(slist_next(slist_next(nodeTemp))==NULL)
                slist_tail(l)=nodeTemp;
            curNode=slist_next(nodeTemp);
            slist_next(nodeTemp)=slist_next(slist_next(nodeTemp));
            slist_size(l)--;
            free(curNode);
            

            return;

        }
        nodeTemp=slist_next(nodeTemp);
    }




}

