#include "DumbBot.h"


DumbBot::DumbBot(int numPlayers, int ID) : Bot(numPlayers, ID)
{
	fprintf(stdout, "Dumb bot created\n");
}


int DumbBot::performAction()
{
	// Do nothing if the player has not been created
	if (!players[botID].isCreated) return STANDBY;
	
	// If cooldown is not finished
	if (!coolDownDone()) return STANDBY;
	
	// If the player is not alive
	if (!players[botID].isAlive)
	{
		// Generate a random spawn location
		srand(time(NULL));
		
		players[botID].x = (rand() % 10)/(float)10;
		players[botID].y = (rand() % 10)/(float)10;
		players[botID].z = (rand() % 10)/(float)10;
		
		players[botID].isAlive = true;
		
		// reset the last action time;
		lastActionTime = clock();
		
		return SPAWN;
	}
	
	// Check if there's any alive player within explosion range
	for (int i = 0; i < numPlayers; i++)
	{
		// If a live player that is not this bot is within explosion range
		if (players[i].isAlive && i != botID && getDistance(botID, i) <= EXPLOSION_RADIUS)
		{
			fprintf(stdout, "Targeting player %d at {%.2f, %.2f, %.2f}\n", i, players[i].x, players[i].y, players[i].z);
			fprintf(stdout, "Distance to target: %.2f\n", getDistance(botID, i));
			
			// Self-annihilate
			players[botID].isAlive = false;
			lastActionTime = clock();
			return EXPLODE;
		}
	}
	
	// If no player is in range, move in 1 out of 6 directions
	// 0: x positive
	// 1: x negative
	// 2: y positive
	// 3: y negative
	// 4: z positive
	// 5: z negative
	
	// Generate a random direction
	srand(time(NULL));
	int dir = rand() % 6;
	
	switch(dir)
	{
		case 0:
		{
			// if moving in the +x direction causes the bot to be out of range
			if (players[botID].x + BOT_STEP > 1)
			{
				players[botID].x = 1;
			}
			else
			{
				players[botID].x += BOT_STEP;
			}
			break;
		}
		case 1:
		{
			// if moving in the -x direction causes the bot to be out of range
			if (players[botID].x - BOT_STEP < 0)
			{
				players[botID].x = 0;
			}
			else
			{
				players[botID].x -= BOT_STEP;
			}
			break;
		}
		case 2:
		{
			// if moving in the +y direction causes the bot to be out of range
			if (players[botID].y + BOT_STEP > 1)
			{
				players[botID].y = 1;
			}
			else
			{
				players[botID].y += BOT_STEP;
			}
			break;
		}
		case 3:
		{
			// if moving in the -y direction causes the bot to be out of range
			if (players[botID].y - BOT_STEP < 0)
			{
				players[botID].y = 0;
			}
			else
			{
				players[botID].y -= BOT_STEP;
			}
			break;
		}
		case 4:
		{
			// if moving in the +z direction causes the bot to be out of range
			if (players[botID].z + BOT_STEP > 1)
			{
				players[botID].z = 1;
			}
			else
			{
				players[botID].z += BOT_STEP;
			}
			break;
		}
		case 5:
		{
			// if moving in the -z direction causes the bot to be out of range
			if (players[botID].z - BOT_STEP < 0)
			{
				players[botID].z = 0;
			}
			else
			{
				players[botID].z -= BOT_STEP;
			}
			break;
		}
	}
	
	// reset the cooldown time
	lastActionTime = clock();
	
	return MOVE;
}
