#include "BaseMonster.h"

int BaseMonster:: count=0;

BaseMonster::BaseMonster(string a, int b, int c, int d, int e, int f, int g,int h,int i) :name(a), attack(b), defense(c), exp(d), money(e), max_hp(f), max_mp(),mp(h),hp(i){

	count++;

}



BaseMonster::~BaseMonster() {


	count--;



}


void BaseMonster::setHP(int a) {




	if (a > max_hp || a < 0) {
		cout << "¶W¹Lhp½d³ò";
		
			hp = 0;
	}
	else
	hp = a;


}

int BaseMonster::getHP() const {
	return hp;

}
void BaseMonster::setMP(int a) {




	if (a > max_mp || a < 0) {
		cout << "¶W¹Lmp½d³ò";
	}
	mp = a;


}

int BaseMonster::getMP() const {
	return mp;

}

int BaseMonster::getInstanceCount() {

	return count;

}

