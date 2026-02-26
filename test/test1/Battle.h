// Battle.h
#ifndef BATTLE_H
#define BATTLE_H

#include "NovicePlayer.h"
#include "BaseMonster.h"
struct Character {
	char type;      // 'p' for player, 'm' for monster
	bool alive;     // true if alive
	void* instance; // pointer to NovicePlayer or BaseMonster
};
class Battle {
private:
	int numPlayers;
	int numMonsters;
	int numTurns;
	Character* ActionList;
	Battle* battle;
public:
	Battle(int nPlayers, int nMonsters, NovicePlayer* players[], BaseMonster* monsters[]);
	~Battle();

	void start();
	void showStatus();
	bool isGameOver();
	Battle();  // 預設建構子
	void fight(NovicePlayer* player, BaseMonster* monster);  // fight函數
	int getRandomAliveIndex(char targetType);

};





#endif
