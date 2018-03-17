#pragma once
class CRecastCost
{
public:
	CRecastCost();
	~CRecastCost();
	static void Load();
	static int GetStoneNum(int quality);

public:
	static std::map<int, int> recast_cost_lib_;
};

