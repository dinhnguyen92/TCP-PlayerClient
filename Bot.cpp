#include "Bot.h"

Bot::Bot(int num, int ID)
{
	botID = ID;
	numPlayers = num;
	players = new Player[numPlayers];
	
	// Initialize all the bots
	for (int i = 0; i < numPlayers; i++)
	{
		players[i].isCreated = false;
		players[i].isAlive = false;
	}
	
	// Set this bot as created
	players[botID].isCreated = true;
	players[botID].score = 0;
	
	// Set the last action time as negative to indicate that player has not taken any action
	lastActionTime = -1;
	
	// No killer of this bot yet
	killerID = -1;
}


Bot::~Bot()
{
	if (players != NULL) delete[] players;
}


float Bot::getDistance(int32_t playerID1, int32_t playerID2)
{
	float x = abs(players[playerID1].x - players[playerID2].x);
	float y = abs(players[playerID1].y - players[playerID2].y);
	float z = abs(players[playerID1].z - players[playerID2].z);
	
	return sqrt(x * x + y * y + z * z);
}


bool Bot::coolDownDone()
{	
	if (lastActionTime < 0) return true; // if player has not taken any action
	
	float sec = (clock() - lastActionTime)/(double)CLOCKS_PER_SEC;
	
	return (sec >= ACTION_COOLDOWN);
}


void Bot::playerSpawnUpdate(int playerID, float x, float y, float z)
{
	players[playerID].isCreated = true;
	players[playerID].isAlive = true;
	players[playerID].x = x;
	players[playerID].y = y;
	players[playerID].z = z;
}


void Bot::playerKilledUpdate(int playerID)
{
	players[playerID].isAlive = false;

	// if the player killed is this bot, reset the last action time
	lastActionTime = clock();
}


void Bot::playerLocationUpdat(int playerID, float x, float y, float z)
{
	players[playerID].isAlive = true;
	players[playerID].x = x;
	players[playerID].y = y;
	players[playerID].z = z;
}


void Bot::setKiller(int playerID)
{
	killerID = playerID;
}


void Bot::incrementScore(int score)
{
	players[botID].score += score;
}


float Bot::getX()
{
	return players[botID].x;
}


float Bot::getY()
{
	return players[botID].y;
}


float Bot::getZ()
{
	return players[botID].z;
}


int Bot::getScore()
{
	return players[botID].score;
}


int Bot::getID()
{
	return botID;
}


bool Bot::isAlive()
{
	return players[botID].isAlive;
}


bool Bot::isCreated()
{
	return players[botID].isCreated;
}
