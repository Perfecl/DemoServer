#ifndef THREADCLASS_HPP_INCLUDED
#define THREADCLASS_HPP_INCLUDED

class IThreadClass
{
public:
	IThreadClass(boost::thread::id threadID)
	{
		m_ownerThreadID = threadID;
	}

	IThreadClass()
	{
		m_ownerThreadID = boost::this_thread::get_id();
	}

	virtual ~IThreadClass()
	{

	}

public:
	bool CheckIsThreadSafe()
	{
		if (boost::this_thread::get_id() == m_ownerThreadID)
		{
			return true;
		}

		return false;
	}

protected:
	boost::thread::id m_ownerThreadID;
};

#endif // THREADCLASS_HPP_INCLUDED

