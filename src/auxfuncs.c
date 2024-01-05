#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include "message.pb-c.h"
#include "remote-char.h"

bool Is_ValidIPv4(const char *candidate){
    in_addr_t ip;
    if (inet_pton(AF_INET, candidate, &ip) == 1) /*Converting text to binary*/
        return true;
    return false;
}

bool Is_ValidPort(const char *candidate){
    int port = atoi(candidate);
    int len = strlen(candidate);
    for (int i = 0; i < len; i++){
        if (candidate[i] < '0' || candidate[i] > '9')
            return false;
    }

    if (port >= 1023 && port <= 65535) /*Port 0 is reserved for stdin, max TCP/UDP port is 65535*/
        return true;
    return false;
}


node_t* head[2];
node_t* tail[2];

void initializeQueues() {
    // Initializing each queue to NULL
    head[0] = NULL;
    head[1] = NULL;
    tail[0] = NULL;
    tail[1] = NULL;
}

void enqueue(ProtoCharMessage *msg, int id){
    node_t *newnode = malloc(sizeof(node_t));
    newnode->msg = msg;
    newnode->next = NULL;
    if(tail[id] == NULL){
        head[id] = newnode;
    } else{
        tail[id]->next = newnode;
    }
    tail[id] = newnode;
}

ProtoCharMessage *dequeue(int id){
    if(head[id] == NULL){
        return NULL;
    }
    else {
        ProtoCharMessage *result = head[id]->msg;
        node_t *temp = head[id];
        head[id] = head[id]->next;
        if(head[id] == NULL){
            tail[id] = NULL;
        }
        free(temp);
        return result;
    }
}