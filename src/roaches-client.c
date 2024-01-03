#include <zmq.h>
#include <ncurses.h>
#include "remote-char.h"
#include "message.pb-c.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

int main(int argc, char *argv[])
{	 
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
    void *requester = zmq_socket (context, ZMQ_REQ);
    zmq_connect (requester, candidate1);

    // read number of cockroaches from the user
    int ncock;
    do {
        printf("How many cockroaches (1-10)?: ");
        if (scanf("%d", &ncock) != 1) {
            printf("Invalid input. Please enter a number between 1 and 10.\n");
            while (getchar() != '\n'); // Clear input buffer
            continue;
        }
    } while (ncock < 1 || ncock > 10);

    char password[50];
    for(int i = 0; i < 50; i++){
        password[i] = (char) random()%94 + 32;
    }
    password[49]='\0';

    // send connection message
    ProtoCharMessage m;
    m.msg_type = 3;
    m.ncock = ncock;
    strcpy(m.password, password);
    
    size_t packed_size = proto_char_message__get_packed_size(&m);
    uint8_t* packed_buffer = malloc(packed_size);
    proto_char_message__pack(&m, packed_buffer);
    zmq_send (requester, packed_buffer, packed_size, 0);
    free(packed_buffer);
    packed_buffer=NULL;
   
    zmq_msg_t zmq_msg;
    zmq_msg_init (&zmq_msg);
    packed_size=zmq_recvmsg(requester, &zmq_msg,0);
    packed_buffer = zmq_msg_data(&zmq_msg);
    ProtoCharMessage *recvm=proto_char_message__unpack(NULL, packed_size, packed_buffer);

    //board already full of cockroaches
    if(recvm->ncock == 0){
        printf("Tamos cheios :/\n");
        return 0;
    }

    //load the message information, m.ch is the first position
    m.msg_type = 4;

    int sleep_delay;
    ProtoDirection direction;
    int n = 0;
    int move;
    while (1)
    {
        n++;
        sleep_delay = random()%700000;
        usleep(sleep_delay);

        for(int  i = 0; i < ncock; i++){
            move = random()%2;
            if(move == 1){
            direction = random()%4;

            switch (direction)
            {
            case LEFT:
                mvprintw(0,0,"%d Going Left   \n", n);
                m.cockdir[i] = direction;
                break;
            case RIGHT:
                mvprintw(0,0,"%d Going Right   \n", n);
                m.cockdir[i] = direction;
                break;
            case DOWN:
                mvprintw(0,0,"%d Going Down   \n", n);
                m.cockdir[i] = direction;
                break;
            default:
            case UP:
                mvprintw(0,0,"%d Going Up    \n", n);
                m.cockdir[i] = direction;
                break;
            }
            }else{
                m.cockdir[i] = 5;
            }
        }
        refresh();
        //send the movement message
        packed_size = proto_char_message__get_packed_size(&m);
        packed_buffer = malloc(packed_size);
        proto_char_message__pack(&m, packed_buffer);
        zmq_send (requester, packed_buffer, packed_size, 0);
        free(packed_buffer);
        packed_buffer=NULL;
   
        packed_size=zmq_recvmsg(requester, &zmq_msg,0);
        packed_buffer = zmq_msg_data(&zmq_msg);
        recvm=proto_char_message__unpack(NULL, packed_size, packed_buffer);

    }

    zmq_close (requester);
    zmq_ctx_destroy (context);
    free(candidate1);
	return 0;
}
