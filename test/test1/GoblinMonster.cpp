#include"GoblinMonster.h"
#include "SwordWeapon.h"
#include "GoblinMonster.h"
#include "AxeWeapon.h"
#include "ShieldArmor.h"
#include "TunicArmor.h"
#include "LifePotion.h"
#include "MagicPotion.h"
GoblinMonster::GoblinMonster() :BaseMonster("Goblin", 60, 40, 12, 30, 100, 50,50,100){
		


}
 string GoblinMonster:: serialize() const {


	return name + " " + to_string(attack) + " " + to_string(defense) + " " + to_string(exp) + " " + to_string(money) + " " + to_string(max_hp) + " " + to_string(max_mp) + " " + to_string(getMP()) + " " + to_string(getHP()) + " ";


}
 BaseMonster* GoblinMonster::unserialize(string a) {


	istringstream iss(a);
	 string name; 
	 int attack; 
	 int defense;
	 int exp; 
	 int money;
	 int max_hp; 
	 int max_mp;
	 int mp;
	 int hp;
	 iss >> name >> attack >> defense >> exp >> money >> max_hp >> max_mp>>mp>>hp;

	 return new GoblinMonster();

}

 Item* GoblinMonster::dropItem() {
	 // 範例：每隻哥布林都掉一把劍
	 return new SwordWeapon();
 }
 vector<Item*> GoblinMonster::dropItems() {
	 std::vector<Item*> drops;
	 drops.push_back(new SwordWeapon());
	 drops.push_back(new AxeWeapon());
	 drops.push_back(new ShieldArmor());
	 drops.push_back(new TunicArmor());
	 drops.push_back(new LifePotion());
	 drops.push_back(new MagicPotion());
	 return drops;
 }