#pragma once
#include "Event.h"
#include "Field.h"
#include <iostream>

class ChangeMapEvent : public Event {
private:
	Field** fields;
	int* currentFieldIndex;
	int targetIndex;
public:
	ChangeMapEvent(Field** f, int* cur, int to)
		: fields(f), currentFieldIndex(cur), targetIndex(to) {}

	void trigger() override {
		if (!triggered) {
			std::cout << "[事件] 你踏入神秘傳送點，來到新地圖...\n";
			*currentFieldIndex = targetIndex;
			triggered = true;
		}
	}

	std::string getDescription() const override {
		return "場景傳送事件";
	}
};
