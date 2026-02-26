#pragma once
class Event {
protected:
	bool triggered = false;
public:
	virtual bool canTrigger() const { return !triggered; }
	virtual void trigger() = 0;
	virtual std::string getDescription() const = 0;

	// 🔽 這兩個一定要有！
	virtual bool isTriggered() const { return triggered; }
	virtual void setTriggered(bool val) { triggered = val; }

	virtual ~Event() {}
};
