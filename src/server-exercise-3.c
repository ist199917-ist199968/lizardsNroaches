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

typedef struct ch_info_t
{
    int ch;
    int pos_x, pos_y;
    int score;
    bool win;
    direction_t dir;
} ch_info_t;

typedef struct board_t
{
    //printed char
    int ch;
    //number of characters overlaying each other
    int nlayers;
    //winning layers underneath '*'
    int wlayers;
} board_t;


typedef struct cockroach_info_t
{
    //cockroach value
    int value;
    //cockroach position
    int posx;
    int posy;
    //time it was eaten
    time_t time_eaten;
} cockroach_info_t;

//find what's under
int what_under(board_t board, int posx, int posy, cockroach_info_t cock_data[MAX_COCK], int cockid, bool is_winner){
    
    int under = 0;
    //draw tail flag is 999
    if(board.nlayers == 1 && cockid == 999){
        under =  0;
    //cockroach draw
    }else if(board.nlayers > 0){
        //found lizard body
        if(((board.wlayers) == board.nlayers && cockid == 999 && is_winner == true) || ((board.wlayers+1) == board.nlayers && cockid == 999 && is_winner == false) || ((board.wlayers) == board.nlayers && cockid != 999)){ //send '*'
            under = 8;
        }else{ under = 9;} //send '.'
    }
    for(int i = 0; i < MAX_COCK; i++){
        //found cockroach under
        if(cock_data[i].posx == posx && cock_data[i].posy == posy && i != cockid && (time(&start_time)-cock_data[i].time_eaten)>=5){
            under = cock_data[i].value;
            break;
        }
    }
    //nothing found under
    return under;
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

int available_char(int n_chars, int ch, ch_info_t char_data[]){

    for(int i = 0; i < n_chars; i++){ 
        if(char_data[i].ch == ch){
            //if it is then search for a character that isn't
            for(int j = 'a'; j <= 'z'; j++){
                for(int h = 0; h < n_chars; h++){
                    if(char_data[h].ch == j){
                        ch = 0;
                        break;
                    } else{
                        ch = j;
                    }
                }
                if(ch != 0)
                    return ch;
            }
        }
    }
    return ch;
}

void scoreboard(ch_info_t char_data[], int n_chars){
    mvprintw(0, WINDOW_SIZE + 1,"Scoreboard");
    //delete previous scoreboard
    for(int i = 0; i < MAX_PLAYERS; i++){
        mvprintw(i + 2, WINDOW_SIZE + 1,"               ");
    }
    //print new scoreboard
    for(int i = 0; i < n_chars; i++){
        mvprintw(i + 2, WINDOW_SIZE + 1,"%c score: %2d", char_data[i].ch, char_data[i].score);
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
            board[a][b].wlayers = 0; 
        }
    }
    int total_score = 0;
    ch_info_t char_data[MAX_PLAYERS];
    int n_chars = 0;
    remote_char_t m;
    remote_display_t m2;
    cockroach_info_t cock_data[MAX_COCK];

    void *context = zmq_ctx_new ();
    void *responder = zmq_socket (context, ZMQ_REP);
    int rc = zmq_bind (responder, "tcp://127.0.0.1:5610");
    assert (rc == 0);

    void *context2 = zmq_ctx_new ();
    void *publisher = zmq_socket (context2, ZMQ_PUB);
    int rc2 = zmq_bind (publisher, "tcp://127.0.0.2:5620");
    assert(rc2 == 0);

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
    int i, k, l, under;
    int ncock, total_cock = 0, fcock;
    direction_t  direction;
    while (1)
    {
        zmq_recv (responder, &m, sizeof(m), 0);
        
        //new lizard character joins
        if(m.msg_type == 0){
            //max players
            if(n_chars == MAX_PLAYERS){
                m.ch = 0;
                zmq_send (responder, &m, sizeof(m), 0);
            }else{
            //check if character is already in char_data
            ch = m.ch;          
            ch = available_char(n_chars, ch, char_data);
            m.ch = ch;
            zmq_send (responder, &m, sizeof(m), 0);
            
            //find a free space (it can have a cockroach or a lizard body)
            while(1){
                pos_x = random()%(WINDOW_SIZE-2) + 1;
                pos_y = random()%(WINDOW_SIZE-2) + 1;
                if(board[pos_y][pos_x].ch <= 46)
                    break;
            }

            char_data[n_chars].ch = ch;
            char_data[n_chars].pos_x = pos_x;
            char_data[n_chars].pos_y = pos_y;
            char_data[n_chars].score = 0;
            char_data[n_chars].win = false;
            board[pos_y][pos_x].ch = ch;

            //initiate lizard going up
            direction = UP;
            char_data[n_chars].dir = UP;
            for(i = 1; i <= 5; i++){
                    if(pos_x + i < WINDOW_SIZE-1){
                        if(board[pos_y][pos_x + i].ch <= 46){
                            wmove(my_win, pos_x + i, pos_y);
                            waddch(my_win, '.'| A_BOLD);
                            board[pos_y][pos_x + i].ch = '.';
                            m2.ch = '.';
                            m2.posx = pos_x + i;
                            m2.posy = pos_y;	
                            zmq_send (publisher, &m2, sizeof(m2), 0);
                        }
                        board[pos_y][pos_x + i].nlayers++;
                    }else{
                        break;
                    }
                }
            n_chars++;

            /* draw mark on new position */
            wmove(my_win, pos_x, pos_y);
            waddch(my_win, ch| A_BOLD);
            wrefresh(my_win);	
            m2.ch = ch;
            m2.posx = pos_x;
            m2.posy = pos_y;	
            zmq_send (publisher, &m2, sizeof(m2), 0);
            }
        }
        //lizard movement message
        else if(m.msg_type == 1){
            //check if current ch_pos is a winner
            bool winner = false;
            int ch_pos = find_ch_info(char_data, n_chars, m.ch);
            if(ch_pos != -1){
                pos_x = char_data[ch_pos].pos_x;
                pos_y = char_data[ch_pos].pos_y;
                ch = char_data[ch_pos].ch;
                winner = char_data[ch_pos].win;
                //load old direction
                direction = char_data[ch_pos].dir;
                /*deletes old place */
                board[pos_y][pos_x].ch = ' ';
                wmove(my_win, pos_x, pos_y);
                waddch(my_win,' ');
                m2.ch = ' ';
                m2.posx = pos_x;
                m2.posy = pos_y;	
                zmq_send (publisher, &m2, sizeof(m2), 0);
            switch (direction)
            {
            case UP:
                for(i = 1; i <= 5; i++){
                    if(pos_x + i < WINDOW_SIZE-1){
                        if(board[pos_y][pos_x + i].ch <= 46){
                            under = what_under(board[pos_y][pos_x + i], pos_x + i, pos_y, cock_data, 999, winner);
                            if(under == 0){
                                wmove(my_win, pos_x + i, pos_y);
                                waddch(my_win, ' ');
                                board[pos_y][pos_x + i].ch = ' ';
                                m2.ch = ' ';
                                m2.posx = pos_x + i;
                                m2.posy = pos_y;	
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }else if(under == 8){
                                wmove(my_win, pos_x + i, pos_y);
                                waddch(my_win, '*'| A_BOLD);
                                board[pos_y][pos_x + i].ch = '*';
                                m2.ch = '*';
                                m2.posx = pos_x + i;
                                m2.posy = pos_y;	
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }else if(under == 9){
                                wmove(my_win, pos_x + i, pos_y);
                                waddch(my_win, '.'| A_BOLD);
                                board[pos_y][pos_x + i].ch = '.';
                                m2.ch = '.';
                                m2.posx = pos_x + i;
                                m2.posy = pos_y;	
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }else{
                                wmove(my_win, pos_x + i, pos_y);
                                waddch(my_win, (under + '0')| A_BOLD);
                                board[pos_y][pos_x + i].ch = under;
                                m2.ch = (char)(under + '0');
                                m2.posx = pos_x + i;
                                m2.posy = pos_y;	
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }
                        }    
                        board[pos_y][pos_x + i].nlayers--;
                        if(char_data[ch_pos].win == true)
                            board[pos_y][pos_x + i].wlayers--;
                    }else{
                        break;
                    }
                }
                break;
            case DOWN:
                for(i = 1; i <= 5; i++){
                    if(pos_x - i > 0){
                        if(board[pos_y][pos_x - i].ch <= 46){
                            under = what_under(board[pos_y][pos_x - i], pos_x - i, pos_y, cock_data, 999, winner);
                            if(under == 0){
                                wmove(my_win, pos_x - i, pos_y);
                                waddch(my_win, ' ');
                                board[pos_y][pos_x - i].ch = ' ';
                                m2.ch = ' ';
                                m2.posx = pos_x - i;
                                m2.posy = pos_y;	
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }else if(under == 8){
                                wmove(my_win, pos_x - i, pos_y);
                                waddch(my_win, '*'| A_BOLD);
                                board[pos_y][pos_x - i].ch = '*';
                                m2.ch = '*';
                                m2.posx = pos_x - i;
                                m2.posy = pos_y;	
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }else if(under == 9){
                                wmove(my_win, pos_x - i, pos_y);
                                waddch(my_win, '.'| A_BOLD);
                                board[pos_y][pos_x - i].ch = '.';
                                m2.ch = '.';
                                m2.posx = pos_x - i;
                                m2.posy = pos_y;	
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }else{
                                wmove(my_win, pos_x - i, pos_y);
                                waddch(my_win, (under + '0')| A_BOLD);
                                board[pos_y][pos_x - i].ch = under;
                                m2.ch = (char)(under + '0');
                                m2.posx = pos_x - i;
                                m2.posy = pos_y;	
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }
                        }
                        board[pos_y][pos_x - i].nlayers--;
                        if(char_data[ch_pos].win == true)
                            board[pos_y][pos_x - i].wlayers--;
                    }else{
                        break;
                    }
                }
                break;
            case LEFT:
                for(i = 1; i <= 5; i++){
                    if(pos_y + i < WINDOW_SIZE-1){
                        if(board[pos_y + i][pos_x].ch <= 46){  
                            under = what_under(board[pos_y + i][pos_x], pos_x, pos_y + i, cock_data, 999, winner);
                            if(under == 0){
                                wmove(my_win, pos_x, pos_y + i);
                                waddch(my_win, ' ');
                                board[pos_y + i][pos_x].ch = ' ';
                                m2.ch = ' ';
                                m2.posx = pos_x;
                                m2.posy = pos_y + i;	
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }else if(under == 8){
                                wmove(my_win, pos_x, pos_y + i);
                                waddch(my_win, '*'| A_BOLD);
                                board[pos_y + i][pos_x].ch = '*';
                                m2.ch = '*';
                                m2.posx = pos_x;
                                m2.posy = pos_y + i;	
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }else if(under == 9){
                                wmove(my_win, pos_x, pos_y + i);
                                waddch(my_win, '.'| A_BOLD);
                                board[pos_y + i][pos_x].ch = '.';
                                m2.ch = '.';
                                m2.posx = pos_x;
                                m2.posy = pos_y + i;	
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }else{
                                wmove(my_win, pos_x, pos_y + i);
                                waddch(my_win, (under + '0')| A_BOLD);
                                board[pos_y + i][pos_x].ch = under;
                                m2.ch = (char)(under + '0');
                                m2.posx = pos_x;
                                m2.posy = pos_y + i;	
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }
                        }
                        board[pos_y + i][pos_x].nlayers--;
                        if(char_data[ch_pos].win == true)
                            board[pos_y + i][pos_x].wlayers--;
                    }else{
                        break;
                    }
                }
                break;
            case RIGHT:
                for(i = 1; i <= 5; i++){
                    if(pos_y - i > 0){
                        if(board[pos_y - i][pos_x].ch <= 46){
                            under = what_under(board[pos_y - i][pos_x], pos_x, pos_y - i, cock_data, 999, winner);
                            if(under == 0){
                                wmove(my_win, pos_x, pos_y - i);
                                waddch(my_win, ' ');
                                board[pos_y - i][pos_x].ch = ' ';
                                m2.ch = ' ';
                                m2.posx = pos_x;
                                m2.posy = pos_y - i;	
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }else if(under == 8){
                                wmove(my_win, pos_x, pos_y - i);
                                waddch(my_win, '*'| A_BOLD);
                                board[pos_y - i][pos_x].ch = '*';
                                m2.ch = '*';
                                m2.posx = pos_x;
                                m2.posy = pos_y - i;	
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }else if(under == 9){
                                wmove(my_win, pos_x, pos_y - i);
                                waddch(my_win, '.'| A_BOLD);
                                board[pos_y - i][pos_x].ch = '.';
                                m2.ch = '.';
                                m2.posx = pos_x;
                                m2.posy = pos_y - i;	
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }else{
                                wmove(my_win, pos_x, pos_y - i);
                                waddch(my_win, (under + '0')| A_BOLD);
                                board[pos_y - i][pos_x].ch = under;
                                m2.ch = (char)(under + '0');
                                m2.posx = pos_x;
                                m2.posy = pos_y - i;	
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }
                        }
                        board[pos_y - i][pos_x].nlayers--;    
                        if(char_data[ch_pos].win == true)
                            board[pos_y - i][pos_x].wlayers--;
                    }else{
                        break;
                    }
                }
                break;
            default:
                break;
            }
                /* claculates new direction */
                direction = m.direction;
                char_data[ch_pos].dir = direction;
                m2.posx = pos_x;
                m2.posy = pos_y;
                /* claculates new mark position */
                new_position(&pos_x, &pos_y, direction);
                //collision, don't change position
                if(board[pos_y][pos_x].ch > 46 && board[pos_y][pos_x].ch != ch){
                    //half of total score
                    i = find_ch_info(char_data, n_chars, board[pos_y][pos_x].ch);
                    total_score = char_data[ch_pos].score + char_data[i].score;
                    char_data[ch_pos].score = total_score/2;
                    char_data[i].score = total_score/2;
                    //don't change position
                    pos_x = char_data[ch_pos].pos_x;
                    pos_y = char_data[ch_pos].pos_y;

                }else{//can move
                    char_data[ch_pos].pos_x = pos_x;
                    char_data[ch_pos].pos_y = pos_y;
                    //check if any cockroach in the new position and score
                    for(i = 0; i < total_cock; i++){
                        if(cock_data[i].posx == pos_x && cock_data[i].posy == pos_y && (time(&start_time)-cock_data[i].time_eaten)>=5){
                            char_data[ch_pos].score += cock_data[i].value;
                            cock_data[i].time_eaten = time(&start_time);
                        }
                    }
                }
                //winning
                if(char_data[ch_pos].score >= 50)
                    char_data[ch_pos].win = true;
                //send score back to lizard
                m.ncock = char_data[ch_pos].score;
                zmq_send (responder, &m, sizeof(m), 0);
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
                            if(char_data[ch_pos].win == true){
                                waddch(my_win, '*'| A_BOLD);
                                m2.ch = '*';
                            }else{
                                waddch(my_win, '.'| A_BOLD);
                                m2.ch = '.';
                            }
                            m2.posx = pos_x + i;
                            m2.posy = pos_y;	
                            zmq_send (publisher, &m2, sizeof(m2), 0);
                        }
                        board[pos_y][pos_x + i].nlayers++;
                        if(char_data[ch_pos].win == true)
                            board[pos_y][pos_x + i].wlayers++;
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
                            if(char_data[ch_pos].win == true){
                                waddch(my_win, '*'| A_BOLD);
                                m2.ch = '*';
                            }else{
                                waddch(my_win, '.'| A_BOLD);
                                m2.ch = '.';
                            }
                            m2.posx = pos_x - i;
                            m2.posy = pos_y;	
                            zmq_send (publisher, &m2, sizeof(m2), 0);
                        }
                        board[pos_y][pos_x - i].nlayers++;
                        if(char_data[ch_pos].win == true)
                            board[pos_y][pos_x - i].wlayers++;
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
                            if(char_data[ch_pos].win == true){
                                waddch(my_win, '*'| A_BOLD);
                                m2.ch = '*';
                            }else{
                                waddch(my_win, '.'| A_BOLD);
                                m2.ch = '.';
                            }
                            m2.posx = pos_x;
                            m2.posy = pos_y + i;	
                            zmq_send (publisher, &m2, sizeof(m2), 0);
                        }
                        board[pos_y + i][pos_x].nlayers++;
                        if(char_data[ch_pos].win == true)
                            board[pos_y + i][pos_x].wlayers++;
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
                            if(char_data[ch_pos].win == true){
                                waddch(my_win, '*'| A_BOLD);
                                m2.ch = '*';
                            }else{
                                waddch(my_win, '.'| A_BOLD);
                                m2.ch = '.';
                            }
                            m2.posx = pos_x;
                            m2.posy = pos_y - i;	
                            zmq_send (publisher, &m2, sizeof(m2), 0);
                        }
                        board[pos_y - i][pos_x].nlayers++;    
                        if(char_data[ch_pos].win == true)
                            board[pos_y - i][pos_x].wlayers++;
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
            scoreboard(char_data, n_chars);
            //send scoreboard
            m2.posx = 999; //flag for scoreboard
            m2.posy = n_chars; //send number of characters
            zmq_send (publisher, &m2, sizeof(m2), 0);
            for(i = 0; i < n_chars; i++){
                m2.posx = char_data[i].ch;
                //m2.ch = (char)(char_data[i].ch + '0');
                m2.score = char_data[i].score;
                zmq_send (publisher, &m2, sizeof(m2), 0);
            }
            refresh();
            box(my_win, 0 , 0);
            wmove(my_win, pos_x, pos_y);
            waddch(my_win, ch| A_BOLD);
            wrefresh(my_win);	
            m2.ch = ch;		
            m2.posx = pos_x;
            m2.posy = pos_y;
            zmq_send (publisher, &m2, sizeof(m2), 0);        
        }
        //remote display joins
        else if(m.msg_type == 2){
            //send the board with the printed chars
            zmq_send (responder, &board, sizeof(board), 0);

        }
        //cockroach joins
        else if(m.msg_type == 3){
            //number of cockroaches from this user
            ncock = m.ncock;
            //check if there is no more space for cockroaches
            if(total_cock + ncock > MAX_COCK){
                //"too many cockroaches" and close the client
                m.ncock = 0;
                zmq_send (responder, &m, sizeof(m), 0);
            }else{                
                //random position for each cockroach
                i = 0;
                while(i != ncock){
                    k = random()%(WINDOW_SIZE-2) + 1;
                    l = random()%(WINDOW_SIZE-2) + 1;
                    //if it is a free space, a lizard body or another cockroach
                    if(board[l][k].ch <= 46){
                        //save the cockroach data
                        ch = random()%5 + 1;
                        cock_data[total_cock + i].value = ch;
                        cock_data[total_cock + i].posx = k;
                        cock_data[total_cock + i].posy = l;
                        board[l][k].ch = ch;
                        //print the cockroach
                        wmove(my_win, k, l);
                        waddch(my_win, (ch + '0')| A_BOLD);
                        //send to displays
                        m2.ch = (char)(ch + '0');
                        m2.posx = k;
                        m2.posy = l;		
                        zmq_send (publisher, &m2, sizeof(m2), 0);
                        i++;
                    }
                }
                wrefresh(my_win);
                //send the position of the first cockroach in the data array
                m.ch = total_cock;
                zmq_send (responder, &m, sizeof(m), 0);
                //update the number of cockroaches in the server
                total_cock = total_cock + ncock;
            }
        }
        //cockroach movement
        else if(m.msg_type == 4){
            ncock = m.ncock;
            fcock = m.ch;
            bool winner = false;
            for(i = 0; i < ncock; i++){
                //load previous data
                pos_x = cock_data[fcock + i].posx;
                pos_y = cock_data[fcock + i].posy;
                ch = cock_data[fcock + i].value;
                direction = m.cockdir[i];
            if((time(&start_time)-cock_data[fcock + i].time_eaten)>=5){
                switch (direction)
                {
                case UP:
                    if(pos_x - 1 > 0 && board[pos_y][pos_x - 1].ch <= 46){
                        under = what_under(board[pos_y][pos_x], pos_x, pos_y, cock_data, fcock + i, winner);
                        if(under == 0){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, ' ');
                            board[pos_y][pos_x].ch = ' ';
                            m2.ch = ' ';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            zmq_send (publisher, &m2, sizeof(m2), 0);
                        }else if(under == 8){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, '*'| A_BOLD);
                            board[pos_y][pos_x].ch = '*';
                            m2.ch = '*';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            zmq_send (publisher, &m2, sizeof(m2), 0);
                        }else if(under == 9){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, '.'| A_BOLD);
                            board[pos_y][pos_x].ch = '.';
                            m2.ch = '.';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            zmq_send (publisher, &m2, sizeof(m2), 0);
                        }else{
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, (under + '0')| A_BOLD);
                            board[pos_y][pos_x].ch = under;
                            m2.ch = (char)(under + '0');
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            zmq_send (publisher, &m2, sizeof(m2), 0);
                        }
                        pos_x--;
                    }
                    break;
                case DOWN:
                    if(pos_x + 1 < WINDOW_SIZE-1 && board[pos_y][pos_x + 1].ch <= 46){
                        under = what_under(board[pos_y][pos_x], pos_x, pos_y, cock_data, fcock + i, winner);
                        if(under == 0){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, ' ');
                            board[pos_y][pos_x].ch = ' ';
                            m2.ch = ' ';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            zmq_send (publisher, &m2, sizeof(m2), 0);
                        }else if(under == 8){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, '*'| A_BOLD);
                            board[pos_y][pos_x].ch = '*';
                            m2.ch = '*';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            zmq_send (publisher, &m2, sizeof(m2), 0);
                        }else if(under == 9){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, '.'| A_BOLD);
                            board[pos_y][pos_x].ch = '.';
                            m2.ch = '.';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            zmq_send (publisher, &m2, sizeof(m2), 0);
                        }else{
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, (under + '0')| A_BOLD);
                            board[pos_y][pos_x].ch = under;
                            m2.ch = (char)(under + '0');
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            zmq_send (publisher, &m2, sizeof(m2), 0);
                        }
                        pos_x++;
                    }
                    break;
                case LEFT:
                    if(pos_y - 1 > 0 && board[pos_y - 1][pos_x].ch <= 46){
                        under = what_under(board[pos_y][pos_x], pos_x, pos_y, cock_data, fcock + i, winner);
                        if(under == 0){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, ' ');
                            board[pos_y][pos_x].ch = ' ';
                            m2.ch = ' ';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            zmq_send (publisher, &m2, sizeof(m2), 0);
                        }else if(under == 8){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, '*'| A_BOLD);
                            board[pos_y][pos_x].ch = '*';
                            m2.ch = '*';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            zmq_send (publisher, &m2, sizeof(m2), 0);
                        }else if(under == 9){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, '.'| A_BOLD);
                            board[pos_y][pos_x].ch = '.';
                            m2.ch = '.';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            zmq_send (publisher, &m2, sizeof(m2), 0);
                        }else{
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, (under + '0')| A_BOLD);
                            board[pos_y][pos_x].ch = under;
                            m2.ch = (char)(under + '0');
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            zmq_send (publisher, &m2, sizeof(m2), 0);
                        }
                        pos_y--;
                    }
                    break;
                case RIGHT:
                    if(pos_y + 1 < WINDOW_SIZE-1 && board[pos_y + 1][pos_x].ch <= 46){
                        under = what_under(board[pos_y][pos_x], pos_x, pos_y, cock_data, fcock + i, winner);
                        if(under == 0){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, ' ');
                            board[pos_y][pos_x].ch = ' ';
                            m2.ch = ' ';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            zmq_send (publisher, &m2, sizeof(m2), 0);
                        }else if(under == 8){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, '*'| A_BOLD);
                            board[pos_y][pos_x].ch = '*';
                            m2.ch = '*';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            zmq_send (publisher, &m2, sizeof(m2), 0);
                        }else if(under == 9){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, '.'| A_BOLD);
                            board[pos_y][pos_x].ch = '.';
                            m2.ch = '.';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            zmq_send (publisher, &m2, sizeof(m2), 0);
                        }else{
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, (under + '0')| A_BOLD);
                            board[pos_y][pos_x].ch = under;
                            m2.ch = (char)(under + '0');
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            zmq_send (publisher, &m2, sizeof(m2), 0);
                        }
                        pos_y++;
                    }
                    break;
                default:
                    break;
                }
            
                //saves data in new position
                board[pos_y][pos_x].ch = ch;
                cock_data[fcock +i].posx = pos_x;
                cock_data[fcock +i].posy = pos_y;
                //print new position
                wmove(my_win, pos_x, pos_y);
                waddch(my_win, (ch + '0')| A_BOLD);
                m2.ch = (char)(ch + '0');
                m2.posx = pos_x;
                m2.posy = pos_y;		
                zmq_send (publisher, &m2, sizeof(m2), 0);
            }
            }
            wrefresh(my_win);
            zmq_send (responder, &m, sizeof(m), 0);
        } 
        //lizard disconnect
        else if(m.msg_type ==5){
            zmq_send (responder, &m, sizeof(m), 0);
            int ch_pos = find_ch_info(char_data, n_chars, m.ch);
            bool winner = char_data[ch_pos].win;
            pos_x = char_data[ch_pos].pos_x;
            pos_y = char_data[ch_pos].pos_y;
            ch = char_data[ch_pos].ch;
            direction = char_data[ch_pos].dir;
            board[pos_y][pos_x].ch = ' ';
            wmove(my_win, pos_x, pos_y);
            waddch(my_win,' ');
            m2.ch = ' ';
            m2.posx = pos_x;
            m2.posy = pos_y;		
            zmq_send (publisher, &m2, sizeof(m2), 0);
            //delete from board
            switch (direction)
            {
            case UP:
                for(i = 1; i <= 5; i++){
                    if(pos_x + i < WINDOW_SIZE-1){
                        if(board[pos_y][pos_x + i].ch <= 46){
                            under = what_under(board[pos_y][pos_x + i], pos_x + i, pos_y, cock_data, 999, winner);
                            if(under == 0){
                                wmove(my_win, pos_x + i, pos_y);
                                waddch(my_win, ' ');
                                board[pos_y][pos_x + i].ch = ' ';
                                m2.ch = ' ';
                                m2.posx = pos_x + i;
                                m2.posy = pos_y;		
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }else if(under == 8){
                                wmove(my_win, pos_x + i, pos_y);
                                waddch(my_win, '*'| A_BOLD);
                                board[pos_y][pos_x + i].ch = '*';
                                m2.ch = '*';
                                m2.posx = pos_x + i;
                                m2.posy = pos_y;		
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }else if(under == 9){
                                wmove(my_win, pos_x + i, pos_y);
                                waddch(my_win, '.'| A_BOLD);
                                board[pos_y][pos_x + i].ch = '.';
                                m2.ch = '.';
                                m2.posx = pos_x + i;
                                m2.posy = pos_y;		
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }else{
                                wmove(my_win, pos_x + i, pos_y);
                                waddch(my_win, (under + '0')| A_BOLD);
                                board[pos_y][pos_x + i].ch = under;
                                m2.ch = (char)(under + '0');
                                m2.posx = pos_x + i;
                                m2.posy = pos_y;		
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }
                        }    
                        board[pos_y][pos_x + i].nlayers--;
                        if(char_data[ch_pos].win == true)
                            board[pos_y][pos_x + i].wlayers--;
                    }else{
                        break;
                    }
                }
                break;
            case DOWN:
                for(i = 1; i <= 5; i++){
                    if(pos_x - i > 0){
                        if(board[pos_y][pos_x - i].ch <= 46){
                            under = what_under(board[pos_y][pos_x - i], pos_x - i, pos_y, cock_data, 999, winner);
                            if(under == 0){
                                wmove(my_win, pos_x - i, pos_y);
                                waddch(my_win, ' ');
                                board[pos_y][pos_x - i].ch = ' ';
                                m2.ch = ' ';
                                m2.posx = pos_x - i;
                                m2.posy = pos_y;		
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }else if(under == 8){
                                wmove(my_win, pos_x - i, pos_y);
                                waddch(my_win, '*'| A_BOLD);
                                board[pos_y][pos_x - i].ch = '*';
                                m2.ch = '*';
                                m2.posx = pos_x - i;
                                m2.posy = pos_y;		
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }else if(under == 9){
                                wmove(my_win, pos_x - i, pos_y);
                                waddch(my_win, '.'| A_BOLD);
                                board[pos_y][pos_x - i].ch = '.';
                                m2.ch = '.';
                                m2.posx = pos_x - i;
                                m2.posy = pos_y;		
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }else{
                                wmove(my_win, pos_x - i, pos_y);
                                waddch(my_win, (under + '0')| A_BOLD);
                                board[pos_y][pos_x - i].ch = under;
                                m2.ch = (char)(under + '0');
                                m2.posx = pos_x - i;
                                m2.posy = pos_y;		
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }
                        }
                        board[pos_y][pos_x - i].nlayers--;
                        if(char_data[ch_pos].win == true)
                            board[pos_y][pos_x - i].wlayers--;
                    }else{
                        break;
                    }
                }
                break;
            case LEFT:
                for(i = 1; i <= 5; i++){
                    if(pos_y + i < WINDOW_SIZE-1){
                        if(board[pos_y + i][pos_x].ch <= 46){  
                            under = what_under(board[pos_y + i][pos_x], pos_x, pos_y + i, cock_data, 999, winner);
                            if(under == 0){
                                wmove(my_win, pos_x, pos_y + i);
                                waddch(my_win, ' ');
                                board[pos_y + i][pos_x].ch = ' ';
                                m2.ch = ' ';
                                m2.posx = pos_x;
                                m2.posy = pos_y + i;		
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }else if(under == 8){
                                wmove(my_win, pos_x, pos_y + i);
                                waddch(my_win, '*'| A_BOLD);
                                board[pos_y + i][pos_x].ch = '*';
                                m2.ch = '*';
                                m2.posx = pos_x;
                                m2.posy = pos_y + i;		
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }else if(under == 9){
                                wmove(my_win, pos_x, pos_y + i);
                                waddch(my_win, '.'| A_BOLD);
                                board[pos_y + i][pos_x].ch = '.';
                                m2.ch = '.';
                                m2.posx = pos_x;
                                m2.posy = pos_y + i;		
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }else{
                                wmove(my_win, pos_x, pos_y + i);
                                waddch(my_win, (under + '0')| A_BOLD);
                                board[pos_y + i][pos_x].ch = under;
                                m2.ch = (char)(under + '0');
                                m2.posx = pos_x;
                                m2.posy = pos_y + i;		
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }
                        }
                        board[pos_y + i][pos_x].nlayers--;
                        if(char_data[ch_pos].win == true)
                            board[pos_y + i][pos_x].wlayers--;
                    }else{
                        break;
                    }
                }
                break;
            case RIGHT:
                for(i = 1; i <= 5; i++){
                    if(pos_y - i > 0){
                        if(board[pos_y - i][pos_x].ch <= 46){
                            under = what_under(board[pos_y - i][pos_x], pos_x, pos_y - i, cock_data, 999, winner);
                            if(under == 0){
                                wmove(my_win, pos_x, pos_y - i);
                                waddch(my_win, ' ');
                                board[pos_y - i][pos_x].ch = ' ';
                                m2.ch = ' ';
                                m2.posx = pos_x;
                                m2.posy = pos_y - i;		
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }else if(under == 8){
                                wmove(my_win, pos_x, pos_y - i);
                                waddch(my_win, '*'| A_BOLD);
                                board[pos_y - i][pos_x].ch = '*';
                                m2.ch = '*';
                                m2.posx = pos_x;
                                m2.posy = pos_y - i;		
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }else if(under == 9){
                                wmove(my_win, pos_x, pos_y - i);
                                waddch(my_win, '.'| A_BOLD);
                                board[pos_y - i][pos_x].ch = '.';
                                m2.ch = '.';
                                m2.posx = pos_x;
                                m2.posy = pos_y - i;		
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }else{
                                wmove(my_win, pos_x, pos_y - i);
                                waddch(my_win, (under + '0')| A_BOLD);
                                board[pos_y - i][pos_x].ch = under;
                                m2.ch = (char)(under + '0');
                                m2.posx = pos_x;
                                m2.posy = pos_y - i;		
                                zmq_send (publisher, &m2, sizeof(m2), 0);
                            }
                        }
                        board[pos_y - i][pos_x].nlayers--;
                        if(char_data[ch_pos].win == true)
                            board[pos_y - i][pos_x].wlayers--;    
                    }else{
                        break;
                    }
                }
                break;
            default:
                break;
            }
            //remove from char_data, reduce the index of every lizard that comes after it
            for(i = ch_pos; i <= (n_chars-2); i++){
                char_data[i].ch = char_data[i+1].ch;
                char_data[i].pos_x = char_data[i+1].pos_x;
                char_data[i].pos_y = char_data[i+1].pos_y;
                char_data[i].dir = char_data[i+1].dir;
                char_data[i].score = char_data[i+1].score;
                char_data[i].win = char_data[i+1].win;;
            }
            n_chars--;
            scoreboard(char_data, n_chars);
            //send scoreboard
            m2.posx = 999; //flag for scoreboard
            m2.posy = n_chars; //send number of characters
            zmq_send (publisher, &m2, sizeof(m2), 0);
            for(i = 0; i < n_chars; i++){
                m2.posx = char_data[i].ch;
                //m2.ch = (char)(char_data[i].ch + '0');
                m2.score = char_data[i].score;
                zmq_send (publisher, &m2, sizeof(m2), 0);
            }
            refresh();
            box(my_win, 0 , 0);
            wrefresh(my_win);
        }

        
    }
  	
    zmq_close(publisher);
    zmq_close(responder);
    zmq_ctx_shutdown(context);
    zmq_ctx_shutdown(context2);
    zmq_ctx_term(context);
    zmq_ctx_term(context2);
    endwin();			/* End curses mode		  */
	return 0;
}
