#ifndef BASEMONSTER_H
#define BASEMONSTER_H
#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "Item.h"
using namespace std;

class BaseMonster {

	public:
		const string name; // Name of the monster
		const int attack; // Attack of the monster
		const int defense; // Defense of the monster 
		const int exp; // Experience earned by players after beating this monster
		const int money; // Amount of money dropped after beating this monster
		const int max_hp; // The monster’s maximum HP
		const int max_mp; // The monster’s maximum MP
		BaseMonster(string, int, int, int, int, int, int,int,int);
		virtual ~BaseMonster();
		void setHP(int);
		int getHP() const;
		void setMP(int);
		int getMP() const;
		int getInstanceCount();
		virtual string serialize() const = 0;
		static BaseMonster*unserialize(string);
		void display() {
			cout << "-------------------------" << endl;
			cout << setw(10) << "name:" << name << endl;
			cout << setw(10) << "hp:" << hp << endl;
			cout << setw(10) << "mp:" << mp << endl;
			cout << setw(10) << "attack:" << attack << endl;
			cout << setw(10) << "defense:" << defense << endl;
			cout << setw(10) << "maxhp:" << max_hp << endl;
			cout << setw(10) << "maxmp:" << max_mp << endl;
			cout << "-------------------------" << endl;
		}
		virtual Item* dropItem() {
			return nullptr; // 預設無掉落，子類可以覆寫
		}

	private:
		int hp; // Current HP of this monster, range: [0, max_hp]
		int mp; // Current MP of this monster, range: [0, max_mp]
		static int count; // Number of instances of monster series classes

};







#endif