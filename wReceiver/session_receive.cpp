# include <iostream>
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
}

int WReceiver::set_package(char *data)
{
	int dlen = strlen(data);
	int total_len = sizeof(PacketHeader)+dlen;

	PacketHeader ACKHeader;
	
	ACKHeader.type = type;					// class varible
	ACKHeader.seqNum = seq_num;				// class varible
	ACKHeader.length = dlen;
	ACKHeader.checksum = crc32(data, dlen);

	memcpy(buffer, (char *)&ACKHeader, total_len);
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
		return -1;
	}

	cout << "here?" << endl;
	switch (type)
	{
		case START:
		{
			if (isblock)
				return -1;

			seq_exp = 0;
			seq_num = seqNum;
			type = ACK;
			isblock=1;
			break;
		}
		case END:
		{
			seq_num = seqNum;
			type = ACK;
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
			}
			else
			{
				seq_exp+=1;
				while(seq_exp==s.slide(dBuffer))
				{
					seq_exp+=1;
					//maybe need saving data
				}
			}
			type = ACK;
			seq_num = seq_exp;
			break;
		}
		default:
			return -1;

	}

	return 0;
}

int WReceiver::Receiver()
{
	struct sockaddr_in recv_addr={}, send_addr={};
	socklen_t len = sizeof(send_addr);
	int sockfd;
	
	// // sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if((sockfd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		error("Error opening socket\n");

	recv_addr.sin_family = AF_INET;
	recv_addr.sin_port = htons(port_num);
	recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if( bind(sockfd , (struct sockaddr*)&recv_addr, sizeof(recv_addr) ) == -1)
    {
        error("bind");
    }

	int n_recv = recvfrom(sockfd, 
		buffer, 
		buffersize, 
		0, 
		(sockaddr *)&send_addr,
		&len);
	if (n_recv<0)
		error("Error: reading from socket\n");

	cout << "revieved datasize: " << n_recv <<endl;
	cout << buffer<<endl;

	if (!decode_package())
	{
		int len_packet = set_package((char *)"");
		sendto(sockfd, 
			buffer, 
			buffersize, 
			0, 
			(struct sockaddr*)&send_addr,
			len);
	}
	else 
	{
		cout << "here?" << endl;
		sendto(sockfd,
			"The connection is rejected.\n",
			28,
			0,
			(struct sockaddr*)&send_addr,
			len);
	}
	close(sockfd);
	return 0;

}