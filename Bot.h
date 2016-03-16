#ifndef BOT_H
#define BOT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>
#include <ctime>
#include <time.h>


// Bot action
#define MOVE				12
#define EXPLODE				13
#define SPAWN				14
#define STANDBY				15

#define EXPLOSION_RADIUS 	0.4
#define BOT_STEP			0.1
#define ACTION_COOLDOWN		1

using namespace std;


typedef struct
{
	bool isCreated;
	float x, y, z;
	bool isAlive;
	int score;
	
} Player;

class Bot
{
	private:
		
		
	
	public:
	
		int botID;
		int killerID; // ID of the most recent killer of the bot
		Player* players; // array to store info about players in the arena (including the bot itself)
		int numPlayers;
		float lastActionTime; // the last time the bot took some action (MOVE, EXPLODE, SPAWN)
	

		Bot(int numPlayers, int ID);	
		
		~Bot();
		
		// Tell the bot to perform the next action
		// Return the code of the action performed by the bot
		virtual int performAction() = 0;
		
		// Update the bot of a player spawn
		void playerSpawnUpdate(int playerID, float x, float y, float z);
		
		// Update the bot of a player killed
		void playerKilledUpdate(int playerID);
		
		// Update the bot of the location of a player
		void playerLocationUpdat(int playerID, float x, float y, float z);
		
		// Increment the score of the bot
		void incrementScore(int score); 
		
		void setKiller(int playerID);
		
		// Get the x coordinate of the bot
		float getX();
		
		// Get the Y coordinate of the bot
		float getY();
		
		// Get the Z coordinate of the bot
		float getZ();
		
		int getScore();
		
		int getID();
		
		bool isAlive();
		
		bool isCreated();
		
		// Determine of action cool down is complete
		bool coolDownDone();
		
		float getDistance(int32_t playerID1, int32_t playerID2);
};

#endif
