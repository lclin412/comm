
#include <assert.h>
#include <string.h>
#include "timer_heap_queue.h"

const int TOP_INDEX = 1;
const int TOP_CHILD_INDEX = 2;

//////////////////////////
// class CTimerHeapQueue
CTimerHeapQueue::CTimerHeapQueue()
: m_nHeapLen(0)
, m_l_nChild(0)
, m_l_nParent(0)
, m_iExpiredCnt(0)
, m_l_pNode(NULL)
, m_l_pNode2(NULL)
{
	m_pNodeHeap = new CNode*[DEFAULT_QUEUE_LEN];
	assert(m_pNodeHeap);
	memset(m_pNodeHeap, 0, DEFAULT_QUEUE_LEN * sizeof(CNode*));
}

CTimerHeapQueue::~CTimerHeapQueue()
{
	Clean_i();
	delete[] m_pNodeHeap;
}

int CTimerHeapQueue::Enqueue_i(CNode& oNode)
{
	// Set Sentinel
	m_pNodeHeap[0] = &oNode; 

	// put the new node at the end
	++m_nHeapLen;
	if(m_nHeapLen >= DEFAULT_QUEUE_LEN)
		return -1;

	m_pNodeHeap[m_nHeapLen] = &oNode;

	// Now push the task up in the heap until it has reached its place
	m_l_nChild = m_nHeapLen;
	m_l_nParent = m_l_nChild / 2;

	while (m_pNodeHeap[m_l_nParent]->m_tvExpired > oNode.m_tvExpired)
	{
		m_pNodeHeap[m_l_nChild] = m_pNodeHeap[m_l_nParent];
		m_l_nChild = m_l_nParent;
		m_l_nParent = m_l_nChild / 2;
	}

	// This is the correct place for the new task
	m_pNodeHeap[m_l_nChild] = &oNode;
	m_pNodeHeap[0] = NULL;		// clear sentinel

	return 0;
}

void CTimerHeapQueue::Clean_i()
{
	for(uint32_t i = TOP_INDEX; i <= m_nHeapLen; ++i)
	{
		assert(m_pNodeHeap[i]);
		delete m_pNodeHeap[i];
		m_pNodeHeap[i] = NULL;
	}
}

CTimerHeapQueue::CNode* CTimerHeapQueue::GetTop_i()
{
	if (0 == m_nHeapLen)
	{
		return NULL;
	}

	return m_pNodeHeap[TOP_INDEX];
}

CTimerHeapQueue::CNode* CTimerHeapQueue::CheckExpire_i(CTimeValue& rtvCurrent)
{
	if(0 == m_nHeapLen)
		return NULL;

	m_l_pNode = GetTop_i();
	
	if (m_l_pNode == NULL)
	{
		return NULL;
	}

	// The time to wait until the m_l_pNode should be served
	// TODO to check if current time is too larger than expired
	if(m_l_pNode->m_tvExpired > rtvCurrent)
	{
		return NULL;
	}

	// Reconstruct the heap
	CNode* l_pLastNode = m_pNodeHeap[m_nHeapLen];
	m_pNodeHeap[m_nHeapLen] = NULL;
	--m_nHeapLen;

	// Drop LastNode at the beginning and move it down the heap
	m_l_nParent = TOP_INDEX;
	m_l_nChild = TOP_CHILD_INDEX;
	m_pNodeHeap[TOP_INDEX] = l_pLastNode; // Previous m_pNodeHeap[TOP_INDEX] in m_l_pNode NOW, Requeue the heap
	
	while(m_l_nChild <= m_nHeapLen)
	{
		if (m_l_nChild < m_nHeapLen)
		{
			if (m_pNodeHeap[m_l_nChild]->m_tvExpired > m_pNodeHeap[m_l_nChild + 1]->m_tvExpired)
			{
				++m_l_nChild;
			}
		}
		
		if (l_pLastNode->m_tvExpired <= m_pNodeHeap[m_l_nChild]->m_tvExpired)
			break;		// found the correct place (the parent) - done
		
		m_pNodeHeap[m_l_nParent] = m_pNodeHeap[m_l_nChild];
		m_l_nParent = m_l_nChild;
		m_l_nChild = m_l_nParent * 2;
	}
	
	// this is the correct new place for the lastTask
	m_pNodeHeap[m_l_nParent] = l_pLastNode;
	
	// return the node
	return m_l_pNode;
}

int CTimerHeapQueue::RemoveNode_i(int iTimerID)
{
	m_l_pNode = NULL;
	for(uint32_t i = TOP_INDEX; i <= m_nHeapLen; ++i)
	{
		m_l_pNode = m_pNodeHeap[i];
		assert(m_l_pNode);
		
		if(m_l_pNode->m_iTimerID == iTimerID)
		{
			m_l_pNode->bEnable = false;
			return 0;
		}
	}

	// If User call CancalTimer in OnTimer(...), we need this !
	if(m_l_pNode2->m_iTimerID == iTimerID)
	{
		m_l_pNode2->bEnable = false;
		return 0;
	}

	return -1;
}

int CTimerHeapQueue::ScheduleTimer(
	ITimerHandler *pTimerHandler, 
	int iTimerID, 
	const CTimeValue &oInterval,
	unsigned int dwCount)
{
	assert(pTimerHandler);
	assert(oInterval > CTimeValue::s_tvZero || dwCount == 1);
	
	m_l_pNode = new CNode(pTimerHandler, iTimerID);
	m_l_pNode->m_tvInterval = oInterval;
	m_l_pNode->m_tvExpired = CTimeValue::GetTimeOfDay() + m_l_pNode->m_tvInterval;

	if (dwCount > 0)
		m_l_pNode->m_dwCount = dwCount;
	else
		m_l_pNode->m_dwCount = (unsigned int)-1;
	
	if(Enqueue_i(*m_l_pNode) != 0)
		return -1;

	return 0; 
}

int CTimerHeapQueue::CancelTimer(int iTimerID)
{
	return RemoveNode_i(iTimerID);
}

int CTimerHeapQueue::CheckExpire(CTimeValue *oRemainTime)
{
	m_l_pNode2 = NULL;
	m_iExpiredCnt = 0;
	CTimeValue tvCurrent = CTimeValue::GetTimeOfDay();

	while((m_l_pNode2 = CheckExpire_i(tvCurrent)) != NULL) 
	{
		if(!m_l_pNode2->bEnable || (m_l_pNode2->m_dwCount < 0))
		{
			delete m_l_pNode2;
			continue;
		}
			
		assert(m_l_pNode2->m_pTimerHandler);
		m_l_pNode2->m_pTimerHandler->OnTimer(m_l_pNode2->m_iTimerID);
		m_iExpiredCnt++;

		// Do Re-Schedule if possible
		if(m_l_pNode2->m_dwCount > 0)
		{
			m_l_pNode2->m_tvExpired = tvCurrent + m_l_pNode2->m_tvInterval;
			--m_l_pNode2->m_dwCount;
			
			if(Enqueue_i(*m_l_pNode2) != 0)
				return -1;
		}
	}

	return m_iExpiredCnt;
}
