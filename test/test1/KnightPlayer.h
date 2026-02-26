#ifndef KNIGHTPLAYER_H
#define KNIGHTPLAYER_H
#include"NovicePlayer.h"

class KnightPlayer :public NovicePlayer
{
public:
	KnightPlayer();
	KnightPlayer(int);
	KnightPlayer(int, string);
	KnightPlayer(const KnightPlayer&);
	KnightPlayer(string, int, int, int, int, int, int, int, int, int,int);
	void setLevel(int);
	string getJobName() const override { return "Knight"; }
	void specialSkill() override;
	 string serialize();
	static NovicePlayer*unserialize(string);
};
#endif
