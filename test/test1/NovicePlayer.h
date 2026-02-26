#ifndef NOVICEPLAYER_H
#define NOVICEPLAYER_H

#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

// 前向宣告，減少編譯依賴
class Item;
class WeaponItem;
class ArmorItem;
class ConsumableItem;

class NovicePlayer {
private:
	string name;
	int hp;
	int mp;
	int exp;

protected:
	int level;
	int attack;
	int defense;
	int maxhp;
	int maxmp;
	int lvupexp;
	int money;

	// === 新增：裝備與背包 ===
	Item** backpack;
	int backpack_weight;
	int backpack_weight_limit;
	int backpack_slot_limit;

	WeaponItem* weapon;
	ArmorItem* armor;

public:
	// === 建構與反序列化 ===
	void saveBackpack(std::ostream& os) const;
	void loadBackpack(std::istream& is);
	virtual std::string getJobName() const { return "Novice"; }
	void saveEquipment(std::ostream& os) const;
	void loadEquipment(std::istream& is);

	NovicePlayer();
	NovicePlayer(int);
	NovicePlayer(int, string);
	NovicePlayer(const NovicePlayer&);
	NovicePlayer(string, int, int, int, int, int, int, int, int, int, int);
	~NovicePlayer();
	virtual void specialSkill();
	virtual string serialize() const;
	static NovicePlayer* unserialize(const string&);

	// === 基本存取方法 ===
	void setName(string);
	string getName() const;

	virtual void setLevel(int);
	int getLevel() const;

	void setHp(int);
	int getHp() const;

	void setMp(int);
	int getMp() const;

	void setExp(int);
	int getExp() const;

	void setMoney(int);
	int getMoney() const;

	int getAttack() const;
	int getDefense() const;
	int getMaxHP() const;
	int getMaxMP() const;
	int getLvupExp() const;

	// === 背包與裝備操作 ===
	bool putItem(Item* item);
	Item* takeItem(int index);
	bool equipWeapon(WeaponItem* newWeapon);
	bool equipArmor(ArmorItem* newArmor);
	void useConsumable(ConsumableItem* item);
	void viewBackpack() const;

	// === 顯示資訊 ===
	void print() {
		cout << "-------------------------" << endl;
		cout << setw(10) << "name:" << getName() << endl;
		cout << setw(10) << "hp:" << getHp() << endl;
		cout << setw(10) << "mp:" << getMp() << endl;
		cout << setw(10) << "exp:" << getExp() << endl;
		cout << setw(10) << "money:" << getMoney() << endl;
		cout << setw(10) << "level:" << getLevel() << endl;
		cout << setw(10) << "attack:" << getAttack() << endl;
		cout << setw(10) << "defense:" << getDefense() << endl;
		cout << setw(10) << "maxhp:" << getMaxHP() << endl;
		cout << setw(10) << "maxmp:" << getMaxMP() << endl;
		cout << setw(10) << "lvup_exp:" << getLvupExp() << endl;
		cout << "-------------------------" << endl;
	}

	// === 初始化背包屬性 ===
	void setAttr();  // 設定背包容量、限制等
};

#endif
