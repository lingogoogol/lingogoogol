#include "Game.h"
#include "MainMenu.h"
#include <conio.h>

int main() {
	Game myRPG;
	MainMenu menu(&myRPG);

	while (true) {
		menu.show();
		menu.handleInput();
		break; // 暫時不循環主選單，可依需求擴充
	}

	return 0;
}
