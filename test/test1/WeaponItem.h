#ifndef WEAPONITEM_H
#define WEAPONITEM_H

#include "Item.h"

class WeaponItem : public Item {
public:
	int attack_increment;  // 攻擊加成

	WeaponItem(); // 預設建構子
	WeaponItem(int level_required, const string& name, const string& effects,
		const string& description, int weight, int attack_increment);

	virtual void print() const override;
};

#endif
