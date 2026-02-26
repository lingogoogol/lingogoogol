#ifndef LIFE_POTION_H
#define LIFE_POTION_H

#include "ConsumableItem.h"

class LifePotion : public ConsumableItem {
public:
	LifePotion();
	void use(NovicePlayer* player) override;
};

#endif
