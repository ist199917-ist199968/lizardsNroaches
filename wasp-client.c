#include <zmq.h>
#include <ncurses.h>
#include "remote-char.h"
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
    remote_char_t m;
    m.msg_type = 6;
    m.ncock = nwasp;
    strcpy(m.password, password);
    zmq_send (requester, &m, sizeof(m), 0);
    zmq_recv (requester, &m, sizeof(m), 0);

    //board already full of cockroaches or wasps
    if(m.ncock == 0){
        printf("Tamos cheios :/\n");
        return 0;
    }

    //load the message information, m.ch is the first position
    m.msg_type = 7;

    int sleep_delay;
    direction_t direction;
    int n = 0;
    int move;
    while (1)
    {
        n++;
        sleep_delay = random()%700000;
        usleep(sleep_delay);

        for(int  i = 0; i < nwasp; i++){
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
        zmq_send (requester, &m, sizeof(m), 0);
        zmq_recv (requester, &m, sizeof(m), 0);
    }

    zmq_close (requester);
    zmq_ctx_destroy (context);
    free(candidate1);
	return 0;
}
