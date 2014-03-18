
#ifndef TIMER_HEAP_QUEUE_H
#define TIMER_HEAP_QUEUE_H

#include "time_value.h"
#include "reactor_handler.h"

const uint32_t DEFAULT_QUEUE_LEN = 1 + 64;

class CTimerHeapQueue  
{
protected:
	struct CNode
	{
		CNode(ITimerHandler *pTimerHandler = NULL, int iTimerID = 0) 
			: m_pTimerHandler(pTimerHandler)
			, m_iTimerID(iTimerID)
			, m_dwCount(0) 
			, bEnable(true)
		{ }

		ITimerHandler *m_pTimerHandler;
		int m_iTimerID;
		
		CTimeValue m_tvExpired;		// TimeValue for first check
		CTimeValue m_tvInterval;	// Time check interval
		unsigned int m_dwCount;		// Counter for auto re-schedule
		bool bEnable;
	};

public:
	CTimerHeapQueue();
	virtual ~CTimerHeapQueue();

public:
	int ScheduleTimer(
		ITimerHandler *pTimerHandler, 
		int iTimerID, 
		const CTimeValue &oInterval,
		unsigned int dwCount);
	
	int CancelTimer(int iTimerID);

	/*
	 * return the number of timers expired in the queue.
	 */ 
	int CheckExpire(CTimeValue *oRemainTime);
	
protected:
	int Enqueue_i(CNode& oNode);
	int RemoveNode_i(int iTimerID);
	void Clean_i();
	CNode* GetTop_i();
	CNode* CheckExpire_i(CTimeValue& rtvCurrent);

private:
	uint32_t m_nHeapLen;
	CNode** m_pNodeHeap;
	CTimeValue m_tvCurrent;
	uint32_t m_l_nChild;
	uint32_t m_l_nParent;
	int m_iExpiredCnt;
	CNode* m_l_pNode;
	CNode* m_l_pNode2;
};

#endif // !TIMER_HEAP_QUEUE_H
