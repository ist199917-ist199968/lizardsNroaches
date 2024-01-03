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
    password[49]='\0';
    // send connection message
    //remote_char_t m;
    ProtoCharMessage m;
    proto_char_message__init(&m); 
    m.msg_type = 0;
    m.ch=malloc(sizeof(char));
    m.password=malloc(50*sizeof(char));
    m.ch = &ch;
    m.direction = 0;
    strcpy(m.password, password);
    
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
	initscr();			/* Start curses mode 		*/
	cbreak();				/* Line buffering disabled	*/
	keypad(stdscr, TRUE);		/* We get F1, F2 etc..		*/
	noecho();			/* Don't echo() while we do getch */
    proto_char_message__free_unpacked(recvm, NULL);
    recvm=NULL;
    int n = 0;
    mvprintw(0,0,"Player %c", ch);
    // prepare the movement message
    m.msg_type = 1;
    m.ch = &ch;
    int key, score = 0;
    while(1){
    	key = getch();	
        n++;

        switch (key)
        {
        case KEY_LEFT:
            mvprintw(3,0,"%d Left arrow is pressed", n);
            // prepare the movement message
           m.direction = PROTO_DIRECTION__LEFT;
            break;
        case KEY_RIGHT:
            mvprintw(3,0,"%d Right arrow is pressed", n);
            // prepare the movement message
            m.direction = PROTO_DIRECTION__RIGHT;
            break;
        case KEY_DOWN:
            mvprintw(3,0,"%d Down arrow is pressed", n);
            // prepare the movement message
           m.direction = PROTO_DIRECTION__DOWN;
            break;
        case KEY_UP:
            mvprintw(3,0,"%d :Up arrow is pressed", n);
            // prepare the movement message
            m.direction = PROTO_DIRECTION__UP;
            break;
        
        case 'q':
        case 'Q':
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
            packed_size = proto_char_message__get_packed_size(&m);
            packed_buffer = malloc(packed_size);
            proto_char_message__pack(&m, packed_buffer);
            zmq_send (requester, packed_buffer, packed_size, 0);
            free(packed_buffer);
            packed_buffer=NULL;
            packed_size=zmq_recvmsg(requester, &zmq_msg,0);
            packed_buffer = zmq_msg_data(&zmq_msg);
            recvm=proto_char_message__unpack(NULL, packed_size, packed_buffer);
            score = recvm->ncock;
            proto_char_message__free_unpacked(recvm, NULL);
            recvm=NULL;
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
