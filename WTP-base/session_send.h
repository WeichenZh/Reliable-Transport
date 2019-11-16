#ifndef SESSION_SEND_H
#define SESSION_SEND_H

//parameters
#define BUFFERSIZESMALL 2000
#define DATALEN 1472
#define IPSIZE 200

//models
#include <chrono>
#include <vector>
#include <iostream>
#include <fstream>
#include <sys/socket.h> 
#include "../starter_files/PacketHeader.h"
#include <string.h> 
using namespace std;



class WSender
{
    //inputs
    int port, win_size;
    char host[IPSIZE], log_path[IPSIZE], input_path[IPSIZE];
    
    //connect
    int sockfd; 
    int seq, seq_curr, seq_st;

    //input ctrl
    int offset=0, last_seq=0;
    //std::ifstream fin;
    bool flag_fin=false;
    string input_data, input_data_buff;

    //read_file
    char send_buff[BUFFERSIZESMALL], recv_buff[DATALEN], data_buffer[DATALEN];
    int len_send, len_recv, len_input;

    //other

    //functions
    int _get_seq(int sq) { return sq - seq_st;}
    int _get_seq() { return seq - seq_st;}
    int _get_curr_seq() { return seq_curr - seq_st;}
    void write_to_logfile(PacketHeader *wdphdr);
    void set_package(char *d, int type, int len);
    void decode_package();
    void read_to_data();
    void my_send(const sockaddr *si_other, socklen_t slen);
    void my_recv(sockaddr *si_other, socklen_t *slen);
public:
    WSender(char const *ho, int pt, int ws, char const *lp);
    void send(char const *path);
};



#endif
