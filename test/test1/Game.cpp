#include "Game.h"
#include "GoblinMonster.h"
#include "ZombieMonster.h"
#include "JWMonster.h"
#include <iostream>
#include <conio.h>
#include <cstdlib>
#include <ctime>
#include "Item.h"
#include"WeaponItem.h"
#include"ArmorItem.h"
#include "ConsumableItem.h"
#include "LifePotion.h"
#include "MagicPotion.h"
#include "SwordWeapon.h"
#include "AxeWeapon.h"
#include "ShieldArmor.h"
#include "TunicArmor.h"
#include "TalkEvent.h"
#include "TreasureEvent.h"


using namespace std;
#include <fstream>
int** createTestMap();
// 儲存遊戲狀態
void Game::save(const std::string& filename) {
	std::ofstream ofs(filename);
	if (!ofs) {
		std::cerr << "無法開啟存檔檔案: " << filename << std::endl;
		return;
	}

	ofs << "STORY " << storylineProgress << std::endl;

	// 儲存玩家狀態
	ofs << players.size() << std::endl;
	for (auto p : players) {
		ofs << p->getJobName() << " "
			<< p->getName() << " "
			<< p->getHp() << " "
			<< p->getMp() << " "
			<< p->getExp() << " "
			<< p->getMoney() << " "
			<< p->getLevel() << std::endl;

		p->saveBackpack(ofs);
		p->saveEquipment(ofs);
	}

	// 儲存事件
	ofs << "EVENTS " << field->getEventMap().size() << std::endl;
	for (const auto& pair : field->getEventMap()) {
		ofs << pair.first.first << " " << pair.first.second << " "
			<< pair.second->isTriggered() << std::endl;
	}

	// 儲存位置
	ofs << field->getPlayerX() << " " << field->getPlayerY() << std::endl;

	std::cout << "遊戲已儲存至 " << filename << std::endl;
}

void Game::load(const std::string& filename) {
	std::ifstream ifs(filename);
	if (!ifs) {
		std::cerr << "無法開啟讀檔檔案: " << filename << std::endl;
		return;
	}

	std::string label;
	ifs >> label;
	if (label == "STORY") {
		ifs >> storylineProgress;
	}

	// 清除舊資料
	for (auto p : players) delete p;
	players.clear();

	int numPlayers;
	ifs >> numPlayers;

	for (int i = 0; i < numPlayers; ++i) {
		std::string job, name;
		int hp, mp, exp, money, level;
		ifs >> job >> name >> hp >> mp >> exp >> money >> level;

		NovicePlayer* player = nullptr;
		if (job == "Novice") player = new NovicePlayer(level, name);
		else if (job == "Knight") player = new KnightPlayer(level, name);
		else if (job == "Magician") player = new MagicianPlayer(level, name);
		else continue;

		player->setHp(hp);
		player->setMp(mp);
		player->setExp(exp);
		player->setMoney(money);
		player->loadBackpack(ifs);
		player->loadEquipment(ifs);

		players.push_back(player);
	}

	ifs >> label;
	if (label == "EVENTS") {
		int ecount;
		ifs >> ecount;
		for (int i = 0; i < ecount; ++i) {
			int x, y;
			bool triggered;
			ifs >> x >> y >> triggered;
			auto& ev = field->getEventRef(x, y);
			if (ev) ev->setTriggered(triggered);
		}
	}

	int x, y;
	ifs >> x >> y;

	// 重建地圖與戰鬥
	setupMap();
	if (field) delete field;
	if (battle) delete battle;

	battle = new Battle(players.size(), monsters.size(), players.data(), monsters.data());
	field = new Field(map, x, y, 80, 50, mapWidth, mapHeight, battle);

	// 重建事件（每次都要重建並手動補進 events 陣列）
	events.clear();
	events.push_back(new TalkEvent("你聽到遠方傳來神秘聲音..."));
	field->addEvent(5, 9, events.back());
	events.push_back(new TreasureEvent(players[0], new LifePotion()));
	field->addEvent(10, 10, events.back());

	std::cout << "遊戲已從 " << filename << " 載入！" << std::endl;
}

void runBackpackMenu(NovicePlayer* player) {
	while (true) {
		cout << "\n=== 背包功能選單 ===" << endl;
		cout << "1. 檢視背包" << endl;
		cout << "2. 使用或裝備道具" << endl;
		cout << "3. 返回主選單" << endl;
		cout << "請輸入選項：";

		int choice;
		cin >> choice;

		if (choice == 1) {
			player->viewBackpack();
		}
		else if (choice == 2) {
			player->viewBackpack();
			cout << "輸入要使用的道具編號：";
			int index;
			cin >> index;

			Item* item = player->takeItem(index);
			if (!item) {
				cout << "該格子無道具！" << endl;
				continue;
			}

			ConsumableItem* consumable = dynamic_cast<ConsumableItem*>(item);
			WeaponItem* weapon = dynamic_cast<WeaponItem*>(item);
			ArmorItem* armor = dynamic_cast<ArmorItem*>(item);

			if (consumable) {
				player->useConsumable(consumable);
			}
			else if (weapon) {
				if (!player->equipWeapon(weapon)) {
					player->putItem(weapon);
				}
			}
			else if (armor) {
				if (!player->equipArmor(armor)) {
					player->putItem(armor);
				}
			}
			else {
				cout << "這個物品無法使用或裝備。" << endl;
				player->putItem(item);
			}
		}
		else if (choice == 3) {
			break;
		}
		else {
			cout << "無效的輸入！" << endl;
		}
	}
}

Game::Game() {
	setupPlayers();
	setupMonsters();
	setupMap();

	battle = new Battle(players.size(), monsters.size(), players.data(), monsters.data());
	field = new Field(map, 5, 5, 80, 50, mapWidth, mapHeight, battle);
	field->addMonster(6, 5);
	field->addMonster(4, 6);
	field->addMonster(3, 6);
}

Game::~Game() {
	delete field;
	delete battle;
	for (auto p : players) delete p;
	for (auto m : monsters) delete m;
	for (int i = 0; i < mapHeight; ++i)
		delete[] map[i];
	delete[] map;
}

void Game::setupPlayers() {
	players.push_back(new NovicePlayer(100, "新手A"));
	players.push_back(new KnightPlayer(2, "騎士B"));
	players.push_back(new MagicianPlayer(3, "法師C"));
}

void Game::setupMonsters() {
	monsters.push_back(new GoblinMonster());
	monsters.push_back(new ZombieMonster());
	monsters.push_back(new JWMonster());
}

void Game::setupMap() {
	int** map1 = createTestMap();
	int** map2 = createTestMap();
	int** map3 = createTestMap();

	map = new int*[mapHeight];
	for (int i = 0; i < mapHeight; ++i) {
		map[i] = new int[mapWidth];
		for (int j = 0; j < mapWidth; ++j) {
			map[i][j] = 0;
		}
	}
	fields[0] = new Field(map1, 5, 5, 80, 50, 1000, 1000, battle);
	fields[1] = new Field(map2, 10, 10, 80, 50, 1000, 1000, battle);
	fields[2] = new Field(map3, 15, 15, 80, 50, 1000, 1000, battle);
	currentFieldIndex = 0;
	field = fields[currentFieldIndex];

	for (int i = 0; i < mapWidth; ++i) {
		map[0][i] = map[99][i] = 1;
		map[i][0] = map[i][99] = 1;
	}

	map[3][3] = 1; map[4][4] = 1; map[5][7] = 1; map[5][8] = 1;
	map[5][9] = 2; map[10][10] = 2; map[20][20] = 2;
	map[15][15] = 3; map[16][16] = 3;

	srand((unsigned)time(0));
	for (int i = 0; i < 50; ++i) {
		int x = rand() % 998 + 1;
		int y = rand() % 998 + 1;
		map[y][x] = 1;
	}
}

void Game::clearScreen() {
	cout << "\033[2J\033[1;1H";
	system("cls");
}

void Game::gameLoop() {
	char cmd;
	do {
		field->display();
		cout << "輸入移動指令(WASD), I=背包, z=存檔, Q=退出: ";
		cmd = _getch();
		cmd = toupper(cmd);

		if (cmd == 'z') {
			save("save.txt");
			continue;
		}
		if (cmd == 'Q') break;

		if (cmd == 'I') {
			runBackpackMenu(players[0]);
			continue;
		}

		if (!field->move(cmd)) {
			cout << "無法移動到該方向！" << endl;
		}

	} while (cmd != 'Q');

	cout << "按任意鍵結束遊戲..." << endl;
	_getch();
}

void Game::run() {
	gameLoop();
}

int** createTestMap() {
	int** map = new int*[1000];
	for (int i = 0; i < 1000; ++i) {
		map[i] = new int[1000];
		for (int j = 0; j < 1000; ++j) {
			map[i][j] = 0;
		}
	}

	for (int i = 0; i < 1000; ++i) {
		map[0][i] = map[99][i] = 1;
		map[i][0] = map[i][99] = 1;
	}

	map[3][3] = 1;
	map[4][4] = 1;
	map[5][7] = 1;
	map[5][8] = 1;
	map[5][9] = 2;
	map[10][10] = 2;
	map[20][20] = 2;
	map[15][15] = 3;
	map[16][16] = 3;

	srand(static_cast<unsigned int>(time(nullptr)));
	for (int i = 0; i < 50; ++i) {
		int x = rand() % 998 + 1;
		int y = rand() % 998 + 1;
		map[y][x] = 1;
	}

	return map;
}