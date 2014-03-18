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

	std::string strMsg;

	iResult = msgQ.GetOne(strMsg,MSG_TYPE);

	if(iResult<0)
	{
		std::cerr<<"Msgq GetOne Failed,ErrMsg:"<<msgQ.GetLastErrMsg()<<",Result:"<<iResult<<std::endl;
		return iResult;
	}

	std::cout<<"Server Get Msg Finished,Msg:"<<strMsg<<std::endl;
}
