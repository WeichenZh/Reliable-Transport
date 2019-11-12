# ifndef SESSION_REVEIVE_H
# define SESSION_REVEIVE_H

# include "SlidingWindow.h"

# define buffersize 2000
# define dBuffersize 1048576
using namespace std;

class WReceiver
{
	// input 
	int port_num, win_size;
	char output_dir[1000], log_path[1000];

	// socket
	int socket;
	unsigned int seq_num, seq_exp, ptype, stype; 	// seq_num should always equal to seq_exp, except when type=START/END
	int isblock;							//block other connection

	// receive & ACK
	char buffer[buffersize];
	char dBuffer[dBuffersize];
	slidingwindow s;
public:
	WReceiver(int pt_num, int wz, const char *od, const char *lp);
	int set_package(char *data);
	int decode_package();
	int log(char *message, char *lp);
	int Receiver();

};

# endif