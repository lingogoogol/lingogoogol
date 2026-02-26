#include"NovicePlayer.h"
#include"MagicianPlayer.h"
#include"OrcPlayer.h"
#include"KnightPlayer.h"


KnightPlayer::KnightPlayer() {

	setLevel(1);
	setName("anonymous");

}

KnightPlayer::KnightPlayer(int a) {

	setLevel(a);
	setName("anonymous");
}


KnightPlayer::KnightPlayer(int a, string b) {

	setLevel(a);
	setName(b);
}

KnightPlayer::KnightPlayer(const KnightPlayer&a) {

	*this = a;

}


void KnightPlayer::setLevel(int a) {

	level = a;
	attack = (40 + 10 * getLevel());
	defense = (20 + 12 * getLevel());
	maxhp = (150 + 25 * getLevel());
	maxmp = (70 + 10 * getLevel());
	setMp(maxmp);
	setHp(maxhp);
	setExp(0);
	setMoney(0);
	lvupexp = ceil(pow(10, log2(getLevel() + 1)));
}
void KnightPlayer::specialSkill() {

	setHp(getHp() + level * 10);
	setMp(getMp() - level * 5);

}

string KnightPlayer::serialize() {


	return getName() + " " + to_string(getHp()) + " " + to_string(getMp()) + " " + to_string(getExp()) + " " + to_string(getMoney()) + " " + to_string(getLevel()) + " " + to_string(getAttack()) + " " + to_string(getDefense()) + " " + to_string(getMaxHP()) + " " + to_string(getMaxMP()) + " " + to_string(getLvupExp());


}
NovicePlayer* KnightPlayer::unserialize(string a) {


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
	iss >> AA >> BB >> CC >> DD >> EE >> FF >> GG>>HH >> II >> JJ >> KK;

	return new KnightPlayer (AA, BB, CC, DD, EE, FF, GG,HH,II,JJ,KK);

}

KnightPlayer::KnightPlayer(string A, int B, int C, int D, int E, int F, int G, int H, int I, int J, int K) {
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