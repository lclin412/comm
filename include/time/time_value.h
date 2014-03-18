
#ifndef TIME_VALUE_H
#define TIME_VALUE_H

#define ONE_SECOND_IN_MSECS 1000L
#define ONE_SECOND_IN_USECS 1000000L
#define ONE_SECOND_IN_NSECS 1000000000L

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

class CTimeValue  
{
public:
	// add the follwoing two functions to avoid call Normalize().
	CTimeValue();
	CTimeValue(long lSec);
	CTimeValue(long lSec, long lUSec);
	CTimeValue(const timeval &tvTimeValue);
	
	void Set(long lSec, long lUSec);
	void Set(const timeval &tvTimeValue);

	long GetSec() const ;
	long GetUSec() const ;

	void SetByTotalMSec(long aMillilSeconds);
	long GetTotalInMSec() const;

	void operator += (const CTimeValue &oRight);
	void operator -= (const CTimeValue &oRight);
	void operator = (const CTimeValue &oRight);

	friend CTimeValue operator + (const CTimeValue &oLeft, const CTimeValue &oRight);
	friend CTimeValue operator - (const CTimeValue &oLeft, const CTimeValue &oRight);
	friend int operator < (const CTimeValue &oLeft, const CTimeValue &oRight);
	friend int operator > (const CTimeValue &oLeft, const CTimeValue &oRight);
	friend int operator <= (const CTimeValue &oLeft, const CTimeValue &oRight);
	friend int operator >= (const CTimeValue &oLeft, const CTimeValue &oRight);
	friend int operator == (const CTimeValue &oLeft, const CTimeValue &oRight);
	friend int operator != (const CTimeValue &oLeft, const CTimeValue &oRight);

public:

	static CTimeValue GetTimeOfDay();

	static const CTimeValue s_tvZero;
	static const CTimeValue s_tvMax;
	
private:
	void Normalize();
	
	long m_lSec;
	long m_lUSec;
};

// inline functions
inline CTimeValue::CTimeValue()
	: m_lSec(0)
	, m_lUSec(0)
{
}

inline CTimeValue::CTimeValue(long lSec)
	: m_lSec(lSec)
	, m_lUSec(0)
{
}

inline CTimeValue::CTimeValue(long lSec, long lUSec)
{
	Set(lSec, lUSec);
}

inline CTimeValue::CTimeValue(const timeval &tvTimeValue)
{
	Set(tvTimeValue);
}

inline void CTimeValue::Set(long lSec, long lUSec)
{
	m_lSec = lSec;
	m_lUSec = lUSec;
	Normalize();
}

inline void CTimeValue::Set(const timeval &tvTimeValue)
{
	m_lSec = tvTimeValue.tv_sec;
	m_lUSec = tvTimeValue.tv_usec;
	Normalize();
}

inline void CTimeValue::SetByTotalMSec(long aMillilSeconds)
{
	m_lSec = aMillilSeconds / 1000;
	m_lUSec = (aMillilSeconds - (m_lSec * 1000)) * 1000;
}

inline long CTimeValue::GetSec() const 
{
	return m_lSec;
}

inline long CTimeValue::GetUSec() const 
{
	return m_lUSec;
}

inline long CTimeValue::GetTotalInMSec() const
{
	return m_lSec * 1000 + m_lUSec / 1000;
}

inline CTimeValue CTimeValue::GetTimeOfDay()
{
	timeval tvCur;
	::gettimeofday(&tvCur, NULL);
	return CTimeValue(tvCur);
}

inline int operator > (const CTimeValue &oLeft, const CTimeValue &oRight)
{
	if (oLeft.GetSec() > oRight.GetSec())
		return 1;
	else if (oLeft.GetSec() == oRight.GetSec() 
		&& oLeft.GetUSec() > oRight.GetUSec())
		return 1;
	else
		return 0;
}

inline int operator >= (const CTimeValue &oLeft, const CTimeValue &oRight)
{
	if (oLeft.GetSec() > oRight.GetSec())
		return 1;
	else if (oLeft.GetSec() == oRight.GetSec() 
		&& oLeft.GetUSec() >= oRight.GetUSec())
		return 1;
	else
		return 0;
}

inline int operator < (const CTimeValue &oLeft, const CTimeValue &oRight)
{
	return oLeft > oRight;
}

inline int operator <= (const CTimeValue &oLeft, const CTimeValue &oRight)
{
	return oRight >= oLeft;
}

inline int operator == (const CTimeValue &oLeft, const CTimeValue &oRight)
{
	return oLeft.GetSec() == oRight.GetSec() && 
		   oLeft.GetUSec() == oRight.GetUSec();
}

inline int operator != (const CTimeValue &oLeft, const CTimeValue &oRight)
{
	return !(oLeft == oRight);
}

inline void CTimeValue::operator += (const CTimeValue &oRight)
{
	m_lSec = GetSec() + oRight.GetSec();
	m_lUSec = GetUSec() + oRight.GetUSec();
	Normalize();
}

inline void CTimeValue::operator -= (const CTimeValue &oRight)
{
	m_lSec = GetSec() - oRight.GetSec();
	m_lUSec = GetUSec() - oRight.GetUSec();
	Normalize();
}

inline void CTimeValue::operator = (const CTimeValue &oRight)
{
	m_lSec = oRight.GetSec();
	m_lUSec = oRight.GetUSec();
	Normalize();
}

inline CTimeValue operator + 
(const CTimeValue &oLeft, const CTimeValue &oRight)
{
	return CTimeValue(oLeft.GetSec() + oRight.GetSec(), 
					  oLeft.GetUSec() + oRight.GetUSec());
}

inline CTimeValue operator - 
(const CTimeValue &oLeft, const CTimeValue &oRight)
{
	return CTimeValue(oLeft.GetSec() - oRight.GetSec(), 
					  oLeft.GetUSec() - oRight.GetUSec());
}

#endif // !TIME_VALUE_H
