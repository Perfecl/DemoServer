#pragma once

class CGift;
typedef std::shared_ptr<CGift>		SPGift;

class CGift
{
public:
	CGift();
	~CGift();
	static void Load();
	static SPGift GetGift(int id);
	std::vector<std::pair<int, int>>* GetItem(){ return &item; }
private:
	static std::map<int, SPGift> gift_library;
	int id{ 0 };
	std::vector<std::pair<int, int>> item;
};

