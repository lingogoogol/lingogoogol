#include "WeaponItem.h"
#include <iostream>

WeaponItem::WeaponItem()
	: Item(1, "Unnamed Weapon", "Attack +0", "No description", 1, 'w'),
	attack_increment(0) {}

WeaponItem::WeaponItem(int level_required, const string& name, const string& effects,
	const string& description, int weight, int attack_increment)
	: Item(level_required, name, effects, description, weight, 'w'),
	attack_increment(attack_increment) {}

void WeaponItem::print() const {
	cout << "== 武器：" << name << " ==" << endl;
	cout << "等級限制：" << level_required << endl;
	cout << "攻擊加成：" << attack_increment << endl;
	cout << "效果：" << effects << endl;
	cout << "說明：" << description << endl;
	cout << "重量：" << weight << endl;
}
