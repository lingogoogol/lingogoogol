#include "Item.h"
#include <iostream>

Item::Item() : level_required(1), name("Unnamed"), effects("None"),
description("No description"), weight(1), type('w') {}

Item::Item(int level_required, const string& name, const string& effects,
	const string& description, int weight, char type)
	: level_required(level_required), name(name), effects(effects),
	description(description), weight(weight), type(type) {}

Item::~Item() {
	// 可選擇輸出資訊以除錯
	// cout << "Item [" << name << "] destroyed" << endl;
}

void Item::print() const {
	cout << "== " << name << " ==" << endl;
	cout << "等級限制：" << level_required << endl;
	cout << "效果：" << effects << endl;
	cout << "說明：" << description << endl;
	cout << "重量：" << weight << endl;
	cout << "類型：" << type << endl;
}
