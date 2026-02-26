#include "ConsumableItem.h"
#include <iostream>

ConsumableItem::ConsumableItem()
	: Item(1, "Unnamed Consumable", "None", "No description", 1, 'c') {}

ConsumableItem::ConsumableItem(int level_required, const string& name, const string& effects,
	const string& description, int weight)
	: Item(level_required, name, effects, description, weight, 'c') {}

void ConsumableItem::print() const {
	cout << "== 消耗品：" << name << " ==" << endl;
	cout << "等級限制：" << level_required << endl;
	cout << "效果：" << effects << endl;
	cout << "說明：" << description << endl;
	cout << "重量：" << weight << endl;
}
