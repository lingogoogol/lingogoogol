#include"ZombieMonster.h"

ZombieMonster::ZombieMonster() :BaseMonster("Zombie", 50, 65, 17, 65, 150, 30,30,150) {



}

string ZombieMonster::serialize() const {


	return name + " " + to_string(attack) + " " + to_string(defense) + " " + to_string(exp) + " " + to_string(money) + " " + to_string(max_hp) + " " + to_string(max_mp) + " " + to_string(getMP()) + " " + to_string(getHP()) + " ";


}
BaseMonster* ZombieMonster::unserialize(string a) {


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
	iss >> name >> attack >> defense >> exp >> money >> max_hp >> max_mp >> mp >> hp;

	return new ZombieMonster();

}