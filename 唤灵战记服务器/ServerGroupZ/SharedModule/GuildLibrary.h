#pragma once

class CGuildLibrary;
typedef std::shared_ptr<CGuildLibrary>		SPGuildLibrary;

class CGuildLibrary
{
public:
	CGuildLibrary();
	~CGuildLibrary();
	static void Load();
	static SPGuildLibrary GetGuildLibrary(int level);
	int exp() const { return exp_; }
private:
	static std::map<int, SPGuildLibrary> guild_library_;
	int level_{ 0 };
	int max_member_{ 0 };
	int exp_{ 0 };
	int times_{ 0 };
};

