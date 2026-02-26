#include "Battle.h"
#include "Item.h"
#include "ConsumableItem.h"
#include "WeaponItem.h"
#include "ArmorItem.h"
#include "GoblinMonster.h"
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
using namespace std;

// 預設建構子
Battle::Battle() {
	numPlayers = 0;
	numMonsters = 0;
	numTurns = 0;
	ActionList = nullptr;
	srand(static_cast<unsigned int>(time(0))); // 修正 warning C4244
}

// 建構子（初始化玩家與怪物）
Battle::Battle(int nPlayers, int nMonsters, NovicePlayer* players[], BaseMonster* monsters[]) {
	numPlayers = nPlayers;
	numMonsters = nMonsters;
	numTurns = 0;
	ActionList = new Character[numPlayers + numMonsters];
	srand(static_cast<unsigned int>(time(0))); // 修正 warning C4244

	for (int i = 0; i < numPlayers; ++i) {
		ActionList[i].type = 'p';
		ActionList[i].alive = true;
		ActionList[i].instance = players[i];
	}

	for (int i = 0; i < numMonsters; ++i) {
		ActionList[numPlayers + i].type = 'm';
		ActionList[numPlayers + i].alive = true;
		ActionList[numPlayers + i].instance = monsters[i];
	}
}

// 解構子
Battle::~Battle() {
	delete[] ActionList;
}

// 玩家對怪物單次戰鬥
void Battle::fight(NovicePlayer* player, BaseMonster* monster) {
	cout << player->getName() << " attacks " << monster->name << "!" << endl;
	int damage = player->getAttack() - monster->defense;
	damage = (damage > 0) ? damage : 1;
	monster->setHP(monster->getHP() - damage);
	cout << monster->name << " took " << damage << " damage!" << endl;

	if (monster->getHP() <= 0) {
		cout << monster->name << " is defeated!" << endl;
		player->setMoney(player->getMoney() + monster->money);
	}
	else {
		cout << monster->name << " attacks back!" << endl;
		int counterDamage = monster->attack - player->getDefense();
		counterDamage = (counterDamage > 0) ? counterDamage : 1;
		player->setHp(player->getHp() - counterDamage);
		cout << player->getName() << " took " << counterDamage << " damage!" << endl;

		if (player->getHp() <= 0) {
			cout << player->getName() << " is defeated!" << endl;
		}
	}
}

// 新增：取得隨機活著角色 index
int Battle::getRandomAliveIndex(char targetType) {
	vector<int> aliveIndices;
	for (int i = 0; i < numPlayers + numMonsters; ++i) {
		if (ActionList[i].alive && ActionList[i].type == targetType) {
			aliveIndices.push_back(i);
		}
	}
	if (aliveIndices.empty()) return -1;
	int randIdx = rand() % aliveIndices.size();
	return aliveIndices[randIdx];
}

// 開始戰鬥（回合制）
void Battle::start() {
	while (!isGameOver()) {
		cout << "\n=== Turn " << numTurns + 1 << " ===" << endl;

		for (int i = 0; i < numPlayers + numMonsters; ++i) {
			if (!ActionList[i].alive) continue;

			// 玩家行動
			if (ActionList[i].type == 'p') {
				NovicePlayer* p = static_cast<NovicePlayer*>(ActionList[i].instance);
				cout << "\n>> 輪到玩家 " << p->getName() << " 行動。" << endl;

				while (true) {
					cout << "請選擇動作：\n"
						<< "1. 攻擊\n"
						<< "2. 使用道具\n"
						<< "3. 跳過回合\n"
						<< "輸入選項：";
					int choice;
					cin >> choice;

					if (choice == 1) {
						// 顯示所有活著的怪物
						vector<int> monsterIndices;
						cout << "請選擇要攻擊的怪物：" << endl;
						for (int j = 0; j < numPlayers + numMonsters; ++j) {
							if (ActionList[j].alive && ActionList[j].type == 'm') {
								BaseMonster* m = static_cast<BaseMonster*>(ActionList[j].instance);
								cout << monsterIndices.size() << ". " << m->name << " (HP: " << m->getHP() << ")" << endl;
								monsterIndices.push_back(j);
							}
						}

						if (monsterIndices.empty()) {
							cout << "沒有可攻擊的怪物！" << endl;
							break;
						}

						int selection = -1;
						while (true) {
							cout << "輸入怪物編號：";
							cin >> selection;
							if (selection >= 0 && selection < static_cast<int>(monsterIndices.size())) break;
							cout << "無效的選擇，請重新輸入。" << endl;
						}

						int targetIdx = monsterIndices[selection];
						BaseMonster* m = static_cast<BaseMonster*>(ActionList[targetIdx].instance);
						int damage = p->getAttack() - m->defense;
						damage = (damage > 0) ? damage : 1;
						m->setHP(m->getHP() - damage);
						cout << p->getName() << " 對 " << m->name << " 造成了 " << damage << " 傷害。" << endl;

						if (m->getHP() <= 0) {
							ActionList[targetIdx].alive = false;
							cout << m->name << " 被擊敗了！獲得金幣：" << m->money << endl;
							p->setMoney(p->getMoney() + m->money);

							GoblinMonster* goblin = dynamic_cast<GoblinMonster*>(m);
							if (goblin) {
								vector<Item*> drops = goblin->dropItems();
								for (Item* drop : drops) {
									if (p->putItem(static_cast<Item*>(drop))) {
										cout << p->getName() << " 獲得了掉落道具：" << drop->name << endl;
									}
									else {
										cout << "背包已滿，無法撿起：" << drop->name << "，已丟棄。" << endl;
										delete drop;
									}
								}
							}
							else {
								Item* drop = m->dropItem();
								if (drop != nullptr) {
									if (p->putItem(static_cast<Item*>(drop))) {
										cout << p->getName() << " 獲得了掉落道具：" << drop->name << endl;
									}
									else {
										cout << "背包已滿，無法撿起道具：" << drop->name << endl;
										delete drop;
									}
								}
							}
						}
						break;
					}
					else if (choice == 2) {
						p->viewBackpack();
						cout << "輸入要使用的道具編號：";
						int index;
						cin >> index;

						Item* item = p->takeItem(index);
						if (!item) {
							cout << "該格子沒有道具。" << endl;
							continue;
						}

						ConsumableItem* consumable = dynamic_cast<ConsumableItem*>(item);
						WeaponItem* weapon = dynamic_cast<WeaponItem*>(item);
						ArmorItem* armor = dynamic_cast<ArmorItem*>(item);

						if (consumable) {
							p->useConsumable(consumable);
						}
						else if (weapon) {
							if (!p->equipWeapon(weapon)) {
								p->putItem(static_cast<Item*>(weapon));
							}
						}
						else if (armor) {
							if (!p->equipArmor(armor)) {
								p->putItem(static_cast<Item*>(armor));
							}
						}
						else {
							cout << "此物品無法使用或裝備，放回背包。" << endl;
							p->putItem(item);
						}
						continue;
					}
					else if (choice == 3) {
						cout << p->getName() << " 跳過了回合。" << endl;
						break;
					}
					else {
						cout << "無效的輸入。" << endl;
					}
				}
			}
			// 怪物行動
			else if (ActionList[i].type == 'm') {
				BaseMonster* m = static_cast<BaseMonster*>(ActionList[i].instance);
				cout << m->name << " attacks!" << endl;

				int targetIdx = getRandomAliveIndex('p');
				if (targetIdx != -1) {
					NovicePlayer* p = static_cast<NovicePlayer*>(ActionList[targetIdx].instance);
					int damage = m->attack - p->getDefense();
					damage = (damage > 0) ? damage : 1;
					p->setHp(p->getHp() - damage);
					cout << m->name << " 對 " << p->getName() << " 造成了 " << damage << " 傷害。" << endl;

					if (p->getHp() <= 0) {
						ActionList[targetIdx].alive = false;
						cout << p->getName() << " 被擊敗了！" << endl;
					}
				}
			}
		}

		showStatus();
		numTurns++;
	}

	int alivePlayers = 0;
	for (int i = 0; i < numPlayers; ++i)
		if (ActionList[i].alive) alivePlayers++;

	if (alivePlayers > 0)
		cout << "[玩家獲勝！]" << endl;
	else
		cout << "[怪物獲勝！]" << endl;
}

// 顯示目前狀態
void Battle::showStatus() {
	cout << "\n-- Current Status --" << endl;
	for (int i = 0; i < numPlayers + numMonsters; ++i) {
		if (ActionList[i].type == 'p') {
			NovicePlayer* p = static_cast<NovicePlayer*>(ActionList[i].instance);
			cout << p->getName() << " HP: " << p->getHp() << (ActionList[i].alive ? " [alive]" : " [dead]") << endl;
		}
		else {
			BaseMonster* m = static_cast<BaseMonster*>(ActionList[i].instance);
			cout << m->name << " HP: " << m->getHP() << (ActionList[i].alive ? " [alive]" : " [dead]") << endl;
		}
	}
	cout << "--------------------\n" << endl;
}

// 判斷遊戲是否結束
bool Battle::isGameOver() {
	bool playersAlive = false;
	bool monstersAlive = false;

	for (int i = 0; i < numPlayers + numMonsters; ++i) {
		if (ActionList[i].alive) {
			if (ActionList[i].type == 'p') playersAlive = true;
			else if (ActionList[i].type == 'm') monstersAlive = true;
		}
	}
	return !playersAlive || !monstersAlive;
}
