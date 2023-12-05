#include <zmq.h>
#include <ncurses.h>
#include "remote-char.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <assert.h>

#define WINDOW_SIZE 15 
#define MAX_COCK 30

typedef struct ch_info_t
{
    int ch;
    int pos_x, pos_y;
    direction_t dir;
} ch_info_t;

typedef struct board_t
{
    //printed char
    int ch;
    //number of characters overlaying each other
    int nlayers;
    
} board_t;

typedef struct cockroach_info_t
{
    //cockroach value
    int value;
    //cockroach position
    int posx;
    int posy;
    //TODO: include sleep
} cockroach_info_t;

direction_t random_direction(){
    return  random()%4;

}
    void new_position(int* x, int *y, direction_t direction){
        switch (direction)
        {
        case UP:
            (*x) --;
            if(*x ==0)
                *x = 2;
            break;
        case DOWN:
            (*x) ++;
            if(*x ==WINDOW_SIZE-1)
                *x = WINDOW_SIZE-3;
            break;
        case LEFT:
            (*y) --;
            if(*y ==0)
                *y = 2;
            break;
        case RIGHT:
            (*y) ++;
            if(*y ==WINDOW_SIZE-1)
                *y = WINDOW_SIZE-3;
            break;
        default:
            break;
        }
}

int find_ch_info(ch_info_t char_data[], int n_char, int ch){

    for (int i = 0 ; i < n_char; i++){
        if(ch == char_data[i].ch){
            return i;
        }
    }
    return -1;
}

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

    ch_info_t char_data[100];
    int n_chars = 0;
    remote_char_t m;
    cockroach_info_t cock_data[MAX_COCK];

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

    int ch;
    int pos_x;
    int pos_y;
    int i;
    int ncock, total_cock = 0;
    direction_t  direction;

    //message from the server with a character's updated position
    remote_display_t m2;

    //joining server
    m.msg_type = 2;
    zmq_send (requester, &m, sizeof(m), 0);
    zmq_recv (requester, &m, sizeof(m), 0);
    //load the amount of chars in the server
    n_chars = m.msg_type;
    //load the chars in the server
    if(n_chars > 0){
        for(i = 0; i < n_chars; i++){
            zmq_recv (subscriber, &m2, sizeof(m2), 0);
            if(m2.msg_type == -1){
                break;
            }
            
            char_data[i].ch = m2.ch;
            char_data[i].pos_x = m2.posx;
            char_data[i].pos_y = m2.posy;
            char_data[i].dir = m2.direction;
            pos_x = m2.posx;
            pos_y = m2.posy;	
        }
    }else{
        wrefresh(my_win);
    }

    //copy and draw board from server
    zmq_recv (subscriber, &board, sizeof(board), 0);
    for (int a = 1; a < WINDOW_SIZE-1; a++) {
        for (int b = 1; b < WINDOW_SIZE-1; b++) {
            wmove(my_win, a, b);
            waddch(my_win, board[b][a].ch| A_BOLD);
        }
    }
    wrefresh(my_win);

    while (1)
    {
        zmq_recv (subscriber, &m2, sizeof(m2), 0);
        if(m2.msg_type == 0){
            ch = m2.ch;
            pos_x = m2.posx;
            pos_y = m2.posy;

            //STEP 3
            char_data[n_chars].ch = ch;
            char_data[n_chars].pos_x = pos_x;
            char_data[n_chars].pos_y = pos_y;
            board[pos_y][pos_x].ch = ch;
            //initiate lizard going up
            char_data[n_chars].dir = m2.direction;
            for(i = 1; i <= 5; i++){
                    if(pos_x + i < WINDOW_SIZE-1){
                        board[pos_y][pos_x + i].ch = '.';
                        board[pos_y][pos_x + i].nlayers++;
                        wmove(my_win, pos_x + i, pos_y);
                        waddch(my_win, '.');
                    }else{
                        break;
                    }
                }
            n_chars++;
            /* draw mark on new position */
            wmove(my_win, pos_x, pos_y);
            waddch(my_win,ch| A_BOLD);
            wrefresh(my_win);
        }
        else if(m2.msg_type == 1){
            
            int ch_pos = find_ch_info(char_data, n_chars, m2.ch);
            if(ch_pos != -1){
                pos_x = char_data[ch_pos].pos_x;
                pos_y = char_data[ch_pos].pos_y;
                ch = char_data[ch_pos].ch;

                //load old direction
                direction = char_data[ch_pos].dir;
                /*deletes old place */
                board[pos_y][pos_x].ch = ' ';
                wmove(my_win, pos_x, pos_y);
                waddch(my_win,' ');
            switch (direction)
            {
            case UP:
                for(i = 1; i <= 5; i++){
                    if(pos_x + i < WINDOW_SIZE-1){
                        if(board[pos_y][pos_x + i].ch <= 46){
                            if(board[pos_y][pos_x + i].nlayers == 1){
                                wmove(my_win, pos_x + i, pos_y);
                                waddch(my_win, ' ');
                                board[pos_y][pos_x + i].ch = ' ';
                            }else{
                                //draw what's under the tail (need to implement cockroaches)
                            }
                        }
                        board[pos_y][pos_x + i].nlayers--;
                    }else{
                        break;
                    }
                }
                break;
            case DOWN:
                for(i = 1; i <= 5; i++){
                    if(pos_x - i > 0){
                        if(board[pos_y][pos_x - i].ch <= 46){
                            if(board[pos_y][pos_x - i].nlayers == 1){
                                wmove(my_win, pos_x - i, pos_y);
                                waddch(my_win, ' ');
                                board[pos_y][pos_x - i].ch =  ' ';
                            }else{}
                        }
                        board[pos_y][pos_x - i].nlayers--;
                    }else{
                        break;
                    }
                }
                break;
            case LEFT:
                for(i = 1; i <= 5; i++){
                    if(pos_y + i < WINDOW_SIZE-1){
                        if(board[pos_y + i][pos_x].ch <= 46){  
                            if(board[pos_y + i][pos_x].nlayers == 1){  
                                wmove(my_win, pos_x, pos_y + i);
                                waddch(my_win, ' ');
                                board[pos_y + i][pos_x].ch = ' ';
                            }else{}
                        }
                        board[pos_y + i][pos_x].nlayers--;
                    }else{
                        break;
                    }
                }
                break;
            case RIGHT:
                for(i = 1; i <= 5; i++){
                    if(pos_y - i > 0){
                        if(board[pos_y - i][pos_x].ch <= 46){
                            if(board[pos_y - i][pos_x].nlayers == 1){
                                wmove(my_win, pos_x, pos_y - i);
                                waddch(my_win, ' ');
                                board[pos_y - i][pos_x].ch = ' ';
                            }else{}
                        }
                        board[pos_y - i][pos_x].nlayers--;    
                    }else{
                        break;
                    }
                }
                break;
            default:
                break;
            }
                /* claculates new direction */
                direction = m2.direction;
                char_data[ch_pos].dir = direction;
                /* claculates new mark position */
                new_position(&pos_x, &pos_y, direction);
                if(board[pos_y][pos_x].ch > 46 && board[pos_y][pos_x].ch != ch){
                    pos_x = char_data[ch_pos].pos_x;
                    pos_y = char_data[ch_pos].pos_y;
                }else{
                    char_data[ch_pos].pos_x = m2.posx;
                    char_data[ch_pos].pos_y = m2.posy;
                    pos_x = m2.posx;
                    pos_y = m2.posy;
                }
            }    
            //draw lizard tail
            switch (direction)
            {
            case UP:
                for(i = 1; i <= 5; i++){
                    if(pos_x + i < WINDOW_SIZE-1){
                        if(board[pos_y][pos_x + i].ch <= 32){
                            board[pos_y][pos_x + i].ch = '.';
                            wmove(my_win, pos_x + i, pos_y);
                            waddch(my_win, '.');
                        }
                        board[pos_y][pos_x + i].nlayers++;
                    }else{
                        break;
                    }
                }
                break;
            case DOWN:
                for(i = 1; i <= 5; i++){
                    if(pos_x - i > 0){
                        if(board[pos_y][pos_x - i].ch <= 32){
                            board[pos_y][pos_x - i].ch = '.';
                            wmove(my_win, pos_x - i, pos_y);
                            waddch(my_win, '.');
                        }
                        board[pos_y][pos_x - i].nlayers++;
                    }else{
                        break;
                    }
                }
                break;
            case LEFT:
                for(i = 1; i <= 5; i++){
                    if(pos_y + i < WINDOW_SIZE-1){
                        if(board[pos_y + i][pos_x].ch <= 32){    
                            board[pos_y + i][pos_x].ch = '.';
                            wmove(my_win, pos_x, pos_y + i);
                            waddch(my_win, '.');
                        }
                        board[pos_y + i][pos_x].nlayers++;
                    }else{
                        break;
                    }
                }
                break;
            case RIGHT:
                for(i = 1; i <= 5; i++){
                    if(pos_y - i > 0){
                        if(board[pos_y - i][pos_x].ch <= 32){
                            board[pos_y - i][pos_x].ch = '.';
                            wmove(my_win, pos_x, pos_y - i);
                            waddch(my_win, '.');
                        }
                        board[pos_y - i][pos_x].nlayers++;    
                    }else{
                        break;
                    }
                }
                break;
            default:
                break;
            }
                
            /* draw mark on new position */
            board[pos_y][pos_x].ch = ch;
            wmove(my_win, pos_x, pos_y);
            waddch(my_win, ch| A_BOLD);
            wrefresh(my_win);    
        }
        else if(m2.msg_type == 3){
            board[m2.posy][m2.posx].ch = m2.ch;
            cock_data[total_cock++].value = m2.ch;
            cock_data[total_cock++].posx = m2.posx;
            cock_data[total_cock++].posy = m2.posy;
            //print the cockroach
            wmove(my_win, m2.posx, m2.posy);
            waddch(my_win, cock_data[total_cock + i].value| A_BOLD);
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