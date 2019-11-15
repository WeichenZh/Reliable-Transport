// Server side C/C++ program to demonstrate Socket programming 
#include <unistd.h>
#include <string.h>
#include <stdio.h> 
#include <stdlib.h>
#include <iostream>

#include "session_send.h"
 
using namespace std;

static int err(){
    printf("Error: missing or extra arguments\n");
    exit(EXIT_FAILURE);
}
static bool same(char const * c, char const * tar){
    return strcmp(c, tar)==0;
}

//<receiver-IP> <receiver-port> <window-size> <input-file> <log>
int main(int argc, char const *argv[]){
	if(argc != 6) err();
	int port, win_size;
	char const *host, *input_path, *log_path;
	host = argv[1];
	port = atoi(argv[2]);
	win_size = atoi(argv[3]);
	input_path = argv[4];
	log_path = argv[5];
	WSender sender(host, port, win_size, log_path, input_path);
	sender.send();
	return 0;
}
