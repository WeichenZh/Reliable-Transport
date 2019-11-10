#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
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

int err(char const *error){
    printf("Error: %s\n", error);
    exit(EXIT_FAILURE);
}


int main(int argc, char const *argv[]){
    //UDP connect
    struct sockaddr_in     si_me, si_other; 
    socklen_t slen = sizeof(si_other);
    int sockfd;
    char packet[2000], sentbuff[2000];
    bool flagFin = false;

    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) err("socket creation failed"); 
    memset(&si_me, 0, sizeof(si_me)); 
    // Filling server information 
    si_me.sin_family = AF_INET; 
    si_me.sin_port = htons(8080); 
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sockfd, (struct sockaddr *)&si_me, slen)<0) err("bind failed");

    while (true){
        int n = recvfrom(sockfd, (char *)packet, 1500, 0, 
                     (struct sockaddr *) &si_other, &slen); 
        packet[n] = '\0';
        PacketHeader *wdphdr = (struct PacketHeader*) packet;

        char content_head[20];
        content_head[19] = '\0';
        strncpy(content_head, packet+sizeof(PacketHeader), 19);
        //printf("recv: %s\n", content_head);
        printf("recv:%d: %s\n", wdphdr->type, packet+sizeof(PacketHeader));
        
        if (wdphdr->type == 1){
            flagFin = true;
            printf("%s\n", "FIN");
        }

        //printf("%d, %d\n", wdphdr->type, wdphdr->seqNum);
        /*
        switch(wdphdr->type) {
            case 0 :{
                printf("START\n");
            }
            case 1 :{
                flagFin = true;
                printf("FIN\n");
            }
            case 2 :{
                char *chrptr = packet + sizeof(struct PacketHeader);
                printf("%s\n", chrptr);
            }
            case 3 : err("type");
        }
        strncpy(sentbuff, packet, sizeof(struct PacketHeader));
        wdphdr = (struct PacketHeader*) sentbuff;
        */
        //printf("%d, %d\n", wdphdr->type, wdphdr->seqNum);
        wdphdr->type = 3;
        wdphdr->length = 0;
        wdphdr->checksum = 0;
        (wdphdr->seqNum)++;
        printf("%d, %d\n", wdphdr->type, wdphdr->seqNum);
        sendto(sockfd, (const char *)packet, sizeof(struct PacketHeader), 0, 
               (const struct sockaddr *) &si_other,  slen); 
        
        if (flagFin) break;
    }    
    close(sockfd); 
    return 0;
}