#include "PlayerClient.h"


addrinfo* PlayerClient::getTCPServerAddrInfo(const char* hostName, const char* portNum)
{
	// Written based on "socket-tutorial" by GauthierDickey
	
	// Set hints for creating server address info
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	
	// Create the server address info
	struct addrinfo* serverAddrInfo;
	
	int result = getaddrinfo(hostName, portNum, &hints, &serverAddrInfo);
	
	if (result != 0)
	{
		fprintf(stderr, "Error resolving port %s: %s\n", portNum, gai_strerror(result));
		return NULL;
	}
	
	return serverAddrInfo;
}


TCPHost* PlayerClient::createTCPServer(const char* hostName, const char* portNum)
{
	struct addrinfo* serverAddr = getTCPServerAddrInfo(hostName, portNum);
	
	if (serverAddr == NULL) return NULL;
	
	int sockfd = createSocketFD(serverAddr);
	
	if (sockfd == -1) return NULL;
	
	int res = setSocketNonBlocking(sockfd);
	
	if (res == -1) return NULL;
	
	// Create the TCPHost 
	TCPHost* host = (TCPHost*)malloc(sizeof(TCPHost));
	
	if (host == NULL)
	{
		fprintf(stderr, "Failed to allocate memory for TCP server.\n");
		return NULL;
	}
	
	memset(host, 0, sizeof(TCPHost));
	
	host->sockfd = sockfd; 
	host->hostName = hostName;
	host->portNum = portNum;
	memcpy(&host->info, serverAddr, sizeof(struct addrinfo));
	memcpy(&host->addr, serverAddr->ai_addr, sizeof(struct sockaddr));
	host->addrlen = serverAddr->ai_addrlen;
	
	return host;
}


int PlayerClient::createSocketFD(struct addrinfo* hostAddr)
{
	// Written based on "socket-tutorial" by GauthierDickey
	
	int sockfd;
	struct addrinfo* p;
	
	// Iterate through the list of host address info
	// Create and bind the socket to the first one that workds
	for (p = hostAddr; p != NULL; p = p->ai_next)
	{
		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		
		if (sockfd == - 1)
		{
			perror("Unable to create socket ");
			continue;
		}
		break;
	}
	
	if (p == NULL)
	{
		fprintf(stderr, "Failed to bind socket to a valid server address.\n");
		return -1;
	}
	
	return sockfd;
}


int PlayerClient::setSocketNonBlocking(int sockfd)
{
	// Written based on "socket-tutorial" by GauthierDickey
	
	// Get current flags of the socket
	int flags = fcntl(sockfd, F_GETFL);
	
	if (flags == -1)
	{
		perror("Failed to get socket flags using fcntl() ");
		return flags;
	}
	
	// Set socket flag to non-blocking
	int res = fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
	
	if (res == -1)
	{
		perror("Failed to set socket to non-blocking using fcntl() ");
	}
	
	return res;
}


PlayerClient::PlayerClient(const char* serverHostName, const char* serverPortNum, int AIType)
{
	server = createTCPServer(serverHostName, serverPortNum);
	
	if (server == NULL)
	{
		fprintf(stderr, "ERROR: game server not created\n");
		exit(EXIT_FAILURE);
	}
	
	// Since the server socket is the first socket
	// It is also the maximum socket fd
	maxfd = server->sockfd;
	
	timeout.tv_sec = 0;
	timeout.tv_usec = 2000;

	botAIType = AIType;
	
	fprintf(stdout, "Player client created\n");
}


PlayerClient::~PlayerClient()
{
	if (server != NULL)
	{
		close(server->sockfd);
		delete server;
	}
	if (bot != NULL) delete bot;
}


void PlayerClient::run()
{
	fprintf(stdout, "Player client started\n");
	
	int res = connectToServer();
	
	// If there's an error, terminate
	if (res == -1)
	{
		fprintf(stderr, "Failed to connect to the server: %s\n", strerror(errno));
		return;
	}
	
	fprintf(stdout, "Connected to server %s at port %s\n", server->hostName, server->portNum);
	
	while (true)
	{
		// Reset the file descriptor master set
		// The master set is used to keep track of active sockets
		// Once all active sockets are added to the master set
		// The master set is copied to all other fd sets
		FD_ZERO(&masterSet);
		
		// Add all active sockets to the master set
		// Currently the server socket is the only active socket
		FD_SET(server->sockfd, &masterSet);
		
		// Copy the master set to other fd sets
		readSet = masterSet;
		writeSet = masterSet;
		exceptSet = masterSet;
		
		// Use select to wait for socket activity
		res = select(maxfd + 1, &readSet, &writeSet, &exceptSet, &timeout);
		
		// If there's an error
		if (res == -1)
		{
			fprintf(stderr, "Error waiting for socket activity: %s\n", strerror(errno));
			continue;
		}
		
		// If the server sends a message
		if (FD_ISSET(server->sockfd, &readSet))
		{
			//fprintf(stdout, "Message received from server\n");
			
			int code = processServerMessage();
			
			if (code == -1)
			{
				fprintf(stderr, "Error processing message from server\n");
			}		
		}
		// If the socket is ready to be written to 
		if (FD_ISSET(server->sockfd, &writeSet))
		{
			int action = STANDBY;
			
			if (bot != NULL)
			{
				action = bot->performAction();
			}
			
			switch(action)
			{
				case MOVE:
					sendPlayerMoveMessage();
					break;
					
				case EXPLODE:
					sendPlayerSelfAnnihilateMessage();
					break;
					
				case SPAWN:
					sendPlayerSpawnMessage();
					break;
					
				case STANDBY:
					// do nothing
					break;
					
				default:
					break;
			}
		}
		if (FD_ISSET(server->sockfd, &exceptSet))
		{
			// No game logic for now
			// No error handling for now
		}
	}
}


int PlayerClient::connectToServer()
{
	int res = connect(server->sockfd, &server->addr, server->addrlen);
	
	// If there's an error, retry 3 times
	int count = 3;
	
	while (res == -1 && count > 0)
	{
		res = connect(server->sockfd, &server->addr, server->addrlen);
		count--;
	}
	
	return res;
}


int PlayerClient::processServerMessage()
{
	ssize_t bytes = recv(server->sockfd, server->recvBuffer, BUFFER_SIZE, 0); 
	
	if (bytes == -1)
	{
		fprintf(stderr, "Error receiving server message: %s\n", strerror(errno));
		return -1;
	}
	if (bytes == 0)
	{
		return 0;
	}
	
	// Read the number of bytes in the packet
	uint32_t rawBytes = 0;
	rawBytes |= ((uint32_t)server->recvBuffer[0]) << 24;
	rawBytes |= ((uint32_t)server->recvBuffer[1]) << 16;
	rawBytes |= ((uint32_t)server->recvBuffer[2]) << 8;
	rawBytes |= ((uint32_t)server->recvBuffer[3]);
	
	uint32_t numBytes = ntohl(rawBytes);
	
	// If the number of bytes actually received is less than the expected number of bytes
	if (bytes < numBytes)
	{
		fprintf(stderr, "Number of bytes received from server is less than expected. Received %ld bytes, expected %u bytes\n", bytes, numBytes);
		
		for (int i = 0; i < bytes; i++)
		{
			fprintf(stdout, "byte %d: %d\n", i, server->recvBuffer[i]);
		}
		
		return -1;
	}
	// Check the version number
	if (server->recvBuffer[4] != VERSION_NUM)
	{
		fprintf(stderr, "Wrong version number in server message\n");
		return -1;
	}
	
	int res = 0;
	
	// Check the message code
	switch(server->recvBuffer[5])
	{
		case PLAYER_JOIN_RESPONSE:
		{
			// 10 bytes are expected for player join response message
			if (numBytes != 10)
			{
				fprintf(stderr, "Wrong number of bytes received in player join response message: %u\n", numBytes);
				
				for (int i = 0; i < (int)numBytes; i++)
				{
					fprintf(stdout, "Byte %d: %d\n", i, server->recvBuffer[i]);
				}
				res = -1;
			}
			else
			{
				// Read the player ID
				int32_t rawBits = 0;
				rawBits |= ((int32_t)server->recvBuffer[6]) << 24; 	// byte 3 of ID
				rawBits |= ((int32_t)server->recvBuffer[7]) << 16; 	// byte 2 of ID
				rawBits |= ((int32_t)server->recvBuffer[8]) << 8;	// byte 1 of ID
				rawBits |= ((int32_t)server->recvBuffer[9]);		// byte 0 of ID
				
				int botID = ntohl(rawBits);
				
				fprintf(stdout, "Player join response received from server. Assigned ID: %d\n", botID);
					
				// Initialize the bot
				bot = BotFactory::createBot(botAIType, PLAYER_LIMIT, botID);
			}
			break;
		}
		case SERVER_MAP_UPDATE:
		{
			// At least 8 bytes are expected for map update
			if (numBytes < 8)
			{
				fprintf(stderr, "Wrong number of bytes received in map update message: %u\n", numBytes);
				for (int i = 0; i < (int)numBytes; i++)
				{
					fprintf(stdout, "Byte %d: %d\n", i, server->recvBuffer[i]);
				}
				res = -1;
			}
			else
			{
				// Read the number of players in the map
				uint16_t rawNumPlayers = 0;
				rawNumPlayers |= ((uint16_t)server->recvBuffer[6]) << 8; 	// byte 1 of numPlayers
				rawNumPlayers |= ((uint16_t)server->recvBuffer[7]); 		// byte 0 of numPlayers
				
				int16_t numPlayers = (int16_t)(ntohs(rawNumPlayers));
				
				int index = 8;
				
				// Iterate through each player on map and read their info
				for (int32_t i = 0; i < (int32_t)numPlayers; i++)
				{
					// Read the player's ID		
					uint32_t rawID = 0;
					rawID |= ((uint32_t)server->recvBuffer[index]) << 24; 		// byte 3 of ID
					rawID |= ((uint32_t)server->recvBuffer[index + 1]) << 16; 	// byte 2 of ID
					rawID |= ((uint32_t)server->recvBuffer[index + 2]) << 8;	// byte 1 of ID
					rawID |= ((uint32_t)server->recvBuffer[index + 3]);			// byte 0 of ID
					
					int32_t playerID = (int32_t)ntohl(rawID);
					
					uint32_t rawX = 0;
					uint32_t rawY = 0;
					uint32_t rawZ = 0;
					
					float x = 0;
					float y = 0;
					float z = 0;
					
					// Read the player's x coordinate
					rawX |= ((uint32_t)server->recvBuffer[index + 4]) << 24; 	// byte 3 of x
					rawX |= ((uint32_t)server->recvBuffer[index + 5]) << 16; 	// byte 2 of x
					rawX |= ((uint32_t)server->recvBuffer[index + 6]) << 8;		// byte 1 of x
					rawX |= ((uint32_t)server->recvBuffer[index + 7]);			// byte 0 of x				
					uint32_t binaryX = ntohl(rawX);
					memcpy(&x, &binaryX, sizeof(float));

					// Read the player's y coordinate
					rawY |= ((uint32_t)server->recvBuffer[index + 8]) << 24; 	// byte 3 of y
					rawY |= ((uint32_t)server->recvBuffer[index + 9]) << 16; 	// byte 2 of y
					rawY |= ((uint32_t)server->recvBuffer[index + 10]) << 8;	// byte 1 of y
					rawY |= ((uint32_t)server->recvBuffer[index + 11]);			// byte 0 of y					
					uint32_t binaryY = ntohl(rawY);
					memcpy(&y, &binaryY, sizeof(float));
					
					// Read the player's z coordinate
					rawZ |= ((uint32_t)server->recvBuffer[index + 12]) << 24; 	// byte 3 of z
					rawZ |= ((uint32_t)server->recvBuffer[index + 13]) << 16; 	// byte 2 of z
					rawZ |= ((uint32_t)server->recvBuffer[index + 14]) << 8;	// byte 1 of z
					rawZ |= ((uint32_t)server->recvBuffer[index + 15]);			// byte 0 of z					
					uint32_t binaryZ = ntohl(rawZ);
					memcpy(&z, &binaryZ, sizeof(float));
					
					// Update the bot about the player's location
					if (bot != NULL)
					{
						bot->playerLocationUpdat(playerID, x, y, z);
					}			
					
					// Move the index to the next 16 bytes block
					index += 16;
				}
			}
			break;
		}
		case PLAYER_SPAWN_WITH_ID:
		{
			// 22 bytes are expected in player spawn with ID message from server
			if (numBytes != 22)
			{
				fprintf(stderr, "Wrong number of bytes received in player spawn with ID message: %u\n", numBytes);
				for (int i = 0; i < (int)numBytes; i++)
				{
					fprintf(stdout, "Byte %d: %d\n", i, server->recvBuffer[i]);
				}
				res = -1;
			}
			else
			{
				// Read the player's ID
	
				uint32_t rawID = 0;
				rawID |= ((uint32_t)server->recvBuffer[6]) << 24; 		// byte 3 of ID
				rawID |= ((uint32_t)server->recvBuffer[7]) << 16; 	// byte 2 of ID
				rawID |= ((uint32_t)server->recvBuffer[8]) << 8;	// byte 1 of ID
				rawID |= ((uint32_t)server->recvBuffer[9]);			// byte 0 of ID
					
				int32_t playerID = (int32_t)ntohl(rawID);
				
				uint32_t rawX = 0;
				uint32_t rawY = 0;
				uint32_t rawZ = 0;
				
				float x = 0;
				float y = 0;
				float z = 0;
				
				// Read the player's x coordinate
				rawX |= ((uint32_t)server->recvBuffer[10]) << 24; 	// byte 3 of x
				rawX |= ((uint32_t)server->recvBuffer[11]) << 16; 	// byte 2 of x
				rawX |= ((uint32_t)server->recvBuffer[12]) << 8;		// byte 1 of x
				rawX |= ((uint32_t)server->recvBuffer[13]);			// byte 0 of x				
				uint32_t binaryX = ntohl(rawX);
				memcpy(&x, &binaryX, sizeof(float));

				// Read the player's y coordinate
				rawY |= ((uint32_t)server->recvBuffer[14]) << 24; 	// byte 3 of y
				rawY |= ((uint32_t)server->recvBuffer[15]) << 16; 	// byte 2 of y
				rawY |= ((uint32_t)server->recvBuffer[16]) << 8;	// byte 1 of y
				rawY |= ((uint32_t)server->recvBuffer[17]);			// byte 0 of y					
				uint32_t binaryY = ntohl(rawY);
				memcpy(&y, &binaryY, sizeof(float));
				
				// Read the player's z coordinate
				rawZ |= ((uint32_t)server->recvBuffer[18]) << 24; 	// byte 3 of z
				rawZ |= ((uint32_t)server->recvBuffer[19]) << 16; 	// byte 2 of z
				rawZ |= ((uint32_t)server->recvBuffer[20]) << 8;	// byte 1 of z
				rawZ |= ((uint32_t)server->recvBuffer[21]);			// byte 0 of z					
				uint32_t binaryZ = ntohl(rawZ);
				memcpy(&z, &binaryZ, sizeof(float));
				
				// Inform the bot of the new spawn
				if (bot != NULL)
				{
					bot->playerSpawnUpdate(playerID, x, y, z);
				}
				
				fprintf(stdout, "Player %d spawned at {%.2f, %.2f, %.2f}\n", playerID, x, y, z);
			}
			break;
		}
		case ANNIHILATION_RESULTS:
		{
			// At least 12 bytes are expected from annihilation results message
			if (numBytes < 12)
			{
				fprintf(stderr, "Wrong number of bytes received in annihilation result message: %u\n", numBytes);
				for (int i = 0; i < (int)numBytes; i++)
				{
					fprintf(stdout, "Byte %d: %d\n", i, server->recvBuffer[i]);
				}
				res = -1;
			}
			else
			{
				// Read the ID of self-annihilated player		
				uint32_t rawID = 0;
				rawID |= ((uint32_t)server->recvBuffer[6]) << 24; 		// byte 3 of ID
				rawID |= ((uint32_t)server->recvBuffer[7]) << 16; 	// byte 2 of ID
				rawID |= ((uint32_t)server->recvBuffer[8]) << 8;	// byte 1 of ID
				rawID |= ((uint32_t)server->recvBuffer[9]);			// byte 0 of ID
					
				int32_t killerID = (int32_t)ntohl(rawID);
				
				// Inform the bot that a player is killed
				if (bot != NULL)
				{
					bot->playerKilledUpdate(killerID);
				}
				
				fprintf(stdout, "Player %d self-annihilated!!!\n", killerID);
				
				// Read the number of player killed
				uint16_t rawNumKills = 0;
				rawNumKills |= ((uint16_t)server->recvBuffer[10]) << 8; 	// byte 1 of numKills
				rawNumKills |= ((uint16_t)server->recvBuffer[11]); 		// byte 0 of numKills
				
				int16_t numKills = (int16_t)(ntohs(rawNumKills));
				
				// Update the score of the player if the player is the one causing the explosion
				if (bot != NULL && bot->getID() == killerID)
				{
					bot->incrementScore(numKills); 
				}
				
				int index = 12;
				
				// Read the data of each killed player
				for (int32_t i = 0; i < (int32_t)numKills; i++)
				{
					rawID = 0;
					rawID |= ((uint32_t)server->recvBuffer[index]) << 24; 		// byte 3 of ID
					rawID |= ((uint32_t)server->recvBuffer[index + 1]) << 16; 	// byte 2 of ID
					rawID |= ((uint32_t)server->recvBuffer[index + 2]) << 8;	// byte 1 of ID
					rawID |= ((uint32_t)server->recvBuffer[index + 3]);			// byte 0 of ID
					
					int32_t playerID = (int32_t)ntohl(rawID);
					
					// Inform the bot that the player is killed
					if (bot != NULL)
					{
						bot->playerKilledUpdate(playerID);
					}
					
					// If the killed player is this bot, and this bot is a punisher bot
					// set the killer bot as the target
					if (bot != NULL && bot->getID() == playerID)
					{	
						bot->setKiller(killerID);
					}
				
					fprintf(stdout, "Player %d blown to pieces!!!\n", playerID);
					
					// Move the index to the next 4 bytes block
					index += 4;
				}
				
				if (bot != NULL)
				{
					fprintf(stdout, "Current player score: %d\n", bot->getScore());
				}		
			}
			break;
		}
		default:
		{
			fprintf(stderr, "Wrong message code in player message\n");
			res = -1;
			break;
		}
	}
	
	return res;
}


int PlayerClient::sendPlayerSpawnMessage()
{	
	uint32_t numBytes = 18;
	uint32_t convertedBytes = htonl(numBytes);
	
	uint32_t binaryX;
	uint32_t binaryY;
	uint32_t binaryZ;
	
	float x = bot->getX();
	float y = bot->getY();
	float z = bot->getZ();
	
	// Copy the binary bits in the float coordinates into uint32_t variables
	// This must be done instead of casting because
	// casting the float values to uint32_t is equivalent to rounding
	memcpy(&binaryX, &x, sizeof(float));
	memcpy(&binaryY, &y, sizeof(float));
	memcpy(&binaryZ, &z, sizeof(float));

	uint32_t convertedX = htonl(binaryX);
	uint32_t convertedY = htonl(binaryY);
	uint32_t convertedZ = htonl(binaryZ);
	
	server->sendBuffer[0] = GET_BYTE_3(convertedBytes);
	server->sendBuffer[1] = GET_BYTE_2(convertedBytes);
	server->sendBuffer[2] = GET_BYTE_1(convertedBytes);
	server->sendBuffer[3] = GET_BYTE_0(convertedBytes);
	server->sendBuffer[4] = VERSION_NUM;
	server->sendBuffer[5] = PLAYER_SPAWN;
	server->sendBuffer[6] = GET_BYTE_3(convertedX);	// byte 3 of x
	server->sendBuffer[7] = GET_BYTE_2(convertedX);	// byte 2 of x
	server->sendBuffer[8] = GET_BYTE_1(convertedX);	// byte 1 of x
	server->sendBuffer[9] = GET_BYTE_0(convertedX);	// byte 0 of x
	server->sendBuffer[10] = GET_BYTE_3(convertedY);// byte 3 of y
	server->sendBuffer[11] = GET_BYTE_2(convertedY);// byte 2 of y
	server->sendBuffer[12] = GET_BYTE_1(convertedY);// byte 1 of y
	server->sendBuffer[13] = GET_BYTE_0(convertedY);// byte 0 of y
	server->sendBuffer[14] = GET_BYTE_3(convertedZ);// byte 3 of z
	server->sendBuffer[15] = GET_BYTE_2(convertedZ);// byte 2 of z
	server->sendBuffer[16] = GET_BYTE_1(convertedZ);// byte 1 of z
	server->sendBuffer[17] = GET_BYTE_0(convertedZ);// byte 0 of z
	
	ssize_t bytes = -1;
	
	if (FD_ISSET(server->sockfd, &writeSet))
	{
		bytes = send(server->sockfd, server->sendBuffer, (int)numBytes, 0);
	}
	
	// If an error occurred, retry 3 times
	int count = 3;
	while (bytes != numBytes && count > 0)
	{
		if (FD_ISSET(server->sockfd, &writeSet))
		{
			bytes = send(server->sockfd, server->sendBuffer, (int)numBytes, 0);
		}
		count--;
	}
	
	if (bytes == numBytes)
	{
		fprintf(stdout, "Player spawned at {%.2f, %.2f, %.2f}\n", x, y ,z);
		return 0;
	}
	
	fprintf(stderr, "Failed to send player spawn message\n");
	return -1;
	
	// More sophisticated error handling is needed in a real game
}


int PlayerClient::sendPlayerMoveMessage()
{	
	uint32_t numBytes = 18;
	uint32_t convertedBytes = htonl(numBytes);
	
	uint32_t binaryX;
	uint32_t binaryY;
	uint32_t binaryZ;
			
	float x = bot->getX();
	float y = bot->getY();
	float z = bot->getZ();
	
	// Copy the binary bits in the float coordinates into uint32_t variables
	// This must be done instead of casting because
	// casting the float values to uint32_t is equivalent to rounding
	memcpy(&binaryX, &x, sizeof(float));
	memcpy(&binaryY, &y, sizeof(float));
	memcpy(&binaryZ, &z, sizeof(float));

	uint32_t convertedX = htonl(binaryX);
	uint32_t convertedY = htonl(binaryY);
	uint32_t convertedZ = htonl(binaryZ);
	
	server->sendBuffer[0] = GET_BYTE_3(convertedBytes);
	server->sendBuffer[1] = GET_BYTE_2(convertedBytes);
	server->sendBuffer[2] = GET_BYTE_1(convertedBytes);
	server->sendBuffer[3] = GET_BYTE_0(convertedBytes);
	server->sendBuffer[4] = VERSION_NUM;
	server->sendBuffer[5] = PLAYER_MOVE;
	server->sendBuffer[6] = GET_BYTE_3(convertedX);	// byte 3 of x
	server->sendBuffer[7] = GET_BYTE_2(convertedX);	// byte 2 of x
	server->sendBuffer[8] = GET_BYTE_1(convertedX);	// byte 1 of x
	server->sendBuffer[9] = GET_BYTE_0(convertedX);	// byte 0 of x
	server->sendBuffer[10] = GET_BYTE_3(convertedY);// byte 3 of y
	server->sendBuffer[11] = GET_BYTE_2(convertedY);// byte 2 of y
	server->sendBuffer[12] = GET_BYTE_1(convertedY);// byte 1 of y
	server->sendBuffer[13] = GET_BYTE_0(convertedY);// byte 0 of y
	server->sendBuffer[14] = GET_BYTE_3(convertedZ);// byte 3 of z
	server->sendBuffer[15] = GET_BYTE_2(convertedZ);// byte 2 of z
	server->sendBuffer[16] = GET_BYTE_1(convertedZ);// byte 1 of z
	server->sendBuffer[17] = GET_BYTE_0(convertedZ);// byte 0 of z
	
	ssize_t bytes = -1;
	
	if (FD_ISSET(server->sockfd, &writeSet))
	{
		bytes = send(server->sockfd, server->sendBuffer, (int)numBytes, 0);
	}
	
	// If an error occurred, retry 3 times
	int count = 3;
	while (bytes != numBytes && count > 0)
	{
		if (FD_ISSET(server->sockfd, &writeSet))
		{
			bytes = send(server->sockfd, server->sendBuffer, (int)numBytes, 0);
		}
		count--;
	}
	
	if (bytes == numBytes)
	{
		fprintf(stdout, "Player moved to {%.2f, %.2f, %.2f}\n", x, y ,z);
		return 0;
	}
	
	fprintf(stderr, "Failed to send player move message\n");
	return -1;
	
	// More sophisticated error handling is needed in a real game
}


int PlayerClient::sendPlayerSelfAnnihilateMessage()
{
	uint32_t numBytes = 6;
	uint32_t convertedBytes = htonl(numBytes);
	
	server->sendBuffer[0] = GET_BYTE_3(convertedBytes);
	server->sendBuffer[1] = GET_BYTE_2(convertedBytes);
	server->sendBuffer[2] = GET_BYTE_1(convertedBytes);
	server->sendBuffer[3] = GET_BYTE_0(convertedBytes);
	server->sendBuffer[4] = VERSION_NUM;
	server->sendBuffer[5] = PLAYER_SELF_ANNIHILATE;
	
	ssize_t bytes = -1;
	
	if (FD_ISSET(server->sockfd, &writeSet))
	{
		bytes = send(server->sockfd, server->sendBuffer, (int)numBytes, 0);
	}
	
	// If an error occurred, retry 3 times
	int count = 3;
	while (bytes != numBytes && count > 0)
	{
		if (FD_ISSET(server->sockfd, &writeSet))
		{
			bytes = send(server->sockfd, server->sendBuffer, (int)numBytes, 0);
		}
		count--;
	}
	
	if (bytes == numBytes)
	{
		fprintf(stdout, "Player self-annihilated at {%.2f, %.2f, %.2f}!!!\n", bot->getX(), bot->getY(), bot->getZ());
		return 0;
	}
	
	fprintf(stderr, "Failed to send player self-annihilate message. Num bytes sent: %ld\n", bytes);
	return -1;
	
	// More sophisticated error handling is needed in a real game
}
