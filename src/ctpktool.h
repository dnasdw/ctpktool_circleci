#ifndef CTPKTOOL_H_
#define CTPKTOOL_H_

#include <sdw.h>

class CCtpkTool
{
public:
	enum EParseOptionReturn
	{
		kParseOptionReturnSuccess,
		kParseOptionReturnIllegalOption,
		kParseOptionReturnNoArgument,
		kParseOptionReturnOptionConflict
	};
	enum EAction
	{
		kActionNone,
		kActionExport,
		kActionImport,
		kActionHelp
	};
	struct SOption
	{
		const UChar* Name;
		int Key;
		const UChar* Doc;
	};
	CCtpkTool();
	~CCtpkTool();
	int ParseOptions(int a_nArgc, UChar* a_pArgv[]);
	int CheckOptions();
	int Help();
	int Action();
	static SOption s_Option[];
private:
	EParseOptionReturn parseOptions(const UChar* a_pName, int& a_nIndex, int a_nArgc, UChar* a_pArgv[]);
	EParseOptionReturn parseOptions(int a_nKey, int& a_nIndex, int a_nArgc, UChar* a_pArgv[]);
	bool exportFile();
	bool importFile();
	EAction m_eAction;
	UString m_sFileName;
	UString m_sDirName;
	bool m_bVerbose;
};

#endif	// CTPKTOOL_H_
