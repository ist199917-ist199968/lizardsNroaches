#include <zmq.h>
#include <ncurses.h>
#include <stdbool.h>
#include "message.pb-c.h"
#include "remote-char.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

#define WINDOW_SIZE 30 
#define MAX_COCK (WINDOW_SIZE*WINDOW_SIZE)/3
#define MAX_PLAYERS 26
#define LIZARD_THREADS 4
#define ROACH_THREADS 1

time_t start_time;

pthread_mutex_t lizard_dealer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t roach_dealer_mutex = PTHREAD_MUTEX_INITIALIZER; //provavelmente nao precisa pq é só uma thread
pthread_mutex_t mutex_test1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_test2 = PTHREAD_MUTEX_INITIALIZER;

typedef struct ch_info_t
{
    int ch;
    int pos_x, pos_y;
    int score;
    bool win;
    bool lose; //false if above or 0, true if below
    ProtoDirection dir;
    char password[50];
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
    char password[50];

} cockroach_info_t;

//GLOBAL
//matrix with everything drawn
board_t board[WINDOW_SIZE-1][WINDOW_SIZE-1];
ch_info_t char_data[MAX_PLAYERS];
int n_chars = 0;
cockroach_info_t cock_data[MAX_COCK];
int total_cock = 0;
WINDOW * my_win = NULL;
void *publisher = NULL;
char *candidate1;
//int teste = 0;
//find what's under; 0 = blank space, 8 = winner body '*'; 9 = lizard body '.'; value = cockroach value; 7 = wasp 
int what_under(board_t board, int posx, int posy, cockroach_info_t cock_data[MAX_COCK], int cockid, bool is_winner){
    
    int under = 0;
    //found wasp under
    if(board.ch == '#'){
        under = 7;
    }
    //draw tail flag is -1
    if(board.nlayers == 1 && cockid == -1){
        under =  0;
    //cockroach draw
    }else if(board.nlayers > 0){
        //found lizard body
        if(((board.wlayers) == board.nlayers && cockid == -1 && is_winner == true) || ((board.wlayers+1) == board.nlayers && cockid == -1 && is_winner == false) || ((board.wlayers) == board.nlayers && cockid != -1)){ //send '*'
            under = 8;
        }else{ under = 9;} //send '.'
    }
    if(cockid != -2){
        for(int i = 0; i < MAX_COCK; i++){
            //found cockroach under
            if(cock_data[i].posx == posx && cock_data[i].posy == posy && i != cockid && (time(&start_time)-cock_data[i].time_eaten)>=5){
                under = cock_data[i].value;
                break;
            }
        }
    }

    //nothing found under
    return under;
}

void new_position(int* x, int *y, ProtoDirection direction){
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


//lizard handling thread function
void *lizard_function(void *context){
    ProtoDisplayMessage m2;
    void *responder = zmq_socket(context, ZMQ_REP);
    zmq_connect(responder, "inproc://mydealer1");
    proto_display_message__init(&m2);
    m2.ch=malloc(sizeof(char));
    while(1){
        
        int ch;
        int pos_x;
        int pos_y;
        int i, under;
        int total_score = 0;
        ProtoDirection  direction, bumped_direction;
        size_t packed_size;
        uint8_t* packed_buffer;
        bool verify = false;
        ProtoCharMessage *recvm = NULL;
        zmq_msg_t zmq_msg;
        zmq_msg_init(&zmq_msg);
    
        //pthread_mutex_lock(&lizard_dealer_mutex);
        packed_size=zmq_recvmsg(responder, &zmq_msg,0); 
        packed_buffer = zmq_msg_data(&zmq_msg);
        recvm=proto_char_message__unpack(NULL, packed_size, packed_buffer);
        //pthread_mutex_unlock(&lizard_dealer_mutex);

        if(recvm->msg_type == 0 || recvm->msg_type == 2)
            verify=true;

        if(recvm->msg_type == 1 || recvm->msg_type == 5){
            verify=false;
            i = find_ch_info(char_data, n_chars, *(recvm->ch));
            if(strcmp(char_data[i].password, recvm->password) != 0){
                verify = false;
                //printf("Rejected package with wrong password: %s is diff from %s\n", char_data[i].password, recvm->password);
                zmq_send (responder, packed_buffer, packed_size, 0);
                proto_char_message__free_unpacked(recvm, NULL);
                recvm=NULL;

            } else{verify = true;}
        
        }

        pthread_mutex_lock(&mutex_test1);
        if(verify == true){
        //new lizard character joins
        if(recvm->msg_type == 0){
            //max players
            if(n_chars == MAX_PLAYERS){
                *(recvm->ch) = 0;
                packed_size = proto_char_message__get_packed_size(recvm);
                packed_buffer = malloc(packed_size);
                proto_char_message__pack(recvm, packed_buffer);
                
                zmq_send (responder, packed_buffer, packed_size, 0);
                free(packed_buffer);
                packed_buffer=NULL;
                proto_char_message__free_unpacked(recvm, NULL);
                recvm=NULL;
            }else{
            //check if character is already in char_data
            ch = *(recvm->ch);          
            ch = available_char(n_chars, ch, char_data);
            *(recvm->ch) = ch;
            strcpy(char_data[n_chars].password, recvm->password);
            packed_size = proto_char_message__get_packed_size(recvm);
            packed_buffer = malloc(packed_size);
            proto_char_message__pack(recvm, packed_buffer);
            zmq_send (responder, packed_buffer, packed_size, 0);
            free(packed_buffer);
            packed_buffer=NULL;
            proto_char_message__free_unpacked(recvm, NULL);
            recvm=NULL;

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
            char_data[n_chars].lose = false;
            board[pos_y][pos_x].ch = ch;

            //initiate lizard going up
            direction = PROTO_DIRECTION__UP;
            char_data[n_chars].dir = PROTO_DIRECTION__UP;
            for(i = 1; i <= 5; i++){
                    if(pos_x + i < WINDOW_SIZE-1){
                        if(board[pos_y][pos_x + i].ch <= 46 && board[pos_y][pos_x + i].ch != 35){
                            wmove(my_win, pos_x + i, pos_y);
                            waddch(my_win, '.'| A_BOLD);
                            board[pos_y][pos_x + i].ch = '.';
                            *(m2.ch) = '.';
                            m2.posx = pos_x + i;
                            m2.posy = pos_y;
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;
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
            scoreboard(char_data, n_chars);
            wrefresh(my_win);	
            *(m2.ch) = ch;
            m2.posx = pos_x;
            m2.posy = pos_y;	
            packed_size = proto_display_message__get_packed_size(&m2);
            packed_buffer = malloc(packed_size);
            proto_display_message__pack(&m2, packed_buffer);
            zmq_send (publisher, packed_buffer, packed_size, 0);
            free(packed_buffer);
            packed_buffer=NULL;

            }
        }
        //lizard movement message
        else if(recvm->msg_type == 1){
            //check if current ch_pos is a winner
            bool winner = false;
            bool loser = false;
            int ch_pos = find_ch_info(char_data, n_chars, *(recvm->ch));
            int ch_pos2;
            if(ch_pos != -1){

                pos_x = char_data[ch_pos].pos_x;
                pos_y = char_data[ch_pos].pos_y;
                ch = char_data[ch_pos].ch;
                winner = char_data[ch_pos].win;
                loser = char_data[ch_pos].lose;
                //load old direction
                direction = char_data[ch_pos].dir;
                /*deletes old place */
                board[pos_y][pos_x].ch = ' ';
                wmove(my_win, pos_x, pos_y);
                waddch(my_win,' ');
                *(m2.ch) = ' ';
                m2.posx = pos_x;
                m2.posy = pos_y;	
                packed_size = proto_display_message__get_packed_size(&m2);
                packed_buffer = malloc(packed_size);
                proto_display_message__pack(&m2, packed_buffer);
                zmq_send (publisher, packed_buffer, packed_size, 0);
                free(packed_buffer);
                packed_buffer=NULL;
                //remove tail only if not a loser
            if(loser == false){
            switch (direction)
            {
            case UP:
                for(i = 1; i <= 5; i++){
                    if(pos_x + i < WINDOW_SIZE-1){
                        if(board[pos_y][pos_x + i].ch <= 46 && board[pos_y][pos_x + i].ch != 35){
                            under = what_under(board[pos_y][pos_x + i], pos_x + i, pos_y, cock_data, -1, winner);
                            if(under == 0){
                                wmove(my_win, pos_x + i, pos_y);
                                waddch(my_win, ' ');
                                board[pos_y][pos_x + i].ch = ' ';
                                    *(m2.ch) = ' ';
                                m2.posx = pos_x + i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                    zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
                            }else if(under == 7){
                                wmove(my_win, pos_x + i, pos_y);
                                waddch(my_win, '#'| A_BOLD);
                                board[pos_y][pos_x + i].ch = '#';
                                    *(m2.ch) = '#';
                                m2.posx = pos_x + i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                    zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
                            }else if(under == 8){
                                wmove(my_win, pos_x + i, pos_y);
                                waddch(my_win, '*'| A_BOLD);
                                board[pos_y][pos_x + i].ch = '*';
                                    *(m2.ch) = '*';
                                m2.posx = pos_x + i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                    zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
                            }else if(under == 9){
                                wmove(my_win, pos_x + i, pos_y);
                                waddch(my_win, '.'| A_BOLD);
                                board[pos_y][pos_x + i].ch = '.';
                                    *(m2.ch) = '.';
                                m2.posx = pos_x + i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                    zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
                            }else{
                                wmove(my_win, pos_x + i, pos_y);
                                waddch(my_win, (under + '0')| A_BOLD);
                                board[pos_y][pos_x + i].ch = under;
                                    *(m2.ch) = (char)(under + '0');
                                m2.posx = pos_x + i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                    zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
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
                        if(board[pos_y][pos_x - i].ch <= 46 && board[pos_y][pos_x - i].ch != 35){
                            under = what_under(board[pos_y][pos_x - i], pos_x - i, pos_y, cock_data, -1, winner);
                            if(under == 0){
                                wmove(my_win, pos_x - i, pos_y);
                                waddch(my_win, ' ');
                                board[pos_y][pos_x - i].ch = ' ';
                                    *(m2.ch) = ' ';
                                m2.posx = pos_x - i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                    zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
                            }else if(under == 7){
                                wmove(my_win, pos_x - i, pos_y);
                                waddch(my_win, '#'| A_BOLD);
                                board[pos_y][pos_x - i].ch = '#';
                                    *(m2.ch) = '#';
                                m2.posx = pos_x - i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                    zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
                            }else if(under == 8){
                                wmove(my_win, pos_x - i, pos_y);
                                waddch(my_win, '*'| A_BOLD);
                                board[pos_y][pos_x - i].ch = '*';
                                    *(m2.ch)= '*';
                                m2.posx = pos_x - i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                    zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
                            }else if(under == 9){
                                wmove(my_win, pos_x - i, pos_y);
                                waddch(my_win, '.'| A_BOLD);
                                board[pos_y][pos_x - i].ch = '.';
                                    *(m2.ch)= '.';
                                m2.posx = pos_x - i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                    zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
                            }else{
                                wmove(my_win, pos_x - i, pos_y);
                                waddch(my_win, (under + '0')| A_BOLD);
                                board[pos_y][pos_x - i].ch = under;
                                    *(m2.ch)= (char)(under + '0');
                                m2.posx = pos_x - i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                    zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
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
                        if(board[pos_y + i][pos_x].ch <= 46 && board[pos_y + i][pos_x].ch != 35){  
                            under = what_under(board[pos_y + i][pos_x], pos_x, pos_y + i, cock_data, -1, winner);
                            if(under == 0){
                                wmove(my_win, pos_x, pos_y + i);
                                waddch(my_win, ' ');
                                board[pos_y + i][pos_x].ch = ' ';
                                    *(m2.ch)= ' ';
                                m2.posx = pos_x;
                                m2.posy = pos_y + i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                    zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
                            }else if(under == 7){
                                wmove(my_win, pos_x, pos_y + i);
                                waddch(my_win, '#'| A_BOLD);
                                board[pos_y + i][pos_x].ch = '#';
                                    *(m2.ch)= '#';
                                m2.posx = pos_x;
                                m2.posy = pos_y + i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                    zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
                            }else if(under == 8){
                                wmove(my_win, pos_x, pos_y + i);
                                waddch(my_win, '*'| A_BOLD);
                                board[pos_y + i][pos_x].ch = '*';
                                    *(m2.ch)= '*';
                                m2.posx = pos_x;
                                m2.posy = pos_y + i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                    zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
                            }else if(under == 9){
                                wmove(my_win, pos_x, pos_y + i);
                                waddch(my_win, '.'| A_BOLD);
                                board[pos_y + i][pos_x].ch = '.';
                                    *(m2.ch)= '.';
                                m2.posx = pos_x;
                                m2.posy = pos_y + i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                    zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
                            }else{
                                wmove(my_win, pos_x, pos_y + i);
                                waddch(my_win, (under + '0')| A_BOLD);
                                board[pos_y + i][pos_x].ch = under;
                                    *(m2.ch)= (char)(under + '0');
                                m2.posx = pos_x;
                                m2.posy = pos_y + i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                    zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
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
                        if(board[pos_y - i][pos_x].ch <= 46 && board[pos_y - i][pos_x].ch != 35){
                            under = what_under(board[pos_y - i][pos_x], pos_x, pos_y - i, cock_data, -1, winner);
                            if(under == 0){
                                wmove(my_win, pos_x, pos_y - i);
                                waddch(my_win, ' ');
                                board[pos_y - i][pos_x].ch = ' ';
                                    *(m2.ch)= ' ';
                                m2.posx = pos_x;
                                m2.posy = pos_y - i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                    zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
                            }else if(under == 7){
                                wmove(my_win, pos_x, pos_y - i);
                                waddch(my_win, '#'| A_BOLD);
                                board[pos_y - i][pos_x].ch = '#';
                                    *(m2.ch)= '#';
                                m2.posx = pos_x;
                                m2.posy = pos_y - i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                    zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
                            }else if(under == 8){
                                wmove(my_win, pos_x, pos_y - i);
                                waddch(my_win, '*'| A_BOLD);
                                board[pos_y - i][pos_x].ch = '*';
                                    *(m2.ch)= '*';
                                m2.posx = pos_x;
                                m2.posy = pos_y - i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                    zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
                            }else if(under == 9){
                                wmove(my_win, pos_x, pos_y - i);
                                waddch(my_win, '.'| A_BOLD);
                                board[pos_y - i][pos_x].ch = '.';
                                    *(m2.ch)= '.';
                                m2.posx = pos_x;
                                m2.posy = pos_y - i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                    zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
                            }else{
                                wmove(my_win, pos_x, pos_y - i);
                                waddch(my_win, (under + '0')| A_BOLD);
                                board[pos_y - i][pos_x].ch = under;
                                    *(m2.ch)= (char)(under + '0');
                                m2.posx = pos_x;
                                m2.posy = pos_y - i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                    zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
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
            }   
                /* claculates new direction */
                direction = recvm->direction;
                char_data[ch_pos].dir = direction;
                m2.posx = pos_x;
                m2.posy = pos_y;
                /* claculates new mark position */
                new_position(&pos_x, &pos_y, direction);
                //collision with lizard, don't change position
                if(board[pos_y][pos_x].ch > 46 && board[pos_y][pos_x].ch != ch){
                    //half of total score
                    ch_pos2 = find_ch_info(char_data, n_chars, board[pos_y][pos_x].ch);
                    total_score = char_data[ch_pos].score + char_data[ch_pos2].score;
                    char_data[ch_pos].score = total_score/2;
                    char_data[ch_pos2].score = total_score/2;
                    bumped_direction = char_data[ch_pos2].dir;

                    //if the bumped lizard was not a loser and now it is
                    if(char_data[ch_pos2].score < 0 && char_data[ch_pos2].lose == false){
                        char_data[ch_pos2].win = false;
                        char_data[ch_pos2].lose = true;
                        //TODO: apagar a cauda do outro (ch_pos2) lizard
                        //delete from board
            switch (bumped_direction)
            {
            case UP:
                for(i = 1; i <= 5; i++){
                    if(pos_x + i < WINDOW_SIZE-1){
                        if(board[pos_y][pos_x + i].ch <= 46){
                            under = what_under(board[pos_y][pos_x + i], pos_x + i, pos_y, cock_data, -1, winner);
                            if(under == 0){
                                wmove(my_win, pos_x + i, pos_y);
                                waddch(my_win, ' ');
                                board[pos_y][pos_x + i].ch = ' ';
                                *(m2.ch)= ' ';
                                m2.posx = pos_x + i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
                            }else if(under == 7){
                                wmove(my_win, pos_x + i, pos_y);
                                waddch(my_win, '#'| A_BOLD);
                                board[pos_y][pos_x + i].ch = '#';
                                *(m2.ch)= '#';
                                m2.posx = pos_x + i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 8){
                                wmove(my_win, pos_x + i, pos_y);
                                waddch(my_win, '*'| A_BOLD);
                                board[pos_y][pos_x + i].ch = '*';
                                *(m2.ch)= '*';
                                m2.posx = pos_x + i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 9){
                                wmove(my_win, pos_x + i, pos_y);
                                waddch(my_win, '.'| A_BOLD);
                                board[pos_y][pos_x + i].ch = '.';
                                *(m2.ch)= '.';
                                m2.posx = pos_x + i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else{
                                wmove(my_win, pos_x + i, pos_y);
                                waddch(my_win, (under + '0')| A_BOLD);
                                board[pos_y][pos_x + i].ch = under;
                                *(m2.ch)= (char)(under + '0');
                                m2.posx = pos_x + i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }
                        }    
                        board[pos_y][pos_x + i].nlayers--;
                        if(char_data[ch_pos2].win == true)
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
                            under = what_under(board[pos_y][pos_x - i], pos_x - i, pos_y, cock_data, -1, winner);
                            if(under == 0){
                                wmove(my_win, pos_x - i, pos_y);
                                waddch(my_win, ' ');
                                board[pos_y][pos_x - i].ch = ' ';
                                *(m2.ch)= ' ';
                                m2.posx = pos_x - i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 7){
                                wmove(my_win, pos_x - i, pos_y);
                                waddch(my_win, '#'| A_BOLD);
                                board[pos_y][pos_x - i].ch = '#';
                                *(m2.ch)= '#';
                                m2.posx = pos_x - i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 8){
                                wmove(my_win, pos_x - i, pos_y);
                                waddch(my_win, '*'| A_BOLD);
                                board[pos_y][pos_x - i].ch = '*';
                                *(m2.ch)= '*';
                                m2.posx = pos_x - i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 9){
                                wmove(my_win, pos_x - i, pos_y);
                                waddch(my_win, '.'| A_BOLD);
                                board[pos_y][pos_x - i].ch = '.';
                                *(m2.ch)= '.';
                                m2.posx = pos_x - i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
                            }else{
                                wmove(my_win, pos_x - i, pos_y);
                                waddch(my_win, (under + '0')| A_BOLD);
                                board[pos_y][pos_x - i].ch = under;
                                *(m2.ch)= (char)(under + '0');
                                m2.posx = pos_x - i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
                            }
                        }
                        board[pos_y][pos_x - i].nlayers--;
                        if(char_data[ch_pos2].win == true)
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
                            under = what_under(board[pos_y + i][pos_x], pos_x, pos_y + i, cock_data, -1, winner);
                            if(under == 0){
                                wmove(my_win, pos_x, pos_y + i);
                                waddch(my_win, ' ');
                                board[pos_y + i][pos_x].ch = ' ';
                                *(m2.ch)= ' ';
                                m2.posx = pos_x;
                                m2.posy = pos_y + i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 7){
                                wmove(my_win, pos_x, pos_y + i);
                                waddch(my_win, '#'| A_BOLD);
                                board[pos_y + i][pos_x].ch = '#';
                                *(m2.ch)= '#';
                                m2.posx = pos_x;
                                m2.posy = pos_y + i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 8){
                                wmove(my_win, pos_x, pos_y + i);
                                waddch(my_win, '*'| A_BOLD);
                                board[pos_y + i][pos_x].ch = '*';
                                *(m2.ch)= '*';
                                m2.posx = pos_x;
                                m2.posy = pos_y + i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 9){
                                wmove(my_win, pos_x, pos_y + i);
                                waddch(my_win, '.'| A_BOLD);
                                board[pos_y + i][pos_x].ch = '.';
                                *(m2.ch)= '.';
                                m2.posx = pos_x;
                                m2.posy = pos_y + i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else{
                                wmove(my_win, pos_x, pos_y + i);
                                waddch(my_win, (under + '0')| A_BOLD);
                                board[pos_y + i][pos_x].ch = under;
                                *(m2.ch)= (char)(under + '0');
                                m2.posx = pos_x;
                                m2.posy = pos_y + i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }
                        }
                        board[pos_y + i][pos_x].nlayers--;
                        if(char_data[ch_pos2].win == true)
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
                            under = what_under(board[pos_y - i][pos_x], pos_x, pos_y - i, cock_data, -1, winner);
                            if(under == 0){
                                wmove(my_win, pos_x, pos_y - i);
                                waddch(my_win, ' ');
                                board[pos_y - i][pos_x].ch = ' ';
                                *(m2.ch)= ' ';
                                m2.posx = pos_x;
                                m2.posy = pos_y - i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 7){
                                wmove(my_win, pos_x, pos_y - i);
                                waddch(my_win, '#'| A_BOLD);
                                board[pos_y - i][pos_x].ch = '#';
                                *(m2.ch)= '#';
                                m2.posx = pos_x;
                                m2.posy = pos_y - i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 8){
                                wmove(my_win, pos_x, pos_y - i);
                                waddch(my_win, '*'| A_BOLD);
                                board[pos_y - i][pos_x].ch = '*';
                                *(m2.ch)= '*';
                                m2.posx = pos_x;
                                m2.posy = pos_y - i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 9){
                                wmove(my_win, pos_x, pos_y - i);
                                waddch(my_win, '.'| A_BOLD);
                                board[pos_y - i][pos_x].ch = '.';
                                *(m2.ch)= '.';
                                m2.posx = pos_x;
                                m2.posy = pos_y - i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else{
                                wmove(my_win, pos_x, pos_y - i);
                                waddch(my_win, (under + '0')| A_BOLD);
                                board[pos_y - i][pos_x].ch = under;
                                *(m2.ch)= (char)(under + '0');
                                m2.posx = pos_x;
                                m2.posy = pos_y - i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }
                        }
                        board[pos_y - i][pos_x].nlayers--;    
                        if(char_data[ch_pos2].win == true)
                            board[pos_y - i][pos_x].wlayers--;
                    }else{
                        break;
                    }
                }
                break;
            default:
                break;
            }
            
                    }
                    //if bumped lizard is now a winner and wasn't a loser
                    else if(char_data[ch_pos2].score >= 50 && char_data[ch_pos2].lose == false){
                        char_data[ch_pos2].win = true;
                    }

                    //don't change position
                    pos_x = char_data[ch_pos].pos_x;
                    pos_y = char_data[ch_pos].pos_y;

                    if(char_data[ch_pos].score < 0 && char_data[ch_pos].lose == false){
                        char_data[ch_pos].win = false;
                        char_data[ch_pos].lose = true;
                    }

                }else if(board[pos_y][pos_x].ch == 35){ //bumped a wasp
                    //reduce score by 10
                    char_data[ch_pos].score = char_data[ch_pos].score - 10;

                    if(char_data[ch_pos].score < 0 && char_data[ch_pos].lose == false){
                        char_data[ch_pos].win = false;
                        char_data[ch_pos].lose = true;
                    }

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
                            cock_data[i].posx = random()%(WINDOW_SIZE-2) + 1;
                            cock_data[i].posy = random()%(WINDOW_SIZE-2) + 1;
                        }
                    }
                }
                //winning
                if(char_data[ch_pos].score >= 50 && char_data[ch_pos].lose == false)
                    char_data[ch_pos].win = true;

                //send score back to lizard
                recvm->ncock = char_data[ch_pos].score;
                packed_size = proto_char_message__get_packed_size(recvm);
                packed_buffer = malloc(packed_size);
                proto_char_message__pack(recvm, packed_buffer);
                zmq_send (responder, packed_buffer, packed_size, 0);
                free(packed_buffer);
                packed_buffer=NULL;
                proto_char_message__free_unpacked(recvm, NULL);
                recvm=NULL;


            }
            //draw lizard tail
            if(char_data[ch_pos].lose == false){
            switch (direction)
            {
            case UP:
                for(i = 1; i <= 5; i++){
                    if(pos_x + i < WINDOW_SIZE-1){
                        if(board[pos_y][pos_x + i].ch <= 32){
                            wmove(my_win, pos_x + i, pos_y);
                            if(char_data[ch_pos].win == true){
                                board[pos_y][pos_x + i].ch = '*';
                                waddch(my_win, '*'| A_BOLD);
                                *(m2.ch) = '*';
                            }else{
                                board[pos_y][pos_x + i].ch = '.';
                                waddch(my_win, '.'| A_BOLD);
                                *(m2.ch) = '.';
                            }
                            m2.posx = pos_x + i;
                            m2.posy = pos_y;	
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;
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
                            wmove(my_win, pos_x - i, pos_y);
                            if(char_data[ch_pos].win == true){
                                board[pos_y][pos_x - i].ch = '*';
                                waddch(my_win, '*'| A_BOLD);
                                *(m2.ch) = '*';
                            }else{
                                board[pos_y][pos_x - i].ch = '.';
                                waddch(my_win, '.'| A_BOLD);
                                *(m2.ch) = '.';
                            }
                            m2.posx = pos_x - i;
                            m2.posy = pos_y;	
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;
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
                            wmove(my_win, pos_x, pos_y + i);
                            if(char_data[ch_pos].win == true){
                                board[pos_y + i][pos_x].ch = '*';
                                waddch(my_win, '*'| A_BOLD);
                                *(m2.ch) = '*';
                            }else{
                                board[pos_y + i][pos_x].ch = '.';
                                waddch(my_win, '.'| A_BOLD);
                                *(m2.ch) = '.';
                            }
                            m2.posx = pos_x;
                            m2.posy = pos_y + i;	
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;
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
                            wmove(my_win, pos_x, pos_y - i);
                            if(char_data[ch_pos].win == true){
                                board[pos_y - i][pos_x].ch = '*';
                                waddch(my_win, '*'| A_BOLD);
                               *( m2.ch) = '*';
                            }else{
                                board[pos_y - i][pos_x].ch = '.';
                                waddch(my_win, '.'| A_BOLD);
                                *(m2.ch) = '.';
                            }
                            m2.posx = pos_x;
                            m2.posy = pos_y - i;	
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;
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
            }
            /* draw mark on new position */
            board[pos_y][pos_x].ch = ch;
            scoreboard(char_data, n_chars);
            //send scoreboard
            m2.posx = 999; //flag for scoreboard
            m2.posy = n_chars; //send number of characters
            packed_size = proto_display_message__get_packed_size(&m2);
            packed_buffer = malloc(packed_size);
            proto_display_message__pack(&m2, packed_buffer);
            zmq_send (publisher, packed_buffer, packed_size, 0);
            free(packed_buffer);
            packed_buffer=NULL;
            for(i = 0; i < n_chars; i++){
                m2.posx = char_data[i].ch;
                //*(m2.ch)= (char)(char_data[i].ch + '0');
                m2.score = char_data[i].score;
                packed_size = proto_display_message__get_packed_size(&m2);
                packed_buffer = malloc(packed_size);
                proto_display_message__pack(&m2, packed_buffer);
                zmq_send (publisher, packed_buffer, packed_size, 0);
                free(packed_buffer);
                packed_buffer=NULL;
            }
            refresh();
            box(my_win, 0 , 0);
            wmove(my_win, pos_x, pos_y);
            waddch(my_win, ch| A_BOLD);
            wrefresh(my_win);	
            *(m2.ch) = ch;		
            m2.posx = pos_x;
            m2.posy = pos_y;
            packed_size = proto_display_message__get_packed_size(&m2);
            packed_buffer = malloc(packed_size);
            proto_display_message__pack(&m2, packed_buffer);
            zmq_send (publisher, packed_buffer, packed_size, 0);
            free(packed_buffer);
            packed_buffer=NULL;
    
        }
        //remote display joins
        else if(recvm->msg_type == 2){
            //send the board with the printed chars
            zmq_send (responder, &board, sizeof(board), 0); //Probably a new board needs to be created in protobuf.

        }
        //lizard disconnect
        else if(recvm->msg_type == 5){
            packed_size = proto_char_message__get_packed_size(recvm);
            packed_buffer = malloc(packed_size);
            proto_char_message__pack(recvm, packed_buffer);
            zmq_send (responder, packed_buffer, packed_size, 0);
            free(packed_buffer);
            packed_buffer=NULL;
            
            int ch_pos = find_ch_info(char_data, n_chars, *(recvm->ch));
            
            proto_char_message__free_unpacked(recvm, NULL);
            recvm=NULL;
            
            bool winner = char_data[ch_pos].win;
            pos_x = char_data[ch_pos].pos_x;
            pos_y = char_data[ch_pos].pos_y;
            ch = char_data[ch_pos].ch;
            direction = char_data[ch_pos].dir;
            board[pos_y][pos_x].ch = ' ';
            wmove(my_win, pos_x, pos_y);
            waddch(my_win,' ');
            *(m2.ch)= ' ';
            m2.posx = pos_x;
            m2.posy = pos_y;		
            packed_size = proto_display_message__get_packed_size(&m2);
            packed_buffer = malloc(packed_size);
            proto_display_message__pack(&m2, packed_buffer);
            zmq_send (publisher, packed_buffer, packed_size, 0);
            free(packed_buffer);
            packed_buffer=NULL;
            //delete from board
            switch (direction)
            {
            case UP:
                for(i = 1; i <= 5; i++){
                    if(pos_x + i < WINDOW_SIZE-1){
                        if(board[pos_y][pos_x + i].ch <= 46){
                            under = what_under(board[pos_y][pos_x + i], pos_x + i, pos_y, cock_data, -1, winner);
                            if(under == 0){
                                wmove(my_win, pos_x + i, pos_y);
                                waddch(my_win, ' ');
                                board[pos_y][pos_x + i].ch = ' ';
                                *(m2.ch)= ' ';
                                m2.posx = pos_x + i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
                            }else if(under == 7){
                                wmove(my_win, pos_x + i, pos_y);
                                waddch(my_win, '#'| A_BOLD);
                                board[pos_y][pos_x + i].ch = '#';
                                *(m2.ch)= '#';
                                m2.posx = pos_x + i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 8){
                                wmove(my_win, pos_x + i, pos_y);
                                waddch(my_win, '*'| A_BOLD);
                                board[pos_y][pos_x + i].ch = '*';
                                *(m2.ch)= '*';
                                m2.posx = pos_x + i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 9){
                                wmove(my_win, pos_x + i, pos_y);
                                waddch(my_win, '.'| A_BOLD);
                                board[pos_y][pos_x + i].ch = '.';
                                *(m2.ch)= '.';
                                m2.posx = pos_x + i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else{
                                wmove(my_win, pos_x + i, pos_y);
                                waddch(my_win, (under + '0')| A_BOLD);
                                board[pos_y][pos_x + i].ch = under;
                                *(m2.ch)= (char)(under + '0');
                                m2.posx = pos_x + i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

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
                            under = what_under(board[pos_y][pos_x - i], pos_x - i, pos_y, cock_data, -1, winner);
                            if(under == 0){
                                wmove(my_win, pos_x - i, pos_y);
                                waddch(my_win, ' ');
                                board[pos_y][pos_x - i].ch = ' ';
                                *(m2.ch)= ' ';
                                m2.posx = pos_x - i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 7){
                                wmove(my_win, pos_x - i, pos_y);
                                waddch(my_win, '#'| A_BOLD);
                                board[pos_y][pos_x - i].ch = '#';
                                *(m2.ch)= '#';
                                m2.posx = pos_x - i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 8){
                                wmove(my_win, pos_x - i, pos_y);
                                waddch(my_win, '*'| A_BOLD);
                                board[pos_y][pos_x - i].ch = '*';
                                *(m2.ch)= '*';
                                m2.posx = pos_x - i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 9){
                                wmove(my_win, pos_x - i, pos_y);
                                waddch(my_win, '.'| A_BOLD);
                                board[pos_y][pos_x - i].ch = '.';
                                *(m2.ch)= '.';
                                m2.posx = pos_x - i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
                            }else{
                                wmove(my_win, pos_x - i, pos_y);
                                waddch(my_win, (under + '0')| A_BOLD);
                                board[pos_y][pos_x - i].ch = under;
                                *(m2.ch)= (char)(under + '0');
                                m2.posx = pos_x - i;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
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
                            under = what_under(board[pos_y + i][pos_x], pos_x, pos_y + i, cock_data, -1, winner);
                            if(under == 0){
                                wmove(my_win, pos_x, pos_y + i);
                                waddch(my_win, ' ');
                                board[pos_y + i][pos_x].ch = ' ';
                                *(m2.ch)= ' ';
                                m2.posx = pos_x;
                                m2.posy = pos_y + i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 7){
                                wmove(my_win, pos_x, pos_y + i);
                                waddch(my_win, '#'| A_BOLD);
                                board[pos_y + i][pos_x].ch = '#';
                                *(m2.ch)= '#';
                                m2.posx = pos_x;
                                m2.posy = pos_y + i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 8){
                                wmove(my_win, pos_x, pos_y + i);
                                waddch(my_win, '*'| A_BOLD);
                                board[pos_y + i][pos_x].ch = '*';
                                *(m2.ch)= '*';
                                m2.posx = pos_x;
                                m2.posy = pos_y + i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 9){
                                wmove(my_win, pos_x, pos_y + i);
                                waddch(my_win, '.'| A_BOLD);
                                board[pos_y + i][pos_x].ch = '.';
                                *(m2.ch)= '.';
                                m2.posx = pos_x;
                                m2.posy = pos_y + i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else{
                                wmove(my_win, pos_x, pos_y + i);
                                waddch(my_win, (under + '0')| A_BOLD);
                                board[pos_y + i][pos_x].ch = under;
                                *(m2.ch)= (char)(under + '0');
                                m2.posx = pos_x;
                                m2.posy = pos_y + i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

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
                            under = what_under(board[pos_y - i][pos_x], pos_x, pos_y - i, cock_data, -1, winner);
                            if(under == 0){
                                wmove(my_win, pos_x, pos_y - i);
                                waddch(my_win, ' ');
                                board[pos_y - i][pos_x].ch = ' ';
                                *(m2.ch)= ' ';
                                m2.posx = pos_x;
                                m2.posy = pos_y - i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 7){
                                wmove(my_win, pos_x, pos_y - i);
                                waddch(my_win, '#'| A_BOLD);
                                board[pos_y - i][pos_x].ch = '#';
                                *(m2.ch)= '#';
                                m2.posx = pos_x;
                                m2.posy = pos_y - i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 8){
                                wmove(my_win, pos_x, pos_y - i);
                                waddch(my_win, '*'| A_BOLD);
                                board[pos_y - i][pos_x].ch = '*';
                                *(m2.ch)= '*';
                                m2.posx = pos_x;
                                m2.posy = pos_y - i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 9){
                                wmove(my_win, pos_x, pos_y - i);
                                waddch(my_win, '.'| A_BOLD);
                                board[pos_y - i][pos_x].ch = '.';
                                *(m2.ch)= '.';
                                m2.posx = pos_x;
                                m2.posy = pos_y - i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else{
                                wmove(my_win, pos_x, pos_y - i);
                                waddch(my_win, (under + '0')| A_BOLD);
                                board[pos_y - i][pos_x].ch = under;
                                *(m2.ch)= (char)(under + '0');
                                m2.posx = pos_x;
                                m2.posy = pos_y - i;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

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
            for(i = ch_pos; i <= (n_chars-1); i++){
                char_data[i].ch = char_data[i+1].ch;
                char_data[i].pos_x = char_data[i+1].pos_x;
                char_data[i].pos_y = char_data[i+1].pos_y;
                char_data[i].dir = char_data[i+1].dir;
                char_data[i].score = char_data[i+1].score;
                char_data[i].win = char_data[i+1].win;
                char_data[i].lose = char_data[i+1].lose;
                strcpy(char_data[i].password,char_data[i+1].password);
                memset(char_data[i+1].password,'\0', sizeof(char_data[i+1].password));

            }
            n_chars--;
            scoreboard(char_data, n_chars);
            //send scoreboard
            m2.posx = 999; //flag for scoreboard
            m2.posy = n_chars; //send number of characters
            packed_size = proto_display_message__get_packed_size(&m2);
            packed_buffer = malloc(packed_size);
            proto_display_message__pack(&m2, packed_buffer);
            zmq_send (publisher, packed_buffer, packed_size, 0);
            free(packed_buffer);
            packed_buffer=NULL;

            for(i = 0; i < n_chars; i++){
                m2.posx = char_data[i].ch;
                //*(m2.ch)= (char)(char_data[i].ch + '0');
                m2.score = char_data[i].score;
                packed_size = proto_display_message__get_packed_size(&m2);
                packed_buffer = malloc(packed_size);
                proto_display_message__pack(&m2, packed_buffer);
                zmq_send (publisher, packed_buffer, packed_size, 0);
                free(packed_buffer);
                packed_buffer=NULL;
            }
            refresh();
            box(my_win, 0 , 0);
            wrefresh(my_win);
        }
        }
        pthread_mutex_unlock(&mutex_test1);
        zmq_msg_close(&zmq_msg);
    }

    return 0;
}

//roaches and wasps handling thread function
void *roach_function(void *context){
    ProtoDisplayMessage m2;
    proto_display_message__init(&m2);
    m2.ch=malloc(sizeof(char));
    void *responder = zmq_socket(context, ZMQ_REP);
    zmq_connect(responder, "inproc://mydealer2");
    while(1){

        int ch;
        int pos_x;
        int pos_y;
        int save_x, save_y;
        int i, k, l, under;
        int ncock, fcock;
        bool verify = false;
        ProtoDirection  direction;
        size_t packed_size;
        uint8_t* packed_buffer;
        ProtoCharMessage *recvm = NULL;
        zmq_msg_t zmq_msg;
        zmq_msg_init(&zmq_msg);

        packed_size=zmq_recvmsg(responder, &zmq_msg,0); 
        packed_buffer = zmq_msg_data(&zmq_msg);
        recvm=proto_char_message__unpack(NULL, packed_size, packed_buffer);

        if(recvm->msg_type == 3 || recvm->msg_type == 6)
            verify=true;

        if(recvm->msg_type == 4 || recvm->msg_type == 7 || recvm->msg_type == 8 || recvm->msg_type == 9){
            verify=false;
            for(i = *(recvm->ch); i < *(recvm->ch) + (recvm->ncock); i++){
                if(strcmp(cock_data[i].password, recvm->password) != 0){
                    verify = false;
                    zmq_send (responder, packed_buffer, packed_size, 0);
                    proto_char_message__free_unpacked(recvm, NULL);
                    recvm=NULL;
                    break;
                }else{verify = true;}
            }
        }

        pthread_mutex_lock(&mutex_test1);
        if(verify == true){
        //cockroach joins
        if(recvm->msg_type == 3){
            //number of cockroaches from this user
            ncock = recvm->ncock;
            //check if there is no more space for cockroaches
            if(total_cock + ncock > MAX_COCK){
                //"too many insects" and close the client
                recvm->ncock = 0;
                packed_size = proto_char_message__get_packed_size(recvm);
                packed_buffer = malloc(packed_size);
                proto_char_message__pack(recvm, packed_buffer);
                zmq_send (responder, packed_buffer, packed_size, 0);
                free(packed_buffer);
                packed_buffer=NULL; 
                proto_char_message__free_unpacked(recvm, NULL);
                recvm=NULL;
                }else{                
                //random position for each cockroach
                i = 0;
                while(i != ncock){
                    k = random()%(WINDOW_SIZE-2) + 1;
                    l = random()%(WINDOW_SIZE-2) + 1;
                    //if it is a free space, a lizard body or another cockroach
                    if(board[l][k].ch <= 46 && board[l][k].ch != 35){
                        //save the cockroach data
                        ch = random()%5 + 1;
                        cock_data[total_cock + i].value = ch;
                        cock_data[total_cock + i].posx = k;
                        cock_data[total_cock + i].posy = l;
                        board[l][k].ch = ch;
                        strcpy(cock_data[total_cock + i].password, recvm->password);
                        //print the cockroach
                        wmove(my_win, k, l);
                        waddch(my_win, (ch + '0')| A_BOLD);
                        //send to displays
                        *(m2.ch)= (char)(ch + '0');
                        m2.posx = k;
                        m2.posy = l;		
                        packed_size = proto_display_message__get_packed_size(&m2);
                        packed_buffer = malloc(packed_size);
                        proto_display_message__pack(&m2, packed_buffer);
                        zmq_send (publisher, packed_buffer, packed_size, 0);
                        free(packed_buffer);
                        packed_buffer=NULL;
                        i++;
                    }
                }
                wrefresh(my_win);
                //send the position of the first cockroach in the data array
                *(recvm->ch) = total_cock;
                packed_size = proto_char_message__get_packed_size(recvm);
                packed_buffer = malloc(packed_size);
                proto_char_message__pack(recvm, packed_buffer);
                zmq_send (responder, packed_buffer, packed_size, 0);
                free(packed_buffer);
                packed_buffer=NULL;                
                proto_char_message__free_unpacked(recvm, NULL);
                recvm=NULL;
                total_cock = total_cock + ncock;
            }
        }
        //cockroach movement
        else if(recvm->msg_type == 4){
            ncock = recvm->ncock;
            fcock = (int) *(recvm->ch);
            bool winner = false;
            for(i = 0; i < ncock; i++){
                //load previous data
                pos_x = cock_data[fcock + i].posx;
                pos_y = cock_data[fcock + i].posy;
                ch = cock_data[fcock + i].value;
                direction = recvm->cockdir[i];
            if((time(&start_time)-cock_data[fcock + i].time_eaten)>=5){
                switch (direction)
                {
                case UP:
                    if(pos_x - 1 > 0 && board[pos_y][pos_x - 1].ch <= 46 && board[pos_y][pos_x - 1].ch != 35){
                        under = what_under(board[pos_y][pos_x], pos_x, pos_y, cock_data, fcock + i, winner);
                        if(under == 0){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, ' ');
                            board[pos_y][pos_x].ch = ' ';
                            *(m2.ch)= ' ';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;
                        }else if(under == 8){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, '*'| A_BOLD);
                            board[pos_y][pos_x].ch = '*';
                            *(m2.ch)= '*';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;
                        }else if(under == 9){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, '.'| A_BOLD);
                            board[pos_y][pos_x].ch = '.';
                            *(m2.ch)= '.';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

                        }else{
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, (under + '0')| A_BOLD);
                            board[pos_y][pos_x].ch = under;
                            *(m2.ch)= (char)(under + '0');
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

                        }
                        pos_x--;
                    }
                    break;
                case DOWN:
                    if(pos_x + 1 < WINDOW_SIZE-1 && board[pos_y][pos_x + 1].ch <= 46 && board[pos_y][pos_x + 1].ch != 35){
                        under = what_under(board[pos_y][pos_x], pos_x, pos_y, cock_data, fcock + i, winner);
                        if(under == 0){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, ' ');
                            board[pos_y][pos_x].ch = ' ';
                            *(m2.ch)= ' ';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

                        }else if(under == 8){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, '*'| A_BOLD);
                            board[pos_y][pos_x].ch = '*';
                            *(m2.ch)= '*';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

                        }else if(under == 9){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, '.'| A_BOLD);
                            board[pos_y][pos_x].ch = '.';
                            *(m2.ch)= '.';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;
                        }else{
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, (under + '0')| A_BOLD);
                            board[pos_y][pos_x].ch = under;
                            *(m2.ch)= (char)(under + '0');
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

                        }
                        pos_x++;
                    }
                    break;
                case LEFT:
                    if(pos_y - 1 > 0 && board[pos_y - 1][pos_x].ch <= 46 && board[pos_y - 1][pos_x].ch != 35){
                        under = what_under(board[pos_y][pos_x], pos_x, pos_y, cock_data, fcock + i, winner);
                        if(under == 0){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, ' ');
                            board[pos_y][pos_x].ch = ' ';
                            *(m2.ch)= ' ';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

                        }else if(under == 8){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, '*'| A_BOLD);
                            board[pos_y][pos_x].ch = '*';
                            *(m2.ch)= '*';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

                        }else if(under == 9){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, '.'| A_BOLD);
                            board[pos_y][pos_x].ch = '.';
                            *(m2.ch)= '.';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

                        }else{
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, (under + '0')| A_BOLD);
                            board[pos_y][pos_x].ch = under;
                            *(m2.ch)= (char)(under + '0');
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

                        }
                        pos_y--;
                    }
                    break;
                case RIGHT:
                    if(pos_y + 1 < WINDOW_SIZE-1 && board[pos_y + 1][pos_x].ch <= 46 && board[pos_y + 1][pos_x].ch != 35){
                        under = what_under(board[pos_y][pos_x], pos_x, pos_y, cock_data, fcock + i, winner);
                        if(under == 0){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, ' ');
                            board[pos_y][pos_x].ch = ' ';
                            *(m2.ch)= ' ';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

                        }else if(under == 8){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, '*'| A_BOLD);
                            board[pos_y][pos_x].ch = '*';
                            *(m2.ch)= '*';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

                        }else if(under == 9){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, '.'| A_BOLD);
                            board[pos_y][pos_x].ch = '.';
                            *(m2.ch)= '.';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

                        }else{
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, (under + '0')| A_BOLD);
                            board[pos_y][pos_x].ch = under;
                            *(m2.ch)= (char)(under + '0');
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

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
                *(m2.ch)= (char)(ch + '0');
                m2.posx = pos_x;
                m2.posy = pos_y;		
                packed_size = proto_display_message__get_packed_size(&m2);
                packed_buffer = malloc(packed_size);
                proto_display_message__pack(&m2, packed_buffer);
                zmq_send (publisher, packed_buffer, packed_size, 0);
                free(packed_buffer);
                packed_buffer=NULL;

            }
            }
            wrefresh(my_win);
            packed_size = proto_char_message__get_packed_size(recvm);
            packed_buffer = malloc(packed_size);
            proto_char_message__pack(recvm, packed_buffer);
            zmq_send (responder, packed_buffer, packed_size, 0);
            free(packed_buffer);
            packed_buffer=NULL;
            proto_char_message__free_unpacked(recvm, NULL);
            recvm=NULL;
        } 
        //wasp joins
        else if(recvm->msg_type == 6){
            //number of wasps from this user
            ncock = recvm->ncock;
            printf("%d", ncock);
            //check if there is no more space for wasps
            if(total_cock + ncock > MAX_COCK){
                //"too many insects" and close the client
                recvm->ncock = 0;
                packed_size = proto_char_message__get_packed_size(recvm);
                packed_buffer = malloc(packed_size);
                proto_char_message__pack(recvm, packed_buffer);
                zmq_send (responder, packed_buffer, packed_size, 0);
                free(packed_buffer);
                packed_buffer=NULL;
                proto_char_message__free_unpacked(recvm, NULL);
                recvm=NULL;

            }else{                
                //random position for each cockroach
                i = 0;
                while(i != ncock){
                    k = random()%(WINDOW_SIZE-2) + 1;
                    l = random()%(WINDOW_SIZE-2) + 1;
                    //if it is a free spaceor a lizard body
                    if((board[l][k].ch == 32) || (board[l][k].ch == 42) || (board[l][k].ch == 46)){
                        //save the wasp data
                        cock_data[total_cock + i].value = '#';
                        cock_data[total_cock + i].posx = k;
                        cock_data[total_cock + i].posy = l;
                        board[l][k].ch = '#';
                        strcpy(cock_data[total_cock + i].password, recvm->password);
                        //print the wasp
                        wmove(my_win, k, l);
                        waddch(my_win, '#'| A_BOLD);
                        //send to displays
                        *(m2.ch)= '#';
                        m2.posx = k;
                        m2.posy = l;		
                        packed_size = proto_display_message__get_packed_size(&m2);
                        packed_buffer = malloc(packed_size);
                        proto_display_message__pack(&m2, packed_buffer);
                        zmq_send (publisher, packed_buffer, packed_size, 0);
                        free(packed_buffer);
                        packed_buffer=NULL;
                        i++;
                    }
                }
                wrefresh(my_win);
                //send the position of the first cockroach in the data array
                *(recvm->ch) = total_cock;
                packed_size = proto_char_message__get_packed_size(recvm);
                packed_buffer = malloc(packed_size);
                proto_char_message__pack(recvm, packed_buffer);
                zmq_send (responder, packed_buffer, packed_size, 0);
                free(packed_buffer);
                packed_buffer=NULL;
                proto_char_message__free_unpacked(recvm, NULL);
                recvm=NULL;
                //update the number of cockroaches in the server
                total_cock = total_cock + ncock;
            }
        }
        //wasp movement
        else if(recvm->msg_type == 7){
            ncock = recvm->ncock;
            fcock = (int) *(recvm->ch);
            int ch_pos;
            bool winner = false;
            bool bumped; //if it bumped a lizard head change to true
            bool loser = false;
            for(i = 0; i < ncock; i++){
                bumped = false;
                loser = false;
                //load previous data
                pos_x = cock_data[fcock + i].posx;
                pos_y = cock_data[fcock + i].posy;
                save_x = pos_x;
                save_y = pos_y;
                ch = '#';
                direction = recvm->cockdir[i];

                switch (direction)
                {
                case UP:
                    //decrease lizard score if there's a collision
                    if(pos_x - 1 > 0 && board[pos_y][pos_x - 1].ch != 35 && board[pos_y][pos_x - 1].ch >= 32){
                        //check if lizard head
                        if(board[pos_y][pos_x - 1].ch <= 46){
                        under = what_under(board[pos_y][pos_x], pos_x, pos_y, cock_data, -2, winner);
                        if(under == 0 || under == 7){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, ' ');
                            board[pos_y][pos_x].ch = ' ';
                            *(m2.ch)= ' ';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;
                        }else if(under == 8){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, '*'| A_BOLD);
                            board[pos_y][pos_x].ch = '*';
                            *(m2.ch)= '*';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

                        }else if(under == 9){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, '.'| A_BOLD);
                            board[pos_y][pos_x].ch = '.';
                            *(m2.ch)= '.';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

                        }else{
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, (under + '0')| A_BOLD);
                            board[pos_y][pos_x].ch = under;
                            *(m2.ch)= (char)(under + '0');
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

                        }
                        pos_x--;
                        }
                        //bumped lizard head
                        else{
                            ch_pos = find_ch_info(char_data, n_chars, board[pos_y][pos_x - 1].ch);
                            char_data[ch_pos].score = char_data[ch_pos].score - 10;
                            bumped = true;
                            loser = char_data[ch_pos].lose;
                            if(char_data[ch_pos].score < 0){
                                char_data[ch_pos].lose = true;
                                char_data[ch_pos].win = false;
                            }
                            pos_x = char_data[ch_pos].pos_x;
                            pos_y = char_data[ch_pos].pos_y;
                        }
                    }
                    break;
                case DOWN:
                    if(pos_x + 1 < WINDOW_SIZE-1 && board[pos_y][pos_x + 1].ch != 35 && board[pos_y][pos_x + 1].ch >= 32){
                        if(board[pos_y][pos_x + 1].ch <= 46){
                        under = what_under(board[pos_y][pos_x], pos_x, pos_y, cock_data, -2, winner);
                        if(under == 0 || under == 7){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, ' ');
                            board[pos_y][pos_x].ch = ' ';
                            *(m2.ch)= ' ';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

                        }else if(under == 8){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, '*'| A_BOLD);
                            board[pos_y][pos_x].ch = '*';
                            *(m2.ch)= '*';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

                        }else if(under == 9){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, '.'| A_BOLD);
                            board[pos_y][pos_x].ch = '.';
                            *(m2.ch)= '.';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

                        }else{
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, (under + '0')| A_BOLD);
                            board[pos_y][pos_x].ch = under;
                            *(m2.ch)= (char)(under + '0');
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

                        }
                        pos_x++;
                        } else{
                            ch_pos = find_ch_info(char_data, n_chars, board[pos_y][pos_x + 1].ch);
                            char_data[ch_pos].score = char_data[ch_pos].score - 10;
                            bumped = true;
                            loser = char_data[ch_pos].lose;
                            if(char_data[ch_pos].score < 0){
                                char_data[ch_pos].lose = true;
                                char_data[ch_pos].win = false;
                            }
                            
                            pos_x = char_data[ch_pos].pos_x;
                            pos_y = char_data[ch_pos].pos_y;
                        }
                    }
                    break;
                case LEFT:
                    if(pos_y - 1 > 0 && board[pos_y - 1][pos_x].ch != 35 && board[pos_y - 1][pos_x].ch >= 32){
                        if(board[pos_y - 1][pos_x].ch <= 46){
                        under = what_under(board[pos_y][pos_x], pos_x, pos_y, cock_data, -2, winner);
                        if(under == 0 || under == 7){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, ' ');
                            board[pos_y][pos_x].ch = ' ';
                            *(m2.ch)= ' ';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

                        }else if(under == 8){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, '*'| A_BOLD);
                            board[pos_y][pos_x].ch = '*';
                            *(m2.ch)= '*';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

                        }else if(under == 9){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, '.'| A_BOLD);
                            board[pos_y][pos_x].ch = '.';
                            *(m2.ch)= '.';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;
                        }else{
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, (under + '0')| A_BOLD);
                            board[pos_y][pos_x].ch = under;
                            *(m2.ch)= (char)(under + '0');
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

                        }
                        pos_y--;
                        } else{
                            ch_pos = find_ch_info(char_data, n_chars, board[pos_y - 1][pos_x].ch);
                            char_data[ch_pos].score = char_data[ch_pos].score - 10;
                            bumped = true;
                            loser = char_data[ch_pos].lose;
                            if(char_data[ch_pos].score < 0){
                                char_data[ch_pos].lose = true;
                                char_data[ch_pos].win = false;
                            }
                            pos_x = char_data[ch_pos].pos_x;
                            pos_y = char_data[ch_pos].pos_y;
                        }
                    }
                    break;
                case RIGHT:
                    if(pos_y + 1 < WINDOW_SIZE-1 && board[pos_y + 1][pos_x].ch != 35 && board[pos_y + 1][pos_x].ch >= 32){
                        if(board[pos_y + 1][pos_x].ch <= 46){
                        under = what_under(board[pos_y][pos_x], pos_x, pos_y, cock_data, -2, winner);
                        if(under == 0 || under == 7){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, ' ');
                            board[pos_y][pos_x].ch = ' ';
                            *(m2.ch)= ' ';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;
                        }else if(under == 8){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, '*'| A_BOLD);
                            board[pos_y][pos_x].ch = '*';
                            *(m2.ch)= '*';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

                        }else if(under == 9){
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, '.'| A_BOLD);
                            board[pos_y][pos_x].ch = '.';
                            *(m2.ch)= '.';
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

                        }else{
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, (under + '0')| A_BOLD);
                            board[pos_y][pos_x].ch = under;
                            *(m2.ch)= (char)(under + '0');
                            m2.posx = pos_x;
                            m2.posy = pos_y;		
                            packed_size = proto_display_message__get_packed_size(&m2);
                            packed_buffer = malloc(packed_size);
                            proto_display_message__pack(&m2, packed_buffer);
                            zmq_send (publisher, packed_buffer, packed_size, 0);
                            free(packed_buffer);
                            packed_buffer=NULL;

                        }
                        pos_y++;
                        } else{
                            ch_pos = find_ch_info(char_data, n_chars, board[pos_y + 1][pos_x].ch);
                            char_data[ch_pos].score = char_data[ch_pos].score - 10;
                            bumped = true;
                            loser = char_data[ch_pos].lose;
                            if(char_data[ch_pos].score < 0){
                                char_data[ch_pos].lose = true;
                                char_data[ch_pos].win = false;
                            }
                            pos_x = char_data[ch_pos].pos_x;
                            pos_y = char_data[ch_pos].pos_y;
                        }
                    }
                    break;
                default:
                    break;
                }
            
                //deletes the lizard tail if it bumped and score changed to negative
                if (bumped == true && char_data[ch_pos].score < 0 && loser ==  false){
                    //delete from board
            switch (char_data[ch_pos].dir)
            {
            case UP:
                for(int i2 = 1; i2 <= 5; i2++){
                    if(pos_x + i2 < WINDOW_SIZE-1){
                        if(board[pos_y][pos_x + i2].ch <= 46){
                            under = what_under(board[pos_y][pos_x + i2], pos_x + i2, pos_y, cock_data, -1, winner);
                            if(under == 0){
                                wmove(my_win, pos_x + i2, pos_y);
                                waddch(my_win, ' ');
                                board[pos_y][pos_x + i2].ch = ' ';
                                *(m2.ch)= ' ';
                                m2.posx = pos_x + i2;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
                            }else if(under == 7){
                                wmove(my_win, pos_x + i2, pos_y);
                                waddch(my_win, '#'| A_BOLD);
                                board[pos_y][pos_x + i2].ch = '#';
                                *(m2.ch)= '#';
                                m2.posx = pos_x + i2;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 8){
                                wmove(my_win, pos_x + i2, pos_y);
                                waddch(my_win, '*'| A_BOLD);
                                board[pos_y][pos_x + i2].ch = '*';
                                *(m2.ch)= '*';
                                m2.posx = pos_x + i2;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 9){
                                wmove(my_win, pos_x + i2, pos_y);
                                waddch(my_win, '.'| A_BOLD);
                                board[pos_y][pos_x + i2].ch = '.';
                                *(m2.ch)= '.';
                                m2.posx = pos_x + i2;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else{
                                wmove(my_win, pos_x + i2, pos_y);
                                waddch(my_win, (under + '0')| A_BOLD);
                                board[pos_y][pos_x + i2].ch = under;
                                *(m2.ch)= (char)(under + '0');
                                m2.posx = pos_x + i2;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }
                        }    
                        board[pos_y][pos_x + i2].nlayers--;
                        if(char_data[ch_pos].win == true)
                            board[pos_y][pos_x + i2].wlayers--;
                    }else{
                        break;
                    }
                }
                break;
            case DOWN:
                for(int i2 = 1; i2 <= 5; i2++){
                    if(pos_x - i2 > 0){
                        if(board[pos_y][pos_x - i2].ch <= 46){
                            under = what_under(board[pos_y][pos_x - i2], pos_x - i2, pos_y, cock_data, -1, winner);
                            if(under == 0){
                                wmove(my_win, pos_x - i2, pos_y);
                                waddch(my_win, ' ');
                                board[pos_y][pos_x - i2].ch = ' ';
                                *(m2.ch)= ' ';
                                m2.posx = pos_x - i2;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 7){
                                wmove(my_win, pos_x - i2, pos_y);
                                waddch(my_win, '#'| A_BOLD);
                                board[pos_y][pos_x - i2].ch = '#';
                                *(m2.ch)= '#';
                                m2.posx = pos_x - i2;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 8){
                                wmove(my_win, pos_x - i2, pos_y);
                                waddch(my_win, '*'| A_BOLD);
                                board[pos_y][pos_x - i2].ch = '*';
                                *(m2.ch)= '*';
                                m2.posx = pos_x - i2;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 9){
                                wmove(my_win, pos_x - i2, pos_y);
                                waddch(my_win, '.'| A_BOLD);
                                board[pos_y][pos_x - i2].ch = '.';
                                *(m2.ch)= '.';
                                m2.posx = pos_x - i2;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
                            }else{
                                wmove(my_win, pos_x - i2, pos_y);
                                waddch(my_win, (under + '0')| A_BOLD);
                                board[pos_y][pos_x - i2].ch = under;
                                *(m2.ch)= (char)(under + '0');
                                m2.posx = pos_x - i2;
                                m2.posy = pos_y;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;
                            }
                        }
                        board[pos_y][pos_x - i2].nlayers--;
                        if(char_data[ch_pos].win == true)
                            board[pos_y][pos_x - i2].wlayers--;
                    }else{
                        break;
                    }
                }
                break;
            case LEFT:
                for(int i2 = 1; i2 <= 5; i2++){
                    if(pos_y + i2 < WINDOW_SIZE-1){
                        if(board[pos_y + i2][pos_x].ch <= 46){  
                            under = what_under(board[pos_y + i2][pos_x], pos_x, pos_y + i2, cock_data, -1, winner);
                            if(under == 0){
                                wmove(my_win, pos_x, pos_y + i2);
                                waddch(my_win, ' ');
                                board[pos_y + i2][pos_x].ch = ' ';
                                *(m2.ch)= ' ';
                                m2.posx = pos_x;
                                m2.posy = pos_y + i2;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 7){
                                wmove(my_win, pos_x, pos_y + i2);
                                waddch(my_win, '#'| A_BOLD);
                                board[pos_y + i2][pos_x].ch = '#';
                                *(m2.ch)= '#';
                                m2.posx = pos_x;
                                m2.posy = pos_y + i2;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 8){
                                wmove(my_win, pos_x, pos_y + i2);
                                waddch(my_win, '*'| A_BOLD);
                                board[pos_y + i2][pos_x].ch = '*';
                                *(m2.ch)= '*';
                                m2.posx = pos_x;
                                m2.posy = pos_y + i2;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 9){
                                wmove(my_win, pos_x, pos_y + i2);
                                waddch(my_win, '.'| A_BOLD);
                                board[pos_y + i2][pos_x].ch = '.';
                                *(m2.ch)= '.';
                                m2.posx = pos_x;
                                m2.posy = pos_y + i2;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else{
                                wmove(my_win, pos_x, pos_y + i2);
                                waddch(my_win, (under + '0')| A_BOLD);
                                board[pos_y + i2][pos_x].ch = under;
                                *(m2.ch)= (char)(under + '0');
                                m2.posx = pos_x;
                                m2.posy = pos_y + i2;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }
                        }
                        board[pos_y + i2][pos_x].nlayers--;
                        if(char_data[ch_pos].win == true)
                            board[pos_y + i2][pos_x].wlayers--;
                    }else{
                        break;
                    }
                }
                break;
            case RIGHT:
                for(int i2 = 1; i2 <= 5; i2++){
                    if(pos_y - i2 > 0){
                        if(board[pos_y - i2][pos_x].ch <= 46){
                            under = what_under(board[pos_y - i2][pos_x], pos_x, pos_y - i2, cock_data, -1, winner);
                            if(under == 0){
                                wmove(my_win, pos_x, pos_y - i2);
                                waddch(my_win, ' ');
                                board[pos_y - i2][pos_x].ch = ' ';
                                *(m2.ch)= ' ';
                                m2.posx = pos_x;
                                m2.posy = pos_y - i2;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 7){
                                wmove(my_win, pos_x, pos_y - i2);
                                waddch(my_win, '#'| A_BOLD);
                                board[pos_y - i2][pos_x].ch = '#';
                                *(m2.ch)= '#';
                                m2.posx = pos_x;
                                m2.posy = pos_y - i2;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 8){
                                wmove(my_win, pos_x, pos_y - i2);
                                waddch(my_win, '*'| A_BOLD);
                                board[pos_y - i2][pos_x].ch = '*';
                                *(m2.ch)= '*';
                                m2.posx = pos_x;
                                m2.posy = pos_y - i2;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else if(under == 9){
                                wmove(my_win, pos_x, pos_y - i2);
                                waddch(my_win, '.'| A_BOLD);
                                board[pos_y - i2][pos_x].ch = '.';
                                *(m2.ch)= '.';
                                m2.posx = pos_x;
                                m2.posy = pos_y - i2;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }else{
                                wmove(my_win, pos_x, pos_y - i2);
                                waddch(my_win, (under + '0')| A_BOLD);
                                board[pos_y - i2][pos_x].ch = under;
                                *(m2.ch)= (char)(under + '0');
                                m2.posx = pos_x;
                                m2.posy = pos_y - i2;	
                                packed_size = proto_display_message__get_packed_size(&m2);
                                packed_buffer = malloc(packed_size);
                                proto_display_message__pack(&m2, packed_buffer);
                                zmq_send (publisher, packed_buffer, packed_size, 0);
                                free(packed_buffer);
                                packed_buffer=NULL;

                            }
                        }
                        board[pos_y - i2][pos_x].nlayers--;    
                        if(char_data[ch_pos].win == true)
                            board[pos_y - i2][pos_x].wlayers--;
                    }else{
                        break;
                    }
                }
                break;
            default:
                break;
            }
            
                    pos_x = save_x;
                    pos_y = save_y;
                }else if(bumped == true){
                    pos_x = save_x;
                    pos_y = save_y;
                }
                
                //saves data in new position
                board[pos_y][pos_x].ch = ch;
                cock_data[fcock +i].posx = pos_x;
                cock_data[fcock +i].posy = pos_y;
                scoreboard(char_data, n_chars);
                //send scoreboard
                m2.posx = 999; //flag for scoreboard
                m2.posy = n_chars; //send number of characters
                packed_size = proto_display_message__get_packed_size(&m2);
                packed_buffer = malloc(packed_size);
                proto_display_message__pack(&m2, packed_buffer);
                zmq_send (publisher, packed_buffer, packed_size, 0);
                free(packed_buffer);
                packed_buffer=NULL;

                for(int j = 0; j < n_chars; j++){
                    m2.posx = char_data[j].ch;
                    //*(m2.ch)= (char)(char_data[i].ch + '0');
                    m2.score = char_data[j].score;
                    packed_size = proto_display_message__get_packed_size(&m2);
                    packed_buffer = malloc(packed_size);
                    proto_display_message__pack(&m2, packed_buffer);
                    zmq_send (publisher, packed_buffer, packed_size, 0);
                    free(packed_buffer);
                    packed_buffer=NULL;
                }
                refresh();
                //print new position
                box(my_win, 0 , 0);
                wmove(my_win, pos_x, pos_y);
                waddch(my_win, ch | A_BOLD);
                *(m2.ch)= ch;
                m2.posx = pos_x;
                m2.posy = pos_y;		
                packed_size = proto_display_message__get_packed_size(&m2);
                packed_buffer = malloc(packed_size);
                proto_display_message__pack(&m2, packed_buffer);
                zmq_send (publisher, packed_buffer, packed_size, 0);
                free(packed_buffer);
                packed_buffer=NULL;

            }
            wrefresh(my_win);
            packed_size = proto_char_message__get_packed_size(recvm);
            packed_buffer = malloc(packed_size);
            proto_char_message__pack(recvm, packed_buffer);
            zmq_send (responder, packed_buffer, packed_size, 0);
            free(packed_buffer);
            packed_buffer=NULL;
            proto_char_message__free_unpacked(recvm, NULL);
            recvm=NULL;
        }
        //cockroach disconnect
        else if(recvm->msg_type == 8){
            packed_size = proto_char_message__get_packed_size(recvm);
            packed_buffer = malloc(packed_size);
            proto_char_message__pack(recvm, packed_buffer);
            zmq_send (responder, packed_buffer, packed_size, 0);
            free(packed_buffer);
            packed_buffer=NULL;

            bool winner = false;
            ncock = recvm->ncock;
            fcock = (int) *(recvm->ch);

            for(i = 0; i < ncock; i++){

                //load current position and value of each roach
                pos_x = cock_data[fcock + i].posx;
                pos_y = cock_data[fcock + i].posy;
                ch = cock_data[fcock + i].value;

                //find what's under the roach
                under = what_under(board[pos_y][pos_x], pos_x, pos_y, cock_data, fcock + i, winner);
                if(under == 0){
                    wmove(my_win, pos_x, pos_y);
                    waddch(my_win, ' ');
                    board[pos_y][pos_x].ch = ' ';
                    *(m2.ch)= ' ';
                    m2.posx = pos_x;
                    m2.posy = pos_y;		
                    packed_size = proto_display_message__get_packed_size(&m2);
                    packed_buffer = malloc(packed_size);
                    proto_display_message__pack(&m2, packed_buffer);
                    zmq_send (publisher, packed_buffer, packed_size, 0);
                    free(packed_buffer);
                    packed_buffer=NULL;
                }else if(under == 8){
                    wmove(my_win, pos_x, pos_y);
                    waddch(my_win, '*'| A_BOLD);
                    board[pos_y][pos_x].ch = '*';
                    *(m2.ch)= '*';
                    m2.posx = pos_x;
                    m2.posy = pos_y;		
                    packed_size = proto_display_message__get_packed_size(&m2);
                    packed_buffer = malloc(packed_size);
                    proto_display_message__pack(&m2, packed_buffer);
                    zmq_send (publisher, packed_buffer, packed_size, 0);
                    free(packed_buffer);
                    packed_buffer=NULL;
                }else if(under == 9){
                    wmove(my_win, pos_x, pos_y);
                    waddch(my_win, '.'| A_BOLD);
                    board[pos_y][pos_x].ch = '.';
                    *(m2.ch)= '.';
                    m2.posx = pos_x;
                    m2.posy = pos_y;		
                    packed_size = proto_display_message__get_packed_size(&m2);
                    packed_buffer = malloc(packed_size);
                    proto_display_message__pack(&m2, packed_buffer);
                    zmq_send (publisher, packed_buffer, packed_size, 0);
                    free(packed_buffer);
                    packed_buffer=NULL;

                }else{
                    wmove(my_win, pos_x, pos_y);
                    waddch(my_win, (under + '0')| A_BOLD);
                    board[pos_y][pos_x].ch = under;
                    *(m2.ch)= (char)(under + '0');
                    m2.posx = pos_x;
                    m2.posy = pos_y;		
                    packed_size = proto_display_message__get_packed_size(&m2);
                    packed_buffer = malloc(packed_size);
                    proto_display_message__pack(&m2, packed_buffer);
                    zmq_send (publisher, packed_buffer, packed_size, 0);
                    free(packed_buffer);
                    packed_buffer=NULL;
                }

                cock_data[fcock + i].posx = -1;
                cock_data[fcock + i].posy = -1;
                cock_data[fcock + i].value = 0;
            }
            wrefresh(my_win);

        }       
        //wasp disconnect
        else if(recvm->msg_type == 9){
            packed_size = proto_char_message__get_packed_size(recvm);
            packed_buffer = malloc(packed_size);
            proto_char_message__pack(recvm, packed_buffer);
            zmq_send (responder, packed_buffer, packed_size, 0);
            free(packed_buffer);
            packed_buffer=NULL;

            bool winner = false;
            ncock = recvm->ncock;
            fcock = (int) *(recvm->ch);

            for(i = 0; i < ncock; i++){

                //load current position and value of each roach
                pos_x = cock_data[fcock + i].posx;
                pos_y = cock_data[fcock + i].posy;
                ch = cock_data[fcock + i].value;

                //find what's under the roach
                under = what_under(board[pos_y][pos_x], pos_x, pos_y, cock_data, -2, winner);
                if(under == 0 || under == 7){
                    wmove(my_win, pos_x, pos_y);
                    waddch(my_win, ' ');
                    board[pos_y][pos_x].ch = ' ';
                    *(m2.ch)= ' ';
                    m2.posx = pos_x;
                    m2.posy = pos_y;		
                    packed_size = proto_display_message__get_packed_size(&m2);
                    packed_buffer = malloc(packed_size);
                    proto_display_message__pack(&m2, packed_buffer);
                    zmq_send (publisher, packed_buffer, packed_size, 0);
                    free(packed_buffer);
                    packed_buffer=NULL;
                }else if(under == 8){
                    wmove(my_win, pos_x, pos_y);
                    waddch(my_win, '*'| A_BOLD);
                    board[pos_y][pos_x].ch = '*';
                    *(m2.ch)= '*';
                    m2.posx = pos_x;
                    m2.posy = pos_y;		
                    packed_size = proto_display_message__get_packed_size(&m2);
                    packed_buffer = malloc(packed_size);
                    proto_display_message__pack(&m2, packed_buffer);
                    zmq_send (publisher, packed_buffer, packed_size, 0);
                    free(packed_buffer);
                    packed_buffer=NULL;

                }else if(under == 9){
                    wmove(my_win, pos_x, pos_y);
                    waddch(my_win, '.'| A_BOLD);
                    board[pos_y][pos_x].ch = '.';
                    *(m2.ch)= '.';
                    m2.posx = pos_x;
                    m2.posy = pos_y;		
                    packed_size = proto_display_message__get_packed_size(&m2);
                    packed_buffer = malloc(packed_size);
                    proto_display_message__pack(&m2, packed_buffer);
                    zmq_send (publisher, packed_buffer, packed_size, 0);
                    free(packed_buffer);
                    packed_buffer=NULL;

                }else{
                    wmove(my_win, pos_x, pos_y);
                    waddch(my_win, (under + '0')| A_BOLD);
                    board[pos_y][pos_x].ch = under;
                    *(m2.ch)= (char)(under + '0');
                    m2.posx = pos_x;
                    m2.posy = pos_y;		
                    packed_size = proto_display_message__get_packed_size(&m2);
                    packed_buffer = malloc(packed_size);
                    proto_display_message__pack(&m2, packed_buffer);
                    zmq_send (publisher, packed_buffer, packed_size, 0);
                    free(packed_buffer);
                    packed_buffer=NULL;

                }

                cock_data[fcock + i].posx = -1;
                cock_data[fcock + i].posy = -1;
                cock_data[fcock + i].value = 0;
            }
            wrefresh(my_win);

        } 
        }
        pthread_mutex_unlock(&mutex_test1);
        zmq_msg_close(&zmq_msg);
    }
    return 0;
}

void* proxy_create_function(void *context){

    void *router1 = zmq_socket (context, ZMQ_ROUTER);
    int rc = zmq_bind (router1, candidate1);
    assert (rc == 0);

    void *dealer1 = zmq_socket(context, ZMQ_DEALER);
    rc = zmq_bind(dealer1, "inproc://mydealer1");
    assert(rc ==  0);

    zmq_proxy(router1, dealer1, NULL);

    zmq_close(router1);
    zmq_close(dealer1);
    
    return 0;
}


int main(int argc, char *argv[])
{	
    srand((unsigned int) time(NULL));
    //matrix with everything drawn
    for (int a = 0; a < WINDOW_SIZE-1; a++) {
        for (int b = 0; b < WINDOW_SIZE-1; b++) {
            board[a][b].ch = ' ';    
            board[a][b].nlayers = 0; 
            board[a][b].wlayers = 0; 
        }
    }
    for (int c=0; c<MAX_PLAYERS; c++)
        memset(char_data[c].password,'\0', sizeof(char_data[c].password));
    
    for (int c=0; c<MAX_PLAYERS; c++)
        memset(cock_data[c].password,'\0', sizeof(cock_data[c].password));
    if (argc != 5) {
        printf("Wrong number number of arguments\n");
        return 1;
    }

    char *ip = argv[1];
    char *port1 = argv[2];
    char *port2 = argv[3];
    char *port3 = argv[4];

    if(!Is_ValidIPv4(ip) || !Is_ValidPort(port1) || !Is_ValidPort(port2) || !Is_ValidPort(port3)){
        printf("ERROR[000]: Incorrect Host-Server data. Check host IPv4 and TCP ports.");
        exit(1);
    }

    candidate1 = (char*) calloc (1, sizeof(char)*(strlen("tcp://")+strlen(ip) + 1 + strlen(port1) + 1));
    char *candidate2 = (char*) calloc (1, sizeof(char)*(strlen("tcp://")+strlen(ip) + 1 + strlen(port2) + 1));
    char *candidate3 = (char*) calloc (1, sizeof(char)*(strlen("tcp://")+strlen(ip) + 1 + strlen(port3) + 1));
    strcat(candidate1, "tcp://");
    strcat(candidate1, ip);
    strcat(candidate1, ":");
    strcat(candidate1, port1);
    strcat(candidate2, "tcp://");
    strcat(candidate2, ip);
    strcat(candidate2, ":");
    strcat(candidate2, port2);
    strcat(candidate3, "tcp://");
    strcat(candidate3, ip);
    strcat(candidate3, ":");
    strcat(candidate3, port3);

    //socket creation
    void *context = zmq_ctx_new ();
    //void *router1 = zmq_socket (context, ZMQ_ROUTER);
    //int rc = zmq_bind (router1, candidate1);
    //assert (rc == 0);

    void *router2 = zmq_socket (context, ZMQ_ROUTER);
    int rc = zmq_bind (router2, candidate2);
    assert (rc == 0);

    //void *dealer1 = zmq_socket(context, ZMQ_DEALER);
    //rc = zmq_bind(dealer1, "inproc://mydealer1");
    //assert(rc ==  0);

    void *dealer2 = zmq_socket(context, ZMQ_DEALER);
    rc = zmq_bind(dealer2, "inproc://mydealer2");
    assert(rc ==  0);

    void *context2 = zmq_ctx_new ();
    publisher = zmq_socket (context2, ZMQ_PUB);
    rc = zmq_bind (publisher, candidate3);
    assert(rc == 0);

	initscr();		    	
	cbreak();				
    keypad(stdscr, TRUE);   
	noecho();			    

    /* creates a window and draws a border */
    
    my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(my_win, 0 , 0);	
	wrefresh(my_win);
    
    //init and create threads
    pthread_t lizard_thread[LIZARD_THREADS];
    pthread_t roach_thread[ROACH_THREADS];

    //thread create
    for(int a = 0; a < LIZARD_THREADS; a++){

        pthread_create(&lizard_thread[a], NULL, lizard_function, context);
    }

    for(int a = 0; a < ROACH_THREADS; a++){
        pthread_create(&roach_thread[a], NULL, roach_function, context);
    }


    pthread_t proxy_thread;

    pthread_create(&proxy_thread, NULL, proxy_create_function, context);

    zmq_proxy(router2, dealer2, NULL);

    zmq_close(publisher);
    //zmq_close(router1);
    //zmq_close(dealer1);
    zmq_close(dealer2);
    zmq_ctx_shutdown(context);
    zmq_ctx_shutdown(context2);
    zmq_ctx_term(context);
    zmq_ctx_term(context2);
    endwin();			/* End curses mode		  */
    free(candidate1);
    free(candidate2);
	return 0;
}
