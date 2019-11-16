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

#include "../starter_files/crc32.h"

#define ACKSIZE 20
#define HALFINTRANGE 16000
#define PUREDATALEN 1456


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

    //loadFile
    /*
    fin.open(path, std::ios::in|std::ios::binary);
    fin.seekg(0, std::ios::end);
    len_input = fin.tellg();
    input_data.resize(win_size*PUREDATALEN*2);
    input_data_buff.resize(win_size*PUREDATALEN);
    */
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
    len_recv = recvfrom(sockfd, recv_buff, ACKSIZE, 0, si_other, slen);
    if (len_recv>=0) {
        write_to_logfile((struct PacketHeader*) recv_buff); 
        recv_buff[len_recv] = '\0';
    }
}

void WSender::send(char const *path){
    //read input data
    srand(time(NULL));
    strcpy(input_path, path);
    read_to_data();
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
    vector <bool> acks;
    acks.resize(win_size,false);
    
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
    last_seq = seq;
    int tot_seq = seq + (len_input/dataFrameSize);
    if (len_input%dataFrameSize!=0) ++tot_seq;

    const char *input_data_ptr = input_data.c_str();

    while (seq < tot_seq){
        //send
        int win_size_real = min(win_size, tot_seq-seq);
        for (int i = 0; i < win_size_real; ++i){
        //for (int i = win_size_real-1; i >= 0 ; --i){
            //if (i==7) continue;
            if (acks[i]) continue;
            seq_curr = seq+i;
            int data_size = ((seq_curr==tot_seq-1) &&(len_input%dataFrameSize!=0)) ? (len_input%dataFrameSize):dataFrameSize;
            memcpy(data_buffer,input_data_ptr+_get_curr_seq()*dataFrameSize,data_size);
            set_package(data_buffer, 2, data_size);
            my_send((const struct sockaddr *) &si_other, slen);
        }

        //len_recv = 1; 
        int seq_head_buff = seq;
        while (1){
            my_recv((struct sockaddr *) &si_other, &slen);
            if (len_recv < 0) break;
            if ((len_recv!=ACKSIZE) || (wdphdr->type != 3)) continue;// garbage
            if ((wdphdr->seqNum==seq_st_true) || (wdphdr->seqNum<seq_head_buff) || (seq_head_buff>(seq_head_buff+win_size_real))) continue;

            int seq_required = (wdphdr->seqNum)-seq_head_buff;
            if (seq_required > (seq-seq_head_buff)){
                for (int i = 0; i < seq_required; ++i) acks[i] = true;
                seq = (wdphdr->seqNum);
            }
            if (seq-seq_head_buff == win_size or seq >= tot_seq) break;
        }

        //rearrange acks
        int diff = seq-seq_head_buff;
        int rest = win_size - diff;

        std::rotate(acks.begin(), acks.begin()+diff, acks.end());
        for (int i = 0; i < rest; ++i) acks[i] = acks[i+diff];
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
    //fin.close();
}
void WSender::read_to_data(){
    std::ifstream fin;
    fin.open(input_path, std::ios::in|std::ios::binary);
    fin.seekg(0, std::ios::end);
    input_data.resize(fin.tellg());
    len_input = fin.tellg();
    fin.seekg(0, std::ios::beg);
    fin.read(&input_data[0], len_input);
    fin.close();
}
/*
void WSender::read_to_data(char const *path){
    fin.seekg(offset, std::ios::beg);
    int sizeforsegment = PUREDATALEN*win_size
    int readSize=min(sizeforsegment, len_input-offset);

    fin.read(&input_data[offset],readSize);
    offset+=readSize;
    readSize=min(sizeforsegment, len_input-offset);
    if(readSize==0){ flag_fin=true; }
}
*/

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
