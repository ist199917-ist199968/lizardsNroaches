#include <zmq.h>
#include <ncurses.h>
#include "remote-char.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  
 #include <ctype.h> 
 #include <stdlib.h>
 

int main()
{

    void *context = zmq_ctx_new ();
    void *requester = zmq_socket (context, ZMQ_REQ);
    zmq_connect (requester, "tcp://127.0.0.1:5610");


    // read the character from the user
    char ch;
    do{
        printf("what is your character(a..z)?: ");
        ch = getchar();
        ch = tolower(ch);  
    }while(!isalpha(ch));


    // send connection message
    remote_char_t m;
    m.msg_type = 0;
    m.ch = ch;
    m.direction = 0;
    zmq_send (requester, &m, sizeof(m), 0);
    
    //receive the assigned letter from the server
    zmq_recv (requester, &m, sizeof(m), 0);
    ch = m.ch;

	initscr();			/* Start curses mode 		*/
	cbreak();				/* Line buffering disabled	*/
	keypad(stdscr, TRUE);		/* We get F1, F2 etc..		*/
	noecho();			/* Don't echo() while we do getch */

    int n = 0;

    // prepare the movement message
    m.msg_type = 1;
    m.ch = ch;
    int key;
    do
    {
    	key = getch();	
        n++;

        //TODO: case q or Q is pressed, disconnect from server
        switch (key)
        {
        case KEY_LEFT:
            mvprintw(0,0,"%d Left arrow is pressed", n);
            // prepare the movement message
           m.direction = LEFT;
            break;
        case KEY_RIGHT:
            mvprintw(0,0,"%d Right arrow is pressed", n);
            // prepare the movement message
            m.direction = RIGHT;
            break;
        case KEY_DOWN:
            mvprintw(0,0,"%d Down arrow is pressed", n);
            // prepare the movement message
           m.direction = DOWN;
            break;
        case KEY_UP:
            mvprintw(0,0,"%d :Up arrow is pressed", n);
            // prepare the movement message
            m.direction = UP;
            break;
        /*
        case 'q':
        case 'Q':
            mvprintw(0,0,"%d :exit", n);
            //TODO: make a new msg_type to disconnect from the server
            break;
        */
        default:
            key = 'x'; 
            break;
        }

        //send the movement message
         if (key != 'x'){
            zmq_send (requester, &m, sizeof(m), 0);
            //TODO: receive score and display it
            zmq_recv (requester, &m, sizeof(m), 0);
        }
        refresh();			/* Print it on to the real screen */
    }while(key != 27);
    

    zmq_close (requester);
    zmq_ctx_destroy (context);
    endwin();			/* End curses mode		  */
	return 0;
}