#ifndef BOT_FACTORY_H
#define BOT_FACTORY_H

#include "Bot.h"
#include "DumbBot.h"
#include "PunisherBot.h"

//Bot AI type
#define DUMB_BOT		10
#define PUNISHER_BOT	11

class BotFactory
{
	public:
		
		static Bot* createBot(int botAIType, int numPlayers, int ID);
};

#endif
