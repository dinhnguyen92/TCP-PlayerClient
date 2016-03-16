#include "BotFactory.h"

Bot* BotFactory::createBot(int botAIType, int numPlayers, int ID)
{
	Bot* bot;
	switch(botAIType)
	{
		case DUMB_BOT:
		
			bot = new DumbBot(numPlayers, ID);
			break;
		
		case PUNISHER_BOT:
		
			bot = new PunisherBot(numPlayers, ID);
			break;
		
		default:
		
			bot = new DumbBot(numPlayers, ID);
			break;
	}
	
	return bot;
}
