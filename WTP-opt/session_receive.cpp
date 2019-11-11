# include <iostream>
# include <fstream>

# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <unistd.h>
# include <stdlib.h>
# include <string.h>

# include "../starter_files/PacketHeader.h"
# include "../starter_files/crc32.h"
# include "utils.h"
# include "SlidingWindow.h"
# include "session_receive.h"

# define START 			0
# define END 			1
# define DATA 			2
# define ACK 			3
# define buffersize 	2000
# define dBuffersize 	1048576

using namespace std;

WReceiver::WReceiver(int pt_num, int wz, const char *od, const char *lp)
{
	strcpy(output_dir, od);
	strcpy(log_path, lp);
	port_num = pt_num;
	win_size = wz;
	isblock = 0;
	s.init(wz);
	ofstream log;
	log.open(log_path, ios::trunc);
	log.close();
}

int WReceiver::set_package(char *data)
{
	int dlen = strlen(data);
	int total_len = sizeof(PacketHeader)+dlen;

	PacketHeader ACKHeader;
	
	ACKHeader.type = ptype;					// class varible
	ACKHeader.seqNum = seq_num;				// class varible
	ACKHeader.length = dlen;
	ACKHeader.checksum = crc32(data, dlen);
	
	memcpy(buffer, (char *)&ACKHeader, total_len);

	// char *buf;
	// buf = buffer;
	// PacketHeader *pk;
	// pk = (struct PacketHeader *)buf;
	// cout << pk->type << " " << pk->seqNum << " " << pk->length << endl;

	return total_len;
}

int WReceiver::decode_package()
{
	PacketHeader recvPacketHeader;
	char *data=NULL;
	unsigned int type, seqNum, dlen, checksum;

	memset(&recvPacketHeader, 0, sizeof(recvPacketHeader));
	memcpy(&recvPacketHeader, buffer, sizeof(recvPacketHeader));

	type = recvPacketHeader.type;
	seqNum = recvPacketHeader.seqNum;
	dlen = recvPacketHeader.length;
	checksum = recvPacketHeader.checksum;

	data = (char *)(buffer + sizeof(recvPacketHeader));

	// check corruption
	if (crc32(data, dlen)!=checksum)
	{
		cout <<"crc is not right" <<endl;
		return -1;
	}

	switch (type)
	{
		case START:
		{
			if (isblock)
				return -1;

			seq_exp = 0;
			seq_num = seqNum;
			ptype = ACK;
			isblock=1;
			break;
		}
		case END:
		{
			seq_num = seqNum;
			ptype = ACK;
			isblock=0;
			break;
		}
		case DATA:
		{
			if(seq_exp!=seqNum)
			{
				int index;
				index = seqNum - seq_exp;
				s.push(seqNum, data, index);
				seq_num = seqNum;
			}
			else
			{
				seq_num = seq_exp;
				seq_exp+=1;
				s.slide(dBuffer);
				while(seq_exp==s.topup())
				{
					seq_exp+=1;
					s.slide(dBuffer);
					//maybe need saving data
				}
			}
			ptype = ACK;
			break;
		}
		default:
			return -1;

	}
	return 0;
}

int WReceiver::log(char *message, char *lp)
{
	PacketHeader *PkHeader;
	PkHeader = (struct PacketHeader *)message;
	ofstream log;

    log.open(log_path, ios::app);
    log <<  PkHeader->type << " " << PkHeader->seqNum 
    	<< " " << PkHeader->length << " " << PkHeader->checksum << "\n";;
	log.close();

	return 0;
}

int WReceiver::Receiver()
{
	struct sockaddr_in recv_addr={}, send_addr={};
	socklen_t len = sizeof(send_addr);
	int sockfd;
	
	if((sockfd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		error("Error opening socket\n");

	recv_addr.sin_family = AF_INET;
	recv_addr.sin_port = htons(port_num);
	recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if( bind(sockfd , (struct sockaddr*)&recv_addr, sizeof(recv_addr) ) == -1)
    {
        error("bind");
    }
    while(1)
    {
		int n_recv = recvfrom(sockfd, 
			buffer, 
			buffersize, 
			0, 
			(sockaddr *)&send_addr,
			&len);
		if (n_recv<0)
			error("Error: reading from socket\n");
		log(buffer, log_path);

		if (!decode_package())
		{
			int len_packet = set_package((char *)"");
			int n_send = sendto(sockfd, 
				buffer, 
				20, 
				0, 
				(struct sockaddr*)&send_addr,
				len);
			if(n_send < 0)
				error("Error: writting to socket\n");
			log(buffer, log_path);
		}
	}
	close(sockfd);
	return 0;

}