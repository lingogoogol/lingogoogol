#include "field.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <vector>
using namespace std;

Field::Field(int** data, int x, int y, int width, int height, int mapW, int mapH, Battle* b)
	: current_position_x(x), current_position_y(y),
	vision_width(width), vision_height(height),
	map_width(mapW), map_height(mapH),
	map_name("Default Map"), battle(b) {

	map_data = new int*[map_height];
	for (int i = 0; i < map_height; i++) {
		map_data[i] = new int[map_width];
		for (int j = 0; j < map_width; j++) {
			map_data[i][j] = data[i][j];
		}
	}
}

Field::Field(const char* filename, int x, int y, int width, int height)
	: current_position_x(x), current_position_y(y),
	vision_width(width), vision_height(height), battle(nullptr) {
	ifstream file(filename);
	if (!file.is_open()) {
		cerr << "Failed to open map file." << endl;
		exit(1);
	}
	map_name = filename;

	vector<vector<int>> temp_map;
	string line;
	while (getline(file, line)) {
		istringstream iss(line);
		int val;
		vector<int> row;
		while (iss >> val) {
			row.push_back(val);
		}
		temp_map.push_back(row);
	}
	file.close();

	map_height = temp_map.size();
	map_width = temp_map[0].size();
	map_data = new int*[map_height];
	for (int i = 0; i < map_height; i++) {
		map_data[i] = new int[map_width];
		for (int j = 0; j < map_width; j++) {
			map_data[i][j] = temp_map[i][j];
		}
	}
}

Field::~Field() {
	for (int i = 0; i < map_height; i++) {
		delete[] map_data[i];
	}
	delete[] map_data;
}
void Field::addEvent(int x, int y, Event* event) {
	eventMap[{x, y}] = event;
}


bool Field::move(char dir) {
	switch (dir) {
	case 'W': return moveUp();
	case 'A': return moveLeft();
	case 'S': return moveDown();
	case 'D': return moveRight();
	default: return false;
		checkEvent(); // 移動後觸發事件

	}
}

void Field::addMonster(int x, int y) {
	if (x >= 0 && y >= 0 && x < map_width && y < map_height)
		monster_positions.push_back(make_pair(x, y));
}

bool Field::isMonster(int x, int y) const {
	for (auto& m : monster_positions) {
		if (m.first == x && m.second == y) return true;
	}
	return false;
}

bool Field::moveUp() {
	int newY = current_position_y - 1;
	int x = current_position_x;
	if (newY >= 0 && map_data[newY][x] != 1) {
		if (map_data[newY][x] == 2) {
			cout << "你觸發了事件！\n";
			return true;
		}
		if (isMonster(x, newY)) {
			cout << "遇到怪物！進入戰鬥！" << endl;
			if (battle) battle->start();
			return false;
		}
		current_position_y = newY;
		return true;
	}
	return false;
}
void Field::checkEvent() {
	auto it = eventMap.find({ getPlayerX(), getPlayerY() });
	if (it != eventMap.end() && it->second->canTrigger()) {
		it->second->trigger();
	}
}
bool Field::moveDown() {
	int newY = current_position_y + 1;
	int x = current_position_x;
	if (newY < map_height && map_data[newY][x] != 1) {
		if (map_data[newY][x] == 2) {
			cout << "你觸發了事件！\n";
			return true;
		}
		if (isMonster(x, newY)) {
			cout << "遇到怪物！進入戰鬥！" << endl;
			if (battle) battle->start();
			return false;
		}
		current_position_y = newY;
		return true;
	}
	return false;
}

bool Field::moveLeft() {
	int newX = current_position_x - 1;
	int y = current_position_y;
	if (newX >= 0 && map_data[y][newX] != 1) {
		if (map_data[y][newX] == 2) {
			cout << "你觸發了事件！\n";
			return true;
		}
		if (isMonster(newX, y)) {
			cout << "遇到怪物！進入戰鬥！" << endl;
			if (battle) battle->start();
			return false;
		}
		current_position_x = newX;
		return true;
	}
	return false;
}

bool Field::moveRight() {
	int newX = current_position_x + 1;
	int y = current_position_y;
	if (newX < map_width && map_data[y][newX] != 1) {
		if (map_data[y][newX] == 2) {
			cout << "你觸發了事件！\n";
			return true;
		}
		if (isMonster(newX, y)) {
			cout << "遇到怪物！進入戰鬥！" << endl;
			if (battle) battle->start();
			return false;
		}
		current_position_x = newX;
		return true;
	}
	return false;
}

int Field::getCurrentPositionX() const { return current_position_x; }
int Field::getCurrentPositionY() const { return current_position_y; }
int Field::getVisionWidth() const { return vision_width; }
int Field::getVisionHeight() const { return vision_height; }
string Field::getMapName() const { return map_name; }
int Field::getMapSymbol(int x, int y) { return map_data[y][x]; }

void Field::setPosition(int x, int y) {
	current_position_x = x;
	current_position_y = y;
}

void Field::setMapSymbol(int symbol, int x, int y) {
	if (x >= 0 && x < map_width && y >= 0 && y < map_height)
		map_data[y][x] = symbol;
}

void Field::setVisionSize(int width, int height) {
	vision_width = width;
	vision_height = height;
}

void Field::display() const {
	cout << "====== 地圖視野 ======" << endl;
	cout << "位置: (" << current_position_x << ", " << current_position_y << ")  地圖: " << map_name << endl;

	int half_w = vision_width / 2;
	int half_h = vision_height / 2;

	int startX = current_position_x - half_w;
	int endX = current_position_x + half_w;
	int startY = current_position_y - half_h;
	int endY = current_position_y + half_h;

	cout << "   ";
	for (int x = startX; x <= endX; ++x) {
		if (x >= 0 && x < map_width) cout << x % 10;
		else cout << " ";
	}
	cout << endl;

	for (int y = startY; y <= endY; ++y) {
		if (y >= 0 && y < map_height) cout << y % 10 << " ";
		else cout << "  ";

		for (int x = startX; x <= endX; ++x) {
			if (x == current_position_x && y == current_position_y) cout << "P";
			else if (x >= 0 && y >= 0 && x < map_width && y < map_height) {
				if (isMonster(x, y)) cout << "M";
				else {
					switch (map_data[y][x]) {
					case 0: cout << "."; break;
					case 1: cout << "#"; break;
					case 2: cout << "E"; break;
					default: cout << "?"; break;
					}
				}
			}
			else {
				cout << " ";
			}
		}
		cout << endl;
	}
	cout << "=======================" << endl;
}
int Field::getPlayerX() const {
	return current_position_x;
}
int Field::getPlayerY() const {
	return current_position_y;
}