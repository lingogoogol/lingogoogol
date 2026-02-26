#ifndef FIELD_H
#define FIELD_H

#include <iostream>
#include <string>
#include <vector>
#include "Battle.h"
#include <map>
#include "Event.h"

class Field {
public:
	int getPlayerX() const;
	int getPlayerY() const;
	// 使用地圖資料與尺寸建構 Field
	Field(int** data, int x, int y, int width, int height, int mapW, int mapH, Battle* b);
	// 從檔案讀取地圖建構 Field
	Field(const char* filename, int x, int y, int width, int height);
	~Field();
	map<std::pair<int, int>, Event*> eventMap;
	// 移動控制
	bool move(char dir);
	bool moveUp();
	bool moveDown();
	bool moveLeft();
	bool moveRight();
	void addEvent(int x, int y, Event* event);
	void checkEvent();
	// 怪物位置處理
	void addMonster(int x, int y);
	bool isMonster(int x, int y) const;

	// 地圖與視野資訊
	int getCurrentPositionX() const;
	int getCurrentPositionY() const;
	int getVisionWidth() const;
	int getVisionHeight() const;
	std::string getMapName() const;
	int getMapSymbol(int x, int y);

	void setPosition(int x, int y);
	void setMapSymbol(int symbol, int x, int y);
	void setVisionSize(int width, int height);

	// 顯示地圖
	void display() const;

	std::vector<std::pair<int, int>> monster_positions; // 怪物的位置 (x, y)
	Battle* battle; // 戰鬥系統指標
	std::map<std::pair<int, int>, Event*>& getEventMap() { return eventMap; }
	Event*& getEventRef(int x, int y) { return eventMap[{x, y}]; }
private:
	int** map_data; // 二維地圖資料
	int map_width;
	int map_height;

	int current_position_x;
	int current_position_y;

	int vision_width;
	int vision_height;

	std::string map_name;
};

#endif // FIELD_H
