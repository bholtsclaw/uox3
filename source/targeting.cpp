#include "uox3.h"
#include "cdice.h"
#include "skills.h"
#include "cGuild.h"
#include "townregion.h"
#include "cServerDefinitions.h"
#include "commands.h"
#include "cMagic.h"
#include "ssection.h"
#include "gump.h"
#include "trigger.h"
#include "cScript.h"
#include "cEffects.h"
#include "CPacketSend.h"
#include "classes.h"
#include "regions.h"
#include "combat.h"
#include "magic.h"
#include "Dictionary.h"

#include "ObjectFactory.h"

#undef DBGFILE
#define DBGFILE "targeting.cpp"

namespace UOX
{

void tweakItemMenu( cSocket *s, CItem *j );
void tweakCharMenu( cSocket *s, CChar *c );
void OpenPlank( CItem *p );
void MakeShop( CChar *c );

void PlVBuy( cSocket *s )//PlayerVendors
{
	VALIDATESOCKET( s );

	CChar *vChar = static_cast<CChar *>(s->TempObj());
	s->TempObj( NULL );
	if( !ValidateObject( vChar ) || vChar->isFree() ) 
		return;

	CChar *mChar	= s->CurrcharObj();
	UI32 gleft		= GetItemAmount( mChar, 0x0EED );

	CItem *p		= mChar->GetPackItem();
	if( !ValidateObject( p ) ) 
	{
		s->sysmessage( 773 ); 
		return; 
	}

	CItem *i = calcItemObjFromSer( s->GetDWord( 7 ) );
	if( !ValidateObject( i ) || i->GetCont() == NULL ) 
		return;
	
	if( FindItemOwner( i ) != vChar )
		return;
	if( vChar->GetNPCAiType() != aiPLAYERVENDOR )
		return;
	if( mChar == vChar->GetOwnerObj() )
	{
		vChar->talk( s, 999, false );
		return;
	}

	if( gleft < i->GetBuyValue() ) 
	{
		vChar->talk( s, 1000, false );
		return;
	} 
	else 
	{
		UI32 tAmount = DeleteItemAmount( mChar, i->GetBuyValue(), 0x0EED );
		// tAmount > 0 indicates there wasn't enough money...
		// could be expanded to take money from bank too...
	}
	
	vChar->talk( s, 1001, false );
	vChar->SetHoldG( vChar->GetHoldG() + i->GetBuyValue() );

	i->SetCont( p );	// move containers
	s->statwindow( mChar );
}

void HandleGuildTarget( cSocket *s )
{
	VALIDATESOCKET( s );
	CChar *trgChar	= NULL;
	CChar *mChar	= s->CurrcharObj();
	CGuild *mGuild	= NULL, *tGuild = NULL;
	switch( s->GetByte( 5 ) )
	{
		case 0:	// recruit character
			trgChar = calcCharObjFromSer( s->GetDWord( 7 ) );
			if( ValidateObject( trgChar ) )
			{
				if( trgChar->GetGuildNumber() == -1 )	// no existing guild
				{
					mGuild = GuildSys->Guild( mChar->GetGuildNumber() );
					if( mGuild != NULL )
						mGuild->NewRecruit( (*trgChar) );
				}
				else
					s->sysmessage( 1002 );
			}
			break;
		case 1:		// declare fealty
			trgChar = calcCharObjFromSer( s->GetDWord( 7 ) );
			if( ValidateObject( trgChar ) )
			{
				if( trgChar->GetGuildNumber() == mChar->GetGuildNumber() )	// same guild
					mChar->SetGuildFealty( trgChar->GetSerial() );
				else
					s->sysmessage( 1003 );
			}
			break;
		case 2:	// declare war
			trgChar = calcCharObjFromSer( s->GetDWord( 7 ) );
			if( ValidateObject( trgChar ) )
			{
				if( trgChar->GetGuildNumber() != mChar->GetGuildNumber() )
				{
					if( trgChar->GetGuildNumber() == -1 )
						s->sysmessage( 1004 );
					else
					{
						mGuild = GuildSys->Guild(mChar->GetGuildNumber() );
						if( mGuild != NULL )
						{
							mGuild->SetGuildRelation( trgChar->GetGuildNumber(), GR_WAR );
							tGuild = GuildSys->Guild( trgChar->GetGuildNumber() );
							if( tGuild != NULL )
								tGuild->TellMembers( 1005, mGuild->Name().c_str() );
						}
					}
				}
				else
					s->sysmessage( 1006 );
			}
			break;
		case 3:	// declare ally
			trgChar = calcCharObjFromSer( s->GetDWord( 7 ) );
			if( ValidateObject( trgChar ) )
			{
				if( trgChar->GetGuildNumber() != mChar->GetGuildNumber() )
				{
					if( trgChar->GetGuildNumber() == -1 )
						s->sysmessage( 1004 );
					else
					{
						mGuild = GuildSys->Guild( mChar->GetGuildNumber() );
						if( mGuild != NULL )
						{
							mGuild->SetGuildRelation( trgChar->GetGuildNumber(), GR_ALLY );
							tGuild = GuildSys->Guild( trgChar->GetGuildNumber() );
							if( tGuild != NULL )
								tGuild->TellMembers( 1007, mGuild->Name().c_str() );
						}
					}
				}
				else
					s->sysmessage( 1006 );
			}
			break;
	}

}

void HandleSetScpTrig( cSocket *s )
{
	VALIDATESOCKET( s );
	UI16 targTrig		= (UI16)s->TempInt();
	cScript *toExecute	= Trigger->GetScript( targTrig );
	if( toExecute == NULL )
	{
		s->sysmessage( 1752 );
		return;
	}

	SERIAL ser = s->GetDWord( 7 );
	if( ser < BASEITEMSERIAL )
	{	// character
		CChar *targChar = calcCharObjFromSer( ser );
		if( ValidateObject( targChar ) )
		{
			targChar->SetScriptTrigger( targTrig );
			s->sysmessage( 1653 );
		}
	}
	else
	{	// item
		CItem *targetItem = calcItemObjFromSer( ser );
		if( ValidateObject( targetItem ) )
		{
			s->sysmessage( 1652 );
			targetItem->SetScriptTrigger( targTrig );
		}
	}
}

void DoAddTarget( cSocket *mSock, CItem *mItem )
{
	const SI16 tX = mSock->GetWord( 11 );
	const SI16 tY = mSock->GetWord( 13 );
	const SI08 tZ = static_cast<SI08>(mSock->GetByte( 16 ) + Map->TileHeight( mSock->GetWord( 17 ) ));
	
	if( mItem->GetCont() == NULL )
		mItem->SetLocation( tX, tY, tZ );
	else
	{
		mItem->SetX( tX );
		mItem->SetY( tY );
		mItem->SetZ( tZ );
	}
}
void BuildHouse( cSocket *s, UI08 houseEntry );
void BuildHouseTarget( cSocket *s )
{
	VALIDATESOCKET( s );
	if( s->GetDWord( 11 ) == INVALIDSERIAL )
		return;

	BuildHouse( s, s->AddID1() );//If its a valid house, send it to buildhouse!

	s->AddID1( 0 );
}

//o---------------------------------------------------------------------------o
//|   Function    -  void AddTarget( cSocket *s)
//|   Date        -  UnKnown
//|   Programmer  -  UnKnown  (Touched tabstops by Tauriel Dec 29, 1998)
//o---------------------------------------------------------------------------o
//|   Purpose     -  Adds an item when using /add # # .
//o---------------------------------------------------------------------------o
void AddItem( cSocket *s, ObjectType itemType = OT_ITEM )
{
	VALIDATESOCKET( s );
	if( s->GetDWord( 11 ) == INVALIDSERIAL )
		return;

	UI16 modelNum	= static_cast<UI16>(( ( s->AddID1() )<<8 ) + s->AddID2());
	CItem *c		= Items->CreateItem( NULL, s->CurrcharObj(), modelNum, 1, 0, itemType );
	if( c == NULL ) 
		return;
	
	DoAddTarget( s, c );

	s->AddID1( 0 );
	s->AddID2( 0 );
}

void AddScriptItem( cSocket *s, ObjectType itemType = OT_ITEM ) //Tauriel 11-22-98 updated for new items
{
	VALIDATESOCKET( s );
	if( s->GetDWord( 11 ) == INVALIDSERIAL ) 
		return;

	CItem *c		= Items->CreateScriptItem( s, s->CurrcharObj(), s->XText(), 1, itemType );
	if( c == NULL )
		return;

	DoAddTarget( s, c );
}

void AddNpc( cSocket *s )
{
	VALIDATESOCKET( s );
	if( s->GetDWord( 11 ) == INVALIDSERIAL )
		return;

	CChar *npc = static_cast< CChar * >(ObjectFactory::getSingleton().CreateObject( OT_CHAR ));
	if( npc == NULL )
		return;

	npc->SetName( "Dummy" );
	npc->SetID( ( ( s->AddID1() <<8 ) + s->AddID2()) );
	npc->SetOrgID( npc->GetID() );
	npc->SetSkin( 0 );
	npc->SetSkillTitles( true );
	npc->SetLocation( s->GetWord( 11 ), s->GetWord( 13 ), s->GetByte( 16 ) + Map->TileHeight( s->GetWord( 17 ) ) );
	npc->SetNpc( true );
}

void AddScriptNpc( cSocket *s )
// Abaddon 17th February, 2000
// Need to return the character we've made, else summon creature at least will fail
// We make the char, but never pass it back up the chain
{
	VALIDATESOCKET( s );
	if( s->GetDWord( 11 ) == INVALIDSERIAL )
		return;
	
	CChar *mChar			= s->CurrcharObj();
	const SI16 coreX		= s->GetWord( 11 );
	const SI16 coreY		= s->GetWord( 13 );
	const SI08 coreZ		= static_cast<SI08>(s->GetByte( 16 ) + Map->TileHeight( s->GetWord( 17 ) ));
	CChar *cCreated			= Npcs->CreateNPCxyz( s->XText(), coreX, coreY, coreZ, mChar->WorldNumber() );
}

void TeleTarget( cSocket *s )
{
	VALIDATESOCKET( s );
	if( s->GetDWord( 11 ) == INVALIDSERIAL )
		return;

	const SERIAL serial = s->GetDWord( 7 );

	cBaseObject *mObj = NULL;
	if( serial >= BASEITEMSERIAL )
		mObj = calcItemObjFromSer( serial );
	else
		mObj = calcCharObjFromSer( serial );

	SI16 targX, targY;
	SI08 targZ;
	if( ValidateObject( mObj ) )
	{
		targX = mObj->GetX();
		targY = mObj->GetY();
		targZ = mObj->GetZ();
	}
	else
	{
		targX = s->GetWord( 11 );
		targY = s->GetWord( 13 );
		targZ = (SI08)(s->GetByte( 16 ) + Map->TileHeight( s->GetWord( 17 ) ));
	}
	CChar *mChar = s->CurrcharObj();

	if( mChar->IsGM() || LineOfSight( s, mChar, targX, targY, targZ, WALLS_CHIMNEYS + DOORS + ROOFING_SLANTED ) )	
	{
		if( s->CurrentSpellType() != 2 )  // not a wand cast
		{
			Magic->SubtractMana( mChar, 3 );  // subtract mana on scroll or spell
			if( s->CurrentSpellType() == 0 )             // del regs on normal spell
			{
				reag_st toDel;
				toDel.drake = 1;
				toDel.moss = 1;
				Magic->DelReagents( mChar, toDel );
			}
		}
		
		Effects->PlaySound( s, 0x01FE, true );
		
		mChar->SetLocation( targX, targY, targZ );
		Effects->PlayStaticAnimation( mChar, 0x372A, 0x09, 0x06 );
	} 
}

void DyeTarget( cSocket *s )
{
	VALIDATESOCKET( s );
	int b;
	UI16 colour, k;
	CItem *i		= NULL;
	CChar *c		= NULL;
	SERIAL serial	= s->GetDWord( 7 );
	if( s->AddID1() == 0xFF && s->AddID2() == 0xFF )
	{
		CPDyeVat toSend;
		if( serial >= BASEITEMSERIAL )
		{
			i = calcItemObjFromSer( serial );
			if( ValidateObject( i ) )
			{
				toSend = (*i);
				s->Send( &toSend );
			}
		}
		else
		{
			c = calcCharObjFromSer( serial );
			if( ValidateObject( c ) )
			{
				toSend = (*c);
				toSend.Model( 0x2106 );
				s->Send( &toSend );
			}
		}
	}
	else
	{
		if( serial >= BASEITEMSERIAL )
		{
			i = calcItemObjFromSer( serial );
			if( !ValidateObject( i ) )
				return;
			colour = (UI16)(( (s->AddID1())<<8 ) + s->AddID2());
			if( !s->DyeAll() )
			{
				if( colour < 0x0002 || colour > 0x03E9 )
					colour = 0x03E9;
			}
			
			b = ((colour&0x4000)>>14) + ((colour&0x8000)>>15);   
			if( !b )
				i->SetColour( colour );
		}
		else
		{
			c = calcCharObjFromSer( serial );
			if( !ValidateObject( c ) )
				return;
			UI16 body = c->GetID();
			k = (UI16)(( ( s->AddID1() )<<8 ) + s->AddID2());
			
			b = k&0x4000; 
			if( b == 16384 && ( body >= 0x0190 && body <= 0x03E1 ) ) 
				k = 0xF000; // but assigning the only "transparent" value that works, namly semi-trasnparency.
			
			if( k != 0x8000 ) // 0x8000 also crashes client ...
			{	
				c->SetSkin( k );
				c->SetOrgSkin( k );
			}
		}
	}
}

void KeyTarget( cSocket *s )
{
	VALIDATESOCKET( s );
	CItem *i = calcItemObjFromSer( s->GetDWord( 7 ) );
	if( !ValidateObject( i ) )
		return;

	CChar *mChar		= s->CurrcharObj();
	SERIAL moreSerial	= s->AddID();
	if( i->GetTempVar( CITV_MORE ) == 0 )
	{       
		if( i->GetType() == IT_KEY && objInRange( mChar, i, DIST_NEARBY ) )
		{
			if( !Skills->CheckSkill( mChar, TINKERING, 400, 1000 ) ) 
			{
				s->sysmessage( 1016 );
				i->Delete();
				return;
			}
			i->SetTempVar( CITV_MORE, moreSerial );
			s->sysmessage( 1017 );
		}
	}
	else if( i->GetTempVar( CITV_MORE ) == moreSerial || s->AddID1() == 0xFF )
	{
		if( objInRange( mChar, i, DIST_NEARBY ) )
		{
			switch( i->GetType() )
			{
			case IT_CONTAINER:
			case IT_SPAWNCONT:
				if( i->GetType() == IT_CONTAINER ) 
					i->SetType( IT_LOCKEDCONTAINER );
				else if( i->GetType() == IT_SPAWNCONT ) 
					i->SetType( IT_LOCKEDSPAWNCONT );
				s->sysmessage( 1018 );
				break;
			case IT_KEY:
				mChar->SetSpeechItem( i );
				mChar->SetSpeechMode( 5 );
				s->sysmessage( 1019 );
				break;
			case IT_LOCKEDCONTAINER:
			case IT_LOCKEDSPAWNCONT:
				if( i->GetType() == IT_LOCKEDCONTAINER ) 
					i->SetType( IT_CONTAINER );
				if( i->GetType() == IT_LOCKEDSPAWNCONT ) 
					i->SetType( IT_SPAWNCONT );
				s->sysmessage( 1020 );
				break;
			case IT_DOOR:
				i->SetType( IT_LOCKEDDOOR );
				s->sysmessage( 1021 );
				Effects->PlaySound( s, 0x0049, true );
				break;
			case IT_LOCKEDDOOR:
				i->SetType( IT_DOOR );
				s->sysmessage( 1022 );
				Effects->PlaySound( s, 0x0049, true );
				break;
			case IT_HOUSESIGN:
				s->sysmessage( 1023 );
				mChar->SetSpeechMode( 8 );
				mChar->SetSpeechItem( i );
				break;
			case IT_PLANK:
				OpenPlank( i );
				s->sysmessage( "You open the plank" );
				break;
			}
		}
		else
			s->sysmessage( 393 );
	}
	else
	{
		if( i->GetType() == IT_KEY ) 
			s->sysmessage( 1024 );
		else if( i->GetTempVar( CITV_MORE, 1 ) == 0 ) 
			s->sysmessage( 1025 );
		else 
			s->sysmessage( 1026 );
	}
}

void WstatsTarget( cSocket *s )
{
	VALIDATESOCKET( s );
	CChar *i = calcCharObjFromSer( s->GetDWord( 7 ) );
	if( !ValidateObject( i ) )
		return;
	GumpDisplay wStat( s, 300, 300 );
	wStat.SetTitle( "Walking Stats" );
	SERIAL charSerial = i->GetSerial();
	UI16 charID = i->GetID();
	wStat.AddData( "Serial", charSerial, 3 );
	wStat.AddData( "Body ID", charID, 5 );
	wStat.AddData( "Name", i->GetName() );
	wStat.AddData( "X", i->GetX() );
	wStat.AddData( "Y", i->GetY() );
	char temp[15];
	sprintf( temp, "%d/%d", i->GetZ(), i->GetDispZ() );
	wStat.AddData( "Z/DispZ", temp );
	wStat.AddData( "Wander", i->GetNpcWander() );
	wStat.AddData( "FX1", i->GetFx( 0 ) );
	wStat.AddData( "FY1", i->GetFy( 0 ) );
	wStat.AddData( "FZ1", i->GetFz() );
	wStat.AddData( "FX2", i->GetFx( 1 ) );
	wStat.AddData( "FY2", i->GetFy( 1 ) );
	wStat.Send( 4, false, INVALIDSERIAL );
}

void ColorsTarget( cSocket *s )
{
	VALIDATESOCKET( s );
	CItem *i = calcItemObjFromSer( s->GetDWord( 7 ) );
	if( !ValidateObject( i ) )
		return;
	if( i->GetID() == 0x0FAB || i->GetID() == 0x0EFF || i->GetID() == 0x0E27 )	// dye vat, hair dye
	{
		CPDyeVat toSend = (*i);
		s->Send( &toSend );
	}
	else
		s->sysmessage( 1031 );
}

void DvatTarget( cSocket *s )
{
	VALIDATESOCKET( s );
	SERIAL serial	= s->GetDWord( 7 );
	CItem *i		= calcItemObjFromSer( serial );
	if( !ValidateObject( i ) )
		return;

	CChar *mChar = s->CurrcharObj();
	if( i->isDyeable() )
	{
		if( i->GetCont() != NULL )
		{
			CChar *c = FindItemOwner( i );
			if( ValidateObject( c ) && c != mChar )
			{
				s->sysmessage( 1032 );
				return;
			}
		}
		i->SetColour( ( ( s->AddID1() )<<8) + s->AddID2() );
		Effects->PlaySound( s, 0x023E, true );
	}
	else
		s->sysmessage( 1033 );
}

void InfoTarget( cSocket *s )
{
	VALIDATESOCKET( s );
	if( s->GetDWord( 11 ) == INVALIDSERIAL )
		return;

	map_st map1;
	CLand land;
	
	SI16 x = s->GetWord( 0x0B );
	SI16 y = s->GetWord( 0x0D );
//	SI08 z = s->GetByte( 0x10 );

	UI08 worldNumber = 0;
	CChar *mChar = s->CurrcharObj();
	if( ValidateObject( mChar ) )
		worldNumber = mChar->WorldNumber();
	if( s->GetWord( 0x11 ) == 0 )
	{
		// manually calculating the ID's if it's a maptype
		map1 = Map->SeekMap0( x, y, worldNumber );
		Map->SeekLand( map1.id, &land );
		GumpDisplay mapStat( s, 300, 300 );
		mapStat.SetTitle( "Map Tile" );

		mapStat.AddData( "Tilenum", map1.id );
		mapStat.AddData( "Flag1", land.Flag1(), 1 );
		mapStat.AddData( "Flag2", land.Flag2(), 1 );
		mapStat.AddData( "Flag3", land.Flag3(), 1 );
		mapStat.AddData( "Flag4", land.Flag4(), 1 );
		mapStat.AddData( "Unknown1", land.Unknown1(), 1 );
		mapStat.AddData( "Unknown2", land.Unknown2(), 1 );
		mapStat.AddData( "Name", land.Name() );
		mapStat.Send( 4, false, INVALIDSERIAL );
	} 
	else
	{
		CTile tile;
		UI16 tilenum = s->GetWord( 0x11 );
		Map->SeekTile( tilenum, &tile );

		GumpDisplay statTile( s, 300, 300 );
		statTile.SetTitle( "Map Tile" );

		statTile.AddData( "Tilenum", tilenum );
		statTile.AddData( "Flag1", tile.Flag1(), 1 );
		statTile.AddData( "Flag2", tile.Flag2(), 1 );
		statTile.AddData( "Flag3", tile.Flag3(), 1 );
		statTile.AddData( "Flag4", tile.Flag4(), 1 );
		statTile.AddData( "Weight", tile.Weight() );
		statTile.AddData( "Layer", tile.Layer(), 1 );
		statTile.AddData( "Anim", tile.Animation(), 1 );
		statTile.AddData( "Unknown1", tile.Unknown1(), 1 );
		statTile.AddData( "Unknown2", tile.Unknown2(), 1 );
		statTile.AddData( "Unknown3", tile.Unknown3(), 1 );
		statTile.AddData( "Height", tile.Height(), 1 );
		statTile.AddData( "Name", tile.Name() );
		statTile.Send( 4, false, INVALIDSERIAL );
	}
	s->sysmessage( 1034 );
}

void TweakTarget( cSocket *s )
{
	VALIDATESOCKET( s );
	SERIAL serial	= s->GetDWord( 7 );
	CChar *c		= calcCharObjFromSer( serial );
	if( ValidateObject( c ) )
		tweakCharMenu( s, c );
	else 
	{
		CItem *i = calcItemObjFromSer( serial );
		if( ValidateObject( i ) )
			tweakItemMenu( s, i );
	}
}

void LoadCannon( cSocket *s )
{
	VALIDATESOCKET( s );
	CItem *i = calcItemObjFromSer( s->GetDWord( 7 ) );
	if( !ValidateObject( i ) )
		return;
	SERIAL moreSerial = s->AddID();
	if( i->GetTempVar( CITV_MORE ) == moreSerial || s->AddID1() == 0xFF )
	{
		if( i->GetTempVar( CITV_MOREZ ) == 0 && objInRange( s, i, DIST_NEARBY ) )
		{
			i->SetTempVar( CITV_MOREZ, 1 );
			s->sysmessage( 1035 );
		}
		else
		{
			if( i->GetTempVar( CITV_MORE, 1 ) == 0x00 ) 
				s->sysmessage( 1036 );
			else 
				s->sysmessage( 1037 );
		}
	}
}

void Tiling( cSocket *s )  // Clicking the corners of tiling calls this function - Crwth 01/11/1999
{
	VALIDATESOCKET( s );
	if( s->GetDWord( 11 ) == INVALIDSERIAL )
		return;

	if( s->ClickX() == -1 && s->ClickY() == -1 )
	{
		s->ClickX( s->GetWord( 11 ) );
		s->ClickY( s->GetWord( 13 ) );
		s->target( 0, TARGET_TILING, 1038 );
		return;
	}

	SI16 x1 = s->ClickX(), x2 = s->GetWord( 11 );
	SI16 y1 = s->ClickY(), y2 = s->GetWord( 13 );
	SI16 j;

	s->ClickX( -1 );
	s->ClickY( -1 );
	
	if( x1 > x2 ) 
	{ 
		j = x1;
		x1 = x2;
		x2 = j;
	}
	if( y1 > y2 ) 
	{ 
		j = y1; 
		y1 = y2; 
		y2 = j;
	}
	
	UI16 addid = (UI16)(( ( s->AddID1() ) << 8 ) + s->AddID2());

	CItem *c = NULL;
	for( SI16 x = x1; x <= x2; ++x )
	{
		for( SI16 y = y1; y <= y2; ++y ) 
		{
			c = Items->CreateItem( NULL, s->CurrcharObj(), addid, 1, 0, OT_ITEM );
			if( c == NULL ) 
				return;
			c->SetLocation( x, y, s->GetByte( 16 ) + Map->TileHeight( s->GetWord( 17 ) ) );
		}
	}
	s->AddID1( 0 );
	s->AddID2( 0 );
}

enum AreaCommandTypes
{
	ACT_DYE = 0,
	ACT_WIPE,
	ACT_INCX,
	ACT_INCY,
	ACT_INCZ,
	ACT_SETX,
	ACT_SETY,
	ACT_SETZ,
	ACT_SETTYPE,
	ACT_NEWBIE,
	ACT_SETSCPTRIG,
	ACT_MOVABLE,
	INVALID_CMD,
	ACT_CMDCOUNT
};

bool AreaCommandFunctor( cBaseObject *a, UI32 &b, void *extraData )
{
	UI32 *ourData = (UI32 *)extraData;
	UI32 x1			= ourData[0];
	UI32 x2			= ourData[1];
	UI32 y1			= ourData[2];
	UI32 y2			= ourData[3];
	UString *value	= (UString *)(ourData[4]);
	if( ValidateObject( a ) && a->CanBeObjType( OT_ITEM ) )
	{
		CItem *i = static_cast< CItem * >(a);
		if( i->GetCont() != NULL )
			return true;
		if( i->GetX() >= ourData[0] && i->GetX() <= ourData[1] && i->GetY() >= ourData[2] && i->GetY() <= ourData[3] )
		{
			switch( b )
			{
				case ACT_DYE:			i->SetColour( value->toUShort() );							break;	// dye
				case ACT_WIPE:			i->Delete();												break;	// wipe
				case ACT_INCX:			i->IncLocation( value->toShort(), 0 );						break;	// incx
				case ACT_INCY:			i->IncLocation( 0, value->toShort() );						break;	// incy
				case ACT_INCZ:			i->IncZ( value->toByte() );									break;	// incz
				case ACT_SETX:			i->SetLocation( value->toShort(), i->GetY(), i->GetZ() );	break;	// setx
				case ACT_SETY:			i->SetLocation( i->GetX(), value->toShort(), i->GetZ() );	break;	// sety
				case ACT_SETZ:			i->SetZ( value->toByte() );									break;	// setz
				case ACT_SETTYPE:		i->SetType( static_cast< ItemTypes >(value->toUByte()) );	break;	// settype
				case ACT_NEWBIE:		i->SetNewbie( value->toUShort() != 0 );						break;	// newbie
				case ACT_SETSCPTRIG:	i->SetScriptTrigger( value->toUShort() );					break;	// set script #
				case ACT_MOVABLE:		i->SetMovable( value->toByte() );							break;	// movable property
				default:																			break;
			}
			// process command here!
		}
	}
	return true;
}
void AreaCommand( cSocket *s )
{
	VALIDATESOCKET( s );
	if( s->GetDWord( 11 ) == INVALIDSERIAL )
		return;
	
	if( s->ClickX() == -1 && s->ClickY() == -1 )
	{
		s->ClickX( s->GetWord( 11 ) );
		s->ClickY( s->GetWord( 13 ) );
		s->target( 0, TARGET_AREACOMMAND, 1040 );
		return;
	}
	
	SI16 x1 = s->ClickX(), x2 = s->GetWord( 11 );
	SI16 y1 = s->ClickY(), y2 = s->GetWord( 13 );
	
	s->ClickX( -1 );
	s->ClickY( -1 );
	
	SI16 c;
	
	if( x1 > x2 ) 
	{ 
		c = x1;
		x1 = x2;
		x2 = c;
	}
	if( y1 > y2 ) 
	{ 
		c = y1; 
		y1 = y2; 
		y2 = c;
	}

	const std::string areaCommandStrings[ACT_CMDCOUNT] =
	{
		"dye",
		"wipe",
		"incx",
		"incy",
		"incz",
		"setx",
		"sety",
		"setz",
		"settype",
		"newbie",
		"setscptrig",
		"movable",
		""
	};

	AreaCommandTypes cmdType = INVALID_CMD;
	UString orgString( s->XText() );
	UString key, value;

	if( orgString.sectionCount( " " ) != 0 )
	{
		key		= orgString.section( " ", 0, 0 ).stripWhiteSpace();
		value	= orgString.section( " ", 1, 1 ).stripWhiteSpace();
	}
	else
		key = orgString;

	for( AreaCommandTypes k = ACT_DYE; k < INVALID_CMD; k = (AreaCommandTypes)((int)k + 1) )
	{
		if( key == areaCommandStrings[k] )
		{
			cmdType = k;
			break;
		}
	}

	if( cmdType == INVALID_CMD )
	{
		s->sysmessage( 1753 );
		return;
	}

	UI32 toPass[5];
	toPass[0]	= x1;
	toPass[1]	= x2;
	toPass[2]	= y1;
	toPass[3]	= y2;
	toPass[4]	= (UI32)&value;
	UI32 b		= cmdType;
	ObjectFactory::getSingleton().IterateOver( OT_ITEM, b, toPass, &AreaCommandFunctor );
}

bool WipeFunctor( cBaseObject *a, UI32 &b, void *extraData )
{
	UI32 *ourData = (UI32 *)extraData;
	UI32 x1			= ourData[0];
	UI32 x2			= ourData[1];
	UI32 y1			= ourData[2];
	UI32 y2			= ourData[3];
	bool wipeVal	= (ourData[4] == 1);
	bool shouldWipe	= wipeVal;
	if( ValidateObject( a ) && a->CanBeObjType( OT_ITEM ) )
	{
		CItem *i = static_cast< CItem * >(a);
		if( i->isWipeable() && i->GetCont() == NULL )
		{
			UI16 iX = i->GetX();
			UI16 iY = i->GetY();
			if( iX >= x1 && iX <= x2 && iY >= y1 && iY <= y2 )
				shouldWipe = !wipeVal;
			if( shouldWipe )
				i->Delete();
		}
	}
	return true;
}

void Wiping( cSocket *s )  // Clicking the corners of wiping calls this function - Crwth 01/11/1999
{
	VALIDATESOCKET( s );
	if( s->GetDWord( 11 ) == INVALIDSERIAL )
		return;
	
	if( s->ClickX() == -1 && s->ClickY() == -1 )
	{
		s->ClickX( s->GetWord( 11 ) );
		s->ClickY( s->GetWord( 13 ) );
		if( s->AddID1() ) 
			s->target( 0, TARGET_WIPING, 1039 );
		else 
			s->target( 0, TARGET_WIPING, 1040 );
		return;
	}
	
	SI16 x1 = s->ClickX(), x2 = s->GetWord( 11 );
	SI16 y1 = s->ClickY(), y2 = s->GetWord( 13 );
	
	SI16 c;
	
	if( x1 > x2 ) 
	{ 
		c = x1;
		x1 = x2;
		x2 = c;
	}
	if( y1 > y2 ) 
	{ 
		c = y1; 
		y1 = y2; 
		y2 = c;
	}

	bool iWipe = (s->AddID1() == 1), shouldWipe = iWipe;
	UI32 toPass[5];
	toPass[0]	= x1;
	toPass[1]	= x2;
	toPass[2]	= y1;
	toPass[3]	= y2;
	toPass[4]	= (iWipe ? 1 : 0);
	UI32 b		= 0;
	ObjectFactory::getSingleton().IterateOver( OT_ITEM, b, toPass, &WipeFunctor );
	s->ClickX( -1 );
	s->ClickY( -1 );
	s->AddID1( 0 );
}

//o--------------------------------------------------------------------------o
//|	Function/Class	-	void newCarveTarget( cSocket *s, CItem *i )
//|	Date			-	09/22/2002
//|	Developer(s)	-	Unknown
//|	Company/Team	-	UOX3 DevTeam
//o--------------------------------------------------------------------------o
//|	Description		-	Target carving system.
//|									
//|	Modification	-	unknown   	-	Human-corpse carving code added
//|									
//|	Modification	-	unknown   	-	Scriptable carving product added
//|									
//|	Modification	-	09/22/2002	-	Xuri - Fixed erroneous names for body parts 
//|									& made all body parts that are carved from human corpse	
//|									lie in same direction.
//o--------------------------------------------------------------------------o
void newCarveTarget( cSocket *s, CItem *i )
{
	VALIDATESOCKET( s );
	bool deletecorpse = false;
	CChar *mChar = s->CurrcharObj();
	CItem *c = Items->CreateItem( NULL, mChar, 0x122A, 1, 0, OT_ITEM ); // add the blood puddle
	if( c == NULL ) 
		return;
	c->SetLocation( i );
	c->SetMovable( 2 );
	c->SetDecayTime( BuildTimeValue( static_cast<R32>(cwmWorldState->ServerData()->SystemTimer( DECAY ) )) );

	char temp[1024];
	// if it's a human corpse
	// Sept 22, 2002 - Xuri - Corrected the alignment of body parts that are carved.
	if( i->GetTempVar( CITV_MOREY ) )
	{
		// create the Head
		sprintf( temp, Dictionary->GetEntry( 1058 ).c_str(), i->GetName2() );
		c = Items->CreateItem( NULL, mChar, 0x1DAE, 1, 0, OT_ITEM );
		if( c == NULL ) 
			return;
		c->SetName( temp );
		c->SetLocation( i );
		c->SetOwner( i->GetOwnerObj() );

		// create the Body
		sprintf( temp, Dictionary->GetEntry( 1059 ).c_str(), i->GetName2() );
		c = Items->CreateItem( NULL, mChar, 0x1CED, 1, 0, OT_ITEM );
		if( c == NULL ) 
			return;
		c->SetName( temp );
		c->SetLocation( i );
		c->SetOwner( i->GetOwnerObj() );

		sprintf( temp, Dictionary->GetEntry( 1057 ).c_str(), i->GetName2() );
		c = Items->CreateItem( NULL, mChar, 0x1DAD, 1, 0, OT_ITEM );
		if( c == NULL ) 
			return;
		c->SetName( temp );
		c->SetLocation( i );
		c->SetOwner( i->GetOwnerObj() );

		// create the Left Arm
		sprintf( temp, Dictionary->GetEntry( 1060 ).c_str(), i->GetName2() );
		c = Items->CreateItem( NULL, mChar, 0x1D80, 1, 0, OT_ITEM );
		if( c == NULL ) 
			return;
		c->SetName( temp );
		c->SetLocation( i );
		c->SetOwner( i->GetOwnerObj() );

		// create the Right Arm
		sprintf( temp, Dictionary->GetEntry( 1061 ).c_str(), i->GetName2() );
		c = Items->CreateItem( NULL, mChar, 0x1DAF, 1, 0, OT_ITEM );
		if( c == NULL ) 
			return;
		c->SetName( temp );
		c->SetLocation( i );
		c->SetOwner( i->GetOwnerObj() );

		// create the Left Leg
		sprintf( temp, Dictionary->GetEntry( 1062 ).c_str(), i->GetName2() );
		c = Items->CreateItem( NULL, mChar, 0x1DB2, 1, 0, OT_ITEM );
		if( c == NULL ) 
			return;
		c->SetName( temp );
		c->SetLocation( i );
		c->SetOwner( i->GetOwnerObj() );

		// create the Right Leg
		sprintf( temp, Dictionary->GetEntry( 1063 ).c_str(), i->GetName2() );
		c = Items->CreateItem( NULL, mChar, 0x1D81, 1, 0, OT_ITEM );
		if( c == NULL ) 
			return;
		c->SetName( temp );
		c->SetLocation( i );
		c->SetOwner( i->GetOwnerObj() );

		//human: always delete corpse!
		deletecorpse = true;
		criminal( mChar );
	} 
	else
	{
		UString sect			= "CARVE " + UString::number( i->GetCarve() );
		ScriptSection *toFind	= FileLookup->FindEntry( sect, carve_def );
		if( toFind == NULL )
			return;
		UString tag;
		UString data;
		UI08 worldNumber = mChar->WorldNumber();
		for( tag = toFind->First(); !toFind->AtEnd(); tag = toFind->Next() )
		{
			data = toFind->GrabData();
			if( tag.upper() == "ADDITEM" )
			{
				if( data.sectionCount( "," ) != 0 )
					Items->CreateScriptItem( s, mChar, data.section( ",", 0, 0 ).stripWhiteSpace(), data.section( ",", 1, 1 ).stripWhiteSpace().toUShort(), OT_ITEM, true );
				else
					Items->CreateScriptItem( s, mChar, data, 0, OT_ITEM, true );
			}
		}
	}

	if( deletecorpse )
	{
		for( c = i->Contains.First(); !i->Contains.Finished(); c = i->Contains.Next() )
		{
			if( ValidateObject( c ) )
			{
				c->SetCont( NULL );
				c->SetLocation( i );
				c->SetDecayTime( BuildTimeValue( static_cast<R32>(cwmWorldState->ServerData()->SystemTimer( DECAY ) )) );
			}
		}
		i->Delete();
	}
}

void CorpseTarget( cSocket *s )
{
	VALIDATESOCKET( s );
	CItem *i = calcItemObjFromSer( s->GetDWord( 7 ) );
	CChar *mChar = s->CurrcharObj();
	if( !ValidateObject( i ) || !ValidateObject( mChar ) )
		return;
	if( objInRange( mChar, i, DIST_NEARBY ) ) 
	{
		Effects->PlayCharacterAnimation( mChar, 0x20 );
		if( i->GetTempVar( CITV_MORE, 1 ) == 0 )
		{
			i->SetTempVar( CITV_MORE, 1, 1 );
			if( i->GetTempVar( CITV_MOREY ) || i->GetCarve() != -1 )
				newCarveTarget( s, i );
		} 
		else 
			s->sysmessage( 1051 );
	}
	else
		s->sysmessage( 393 );
}

void SwordTarget( cSocket *s )
{
	VALIDATESOCKET( s );

	CChar *p		= calcCharObjFromSer( s->GetDWord( 7 ) );
	CChar *mChar	= s->CurrcharObj();

	if( !ValidateObject( mChar ) )
		return;

	if( ValidateObject( p ) )
	{
		if( p->GetID() == 0xCF )
		{
			// Unshorn sheep
			// -> Add Wool and change id of the Sheep
			CItem *c = Items->CreateItem( s, mChar, 0x0DF8, 1, 0, OT_ITEM, true );

			p->SetID( 0xDF );			

			// Add an effect so the sheep can regain it's wool
			cDice myDice( 2, 3, 0 );
			UI32 Delay = myDice.roll();

			Effects->tempeffect( p, p, 43, static_cast<UI16>(Delay*300), 0, 0 );
		}
		else
		{
			// Already sheered
			//s->sysmessage( "" );
		}

		return;
	}
	
	if( s->GetDWord( 11 ) == INVALIDSERIAL )
		return;

	UI16 targetID = s->GetWord( 0x11 );

	switch( targetID )
	{
		case 0x0CD0:
		case 0x0CD3:
		case 0x0CD6:
		case 0x0CD8:
		case 0x0CDA:
		case 0x0CDD:
		case 0x0CE0:
		case 0x0CE3:
		case 0x0CE6:
		case 0x0CCA:
		case 0x0CCB:
		case 0x0CCC:
		case 0x0CCD:
		case 0x0C12:
		case 0x0CB8:
		case 0x0CB9:
		case 0x0CBA:
		case 0x0CBB:
		{
			SI16 targetX = s->GetWord( 0x0B );		// store our target x y and z locations
			SI16 targetY = s->GetWord( 0x0D );
			SI08 targetZ = s->GetByte( 0x10 );

			SI08 distZ = abs( targetZ - mChar->GetZ() );
			SI16 distY = abs( targetY - mChar->GetY() );
			SI16 distX = abs( targetX - mChar->GetX() );

			if( distY > 5 || distX > 5 || distZ > 9 )
			{
				s->sysmessage( 393 );
				return;
			}
			if( !mChar->IsOnHorse() )
				Effects->PlayCharacterAnimation( mChar, 0x0D );
			else 
				Effects->PlayCharacterAnimation( mChar, 0x1D );
			Effects->PlaySound( s, 0x013E, true );
			CItem *c = Items->CreateItem( s, mChar, 0x0DE1, 1, 0, OT_ITEM, true ); //Kindling
			if( c == NULL ) 
				return;
			s->sysmessage( 1049 );
			return;
		}
		case 0x09CC: 
		case 0x09CD: 
		case 0x09CE: 
		case 0x09CF: 
		{	
			CItem *i = calcItemObjFromSer( s->GetDWord( 7 ) ); 
			if( !ValidateObject( i ) ) 
				return;		
			if( FindItemOwner( i ) != mChar ) 
			{ 
				s->sysmessage( 775 ); 
				return; 
			} 
			else 
			{ 
				UI32 getAmt = GetItemAmount( mChar, i->GetID() ); 
				if( getAmt < 1 ) 
				{ 
					s->sysmessage( 776 ); 
					return; 
				} 
				Effects->PlaySound( s, 0x013E, true); // I'm not sure 
				CItem *c = Items->CreateItem( s, mChar, 0x097A, 4, 0, OT_ITEM, true ); 
				if( ValidateObject( c ) ) 
				{
					c->SetName( "raw fish steak" );
					i->IncAmount( -1 );
				}
				return; 
			} 
		}
		case 0x1BDD:
		case 0x1BE0:
		{
			Skills->BowCraft( s );
			return;
		}
		case 0x2006:
		{
			CorpseTarget( s );
			return;
		}
	}
	s->sysmessage( 1050 );
}

void MakeShopTarget( cSocket *s )
{
	VALIDATESOCKET( s );
	CChar *i = calcCharObjFromSer( s->GetDWord( 7 ) );
	if( ValidateObject( i ) )
	{
		MakeShop( i );
		s->sysmessage( 1068 );
		return;
	}
	s->sysmessage( 1069 );
}

void AttackTarget( cSocket *s )
{
	VALIDATESOCKET( s );
	CChar *target	= static_cast<CChar *>(s->TempObj());
	CChar *target2	= calcCharObjFromSer( s->GetDWord( 7 ) );
	s->TempObj( NULL );
	
	if( !ValidateObject( target2 ) || !ValidateObject( target ) ) 
		return;
	if( target2 == target )
	{
		s->sysmessage( 1073 );
		return;
	}
	Combat->AttackTarget( target, target2 );
	if( target2->IsInnocent() && target2 != target->GetOwnerObj() )
	{
		CChar *pOwner = target->GetOwnerObj();
		if( ValidateObject( pOwner ) && Combat->WillResultInCriminal( pOwner, target2 ) )
			criminal( pOwner );
	}
}

void FollowTarget( cSocket *s )
{
	VALIDATESOCKET( s );
	CChar *char1	= static_cast<CChar *>(s->TempObj());
	CChar *char2	= calcCharObjFromSer( s->GetDWord( 7 ) );
	s->TempObj( NULL );
	if( !ValidateObject( char1 ) || !ValidateObject( char2 ) )
		return;
	
	char1->SetFTarg( char2 );
	char1->SetNpcWander( 1 );
}

void TransferTarget( cSocket *s )
{
	VALIDATESOCKET( s );
	CChar *char1 = static_cast<CChar *>(s->TempObj());
	CChar *char2 = calcCharObjFromSer( s->GetDWord( 7 ) );
	s->TempObj( NULL );
	
	if( !ValidateObject( char1 ) )
		return;

	if( !ValidateObject( char2 ) )
	{
		s->sysmessage( 1066 );
		return;
	}
	if( char1 == char2 )
	{
		s->sysmessage( 1066 );
		return;
	}

	Npcs->stopPetGuarding( char1 );

	char1->talkAll( 1074, false, char1->GetName().c_str(), char2->GetName().c_str() );
	
	char1->SetOwner( char2 );
	char1->SetNpcWander( 1 );
	
	char1->SetFTarg( NULL );
	char1->SetNpcWander( 0 );
}

bool BuyShop( cSocket *s, CChar *c )
{
	if( s == NULL )
		return false;
	if( !ValidateObject( c ) )
		return false;
	CItem *buyPack		= c->GetItemAtLayer( IL_BUYCONTAINER );
	CItem *boughtPack	= c->GetItemAtLayer( IL_BOUGHTCONTAINER );
	
	if( !ValidateObject( buyPack ) || !ValidateObject( boughtPack ) )
		return false;
	
	buyPack->Sort();
	boughtPack->Sort();
	c->Update( s );

	CPItemsInContainer iic( s, buyPack,c );	s->Send( &iic );
	CPOpenBuyWindow obw( buyPack, c );		s->Send( &obw );

	CPItemsInContainer iic2( s, boughtPack, c );	s->Send( &iic2 );
	CPOpenBuyWindow obw2( boughtPack, c );		s->Send( &obw2 );

	CPDrawContainer toSend;
	toSend.Model( 0x0030 );
	toSend.Serial( c->GetSerial() );
	s->Send( &toSend );
	s->statwindow( s->CurrcharObj() ); // Make sure the gold total has been sent.
	return true;
}

void AxeTarget( cSocket *s )
{
	VALIDATESOCKET( s );
	if( s->GetDWord( 11 ) == INVALIDSERIAL )
		return;
	
	UI16 realID = s->GetWord( 0x11 );
    // [krazyglue] it may take more lines, but at least its readable and easier to debug =)
	if( realID == 0x0CD0 || realID == 0x0CD3 || realID == 0x0CD6 || realID == 0x0CD8 || realID == 0x0CDA || 
		realID == 0x0CDD || realID == 0x0CE0 || realID == 0x0CE3 || realID == 0x0CE6 || realID == 0x0D58 || 
		realID >= 0x0CCA && realID <= 0x0CCE || realID >= 0x12B8 && realID <= 0x12BB || realID == 0x0D42 ||
		realID == 0x0D43 || realID == 0x0D58 || realID == 0x0D59 || realID == 0x0D70 || realID == 0x0D85 || 
		realID == 0x0D94 || realID == 0x0D95 || realID == 0x0D98 || realID == 0x0DA4 || realID == 0x0DA8 )
	{
		Skills->TreeTarget( s );
	}
	else if( realID == 0x2006 )
		CorpseTarget( s );
    else 
		Skills->BowCraft( s );
}

//o---------------------------------------------------------------------------o
//|   Function		-  void npcresurrecttarget( CChar *i )
//|   Date			-  UnKnown
//|   Programmer	-  UnKnown  (Touched tabstops by Tauriel Dec 28, 1998)
//|									
//|	Modification	-	09/22/2002	-	Xuri - Made players not appear with full 
//|									health/stamina after being resurrected by NPC Healer
//o---------------------------------------------------------------------------o
//|   Purpose     -  Resurrects a character
//o---------------------------------------------------------------------------o
void NpcResurrectTarget( CChar *i )
{
	if( !ValidateObject( i ) )
		return;

	if( i->IsNpc() )
	{
		Console.Error( 2, Dictionary->GetEntry( 1079 ).c_str(), i );
		return;
	}
	cSocket *mSock = calcSocketObjFromChar( i );
	// the char is a PC, but not logged in.....
	if( mSock != NULL )
	{
		if( i->IsDead() )
		{
			Fame( i, 0 );
			Effects->PlaySound( i, 0x0214 );
			i->SetID( i->GetOrgID() );
			i->SetSkin( i->GetOrgSkin() );
			i->SetDead( false );
			// Sept 22, 2002 - Xuri
			i->SetHP( i->GetMaxHP() / 10 );
			i->SetStamina( i->GetMaxStam() / 10 );
			//
			i->SetMana( i->GetMaxMana() / 10 );
			i->SetAttacker( NULL );
			i->SetAttackFirst( false );
			i->SetWar( false );
			i->SetHunger( 6 );
			CItem *c = NULL;
			for( CItem *j = i->FirstItem(); !i->FinishedItems(); j = i->NextItem() )
			{
				if( ValidateObject( j ) && !j->isFree() )
				{
					if( j->GetLayer() == IL_BUYCONTAINER )
					{
						j->SetLayer( IL_PACKITEM );	
						i->SetPackItem( j );
					}
					if( j->GetSerial() == i->GetRobe() )
					{
						j->Delete();
						
						c = Items->CreateItem( NULL, i, 0x1F03, 1, 0, OT_ITEM );
						if( c != NULL )
						{
							c->SetName( "a resurrect robe" );
							c->SetLayer( IL_ROBE );
							if( c->SetCont( i ) )
								c->SetDye( true );
						}
					}
				}
			}
			UI16 targTrig		= i->GetScriptTrigger();
			cScript *toExecute	= Trigger->GetScript( targTrig );
			if( toExecute != NULL )
				toExecute->OnResurrect( i );
		}
		else
			mSock->sysmessage( 1080 );
	}
	else
		Console.Warning( 2, "Attempt made to resurrect a PC (serial: 0x%X) that's not logged in", i->GetSerial() );
}


void killKeys( SERIAL targSerial );
void HouseOwnerTarget( cSocket *s )
{
	VALIDATESOCKET( s );
	CChar *mChar = s->CurrcharObj();
	if( !ValidateObject( mChar ) )
		return;

	SERIAL o_serial = s->GetDWord( 7 );
	if( o_serial == INVALIDSERIAL ) 
		return;

	CChar *own		= calcCharObjFromSer( o_serial );
	cSocket *oSock	= calcSocketObjFromChar( own );
	CItem *sign		= static_cast<CItem *>(s->TempObj());
	CItem *house	= calcItemObjFromSer( sign->GetTempVar( CITV_MORE ) );
	s->TempObj( NULL );
	if( !ValidateObject( sign ) || !ValidateObject( house ) )
		return;
	sign->SetOwner( own );
	house->SetOwner( own );

	killKeys( house->GetSerial() );

	CItem *key = Items->CreateItem( oSock, own, 0x100F, 1, 0, OT_ITEM, true );	// gold key for everything else
	if( key == NULL )
		return;
	key->SetName( "a house key" );
	key->SetTempVar( CITV_MORE, house->GetSerial() );
	key->SetType( IT_KEY );

	s->sysmessage( 1081, own->GetName().c_str() );
	oSock->sysmessage( 1082, mChar->GetName().c_str() );
}

void HouseEjectTarget( cSocket *s )
{
	VALIDATESOCKET( s );
	CChar *c = calcCharObjFromSer( s->GetDWord( 7 ) );
	CItem *h = static_cast<CItem *>(s->TempObj());
	s->TempObj( NULL );
	if( ValidateObject( c ) && ValidateObject( h ) ) 
	{
		SI16 sx, sy, ex, ey;
		Map->MultiArea( (CMultiObj *)h, sx, sy, ex, ey );
		if( c->GetX() >= sx && c->GetY() >= sy && c->GetX() <= ex && c->GetY() <= ey )
		{
			c->SetLocation( ex, ey, c->GetZ() );
			s->sysmessage( 1083 );
		} 
		else 
			s->sysmessage( 1084 );
	}
}

UI08 AddToHouse( CMultiObj *house, CChar *toAdd, UI08 mode = 0 );
void HouseBanTarget( cSocket *s )
{
	VALIDATESOCKET( s );
	// first, eject the player
	HouseEjectTarget( s );
	CChar *c		= calcCharObjFromSer( s->GetDWord( 7 ) );
	CMultiObj *h	= static_cast<CMultiObj *>(s->TempObj());
	s->TempObj( NULL );
	if( ValidateObject( c ) && ValidateObject( h ) ) 
	{
		UI08 r = AddToHouse( h, c, 1 );
		if( r == 1 ) 
			s->sysmessage( 1085, c->GetName().c_str() );
		else if( r == 2 ) 
			s->sysmessage( 1086 );
		else 
			s->sysmessage( 1087 );
	}
}

void HouseFriendTarget( cSocket *s )
{
	VALIDATESOCKET( s );
	CChar *c		= calcCharObjFromSer( s->GetDWord( 7 ) );
	CMultiObj *h	= static_cast<CMultiObj *>(s->TempObj());
	s->TempObj( NULL );
	if( ValidateObject( c ) && ValidateObject( h ) ) 
	{
		UI08 r = AddToHouse( h, c );
		if( r == 1 ) 
		{
			cSocket *cSock = calcSocketObjFromChar( c );
			if( cSock != NULL )
				cSock->sysmessage( 1089 );
			s->sysmessage( 1088, c->GetName().c_str() );
		} 
		else if( r == 2 ) 
			s->sysmessage( 1090 );
		else 
			s->sysmessage( 1091 );
	}
}

bool DeleteFromHouseList( CMultiObj *house, CChar *toDelete, UI08 mode = 0 );
void HouseUnlistTarget( cSocket *s )
{
	VALIDATESOCKET( s );
	CChar *c		= calcCharObjFromSer( s->GetDWord( 7 ) );
	CMultiObj *h	=  static_cast<CMultiObj *>(s->TempObj());
	s->TempObj( NULL );
	if( ValidateObject( c ) && ValidateObject( h ) ) 
	{
		bool r = DeleteFromHouseList( h, c );
		if( r )
			s->sysmessage( 1092, c->GetName().c_str() );
		else 
			s->sysmessage( 1093 );
	}
}

void ShowSkillTarget( cSocket *s )
{
	VALIDATESOCKET( s );
	CChar *p = calcCharObjFromSer( s->GetDWord( 7 ) );
	if( !ValidateObject( p ) )
	{
		s->sysmessage( 1103 );
		return;
	}
	char spc[2]="\x20";
	char temp[1024];
	int j, k, zz, ges = 0;
	char skill_info[(ALLSKILLS+1)*40];
	UString sk;

	SI32 z = s->TempInt();
	if( z < 0 || z > 3 ) 
		z = 0;
	if( z == 2 || z == 3 ) 
		sprintf( skill_info, "%s's skills:", p->GetName().c_str() ); 
	else 
		sprintf( skill_info, "%s's baseskills:", p->GetName().c_str() );
	size_t b = p->GetName().size() + 11;
	if( b > 23 ) 
		b = 23;

	UI08 c;
	for( c = b; c <= 26; ++c )
	{
		strcpy( temp, spc );
		strcpy( &skill_info[strlen( skill_info )], temp );
	}

	sk = UString::number( ges );
	sprintf( temp, "sum: %s", sk.c_str() );
	strcpy( &skill_info[strlen( skill_info )], temp );
	for( UI08 a = 0; a < ALLSKILLS; ++a )
	{
		if( z == 0 || z == 1 ) 
			k = p->GetBaseSkill( a ); 
		else 
			k = p->GetSkill( a );
		if( z == 0 || z == 2 ) 
			zz = 9; 
		else 
			zz = -1;

		if( k > zz ) // show only if skills >= 1
		{
			if( z == 2 || z == 3 ) 
				j = p->GetSkill( a )/10; 
			else 
				j = p->GetBaseSkill( a )/10; // get skill value
			sk = UString::number( j );	// skill value string in sk
			ges += j;
			sprintf( temp, "%s%s%s", skillname[a], spc, sk.c_str() );
			strcpy( &skill_info[strlen( skill_info )], temp );

			b = strlen( skillname[a] ) + sk.length() + 1; // it doesn't like \n's, so insert spaces till end of line
			if( b > 23 )
				b = 23;
			for( c = b; c <= 26; ++c )
			{
				strcpy( temp, spc );
				strcpy( &skill_info[strlen( skill_info )], temp );
			}
		}
	}

	sk = UString::number( ges );
	sprintf( temp, "sum: %s  ", sk.c_str() );
	strcpy( &skill_info[ strlen( skill_info )], temp );

	CPUpdScroll toSend( 2 );
	toSend.AddString( skill_info );
	toSend.Finalize();
	s->Send( &toSend );
}

void FriendTarget( cSocket *s )
{
	VALIDATESOCKET( s );
	CChar *mChar = s->CurrcharObj();
	if( !ValidateObject( mChar ) )
		return;

	CChar *targChar = calcCharObjFromSer( s->GetDWord( 7 ) );
	if( !ValidateObject( targChar ) )
	{
		s->sysmessage( 1103 );
		return;
	}
	if( targChar->IsNpc() || !isOnline( targChar ) || targChar == mChar )
	{
		s->sysmessage( 1622 );
		return;
	}

	CChar *pet = static_cast<CChar *>(s->TempObj());
	s->TempObj( NULL );
	if( Npcs->checkPetFriend( targChar, pet ) )
	{
		s->sysmessage( 1621 );
		return;
	}

	CHARLIST *petFriends = pet->GetFriendList();
	// Make sure to cover the STL response
	if( petFriends != NULL )
	{
		if( petFriends->size() >= 10 )
		{
			s->sysmessage( 1623 );
			return;
		}
	}

	pet->AddFriend( targChar );
	s->sysmessage( 1624, pet->GetName().c_str(), targChar->GetName().c_str() );

	cSocket *targSock = calcSocketObjFromChar( targChar );
	if( targSock != NULL )
		targSock->sysmessage( 1625, mChar->GetName().c_str(), pet->GetName().c_str() );
}

void GuardTarget( cSocket *s )
//PRE:	Pet has been commanded to guard
//POST: Pet guards person, if owner currently
//DEV:	Abaddon
//DATE: 3rd October
{
	VALIDATESOCKET( s );
	CChar *mChar = s->CurrcharObj();
	if( !ValidateObject( mChar ) )
		return;

	CChar *petGuarding = static_cast<CChar *>(s->TempObj());
	s->TempObj( NULL );
	if( !ValidateObject( petGuarding ) )
		return;

	Npcs->stopPetGuarding( petGuarding );

	CChar *charToGuard = calcCharObjFromSer( s->GetDWord( 7 ) );
	if( ValidateObject( charToGuard ) )
	{
		if( charToGuard != petGuarding->GetOwnerObj() && !Npcs->checkPetFriend( charToGuard, petGuarding ) )
		{
			s->sysmessage( 1628 );
			return;
		}
		petGuarding->SetNPCAiType( aiPET_GUARD ); // 32 is guard mode
		petGuarding->SetGuarding( charToGuard );
		mChar->SetGuarded( true );
		return;
	}
	CItem *itemToGuard = calcItemObjFromSer( s->GetDWord( 7 ) );
	if( ValidateObject( itemToGuard ) && !itemToGuard->isPileable() )
	{
		CItem *p = mChar->GetPackItem();
		if( ValidateObject( p ) )
		{
			if( itemToGuard->GetCont() == p || itemToGuard->GetCont() == mChar )
			{
				itemToGuard->SetGuarded( true );
				petGuarding->SetGuarding( itemToGuard );
			}
		}
		else
		{
			CMultiObj *multi = findMulti( itemToGuard );
			if( ValidateObject( multi ) )
			{
				if( multi->GetOwnerObj() == mChar )
				{
					itemToGuard->SetGuarded( true );
					petGuarding->SetGuarding( itemToGuard );
				}
			}
			else
				s->sysmessage( 1628 );
		}
	}
}

void HouseLockdown( cSocket *s ) // Abaddon
// PRE:		S is the socket of a valid owner/coowner and is in a valid house
// POST:	either locks down the item, or puts a message to the owner saying he's a moron
// CODER:	Abaddon
// DATE:	17th December, 1999
{
	VALIDATESOCKET( s );
	CItem *itemToLock = calcItemObjFromSer( s->GetDWord( 7 ) );
	if( ValidateObject( itemToLock ) )
	{
		if( !itemToLock->CanBeLockedDown() )
		{
			s->sysmessage( 1106 );
			return;
		}
		CMultiObj *house =  static_cast<CMultiObj *>(s->TempObj());
		s->TempObj( NULL );
		// time to lock it down!
		CMultiObj *multi = findMulti( itemToLock );
		if( ValidateObject( multi ) )
		{
			if( multi != house )
			{
				s->sysmessage( 1105 );
				return;
			}
			if( multi->GetLockDownCount() < multi->GetMaxLockDowns() )
				multi->LockDownItem( itemToLock );
			else
				s->sysmessage( "You have too many locked down items" );
			return;
		}
		// not in a multi!
		s->sysmessage( 1107 );
	}
	else
		s->sysmessage( 1108 );
}

void HouseRelease( cSocket *s ) // Abaddon
// PRE:		S is the socket of a valid owner/coowner and is in a valid house, the item is locked down
// POST:	either releases the item from lockdown, or puts a message to the owner saying he's a moron
// CODER:	Abaddon
// DATE:	17th December, 1999
{
	VALIDATESOCKET( s );
	CItem *itemToLock = calcItemObjFromSer( s->GetDWord( 7 ) );

	if( ValidateObject( itemToLock ) || !itemToLock->IsLockedDown() )
	{
		CMultiObj *house =  static_cast<CMultiObj *>(s->TempObj());	// let's find our house
		s->TempObj( NULL );
		// time to lock it down!
		CMultiObj *multi = findMulti( itemToLock );
		if( ValidateObject( multi ) )
		{
			if( multi != house )
			{
				s->sysmessage( 1109 );
				return;
			}
			if( multi->GetLockDownCount() > 0 )
				multi->RemoveLockDown( itemToLock );	// Default as stored by the client, perhaps we should keep a backup?
			return;
		}
		// not in a multi!
		s->sysmessage( 1107 );
	}
	else
		s->sysmessage( 1108 );
}

void MakeTownAlly( cSocket *s )
{
	VALIDATESOCKET( s );
	CChar *mChar = s->CurrcharObj();
	if( !ValidateObject( mChar ) )
		return;

	CChar *targetChar = calcCharObjFromSer( s->GetDWord( 7 ) );
	if( !ValidateObject( targetChar ) )
	{
		s->sysmessage( 1110 );
		return;
	}
	UI08 srcTown = mChar->GetTown();
	UI08 trgTown = targetChar->GetTown();

//	if( srcTown == trgTown )
//		s->sysmessage( "That person already belongs to your town" );
//	else
//	{
		if( !regions[srcTown]->MakeAlliedTown( trgTown ) )	
			s->sysmessage( 1111 );
//	}
}

void MakeStatusTarget( cSocket *sock )
{
	VALIDATESOCKET( sock );
	CChar *targetChar = calcCharObjFromSer( sock->GetDWord( 7 ) );
	if( !ValidateObject( targetChar ) )
	{
		sock->sysmessage( 1110 );
		return;
	}
	UI08 origCommand			= targetChar->GetCommandLevel();
	commandLevel_st *targLevel	= Commands->GetClearance( sock->XText() );
	commandLevel_st *origLevel	= Commands->GetClearance( origCommand );
	
	if( targLevel == NULL )
	{
		sock->sysmessage( 1112 );
		return;
	}
	CChar *mChar = sock->CurrcharObj();
	char temp[1024], temp2[1024];

	UI08 targetCommand = targLevel->commandLevel;
	sprintf( temp, "account%i.log", mChar->GetAccount().wAccountIndex );
	sprintf( temp2, "%s has made %s a %s.\n", mChar->GetName().c_str(), targetChar->GetName().c_str(), targLevel->name.c_str() );
	Console.Log( temp2, temp );

	DismountCreature( targetChar );

	if( targLevel->targBody != 0 )
	{
		targetChar->SetID( targLevel->targBody );
		targetChar->SetSkin( targLevel->bodyColour );
		targetChar->SetOrgID( targLevel->targBody );
		targetChar->SetOrgSkin( targLevel->bodyColour );
	}

	targetChar->SetPriv( targLevel->defaultPriv );
	targetChar->SetCommandLevel( targetCommand );
	
	if( targLevel->allSkillVals != 0 )
	{
		for( UI08 j = 0; j < ALLSKILLS; ++j )
		{
			targetChar->SetBaseSkill( targLevel->allSkillVals, j );
			targetChar->SetSkill( targLevel->allSkillVals, j );
		}
		targetChar->SetStrength( 100 );
		targetChar->SetDexterity( 100 );
		targetChar->SetIntelligence( 100 );
		targetChar->SetStamina(  100 );
		targetChar->SetHP( 100 );
		targetChar->SetMana( 100 );
	}
	char *playerName = new char[targetChar->GetName().size() + 1];
	strcpy( playerName, targetChar->GetName().c_str() );

	if( targetCommand != 0 && targetCommand != origCommand )
	{
		if( origLevel != NULL )
		{	// Strip name off it
			if( !strnicmp( origLevel->name.c_str(), playerName, strlen( origLevel->name.c_str() ) ) )
				playerName += ( strlen( origLevel->name.c_str() ) + 1 );
		}
		sprintf( temp, "%s %s", targLevel->name.c_str(), playerName );
		targetChar->SetName( temp );
	}
	else if( origCommand != 0 )
	{
		if( origLevel != NULL )
		{	// Strip name off it
			if( !strnicmp( origLevel->name.c_str(), playerName, strlen( origLevel->name.c_str() ) ) )
				playerName += ( strlen( origLevel->name.c_str() ) + 1 );
		}
		strcpy( temp, (const char*)playerName );
		targetChar->SetName( temp );
	}
	CItem *retitem	= NULL;
	CItem *mypack	= targetChar->GetPackItem();

	if( targLevel->stripOff )
	{
		for( CItem *z = targetChar->FirstItem(); !targetChar->FinishedItems(); z = targetChar->NextItem() )
		{
			if( ValidateObject( z ) )
			{
				switch( z->GetLayer() )
				{
					case IL_HAIR:
					case IL_FACIALHAIR:
						z->Delete();
						break;
					case IL_PACKITEM:
					case IL_BANKBOX:
						break;
					default:
						if( !ValidateObject( mypack ) )
							mypack = targetChar->GetPackItem();
						if( !ValidateObject( mypack ) )
						{
							CItem *iMade = Items->CreateItem( NULL, targetChar, 0x0E75, 1, 0, OT_ITEM );
							targetChar->SetPackItem( iMade );
							if( iMade == NULL ) 
								return;
							iMade->SetLayer( IL_PACKITEM );
							if( iMade->SetCont( targetChar ) )
							{
								iMade->SetType( IT_CONTAINER );
								iMade->SetDye( true );
								mypack = iMade;
								retitem = iMade;
							}
						}
						z->SetCont( mypack );
						z->PlaceInPack();
						break;
				}
			}
		}
	}
	delete [] playerName;
}

void SmeltTarget( cSocket *s )
{
	VALIDATESOCKET( s );

	CItem *i = calcItemObjFromSer( s->GetDWord( 7 ) );
	if( !ValidateObject( i ) || i->GetCont() == NULL )
		return;
	if( i->GetCreator() == INVALIDSERIAL )
	{
		s->sysmessage( 1113 );
		return;
	}
	UI16 iMadeFrom = i->EntryMadeFrom();

	createEntry *ourCreateEntry = Skills->FindItem( iMadeFrom );
	if( iMadeFrom == 0 || ourCreateEntry == NULL )
	{
		s->sysmessage( 1114 );
		return;
	}

	CChar *mChar = s->CurrcharObj();

	R32 avgMin = ourCreateEntry->AverageMinSkill();
	if( mChar->GetSkill( MINING ) < avgMin )
	{
		s->sysmessage( 1115 );
		return;
	}
	R32 avgMax = ourCreateEntry->AverageMaxSkill();

	Skills->CheckSkill( mChar, MINING, (SI16)avgMin, (SI16)avgMax );

	R32 sumAmountRestored = 0.0f;

	for( UI32 skCtr = 0; skCtr < ourCreateEntry->resourceNeeded.size(); ++skCtr )
	{
		UI16 amtToRestore = ourCreateEntry->resourceNeeded[skCtr].amountNeeded / 2;
		UI16 itemID = ourCreateEntry->resourceNeeded[skCtr].itemID;
		UI16 itemColour = ourCreateEntry->resourceNeeded[skCtr].colour;
		sumAmountRestored += amtToRestore;
		Items->CreateItem( s, mChar, itemID, amtToRestore, itemColour, OT_ITEM, true );
	}

	s->sysmessage( 1116, sumAmountRestored );
	i->Delete();
}

void VialTarget( cSocket *mSock )
{
	VALIDATESOCKET( mSock );
	SERIAL targSerial = mSock->GetDWord( 7 );
	if( targSerial == INVALIDSERIAL )
		return;

	CChar *mChar = mSock->CurrcharObj();
	if( !ValidateObject( mChar ) )
		return;

	CItem *nVialID = static_cast<CItem *>(mSock->TempObj());
	mSock->TempObj( NULL );
	if( ValidateObject( nVialID ) )
	{
		CItem *nDagger = Combat->getWeapon( mChar );
		if( !ValidateObject( nDagger ) ) 
		{
			mSock->sysmessage( 742 );
			return;
		}
		if( nDagger->GetID() != 0x0F51 && nDagger->GetID() != 0x0F52 )
		{
			mSock->sysmessage( 743 );
			return;
		}

		nVialID->SetTempVar( CITV_MORE, 0, 1 );
		if( targSerial >= BASEITEMSERIAL )
		{	// it's an item
			CItem *targItem = calcItemObjFromSer( targSerial );
			if( !targItem->isCorpse() )
				mSock->sysmessage( 749 );
			else
			{
				nVialID->SetTempVar( CITV_MORE, targItem->GetTempVar( CITV_MORE, 1 ), 1 );
				Karma( mChar, NULL, -1000 );
				if( targItem->GetTempVar( CITV_MORE, 2 ) < 4 )
				{
					mSock->sysmessage( 750 );
					Skills->MakeNecroReg( mSock, nVialID, 0x0E24 );
					targItem->SetTempVar( CITV_MORE, targItem->GetTempVar( CITV_MORE, 2 ) + 1, 2 );
				}
				else 
					mSock->sysmessage( 751 );
			}
		}
		else
		{	// it's a char
			CChar *targChar = calcCharObjFromSer( targSerial );
			if( targChar == mChar )
			{
				if( targChar->GetHP() <= 10 )
					mSock->sysmessage( 744 );
				else
					mSock->sysmessage( 745 );
			}
			else if( objInRange( mChar, targChar, DIST_NEARBY ) )
			{
				if( targChar->IsNpc() )
				{
					if( targChar->GetID( 1 ) == 0x00 && ( targChar->GetID( 2 ) == 0x0C ||
						( targChar->GetID( 2 ) >= 0x3B && targChar->GetID( 2 ) <= 0x3D ) ) )
						nVialID->SetTempVar( CITV_MORE, 1, 1 );
				}
				else
				{
					cSocket *nCharSocket = calcSocketObjFromChar( targChar );
					nCharSocket->sysmessage( 746, mChar->GetName().c_str() );
				}
				if( Combat->WillResultInCriminal( mChar, targChar ) )
					criminal( mChar );
				Karma( mChar, targChar, -targChar->GetKarma() );
			}
			else
			{
				mSock->sysmessage( 747 );
				return;
			}
			targChar->SetHP( targChar->GetHP() - ( RandomNum( 0, 5 ) + 2 ) );
			Skills->MakeNecroReg( mSock, nVialID, 0x0E24 );
		}
	}	
}

void MultiSetCharTarget( cSocket *s, TargetIDs targetID )
{
	VALIDATESOCKET( s );
	CChar *i = calcCharObjFromSer( s->GetDWord( 7 ) );
	if( ValidateObject( i ) )
	{
		switch( targetID )
		{
		case TARGET_XGO:
									i->SetLocation( s->ClickX(), s->ClickY(), s->ClickZ(), s->AddID4() );
									s->ClickX( -1 );
									s->ClickY( -1 );
									s->ClickZ( -1 );
									s->AddID4( 0 );
									break;
		default:					Console.Error( 2, " MultiTarget() was passed a bad targetID (%u)", static_cast<UI08>(targetID) );	break;
		}
	}
}

//o--------------------------------------------------------------------------o
//|	Function		-	void MultiTarget( cSocket *s )
//|	Date			-	Unknown
//|	Developers		-	Unknown
//|	Organization	-	UOX3 DevTeam
//o--------------------------------------------------------------------------o
//|	Description		-	Runs various commands based upon the target ID we sent to the socket
//o--------------------------------------------------------------------------o
//| Modifications	-	Overhauled to use an enum allowing simple modification (Zane)
//o--------------------------------------------------------------------------o
bool CPITargetCursor::Handle( void )
{
	CChar *mChar = tSock->CurrcharObj();
	if( tSock->TargetOK() )
	{
		if( tSock->GetDWord( 11 ) == INVALIDSERIAL )
		{
			if( mChar->GetSpellCast() != -1 )	// need to stop casting if we don't target right
				mChar->StopSpell();
			return true; // do nothing if user cancels, avoids CRASH! - Morrolan
		}
		
		UI08 a1 = tSock->GetByte( 2 );
		UI08 a2 = tSock->GetByte( 3 );
		UI08 a3 = tSock->GetByte( 4 );
		TargetIDs targetID = (TargetIDs)tSock->GetByte( 5 );
		tSock->TargetOK( false );
		if( mChar->IsDead() && !mChar->IsGM() && mChar->GetAccount().wAccountIndex != 0 )
		{
			tSock->sysmessage( 1008 );
			if( mChar->GetSpellCast() != -1 )	// need to stop casting if we don't target right
				mChar->StopSpell();
			return true;
		}
		if( a1 == 0 && a2 == 1 )
		{
			if( a3 == 2 )	// Guilds
			{
				HandleGuildTarget( tSock );
				return true;
			}
			else if( a3 == 1 )	// CustomTarget
			{
				cScript *tScript	= (cScript *)tSock->TempInt();
				if( tScript != NULL )
					tScript->DoCallback( tSock, tSock->GetDWord( 7 ), static_cast<UI08>(targetID) );
				return true;
			}
 			else if( a3 == 0 )
			{
				switch( targetID )
				{
					case TARGET_ADDITEM:			AddItem( tSock );						break;
					case TARGET_ADDSCRIPTITEM:		AddScriptItem( tSock );					break;
					case TARGET_ADDNPC:				AddNpc( tSock );						break;
					case TARGET_ADDSCRIPTNPC:		AddScriptNpc( tSock );					break;
					case TARGET_ADDSPAWNER:			AddItem( tSock, OT_SPAWNER );			break;
					case TARGET_ADDSCRIPTSPAWNER:	AddScriptItem( tSock, OT_SPAWNER );		break;
					case TARGET_BUILDHOUSE:			BuildHouseTarget( tSock );				break;
					case TARGET_TELE:				TeleTarget( tSock );					break;	
					case TARGET_DYE:			DyeTarget( tSock );						break;
					case TARGET_KEY:			KeyTarget( tSock );						break;
					case TARGET_DYEALL:			ColorsTarget( tSock );					break;
					case TARGET_DVAT:			DvatTarget( tSock );					break;
					case TARGET_INFO:			InfoTarget( tSock );					break;
					case TARGET_WSTATS:			WstatsTarget( tSock );					break;
					case TARGET_NPCRESURRECT:	NpcResurrectTarget( mChar );			break;
					case TARGET_TWEAK:			TweakTarget( tSock );					break;
					case TARGET_MAKESTATUS:		MakeStatusTarget( tSock );				break;
					case TARGET_SETSCPTRIG:		HandleSetScpTrig( tSock );				break;
					case TARGET_AXE:			AxeTarget( tSock );						break;
					case TARGET_SWORD:			SwordTarget( tSock );					break;
					case TARGET_AREACOMMAND:	AreaCommand( tSock );					break;
					case TARGET_LOADCANNON:		LoadCannon( tSock );					break;
					case TARGET_VIAL:			VialTarget( tSock );					break;
					case TARGET_TILING:			Tiling( tSock );						break;
					case TARGET_WIPING:			Wiping( tSock );						break;
					case TARGET_SHOWSKILLS:		ShowSkillTarget( tSock );				break;
					// Vendors
					case TARGET_MAKESHOP:		MakeShopTarget( tSock );				break;
					case TARGET_PLVBUY:			PlVBuy( tSock );						break;
					// Town Stuff
					case TARGET_TOWNALLY:		MakeTownAlly( tSock );					break;
					case TARGET_VOTEFORMAYOR:	regions[mChar->GetTown()]->VoteForMayor( tSock ); break;
					// House Functions
					case TARGET_HOUSEOWNER:		HouseOwnerTarget( tSock );				break;
					case TARGET_HOUSEEJECT:		HouseEjectTarget( tSock );				break;
					case TARGET_HOUSEBAN:		HouseBanTarget( tSock );				break;
					case TARGET_HOUSEFRIEND:	HouseFriendTarget( tSock );				break;
					case TARGET_HOUSEUNLIST:	HouseUnlistTarget( tSock );				break;
					case TARGET_HOUSELOCKDOWN:	HouseLockdown( tSock );					break;
					case TARGET_HOUSERELEASE:	HouseRelease( tSock );					break;
					// Pets
					case TARGET_FOLLOW:			FollowTarget( tSock );					break;
					case TARGET_ATTACK:			AttackTarget( tSock );					break;
					case TARGET_TRANSFER:		TransferTarget( tSock );				break;
					case TARGET_GUARD:			GuardTarget( tSock );					break;
					case TARGET_FRIEND:			FriendTarget( tSock );					break;
					// Magic
					case TARGET_CASTSPELL:		Magic->CastSpell( tSock, mChar );		break;					
					// Skills Functions
					case TARGET_ARMSLORE:		Skills->ArmsLoreTarget( tSock );		break;
					case TARGET_ANATOMY:		Skills->AnatomyTarget( tSock );			break;
					case TARGET_ITEMID:			Skills->ItemIDTarget( tSock );			break;
					case TARGET_EVALINT:		Skills->EvaluateIntTarget( tSock );		break;
					case TARGET_TAME:			Skills->TameTarget( tSock );			break;
					case TARGET_FISH:			Skills->FishTarget( tSock );			break;
					case TARGET_SMITH:			Skills->Smith( tSock );					break;
					case TARGET_MINE:			Skills->Mine( tSock );					break;
					case TARGET_SMELTORE:		Skills->SmeltOre( tSock );				break;
					case TARGET_REPAIRLEATHER:	Skills->RepairLeather( tSock );			break;
					case TARGET_REPAIRBOW:		Skills->RepairBow( tSock );				break;
					case TARGET_REPAIRMETAL:	Skills->RepairMetal( tSock );			break;
					case TARGET_SMELT:			SmeltTarget( tSock );					break;
					case TARGET_DETECTHIDDEN:	Skills->DetectHidden( tSock );			break;
					case TARGET_PROVOCATION:	Skills->ProvocationTarget1( tSock );	break;
					case TARGET_PROVOCATION2:	Skills->ProvocationTarget2( tSock );	break;
					case TARGET_ENTICEMENT:		Skills->EnticementTarget1( tSock );		break;
					case TARGET_ENTICEMENT2:	Skills->EnticementTarget2( tSock );		break;
					case TARGET_ALCHEMY:		Skills->AlchemyTarget( tSock );			break;
					case TARGET_CREATEBANDAGE:	Skills->CreateBandageTarget( tSock );	break;
					case TARGET_HEALING:		Skills->HealingSkillTarget( tSock );	break;
					case TARGET_CARPENTRY:		Skills->Carpentry( tSock );				break;
					case TARGET_BEGGING:		Skills->BeggingTarget( tSock );			break;
					case TARGET_ANIMALLORE:		Skills->AnimalLoreTarget( tSock );		break;
					case TARGET_FORENSICS:		Skills->ForensicsTarget( tSock );		break;
					case TARGET_APPLYPOISON: 
												tSock->TempObj( calcItemObjFromSer( tSock->GetDWord( 7 ) ) );
												tSock->target( 0, TARGET_POISONITEM, 1613 );
												break;
					case TARGET_POISONITEM:		Skills->PoisoningTarget( tSock );		break;
					case TARGET_INSCRIBE:		Skills->Inscribe( tSock );				break;
					case TARGET_LOCKPICK:		Skills->LockPick( tSock );				break;
					case TARGET_WHEEL:			Skills->Wheel( tSock );					break;
					case TARGET_LOOM:			Skills->Loom( tSock );					break;
					case TARGET_TAILORING:		Skills->Tailoring( tSock );				break;
					case TARGET_COOKING:		Skills->handleCooking( tSock );			break;
					case TARGET_FLETCHING:		Skills->Fletching( tSock );				break;
					case TARGET_TINKERING:		Skills->Tinkering( tSock );				break; 
					case TARGET_TINKERAXEL:		Skills->TinkerAxel( tSock );			break;
					case TARGET_TINKERAWG:		Skills->TinkerAwg( tSock );				break;
					case TARGET_TINKERCLOCK:	Skills->TinkerClock( tSock );			break;
					case TARGET_STEALING:		Skills->StealingTarget( tSock );		break;
					default:					
						if( tSock->GetDWord( 7 ) < BASEITEMSERIAL )
							MultiSetCharTarget( tSock, targetID );
						break;
				}
			}
		}
	}
	mChar->BreakConcentration( tSock );
	return true;
}

}
