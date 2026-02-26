#ifndef ITEM_H
#define ITEM_H

#include <string>
using namespace std;

class Item {
public:
	int level_required;  // 使用/裝備等級限制
	string name;         // 道具名稱
	string effects;      // 效果敘述
	string description;  // 描述文字
	int weight;          // 重量
	char type;           // 類型：'w' 武器, 'a' 護甲, 'c' 消耗品

	Item(); // 預設建構子
	Item(int level_required, const string& name, const string& effects,
		const string& description, int weight, char type);
	virtual std::string getName() const { return name; } // ✅ 加這行

	virtual ~Item(); // 虛擬解構子，保證多型刪除安全

	virtual void print() const; // 印出道具資訊
};

#endif
