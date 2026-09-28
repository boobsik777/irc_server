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
#include <poll.h>

#define USERNAME_STRLEN 20
#define CHANNEL_NAME_STRLEN 255
#define MAX_CONNECTED 100

typedef struct client{
    int fd;
    char username[USERNAME_STRLEN];
    char ip[INET_ADDRSTRLEN];
    //char *password;
    //int is_registered;
    struct client *next;
}client;
client *clihead = NULL;

typedef struct channel{
    int member_count;
    struct client *members[MAX_CONNECTED];
    char chan_name[CHANNEL_NAME_STRLEN];
    struct channel *next;
}channel;
channel *chanhead = NULL;


int serverInit(int port);

channel* chanInit(const char *name);

void chanAdd(const char *name);

void chanList();

void chanDel(const char *name);

void chanFree();

void broadcastMessage(const char *chan_name, client *sender, const char *message);

client* clientAdd(const char *username, int fd, const char *ip);

void clientFree(); 

void clientList();

void clientDel(int fd); // finish removing from channel array

client* clientFind(int fd);





int main(void){
    int servfd = serverInit(6667);
    struct pollfd fds[MAX_CONNECTED];

    fds[0].fd = servfd;
    fds[0].events = POLLIN;

    int count = 1;

    if (fds[0].revents & POLLIN){
        struct sockaddr_in cli_addr;
        socklen_t addr_len = sizeof(cli_addr);
        
        int new_fd = accept(servfd, (struct sockaddr *)&cli_addr, &addr_len);

        if (new_fd >= 0 && count < MAX_CONNECTED){
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(cli_addr.sin_addr), ip_str, sizeof(ip_str));
            printf("Accepted connection from %s on FD %d\n", ip_str, new_fd);
            clientAdd("Guest", new_fd, ip_str);
            
            fds[count].fd = new_fd;
            fds[count].events = POLLIN;
            count++;
        }
    }

    return 0;
}





int serverInit(int port){
    int servfd = socket(AF_INET, SOCK_STREAM, 0);
    if (servfd == -1){
        fprintf(stderr, "serverInit(): Failed to open socket, return code %d\n", servfd);
        return servfd;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    
    int option = 1; 
    setsockopt(servfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(servfd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        perror("serverInit(): Bind failed!");
        close(servfd);
        return -1;
    };

    if (listen(servfd, 10) < 0) {
    perror("serverInit(): Listen failed");
    close(servfd);
    return -1;
    }
    return servfd;
}

channel* chanInit(const char *name){
    channel *chan = malloc(sizeof(channel));
    if (chan == NULL){
        fprintf(stderr, "chanInit(): Failed to allocate memory\n");     
        return NULL;
    }
    memset(chan, 0, sizeof(channel));
    snprintf(chan->chan_name, CHANNEL_NAME_STRLEN, "%s", name);
    chan->next = NULL;
    return chan;
}

void chanAdd(const char *name){
    channel *p = chanhead;
    while (p != NULL) {
        if (strcmp(p->chan_name, name) == 0) {
            printf("chanAdd(): Channel \"%s\" already exists\n", name);
            return;
        }
        p = p->next;
    }

    channel *new = chanInit(name);
    if (new == NULL){
        return;
    }
    new->next = chanhead;
    chanhead = new;
    
}

void chanList(){
    channel *p;
    if (chanhead == NULL){
        printf("chanList(): There are no channels\n");
        return;
    } 
    p = chanhead;
    printf("chanList(): Alive channels:\n");
    for (int i = 1; p != NULL; i++){
        printf("chanList(): %d. %s\n", i, p->chan_name);
        p = p->next;
    }
    return;
}

void chanDel(const char *name){
    channel *prev = NULL;
    channel *current = chanhead;
    while (current != NULL && strcmp(current->chan_name, name) != 0){
        prev = current;
        current = current->next;
    }

    if (current == NULL){
        printf("chanDel(): Channel \"%s\" doesn't exist\n", name);
        return;
    }
  
    if (prev == NULL) {
        chanhead = current->next;
    }else{
        prev->next = current->next;
    }

    printf("chanDel(): Deleted channel \"%s\"\n", name);
    
    for (int i = 0; i < MAX_CONNECTED; i++){
    current->members[i] = NULL;    
    }
    free(current);
    return;
}

void chanFree(){
    channel *current = chanhead;
    channel *next = NULL;
    while (current != NULL){
        next = current->next;

        for (int i = 0; i < MAX_CONNECTED; i++){
            if (current->members[i] != NULL){
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
    channel *chan = chanhead;
    while (chan != NULL && strcmp(chan->chan_name, chan_name) != 0) {
        chan = chan->next;
    }

    if (chan == NULL){
        if (sender != NULL){
            char err[128];
            snprintf(err, sizeof(err), "broadcastMessage(): Error: channel %s does not exist\n", chan_name);
            send(sender->fd, err, strlen(err), 0);
        }
        return;
    }

    char buffer[1024];

    char sender_name[USERNAME_STRLEN];
    if (sender != NULL) {
        snprintf(sender_name, sizeof(sender_name), "%s", sender->username);
    }else{
        snprintf(sender_name, sizeof(sender_name), "SYSTEM");
    }

    int msg_len = snprintf(buffer, sizeof(buffer), "[%s] %s: %s\n", chan->chan_name, sender_name, message);
    if (msg_len >= sizeof(buffer)) {
        msg_len = sizeof(buffer) - 1; // Prevent truncation overflows
    }

    int checked_members = 0;
    for (int i = 0; i < MAX_CONNECTED; i++) {
        // Optimization: stop iterating early if we've already processed all active channel members
        if (checked_members >= chan->member_count) {
            break;
        }

        client *member = chan->members[i];
        if (member != NULL) {
            checked_members++; // Count active member found

            if (sender == NULL){
                // System message: send to everyone in the channel
                send(member->fd, buffer, msg_len, 0);
            }else{
                // User message: send to everyone EXCEPT the sender
                if (member->fd != sender->fd){
                    send(member->fd, buffer, msg_len, 0);
                }
            }
        }
    }
    return;
    
}

client* clientAdd(const char *username, int fd, const char *ip){
    client *p = clihead;
    while (p != NULL){
        if (p->fd == fd || strcmp(p->username, username) == 0) {
            fprintf(stderr, "clientAdd(): Client with FD %d or username '%s' already exists\n", fd, username);
            return NULL;
        }
        p = p->next;
    }

    client *cli = malloc(sizeof(client));
    if (cli == NULL){
        fprintf(stderr, "clientAdd(): Failed to allocate memory\n");
        return NULL;
    }

    cli->fd = fd;
    snprintf(cli->username, USERNAME_STRLEN, "%s", username);
    
    if(ip != NULL){
        snprintf(cli->ip, sizeof(cli->ip), "%s", ip);
    }else{
        snprintf(cli->ip, sizeof(cli->ip), "0.0.0.0");
    }

    cli->next = clihead;
    clihead = cli;
    
    return cli;
}

void clientList(){

    client *p;
    if (clihead == NULL){
        printf("clientList(): There are no clients connected\n");
        return;
    } 
    p = clihead;
    printf("clientList(): Connected clients:\n");
    for (int i = 1; p != NULL; i++){
        printf("clientList(): %d. %s\n", i, p->username);
        p = p->next;
    }
    return;
}

void clientDel(int fd){
    client *prev = NULL;
    client *current = clihead;

    while(current != NULL && current->fd != fd){
        prev = current;
        current = current->next;
    }

    if(current == NULL){
        printf("clientDel(): Client with FD: %d doesn't exist\n", fd);
        return;
    }
        
    channel *chan = chanhead;
    while(chan != NULL){
        for(int i = 0; i < MAX_CONNECTED; i++){
            if(chan->members[i] == current){
                chan->members[i] = NULL;
                chan->member_count--;
                break;
            }
        }
        chan = chan->next;
    }

    if (prev == NULL) {
        clihead = current->next;
    } else {
        prev->next = current->next;
    }

    printf("clientDel(): Deleted client \"%s\", FD: %d\n", current->username, fd);
    close(current->fd);
    free(current);
    return;

}

void clientFree(){
    client *current = clihead;
    client *next = NULL;
    while (current != NULL){
        next = current->next;
        close(current->fd);
        free(current);
        current = next;
    }
    clihead = NULL;
    return;
}

client* clientFind(int fd){
    client* curr = clihead;
    while (curr != NULL){
        if (curr->fd == fd){
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}
