#include "uox3.h"
#include "fileio.h"
#include "cGuild.h"
#include "cServerDefinitions.h"
#include "ssection.h"
#include "cSpawnRegion.h"
#include "scriptc.h"
#include "townregion.h"
#include "regions.h"

#if UOX_PLATFORM != PLATFORM_WIN32
#  include <sys/types.h> // open, stat, mmap, munmap
#  include <sys/stat.h>  // stat
#  include <fcntl.h>     // open
#  include <unistd.h>    // stat, mmap, munmap
#  include <sys/mman.h>  // mmap, mmunmap
#endif



namespace UOX
{

//
// support read-only binary mode.
//
UOXFile::UOXFile( const char* const fileName, const char * const)
:memPtr(0), bIndex(0), fileSize(0)
{
#if UOX_PLATFORM == PLATFORM_WIN32
	HANDLE hFile = ::CreateFileA(
				fileName,
				GENERIC_READ,
				FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,
				FILE_FLAG_SEQUENTIAL_SCAN,
				NULL
			);

	if (hFile == INVALID_HANDLE_VALUE)
		return;

	// Store the size of the file, it's used to construct
	//  the end iterator
	fileSize = ::GetFileSize(hFile, NULL);

	HANDLE hMap = ::CreateFileMapping(
				hFile,
				NULL,
				PAGE_READONLY,
				0, 0,
				NULL
			);

	if (hMap == NULL)
	{
		::CloseHandle(hFile);
		return;
	}

	memPtr = (char*)::MapViewOfFile(
						hMap,
						FILE_MAP_READ,
						0, 0, 0
					);

	// We hold both the file handle and the memory pointer.
	// We can close the hMap handle now because Windows holds internally
	//  a reference to it since there is a view mapped.
	::CloseHandle(hMap);

	// It seems like we can close the file handle as well (because
	//  a reference is hold by the filemap object).
	::CloseHandle(hFile);

#else
	// postfix version
        // open the file
	int fd = open(fileName,
#ifdef O_NOCTTY
				O_NOCTTY | // if stdin was closed then opening a file
								// would cause the file to become the controlling
								// terminal if the filename refers to a tty. Setting
								// O_NOCTTY inhibits this.
#endif
				O_RDONLY);

	if (fd == -1)
		return;

	// call fstat to find get information about the file just
	// opened (size and file type)
	struct stat stat_buf;
	if ((fstat(fd, &stat_buf) != 0) || !S_ISREG(stat_buf.st_mode))
	{	// if fstat returns an error or if the file isn't a
		// regular file we give up.
		close(fd);
		return;
	}

	fileSize = stat_buf.st_size;
	// perform the actual mapping
	memPtr = (char*)mmap(0, stat_buf.st_size, PROT_READ, MAP_SHARED, fd, 0);
	// it is safe to close() here. POSIX requires that the OS keeps a
	// second handle to the file while the file is mmapped.
	close(fd);
#endif
}


UOXFile::~UOXFile()
{
	if( memPtr )
	{ 
#if UOX_PLATFORM == PLATFORM_WIN32
		UnmapViewOfFile( memPtr );
#else
		munmap(memPtr, fileSize);
#endif
	}
}


void UOXFile::seek( long offset, int whence )
{
	if( whence == SEEK_SET )
	{	// seek from beginnng
		bIndex = offset;
	} else if ( whence == SEEK_CUR )
	{	// seek from current
		bIndex += offset;
	}	else if ( whence == SEEK_END )
	{	// seek from end
		bIndex = fileSize + offset;
	}
}


int UOXFile::getch(void)
{
	return *(memPtr + bIndex++);
}


//
// never used deprecated
/*
char *UOXFile::gets( char *lnBuff, int lnSize )
{
	if( fmode == 0 )
	{
		int i;

		--lnSize;
		i = 0;
		do
		{
			if( qRefill() ) 
				refill();

			while( i < lnSize && bIndex != bSize && ioBuff[(int)bIndex] != 0x0A )
			{
				if( ioBuff[(int)bIndex] == 0x0D ) 
					++bIndex;
				else 
					lnBuff[i++] = ioBuff[(int)bIndex++];
			}

			if( ioBuff[(int)bIndex] == 0x0A )
			{
				++bIndex;
				if( i != lnSize ) 
					lnBuff[i++] = 0x0A;
				break;
			}

			if( i == lnSize ) 
			{ 
				break; 
			}
		} while( bSize != 0 );

		lnBuff[i] = '\0';

		if( bSize != IOBUFFLEN && i == 0 ) 
			return NULL;

		return lnBuff;
	}
	else 
		return NULL;
}*/


//
// deprectated, never used.  Since memory-mapped file cannot be appended,
// so there could be surprised, and since we are not using it, we might 
// as well deprecated it.
//
//int UOXFile::puts( char *lnBuff )
//{
//	if( fmode == 1 && lnBuff )
//		return fwrite( lnBuff, sizeof( char ), strlen( lnBuff ), theFile );
//
//	return -1;
//}


void UOXFile::getUChar( UI08 *buff, UI32 number )
{
	memcpy( buff, memPtr+bIndex, number);
	bIndex += number;
}


void UOXFile::getChar( SI08 *buff, UI32 number )
{
	memcpy( buff, memPtr+bIndex, number);
	bIndex += number;
}


void UOXFile::getUShort( UI16 *buff, UI32 number )
{
	number *= sizeof(UI16);
	memcpy( buff, memPtr+bIndex, number);
	bIndex += number;
}


void UOXFile::getShort( SI16 *buff, UI32 number )
{
	number *= sizeof(SI16);        
	memcpy( buff, memPtr+bIndex, number);
	bIndex += number;
}


void UOXFile::getULong( UI32 *buff, UI32 number )
{
	number *= sizeof(UI32);
	memcpy( buff, memPtr+bIndex, number);
	bIndex += number;
}

void UOXFile::getLong( SI32 *buff, UI32 number )
{
	number *= sizeof(UI32);
	memcpy( buff, memPtr+bIndex, number);
	bIndex += number;
}


void UOXFile::get_versionrecord( struct versionrecord *buff, UI32 number )
{
	for( UI32 i = 0; i < number; ++i )
	{
		getLong( &buff[i].file );
		getLong( &buff[i].block );
		getLong( &buff[i].filepos );
		getLong( &buff[i].length );
		getLong( &buff[i].unknown );
	}
}



/*
** More info from Alazane & Circonian on this...
**
Index entry:
DWORD: File ID (see index below)
DWORD: Block (Item number, Gump number or whatever; like in the file)
DWORD: Position (Where to find this block in verdata.mul)
DWORD: Size (Size in Byte)
DWORD: Unknown (Perhaps some CRC for the block, most blocks in UO files got this) 

    File IDs: (* means used in current verdata)
00 - map0.mul
01 - staidx0.mul
02 - statics0.mul
03 - artidx.mul
04 - art.mul*
05 - anim.idx
06 - anim.mul
07 - soundidx.mul
08 - sound.mul
09 - texidx.mul
0A - texmaps.mul
0B - gumpidx.mul
0C - gumpart.mul*
0D - multi.i
*/

void UOXFile::get_st_multi( struct st_multi *buff, UI32 number )
{
	for( UI32 i = 0; i < number; ++i )
	{
		getUShort( &buff[i].tile );
		getShort( &buff[i].x );
		getShort( &buff[i].y );
		getChar( (SI08 *) &buff[i].z );
		getChar( (SI08 *) &buff[i].empty );
		getLong( &buff[i].visible );
	}
}


void UOXFile::get_land_st( CLand *buff, UI32 number)
{
	for( UI32 i = 0; i < number; ++i )
		buff[i].Read( this );
}


void UOXFile::get_tile_st( CTile *buff, UI32 number)
{
	for( UI32 i = 0; i < number; ++i )
		buff[i].Read( this );
}


void UOXFile::get_map_st(struct map_st *buff, UI32 number)
{
	for( UI32 i = 0; i < number; ++i)
	{
		getUShort(&buff[i].id);
		getChar(&buff[i].z);
	}
}


void UOXFile::get_st_multiidx(struct st_multiidx *buff, UI32 number )
{
	for( UI32 i = 0; i < number; ++i)
	{
		getLong(&buff[i].start);
		getLong(&buff[i].length);
		getLong(&buff[i].unknown);
	}
}


void UOXFile::get_staticrecord( struct staticrecord *buff, UI32 number )
{
	for( UI32 i = 0; i < number; ++i)
	{
		getUShort( &buff[i].itemid );
		getUChar( &buff[i].xoff );
		getUChar( &buff[i].yoff );
		getChar( &buff[i].zoff );
		SI16 extra;
		getShort( &extra );
	}
}

//o---------------------------------------------------------------------------o
//|   Function    :  LoadINIFile()
//|   Date        :  Unknown
//|   Programmer  :  UOX3 DevTeam
//o---------------------------------------------------------------------------o
//|   Purpose     :  Loads uox3.ini
//o---------------------------------------------------------------------------o
void LoadINIFile( void )
{
	// hmm, fileExists isn't a valid func... let's call our
	bool uox3test = FileExists( "uox3test.ini" );
	bool uox3     = FileExists( "uox3.ini" );
	bool uox      = FileExists( "uox.ini" );
	if( !uox )
	{
		if( uox3 ) 
		{
			Console << "NOTICE: uox3.ini is no longer needed." << myendl;
			Console << "Rewriting as uox.ini." << myendl;
			cwmWorldState->ServerData()->load( "uox3.ini" );//load this anyway, in case they don't have the other one.
			cwmWorldState->ServerData()->save();
		}
		else if( uox3test ) 
		{
			Console << "NOTICE: uox3test.ini is no longer needed." << myendl;
			Console << "Rewriting as uox.ini." << myendl;
			cwmWorldState->ServerData()->load( "uox3test.ini" );//load this anyway, in case they don't have the other one.
			cwmWorldState->ServerData()->save();
		}
	}
	else if( uox3 || uox3test )
	{
		Console << "You have both old style (uox3.ini and/or uox3test.ini) and new style (uox.ini) files." << myendl;
		Console << "We will only be reading the uox.ini file" << myendl;
	}
	cwmWorldState->ServerData()->load();
	if( !uox && !uox3 && !uox3test )
		cwmWorldState->ServerData()->save();
}

//o---------------------------------------------------------------------------o
//|   Function    :  LoadCustomTitle()
//|   Date        :  Unknown
//|   Programmer  :  UOX3 DevTeam
//o---------------------------------------------------------------------------o
//|   Purpose     :  Loads players titles (Karma, Fame, Murder, ect)
//o---------------------------------------------------------------------------o
void LoadCustomTitle( void )
{ 
	size_t titlecount = 0;
	UString tag;
	UString data;
	ScriptSection *CustomTitle = FileLookup->FindEntry( "SKILL", titles_def );
	if( CustomTitle == NULL )
		return;
	for( tag = CustomTitle->First(); !CustomTitle->AtEnd() && titlecount < ALLSKILLS; tag = CustomTitle->Next() )
	{
		data = CustomTitle->GrabData();
		cwmWorldState->title[titlecount].skill	= data;
		++titlecount;
	}
	CustomTitle = FileLookup->FindEntry( "PROWESS", titles_def );
	if( CustomTitle == NULL )
		return;
	for( tag = CustomTitle->First(); !CustomTitle->AtEnd(); tag = CustomTitle->Next() )
	{
		data = CustomTitle->GrabData();
		cwmWorldState->prowessTitles.push_back( ProwessTitle( tag.toShort(), data ) );
	}

	CustomTitle = FileLookup->FindEntry( "FAME", titles_def );
	titlecount	= 0;

	for( tag = CustomTitle->First(); !CustomTitle->AtEnd() && titlecount < ALLSKILLS; tag = CustomTitle->Next() )
	{
		data = CustomTitle->GrabData();
		cwmWorldState->title[titlecount].fame = data;
		++titlecount;
	}

	// Murder tags now scriptable in SECTION MURDER - Titles.scp - Thanks Ab - Zane
	CustomTitle = FileLookup->FindEntry( "MURDERER", titles_def );
	if( CustomTitle == NULL )
		return;
	for( tag = CustomTitle->First(); !CustomTitle->AtEnd(); tag = CustomTitle->Next() )
	{
		data = CustomTitle->GrabData();
		cwmWorldState->murdererTags.push_back( MurderPair( tag.toShort(), data ) );
	}
}

//o---------------------------------------------------------------------------o
//|	Function	-	LoadSkills()
//|	Programmer	-	UOX3 DevTeam
//o---------------------------------------------------------------------------o
//|	Purpose		-	Load skills
//o---------------------------------------------------------------------------o
void LoadSkills( void )
{
	UString sect;
	ScriptSection *SkillList = NULL;
	UString tag;
	UString data;
	UString UTag;
	for( UI08 i = 0; i <= ALLSKILLS + 3; ++i)
	{
		cwmWorldState->skill[i].strength		= 0;
		cwmWorldState->skill[i].dexterity		= 0;
		cwmWorldState->skill[i].intelligence	= 0;
		cwmWorldState->skill[i].madeword		= "made";
		sect = "SKILL " + UString::number( i );
		SkillList = FileLookup->FindEntry( sect, skills_def );
		if( SkillList != NULL )
		{
			for( tag = SkillList->First(); !SkillList->AtEnd(); tag = SkillList->Next() )
			{
				UTag = tag.upper();
				data = SkillList->GrabData();
				if( UTag == "STR" )
					cwmWorldState->skill[i].strength = data.toUShort();
				else if( UTag == "DEX" )
					cwmWorldState->skill[i].dexterity = data.toUShort();
				else if( UTag == "INT" )
					cwmWorldState->skill[i].intelligence = data.toUShort();
				else if( UTag == "SKILLPOINT" )
				{
					advance_st tempAdvance;
					data = data.simplifyWhiteSpace();
					tempAdvance.base	= data.section( " ", 0, 0 ).toUShort();
					tempAdvance.success = data.section( " ", 1, 1 ).toUShort();
					tempAdvance.failure = data.section( " ", 2, 2 ).toUShort();
					cwmWorldState->skill[i].advancement.push_back( tempAdvance );
				}
				else if( UTag == "MADEWORD" )
					cwmWorldState->skill[i].madeword = data.stripWhiteSpace();
			}
		}
	}
}

//o---------------------------------------------------------------------------o
//|	Function	-	LoadSpawnRegions()
//|	Programmer	-	UOX3 DevTeam
//o---------------------------------------------------------------------------o
//|	Purpose		-	Loads spawning regions
//o---------------------------------------------------------------------------o
void LoadSpawnRegions( void )
{
	spawnregions.resize( 0 );
	UI16 i					= 0;
	ScriptSection *toScan	= NULL;
	VECSCRIPTLIST *tScn		= FileLookup->GetFiles( spawn_def );
	if( tScn == NULL )
		return;
	for( VECSCRIPTLIST_CITERATOR lIter = tScn->begin(); lIter != tScn->end(); ++lIter )
	{
		Script *spnScp = (*lIter);
		if( spnScp == NULL )
			continue;
		for( toScan = spnScp->FirstEntry(); toScan != NULL; toScan = spnScp->NextEntry() )
		{
			if( toScan == NULL )
				continue;
			if( "REGIONSPAWN" == spnScp->EntryName().substr( 0, 11 ) ) // Is it a region spawn entry?
			{
				spawnregions.push_back( new CSpawnRegion( i ) );
				spawnregions[i]->Load( toScan );
				++i;
			}
		}
	}
}

//o---------------------------------------------------------------------------o
//|	Function	-	LoadRegions()
//|	Programmer	-	UOX3 DevTeam
//o---------------------------------------------------------------------------o
//|	Purpose		-	Load regions from regions.dfn and townregions from regions.wsc
//o---------------------------------------------------------------------------o
void LoadRegions( void )
{
	regions.resize( 0 );
	std::string regionsFile = cwmWorldState->ServerData()->Directory( CSDDP_SHARED ) + "regions.wsc";
	bool performLoad		= false;
	Script *ourRegions		= NULL;
	if( FileExists( regionsFile ) )
	{
		performLoad = true;
		ourRegions = new Script( regionsFile, NUM_DEFS, false );
	}

	UI08 i					= 0;
	ScriptSection *toScan	= NULL;
	VECSCRIPTLIST *tScn		= FileLookup->GetFiles( regions_def );
	if( tScn == NULL )
		return;

	UString regEntry;
	for( VECSCRIPTLIST_CITERATOR lIter = tScn->begin(); lIter != tScn->end(); ++lIter )
	{
		Script *regScp = (*lIter);
		if( regScp == NULL )
			continue;

		for( toScan = regScp->FirstEntry(); toScan != NULL; toScan = regScp->NextEntry() )
		{
			if( toScan == NULL )
				continue;

			regEntry = regScp->EntryName();
			if( regEntry.section( " ", 0, 0 ) == "REGION" )
			{
				i = regEntry.section( " ", 1, 1 ).toUByte();
				if( i >= regions.size() )
					regions.resize( i+1 );
				regions[i] = new CTownRegion( i );
				regions[i]->InitFromScript( toScan );
				if( performLoad )
					regions[i]->Load( ourRegions );
			}
		}
	}
	if( performLoad )
	{
		delete ourRegions;
		ourRegions = NULL;
	}
	ScriptSection *InstaLog = FileLookup->FindEntry( "INSTALOG", regions_def );
	if( InstaLog == NULL ) 
		return;
	LogoutLocationEntry toAdd;
	UString data, UTag;
	for( UString tag = InstaLog->First(); !InstaLog->AtEnd(); tag = InstaLog->Next() )
	{
		UTag = tag.upper();
		data	= InstaLog->GrabData();
		if( UTag == "X1" ) 
			toAdd.x1 = data.toShort();
		else if( UTag == "Y1" ) 
			toAdd.y1 = data.toShort();
		else if( UTag == "X2" )
			toAdd.x2 = data.toShort();
		else if( UTag == "Y2" )
		{
			toAdd.y2 = data.toShort();
			cwmWorldState->logoutLocs.push_back( toAdd );
		}
	}
}

//o---------------------------------------------------------------------------o
//|	Function	-	LoadTeleportLocations()
//|	Programmer	-	UOX3 DevTeam
//o---------------------------------------------------------------------------o
//|	Purpose		-	Load teleport locations
//o---------------------------------------------------------------------------o
void LoadTeleportLocations( void )
{
	std::string filename = cwmWorldState->ServerData()->Directory( CSDDP_SCRIPTS ) + "teleport.scp"; 
	cwmWorldState->teleLocs.resize( 0 );
	
	if( !FileExists( filename ) )
	{
		Console << myendl;
		Console.Error( 1, " Failed to open teleport data script %s", filename.c_str() );
		Console.Error( 1, " Teleport Data not found" );
		cwmWorldState->SetKeepRun( false );
		cwmWorldState->SetError( true );
		return;
	}

	Script *teleportData = new Script( filename, NUM_DEFS, false );
	if( teleportData != NULL )
	{
		UI16 tempX, tempY;
		SI08 tempZ;
		ScriptSection *teleportSect = NULL;
		UString tag, data, temp;
		for( teleportSect = teleportData->FirstEntry(); teleportSect != NULL; teleportSect = teleportData->NextEntry() )
		{
			if( teleportSect != NULL )
			{
				for( tag = teleportSect->First(); !teleportSect->AtEnd(); tag = teleportSect->Next() )
				{
					CTeleLocationEntry toAdd;
					if( tag.upper() == "ENTRY" )
					{
						tempX = 0, tempY = 0;
						tempZ = ILLEGAL_Z;
						data = teleportSect->GrabData().simplifyWhiteSpace();
						int sectCount = data.sectionCount( "," );
						if( sectCount >= 5 )
						{
							tempX	= data.section( ",", 0, 0 ).toUShort();
							tempY	= data.section( ",", 1, 1 ).toUShort();
							temp	= data.section( ",", 2, 2 ).upper();
							temp	= temp.stripWhiteSpace().upper();
							if( temp != "ALL" && temp != "A" )
								tempZ = temp.toByte();
							toAdd.SourceLocation( tempX, tempY, tempZ );

							tempX	= data.section( ",", 3, 3 ).toUShort();
							tempY	= data.section( ",", 4, 4 ).toUShort();
							tempZ	= data.section( ",", 5, 5 ).toByte();
							toAdd.TargetLocation( tempX, tempY, tempZ );

							if( sectCount >= 6 )
							{
								toAdd.SourceWorld( data.section( ",", 6, 6 ).toUByte() );
								if( sectCount >= 7 )
									toAdd.TargetWorld( data.section( ",", 7, 7 ).toUByte() );
							}
							cwmWorldState->teleLocs.push_back( toAdd );
						}
						else
							Console.Error( 2, "Insufficient parameters for teleport entry" );
					}
				}
			}
		}
		delete teleportData;
	}
}

//o---------------------------------------------------------------------------o
//|	Function	-	LoadCreatures()
//|	Programmer	-	UOX3 DevTeam
//o---------------------------------------------------------------------------o
//|	Purpose		-	Loads creatures from creatures.dfn
//o---------------------------------------------------------------------------o
void LoadCreatures( void )
{
	char sect[128];
	UString tag;
	UString data;
	UString UTag;
	for( UI16 iCounter = 0; iCounter < 2048; ++iCounter )
	{
		sprintf( sect, "CREATURE 0x%x", iCounter );
		ScriptSection *creatureData = FileLookup->FindEntry( sect, creatures_def );
		if( creatureData != NULL )
		{
			for( tag = creatureData->First(); !creatureData->AtEnd(); tag = creatureData->Next() )
			{
				if( tag.empty() )
					continue;
				data = creatureData->GrabData();
				UTag = tag.upper();
				switch( (tag.data()[0]) )
				{
					case 'A':
						if( UTag == "ANTIBLINK" )
							cwmWorldState->creatures[iCounter].AntiBlink( true );
						else if( UTag == "ANIMAL" )
							cwmWorldState->creatures[iCounter].IsAnimal( true );
						break;
					case 'B':
						if( UTag == "BASESOUND" )
							cwmWorldState->creatures[iCounter].BaseSound( data.toUShort() );
						break;
					case 'F':
						if( UTag == "FLIES" )
							cwmWorldState->creatures[iCounter].CanFly( true );
						break;
					case 'I':
						if( UTag == "ICON" )
							cwmWorldState->creatures[iCounter].Icon( data.toUShort() );
						break;
					case 'S':
						if( UTag == "SOUNDFLAG" )
							cwmWorldState->creatures[iCounter].SoundFlag( data.toUByte() );
						break;
					case 'W':
						if( UTag == "WATERCREATURE" )
							cwmWorldState->creatures[iCounter].IsWater( true );
						break;
				}
			}
		}
	}
}


void ReadWorldTagData( std::ifstream &inStream, UString &tag, UString &data )
{
	char temp[4096];
	tag = "o---o";
	data = "o---o";
	while( !inStream.eof() && !inStream.fail() )
	{
		inStream.getline( temp, 4096 );
		UString sLine( temp );
		sLine = sLine.removeComment().stripWhiteSpace();
		if( !sLine.empty() )
		{
			if( sLine != "o---o" )
			{
				if( sLine.sectionCount( "=" ) == 1 )
				{
					tag		= sLine.section( "=", 0, 0 ).stripWhiteSpace();
					data	= sLine.section( "=", 1 ).stripWhiteSpace();
					break;
				}
			}
			else
				break;
		}
	}
}

//o---------------------------------------------------------------------------o
//|	Function	-	LoadTeleportLocations()
//|	Programmer	-	UOX3 DevTeam
//o---------------------------------------------------------------------------o
//|	Purpose		-	Load teleport locations
//o---------------------------------------------------------------------------o
void LoadPlaces( void )
{
	cwmWorldState->goPlaces.clear();
	UString data, UTag, entryName;

	GoPlaces_st *toAdd		= NULL;
	ScriptSection *toScan	= NULL;
	VECSCRIPTLIST *tScn		= FileLookup->GetFiles( location_def );
	if( tScn == NULL )
		return;
	for( VECSCRIPTLIST_CITERATOR lIter = tScn->begin(); lIter != tScn->end(); ++lIter )
	{
		Script *locScp = (*lIter);
		if( locScp == NULL )
			continue;
		for( toScan = locScp->FirstEntry(); toScan != NULL; toScan = locScp->NextEntry() )
		{
			if( toScan == NULL )
				continue;
			entryName			= locScp->EntryName();
			size_t entryNum		= entryName.section( " ", 1, 1 ).toULong();
			if( entryName.section( " ", 0, 0 ).upper() == "LOCATION" && entryNum )
			{
				if( cwmWorldState->goPlaces.find( entryNum ) != cwmWorldState->goPlaces.end() )
					Console.Warning( 2, "Doubled up entry in Location.dfn (%u)", entryNum );
				toAdd = &cwmWorldState->goPlaces[entryNum];
				if( toAdd != NULL )
				{
					for( UString tag = toScan->First(); !toScan->AtEnd(); tag = toScan->Next() )
					{
						data = toScan->GrabData();
						UTag = tag.upper();
						if( UTag == "X" )
							toAdd->x = data.toShort();
						else if( UTag == "Y" )
							toAdd->y = data.toShort();
						else if( UTag == "Z" )
							toAdd->z = data.toByte();
						else if( UTag == "WORLD" )
							toAdd->worldNum = data.toUByte();
					}
				}
			}
		}
	}
}

bool FileExists( std::string filepath )
{
	std::ifstream ifsFile;
	ifsFile.open( filepath.c_str(), std::ios::in );
	if( ifsFile.is_open() )
	{
		ifsFile.close();
		return true;
	}
	return false;
}

}
