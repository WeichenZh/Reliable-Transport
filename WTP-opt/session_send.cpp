#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
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
#include "../starter_files/PacketHeader.h"


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

int WSender::set_package(char *d, int type, int len=0){
    //int len = strlen(d);
    int tot_len = sizeof(struct PacketHeader) + len;
    //set header
    char *packet_ptr = packet;
    PacketHeader *wdphdr = (struct PacketHeader*) packet;
    wdphdr->type = type;
    wdphdr->seqNum = seq_curr;
    wdphdr->length = len;
    wdphdr->checksum = crc32(d, len);
    //copy data
    char *send_buff = (char *) (packet_ptr + sizeof(struct PacketHeader));
    if (len>0) memcpy(send_buff, d, len);
    return tot_len;
}

void WSender::decode_package(){
    char *buf = packet;
    PacketHeader *wdphdr = (struct PacketHeader*) buf;
    printf("decoding: %d,%d,%d\n", wdphdr->type, wdphdr->seqNum, wdphdr->length);
    char *recv_buff = (char *) (buf + sizeof(struct PacketHeader));
    printf("%s\n", recv_buff);
}

void WSender::send(char const *path){
    //read input data
    read_to_data(path);


    //prepare data for debug
    char *hello = "Hello from client", *chptr=input_data;
    for (int i = 0; i < 10000; ++i){
        strcpy(chptr, hello);
        chptr += strlen(hello);
    } 
    strcpy(chptr, "FIN");
    
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
    //START
    int dataFrameSize = (DATALEN-sizeof(struct PacketHeader));
    data_buffer[dataFrameSize] = '\0';
    int n, len_package, tot_seq = seq + (strlen(input_data)/dataFrameSize) + 1;//TODO
    vector <bool> acks;
    acks.resize(win_size,false);
    
    int seq_st_true = rand() % BUFFERSIZE;
    seq_curr = seq_st_true;
    //printf("data: %d\n", strlen(input_data));
    //printf("tot_seq: %d\n", tot_seq);
    len_package = set_package("", 0);
    sendto(sockfd, (const char *)packet, len_package, 0, 
           (const struct sockaddr *) &si_other, slen); 
    write_to_logfile();
    //recv
    n = recvfrom(sockfd, (char *)packet, BUFFERSIZESMALL, 0, 
                 (struct sockaddr *) &si_other, &slen); 
    packet[n] = '\0';
    Timer tmr;
    seq = 0; // rand() % BUFFERSIZE;
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
            int data_size = (seq_curr == tot_seq-1) ? (sizeof(input_data)%dataFrameSize):dataFrameSize;
            //printf("send-- seq: %d, %d, size: %d\n", seq_curr, _get_curr_seq(), data_size);
            memcpy(data_buffer,input_data+_get_curr_seq()*dataFrameSize,data_size);
            len_package = set_package(data_buffer, 2, data_size);
            write_to_logfile();
            sendto(sockfd, (const char *)packet, len_package, 0, 
                   (const struct sockaddr *) &si_other, slen); 
        }

        n = 1; 
        int seq_head_buff = seq;
        PacketHeader *wdphdr = (struct PacketHeader*) packet;
        while (1){
            n = recvfrom(sockfd, (char *)packet, BUFFERSIZESMALL, 0, 
                         (struct sockaddr *) &si_other, &slen);
            if (n < 0) break;
            if (n < sizeof(struct PacketHeader) || wdphdr->type>3 || wdphdr->type <0) continue;// garbage
            packet[n] = '\0';

            //printf("recv--Seq: %d, Type: %d\n",wdphdr->seqNum,wdphdr->type);
            if (wdphdr->type != 3) err("recv nonACK");

            int seq_acked = (wdphdr->seqNum)-seq_head_buff;
            acks[seq_acked] = true;
            //printf("%d\n", seq-seq_head_buff);
            if (acks[seq-seq_head_buff]){
                while (acks[seq-seq_head_buff]) {
                    seq++;
                    if (seq-seq_head_buff == win_size) break;
                }
                tmr.restart();
            }
            else if (tmr.duration_ms() > 500){//timeout
                printf("%s\n", "should not use this!");
                tmr.restart();
                break;
            }
            if (seq-seq_head_buff == win_size or seq >= tot_seq) break;
        }
        /*
        if (n==-1){ 
            printf("%s\n", "TIMEOUT");
        }
        */
        int diff = seq-seq_head_buff;
        int rest = win_size - diff;
        //printf("diff:%d| rest:%d\n", diff, rest);
        for (int i = 0; i < rest; ++i){
            acks[i] = acks[i+diff];
        }
        for (int i = rest; i < win_size; ++i){
            acks[i] = false;
        }
    }
    
    //FIN
    seq_curr = seq_st_true;
    memset(packet, 0, DATALEN); 
    len_package = set_package("", 1);
    sendto(sockfd, (const char *)packet, len_package, 0, 
           (const struct sockaddr *) &si_other, slen); 
    write_to_logfile();
    
    n = recvfrom(sockfd, (char *)packet, BUFFERSIZESMALL, MSG_WAITALL, 
                 (struct sockaddr *) &si_other, &slen); 
    packet[n] = '\0';

    close(sockfd); 
}

void WSender::read_to_data(char const *path){
    //plz read file "path" to WSender.data 
}


void WSender::write_to_logfile(){
    //<type> <seqNum> <length> <checksum>
    PacketHeader *wdphdr = (struct PacketHeader*) packet;
    string log(to_string(wdphdr->type)), sep(" ");
    log += sep + to_string(wdphdr->seqNum);
    log += sep + to_string(wdphdr->length);
    log += sep + to_string(wdphdr->checksum);
    ofstream out(log_path,ios::app);
    out<<log<<endl;
    out.close();
}
