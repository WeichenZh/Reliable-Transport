# include <iostream>
# include <stdlib.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <unistd.h>
# include "utils.h"
# include "session_receive.h"

using namespace std;

int main(int argc, char const *argv[])
{
	if (argc != 5) 
		error("Error: missing or extra arguments\n");
	int port, win_size;
	const char *output_dir, *log_path;

	port  = atoi(argv[1]);
	win_size = atoi(argv[2]);
	output_dir = argv[3];
	log_path = argv[4];

	string outFilePath = string(output_dir) + "/FILE-0.out";
	ofstream output_file;

	cout << "first create output file:"  << outFilePath <<endl;
	output_file.open(outFilePath.c_str(), ios::trunc);
	output_file.close();
	
	WReceiver receiver(port, win_size, output_dir, log_path);
	while(1)
	{
		receiver.Receiver();
		//receiver.count_connection();
	}
	return 0;
}
