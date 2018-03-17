#pragma once

class CMall
{
public:
	CMall();
	~CMall();
	static void Load();
	static const CMall* GetMall(int id);
	static const float  GetDiscount(int id);
	int num() const { return num_; }
	int item_id() const { return item_id_; }
	int currency_type() const { return  currency_type_; }
	int currency_parameter() const { return  currency_parameter_; }
	int currency_num() const { return currency_num_; }
	int page_type() const { return page_type_; }

private:
	static std::map<int, const CMall*> mall_library_;
	static std::map<int, const float> discount_library_;
	int id_{ 0 };
	int item_id_{ 0 };
	int num_{ 0 };
	int currency_type_{ 0 };
	int currency_parameter_{ 0 };
	int currency_num_{ 0 };
	int page_type_{ 0 };
};

