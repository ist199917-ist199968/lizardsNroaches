#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include "message.pb-c.h"
#include "remote-char.h"

bool Is_ValidIPv4(const char *candidate){
    in_addr_t ip;
    if (inet_pton(AF_INET, candidate, &ip) == 1) /*Converting text to binary*/
        return true;
    return false;
}

bool Is_ValidPort(const char *candidate){
    int port = atoi(candidate);
    int len = strlen(candidate);
    for (int i = 0; i < len; i++){
        if (candidate[i] < '0' || candidate[i] > '9')
            return false;
    }

    if (port >= 1023 && port <= 65535) /*Port 0 is reserved for stdin, max TCP/UDP port is 65535*/
        return true;
    return false;
}
