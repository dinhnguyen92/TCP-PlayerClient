#ifndef PUNISHER_BOT_H
#define PUNISHER_BOT_H

#include "Bot.h"

class PunisherBot: public Bot
{
	private:
	
	public:

		// Create a punisher bot
		// Inform the bot of the maximum number of players in the arena (including itself)
		// Assign an ID to the bot
		PunisherBot(int numPlayers, int ID);
		
		int performAction() override;
};

#endif
