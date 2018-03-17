#pragma once

class CFlashPolicy : public CBasicServer
{
public:
	CFlashPolicy();
	~CFlashPolicy();

protected:
	virtual void _OnReceive(networkz::ISession* session, int error_code) override;

private:
	void __ShowServerInfo();
};

