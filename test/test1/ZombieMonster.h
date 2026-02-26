#ifndef ZombieMonster_H
#define ZombieMonster_H
#include"BaseMonster.h"

class ZombieMonster :public BaseMonster
{
public:


	ZombieMonster();
	 string serialize() const;
	 static BaseMonster*unserialize(string );

};
#endif