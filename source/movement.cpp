//------------------------------------------------------------------------
//  walking.cpp
//
//------------------------------------------------------------------------
//  This File is part of UOX3
//  Ultima Offline eXperiment III
//  UO Server Emulation Program
//  
//  Copyright 1997 - 2001 by Marcus Rating (Cironian)
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//  
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//	
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//   
//------------------------------------------------------------------------
// Thyme 2000.09.21
// This is my attempt to rewriting the walking code. I'm going to take the code and documentation
// that others before me have used and incorporate my little (big?) fixes and comments.

// Many thanks to all of the previous contributors, and I hope these changes help out.
// Lord Binary
// Morrolan
// Anti-Christ
// fur            : 1999.10.27 - rewrite of walking.cpp with documentation!
//                : 1999.10.27 - ripped apart walking into smaller functions
// Tauriel        : 1999.03.06 - For all of the region stuff
// knoxos         : 2000.08.?? - For finally making use of the flags, and height blocking

// TODO:
// Modify path struct to use only directions, not store x,y
// Add running support in PathFind
// Modify flying

// An explination of each function precedes it in the code. Hope I can provide ample enough
// information to help out the future bug fixers. I will try to explain EVERY bit of logic
// behind what I've done to make things easier.

#include "uox3.h"
#include "movement.h"
#include "weight.h"
#include "cGuild.h"
#include "combat.h"
#include "msgboard.h"
#include "cRaces.h"
#include "cMagic.h"
#include "trigger.h"
#include "cScript.h"
#include "cEffects.h"
#include "CPacketSend.h"
#include "regions.h"

namespace UOX
{

cMovement *Movement;

// These are defines that I'll use. I have a history of working with properties, so that's why
// I'm using custom definitions here versus what may be defined in the other includes.
// NOTE: P = Property, P_C = Property for Characters, P_PF = Property for Pathfinding
//       P_M = Property for Movement

// These are the debugging defines

// These are the errors that we should want to see. RARELY results in any console spam unless
// someone is trying to use an exploit or is sending invalid data directly to the server.

#define DEBUG_WALKING		0
#define DEBUG_NPCWALK		0
#define DEBUG_PATHFIND		0

#undef DBGFILE
#define DBGFILE "movement.cpp"

#define XYMAX					256		// Maximum items UOX can handle on one X/Y square
#define MAX_ITEM_Z_INFLUENCE	10		// Any item above this height is discarded as being too far away to effect the char
#define MAX_Z_LEVITATE			10		// Maximum total height to reach a tile marked as 'LEVITATABLE'
										// Items with a mark as climbable have no height limit


inline SI08 higher( SI08 a, SI08 b )
{
	if( a < b )
		return b;
	else
		return a;
}

inline UI08 turn_clock_wise( UI08 dir )
{
	UI08 t = ((dir & 0x07 ) + 1) % 8;
	return (UI08)(dir & 0x80) ? ( t | 0x80) : t;
}

inline UI08 turn_counter_clock_wise( UI08 dir )
{
	UI08 t = (dir - 1) & 7;
	return (UI08)(dir & 0x80) ? ( t | 0x80) : t;
}


//o-------------------------------------------------------------o
//| Function : calcTileHeight
//| Author   : knoxos
//o-------------------------------------------------------------o
//| Description : 
//|   Out of some strange reason the tile height seems
//|   to be an absolute value of a two's complement of 
//|   the last four bit. Don't know if it has a special
//|   meaning if the tile is height is "negative"
//|
//|   (stairs in despise blocking bug)
//| Arguments :
//|   int h   orignial height as safed in mul file
//|
//| Return code:
//|   The absoulte value of the two's complement if the
//|   the value was "negative"
//o-------------------------------------------------------------o

inline SI08 calcTileHeight( SI08 h )
{
  ///return ((h & 0x8) ? (((h & 0xF) ^ 0xF) + 1) : h & 0xF);
	//return (h & 0x7);
	//return ((h & 0x8) ? (((h & 0xF) ^ 0xF) + 1) : h & 0xF);
	return (SI08)((h & 0x8) ? ((h & 0xF) >> 1) : h & 0xF);
} 

UI08 FlagColour( CChar *a, CChar *b )
{
	if( !ValidateObject( a ) )
		return 3;
	GUILDRELATION guild;
	SI08 race;
	if( ValidateObject( b ) )
	{
	    guild = GuildSys->Compare( a, b );
		race = Races->Compare( a, b );
	}
	else
	{
		guild = GR_UNKNOWN;
		race = 0;
	}
	if( a->GetKills() > cwmWorldState->ServerData()->RepMaxKills() ) 
		return 6;
	else if( race != 0 || guild != GR_UNKNOWN )
	{
		if( guild == GR_ALLY || guild == GR_SAME || race > 0 )//Same guild (Green)
			return 2;
		else if( guild == GR_WAR || race < 0 ) // Enemy guild.. set to orange
			return 5;
	}

	if( a->IsMurderer() )
		return 6;		// If a bad, show as red.
	else if( a->IsInnocent() )
		return 1;		// If a good, show as blue.
	else 
		return 3;		// grey
}

/*
** Walking() This function is called whenever we get a message from the client
** to walk/run somewhere.   It is also called by the NPC movement functions in this
** class to make the NPCs move.  The arguments are fairly fixed because we don't
** have a lot of control about what the client gives us.
**
** CHARACTER s - Obviously the character index of the character trying to move.
**
** dir - Which direction the character is trying to move. The first nibble holds
** the cardinal direction.      If the bit 0x80 is set, it means the character is
** running instead of walking.  
**              0: // North
**              1: // Northeast
**              2: // East
**              3: // Southeast
**              4: // South
**              5: // Southwest
**              6: // West
**              7: // Northwest
**
** sequence - This is what point in the walking sequence we are at, this seems to
**            roll over once it hits 256
**
*/

//o---------------------------------------------------------------------------o
//|	Function	-	void teleporters( CChar *s )
//|	Programmer	-	Unknown
//o---------------------------------------------------------------------------o
//|	Purpose		-	If character is on a teleport location, teleport him
//o---------------------------------------------------------------------------o
void HandleTeleporters( CChar *s )
{
	if( !ValidateObject( s ) )
		return;
	UI08 charWorld						= s->WorldNumber();
	const CTeleLocationEntry *getTeleLoc = NULL;
	bool isOnTeleporter;
	for( size_t i = 0; i < cwmWorldState->teleLocs.size(); ++i )
	{
		isOnTeleporter	= false;
		getTeleLoc		= &cwmWorldState->teleLocs[i];
		if( getTeleLoc == NULL )
			continue;
		if( getTeleLoc->SourceWorld() == 0xFF || getTeleLoc->SourceWorld() == charWorld )
		{
			if( getTeleLoc->SourceLocation().z != ILLEGAL_Z )
				isOnTeleporter = ( getTeleLoc->SourceLocation().Mag3D() == s->GetLocation().Mag3D() );
			else
				isOnTeleporter = ( getTeleLoc->SourceLocation().Mag() == s->GetLocation().Mag() );
			if( isOnTeleporter )
			{
				if( getTeleLoc->TargetWorld() != charWorld )
				{
					s->SetLocation( (SI16)getTeleLoc->SourceLocation().x, (SI16)getTeleLoc->SourceLocation().y, (UI08)getTeleLoc->SourceLocation().z, getTeleLoc->TargetWorld() );
					if( !s->IsNpc() )
						SendMapChange( getTeleLoc->TargetWorld(), calcSocketObjFromChar( s ) );
				}
				else
					s->SetLocation( (SI16)getTeleLoc->SourceLocation().x, (SI16)getTeleLoc->SourceLocation().y, (UI08)getTeleLoc->SourceLocation().z );
			}
			else if( s->GetY() < getTeleLoc->SourceLocation().y || s->GetX() < getTeleLoc->SourceLocation().x )
				break;
		}
	}
}

void checkRegion( CSocket *mSock, CChar *i );
void cMovement::Walking( CSocket *mSock, CChar *c, UI08 dir, SI16 sequence )
{
	// sometimes the NPC movement code comes up with -1, for example, if we are following someone
	// and we are directly on top of them
	// ^ That's because chardir returns -1 when direction is the same.  Actually, this
	// should only happen when a player walks on top of an NPC and the NPC tries to move.

	if( !isValidDirection( dir ) )
	{
		Console.Error( 2, "%s (cMovement::Walking) caught bad direction = %s %d 0x%x\n", DBGFILE, c->GetName().c_str(), dir, dir );
		// If I don't do this, the NPC will keep trying to walk on the same step, which is
		// where he's already at. Can cause an infinite loop. (Trust me, was one of the things
		// that locked up NW Alpha 2)
		// Will call something like NPCRandomMove in the future.
		if( c->IsNpc() )
			c->FlushPath();
		return;
	}

    if( !VerifySequence( c, mSock, sequence ) )
        return;

	// If checking for weight is more expensive, shouldn't we check for frozen first?
	if( isFrozen( c, mSock, sequence ) )
		return;
    
    if( isOverloaded( c, mSock, sequence ) )
        return;
    
	// save our original location before we even think about moving
	const SI16 oldx = c->GetX();
	const SI16 oldy = c->GetY();

	// see if we have stopped to turn or if we are moving
	const bool amTurning = ( (dir&0x07) != c->GetDir() );
	if( !amTurning )
	{
		if( !CheckForRunning( c, dir ) )
			return;
		
		if( !CheckForStealth( c ) )
			return;
		
		SI08 myz = ILLEGAL_Z;
		SI16 myx = GetXfromDir( dir, c->GetX() );
		SI16 myy = GetYfromDir( dir, c->GetY() );
		if( !calc_move( c, c->GetX(), c->GetY(), myz, dir ) )
		{
#if DEBUG_WALKING
			Console.Print( "DEBUG: %s (cMovement::Walking) Character Walk Failed for %s\n", DBGFILE, c->GetName() );
			Console.Print( "DEBUG: %s (cMovement::Walking) sx (%d) sy (%d) sz (%d)\n", DBGFILE, c->GetX(), c->GetY(), c->GetZ() );
			Console.Print( "DEBUG: %s (cMovement::Walking) dx (%d) dy (%d) dz (%d)\n", DBGFILE, myx, myy, myz );
#endif
			if( mSock != NULL )
				deny( mSock, c, sequence );
			if( c->IsNpc() )
				c->FlushPath();
			return;
		}
#if DEBUG_WALKING
		Console.Print( "DEBUG: %s (cMovement::Walking) Character Walk Passed for %s\n", DBGFILE, c->GetName() );
		Console.Print( "DEBUG: %s (cMovement::Walking) sx (%d) sy (%d) sz (%d)\n", DBGFILE, c->GetX(), c->GetY(), c->GetZ() );
		Console.Print( "DEBUG: %s (cMovement::Walking) dx (%d) dy (%d) dz (%d)\n", DBGFILE, myx, myy, myz );
#endif

		if( c->IsNpc() && CheckForCharacterAtXYZ( c, myx, myy, myz ) )
		{
			c->FlushPath();
			return;
		}

		MoveCharForDirection( c, myz, dir );
		
		// i actually moved this for now after the z =  illegal_z, in the end of CrazyXYBlockStuff()
		// can't see how that would hurt anything
		if( !CheckForHouseBan( c, mSock ) )
			return;
		
		//	
		//	 OK AT THIS POINT IT IS NOW OFFICIALLY A LEGAL MOVE TO MAKE, LETS GO FOR IT!
		//	
		//	 That means any bugs concerning if a move was legal must be before this point!
		//	

		// i moved this down after we are certain we are moving
		if( cwmWorldState->ServerData()->AmbientFootsteps() )
  			Effects->playTileSound( mSock );
		
		// since we actually moved, update the regions code
		HandleRegionStuffAfterMove( c, oldx, oldy );
	}
	
	// do all of the following regardless of whether turning or moving i guess
	
	// set the player direction to contain only the cardinal direction bits
//	c->SetDir( (dir&0x07) );
	c->WalkDir( (dir&0x07) );
	
	SendWalkToPlayer( c, mSock, sequence );
	SendWalkToOtherPlayers( c, dir, oldx, oldy );
	OutputShoveMessage( c, mSock );
	
	// i'm going ahead and optimizing this, if you haven't really moved, should be
	// no need to check for teleporters and the weather shouldn't change
	if( !amTurning )
	{
		HandleItemCollision( c, mSock, oldx, oldy );
		HandleTeleporters( c );
		HandleWeatherChanges( c, mSock );
		checkRegion( mSock, c );
	}
}

// Function      : cMovement::isValidDirection()
// Written by    : Unknown
// Revised by    : Thyme
// Revision Date : 2000.09.21
// Purpose       : Check if a given direction is valid
// Method        : Return true on the below values:
//
// Direction   Walking Value   Running Value
// North          0 0x00         128 0x80
// Northeast      1 0x01         129 0x81
// East           2 0x02         130 0x82
// Southeast      3 0x03         131 0x83
// South          4 0x04         132 0x84
// Southwest      5 0x05         133 0x85
// West           6 0x06         134 0x86
// Northwest      7 0x07         135 0x87

bool cMovement::isValidDirection( UI08 dir )
{
	return ( dir == ( dir & 0x87 ) );
}

// Function      : cMovement::isFrozen()
// Written by    : Unknown
// Revised by    : Thyme
// Revision Date : 2000.09.21
// Purpose       : Check if a character is frozen or casting a spell
// Method        : Because of the way the source uses the frozen flag, I decided to change
// something in how this works. If the character is casting a spell (chars.casting is true)
// OR if they're frozen (chars.priv2 & FROZEN_BIT) then they can't walk. Why? If a player/npc
// has their frozen bit set, and they cast a spell, they will lose their frozen bit at the
// end of the spell cast. With this new check, we don't even need to set the frozen bit when
// casting a spell!

bool cMovement::isFrozen( CChar *c, CSocket *mSock, SI16 sequence )
{
	if( c->IsCasting() )
	{
		if( mSock != NULL )
		{
			mSock->sysmessage( 1380 );
			deny( mSock, c, sequence );
		}
#if DEBUG_WALKING
		Console.Print( "DEBUG: %s (cMovement::isFrozen) casting char %s\n", DBGFILE, c->GetName() );
#endif
		return true;
	}
	if( c->IsFrozen() )
	{
		if( mSock != NULL )
		{
			mSock->sysmessage( 1381 );
			deny( mSock, c, sequence );
		}
#if DEBUG_WALKING
		Console..Print( "DEBUG: %s (cMovement::isFrozen) frozen char %s\n", DBGFILE, c->GetName() );
#endif
		return true;
	} 
	return false;
}

// Thyme 07/28/00

// return TRUE is character is overloaded (with weight)
// return FALSE otherwise

// CheckForWeight was confusing...

// Old code called check weight first then checked socket... I changed it the other way.
// Why, well odds are (I may be wrong) if you don't have a socket, you're an NPC and if you
// have one, you're a character. We said in the first line that we didn't want to restrict
// based upon NPC, so if you're an NPC, the socket/checkweight will never be called anyway.

// Rewrote to deny the client... We'll see if it works.

bool cMovement::isOverloaded( CChar *c, CSocket *mSock, SI16 sequence )
{
	// Who are we going to check for weight restrictions?
	if( !c->IsDead() && !c->IsNpc() && ( !c->IsCounselor() && !c->IsGM() ) )
	{
		if( mSock != NULL )
		{
			if( Weight->isOverloaded( c ) )
				c->SetStamina( (UI16)(((c->GetWeight() / 100) - ((c->GetStrength() * cwmWorldState->ServerData()->WeightPerStr()) + 30)) * 2) );
			if( c->GetStamina() <= 0 )
			{
				c->SetStamina( 0 );
				mSock->sysmessage( 1382 );
				deny( mSock, c, sequence );
#if DEBUG_WALKING
				Console.Print( "DEBUG: %s (cMovement::Walking) overloaded char %s\n", DBGFILE, c->GetName() );
#endif
				return true;
			}
		}
	}
	return false;
}
// Thyme 07/28/00

// Here's how I'm going to use it for now.  Movement Type may be used for races, that's why
// I put it as an integer.  Here are the values I'm going to use:
// BTW, bird doesn't work right now! These values should be scripted, as well, but I want
// to test walking before another var is added to chars struct.
// GM Body  0x01
// Player   0x02
// Bird     0x20 (basically, a fish and an NPC, so I could use 0xc0, but I don't wanna)
// NPC      0x40
// Fish     0x80 (So they can swim!)
// I left a gap between Player and NPC because someone may want to implement race
// restrictions... 

// Examins a location (cx, cy, cz) to determine if a character is found there.  If so, return true
// If not, return false.  Used for the npc stacking problem
bool cMovement::CheckForCharacterAtXYZ( CChar *c, SI16 cx, SI16 cy, SI08 cz )
{
	SubRegion *MapArea = MapRegion->GetCell( cx, cy, c->WorldNumber() );	// check 3 cols... do we really NEED to?
	if( MapArea == NULL )	// no valid region
		return false;
	CDataList< CChar * > *regChars = MapArea->GetCharList();
	regChars->Push();
	for( CChar *tempChar = regChars->First(); !regChars->Finished(); tempChar = regChars->Next() )
	{
		if( !ValidateObject( tempChar ) )
			continue;
		if( tempChar != c && ( isOnline( tempChar ) || tempChar->IsNpc() ) )
		{	// x=x,y=y, and distance btw z's <= MAX STEP
			if( tempChar->GetX() == cx && tempChar->GetY() == cy && tempChar->GetZ() >= cz && tempChar->GetZ() <= (cz + 5) )	// 2 people will still bump into each other, if slightly offset
			{
				regChars->Pop();	// restore before returning
				return true;
			}
		}
	}
	regChars->Pop();
	return false;
}

// if we have a valid socket, see if we need to deny the movement request because of
// something to do with the walk sequence being out of sync.
bool cMovement::VerifySequence( CChar *c, CSocket *mSock, SI16 sequence )
{
    if( mSock != NULL )
    {
        if( mSock->WalkSequence() + 1 != sequence && sequence != 256 )
        {
            deny( mSock, c, sequence );  
            return false;
        }
        
    }
    return true;
}

bool cMovement::CheckForRunning( CChar *c, UI08 dir )
// returns true if updatechar required, or false if not
{
	// if we are running
	if( dir&0x80 )
	{
		if( !c->IsNpc() )
			c->SetTimer( tNPC_MOVETIME, cwmWorldState->GetUICurrentTime() + ( ( cwmWorldState->ServerData()->SystemTimer( STAMINA_REGEN ) * 1000 ) / 2 ) );

		// if we are using stealth
		if( c->GetStealth() != -1 )	// Stealth - stop hiding if player runs
			c->ExposeToView();
		//Don't regenerate stamina while running
		c->SetRegen( BuildTimeValue( static_cast<R32>(cwmWorldState->ServerData()->SystemTimer( STAMINA_REGEN ) )), 1 );
		c->SetRunning( c->GetRunning() + 1 );

		if( !c->IsDead() && c->GetCommandLevel() < CNS_CMDLEVEL )
		{
			bool reduceStamina = ( c->IsOnHorse() && c->GetRunning() > ( cwmWorldState->ServerData()->MaxStaminaMovement() * 2 ) );
			if( !reduceStamina )
				reduceStamina = ( c->GetRunning() > ( cwmWorldState->ServerData()->MaxStaminaMovement() * 4 ) );
			if( reduceStamina )
			{
				c->SetRunning( 0 );
				c->SetStamina( c->GetStamina() - 1 );
			}
		}
		if( c->IsAtWar() && ValidateObject( c->GetTarg() ) )
			c->SetTimer( tCHAR_TIMEOUT, BuildTimeValue( 2 ) );

	}
	else
	{
		c->SetRunning( 0 );
		if( !c->IsNpc() )
			c->SetTimer( tNPC_MOVETIME, cwmWorldState->GetUICurrentTime() + ( ( cwmWorldState->ServerData()->SystemTimer( STAMINA_REGEN ) * 1000 ) / 4 ) );
	}
	return true;
}


//o--------------------------------------------------------------------------o
//|	Function/Class	-	bool cMovement::CheckForStealth( CChar *c )
//|	Date			-	09/22/2002
//|	Developer(s)	-	Unknown
//|	Company/Team	-	UOX3 DevTeam
//|	Status			-	
//o--------------------------------------------------------------------------o
//|	Description		-	Check to see if character is in stealth mode. 
//|									
//|	Modification	-	09/22/2002	-	Xuri - Unhide characters who are mounted
//|									while trying to stealth.
//o--------------------------------------------------------------------------o
//|	Returns				- Always returns true it seems
//o--------------------------------------------------------------------------o	
bool cMovement::CheckForStealth( CChar *c )
{
	if( c->GetVisible() == VT_TEMPHIDDEN || c->GetVisible() == VT_INVISIBLE )
	{
		// Sept 22, 2002 - Xuri
		if( c->IsOnHorse() )
		{
			if( !cwmWorldState->ServerData()->CharHideWhileMounted() )
				c->ExposeToView();
		}
		//
		if( c->GetStealth() != -1 )
		{
			c->SetStealth( c->GetStealth() + 1 );
			if( c->GetStealth() > ( cwmWorldState->ServerData()->MaxStealthMovement() * c->GetSkill( STEALTH ) / 1000 ) )
				c->ExposeToView();
		}
		else
			c->ExposeToView();
	}
	return true;
}

// see if a player has tried to move into a house they were banned from
bool cMovement::CheckForHouseBan( CChar *c, CSocket *mSock )
{
	CMultiObj *house = findMulti( c );
	if( ValidateObject( house ) ) 
	{
		if( c->GetMultiObj() != house )
			c->SetMulti( house );//Set them inside the multi!
		if( house->IsOnBanList( c ) )
		{
			SI16 sx, sy, ex, ey;
			Map->MultiArea( house, sx, sy, ex, ey );
			c->SetLocation( ex, ey + 1, c->GetZ() );
			if( mSock != NULL )
			{
				mSock->sysmessage( 1639 );
				mSock->WalkSequence( -1 );
			}
			return false;
		}
	}
	else
	{
		if( c->GetMultiObj() != NULL )
			c->SetMulti( INVALIDSERIAL );
	}
	return true;
}

// Thyme 2000.09.21
// I already made sure I could move there (even the crazy XY block stuff) so this IS a valid move. Just move the
// directions. Oh, and since I we already have the GetX/YfromDir functions (and we need those) why don't we just
// use them here?

void cMovement::MoveCharForDirection( CChar *c, SI08 myz, UI08 dir )
{
	c->WalkXY( GetXfromDir( dir, c->GetX() ), GetYfromDir( dir, c->GetY() ) );
	c->WalkZ( myz );
}

// Split up of FillXYBlockStuff


void cMovement::GetBlockingMap( SI16 x, SI16 y, CTileUni *xyblock, int &xycount, SI16 oldx, SI16 oldy, UI08 worldNumber )
{
	if( xycount >= XYMAX )	// don't overflow
		return;
	// This oldx, oldy stuff is... problematic, to say the least
	UI16 mapid		= 0;
	UI16 mapid_old	= 0;
	SI08 mapz		= Map->AverageMapElevation( x, y, mapid, worldNumber );
	SI08 mapz_old	= Map->AverageMapElevation( oldx, oldy, mapid_old, worldNumber );
	if( mapz_old <= ILLEGAL_Z )	
		mapz_old = mapz;
	if( mapz != ILLEGAL_Z )
	{
		CLand land;
		Map->SeekLand( mapid, &land );
		
		xyblock[xycount].Type( 0 );
		xyblock[xycount].BaseZ( UOX_MIN( mapz_old, mapz ) );
		xyblock[xycount].ID( mapid );
		xyblock[xycount] = land;
		xyblock[xycount].Height( mapz - xyblock[xycount].BaseZ() );
		xyblock[xycount].Weight( 255 );
		++xycount;
	}
}


void cMovement::GetBlockingStatics( SI16 x, SI16 y, CTileUni *xyblock, int &xycount, UI08 worldNumber )
{
	if( xycount >= XYMAX )	// don't overflow
		return;
	MapStaticIterator msi( x, y, worldNumber );
	staticrecord *stat = NULL;
 	while( stat = msi.Next() )
	{
		CTile tile;
		msi.GetTile( &tile );
		xyblock[xycount].Type( 2 );
		xyblock[xycount].BaseZ( stat->zoff );
		xyblock[xycount].ID( stat->itemid );
		xyblock[xycount] = tile;
		xyblock[xycount].Weight( 255 );
		++xycount;
		if( xycount >= XYMAX )	// don't overflow
			return;
	}
}

void cMovement::GetBlockingDynamics( SI16 x, SI16 y, CTileUni *xyblock, int &xycount, UI08 worldNumber )
{
	if( xycount >= XYMAX )	// don't overflow
		return;
	SubRegion *MapArea = MapRegion->GetCell( x, y, worldNumber );
	if( MapArea == NULL )	// no valid region
		return;
	CDataList< CItem * > *regItems = MapArea->GetItemList();
	regItems->Push();
	for( CItem *tItem = regItems->First(); !regItems->Finished(); tItem = regItems->Next() )
	{
		if( !ValidateObject( tItem ) )
			continue;
		if( tItem->GetID() < 0x4000 )
		{
#if DEBUG_WALKING
			Console.Print( "DEBUG: Item X: %i\nItem Y: %i\n", tItem->GetX(), tItem->GetY() );
#endif
			if( tItem->GetX() == x && tItem->GetY() == y )
			{
				CTile tile;
				Map->SeekTile( tItem->GetID(), &tile );
				xyblock[xycount].Type( 1 );
				xyblock[xycount].BaseZ( tItem->GetZ() );
				xyblock[xycount].ID( tItem->GetID() );
				xyblock[xycount] = tile;
				++xycount;
				if( xycount >= XYMAX )	// don't overflow
				{
					regItems->Pop();
					return;
				}
			}
		}
		else if( abs( tItem->GetX() - x ) <= DIST_BUILDRANGE && abs( tItem->GetY() - y) <= DIST_BUILDRANGE )
		{
			UI16 multiID = tItem->GetID() - 0x4000;
			SI32 length = 0;		// should be SI32, not long
			Map->SeekMulti( multiID, &length );
			if( length == -1 || length >= 17000000 ) //Too big... bug fix hopefully (Abaddon 13 Sept 1999)
			{
				Console.Error( 2, "Walking() - Bad length in multi file. Avoiding stall" );
				length = 0;
			}
			for( SI32 j = 0; j < length; ++j )
			{
				st_multi *multi;
				multi = Map->SeekIntoMulti( multiID, j );
				if( multi->visible && (tItem->GetX() + multi->x) == x && (tItem->GetY() + multi->y) == y )
				{
					CTile tile;
					Map->SeekTile( multi->tile, &tile );
					xyblock[xycount].Type( 2 );
					xyblock[xycount].BaseZ( multi->z + tItem->GetZ() );
					xyblock[xycount].ID( multi->tile );
					xyblock[xycount] = tile;
					xyblock[xycount].Weight( 255 );
					++xycount;
					if( xycount >= XYMAX )	// don't overflow
					{
						regItems->Pop();
						return;
					}
				}
			}
		}
	}
	regItems->Pop();
}

// so we are going to move, lets update the regions
// FYI, Items equal to or greater than 1000000 are considered characters...
void cMovement::HandleRegionStuffAfterMove( CChar *c, SI16 oldx, SI16 oldy )
{
	// save where we were moving to
	const SI16 nowx = c->GetX();
	const SI16 nowy = c->GetY();

	// i'm trying a new optimization here, if we end up in the same map cell
	// as we started, i'm sure there's no real reason to remove and readd back
	// to the same spot..
	UI08 worldNumber = c->WorldNumber();
	SubRegion *cell1 = MapRegion->GetCell( oldx, oldy, worldNumber );
	SubRegion *cell2 = MapRegion->GetCell( nowx, nowy, worldNumber );
	if( cell1 != cell2 )
	{
		cell1->GetCharList()->Remove( c );
		if( ValidateObject( c ) )
			cell2->GetCharList()->Add( c );
	}
#if DEBUG_WALKING
	else
		Console.Print( "DEBUG: Character: %s(0x%X) didn't change regions.", c->GetName(), c->GetSerial() );
#endif
}


// actually send the walk command back to the player and increment the sequence
void cMovement::SendWalkToPlayer( CChar *c, CSocket *mSock, SI16 sequence )
{
	if( mSock != NULL )
	{
		CPWalkOK toSend;

		toSend.SequenceNumber( mSock->GetByte( 2 ) );
		if( c->GetVisible() != VT_VISIBLE )
			toSend.OtherByte( 0 );
		else
			toSend.OtherByte( 0x41 );

		mSock->Send( &toSend );
		mSock->WalkSequence( sequence );
		if( mSock->WalkSequence() == 255 )
			mSock->WalkSequence( 0 );
	}
}

void cMovement::SendWalkToOtherPlayers( CChar *c, UI08 dir, SI16 oldx, SI16 oldy )
{
	// lets cache these vars in advance
	const SI16 newx		= c->GetX();
	const SI16 newy		= c->GetY();

	bool checkX			= (oldx != newx);
	bool checkY			= (oldy != newy);

	CPExtMove toSend	= (*c);

	Network->PushConn();
	for( CSocket *tSend = Network->FirstSocket(); !Network->FinishedSockets(); tSend = Network->NextSocket() )
	{	// lets see, its much cheaper to call perm[i] first so i'm reordering this
		if( tSend == NULL )
			continue;
		CChar *mChar = tSend->CurrcharObj();
		if( !ValidateObject( mChar ) )
			continue;
		const UI16 visibleRange = static_cast<UI16>(tSend->Range() + Races->VisRange( mChar->GetRace() ));
		const UI16 clientRange	= tSend->Range();
		if( c != mChar && objInRange( c, mChar, visibleRange ) )
		{
			if( checkX )	// Only check X Plane if their x changed
			{
				UI16 dxNew = static_cast<UI16>(abs( newx - mChar->GetX() ));
				UI16 dxOld = static_cast<UI16>(abs( oldx - mChar->GetX() ));
				if( ( dxNew == visibleRange && dxOld > visibleRange ) || ( dxNew == clientRange && dxOld > clientRange ) )
				{
					c->SendToSocket( tSend );
					continue;	// Incase they moved diagonally, lets not update the same character multiple times
				}
			}
			if( checkY )	// Only check Y plane if their y changed
			{
				UI16 dyNew = static_cast<UI16>(abs( newy - mChar->GetY() ));
				UI16 dyOld = static_cast<UI16>(abs( oldy - mChar->GetY() ));
				if( ( dyNew == visibleRange && dyOld > visibleRange ) || ( dyNew == clientRange && dyOld > clientRange ) )
				{
					c->SendToSocket( tSend );
					continue;
				}
			}
			toSend.FlagColour( FlagColour( c, mChar ) );
			tSend->Send( &toSend );
		}
	}
	Network->PopConn();
}

// see if we should mention that we shove something out of the way
void cMovement::OutputShoveMessage( CChar *c, CSocket *mSock )
{
	if( mSock == NULL )
		return;

	// GMs, counselors, and ghosts don't shove things
	if( c->GetCommandLevel() >= CNS_CMDLEVEL || c->GetID() == 0x03DB || c->IsDead() )
		return;
	// lets cache these vars in advance
	UI08 worldNumber	= c->WorldNumber();
	SubRegion *grid		= MapRegion->GetCell( c->GetX(), c->GetY(), worldNumber );
	if( grid == NULL )
		return;
	CChar *ourChar		= NULL;
	CDataList< CChar * > *regChars = grid->GetCharList();
	regChars->Push();
	SI16 x				= c->GetX();
	SI16 y				= c->GetY();
	SI08 z				= c->GetZ();
	UI16 targTrig		= c->GetScriptTrigger();
	cScript *toExecute	= Trigger->GetScript( targTrig );
	for( ourChar = regChars->First(); !regChars->Finished(); ourChar = regChars->Next() )
	{
		if( !ValidateObject( ourChar ) )
			continue;
		if( ourChar != c && ( ourChar->IsNpc() || isOnline( ourChar ) ) )
		{
			if( ourChar->GetX() == x && ourChar->GetY() == y && ourChar->GetZ() == z )
			{
				if( toExecute != NULL )
					toExecute->OnCollide( mSock, c, ourChar );
				UI16 tTrig		= ourChar->GetScriptTrigger();
				cScript *tExec	= Trigger->GetScript( tTrig );
				if( tExec != NULL )
					tExec->OnCollide( calcSocketObjFromChar( ourChar ), ourChar, c );
				if( ourChar->GetVisible() == VT_TEMPHIDDEN || ourChar->GetVisible() == VT_INVISIBLE )
				{
					mSock->sysmessage( 1383, ourChar->GetName().c_str() );
					c->SetStamina( UOX_MAX( c->GetStamina() - 4, 0 ) );
				}
				else if( ourChar->GetCommandLevel() < CNS_CMDLEVEL )
				{
					mSock->sysmessage( 1384 );
					c->SetStamina( UOX_MAX( c->GetStamina() - 4, 0 ) );
				}
			}
		}
	}
	regChars->Pop();
}

bool UpdateItemsOnPlane( CSocket *mSock, CChar *mChar, CItem *tItem, UI16 id, UI16 dNew, UI16 dOld, UI16 visibleRange, bool isGM )
{
	if( mSock != NULL && ( (id >= 0x407A && id <= 0x407F) || id == 0x5388 ) )
	{
		if( dNew == DIST_BUILDRANGE && dOld > DIST_BUILDRANGE )	// It's a building
		{
			tItem->SendToSocket( mSock );
			return true;
		}
		else if( dOld == DIST_BUILDRANGE && dNew > DIST_BUILDRANGE )
		{
			tItem->RemoveFromSight( mSock );
			return true;
		}
	}
	else if( dNew == visibleRange && dOld > visibleRange )	// Just came into range
	{
		if( tItem->GetVisible() != VT_PERMHIDDEN || isGM )
		{
			if( mSock != NULL )
				tItem->SendToSocket( mSock );
			UI16 targTrig		= mChar->GetScriptTrigger();
			cScript *toExecute	= Trigger->GetScript( targTrig );
			if( toExecute != NULL )
				toExecute->InRange( mChar, tItem );
			return true;
		}
	}
	else if( dOld == visibleRange && dNew > visibleRange )	// Just went out of range
	{
		UI16 targTrig		= mChar->GetScriptTrigger();
		cScript *toExecute	= Trigger->GetScript( targTrig );
		if( toExecute != NULL )
			toExecute->OutOfRange( mChar, tItem );
		if( mSock != NULL )
			tItem->RemoveFromSight( mSock );
		return true;
	}
	return false;
}

bool UpdateCharsOnPlane( CSocket *mSock, CChar *mChar, CChar *tChar, UI16 dNew, UI16 dOld, UI16 visibleRange )
{
	if( dNew == visibleRange && dOld > visibleRange )	// Just came into range
	{
		tChar->SendToSocket( mSock );
		UI16 targTrig		= mChar->GetScriptTrigger();
		cScript *toExecute	= Trigger->GetScript( targTrig );
		if( toExecute != NULL )
			toExecute->InRange( mChar, tChar );
		return true;
	}
	if( dOld == visibleRange && dNew > visibleRange )	// Just went out of range
	{
		tChar->RemoveFromSight( mSock );
		UI16 targTrig		= mChar->GetScriptTrigger();
		cScript *toExecute	= Trigger->GetScript( targTrig );
		if( toExecute != NULL )
			toExecute->OutOfRange( mChar, tChar );
		return true;
	}
	return false;
}

void MonsterGate( CChar *s, std::string scriptEntry );
void advanceObj( CChar *s, UI16 x, bool multiUse );
void SocketMapChange( CSocket *sock, CChar *charMoving, CItem *gate );
void HandleObjectCollisions( CSocket *mSock, CChar *mChar, CItem *itemCheck, ItemTypes type )
{
	switch( itemCheck->GetType() )
	{
		case IT_OBJTELEPORTER:														// teleporters
			if( itemCheck->GetTempVar( CITV_MOREX ) + itemCheck->GetTempVar( CITV_MOREY ) + itemCheck->GetTempVar( CITV_MOREZ ) > 0 )
				mChar->SetLocation( static_cast<UI16>(itemCheck->GetTempVar( CITV_MOREX )), static_cast<UI16>(itemCheck->GetTempVar( CITV_MOREY )), static_cast<SI08>(itemCheck->GetTempVar( CITV_MOREZ )) );
			break;
		case IT_ADVANCEGATE:														// advancement gates
		case IT_MULTIADVANCEGATE:
			if( !mChar->IsNpc() )
				advanceObj( mChar, static_cast<UI16>(itemCheck->GetTempVar( CITV_MOREX )), ( itemCheck->GetType() == IT_MULTIADVANCEGATE ) );
			break;
		case IT_MONSTERGATE:	MonsterGate( mChar, itemCheck->GetDesc() );	break;	// monster gates
		case IT_RACEGATE:														// race gates
			Races->gate( mChar, static_cast<RACEID>(itemCheck->GetTempVar( CITV_MOREX )), itemCheck->GetTempVar( CITV_MOREY ) != 0 );
			break;
		case IT_DAMAGEOBJECT:														// damage objects
			if( !mChar->IsInvulnerable() )
			{
				mChar->SetHP( static_cast<SI16>( mChar->GetHP() - ( itemCheck->GetTempVar( CITV_MOREX ) + RandomNum( itemCheck->GetTempVar( CITV_MOREY ), itemCheck->GetTempVar( CITV_MOREZ ) ) ) ) );
				if( mChar->GetHP() < 1 )
					mChar->SetHP( 0 );
				if( mChar->GetHP() <= 0 )
					HandleDeath( mChar );
			}
			break;
		case IT_SOUNDOBJECT:														// sound objects
			if( static_cast<UI32>(RandomNum( 1, 100 )) <= itemCheck->GetTempVar( CITV_MOREZ ) )
				Effects->PlaySound( itemCheck, static_cast<UI16>(itemCheck->GetTempVar( CITV_MOREX )) );
			break;
		case IT_MAPCHANGEOBJECT:
			SocketMapChange( mSock, mChar, itemCheck );
			break;
		case IT_ZEROKILLSGATE:	mChar->SetKills( 0 );		break;			// zero kill gate
		case IT_WORLDCHANGEGATE:	
			if( !mChar->IsNpc() )	// world change gate
			{
				if( mSock != NULL )
				{
					CPWorldChange wrldChange( (WorldType)itemCheck->GetTempVar( CITV_MOREX ), 1 );
					mSock->Send( &wrldChange );
				}
			}
			break;
	}
}

// handle item collisions, make items that appear on the edge of our sight because
// visible, buildings when they get in range, and if the character steps on something
// that might cause damage
void cMovement::HandleItemCollision( CChar *mChar, CSocket *mSock, SI16 oldx, SI16 oldy )
{
	// lets cache these vars in advance
	UI16 visibleRange	= static_cast<UI16>(MAX_VISRANGE + Races->VisRange( mChar->GetRace() ));
	if( mSock != NULL )
		visibleRange	= static_cast<UI16>(mSock->Range() + Races->VisRange( mChar->GetRace() ));
	const SI16 newx		= mChar->GetX();
	const SI16 newy		= mChar->GetY();
	UI16 id;
	ItemTypes type;
	bool EffRange;
	
	bool isGM			= mChar->IsGM();
	UI16 dxNew, dyNew, dxOld, dyOld;
	const bool checkX	= (oldx != newx);
	const bool checkY	= (oldy != newy);
	/*
	A note to future people (from Zippy on 2/10/02)

	The OSI client MAY be ignoring certain items that it thinks fall out of the vis Range...

	Currently, UOX calculates vis in a square... meaning if an item in the corner of our
	'square' is sent to the OSI client, it MAY ignore it because it falls out of the client's
	apparent circle of visibility.

	At this time, I'm not sure this is causing problems, or whether problems are steming from
	a region issue (fixed now).  So I have not updated this code.  If this problem pops up in
	the future, and I'm no longer around...  This is your piece of the puzzle.
	*/

	REGIONLIST nearbyRegions = MapRegion->PopulateList( newx, newy, mChar->WorldNumber() );
	for( REGIONLIST_CITERATOR rIter = nearbyRegions.begin(); rIter != nearbyRegions.end(); ++rIter )
	{
		SubRegion *MapArea = (*rIter);
		if( MapArea == NULL )	// no valid region
			continue;
		if( mSock != NULL )		// Only send char stuff if we have a valid socket
		{
			CDataList< CChar * > *regChars = MapArea->GetCharList();
			regChars->Push();
			for( CChar *tempChar = regChars->First(); !regChars->Finished(); tempChar = regChars->Next() )
			{
				if( !ValidateObject( tempChar ) )
					continue;
				// Character Send Stuff
				if( tempChar->IsNpc() || isOnline( tempChar ) || ( isGM && cwmWorldState->ServerData()->ShowHiddenNpcStatus() ) )
				{
					if( checkX )	// Only update on x plane if our x changed
					{
						dxNew = static_cast<UI16>(abs( tempChar->GetX() - newx ));
						dxOld = static_cast<UI16>(abs( tempChar->GetX() - oldx ));
						if( UpdateCharsOnPlane( mSock, mChar, tempChar, dxNew, dxOld, visibleRange ) )
							continue;	// If we moved diagonally, don't update the same char twice.
					}
					if( checkY )	// Only update on y plane if our y changed
					{
						dyNew = static_cast<UI16>(abs( tempChar->GetY() - newy ));
						dyOld = static_cast<UI16>(abs( tempChar->GetY() - oldy ));

						if( UpdateCharsOnPlane( mSock, mChar, tempChar, dyNew, dyOld, visibleRange ) )
							continue;
					}
				}
			}
			regChars->Pop();
		}
		CDataList< CItem * > *regItems = MapArea->GetItemList();
		regItems->Push();
		for( CItem *tItem = regItems->First(); !regItems->Finished(); tItem = regItems->Next() )
		{
			if( !ValidateObject( tItem ) )
				continue;
			id		= tItem->GetID();
			type	= tItem->GetType();
			EffRange = (	tItem->GetX() == newx && tItem->GetY() == newy && 
							mChar->GetZ() >= tItem->GetZ() && mChar->GetZ() <= ( tItem->GetZ() + 5 ) );
			if( EffRange )
			{
				if( !Magic->HandleFieldEffects( mChar, tItem, id ) )
				{
					if( id < 0x4000 )
					{
						UI16 targTrig		= tItem->GetScriptTrigger();
						cScript *toExecute	= Trigger->GetScript( targTrig );
						if( toExecute != NULL )
							toExecute->OnCollide( mSock, mChar, tItem );
					}
				}
				HandleObjectCollisions( mSock, mChar, tItem, type );
				Magic->GateCollision( mSock, mChar, tItem, type );
			}

			if( checkX )	// Only update items on furthest x plane if our x changed
			{
				dxNew = static_cast<UI16>(abs( tItem->GetX() - newx ));
				dxOld = static_cast<UI16>(abs( tItem->GetX() - oldx ));
				if( UpdateItemsOnPlane( mSock, mChar, tItem, id, dxNew, dxOld, visibleRange, isGM ) )
					continue;	// If we moved diagonally, lets not update an item twice (since it could match the furthest x and y)
			}
			if( checkY )	// Only update items on furthest y plane if our y changed
			{
				dyNew = static_cast<UI16>(abs( tItem->GetY() - newy ));
				dyOld = static_cast<UI16>(abs( tItem->GetY() - oldy ));

				if( UpdateItemsOnPlane( mSock, mChar, tItem, id, dyNew, dyOld, visibleRange, isGM ) )
					continue;
			}
		}
		regItems->Pop();
	}
}

// start of LB's no rain & snow in buildings stuff 
void cMovement::HandleWeatherChanges( CChar *c, CSocket *mSock )
{
//	if( !c->IsNpc() && isOnline( c ) ) // check for being in buildings (for weather) only for PC's
//		return;
}

// return whether someone is a GM Body
bool cMovement::IsGMBody( CChar *c )
{
	if( c->IsGM() || c->GetID() == 0x03DB || c->GetID() == 0x0192 || c->GetID() == 0x0193 || c->IsDead() ) 
        return true;
    return false;
}

void cMovement::CombatWalk( CChar *i ) // Only for switching to combat mode
{
	if( !ValidateObject( i ) )
		return;
	CPExtMove toSend = (*i);

	if( !i->IsAtWar() )
		i->SetTarg( NULL );
    
	SOCKLIST nearbyChars = FindNearbyPlayers( i );
	for( SOCKLIST_CITERATOR cIter = nearbyChars.begin(); cIter != nearbyChars.end(); ++cIter )
    {
		CChar *mChar = (*cIter)->CurrcharObj();
		if( mChar != i )
        {
			toSend.FlagColour( FlagColour( i, mChar ) );
            (*cIter)->Send( &toSend );
        }
    }
}

bool checkBoundingBox( SI16 xPos, SI16 yPos, SI16 fx1, SI16 fy1, SI08 fz1, SI16 fx2, SI16 fy2, UI08 worldNumber );
bool checkBoundingCircle( SI16 xPos, SI16 yPos, SI16 fx1, SI16 fy1, SI08 fz1, SI16 radius, UI08 worldNumber );
void cMovement::NpcWalk( CChar *i, UI08 j, SI08 getWander )   //type is npcwalk mode (0 for normal, 1 for box, 2 for circle)
{
	SI16 fx1 = i->GetFx( 0 );
	SI16 fx2 = i->GetFx( 1 );
	SI16 fy1 = i->GetFy( 0 );
	SI16 fy2 = i->GetFy( 1 );
	SI08 fz1 = i->GetFz();
    // if we are walking in an area, and the area is not properly defined, just don't bother with the area anymore
    if( ( ( getWander == 3 ) && ( fx1 == -1 || fx2 == -1 || fy1 == -1 || fy2 == -1 ) ) ||
        ( ( getWander == 4 ) && ( fx1 == -1 || fx2 == -1 || fy1 == -1 ) ) ) // circle's don't use fy2, so don't require them! fur 10/30/1999
        
    {
        i->SetNpcWander( 2 ); // Wander freely from now on
    }
	// Thyme New Stuff 2000.09.21
	SI16 newx = GetXfromDir( j, i->GetX() );
	SI16 newy = GetYfromDir( j, i->GetY() );
	UI08 worldNumber = i->WorldNumber();
	// Let's make this a little more readable.
	UI08 jMod = (j & 0x87);
	switch( getWander )
	{
		case 2:	// Wander freely
		case 5:	// Wander freely after fleeing
			Walking( NULL, i, jMod, 256 );
			break;
		case 3:	// Wander inside a box
			if( checkBoundingBox( newx, newy, fx1, fy1, fz1, fx2, fy2, worldNumber ) )
				Walking( NULL, i, jMod, 256 );
			break;
		case 4:	// Wander inside a circle
			if( checkBoundingCircle( newx, newy, fx1, fy1, fz1, fx2, worldNumber ) )
				Walking( NULL, i, jMod, 256 );
			break;
		default:
			Console.Error( 2, "Bad NPC Wander type passed to NpcWalk: %i", getWander );
			break;
	}
}

// Function      : cMovement::GetYfromDir
// Written by    : Unknown
// Revised by    : Thyme
// Revision Date : 2000.09.15
// Purpose       : Return the new y from given dir

SI16 cMovement::GetYfromDir( UI08 dir, SI16 y )
{

	switch ( dir & 0x07 )
	{
		case 0x00 :
		case 0x01 :
		case 0x07 :
			--y; break;
		case 0x03 :
		case 0x04 :
		case 0x05 :
			++y; break;
	}

    return y;

}

// Function      : cMovement::GetXfromDir
// Written by    : Unknown
// Revised by    : Thyme
// Revision Date : 2000.09.15
// Purpose       : Return the new x from given dir

SI16 cMovement::GetXfromDir( UI08 dir, SI16 x )
{

   	switch ( dir & 0x07 )
	{
		case 0x01 :
		case 0x02 :
		case 0x03 :
			++x; break;
		case 0x05 :
		case 0x06 :
		case 0x07 :
			--x; break;
	}

    return x;

}

// Ok, I'm going to babble here, but here's my thinking process...
// Max Heuristic is 5 (for now) and I'm not concerned about walls... I'll add that later
// Easiest way I think would be for recursive call for now... Will change later if need be
// pathfind will call itself with new stuff... as long as the distance get's shorter
// We have to take into consideration if the NPC is going to a point or to a character
// if we don't want NPCs to walk over each other, this has to be known, because the NPC
// that's walking will never reach destination if it's another character.
// We must pass, that way if we get chardist=1 then we're ok.  We are basically searching
// for the shortest path, which is always a diagonal line, followed by lateral to target
// (barring obstacles) On calculation, for the FIRST step, we need to know if a character
// is there or not, then after that no biggie because if so we can just recalc the path if
// something is blocking. If we don't check on that first step, NPCs will get stuck behind horses
// and stuff... Kinda exploitable if I'm on a horse attacking, then step off and behind to hide
// while I heal. The first thing we need to do when walking is determine if i'm blocked... then
// if I'm an NPC, recalculate my path and step... I'm also gonna take out the path structure
// in chars_st... all we need is to hold the directions, not the x and y... Hopefully this will
// save memory.

void cMovement::PathFind( CChar *c, SI16 gx, SI16 gy, bool willRun, UI08 pathLen )
{
	// Make sure this is a valid character before proceeding
	if( !ValidateObject( c ) ) 
		return;

	// Make sure the character has taken used all of their previously saved steps
	if( c->StillGotDirs() ) 
		return;

	// Thyme 2000.09.21
	// initial rewrite of pathfinding...

	SI16 newx, newy;
	SI08 newz;
	const SI16 oldx = c->GetX();
	const SI16 oldy = c->GetY();

#if defined( _MSC_VER )
#pragma note( "PathFind() needs to be touched up, UI08 can possibly be set to -1" )
#endif
	for( UI08 pn = 0; pn < pathLen; ++pn )
	{
		bool bFound	= false;
		int pf_neg	= ( ( RandomNum( 0, 1 ) ) ? 1 : -1 );
		UI08 pf_dir	= Direction( oldx, oldy, gx, gy );
		for( UI08 i = 0 ; i < 8 ; ++i )
		{
			pf_neg *= -1;
			pf_dir += ( i * pf_neg );
			if( pf_dir < 8 && calc_move( c, oldx, oldy, newz, pf_dir ) )
			{
				newx = GetXfromDir( pf_dir, oldx );	// moved inside if to reduce calculations
				newy = GetYfromDir( pf_dir, oldy );
				if( ( pn < P_PF_MRV ) && CheckForCharacterAtXYZ( c, newx, newy, newz ) )
					continue;

				if( willRun )
					c->PushDirection( pf_dir | 0x80 );	// or it with 0x80 to ensure they run
				else
					c->PushDirection( pf_dir );
				bFound = true;
				break;
			}
		}
		if( !bFound )
		{
#if DEBUG_PATHFIND
			Console << "Character stuck!" << myendl;
#endif
			break;
		}
	}
}


//o--------------------------------------------------------------------------o
//|	Function/Class	-	void cMovement::NpcMovement( CChar *i )
//|	Date			-	09/22/2002
//|	Developer(s)	-	Unknown
//|	Company/Team	-	UOX3 DevTeam
//|	Status				-	
//o--------------------------------------------------------------------------o
//|	Description		-	Calculate, and handle NPC AI movement
//|									
//|	Modification	-	09/22/2002	-	Fixed fleeing NPCs by reversing values for 
//|									xfactor & yfactor lines
//o--------------------------------------------------------------------------o
//|	Returns				- NA
//o--------------------------------------------------------------------------o	
void cMovement::NpcMovement( CChar *i )
{
    CChar *kChar;
	UI08 j;
	const R32 npcSpeed	= static_cast< R32 >(cwmWorldState->ServerData()->NPCSpeed());
	bool shouldRun		= false;
    if( i->IsNpc() && ( i->GetTimer( tNPC_MOVETIME ) <= cwmWorldState->GetUICurrentTime() || cwmWorldState->GetOverflow() ) )
    {
#if DEBUG_NPCWALK
		Console.Print( "DEBUG: ENTER (%s): %d AI %d WAR %d J\n", i->GetName(), i->GetNpcWander(), i->IsAtWar(), j );
#endif
		if( i->IsAtWar() && i->GetNpcWander() != 5 )
        {
            CChar *l = i->GetAttacker();
            if( ValidateObject( l ) && ( isOnline( l ) || l->IsNpc() ) )
            {
				UI08 charDir	= Direction( i, l->GetX(), l->GetY() );
				UI16 charDist	= getDist( i, l );
                if( charDir < 8 && ( charDist <= 1 || ( Combat->getCombatSkill( Combat->getWeapon( i ) ) == ARCHERY && charDist <= 3 ) ) )
				{
					i->FlushPath();

					bool los = LineOfSight( NULL, i, l->GetX(), l->GetY(), l->GetZ(), WALLS_CHIMNEYS + DOORS + FLOORS_FLAT_ROOFING );
					if( los && charDir != i->GetDir() )
					{
						i->SetDir( charDir );
						Walking( NULL, i, charDir, 256 );
						return;
					}
				}
				else
                {
					PathFind( i, l->GetX(), l->GetY() );
					j = i->PopDirection();
					shouldRun = (( j&0x80 ) != 0);
					Walking( NULL, i, j, 256 );
                }	
	        }
			else
				i->FlushPath();
        }
        else
        {
            switch( i->GetNpcWander() )
            {
				case 0: // No movement
					break;
				case 1: // Follow the follow target
					kChar = i->GetFTarg();
					if( !ValidateObject( kChar ) ) 
						return;
					if( isOnline( kChar ) || kChar->IsNpc() )
					{
						UI08 charDir = Direction( i, kChar->GetX(), kChar->GetY() );
						if( !objInRange( i, kChar, DIST_NEXTTILE ) && charDir < 8 )
						{
							PathFind( i, kChar->GetX(), kChar->GetY() );
							j = i->PopDirection();
							Walking( NULL, i, j, 256 );
							shouldRun = (( j&0x80 ) != 0);
						}
						// Dupois - Added April 4, 1999
						// Has the Escortee reached the destination ??
						// no need for -1 check on k, as we wouldn't be here if that were true
						if( !kChar->IsDead() && i->GetQuestDestRegion() == i->GetRegionNum() )	// Pay the Escortee and free the NPC
							MsgBoardQuestEscortArrive( i, calcSocketObjFromChar( kChar ) );
					}
					break;
				case 2: // Wander freely, avoiding obstacles.
				case 3: // Wander freely, within a defined box
				case 4: // Wander freely, within a defined circle
					j = RandomNum( 1, 5 );
					if( j == 1 )
						break;
					else if( j == 2 )
						j = RandomNum( 0, 8 );
					else	// Move in the same direction the majority of the time
						j = i->GetDir();
					shouldRun = (( j&0x80 ) != 0);
					NpcWalk( i, j, i->GetNpcWander() );
					break;
				case 5: //FLEE!!!!!!
					kChar = i->GetTarg();
					if( !ValidateObject( kChar ) ) 
						return;
					if( getDist( i, kChar ) < P_PF_MFD )
					{	// calculate a x,y to flee towards
						UI16 mydist = P_PF_MFD - getDist( i, kChar ) + 1;
						j = Direction( i, kChar->GetX(), kChar->GetY() );
						SI16 myx = GetXfromDir( j, i->GetX() );
						SI16 myy = GetYfromDir( j, i->GetY() );

						SI16 xfactor = 0;
						SI16 yfactor = 0;
						// Sept 22, 2002 - Xuri
						if( myx != i->GetX() )
						{
							if( myx < i->GetX() )
								xfactor = 1;
							else
								xfactor = -1;
						}

						if( myy != i->GetY() )
						{
							if( myy < i->GetY() )
								yfactor = 1;
							else
								yfactor = -1;
						}
						//
						myx += (SI16)( xfactor * mydist );
						myy += (SI16)( yfactor * mydist );

						// now, got myx, myy... lets go.
						PathFind( i, myx, myy );
						j = i->PopDirection();
						shouldRun = (( j&0x80 ) != 0);
						Walking( NULL, i, j, 256 );
					}
					else
					{ // wander freely... don't just stop because I'm out of range.
						j = RandomNum( 1, 5 );
						if( j == 1 )
							break;
						else if( j == 2 )
							j = RandomNum( 0, 8 );
						else	// Move in the same direction the majority of the time
        					j = i->GetDir();
						shouldRun = (( j&0x80 ) != 0);
						NpcWalk( i, j, i->GetNpcWander() );
					}
					break;
				case 6:		// Pathfinding!!!!
					if( i->StillGotDirs() )
					{
						j = i->PopDirection();
						shouldRun = (( j&0x80 ) != 0);
						Walking( NULL, i, j, 256 );
					}
					else
						i->SetNpcWander( i->GetOldNpcWander() );
					break;
				default:
					break;
            }
        }
		if( i->GetNpcWander() == 1 )	// Followers need to keep up with Players
			i->SetTimer( tNPC_MOVETIME, BuildTimeValue( static_cast< R32 >(npcSpeed / 4) ) );
		else if( shouldRun )
			i->SetTimer( tNPC_MOVETIME, BuildTimeValue( static_cast< R32 >(npcSpeed / 4) ) );
		else
			i->SetTimer( tNPC_MOVETIME, BuildTimeValue( npcSpeed ) );
    }
}

// This is my attempt at the writing a more effective pathfinding algorithm/sequence

// Function      : cMovement::Distance
// Written by    : Thyme
// Revision Date : 2000.09.08
// Purpose       : Calculate the shortest walkable distance between two points.
// Method        : The methodology behind this is pretty simple actually. The shortest distance
// between two walkable points would be to walk a diagonal line until sx=dx or sy=dy
// and then follow the line until the goal is reached. Since a diagonal step is
// the same distance as a lateral step, there's no need to use the Pythagorean theorem
// in the calculation. This calculation does not take into account any blocking objects.
// It will be used as a heuristic in determining priority of movement.

SI16 cMovement::Distance( SI16 sx, SI16 sy, SI16 dx, SI16 dy )
{
	return (SI16)( ( abs(sx-dx) < abs(sy-dy) ) ? abs(sy-dy) : abs(sx-dx) );
}

// Function      : cMovement::CanCharWalk()
// Written by    : Dupois
// Revised by    : Thyme
// Revision Date : 2000.09.17
// Purpose       : Check if a character can walk to a specified x,y location
// Method        : Modified the old CheckWalkable function so that it can be utilized throughout
// the walking code. Ever wonder why NPCs can walk through walls and stuff in combat mode? This
// is the fix, plus more.

UI08 cMovement::Direction( CChar *mChar, SI16 x, SI16 y )
{
	return Direction( mChar->GetX(), mChar->GetY(), x, y );
}
UI08 cMovement::Direction( SI16 sx, SI16 sy, SI16 dx, SI16 dy )
{
	UI08 dir;
	
	SI16 xdif = (SI16)(dx - sx);
	SI16 ydif = (SI16)(dy - sy);
	
	if( xdif == 0 && ydif < 0 ) 
		dir = NORTH;
	else if( xdif > 0 && ydif < 0 ) 
		dir = NORTHEAST;
	else if( xdif > 0 && ydif == 0 ) 
		dir = EAST;
	else if( xdif > 0 && ydif > 0 ) 
		dir = SOUTHEAST;
	else if( xdif == 0 && ydif > 0 ) 
		dir = SOUTH;
	else if( xdif < 0 && ydif > 0 ) 
		dir = SOUTHWEST;
	else if( xdif < 0 && ydif == 0 ) 
		dir = WEST;
	else if( xdif < 0 && ydif < 0 ) 
		dir = NORTHWEST;
	else 
		dir = UNKNOWNDIR;
	
	return dir;
}

/********************************************************
  Function: cMovement::CheckWalkable

  Description: (knoxos)
    Rewritten checkwalk-function, it calculates new z-position
    for a walking creature (PC or NPC) walks, and checks if 
    movement is blocked.

    This function takes a little more calculation time, than the
    last one, since it walks two times through the static-tile set.
    However at least this one is (more) correct, and these VERy guys 
    now hit their noses on the walls.

    In principle it is the same as the World-kernel in UMelange.
   
  Parameters:  
    CChar *c				Character
	int x, y				new cords.
	int oldx, oldy			old cords.
	bool justask			don't make any changes, the func. is just asked
						  "what if"..

  Return code:
    new z-value        if not blocked
    illegal_z == -128, if walk is blocked

********************************************************/
SI08 cMovement::calc_walk( CChar *c, SI16 x, SI16 y, SI16 oldx, SI16 oldy, bool justask )
{
	if( !ValidateObject( c ) )
		return ILLEGAL_Z;
	const SI08 oldz		= c->GetZ();
	bool may_levitate	= c->MayLevitate();
	bool on_ladder		= false;
	SI08 newz			= ILLEGAL_Z;
	bool blocked		= false;
	char ontype			= 0;
	int xycount			= 0;
	UI08 worldNumber	= c->WorldNumber();
	CTileUni xyblock[XYMAX];
	GetBlockingMap( x, y, xyblock, xycount, oldx, oldy, worldNumber );
	GetBlockingStatics( x, y, xyblock, xycount, worldNumber );
	GetBlockingDynamics( x, y, xyblock, xycount, worldNumber );

	// first calculate newZ value
	for( int i = 0; i < xycount; ++i )
	{
		CTileUni *tb = &xyblock[i]; // this is a easy/little tricky, to save a little calculation
		                                 // since the [i] is calclated several times below
			                             // if it doesn't help, it doesn't hurt either.
		SI08 nItemTop = (SI08)(tb->BaseZ() + ((xyblock[i].Type() == 0) ? xyblock[i].Height() : calcTileHeight( xyblock[i].Height() ) )); // Calculate the items total height

		// check if the creature is floating on a static (keeping Z or falling)
		if( nItemTop >= newz && nItemTop <= oldz )
		{
			if( tb->Standable() )
			{ // walkable tile
				newz = nItemTop;
				ontype = tb->Type();
				if( tb->ClimbableBit2() ) // if it was ladder the char is allowed to `levitate� next move
					on_ladder = true;
				continue;
			}
		}

		// So now comes next step, levitation :o)
		// you can gain Z to a limited amount if yo uwere climbing on last move on a ladder
		if( nItemTop >= newz && may_levitate && nItemTop <= oldz + MAX_Z_LEVITATE && tb->Standable() )
		{
			ontype = tb->Type();
			newz = nItemTop;
			if( tb->ClimbableBit2() ) // if it was ladder the char is allowed to `levitate� next move
				on_ladder = true;
		}
		// check if the creature is climbing on a climbable Z
		// (gaining Z through stairs, ladders, etc)
		// This form has no height limit, and the tile bottom must start lower or
		// equal current height + levitateable limit
		if( nItemTop >= newz && tb->BaseZ() <= oldz + MAX_Z_LEVITATE )
		{
			if( tb->Climbable() || tb->Type() == 0 ||			// Climbable tile, map tiles are also climbable
			( tb->Flag1() == 0 && tb->Flag2() == 0x22 ) ||		// These are a special kind of tiles where OSI forgot to set the climbable flag
			( (nItemTop >= oldz && nItemTop <= oldz + 3) && tb->Standable() ) )		 // Allow to climb a height of 1 even if the climbable flag is not set
			{                 
				ontype = tb->Type();
				newz = nItemTop;
				if( tb->ClimbableBit2() ) // if it was ladder the char is allowed to `levitate� next move
					on_ladder = true;
			}
		}
	}

#if DEBUG_WALKING
		Console.Print( "DEBUG: CheckWalkable calculate Z=%d\n", newz );
#endif
    SI08 item_influence = higher( newz + MAX_ITEM_Z_INFLUENCE, oldz );
	// also take care to look on all tiles the creature has fallen through
	// (npc's walking on ocean bug)
	// now the new Z-cordinate of creature is known, 
	// check if it hits it's head against something (blocking in other words)
	bool isGM = IsGMBody( c );
	for( int ii = 0; ii < xycount; ++ii )
	{
		CTileUni *tb = &xyblock[ii]; 
		SI32 nItemTop = tb->BaseZ() + ( ( tb->Type() == 0) ? tb->Height() : calcTileHeight( tb->Height() ) ); // Calculate the items total height
		if( ( tb->Blocking() || ( tb->Standable() && nItemTop > newz ) ) &&	// Check for blocking tile or stairs
			!( ( isGM || c->IsDead() ) && ( tb->WindowArchDoor() || tb->Door() ) ) )   // ghosts can walk through doors
		{
			// blocking
			if( nItemTop > newz && tb->BaseZ() <= item_influence || ( nItemTop == newz && ontype == 0 ) )
			{ // in effact radius?
				newz = ILLEGAL_Z;
#if DEBUG_WALKING
				Console.Print( "DEBUG: CheckWalkable blocked due to tile=%d at height=%d.\n", xyblock[ii].ID(), xyblock[ii].BaseZ() );
#endif
				blocked = true;
				break;
			}
		}
// knoxos: MAX_ITEM_Z_INFLUENCE is nice, but not truely correct,
//         since the creature height should be the effect radius, if you are i.e.
//         polymorphed to a "slime", you could go through things you normally 
//         wouldn't get under. (Just leaves the question what happens if you
//         unpolymorph in a place where you can't fit, lucky there are no
//         such gaps or tunnels in Britannia). 
//         (Well UO isn't ment to really think in 3d)
	}

#if DEBUG_WALKING
	Console.Print( "DEBUG: CanCharWalk: %dx %dy %dz\n", x, y, z );
#endif
	if( (newz > ILLEGAL_Z) && (!justask) ) // save information if we have climbed on last move.
		c->SetLevitate( on_ladder );
	return newz;
}

// Function      : cMovement::CanCharMove()
// Written by    : Thyme
// Revision Date : 2000.09.17
// Purpose       : Check if a character can walk to a from x,y to dir direction
// Method        : This handles the funky diagonal moves.

bool cMovement::calc_move( CChar *c, SI16 x, SI16 y, SI08 &z, UI08 dir)
{
	if( ( dir & 0x07 ) % 2 )
	{ // check three ways.
		UI08 ndir = turn_counter_clock_wise( dir );
		if( calc_walk( c, GetXfromDir( ndir, x ), GetYfromDir( ndir, y ), x, y, true ) == ILLEGAL_Z )
			return false;
		ndir = turn_clock_wise( dir );
		if( calc_walk( c, GetXfromDir( ndir, x ), GetYfromDir( ndir, y ), x, y, true ) == ILLEGAL_Z )
			return false;
	}
	z = calc_walk( c, GetXfromDir( dir, x ), GetYfromDir( dir, y ), x, y, false );
	return (z > ILLEGAL_Z);
}

// knox, reinserted it since some other files access it,
//       100% sure this is wrong, however the smaller ill.
bool cMovement::validNPCMove( SI16 x, SI16 y, SI08 z, CChar *s )
{
	UI08 worldNumber = s->WorldNumber();
    SubRegion *cell = MapRegion->GetCell( x, y, worldNumber );
	if( cell == NULL )
		return true;
	CDataList< CItem * > *regItems = cell->GetItemList();
	regItems->Push();
	for( CItem *tItem = regItems->First(); !regItems->Finished(); tItem = regItems->Next() )
	{
		if( !ValidateObject( tItem ) )
			continue;
		CTile tile;
		Map->SeekTile( tItem->GetID(), &tile );
		if( tItem->GetX() == x && tItem->GetY() == y && tItem->GetZ() + tile.Height() > z + 1 && tItem->GetZ() < z + MAX_Z_STEP )
		{
			UI16 id = tItem->GetID();
			if( id == 0x3946 || id == 0x3956 )
			{
				regItems->Pop();
				return false;
			}
			if( id <= 0x0200 || id >= 0x0300 && id <= 0x03E2 ) 
			{
				regItems->Pop();
				return false;
			}
			if( id > 0x0854 && id < 0x0866 ) 
			{
				regItems->Pop();
				return false;
			}
        
			if( tItem->GetType() == IT_DOOR )
			{
				if( s->IsNpc() && ( !s->GetTitle().empty() || s->GetNPCAiType() != aiNOAI ) )
					useDoor( NULL, tItem );
				regItems->Pop();
				return false;
			}
		}
	}
	regItems->Pop();

    // see if the map says its ok to move here
    if( Map->CanMonsterMoveHere( x, y, z, worldNumber ) )
		return true;
    return false;
}

void cMovement::deny( CSocket *mSock, CChar *s, SI16 sequence )
{
	CPWalkDeny denPack;

	denPack.SequenceNumber( (SI08)sequence );
	denPack.X( s->GetX() );
	denPack.Y( s->GetY() );
	denPack.Direction( s->GetDir() );
	denPack.Z( s->GetZ() );

	if( mSock != NULL )
	{
		mSock->Send( &denPack );
		mSock->WalkSequence( -1 );
	}
}

}
