#pragma once

class CCriticalSectionZ
{
public:
	CCriticalSectionZ(){ InitializeCriticalSectionAndSpinCount(&critical_section_, 4000); }
	~CCriticalSectionZ(){ DeleteCriticalSection(&critical_section_); }

	inline void lock(){ EnterCriticalSection(&critical_section_); }
	inline void unlock(){ LeaveCriticalSection(&critical_section_); }

private:
	CRITICAL_SECTION  critical_section_;
};

template<typename T> class CLock
{
public:
	CLock(T &lock) : lock_{ lock } { lock_.lock(); }
	~CLock(){ lock_.unlock(); }

	CLock() = delete;
	CLock(const CLock&) = delete;
	CLock& operator =(const CLock&) = delete;

private:
	T &lock_;
};

#define LOCK_BLOCK(x)	CLock<CCriticalSectionZ> do_lock(x);
#define LOCK_BLOCK2(x)	CLock<CCriticalSectionZ> do_lock2(x);
#define LOCK_BLOCK3(x)	CLock<CCriticalSectionZ> do_lock3(x);