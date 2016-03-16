#ifndef PLAYER_CLIENT_H
#define PLAYER_CLIENT_H


/********************************************************************************************************************************************
 * 
 * TCP is a streaming protocol and is not a "packet-based" protocol like UDP.
 * As such, small data "packets" in TCP are often concatenated/accumulated in the sender's buffer before being sent.
 * This is the result of Nagle's algorithm, which is implemented for congestion control purpose.
 * For this application, most of the packets sent are small, and there's no streaming.
 * At first glance, Nagle's algorithm should be turned off to prevent packets from being concatenated.
 * If the algorithm is not disabled, extra overhead is needed on both the sender and receiver to packetize and process concatenated packets.
 * However, testing showed that even with Nagle's algorithm disabled (by setting the socket option TCP_NODELAY),
 * packet concatenation still occurs, even though less frequently.
 * The program was tested on an Ubuntu 15.10 virtual machine on a Window machine.
 * Information from the internet suggests that this is not a problem on real Linux machines.
 * This means that TCP_NODELAY is system-dependent, which means that it should not be used for networking applications.
 * As such, 4 extra header bytes are added to the beginning of every packet to store the number of bytes in the packet
 * in a packet to allow the receiver to parse concatenated packets.
 * 
 *********************************************************************************************************************************************/
 
 
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctime>
#include "Bot.h"
#include "BotFactory.h"
#include "DumbBot.h"
#include "PunisherBot.h"

#define VERSION_NUM					1

// Message code
#define PLAYER_MOVE 				1
#define PLAYER_SELF_ANNIHILATE 		2
#define PLAYER_SPAWN 				3
#define PLAYER_JOIN_RESPONSE 		4
#define SERVER_MAP_UPDATE 			5
#define PLAYER_SPAWN_WITH_ID 		6
#define ANNIHILATION_RESULTS		7

#define BUFFER_SIZE 				1024
#define MAP_UPDATE_MILLISEC			50
#define PLAYER_LIMIT				20

// Macros for extracting bytes
#define GET_BYTE_3(x)	((x & 0xFF000000) >> 24)
#define GET_BYTE_2(x)	((x & 0x00FF0000) >> 16)		
#define GET_BYTE_1(x)	((x & 0x0000FF00) >> 8)
#define GET_BYTE_0(x)	(x & 0x000000FF)

using namespace std;


// Struct writtent based on udp-client.c UDPClient
typedef struct
{
	int sockfd;
	const char* hostName;
	const char* portNum;
	
	// 4kB buffers
	uint8_t recvBuffer[BUFFER_SIZE];
	uint8_t sendBuffer[BUFFER_SIZE];
	
	struct addrinfo info;
	struct sockaddr addr;
	socklen_t addrlen;
	
} TCPHost;


class PlayerClient
{
	private:
		
		TCPHost* server;
		struct timeval timeout;
		int maxfd;
		
		int botAIType;
		Bot* bot;
		
		fd_set masterSet;
		fd_set readSet;
		fd_set writeSet;
		fd_set exceptSet;
		
		
		/*
		 * Functions to set up sockets and hosts
		 */
		
		// Get the addrinfo of the server at the specified host name and port number
		addrinfo* getTCPServerAddrInfo(const char* hostName, const char* portNum);
		
		// Create a TCP server at the specified host name and port number
		TCPHost* createTCPServer(const char* hostName, const char* portNum);

		// Create a socket file descriptor for the host address
		// Return the socket file descriptor or -1 if unsuccessful
		int createSocketFD(struct addrinfo* hostAddr);
		
		// Set the passed-in socket to non-blocking
		// Return -1 if error
		int setSocketNonBlocking(int sockdf);
		
		
		/*
		 * Player Client utility functions 
		 */
		 
		 // Initialize TCP connection to the server
		 // Return 0 on success, -1 on failure
		 int connectToServer();
		 
		 // process message from server
		 // Return 0 if sucess, -1 if error
		 // Important: This function reads the message received in the buffer and update the bot
		 // The function does not implement any bot AI logic
		 // Bot AI logic is be implemented by the bot itself
		 int processServerMessage();
		
		 // Send player spawn message to the server
		 // Return 0 on success, -1 on failure
		 int sendPlayerSpawnMessage();
		 
		 // Send player move message to server
		 // Return 0 on success, -1 on failure
		 // Note: this function does not "move" the player
		 // It's assumed that the player/bot has already moved
		 // The function simply sends to the server the new position of the player
		 int sendPlayerMoveMessage();
		 
		 // Send player self-annihilate message to server
		 // Return 0 on success, -1 on failure
		 // Note: this function does not "self-annihilate" the player
		 // It's assumed that the player/bot has already self-annihilated
		 // The function simply informs the server of the event of the player
		 int sendPlayerSelfAnnihilateMessage();
		 
	public:
	
		// Create the player client
		// The client is connected to server at server host name and server port num
		// The AI type of the bot is specified by botAITYPE
		PlayerClient(const char* serverHostName, const char* serverPortNum, int botAIType);
		
		~PlayerClient();
		
		void run();
};

#endif
