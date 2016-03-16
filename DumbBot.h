#ifndef DUMB_BOT_H
#define DUMB_BOT_H

#include "Bot.h"

class DumbBot: public Bot
{
	private:
	
		
	
	public:

		// Create a dumb bot
		// Inform the bot of the maximum number of players in the arena (including itself)
		// Assign an ID to the bot
		DumbBot(int numPlayers, int ID);
		
		int performAction() override;
};

#endif
