#ifndef _STRING_HELP_H_
#define _STRING_HELP_H_
// =====================================================================================
// 
//       Filename:  string_help.h
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  2014-02-04 06:22:21 CST
//       Revision:  none
// 
// 
// =====================================================================================

//公用方法
inline int SplitStringIntoVector(const char * sContent, 
											const char * sDivider, 
											std::vector<std::string> &vecStr)
{
	char * sNewContent = new char [strlen(sContent)+1];
	snprintf(sNewContent,strlen(sContent)+1,"%s",sContent);

	char * pStart = sNewContent;
	char * pEnd   = sNewContent;

	std::string strContent;

	pEnd = strstr(sNewContent,sDivider);
	if(pEnd == NULL && strlen(sNewContent)>0){
		strContent = pStart; //get the last one;
		vecStr.push_back(strContent);
	}
	

	while(pEnd){
		*pEnd = '\0';
		strContent = pStart;
		vecStr.push_back(strContent);
		
		pStart = pEnd+strlen(sDivider);
		if((*pStart) == '\0'){
			break;
		}
		
		pEnd = strstr(pStart,sDivider);
		
		if(pEnd == NULL){
			strContent = pStart; //get the last one;
			vecStr.push_back(strContent);
		}
		
	}

	delete [] sNewContent;

	return vecStr.size();
}


#endif
