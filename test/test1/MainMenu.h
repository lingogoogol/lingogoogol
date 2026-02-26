#pragma once
#include "Menu.h"

class Game; // 前向宣告

class MainMenu : public Menu {
private:
	Game* game;

public:
	MainMenu(Game* game); // 需要綁定一個 Game 指標以便呼叫 run() 或 load()
	void show() override;
	void handleInput() override;
};
