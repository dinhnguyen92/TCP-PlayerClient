all: client

objects = main.o PlayerClient.o Bot.o DumbBot.o BotFactory.o PunisherBot.o

client: $(objects)
	g++ -std=c++11 -g -Wall -o client $(objects)

main.o: main.cpp
	g++ -std=c++11 -g -Wall -c main.cpp

PlayerClient.o: PlayerClient.cpp
	g++ -std=c++11 -g -Wall -c PlayerClient.cpp

Bot.o: Bot.cpp
	g++ -std=c++11 -g -Wall -c Bot.cpp

DumbBot.o: DumbBot.cpp
	g++ -std=c++11 -g -Wall -c DumbBot.cpp
	
PunisherBot.o: PunisherBot.cpp
	g++ -std=c++11 -g -Wall -c PunisherBot.cpp
	
BotFactory.o: BotFactory.cpp
	g++ -std=c++11 -g -Wall -c BotFactory.cpp

.Phony: clean
clean:
	rm $(objects)

