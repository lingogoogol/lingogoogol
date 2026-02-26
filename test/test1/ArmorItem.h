#ifndef ARMORITEM_H
#define ARMORITEM_H

#include "Item.h"

class ArmorItem : public Item {
public:
	int defense_increment;  // 防禦加成

	ArmorItem(); // 預設建構子
	ArmorItem(int level_required, const string& name, const string& effects,
		const string& description, int weight, int defense_increment);

	virtual void print() const override;
};

#endif
