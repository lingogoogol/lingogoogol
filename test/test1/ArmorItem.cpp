#include "ArmorItem.h"
#include <iostream>

ArmorItem::ArmorItem()
	: Item(1, "Unnamed Armor", "Defense +0", "No description", 1, 'a'),
	defense_increment(0) {}

ArmorItem::ArmorItem(int level_required, const string& name, const string& effects,
	const string& description, int weight, int defense_increment)
	: Item(level_required, name, effects, description, weight, 'a'),
	defense_increment(defense_increment) {}

void ArmorItem::print() const {
	cout << "== 護甲：" << name << " ==" << endl;
	cout << "等級限制：" << level_required << endl;
	cout << "防禦加成：" << defense_increment << endl;
	cout << "效果：" << effects << endl;
	cout << "說明：" << description << endl;
	cout << "重量：" << weight << endl;
}
