# include <iostream>
# include <fstream>
# include <string>

# include <sys/types.h>
# include <sys/stat.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <unistd.h>
# include <stdlib.h>
# include <string.h>
# include <time.h>

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
	s.init(wz);
	
	strcpy(output_dir, od);
	strcpy(log_path, lp);

	string outFilePath = string(output_dir) + "/FILE-0.out";
	ofstream output_file;

	cout << "first create output file:"  << outFilePath <<endl;
	output_file.open(outFilePath.c_str(), ios::trunc);
	output_file.close();
	
	port_num = pt_num;
	win_size = wz;
	isblock = 0;
	connect_fin = 0;
	start_seq = 0;
	end_seq = -1;
	no_of_connection = 0;

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

	return total_len;
}

int WReceiver::decode_package(int *dec_pkg_len)
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

	stype = type;
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
			string outFilePath = set_outFile_path(output_dir, no_of_connection);
			ofstream output_file;

			//cout << outFilePath <<endl;
			output_file.open(outFilePath.c_str(), ios::trunc);
			output_file.close();

			if (isblock)
				return -1;

			seq_exp = 0;
			seq_num = seqNum;
			start_seq = seqNum;
			connect_fin = 0;
			ptype = ACK;

			break;
		}
		case END:
		{
			seq_num = seqNum;
			end_seq = seqNum;
			ptype = ACK;
			isblock=0;
			break;
		}
		case DATA:
		{
			isblock = 1;
			if(seq_exp!=seqNum)
			{
				int index;
				index = seqNum - seq_exp;
				s.push(seqNum, dlen, data, index);
			}
			else
			{
				memcpy(dBuffer, data, dlen);
				seq_exp+=1;
				*dec_pkg_len = dlen;
				*dec_pkg_len += s.slide(dBuffer+(*dec_pkg_len));

				while(seq_exp==s.topup())
				{
					seq_exp+=1;
					*dec_pkg_len += s.slide(dBuffer+(*dec_pkg_len));
					//maybe need saving data
				}
			}
			ptype = ACK;
			seq_num = seq_exp;
			break;
		}
		default:
			return -1;
	}
	// cout << "seq_exp: " << seq_exp<<endl;
	// cout << "get seq : " << seqNum<<endl;
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

int WReceiver::write_to_file(char *dir, int No_of_files, int data_size){
	 ofstream outFile;

	 string file_path = set_outFile_path(dir, No_of_files);
	 outFile.open(file_path.c_str(), ios::app|ios::binary);
	 outFile.write(dBuffer, data_size);
	 outFile.close();

	 return 0;
}

const char *WReceiver::set_outFile_path(char *dir, int No_of_files)
{
	if(access(dir, 0) == -1)
	{
		mkdir(dir, S_IRWXU|S_IRWXG |S_IRWXO);
	}

	string file_path = string(dir) + "/FILE-" + to_string(No_of_files)+".out";
	return file_path.c_str();
}

void WReceiver::count_connection()
{
	no_of_connection++;
}

int WReceiver::Receiver()
{
	struct sockaddr_in recv_addr={}, send_addr={};
	socklen_t len = sizeof(send_addr);
	int sockfd;
	// string outFilePath = set_outFile_path(output_dir, no_of_connection);
	// ofstream output_file;

	// cout << outFilePath <<endl;
	// output_file.open(outFilePath.c_str(), ios::trunc);
	// output_file.close();

	if((sockfd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		error("Error opening socket\n");

	recv_addr.sin_family = AF_INET;
	recv_addr.sin_port = htons(port_num);
	recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// struct timeval tv = {0, 3000000};
	// setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(struct  timeval));

    if( bind(sockfd , (struct sockaddr*)&recv_addr, sizeof(recv_addr) ) == -1)
    {
        error("bind");
    }

    // int close_proc = 0;
    while(1)
    {
		int n_recv = recvfrom(sockfd, 
			buffer, 
			buffersize, 
			0, 
			(sockaddr *)&send_addr,
			&len);
		// if (n_recv ==-1 && close_proc == 1) // time out and end connection
		// {
		// 	cout << "time out and quit connection " << endl;
		// 	break;
		// }
		if (n_recv < 0)
			error("Error: reading to socket\n");

		log(buffer, log_path);

		memset(dBuffer, 0, dBuffersize);
		int dec_pkg_len = 0;
		if (!decode_package(&dec_pkg_len))
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
		if (WReceiver::stype == DATA)
			write_to_file(output_dir, no_of_connection, dec_pkg_len);
		if (WReceiver::stype ==END && end_seq == start_seq && connect_fin == 0)
		{
			count_connection();
			connect_fin = 1;
			break;
		}
	}
	close(sockfd);
	//cout << "socket is closed" << endl;
	return 0;

}
