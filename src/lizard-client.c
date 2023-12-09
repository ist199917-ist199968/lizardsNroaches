#include <zmq.h>
#include <ncurses.h>
#include "remote-char.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  
#include <ctype.h> 
#include <stdlib.h>
#include <string.h>

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


    // read the character from the user
    char ch;
    do{
        printf("what is your character(a..z)?: ");
        ch = getchar();
        ch = tolower(ch);  
    }while(!isalpha(ch));

    char password[50];
    for(int i = 0; i < 50; i++){
        password[i] = (char) random()%94 + 32;
    }

    // send connection message
    remote_char_t m;
    m.msg_type = 0;
    m.ch = ch;
    m.direction = 0;
    strcpy(m.password, password);
    zmq_send (requester, &m, sizeof(m), 0);
    
    //receive the assigned letter from the server or flag that is full
    zmq_recv (requester, &m, sizeof(m), 0);
    ch = m.ch;
    if(ch == 0){
        printf("Tamos cheios :/\n");
        return 0;
    }
	initscr();			/* Start curses mode 		*/
	cbreak();				/* Line buffering disabled	*/
	keypad(stdscr, TRUE);		/* We get F1, F2 etc..		*/
	noecho();			/* Don't echo() while we do getch */

    int n = 0;
    mvprintw(0,0,"Player %c", ch);
    // prepare the movement message
    m.msg_type = 1;
    m.ch = ch;
    int key, score = 0;
    while(1){
    	key = getch();	
        n++;

        switch (key)
        {
        case KEY_LEFT:
            mvprintw(3,0,"%d Left arrow is pressed", n);
            // prepare the movement message
           m.direction = LEFT;
            break;
        case KEY_RIGHT:
            mvprintw(3,0,"%d Right arrow is pressed", n);
            // prepare the movement message
            m.direction = RIGHT;
            break;
        case KEY_DOWN:
            mvprintw(3,0,"%d Down arrow is pressed", n);
            // prepare the movement message
           m.direction = DOWN;
            break;
        case KEY_UP:
            mvprintw(3,0,"%d :Up arrow is pressed", n);
            // prepare the movement message
            m.direction = UP;
            break;
        
        case 'q':
        case 'Q':
            m.msg_type = 5;
            zmq_send (requester, &m, sizeof(m), 0);
            zmq_recv (requester, &m, sizeof(m), 0);
            free(candidate1);
            endwin();
            return 0;
            break;
        
        default:
            key = 'x'; 
            break;
        }

        //send the movement message
        if (key != 'x'){
            zmq_send (requester, &m, sizeof(m), 0);
            //receive score and display it
            zmq_recv (requester, &m, sizeof(m), 0);
            score = m.ncock;
            mvprintw(1,0,"Score - %d", score);
        }
        refresh();			/* Print it on to the real screen */
    };
    
    free(candidate1);
    zmq_close (requester);
    zmq_ctx_destroy (context);
    endwin();			/* End curses mode		  */
	return 0;
}
