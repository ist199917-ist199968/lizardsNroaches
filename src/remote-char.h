#include <time.h>
typedef enum direction_t {UP, DOWN, LEFT, RIGHT} direction_t;

typedef struct remote_char_t
{   
    int msg_type;
    char ch; //also used as first position of cockroach in the server array
    int ncock; //number of cockroaches controlled by the client, or send the score back to the user
    direction_t direction;
    direction_t cockdir[10];
}remote_char_t;

typedef struct remote_display_t
{   
    int posx; 
    int posy; 
    char ch;
    int score;

}remote_display_t;


