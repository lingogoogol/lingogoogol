#ifndef GOBLIN_H
#define GOBLIN_H
#include"BaseMonster.h"

class GoblinMonster:public BaseMonster
{
public:
	

	GoblinMonster() ;
	 string serialize()const;
	static BaseMonster*unserialize(string );
	
	Item* dropItem();
	vector<Item*> dropItems();              // ✅ 新增這個！

};

#endif