#include "MsgQueue.h"
#include <iostream>

#define MSG_KEY 11021
#define MSG_TYPE 11011

int main(int argc, char**argv)
{
	MsgQueue msgQ;

	int iResult = msgQ.InitQueue(MSG_KEY);

	if(iResult)
	{
		std::cerr<<"Msgq Init Failed,ErrMsg:"<<msgQ.GetLastErrMsg()<<std::endl;
		return iResult;
	}

	std::string strMsg="www.google.hk.com";

	iResult = msgQ.PutOne(strMsg,MSG_TYPE);

	if(iResult)
	{
		std::cerr<<"Msgq PutOne Failed,ErrMsg:"<<msgQ.GetLastErrMsg()<<std::endl;
		return iResult;
	}

	std::cout<<"Client Put Msg Finished!!"<<std::endl;
}
