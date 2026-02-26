#pragma once
#include "NovicePlayer.h"
#include "KnightPlayer.h"
#include "MagicianPlayer.h"
#include "Field.h"
#include "Battle.h"
#include <vector>


class Game {
private:
	std::vector<NovicePlayer*> players;
	std::vector<BaseMonster*> monsters;
	Field* field;
	Battle* battle;
	int** map;
	int mapWidth = 1000;
	int mapHeight = 1000;
	
public:
	Field* fields[3];
	int currentFieldIndex = 0;
	int storylineProgress = 0;
	Game();
	~Game();
	void run();
	void setupPlayers();
	void setupMonsters();
	void setupMap();
	void gameLoop();
	void clearScreen();
	void save(const std::string& filename);
	void load(const std::string& filename);
	vector<Event*> events;
};
