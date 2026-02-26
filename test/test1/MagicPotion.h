#ifndef MAGIC_POTION_H
#define MAGIC_POTION_H

#include "ConsumableItem.h"

class MagicPotion : public ConsumableItem {
public:
	MagicPotion();
	void use(NovicePlayer* player) override;
};

#endif
