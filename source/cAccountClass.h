//o--------------------------------------------------------------------------o
//|	File					-	cAccountClass.h
//|	Date					-	12/6/2002 4:33:44 AM
//|	Developers		-	EviLDeD
//|	Organization	-	UOX3 DevTeam
//o--------------------------------------------------------------------------o
//|	Description		-	Header file for the cAccountClass class. See xRFC0004.txt
//|									for more details.
//o--------------------------------------------------------------------------o
//| Modifications	-	
//o--------------------------------------------------------------------------o
#ifndef __CACCOUNTCLASS_H__
#define __CACCOUNTCLASS_H__

#if defined(_MSC_VER)
#pragma warning(disable : 4786 )
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#endif

#include <iosfwd>

#if UOX_PLATFORM != PLATFORM_WIN32
	#include <dirent.h>
	//#define strnicmp(a,b,c) strncasecmp(a,b,c)
	#define _stat stat
	#define _mkdir mkdir
	#define _rmdir rmdir
#else
	#include <direct.h>
	#define _mkdir(s1,s2) _mkdir(s1)
#endif

namespace UOX
{

/* Enums */
enum __ACCOUNTBBLOCK_FLAGS__
{
	AB_USERNAME=1,
	AB_PASSWORD=2,
	AB_FLAGS=4,
	AB_PATH=8,
	AB_TIMEBAN=16,
	AB_CONTACT=32,
	AB_CHARACTER1=64,
	AB_CHARACTER2=128,
	AB_CHARACTER3=256,
	AB_CHARACTER4=512,
	AB_CHARACTER5=1024,
	AB_CHARACTER6=2048,
	AB_ALL=4063
};
//
enum __ACCOUNTBLOCK_ERRORS__
{
	AB_INVALID_ID = 0xFFFF
};
//
enum __ACCOUNTBLOCK_FLAGS__
{
	AB_FLAGS_NONE=0x0000,
	AB_FLAGS_BANNED=0x0001,
	AB_FLAGS_SUSPENDED=0x0002,
	AB_FLAGS_PUBLIC=0x0004,
	AB_FLAGS_ONLINE=0x0008,
	AB_FLAGS_CHARACTER1=0x0010,
	AB_FLAGS_CHARACTER2=0x0020,
	AB_FLAGS_CHARACTER3=0x0040,
	AB_FLAGS_CHARACTER4=0x0080,
	AB_FLAGS_CHARACTER5=0x0100,
	AB_FLAGS_CHARACTER6=0x0200,
	AB_FLAGS_UNUSED8=0x0400,
	AB_FLAGS_UNUSED9=0x0800,
	AB_FLAGS_UNUSED10=0x1000,
	AB_FLAGS_SEER=0x2000,
	AB_FLAGS_COUNSELOR=0x4000,
	AB_FLAGS_GM=0x8000,
	AB_FLAGS_ALL=0xFFFF
};
//

//o--------------------------------------------------------------------------o
//|	Class/Struct	-	typedef struct __ACCOUNTSADM_BLOCK__
//|	Date					-	12/6/2002 5:46:10 AM
//|	Developers		-	EviLDeD
//|	Organization	-	UOX3 DevTeam
//|	Status				-	Currently under development
//o--------------------------------------------------------------------------o
//|	Description		-	All the data has been moved to once again stored in the
//|									accounts.adm file. So changes to this typedef were needed
//|									to reflect the members being moved back, and the inclusion
//|									of PATH, and FLAGS to cover the storage locations.
//|									
//|									sUsername: username for this account
//|									sPassword: password for this account
//|									aPath    : path to where the username.uad file will be stored
//|									sContact : typically the email address for this account
//|									wAccountIndex: which block is this accounts.
//|									wFlags   : flags controlling the allowed accounts functions
//|									dwLastIP : ip address of the last connection from this account
//|									dwCharacters[5]: straight ID number for this account
//|									lpCharacters[5]: 
//o--------------------------------------------------------------------------o
//| Modifications	-	
//o--------------------------------------------------------------------------o
typedef struct __ACCOUNTSADM_BLOCK__
{
#if _NOACTCOPY_
private:
	__ACCOUNTSADM_BLOCK__::__ACCOUNTSADM_BLOCK__( const __ACCOUNTSADM_BLOCK__& );
	__ACCOUNTSADM_BLOCK__& __ACCOUNTSADM_BLOCK__::operator=( const __ACCOUNTSADM_BLOCK__& );
#endif
public:
	__ACCOUNTSADM_BLOCK__( void ) : sUsername( "" ), sPassword( "" ), sPath( "" ), sContact( "" ),
		wAccountIndex( 0xFFFF ), wFlags( 0x0000 ), wTimeBan( 0x0000 ), dwInGame( INVALIDSERIAL ),
		dwLastIP( 0x00000000 ), bChanged( false )
	{
		for( int i = 0; i < 6; ++i )
		{
			dwCharacters[i]=0xFFFFFFFF;
			lpCharacters[i]=NULL;
		}
	};
	void reset(void)
	{
		sUsername="";
		sPassword="";
		sContact="";
		sPath="";
		wAccountIndex=0xFFFF;
		wFlags=0x0000;
		wTimeBan=0x0000;
		dwInGame=0xFFFFFFFF;
		dwLastIP=0x00000000;
		bChanged = false;
		for( int i = 0; i < 6; ++i )
		{
			dwCharacters[i]=0xFFFFFFFF;
			lpCharacters[i]=NULL;
		}
	};
	std::string sUsername;
	std::string sPassword;
	std::string sPath;
	std::string sContact;
	UI16 wAccountIndex;
	UI16 wFlags;
	UI16 wTimeBan;
	UI32 dwInGame;
	UI32 dwLastIP;
	bool bChanged;
	UI32 dwCharacters[6];
	CChar	*lpCharacters[6];
} ACCOUNTSBLOCK,*LPACCOUNTSBLOCK;

// Class typdefs to help simplify the use of map STL
typedef std::map<std::string,ACCOUNTSBLOCK> MAPUSERNAME;
typedef std::map<UI16,ACCOUNTSBLOCK> MAPUSERNAMEID;
typedef std::map<std::string,ACCOUNTSBLOCK>::iterator MAPUSERNAME_ITERATOR;
typedef std::map<std::string,ACCOUNTSBLOCK>::const_iterator MAPUSERNAME_CITERATOR;
typedef std::map<UI16,ACCOUNTSBLOCK>::iterator MAPUSERNAMEID_ITERATOR;
typedef std::map<UI16,ACCOUNTSBLOCK>::const_iterator MAPUSERNAMEID_CITERATOR;
//o--------------------------------------------------------------------------o
//|	Class/Struct	-	class cAccountClass
//|	Date			-	12/6/2002 5:46:02 AM
//|	Developers		-	EviLDeD
//|	Organization	-	UOX3 DevTeam
//|	Status			-	Currently under development
//o--------------------------------------------------------------------------o
//|	Description		-	
//o--------------------------------------------------------------------------o
//| Modifications	-	
//o--------------------------------------------------------------------------o
class cAccountClass  
{
public:
	// Construction/Destruction
	cAccountClass();
	cAccountClass(std::string sAccountsPath);
	~cAccountClass();
	// Operator overloads
	cAccountClass& operator++();
	cAccountClass& operator--(int);
	//cAccountClass& operator+=(int);
	//cAccountClass& operator>>(ACCOUNTSBLOCK& actbRef);
	// Member Functions
	UI16	CreateAccountSystem(void);
	UI16	ImportAccounts(void);
	UI16	AddAccount(std::string sUsername, std::string sPassword, std::string sContact="NONE", UI16 wAttributes=0x0000);
	bool	DelAccount(std::string sUsername);
	bool	DelAccount(UI16 wAccountID);
	bool	ModAccount(std::string sUsername,UI32 dwFlags,ACCOUNTSBLOCK &actbBlock);
	bool	ModAccount(UI16 wAccountID,UI32 dwFlags,ACCOUNTSBLOCK &actbBlock);
	bool	SetPath(std::string sPath);
	std::string GetPath(void);
	UI16	Save(bool bForceLoad=false);
	UI16	Load(void);
	void	Reset(ACCOUNTSBLOCK *actbValue);
	size_t	size(void);
	bool	clear(void);
	bool	isUser(std::string sUsername);
	bool	AddCharacter(UI16 wAccountID, CChar *lpObject);
	bool	AddCharacter(UI16 wAccountID,UI32 dwCharacterID, CChar *lpObject);
	bool	DelCharacter(UI16 wAccountID, int nSlot);
	bool	TransCharacter(UI16 wSAccountID,UI16 wSSlot,UI16 wDAccountID);
	bool	GetAccountByName(std::string sUsername,ACCOUNTSBLOCK& actbBlock);
	bool	GetAccountByID(UI16 wAccountID,ACCOUNTSBLOCK& actbBlock);
	MAPUSERNAMEID_ITERATOR& begin(void);
	MAPUSERNAMEID_ITERATOR& end(void);
	MAPUSERNAMEID_ITERATOR& last(void);
	// Member variables
	UI16 m_wHighestAccount;
	std::string m_sAccountsDirectory;
	MAPUSERNAME m_mapUsernameMap;
	MAPUSERNAMEID m_mapUsernameIDMap;
	MAPUSERNAMEID_ITERATOR I;
private:
	// Member Functions
	void WriteAccountsHeader(std::fstream &fsOut);
	void WriteAccessHeader(std::fstream &fsOut);
	void WriteOrphanHeader(std::fstream &fsOut);
	void WriteUADHeader(std::fstream &fsOut,ACCOUNTSBLOCK& actbTemp);
	void WriteImportHeader(std::fstream &fsOut);
};

extern cAccountClass *Accounts;

}

#endif // __CACCOUNTCLASS_H__


