
typedef enum direction_t {UP, DOWN, LEFT, RIGHT} direction_t;

typedef struct remote_char_t
{   
    int msg_type;
    char ch; //also used as first position of cockroach in the server array
    int ncock;
    direction_t direction;
    direction_t cockdir[10];
}remote_char_t;

typedef struct remote_display_t
{   
    int msg_type;
    int posx; 
    int posy; 
    char ch;
    direction_t direction;
}remote_display_t;


