
#ifndef _JK_APPENDER_O791RES_H_
#define _JK_APPENDER_O791RES_H_

#include "JK_LockFreeQueue.h"
#include "CmdObject.h"
#include <vector>


using namespace std;


class ClientAgent;
class Appender
{
public:
	Appender();
	~Appender();


	static unsigned int AppendThread();
	static void CloseAppendFile();
	static int LoadAppendFile();
	static int ReplicateAppendFile( ClientAgent* pAgent);
	static void HandleCmdLine( char* strLine, bool bOps );

private:
	static int WriteAppendFile( void* pdata );
	static FILE* OpenAppendFile( char* mod );

private:
	static bool			m_bAppendOpen;
	static FILE*		m_fpAppendfile;
};

#endif