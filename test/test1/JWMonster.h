#ifndef JWMonster_H
#define JWMonster_H
#include"BaseMonster.h"

class JWMonster :public BaseMonster
{
public:


	JWMonster();
	 string serialize()const;
	 static BaseMonster*unserialize(string );

};
#endif