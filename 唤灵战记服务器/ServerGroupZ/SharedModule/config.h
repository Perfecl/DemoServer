#pragma once

#pragma warning(disable : 4251)
#pragma warning(disable : 4099)

#ifdef _DEBUG
#define _TEST
#define _TRACE
#endif

//��Ϸ����Ŀ¼
#define GAME_DATA_PATH			"D:\\workspace\\WindofKindom\\GameData\\Data_Server\\"
#define GAME_EDIT_MAP_PATH		"D:\\workspace\\WindofKindom\\GameData\\Map\\"

//�û��ʺ������
const size_t kUsernameMaxLength{ 36 };

//������ֳ���
const size_t kMaxPlayerNameLength{ 14 };

//��ȫ��Կ
#define SECURITY_KEY "6JLIm1MPHL"