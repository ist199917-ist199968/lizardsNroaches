#include <stdint.h>
#include <zmq.h>
#include <ncurses.h>
#include "message.pb-c.h"
#include "remote-char.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  
#include <ctype.h> 
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

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

static char password[50];
static char ch;
void *requester=NULL;
void *context=NULL;
char *candidate1=NULL;
void *context2 = NULL;
void *subscriber = NULL;
board_t board[WINDOW_SIZE-1][WINDOW_SIZE-1];
int trabalha = 1;

void sigintHandler(int signum) {
    
    trabalha = 0;
    ProtoCharMessage m;
    proto_char_message__init(&m); 
    m.msg_type = 0;
    m.ch=malloc(sizeof(char));
    m.password=malloc(50*sizeof(char));
    m.ch = &ch;
    m.direction = 0;
    strcpy(m.password, password);
    

    m.msg_type = 5;
    size_t packed_size = proto_char_message__get_packed_size(&m);
    uint8_t *packed_buffer = malloc(packed_size);
    proto_char_message__pack(&m, packed_buffer);
    zmq_send (requester, packed_buffer, packed_size, 0);
    free(packed_buffer);
    packed_buffer=NULL;
    zmq_msg_t zmq_msg;
    zmq_msg_init (&zmq_msg);
    packed_size=zmq_recvmsg(requester, &zmq_msg,0);
    packed_buffer = zmq_msg_data(&zmq_msg);
    ProtoCharMessage *recvm=proto_char_message__unpack(NULL, packed_size, packed_buffer);
    proto_char_message__free_unpacked(recvm, NULL);
    recvm=NULL;
    endwin();
    free(candidate1);
    zmq_close (requester);
    zmq_ctx_destroy (context);
    endwin();			/* End curses mode*/
    exit(signum);  
}

void *display (){
    ProtoCharMessage m;
    
    size_t packed_size;
    uint8_t* packed_buffer;
    ProtoDisplayMessage *recvm = NULL;
    zmq_msg_t zmq_msg;
    zmq_msg_init(&zmq_msg);

    proto_char_message__init(&m); 
    m.msg_type = 0;
    m.ch=malloc(sizeof(char));
    m.password=malloc(50*sizeof(char));
    m.ch = &ch;
    m.direction = 0;
    strcpy(m.password, password);

    /* creates a window and draws a border */
    WINDOW * my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(my_win, 0 , 0);	
	wrefresh(my_win);

    //message from the server with a character's updated position
   // ProtoDisplayMessage m2;
    int n_chars;
    //joining server
    
    for (int a = 1; a < WINDOW_SIZE-1; a++) {
        for (int b = 1; b < WINDOW_SIZE-1; b++) {
            wmove(my_win, a, b);
            if(board[b][a].ch <= 5){
                waddch(my_win, (char)(board[b][a].ch + '0')| A_BOLD);}
            else{waddch(my_win, (char)(board[b][a].ch)| A_BOLD);}
        }
    }
    wrefresh(my_win);
    while (trabalha)
    {
        //zmq_recv (subscriber, &m2, sizeof(m2), 0);

        packed_size=zmq_recvmsg(subscriber, &zmq_msg,0); 
        packed_buffer = zmq_msg_data(&zmq_msg);
        if(trabalha)
        recvm=proto_display_message__unpack(NULL, packed_size, packed_buffer);
        
        if(recvm->posx == 999){//flag for scoreboard print
            n_chars = recvm->posy;
            mvprintw(0, WINDOW_SIZE + 1,"Scoreboard");
            //delete previous scoreboard
            for(int i = 0; i < MAX_PLAYERS; i++){
                mvprintw(i + 2, WINDOW_SIZE + 1,"               ");
            }

            //print new scoreboard
            
            for(int i = 0; i < n_chars; i++){
                //zmq_recv (subscriber, &m2, sizeof(m2), 0);
                packed_size=zmq_recvmsg(subscriber, &zmq_msg,0); 
                packed_buffer = zmq_msg_data(&zmq_msg);
                recvm=proto_display_message__unpack(NULL, packed_size, packed_buffer);

                mvprintw(i + 2, WINDOW_SIZE + 1,"%c score: %d", recvm->posx , recvm->score);
            }
            refresh();
            box(my_win, 0 , 0);
        }
        else{
            wmove(my_win, recvm->posx, recvm->posy);
            waddch(my_win,*(recvm->ch)| A_BOLD);
            wrefresh(my_win);
        }   
    }
    return 0;   
}


int main(int argc, char *argv[])
{
    signal(SIGINT, sigintHandler); /*log handler*/
    pthread_t threadDisplay;
    srand((unsigned int) time(NULL));
    if (argc != 4) {
        printf("Wrong number number of arguments\n");
        return 1;
    }

    char *ip = argv[1];
    char *port1 = argv[2];
    char *port2 =argv[3];
    if(!Is_ValidIPv4(ip) || !Is_ValidPort(port1)){
        printf("ERROR[000]: Incorrect Host-Server data. Check host IPv4 and TCP ports.");
        exit(1);
    }

    candidate1 = (char*) calloc (1, sizeof(char)*(strlen("tcp://")+strlen(ip) + 1 + strlen(port1) + 1));
    char *candidate2 = (char*) calloc (1, sizeof(char)*(strlen("tcp://")+strlen(ip) + 1 + strlen(port2) + 1));
    strcat(candidate1, "tcp://");
    strcat(candidate1, ip);
    strcat(candidate1, ":");
    strcat(candidate1, port1);
    strcat(candidate2, "tcp://");
    strcat(candidate2, ip);
    strcat(candidate2, ":");
    strcat(candidate2, port2);

    context = zmq_ctx_new ();
    requester = zmq_socket (context, ZMQ_REQ);
    context2 = zmq_ctx_new ();
    subscriber = zmq_socket (context, ZMQ_SUB);
    
    zmq_connect(requester, candidate1);
     int rc = zmq_connect (subscriber, candidate2);
    assert (rc == 0);
    rc = zmq_setsockopt (subscriber, ZMQ_SUBSCRIBE, "", 0);
    assert (rc == 0);

    // read the character from the user
    do{
        printf("what is your character(a..z)?: ");
        ch = getchar();
        ch = tolower(ch);  
    }while(!isalpha(ch));

    for(int i = 0; i < 49; i++){
        password[i] = (char)(random()%93 + 33);
    }
    password[49]='\0';
    // send connection message
    //remote_char_t m;
    ProtoCharMessage m;
    proto_char_message__init(&m); 
    m.msg_type = 0;
    m.ch=malloc(sizeof(char) * 2);
    m.password=malloc(50*sizeof(char));
    m.ch[0] = ch;
    m.ch[1] = '\0';
    m.direction = UP;
    m.n_cockdir = 0;
    m.cockdir = NULL;
    strcpy(m.password, password);
    m.password[49]='\0';
    
    size_t packed_size = proto_char_message__get_packed_size(&m);
    
    uint8_t* packed_buffer = malloc(packed_size);
    proto_char_message__pack(&m, packed_buffer);
    zmq_send (requester, packed_buffer, packed_size, 0);
    free(packed_buffer);
    packed_buffer=NULL;

    //receive the assigned letter from the server or flag that is full
    zmq_msg_t zmq_msg;
    zmq_msg_init (&zmq_msg);
    
    packed_size=zmq_recvmsg(requester, &zmq_msg,0);
        
    packed_buffer = zmq_msg_data(&zmq_msg);
    ProtoCharMessage *recvm=proto_char_message__unpack(NULL, packed_size, packed_buffer);
    strcpy(&ch,recvm->ch);
    if(ch == 0){
        printf("Tamos cheios :/\n");
        return 0;
    }

    for (int a = 0; a < WINDOW_SIZE-1; a++) {
        for (int b = 0; b < WINDOW_SIZE-1; b++) {
            board[a][b].ch = ' ';    
            board[a][b].nlayers = 0; 
        }
    }
    
    m.msg_type = 2;
    packed_size = proto_char_message__get_packed_size(&m);
    packed_buffer = malloc(packed_size);
    proto_char_message__pack(&m, packed_buffer);
    zmq_send (requester, packed_buffer, packed_size, 0);
    free(packed_buffer);
    packed_buffer=NULL;
    
    //receive the current board from the server
    zmq_recv (requester, &board, sizeof(board), 0);
    
    /*packed_size=zmq_recvmsg(requester, &zmq_msg,0);
    packed_buffer = zmq_msg_data(&zmq_msg);
    recvm=proto_char_message__unpack(NULL, packed_size, packed_buffer);*/

	initscr();			/* Start curses mode 		*/
	cbreak();				/* Line buffering disabled	*/
	keypad(stdscr, TRUE);		/* We get F1, F2 etc..		*/
	noecho();			/* Don't echo() while we do getch */
    proto_char_message__free_unpacked(recvm, NULL);
    recvm=NULL;
    //mvprintw(0,0,"Player %c", ch);
    // prepare the movement message
    
    if (pthread_create(&threadDisplay, NULL, display, NULL) != 0) {
        fprintf(stderr, "Error creating thread A\n");
        return 1;
    }

    m.msg_type = 1;
    m.ch = &ch;
    int key, n = 0;
    while(1){
    	key = getch();	
        n++;

        switch (key)
        {
        case KEY_LEFT:
            //mvprintw(3,0,"%d Left arrow is pressed", n);
            // prepare the movement message
           m.direction = PROTO_DIRECTION__LEFT;
            break;
        case KEY_RIGHT:
            //mvprintw(3,0,"%d Right arrow is pressed", n);
            // prepare the movement message
            m.direction = PROTO_DIRECTION__RIGHT;
            break;
        case KEY_DOWN:
            //mvprintw(3,0,"%d Down arrow is pressed", n);
            // prepare the movement message
           m.direction = PROTO_DIRECTION__DOWN;
            break;
        case KEY_UP:
            //mvprintw(3,0,"%d :Up arrow is pressed", n);
            // prepare the movement message
            m.direction = PROTO_DIRECTION__UP;
            break;
        
        case 'q':
        case 'Q':
            trabalha = 0;
            m.msg_type = 5;
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
            endwin();
            free(candidate1);
            zmq_close (requester);
            zmq_ctx_destroy (context);
            exit(0);
            break;
        
        default:
            key = 'x'; 
            break;
        }

        //send the movement message
        if (key != 'x'){
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
            //mvprintw(1,0,"Score - %d", score);
        }
    }
    free(candidate1);
    zmq_close (requester);
    zmq_ctx_destroy (context);
    endwin();			/* End curses mode		  */
	return 0;
}
