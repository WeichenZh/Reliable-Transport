#ifndef SESSION_SEND_H
#define SESSION_SEND_H

//parameters
#define BUFFERSIZESMALL 2000
#define DATALEN 1472
#define PUREDATALEN 1456
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
    const int port, win_size;
    char host[IPSIZE], log_path[IPSIZE], input_path[IPSIZE];
    
    //connect
    int sockfd; 
    int seq, seq_curr, seq_st, tot_seq;

    //input ctrl
    std::ifstream fin;
    std::ofstream fout;
    int len_input, data_chunk_size, len_chunk=0, offset=0, last_seq=0;
    bool flag_all_load=false, flag_all_ack=false;
    string input_data, input_data_buff;

    //send_recv_file
    char send_buff[BUFFERSIZESMALL], recv_buff[DATALEN], data_buffer[PUREDATALEN];
    int len_send, len_recv;
    vector <bool> acks;

    //other

    //functions
    int _get_seq(int sq) { return sq - seq_st;}
    int _get_seq() { return seq - seq_st;}
    int _get_curr_seq() { return seq_curr - seq_st;}
    void write_to_logfile(PacketHeader *wdphdr);
    void set_package(char *d, int type, int len);
    void read_to_data(char *input_data_ptr);
    int load_data(int idx);
    void shift_load_chunk(const int step);
    void my_send(const sockaddr *si_other, socklen_t slen);
    void my_recv(sockaddr *si_other, socklen_t *slen);
public:
    WSender(char const *ho, int pt, int ws, char const *lp, char const *dp);
    ~WSender();
    void send();
};



#endif
