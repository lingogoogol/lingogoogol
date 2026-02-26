#include "NovicePlayer.h"
#include "MagicianPlayer.h"
#include "OrcPlayer.h"
#include "KnightPlayer.h"
#include <cmath>
#include <iostream>
#include <sstream>
#include "LifePotion.h"
#include "MagicPotion.h"
#include "SwordWeapon.h"
#include "AxeWeapon.h"
#include "ShieldArmor.h"
#include "TunicArmor.h"
using namespace std;

#include "Item.h"
#include "WeaponItem.h"
#include "ArmorItem.h"
#include "ConsumableItem.h"

// === 預設建構子 ===
NovicePlayer::NovicePlayer() {
	setLevel(1);
	setName("anonymous");
	setAttr();
}

NovicePlayer::NovicePlayer(int a) {
	setLevel(a);
	setName("anonymous");
	setAttr();
}

NovicePlayer::NovicePlayer(int a, string b) {
	setLevel(a);
	setName(b);
	setAttr();
}

// ⚠️ 修正版：深複製建構子
NovicePlayer::NovicePlayer(const NovicePlayer& other) {
	name = other.name;
	level = other.level;
	hp = other.hp;
	mp = other.mp;
	exp = other.exp;
	money = other.money;
	attack = other.attack;
	defense = other.defense;
	maxhp = other.maxhp;
	maxmp = other.maxmp;
	lvupexp = other.lvupexp;

	weapon = other.weapon;
	armor = other.armor;

	backpack_slot_limit = other.backpack_slot_limit;
	backpack_weight_limit = other.backpack_weight_limit;
	backpack_weight = other.backpack_weight;

	backpack = new Item*[backpack_slot_limit];
	for (int i = 0; i < backpack_slot_limit; ++i) {
		backpack[i] = other.backpack[i];  // 可進一步改為 clone()
	}
}

// 完整建構子
NovicePlayer::NovicePlayer(string A, int B, int C, int D, int E, int F, int G, int H, int I, int J, int K) {
	maxhp = I;
	maxmp = J;
	lvupexp = K;
	setName(A);
	setHp(B);
	setMp(C);
	setExp(D);
	setMoney(E);
	setLevel(F);
	attack = G;
	defense = H;
	setAttr();
}

// 解構子
NovicePlayer::~NovicePlayer() {
	if (backpack) {
		for (int i = 0; i < backpack_slot_limit; ++i) {
			delete backpack[i];
		}
		delete[] backpack;
	}
}

// 初始化背包與裝備欄位
void NovicePlayer::setAttr() {
	backpack_slot_limit = 10;
	backpack_weight_limit = 100;
	backpack_weight = 0;

	weapon = nullptr;
	armor = nullptr;

	backpack = new Item*[backpack_slot_limit];
	for (int i = 0; i < backpack_slot_limit; ++i) {
		backpack[i] = nullptr;
	}
}

// Getter / Setter
void NovicePlayer::setName(string a) { name = a; }
string NovicePlayer::getName() const { return name; }

void NovicePlayer::setLevel(int a) {
	level = a;
	attack = (20 + 5 * getLevel());
	defense = (20 + 5 * getLevel());
	maxhp = (100 + 10 * getLevel());
	maxmp = (40 + 5 * getLevel());
	mp = maxmp;
	hp = maxhp;
	exp = 0;
	money = 0;
	lvupexp = ceil(pow(10, log2(getLevel() + 1)));
}
int NovicePlayer::getLevel() const { return level; }

void NovicePlayer::setHp(int a) {
	if (a > getMaxHP()) hp = maxhp;
	else if (a < 0) hp = 0;
	else hp = a;
}
int NovicePlayer::getHp() const { return hp; }

void NovicePlayer::setMp(int a) {
	if (a > getMaxMP()) mp = maxmp;
	else if (a < 0) mp = 0;
	else mp = a;
}
int NovicePlayer::getMp() const { return mp; }

void NovicePlayer::setExp(int a) {
	if (a > getLvupExp()) exp = getLvupExp();
	else if (a < 0) exp = 0;
	else exp = a;
}
int NovicePlayer::getExp() const { return exp; }

void NovicePlayer::setMoney(int a) { money = a; }
int NovicePlayer::getMoney() const { return money; }

int NovicePlayer::getAttack() const { return attack; }
int NovicePlayer::getDefense() const { return defense; }
int NovicePlayer::getMaxHP() const { return maxhp; }
int NovicePlayer::getMaxMP() const { return maxmp; }
int NovicePlayer::getLvupExp() const { return lvupexp; }

void NovicePlayer::specialSkill() {
	// 預留給子類覆寫
}

string NovicePlayer::serialize() const {
	return getName() + " " + to_string(getHp()) + " " + to_string(getMp()) + " " + to_string(getExp()) + " " +
		to_string(getMoney()) + " " + to_string(getLevel()) + " " + to_string(getAttack()) + " " +
		to_string(getDefense()) + " " + to_string(getMaxHP()) + " " + to_string(getMaxMP()) + " " +
		to_string(getLvupExp());
}

NovicePlayer* NovicePlayer::unserialize(const string& a) {
	istringstream iss(a);
	string AA;
	int BB, CC, DD, EE, FF, GG, HH, II, JJ, KK;
	iss >> AA >> BB >> CC >> DD >> EE >> FF >> GG >> HH >> II >> JJ >> KK;
	return new KnightPlayer(AA, BB, CC, DD, EE, FF, GG, HH, II, JJ, KK);
}

// 放入背包
bool NovicePlayer::putItem(Item* item) {
	if (!item) return false;

	int newWeight = backpack_weight + item->weight;
	if (newWeight > backpack_weight_limit) {
		cout << "背包太重，無法放入：" << item->name << endl;
		return false;
	}

	for (int i = 0; i < backpack_slot_limit; ++i) {
		if (backpack[i] == nullptr) {
			backpack[i] = item;
			backpack_weight = newWeight;
			cout << "放入背包：" << item->name << endl;
			return true;
		}
	}

	cout << "背包已滿，無法放入：" << item->name << endl;
	return false;
}

// 從背包取出
Item* NovicePlayer::takeItem(int index) {
	if (index < 0 || index >= backpack_slot_limit) {
		cout << "索引錯誤！" << endl;
		return nullptr;
	}

	if (backpack[index] == nullptr) {
		cout << "該格子沒有物品！" << endl;
		return nullptr;
	}

	Item* item = backpack[index];
	backpack_weight -= item->weight;
	backpack[index] = nullptr;
	return item;
}

// 裝備武器
bool NovicePlayer::equipWeapon(WeaponItem* newWeapon) {
	if (!newWeapon) return false;
	if (level < newWeapon->level_required) {
		cout << "等級不足，無法裝備此武器！" << endl;
		return false;
	}

	if (weapon != nullptr) {
		attack -= weapon->attack_increment;
		if (!putItem(weapon)) {
			cout << "裝備失敗：舊武器無法放回背包！" << endl;
			return false;
		}
	}

	weapon = newWeapon;
	attack += newWeapon->attack_increment;
	cout << "成功裝備武器：" << newWeapon->name << endl;
	return true;
}

// 裝備護甲
bool NovicePlayer::equipArmor(ArmorItem* newArmor) {
	if (!newArmor) return false;
	if (level < newArmor->level_required) {
		cout << "等級不足，無法裝備此護甲！" << endl;
		return false;
	}

	if (armor != nullptr) {
		defense -= armor->defense_increment;
		if (!putItem(armor)) {
			cout << "裝備失敗：舊護甲無法放回背包！" << endl;
			return false;
		}
	}

	armor = newArmor;
	defense += newArmor->defense_increment;
	cout << "成功裝備護甲：" << newArmor->name << endl;
	return true;
}

// 使用消耗品
void NovicePlayer::useConsumable(ConsumableItem* item) {
	if (!item) return;
	if (level < item->level_required) {
		cout << "等級不足，無法使用此消耗品！" << endl;
		return;
	}

	item->use(this);
	cout << "使用道具：" << item->name << endl;

	for (int i = 0; i < backpack_slot_limit; ++i) {
		if (backpack[i] == item) {
			backpack_weight -= item->weight;
			delete backpack[i];
			backpack[i] = nullptr;
			break;
		}
	}
}

// 顯示背包內容
void NovicePlayer::viewBackpack() const {
	cout << "=== 背包內容 ===" << endl;
	for (int i = 0; i < backpack_slot_limit; ++i) {
		if (backpack[i] != nullptr) {
			cout << "[" << i << "] " << backpack[i]->name
				<< " (重量: " << backpack[i]->weight
				<< ", 等級需求: " << backpack[i]->level_required << ")" << endl;
		}
	}
	cout << "目前重量：" << backpack_weight << " / " << backpack_weight_limit << endl;
}
void NovicePlayer::saveBackpack(std::ostream& os) const {
	os << backpack_slot_limit << std::endl;
	for (int i = 0; i < backpack_slot_limit; ++i) {
		if (backpack[i] != nullptr)
			os << backpack[i]->getName() << std::endl;
		else
			os << "EMPTY" << std::endl;
	}
}

void NovicePlayer::loadBackpack(std::istream& is) {
	int size;
	is >> size;
	is.ignore(); // 忽略換行

	for (int i = 0; i < size; ++i) {
		std::string itemName;
		std::getline(is, itemName);

		if (backpack[i]) delete backpack[i]; // 清空舊的

		if (itemName == "EMPTY") {
			backpack[i] = nullptr;
			continue;
		}

		Item* item = nullptr;

		if (itemName == "LifePotion") item = new LifePotion();
		else if (itemName == "MagicPotion") item = new MagicPotion();
		else if (itemName == "SwordWeapon") item = new SwordWeapon();
		else if (itemName == "AxeWeapon") item = new AxeWeapon();
		else if (itemName == "ShieldArmor") item = new ShieldArmor();
		else if (itemName == "TunicArmor") item = new TunicArmor();
		else {
			std::cerr << "[警告] 無法辨識的道具名稱：" << itemName << std::endl;
		}

		backpack[i] = item;
	}
}
void NovicePlayer::saveEquipment(std::ostream& os) const {
	os << (weapon ? weapon->getName() : "EMPTY") << std::endl;
	os << (armor ? armor->getName() : "EMPTY") << std::endl;
}

void NovicePlayer::loadEquipment(std::istream& is) {
	std::string weaponName, armorName;
	std::getline(is >> std::ws, weaponName);
	std::getline(is >> std::ws, armorName);

	if (weapon) delete weapon;
	if (armor) delete armor;
	weapon = nullptr;
	armor = nullptr;

	if (weaponName == "SwordWeapon") weapon = new SwordWeapon();
	else if (weaponName == "AxeWeapon") weapon = new AxeWeapon();

	if (armorName == "ShieldArmor") armor = new ShieldArmor();
	else if (armorName == "TunicArmor") armor = new TunicArmor();

	if (weapon) attack += weapon->attack_increment;
	if (armor) defense += armor->defense_increment;
}
