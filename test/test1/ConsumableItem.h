#ifndef CONSUMABLEITEM_H
#define CONSUMABLEITEM_H

#include "Item.h"

// 前向宣告，避免包含整個 NovicePlayer
class NovicePlayer;

class ConsumableItem : public Item {
public:
	ConsumableItem();
	ConsumableItem(int level_required, const string& name, const string& effects,
		const string& description, int weight);

	virtual void use(NovicePlayer* player) = 0; // 純虛擬函式
	virtual void print() const override;
};

#endif
