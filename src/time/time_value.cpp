//$Id: time_value.cpp,v 1.2 2005/09/30 02:39:45 henrylu Exp $
#include <limits.h>
#include "time_value.h"

const CTimeValue CTimeValue::s_tvZero;
const CTimeValue CTimeValue::s_tvMax(LONG_MAX, ONE_SECOND_IN_USECS - 1);

void CTimeValue::Normalize()
{
	if (m_lUSec >= ONE_SECOND_IN_USECS) 
	{
		do 
		{
			m_lSec++;
			m_lUSec -= ONE_SECOND_IN_USECS;
		}
		while (m_lUSec >= ONE_SECOND_IN_USECS);
	}
	else if (m_lUSec <= -ONE_SECOND_IN_USECS) 
	{
		do 
		{
			m_lSec--;
			m_lUSec += ONE_SECOND_IN_USECS;
		}
		while (m_lUSec <= -ONE_SECOND_IN_USECS);
	}

	if (m_lSec >= 1 && m_lUSec < 0) 
	{
		m_lSec--;
		m_lUSec += ONE_SECOND_IN_USECS;
	}
	else if (m_lSec < 0 && m_lUSec > 0) 
	{
		m_lSec++;
		m_lUSec -= ONE_SECOND_IN_USECS;
	}
}
