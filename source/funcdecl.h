//o---------------------------------------------------------------------------o
//| funcdecl.h
//o---------------------------------------------------------------------------o
//| This header file contains all of our globally declared functions
//o---------------------------------------------------------------------------o
//| Version History
//| 2.0		giwo	10/16/2003
//|			Many functions removed, overall reorganization and updated to match
//|			the rest of UOX3.
//o---------------------------------------------------------------------------o
#ifndef __FUNCDECLS_H__
#define __FUNCDECLS_H__

namespace UOX
{

//o---------------------------------------------------------------------------o
// Range check functions
//o---------------------------------------------------------------------------o
bool	objInRange( CSocket *mSock, CBaseObject *obj, UI16 distance );
bool	objInRange( CBaseObject *a, CBaseObject *b, UI16 distance );
bool	objInOldRange( CBaseObject *a, CBaseObject *b, UI16 distance );
bool	charInRange( CChar *a, CChar *b );
UI16	getDist( CBaseObject *a, CBaseObject *b );
UI16	getOldDist( CBaseObject *a, CBaseObject *b );
UI16	getDist3D( CBaseObject *a, CBaseObject *b );
SOCKLIST	FindPlayersInVisrange( CBaseObject *myObj );
SOCKLIST	FindPlayersInOldVisrange( CBaseObject *myObj );
SOCKLIST	FindNearbyPlayers( CBaseObject *myObj, UI16 distance );
SOCKLIST	FindNearbyPlayers( CChar *mChar );
//o---------------------------------------------------------------------------o
// Multi functions
//o---------------------------------------------------------------------------o
CMultiObj *	findMulti( SI16 x, SI16 y, SI08 z, UI08 worldNumber );
CMultiObj *	findMulti( CBaseObject *i );

//o---------------------------------------------------------------------------o
// Calculation functions (socket, char, item and so forth)
//o---------------------------------------------------------------------------o
CSocket		*calcSocketObjFromChar( CChar *i );
CItem *		calcItemObjFromSer( SERIAL targSerial );
CChar *		calcCharObjFromSer( SERIAL targSerial );
CMultiObj *	calcMultiFromSer( SERIAL targSerial );
inline UI32 calcserial( UI08 a1, UI08 a2, UI08 a3, UI08 a4 )
{
	return ( (a1<<24) + (a2<<16) + (a3<<8) + a4 );
}

//o---------------------------------------------------------------------------o
// Socket stuff
//o---------------------------------------------------------------------------o
void	SendVecsAsGump( CSocket *sock, STRINGLIST& one, STRINGLIST& two, long type, SERIAL serial );
void	SendMapChange( UI08 worldNumber, CSocket *sock, bool initialLogin = false );
bool	isOnline( CChar *c );

//o---------------------------------------------------------------------------o
// Light related functions
//o---------------------------------------------------------------------------o
void	doLight( CSocket *s, UI08 level );

//o---------------------------------------------------------------------------o
// Amount related
//o---------------------------------------------------------------------------o
UI32	GetItemAmount( CChar *s, UI16 realID, UI16 realColour = 0x0000 );
UI32	DeleteItemAmount( CChar *s, UI32 amount, UI16 realID, UI16 realColour = 0x0000 );
UI32	GetBankCount( CChar *p, UI16 itemID, UI16 realColour = 0x0000 );
UI32	DeleteBankItem( CChar *p, UI32 amt, UI16 itemID, UI16 realColour = 0x0000 );

//o---------------------------------------------------------------------------o
// Region related
//o---------------------------------------------------------------------------o
CTownRegion *calcRegionFromXY( SI16 x, SI16 y, UI08 worldNumber );

//o---------------------------------------------------------------------------o
// Find functions
//o---------------------------------------------------------------------------o
CChar *			FindItemOwner( CItem *p );
CBaseObject *	FindItemOwner( CItem *i, ObjectType &objType );
CItem *			FindItemOfType( CChar *toFind, ItemTypes type );
CItem *			FindItem( CChar *toFind, UI16 itemID );

//o---------------------------------------------------------------------------o
// Reputation Stuff
//o---------------------------------------------------------------------------o
void	Karma( CChar *nCharID, CChar *nKilledID, SI16 nKarma );
void	Fame( CChar *nCharID, SI16 nFame );
void	setcharflag( CChar *c );

//o---------------------------------------------------------------------------o
// Combat Stuff
//o---------------------------------------------------------------------------o
void	criminal( CChar *c );
void	callGuards( CChar *mChar );
void	callGuards( CChar *mChar, CChar *targChar );

//o---------------------------------------------------------------------------o
// Time Functions
//o---------------------------------------------------------------------------o
UI32	getclock( void );
inline char *	RealTime( char *time_str )
{
	struct tm *curtime;
	time_t bintime;
	time( &bintime );
	curtime = localtime( &bintime );
	strftime( time_str, 256, "%B %d %I:%M:%S %p", curtime );
	return time_str;
}
#if UOX_PLATFORM != PLATFORM_WIN32
	inline void StartMilliTimer( UI32 &Seconds, UI32 &Milliseconds )
	{
		struct timeval t;
		gettimeofday( &t, NULL );
		Seconds			= t.tv_sec;
		Milliseconds	= t.tv_usec / 1000;
	}
	inline UI32 CheckMilliTimer( UI32 &Seconds, UI32 &Milliseconds ) 
	{ 
		struct timeval t; 
		gettimeofday( &t, NULL ); 
		return ( 1000 * ( t.tv_sec - Seconds ) + ( (t.tv_usec/1000) - Milliseconds ) ); 
	}
	inline void UOXSleep( int toSleep )
	{
		usleep( toSleep * 1000 );
	}
#else
	inline void StartMilliTimer( UI32 &Seconds, UI32 &Milliseconds ) 
	{ 
		struct timeb t; 
		ftime( &t ); 
		Seconds = t.time; 
		Milliseconds = t.millitm; 
	};
	inline UI32 CheckMilliTimer( UI32 &Seconds, UI32 &Milliseconds ) 
	{ 
		struct timeb t; 
		ftime( &t ); 
		return ( 1000 * ( t.time - Seconds ) + ( t.millitm - Milliseconds ) ); 
	};
	inline void UOXSleep( int toSleep )
	{
		Sleep( toSleep );
	}
#endif

//o---------------------------------------------------------------------------o
// Misc Functions
//o---------------------------------------------------------------------------o
bool	FileExists( std::string filepath );
void	DismountCreature( CChar *s );
size_t	getTileName( CItem *i, std::string& itemname );
bool	LineOfSight( CSocket *s, CChar *mChar, SI16 x2, SI16 y2, SI08 z2, int checkfor );
void	Shutdown( SI32 retCode );
void	useDoor( CSocket *s, CItem *item );
void	doDeathStuff( CChar *i );
void	NpcResurrectTarget( CChar *s );

inline bool ValidateObject( CBaseObject *toValidate )
{
	bool rvalue = true;
	try
	{
		if( toValidate == NULL )
			rvalue = false;
		else if( toValidate->isDeleted() )
			rvalue = false;
	} 
	catch( ... )
	{
		rvalue = false;
		Console.Error( 1, "Invalid Object found: 0x%X", (UI32)toValidate ); 
	}
	return rvalue;
}

template< class T >
inline T RandomNum( T nLowNum, T nHighNum )
{
	if( nHighNum - nLowNum + 1 )
		return static_cast<T>((rand() % ( nHighNum - nLowNum + 1 )) + nLowNum );
	else
		return nLowNum;
}
template< class T >
inline T HalfRandomNum( T HighRange )
{
	T halfSize = static_cast< T >(HighRange / 2);
	return RandomNum( halfSize, HighRange );
}

}

#endif

