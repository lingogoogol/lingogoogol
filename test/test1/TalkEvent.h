#pragma once
#include "Event.h"
#include <string>
#include <iostream>

class TalkEvent : public Event {
private:
	std::string message;
public:
	TalkEvent(const std::string& msg) : message(msg) {}
	void trigger() override {
		if (!triggered) {
			std::cout << "[事件] " << message << std::endl;
			triggered = true;
		}
	}
	std::string getDescription() const override{
		return "對話事件";
	}
};
