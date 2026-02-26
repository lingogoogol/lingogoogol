#pragma once
#include "Event.h"
#include "Item.h"
#include "NovicePlayer.h"
#include <iostream>

class TreasureEvent : public Event {
private:
	NovicePlayer* player;
	Item* reward;
public:
	TreasureEvent(NovicePlayer* p, Item* i) : player(p), reward(i) {}

	void trigger() override {
		if (!triggered) {
			std::cout << "[事件] 你發現了一個寶箱！獲得道具：" << reward->getName() << std::endl;
			player->putItem(reward);
			triggered = true;
		}
	}

	std::string getDescription() const override {
		return "寶箱事件";
	}
};
