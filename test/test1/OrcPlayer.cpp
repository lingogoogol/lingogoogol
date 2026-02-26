#include"NovicePlayer.h"
#include"MagicianPlayer.h"
#include"OrcPlayer.h"
#include"KnightPlayer.h"
OrcPlayer::OrcPlayer() {

	setLevel(1);
	setName("anonymous");

}

OrcPlayer::OrcPlayer(int a) {

	setLevel(a);
	setName("anonymous");
}


OrcPlayer::OrcPlayer(int a, string b) {

	setLevel(a);
	setName(b);
}

OrcPlayer::OrcPlayer(const OrcPlayer&a) {

	*this = a;

}


void OrcPlayer::setLevel(int a) {

	level = a;
	attack = (50 + 12 * getLevel());
	defense = (30 + 10 * getLevel());
	maxhp = (200 + 20 * getLevel());
	maxmp = (50 + 5 * getLevel());
	setMp(maxmp);
	setHp(maxhp);
	setExp(0);
	setMoney(0);
	lvupexp = ceil(pow(10, log2(getLevel() + 1)));
}

string OrcPlayer::serialize() {


	return getName() + " " + to_string(getHp()) + " " + to_string(getMp()) + " " + to_string(getExp()) + " " + to_string(getMoney()) + " " + to_string(getLevel()) + " " + to_string(getAttack()) + " " + to_string(getDefense()) + " " + to_string(getMaxHP()) + " " + to_string(getMaxMP()) + " " + to_string(getLvupExp());


}
NovicePlayer* OrcPlayer::unserialize(string a) {


	istringstream iss(a);
	string AA;
	int BB;
	int CC;
	int DD;
	int EE;
	int FF;
	int GG;
	int HH;
	int II;
	int JJ;
	int KK;
	iss >> AA >> BB >> CC >> DD >> EE >> FF >> GG >> HH >> II >> JJ >> KK;

	return new OrcPlayer (AA, BB, CC, DD, EE, FF, GG, HH, II, JJ, KK);

}

OrcPlayer::OrcPlayer(string A, int B, int C, int D , int E, int F , int G, int H, int I, int J , int K) {
	maxhp = I;
	maxmp = J;
	lvupexp = K;
	setName(A);
		setHp(B);
		setMp(C);
		setExp(D);
		setMoney(E);
		setLevel(F);
		attack=G;
		defense=H;
	

}
