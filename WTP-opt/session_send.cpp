#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <time.h> 
#include <iostream>
#include <fstream>
#include <string.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include "session_send.h"
#include <ctime>
#include <vector>

#include "../starter_files/crc32.h"


using namespace std;

static int err(char const *error){
    printf("Error: %s\n", error);
    exit(EXIT_FAILURE);
}

static bool same(char const * c, char const * tar){
    return strcmp(c, tar)==0;
}
class Timer
{
public:
    Timer() {restart();}
    void restart() {m_StartTime = chrono::system_clock::now();}
    double duration_ns() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(chrono::system_clock::now() - m_StartTime).count();
    }
    double duration_ms() {return duration_ns()/1000000;}
private:
    chrono::time_point<chrono::system_clock> m_StartTime;
};


WSender::WSender(char const *ho, int pt, int ws, char const *lp){
    strcpy(host, ho);
    port     = pt;
    win_size = ws;
    strcpy(log_path, lp);
    ofstream fileout(log_path,ios::trunc);
}

void WSender::set_package(char *d, int type, int len=0){
    //int len = strlen(d);
    int tot_len = sizeof(struct PacketHeader) + len;
    memset(send_buff, 0, BUFFERSIZESMALL); 
    //set header
    char *packet_ptr = send_buff;
    PacketHeader *wdphdr = (struct PacketHeader*) send_buff;
    wdphdr->type = type;
    wdphdr->seqNum = seq_curr;
    wdphdr->length = len;
    wdphdr->checksum = crc32(d, len);
    //copy data
    char *data_ptr = (char *) (packet_ptr + sizeof(struct PacketHeader));
    if (len>0) memcpy(data_ptr, d, len);
    len_send = tot_len;
}

void WSender::decode_package(){
    char *buf = send_buff;
    PacketHeader *wdphdr = (struct PacketHeader*) buf;
    printf("decoding: %d,%d,%d\n", wdphdr->type, wdphdr->seqNum, wdphdr->length);
    char *recv_buff = (char *) (buf + sizeof(struct PacketHeader));
    printf("%s\n", recv_buff);
}

void WSender::my_send(const struct sockaddr *si_other, socklen_t slen){
    sendto(sockfd, (const char *)send_buff, len_send, 0, si_other, slen); 
    write_to_logfile((struct PacketHeader*) send_buff);
}

void WSender::my_recv(struct sockaddr *si_other, socklen_t *slen){
    len_recv = recvfrom(sockfd, recv_buff, DATALEN, 0, si_other, slen);
    if (len_recv>0) {
        write_to_logfile((struct PacketHeader*) recv_buff); 
        recv_buff[len_recv] = '\0';
    }
}


void WSender::send(char const *path){
    //read input data
    srand(time(NULL));
    len_input = read_to_data(path);

    //prepare data for debug
    /*
    char *chptr=input_data;
    for (int i = 0; i < 10000; ++i){
        strcpy(chptr, "Hello from client");
        chptr += 17;
    } 
    strcpy(chptr, "FIN");
    */
    //UDP connect
    struct sockaddr_in si_me, si_other; 
    socklen_t slen = sizeof(si_other);
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) err("socket creation failed"); 
  
    memset(&si_me, 0, sizeof(si_me)); 
    // Filling server information 
    si_me.sin_family = AF_INET; 
    si_me.sin_port = htons(2000);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sockfd, (struct sockaddr *)&si_me, slen)<0) err("bind failed");
    struct timeval tv;
    tv.tv_sec = 1;        // 30 Secs Timeout
    tv.tv_usec = 0;        // Not init'ing this can cause strange errors
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));
    
    memset(&si_other, 0, sizeof(si_other)); 
    // Filling server information 
    si_other.sin_family = AF_INET; 
    si_other.sin_port = htons(port);
    si_other.sin_addr.s_addr = inet_addr(host);

    int dataFrameSize = (DATALEN-sizeof(struct PacketHeader));
    data_buffer[dataFrameSize] = '\0';
    int tot_seq = seq + (len_input/dataFrameSize) + 1;//TODO
    vector <bool> acks;
    acks.resize(win_size,false);
    
    //START
    int seq_st_true = rand() % BUFFERSIZE;
    seq_curr = seq_st_true;
    set_package(nullptr, 0);
    //decode_package();
    len_recv = -1; 
    while (len_recv<0){
        my_send((const struct sockaddr *) &si_other, slen);
        my_recv((struct sockaddr *) &si_other, &slen);
    }

    //DATA from 0
    seq = 0;
    seq_curr = seq;
    seq_st = seq;

    while (seq < tot_seq){
        //send
        int win_size_real = min(win_size, tot_seq-seq);
        for (int i = 0; i < win_size_real; ++i){
        //for (int i = win_size_real-1; i >= 0 ; --i){
            //if (i==7) continue;
            if (acks[i]) continue;
            seq_curr = seq+i;
            int data_size = (seq_curr == tot_seq-1) ? (len_input%dataFrameSize):dataFrameSize;
            //printf("send-- seq: %d, %d, size: %d\n", seq_curr, _get_curr_seq(), data_size);
            memcpy(data_buffer,input_data+_get_curr_seq()*dataFrameSize,data_size);
            set_package(data_buffer, 2, data_size);
            my_send((const struct sockaddr *) &si_other, slen);
        }

        //len_recv = 1; 
        int seq_head_buff = seq;
        PacketHeader *wdphdr = (struct PacketHeader*) recv_buff;

        while (1){
            my_recv((struct sockaddr *) &si_other, &slen);
            //printf("recv--Seq: %d, Type: %d\n",wdphdr->seqNum,wdphdr->type);
            if (len_recv < 0) break;
            if (len_recv < sizeof(struct PacketHeader) || wdphdr->type>3 || wdphdr->type <0) continue;// garbage
            if (wdphdr->type != 3) continue;//err("recv nonACK");

            int seq_required = (wdphdr->seqNum)-seq_head_buff;
            acks[seq_required] = true;
            if (acks[seq-seq_head_buff]){
                while (acks[seq-seq_head_buff]) {
                    seq++;
                    if (seq-seq_head_buff == win_size) break;
                }
            }
            if (seq-seq_head_buff == win_size or seq >= tot_seq) break;
        }
        if (len_recv==-1){ 
            //printf("%s\n", "TIMEOUT");
        }
        //rearrange acks
        int diff = seq-seq_head_buff;
        int rest = win_size - diff;
        //printf("diff:%d| rest:%d\n", diff, rest);
        for (int i = 0; i < rest; ++i) acks[i] = acks[i+diff];
        for (int i = rest; i < win_size; ++i) acks[i] = false;
    }
    
    //FIN
    seq_curr = seq_st_true;
    set_package(nullptr, 1);
    len_recv = -1; 
    while (len_recv<0){
        my_send((const struct sockaddr *) &si_other, slen);
        my_recv((struct sockaddr *) &si_other, &slen);
    }

    close(sockfd); 
}

int WSender::read_to_data(char const *path){
    std::ifstream fin;
    fin.open(path, std::ios::in|std::ios::binary);
    fin.seekg(0, std::ios::end);
    //data.resize(fin.tellg());
    int sz = fin.tellg();
    fin.seekg(0, std::ios::beg);
    fin.read(input_data, sz);
    fin.close();
    return sz;
}

void WSender::write_to_logfile(PacketHeader *wdphdr){
    //<type> <seqNum> <length> <checksum>
    string log(to_string(wdphdr->type)), sep(" ");
    log += sep + to_string(wdphdr->seqNum);
    log += sep + to_string(wdphdr->length);
    log += sep + to_string(wdphdr->checksum);
    ofstream out(log_path,ios::app);
    out<<log<<endl;
    out.close();
}
