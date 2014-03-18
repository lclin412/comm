#ifndef _CHECK_UNIQ_PROCESS_H_
#define _CHECK_UNIQ_PROCESS_H_
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

class CheckUniqProcess
{
/*
* 根据进程名判断进程个数，这里参数是进程的名字
*/
public:
	static int CheckProcessNum(char* processName) 
	{
		if(!processName) 
			return 0;
		
		FILE *pstr;
		char szCmd[256] = {0};
		char result[256] = {0};

		snprintf(szCmd, sizeof(szCmd), "ps aux | grep -v \"grep\"| grep -c \"%s\"", processName);
		pstr = popen(szCmd, "r");
		if(!pstr) return 0;

		if(!fgets(result, sizeof(result) - 1, pstr)) 
		{
			pclose(pstr);
			return 0;
		}

		int processNum = atoi(result);

		pclose(pstr);
		return processNum;

	}


};

#endif

