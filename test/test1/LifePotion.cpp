#include "LifePotion.h"
#include "NovicePlayer.h"  // 假設這個提供 getHP / setHP 等方法

LifePotion::LifePotion() {
	name = "Life Potion";
	level_required = 0;
	weight = 1;
	type = 'c';
	effects = "Restore 50 HP";
	description = "A glowing red potion that heals 50 HP.";
}

void LifePotion::use(NovicePlayer* player) {
	if (player) {
		int newHP = player->getHp() + 50;
		player->setHp(newHP);
	}
}
