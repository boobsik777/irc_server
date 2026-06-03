#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#define USERNAME_STRLEN 20
#define CHANNEL_NAME_STRLEN 255
#define MAX_CONNECTED 100

typedef struct client{
    int fd;
    char *username;
    //char *password;
    //int is_registered;
    struct client *next;

}client;

client *clhead = NULL;

typedef struct channel{
    int member_count;
    struct client *members[MAX_CONNECTED];
    char chan_name[CHANNEL_NAME_STRLEN];
    struct channel *next;
}channel;

channel *chanhead = NULL;

int initServer(int port);
channel* initChannel(const char *name);
void addChannel(const char *name);
void listChan();
void deleteChannel(const char *name);
void memfree();
void broadcastMessage(const char *chan_name, client *sender, const char *message);





int main(void){

    initServer(6667);
    addChannel("chan1");
    listChan();
    printf("----\n");
    addChannel("chan2");
    listChan();

    char buff[1024];
    size_t numbytes;

    memfree();

    return 0;
}

int initServer(int port){
    int servfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    
    if(servfd == -1) return servfd;

    int option = 1; 
    setsockopt(servfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(servfd, (struct sockaddr *)&addr, sizeof(addr));
    listen(servfd, 10);
    return servfd;
}

channel* initChannel(const char *name){
    channel *chan = malloc(sizeof(channel));
    if (chan == NULL){
        fprintf(stderr, "Failed to allocate memory\n");
        return NULL;
    }
    memset(chan, 0, sizeof(channel));
    strlcpy(chan->chan_name, name, CHANNEL_NAME_STRLEN);
    chan->next = NULL;
    return chan;
}

void addChannel(const char *name){
    channel *new = initChannel(name);
    if(chanhead == NULL) {
        chanhead = new;
    }else{
        channel *p = chanhead;
        while(p->next != NULL){
            p = p->next;
        }
        p->next = new;
    }
}

void listChan(){
    channel *p;
    if (chanhead == NULL){
        printf("There are no channels\n");
        return;
    } 
    p = chanhead;
    for(int i = 1; p != NULL; i++){
        printf("%d. %s\n", i, p->chan_name);
        p = p->next;
    }
    return;
}

void deleteChannel(const char *name){
    channel *prev = NULL;
    channel *current = chanhead;
    while(current != NULL){
        if(strcmp(current->chan_name, name) == 0){
            if (prev == NULL) {
                chanhead = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            return;
        }else{
            if(current->next == NULL){
                printf("Channel \"%s\" doesn't exist\n", name);
                return;
            }
            prev = current;
            current = current->next;
        }
    }
}

void memfree(){
    channel *current = chanhead;
    channel *next;
    while(current != NULL){
        next = current->next;

        for(int i = 0; i < MAX_CONNECTED; i++){
            if(current->members[i] != NULL){
                current->members[i] = NULL;
            };
        }
        free(current);
        current = next;
    }
    chanhead = NULL;
    return;
}

void broadcastMessage(const char *chan_name, client *sender, const char *message){
    channel *p = chanhead;
    while(p != NULL){
        if(strcmp(chan_name, p->chan_name) == 0){
            break;
        }
        p = p->next;
    }

    if(p == NULL){
        char err[128];
        snprintf(err, sizeof(err), "Error: channel %s does not exist\n", chan_name);
        send(sender->fd, err, strlen(err), 0);
        return;
    }

    char buffer[512];
    snprintf(buffer, sizeof(buffer), "[%s] %s : %s\n", p->chan_name, sender->username, message);

    for(int i = 0; i<MAX_CONNECTED; i++){
        client *member = p->members[i];
        if(member != NULL && member->fd != sender->fd)
        send(member->fd, buffer, strlen(buffer), 0);
    }
    return;
    
}

//to check later
client* addClient(const char *username, int fd){
    client *cli = malloc(sizeof(client));
    if (cli == NULL){
        fprintf(stderr, "Failed to allocate memory\n");
        return NULL;
    }
    memset(cli, 0, sizeof(channel));
    strlcpy(cli->username, username, USERNAME_STRLEN);
    cli->next = NULL;
    return cli;
}
