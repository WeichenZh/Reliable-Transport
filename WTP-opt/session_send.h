#ifndef SESSION_SEND_H
#define SESSION_SEND_H
//parameters
#define BUFFERSIZE 1048576
#define BUFFERSIZESMALL 2000
#define DATALEN 1472
#define IPSIZE 30
#include <chrono>
#include <vector>
using namespace std;



class WSender
{
    //inputs
    int port, win_size;
    char host[IPSIZE], log_path[BUFFERSIZESMALL];
    
    //connect
    int sockfd; 
    int seq, seq_curr, seq_st;

    //read_file
    char input_data[BUFFERSIZE], packet[DATALEN], data_buffer[DATALEN];

    //other

    //functions
    int _get_seq(int sq) { return sq - seq_st;}
    int _get_seq() { return seq - seq_st;}
    int _get_curr_seq() { return seq_curr - seq_st;}
    void write_to_logfile();
    int set_package(char *d, int type, int len);
    void decode_package();
    int read_to_data(char const *path);
public:
    WSender(char const *ho, int pt, int ws, char const *lp);
    void send(char const *path);
};



#endif
