#include <zmq.h>
#include <ncurses.h>
#include "message.pb-c.h"
#include "remote-char.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

ProtoCharMessage m;
void *requester;

void sigintHandler(int signum) {
    m.msg_type=8;
    size_t packed_size = proto_char_message__get_packed_size(&m);
    uint8_t* packed_buffer = malloc(packed_size);
    proto_char_message__pack(&m, packed_buffer);
    zmq_send (requester, packed_buffer, packed_size, 0);
    free(packed_buffer);
    packed_buffer=NULL;
    ProtoCharMessage *recvm=NULL;
    zmq_msg_t zmq_msg;
    zmq_msg_init (&zmq_msg);

    packed_size=zmq_recvmsg(requester, &zmq_msg,0);
    packed_buffer = zmq_msg_data(&zmq_msg);
    recvm = proto_char_message__unpack(NULL, packed_size, packed_buffer);
    exit(signum);

}

int main(int argc, char *argv[]){	 
    srand((unsigned int) time(NULL));

    if (argc != 3) {
        printf("Wrong number number of arguments\n");
        return 1;
    }

    char *ip = argv[1];
    char *port1 = argv[2];

    if(!Is_ValidIPv4(ip) || !Is_ValidPort(port1)){
        printf("ERROR[000]: Incorrect Host-Server data. Check host IPv4 and TCP ports.");
        exit(1);
    }

    char *candidate1 = (char*) calloc (1, sizeof(char)*(strlen("tcp://")+strlen(ip) + 1 + strlen(port1) + 1));
    strcat(candidate1, "tcp://");
    strcat(candidate1, ip);
    strcat(candidate1, ":");
    strcat(candidate1, port1);
   
    void *context = zmq_ctx_new ();
    requester = zmq_socket (context, ZMQ_REQ);
    zmq_connect (requester, candidate1);

    // read number of wasps from the user
    int nwasp;
    do {
        printf("How many wasps (1-10)?: ");
        if (scanf("%d", &nwasp) != 1) {
            printf("Invalid input. Please enter a number between 1 and 10.\n");
            while (getchar() != '\n'); // Clear input buffer
            continue;
        }
    } while (nwasp < 1 || nwasp > 10);

    char password[50];
    for(int i = 0; i < 50; i++){
        password[i] = (char) random()%94 + 32;
    }
    password[49]='\0';

    // send connection message
        
    proto_char_message__init(&m); 
    m.ch=malloc(sizeof(char));
    m.password=malloc(50*sizeof(char));
    m.cockdir=malloc(10*sizeof(ProtoDirection));
    m.n_cockdir=10;
    m.msg_type = 6;
    m.ncock = nwasp;
    strcpy(m.password, password);
    
    size_t packed_size = proto_char_message__get_packed_size(&m);
    uint8_t* packed_buffer = malloc(packed_size);
    proto_char_message__pack(&m, packed_buffer);
    zmq_send (requester, packed_buffer, packed_size, 0);
    free(packed_buffer);
    packed_buffer=NULL;
    ProtoCharMessage *recvm=NULL;
    zmq_msg_t zmq_msg;
    zmq_msg_init (&zmq_msg);
    supressSIGINT();
    packed_size=zmq_recvmsg(requester, &zmq_msg,0);
    packed_buffer = zmq_msg_data(&zmq_msg); 
    recvm = proto_char_message__unpack(NULL, packed_size, packed_buffer);
    

    //board already full of cockroaches or wasps
    if(recvm->ncock == 0){
        printf("Tamos cheios :/\n");
        return 0;
    }

    *(m.ch) = *(recvm->ch);
    proto_char_message__free_unpacked(recvm, NULL);
    recvm=NULL;
    allowSIGINT();
    //load the message information, m.ch is the first position
    m.msg_type = 7;

    int sleep_delay;
    ProtoDirection direction;
    int n = 0;
    int move;
    while (1)
    {
        //printf("Entered movement section\n");
        n++;
        sleep_delay = random()%700000;
        usleep(sleep_delay);

        for(int  i = 0; i < nwasp; i++){
            move = random()%2;
            if(move == 1){
            direction = random()%(RIGHT+1);

            switch (direction)
            {
            case LEFT:
                printf("%d Going Left   \n", n);
                m.cockdir[i] = direction;
                break;
            case RIGHT:
                printf("%d Going Right   \n", n);
                m.cockdir[i] = direction;
                break;
            case DOWN:
                printf("%d Going Down   \n", n);
                m.cockdir[i] = direction;
                break;
            default:
            case UP:
                printf("%d Going Up    \n", n);
                m.cockdir[i] = direction;
                break;
            }
            }else{
                m.cockdir[i] = 5;
                printf("Cagada\n");
            }
        }
        refresh();
        //send the movement message
        supressSIGINT();
        packed_size = proto_char_message__get_packed_size(&m);
        packed_buffer = malloc(packed_size);
        proto_char_message__pack(&m, packed_buffer);
        zmq_send (requester, packed_buffer, packed_size, 0);
        free(packed_buffer);
        packed_buffer=NULL;
   
        packed_size=zmq_recvmsg(requester, &zmq_msg,0);
        packed_buffer = zmq_msg_data(&zmq_msg);
        recvm=proto_char_message__unpack(NULL, packed_size, packed_buffer);
        proto_char_message__free_unpacked(recvm, NULL);
        recvm=NULL;
        allowSIGINT();

    }

    zmq_close (requester);
    zmq_ctx_destroy (context);
    free(candidate1);
	return 0;
}
