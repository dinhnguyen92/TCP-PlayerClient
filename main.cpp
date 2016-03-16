#include <cstdlib>
#include "PlayerClient.h"


int main(int argc, const char* argv[])
{
	// 3 argument is expected for hostname, portnum, and bot type apart from the program name
	if (argc != 4)
	{
		fprintf(stderr, "Wrong number of arguments\n");
		fprintf(stdout, "Format: './client [hostname] [portnum] [bot type]'\n");
		return 0;
	}
	
	// Parse the argument into AI type code
	int AIType = atoi(argv[3]);
	
	// Set host to "127.0.0.1" to test client and server on same machine
	PlayerClient* playerClient = new PlayerClient(argv[1], argv[2], AIType);
	
	playerClient->run();
	
	return 0;
}
