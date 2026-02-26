#include "MagicPotion.h"
#include "NovicePlayer.h"

MagicPotion::MagicPotion() {
	name = "Magic Potion";
	level_required = 0;
	weight = 1;
	type = 'c';
	effects = "Restore 30 MP";
	description = "A sparkling blue potion that restores 30 magic points.";
}

void MagicPotion::use(NovicePlayer* player) {
	if (player) {
		int newMP = player->getMp() + 30;
		player->setMp(newMP);
	}
}
