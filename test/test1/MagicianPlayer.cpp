#include"NovicePlayer.h"
#include"MagicianPlayer.h"
#include"OrcPlayer.h"
#include"KnightPlayer.h"

MagicianPlayer::MagicianPlayer() {

	setLevel(1);
	setName("anonymous");

}

MagicianPlayer::MagicianPlayer(int a) {

	setLevel(a);
	setName("anonymous");
}


MagicianPlayer::MagicianPlayer(int a, string b) {

	setLevel(a);
	setName(b);
}

MagicianPlayer::MagicianPlayer(const MagicianPlayer&a) {

	*this = a;

}


void MagicianPlayer::setLevel(int a) {

	level = a;
	attack = (30 + 8 * getLevel());
	defense = (20 + 7 * getLevel());
	maxhp = (120 + 15 * getLevel());
	maxmp = (100 + 15 * getLevel());
	setMp(maxmp);
	setHp(maxhp);
	setExp(0);
	setMoney(0);
	lvupexp = ceil(pow(10, log2(getLevel() + 1)));
}

void MagicianPlayer::specialSkill() {

	setMp(getMp() + level * 10);
	setHp(getHp() - level * 5);

}

string MagicianPlayer::serialize() {


	return getName() + " " + to_string(getHp()) + " " + to_string(getMp()) + " " + to_string(getExp()) + " " + to_string(getMoney()) + " " + to_string(getLevel()) + " " + to_string(getAttack()) + " " + to_string(getDefense()) + " " + to_string(getMaxHP()) + " " + to_string(getMaxMP()) + " " + to_string(getLvupExp());


}
NovicePlayer* MagicianPlayer::unserialize(string a) {


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

	return new MagicianPlayer(AA, BB, CC, DD, EE, FF, GG, HH, II, JJ, KK);

}

MagicianPlayer::MagicianPlayer(string A, int B, int C, int D, int E, int F, int G, int H, int I, int J, int K) {
	
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
	


}