#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <time.h> 
#include <string.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include "session_send.h"
#include <ctime>
#include <vector>
#include<bits/stdc++.h> 

#include "../starter_files/crc32.h"

#define ACKSIZE 20
#define HALFINTRANGE 16000


using namespace std;

static int err(char const *error){
    printf("Error: %s\n", error);
    exit(EXIT_FAILURE);
}

WSender::WSender(char const *ho, int pt, int ws, char const *lp, char const *dp):
    port(pt), win_size(ws)
{
    strcpy(host, ho);
    //strcpy(log_path, lp);

    // logFile
    fout.open(lp,ios::trunc);

    //loadFile
    fin.open(dp, std::ios::in|std::ios::binary);
    fin.seekg(0, std::ios::end);
    len_input = fin.tellg();
    data_chunk_size = win_size*PUREDATALEN;
    input_data.resize(data_chunk_size*2);
    input_data_buff.resize(data_chunk_size*2);
}

WSender::~WSender(){
    fout.close();
    fin.close();
}

void WSender::set_package(char *d, int type, int len=0){
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
    // ack
    acks.resize(win_size,false);
}

void print_package(const PacketHeader *wdphdr){
    printf("decoding: %d,%d,%d\n", wdphdr->type, wdphdr->seqNum, wdphdr->length);
}

void WSender::my_send(const struct sockaddr *si_other, socklen_t slen){
    sendto(sockfd, (const char *)send_buff, len_send, 0, si_other, slen); 
    write_to_logfile((struct PacketHeader*) send_buff);
}

void WSender::my_recv(struct sockaddr *si_other, socklen_t *slen){
    len_recv = recvfrom(sockfd, recv_buff, ACKSIZE, MSG_DONTWAIT, si_other, slen);
    if (len_recv==ACKSIZE) {
        write_to_logfile((struct PacketHeader*) recv_buff); 
        //recv_buff[len_recv] = '\0';
    }
}

int WSender::load_data(int idx){
    const char *input_data_ptr = input_data.c_str();
    int data_size = (seq_curr==(tot_seq-1)) ? ((len_input-1)%PUREDATALEN+1):PUREDATALEN;
    memcpy(data_buffer,input_data_ptr+idx*PUREDATALEN,data_size);
    return data_size;
}

void WSender::shift_load_chunk(const int step){
    // shift
    printf("shift_load_chunk: %d, %d, %d, seq: %d\n", len_chunk, step*PUREDATALEN, len_input-offset, seq);
    if (seq==tot_seq) return;
    len_chunk -= step*PUREDATALEN;
    printf("chunk: %d\n", len_chunk);
    if (len_chunk<0) err("abuse shift_load_chunk");
    char *input_data_ptr = &input_data[0];
    char *input_data_buff_ptr = &input_data_buff[0];
    printf("start shift\n");
    memcpy(input_data_buff_ptr, &input_data[step*PUREDATALEN], len_chunk);
    memset(input_data_ptr, 0, data_chunk_size*2); //clean data
    memcpy(input_data_ptr, input_data_buff_ptr, len_chunk); //shifted
    if ((len_chunk>=data_chunk_size) || flag_all_load) return;
    // copy
    printf("start copy\n");
    read_to_data(input_data_ptr+len_chunk);
}

void WSender::forward_sw(int seq_recv, int seq_head_buff){
    if (seq_recv > seq){
        tmr.restart();
        for (int i = 0; i < (seq_recv-seq_head_buff); ++i) 
            acks[i] = true;
        seq = seq_recv;
    }
}

void WSender::send(){
    //read input data
    srand(time(NULL));
    //UDP connect
    struct sockaddr_in si_me, si_other; 
    socklen_t slen = sizeof(si_other);
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) err("socket creation failed"); 
  
    memset(&si_me, 0, sizeof(si_me)); 
    // Filling server information 
    si_me.sin_family = AF_INET; 
    int port_me = 2000;
    si_me.sin_port = htons(port_me);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind
    bool pass_bind = false;
    for (int i = 0; i < 10; ++i){
        if (bind(sockfd, (struct sockaddr *)&si_me, slen)>=0) {
            pass_bind = true;
            break;
        }
        si_me.sin_port = htons(++port_me);
    }
    if (!pass_bind) err("bind failed");

    struct timeval tv={0,300000};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval));
    
    memset(&si_other, 0, sizeof(si_other)); 
    // Filling server information 
    si_other.sin_family = AF_INET; 
    si_other.sin_port = htons(port);
    si_other.sin_addr.s_addr = inet_addr(host);

    int dataFrameSize = (DATALEN-sizeof(struct PacketHeader));
    data_buffer[dataFrameSize] = '\0';
    
    //START
    int seq_st_true = (rand() % HALFINTRANGE)+HALFINTRANGE;
    seq_curr = seq_st_true;
    set_package(nullptr, 0);
    //decode_package();
    len_recv = -1; 
    PacketHeader *wdphdr = (struct PacketHeader*) recv_buff;
    while (1){
        my_send((const struct sockaddr *) &si_other, slen);
        my_recv((struct sockaddr *) &si_other, &slen);
        if (len_recv>=0){
            if (wdphdr->seqNum == seq_st_true) break;
        }
    }

    //DATA from 0
    seq = 0;
    seq_curr = seq;
    seq_st = seq;
    tot_seq = seq + (len_input/dataFrameSize);
    printf("tot_seq: %d\n", tot_seq);
    if (len_input%dataFrameSize!=0) ++tot_seq;

    shift_load_chunk(0);

    while (seq < tot_seq){
        //send
        int win_size_real = min(win_size, tot_seq-seq);
        for (int i = 0; i < win_size_real; ++i){
            if (acks[i]) continue;
            seq_curr = seq+i;
            set_package(data_buffer, 2, load_data(i));
            my_send((const struct sockaddr *) &si_other, slen);
        }

        int seq_head_buff = seq;
        tmr.restart();
        while (1){
            my_recv((struct sockaddr *) &si_other, &slen);
            if (tmr.duration_ms() > 300) {
                printf("TO\n");
                break;
            }
            if (len_recv!=ACKSIZE) continue;
            if (wdphdr->type != 3) continue;
            //if (wdphdr->seqNum==seq_st_true) continue;
            if (wdphdr->seqNum<seq_head_buff) continue;
            if (wdphdr->seqNum>(seq_head_buff+win_size_real)) continue;
            forward_sw(wdphdr->seqNum, seq_head_buff);
            if ((seq-seq_head_buff) == win_size) break;
            if (seq >= tot_seq) break;
        }
        

        //rearrange acks/ reload data
        int diff = seq-seq_head_buff;
        int rest = win_size - diff;
        if (diff>0) shift_load_chunk(diff);
        else printf("skip\n");
        std::rotate(acks.begin(), acks.begin()+diff, acks.end());
        for (int i = rest; i < win_size; ++i) acks[i] = false;
    }
    //FIN
    len_recv = 0;
    while (len_recv>=0){
        my_recv((struct sockaddr *) &si_other, &slen);
    }
    seq_curr = seq_st_true;
    set_package(nullptr, 1);
    len_recv = -1; 
    while (1){
        my_send((const struct sockaddr *) &si_other, slen);
        my_recv((struct sockaddr *) &si_other, &slen);
        if (len_recv>=0){
            if (wdphdr->seqNum == seq_st_true) break;
        }
    }

    close(sockfd);
}

void WSender::read_to_data(char *input_data_ptr){
    fin.seekg(offset, std::ios::beg);
    int readSize=min(data_chunk_size, len_input-offset);

    fin.read(input_data_ptr,readSize);
    printf("read:%d", len_chunk);
    len_chunk+=readSize;
    printf(" -> %d\n", len_chunk);
    offset+=readSize;
    readSize=min(data_chunk_size, len_input-offset);
    if(readSize==0) flag_all_load=true;
}

void WSender::write_to_logfile(PacketHeader *wdphdr){
    //<type> <seqNum> <length> <checksum>
    string log(to_string(wdphdr->type)), sep(" ");
    log += sep + to_string(wdphdr->seqNum);
    log += sep + to_string(wdphdr->length);
    log += sep + to_string(wdphdr->checksum);
    fout<<log<<endl;
}
