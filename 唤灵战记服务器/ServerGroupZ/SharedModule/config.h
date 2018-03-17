#pragma once

#pragma warning(disable : 4251)
#pragma warning(disable : 4099)

#ifdef _DEBUG
#define _TEST
#define _TRACE
#endif

//游戏数据目录
#define GAME_DATA_PATH			"D:\\workspace\\WindofKindom\\GameData\\Data_Server\\"
#define GAME_EDIT_MAP_PATH		"D:\\workspace\\WindofKindom\\GameData\\Map\\"

//用户帐号最长长度
const size_t kUsernameMaxLength{ 36 };

//玩家名字长度
const size_t kMaxPlayerNameLength{ 14 };

//安全密钥
#define SECURITY_KEY "6JLIm1MPHL"