#ifndef __SpawnRegion__
#define __SpawnRegion__

namespace UOX
{

class CSpawnRegion	//Regionspawns
{
private:
	std::string name;			// Any Name to show up when this region is spawned [512]

	STRINGLIST	sNpcs;				// Individual Npcs
	STRINGLIST 	sItems;				// Individual Items

	UI16		regionnum;			// Region Number

// These two will replace maxspawn
	size_t		maxcspawn;			// Maximum amount of characters to spawn
	size_t		maxispawn;			// Maximum amount of items to spawn

// These two will replace current
	SI32		curcspawn;			// Current amount of spawned characters
	SI32		curispawn;			// Current amount of spawned items

	UI08		mintime;			// Minimum spawn time
	UI08		maxtime;			// Maximum spawn time
	TIMERVAL	nexttime;			// Nextspawn time for this region

// Box values
	SI16		x1;					// Top left X
	SI16		x2;					// Bottom right x
	SI16		y1;					// Top left y
	SI16		y2;					// Bottom right y
	
	SI08		z;					// Z position of spawned items

	UI16		call;				// # of times that an NPC or Item is spawned from a list
	UI08		worldNumber;		// which world are we spawning in?

	CDataList< CChar * >	spawnedChars;
	CDataList< CItem * >	spawnedItems;

public:
				CSpawnRegion( UI16 spawnregion );
				~CSpawnRegion();

	void		Load( ScriptSection *toScan );
	void		doRegionSpawn( UI16& itemsSpawned, UI16& npcsSpawned );

	const std::string	GetName( void ) const;
	UI16		GetRegionNum( void ) const;
	size_t		GetMaxSpawn( void ) const;
	size_t		GetMaxCharSpawn( void ) const;
	size_t		GetMaxItemSpawn( void ) const;
	SI32		GetCurrent( void ) const;
	SI32		GetCurrentCharAmt( void ) const;
	SI32		GetCurrentItemAmt( void ) const;
	UI08		GetMinTime( void ) const;
	UI08		GetMaxTime( void ) const;
	TIMERVAL	GetNextTime( void ) const;
	SI16		GetX1( void ) const;
	SI16		GetY1( void ) const;
	SI16		GetX2( void ) const;
	SI16		GetY2( void ) const;
	SI08		GetZ( void ) const;

	void		SetName( const std::string newName );
	void		SetRegionNum( UI16 newVal );
	void		SetMaxCharSpawn( size_t newVal );
	void		SetMaxItemSpawn( size_t newVal );
	void		IncCurrentCharAmt( SI16 incAmt = 1 );
	void		IncCurrentItemAmt( SI16 incAmt = 1 );
	void		SetMinTime( UI08 newVal );
	void		SetMaxTime( UI08 newVal );
	void		SetNextTime( TIMERVAL newVal );
	void		SetX1( SI16 newVal );
	void		SetY1( SI16 newVal );
	void		SetX2( SI16 newVal );
	void		SetY2( SI16 newVal );
	void		SetZ( SI08 newVal );

	UI08		WorldNumber( void ) const;
	void		WorldNumber( UI08 newVal );

	void		checkSpawned();
	void		deleteSpawnedChar( CChar *toDelete );
	void		deleteSpawnedItem( CItem *toDelete );

	CDataList< CItem * > *	GetSpawnedItemsList( void );
	CDataList< CChar * > *	GetSpawnedCharsList( void );
private:
	CChar *		RegionSpawnChar( void );
	CItem *		RegionSpawnItem( void );

	bool		FindSpotToSpawn( CBaseObject *mObj );

	void		LoadNPCList( std::string npcList );
	void		LoadItemList( std::string itemList );
};

extern std::vector< CSpawnRegion * >	spawnregions;	// Zippy

}

#endif



