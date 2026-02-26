#pragma once

class Menu {
public:
	virtual void show() = 0;           // 顯示選單
	virtual void handleInput() = 0;    // 處理使用者輸入
	virtual ~Menu() {}                 // 虛擬解構
};
