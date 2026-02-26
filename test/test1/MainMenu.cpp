#include "MainMenu.h"
#include "Game.h"
#include <iostream>
#include <conio.h>

using namespace std;

MainMenu::MainMenu(Game* g) : game(g) {}

void MainMenu::show() {
	cout << "\n=== 主選單 ===\n";
	cout << "1. 開始新遊戲\n";
	cout << "2. 載入進度（尚未實作）\n";
	cout << "3. 離開\n";
	cout << "請選擇：";
}

void MainMenu::handleInput() {
	char choice = _getch();

	switch (choice) {
	case '1':
		game->run();  // 開始新遊戲
		break;
	case '2':
		game->load("save.txt"); // 從 save.txt 載入
		game->run();
		break;
	case '3':
		std::cout << "\n再見！" << std::endl;
		break;
	default:
		std::cout << "\n無效輸入！請重新選擇。\n";
		break;
	}
}