#include <zmq.h>
#include <ncurses.h>
#include <stdbool.h>
#include "remote-char.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#define WINDOW_SIZE 30 
#define MAX_COCK (WINDOW_SIZE*WINDOW_SIZE)/3
#define MAX_PLAYERS 26

time_t start_time;

typedef struct board_t
{
    //printed char
    int ch;
    //number of characters overlaying each other
    int nlayers;
    //winning layers underneath '*'
    int wlayers;
} board_t;

int main()
{	
    //matrix with everything drawn
    board_t board[WINDOW_SIZE-1][WINDOW_SIZE-1];
    for (int a = 0; a < WINDOW_SIZE-1; a++) {
        for (int b = 0; b < WINDOW_SIZE-1; b++) {
            board[a][b].ch = ' ';    
            board[a][b].nlayers = 0; 
        }
    }

    remote_char_t m;

    void *context = zmq_ctx_new ();
    void *requester = zmq_socket (context, ZMQ_REQ);
    zmq_connect (requester, "tcp://127.0.0.1:5610");

    void *context2 = zmq_ctx_new ();
    void *subscriber = zmq_socket (context, ZMQ_SUB);
    int rc = zmq_connect (subscriber, "tcp://127.0.0.2:5620");
    assert (rc == 0);
    rc = zmq_setsockopt (subscriber, ZMQ_SUBSCRIBE, "", 0);
    assert (rc == 0);

	initscr();		    	
	cbreak();				
    keypad(stdscr, TRUE);   
	noecho();			    

    /* creates a window and draws a border */
    WINDOW * my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(my_win, 0 , 0);	
	wrefresh(my_win);

    //message from the server with a character's updated position
    remote_display_t m2;
    int n_chars;
    //joining server
    m.msg_type = 2;
    zmq_send (requester, &m, sizeof(m), 0);
    zmq_recv (requester, &board, sizeof(board), 0);

    for (int a = 1; a < WINDOW_SIZE-1; a++) {
        for (int b = 1; b < WINDOW_SIZE-1; b++) {
            wmove(my_win, a, b);
            if(board[b][a].ch <= 5){
                waddch(my_win, (char)(board[b][a].ch + '0')| A_BOLD);}
            else{waddch(my_win, (char)(board[b][a].ch)| A_BOLD);}
        }
    }
    wrefresh(my_win);
    while (1)
    {
        zmq_recv (subscriber, &m2, sizeof(m2), 0);
        if(m2.posx == 999){//flag for scoreboard print
            n_chars = m2.posy;
            mvprintw(0, WINDOW_SIZE + 1,"Scoreboard");
            //delete previous scoreboard
            for(int i = 0; i < MAX_PLAYERS; i++){
                mvprintw(i + 2, WINDOW_SIZE + 1,"               ");
            }
            //print new scoreboard
            for(int i = 0; i < n_chars; i++){
                zmq_recv (subscriber, &m2, sizeof(m2), 0);
                mvprintw(i + 2, WINDOW_SIZE + 1,"%c score: %d", m2.posx , m2.score);
            }
            refresh();
            box(my_win, 0 , 0);
        }
        else{
            wmove(my_win, m2.posx, m2.posy);
            waddch(my_win,m2.ch| A_BOLD);
            wrefresh(my_win);
        }   
    }

    
    zmq_close (subscriber);
    zmq_close (requester);
    zmq_ctx_destroy (context);
    zmq_ctx_destroy (context2);
    endwin();			/* End curses mode		  */
	return 0;
}