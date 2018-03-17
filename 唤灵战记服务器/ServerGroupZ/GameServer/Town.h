#pragma once

class CTown
{
public:
	CTown(int id);
	~CTown();

	ENUM_EnterTownResult	EnterTown(SPPlayer player);																								//�������
	void					LeaveTown(int pid);																										//�뿪����
	void					TownMove(int pid, int x, int y);																						//�����ƶ�

	SPPlayer				FindPlayerInTown(int pid);																								//���ҳ����е����

	void					SendAllPlayersInTown(const std::string& content, unsigned char protocol_type, int protocol_id, int except_pid);			//���͸����г��������

	void					SetAllPlayersToProtocol(pto_TOWN_S2C_RES_EnterTown& pto);																//�������������Э��

	void					SetTop10Protocol(pto_PLAYER_S2C_RES_UpdateTownStageRank* pto);															//�������а�Э��
	bool					AddTop10Player(int pid);

	inline void main_progress(int progress){ main_progress_ = progress; }
	inline int	town_id() const { return town_id_; }

private:
	const int	town_id_;											//����ID
	int			main_progress_{ 0 };								//���߽���

	std::map<int, SPPlayer>					players_in_town_;		//�����е����
	CCriticalSectionZ						lock_;					//�߳���

	std::vector<std::pair<int, time_t>>		pass_ranking_list_;		//�������а�

	void __LoadFromDatabase();										//�����ݿ��ж�ȡ
};

