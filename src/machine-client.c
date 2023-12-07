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

int main()
{	 
    srand((unsigned int) time(NULL));
    void *context = zmq_ctx_new ();
    void *requester = zmq_socket (context, ZMQ_REQ);
    zmq_connect (requester, "tcp://127.0.0.1:5610");

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
    // send connection message
    remote_char_t m;
    m.msg_type = 3;
    m.ncock = ncock;
    zmq_send (requester, &m, sizeof(m), 0);
    zmq_recv (requester, &m, sizeof(m), 0);

    //board already full of cockroaches
    if(m.ncock == 0){
        printf("Tamos cheios :/\n");
        return 0;
    }

    //load the message information, m.ch is the first position
    m.msg_type = 4;

    int sleep_delay;
    direction_t direction;
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
    endwin();			/* End curses mode		  */
	return 0;
}