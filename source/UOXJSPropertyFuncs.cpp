//o--------------------------------------------------------------------------o
//|	File			-	UOXJSPropertyFuncts.cpp
//|	Date			-	12/14/2001
//|	Developers		-	Abaddon / EviLDeD
//|	Organization	-	UOX3 DevTeam
//|	Status			-	Currently under development
//o--------------------------------------------------------------------------o
//|	Description		-	1.0		Abaddon		14th December, 2001 Initial implementation
//|									Includes property getters for CItem and CChar, and property
//|									setters for CChar
//o--------------------------------------------------------------------------o
//| Modifications	-	
//o--------------------------------------------------------------------------o
#include "uox3.h"
#include "UOXJSClasses.h"
#include "UOXJSPropertySpecs.h"
#include "UOXJSPropertyEnums.h"
#include "UOXJSPropertyFuncs.h"
#include "JSEncapsulate.h"

#include "cGuild.h"
#include "combat.h"
#include "townregion.h"
#include "cRaces.h"
#include "skills.h"
#include "CJSMapping.h"
#include "cScript.h"
#include "regions.h"
#include "magic.h"
#include "cMagic.h"

namespace UOX
{
	JSBool CGuildsProps_getProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		*vp = INT_TO_JSVAL(0);
		return JS_TRUE;
	}

	JSBool CGuildsProps_setProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		return JS_TRUE;
	}

	JSBool CSpellsProps_getProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		size_t SpellID = JSVAL_TO_INT(id);

		if( SpellID >= Magic->spells.size() || SpellID < 0 )
		{
			Console.Error( 2, "Invalid Spell ID" ); // Revise please...
			*vp = JSVAL_NULL;
			return JS_FALSE;
		}

		SpellInfo *mySpell = &Magic->spells[SpellID];
		if( mySpell == NULL )
		{
			Console.Error( 2, "Invalid Spell" );
			*vp = JSVAL_NULL;
			return JS_FALSE;
		}

		JSObject *jsSpell = JS_NewObject( cx, &UOXSpell_class, NULL, obj );
		JS_DefineProperties( cx, jsSpell, CSpellProperties );
		JS_SetPrivate( cx, jsSpell, mySpell );

		*vp = OBJECT_TO_JSVAL( jsSpell );
		return JS_TRUE;
	}

	JSBool CSpellProps_setProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		return JS_TRUE;
	}

	JSBool CSpellProps_getProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		SpellInfo *gPriv = (SpellInfo*)JS_GetPrivate( cx, obj );
		if( gPriv == NULL )
			return JS_FALSE;
		JSString *tString = NULL;
		size_t i;
		bool bDone = false;

		if( JSVAL_IS_INT( id ) ) 
		{
			switch( JSVAL_TO_INT( id ) )
			{
				case CSP_ID:
					for( i = 0; i < Magic->spells.size() && !bDone; ++i )
					{
						if( &Magic->spells[i] == gPriv ) 
						{
							*vp = INT_TO_JSVAL( i );
							bDone = true;
						}
					}
					break;
				case CSP_ACTION:			*vp = INT_TO_JSVAL( gPriv->Action() );					break;
				case CSP_DELAY:				*vp = INT_TO_JSVAL( gPriv->Delay() );					break;
				case CSP_HEALTH:			*vp = INT_TO_JSVAL( gPriv->Health() );					break;
				case CSP_STAMINA:			*vp = INT_TO_JSVAL( gPriv->Stamina() );					break;
				case CSP_MANA:				*vp = INT_TO_JSVAL( gPriv->Mana() );					break;
				case CSP_MANTRA:			tString = JS_NewStringCopyZ( cx, gPriv->Mantra().c_str() );
											*vp = STRING_TO_JSVAL( tString );
											break;
				case CSP_STRTOSAY:			tString = JS_NewStringCopyZ( cx, gPriv->StringToSay().c_str() );
											*vp = STRING_TO_JSVAL( tString );
											break;
				case CSP_SCROLLLOW:			*vp = INT_TO_JSVAL( gPriv->ScrollLow() );				break;
				case CSP_SCROLLHIGH:		*vp = INT_TO_JSVAL( gPriv->ScrollHigh() );				break;
				case CSP_CIRCLE:			*vp = INT_TO_JSVAL( gPriv->Circle() );					break;
				case CSP_LOWSKILL:			*vp = INT_TO_JSVAL( gPriv->LowSkill() );				break;
				case CSP_HIGHSKILL:			*vp = INT_TO_JSVAL( gPriv->HighSkill() );				break;
				case CSP_GINSENG:			*vp = INT_TO_JSVAL( gPriv->Reagants().ginseng );		break;
				case CSP_MOSS:				*vp = INT_TO_JSVAL( gPriv->Reagants().moss );			break;
				case CSP_DRAKE:				*vp = INT_TO_JSVAL( gPriv->Reagants().drake );			break;
				case CSP_PEARL:				*vp = INT_TO_JSVAL( gPriv->Reagants().pearl );			break;
				case CSP_SILK:				*vp = INT_TO_JSVAL( gPriv->Reagants().silk );			break;
				case CSP_ASH:				*vp = INT_TO_JSVAL( gPriv->Reagants().ash );			break;
				case CSP_SHADE:				*vp = INT_TO_JSVAL( gPriv->Reagants().shade );			break;
				case CSP_GARLIC:			*vp = INT_TO_JSVAL( gPriv->Reagants().garlic );			break;
				case CSP_REQUIRETARGET:		*vp = BOOLEAN_TO_JSVAL( gPriv->RequireTarget() );		break;
				case CSP_REQUIREITEM:		*vp = BOOLEAN_TO_JSVAL( gPriv->RequireItemTarget() );	break;
				case CSP_REQUIRECHAR:		*vp = BOOLEAN_TO_JSVAL( gPriv->RequireCharTarget() );	break;
				case CSP_REQUIRELOCATION:	*vp = BOOLEAN_TO_JSVAL( gPriv->RequireLocTarget() );	break;
				case CSP_TRAVELSPELL:		*vp = BOOLEAN_TO_JSVAL( gPriv->TravelSpell() );			break;
				case CSP_FIELDSPELL:		*vp = BOOLEAN_TO_JSVAL( gPriv->FieldSpell() );			break;
				case CSP_REFLECTABLE:		*vp = BOOLEAN_TO_JSVAL( gPriv->SpellReflectable() );	break;
				case CSP_AGRESSIVESPELL:	*vp = BOOLEAN_TO_JSVAL( gPriv->AggressiveSpell() );		break;
				case CSP_RESISTABLE:		*vp = BOOLEAN_TO_JSVAL( gPriv->Resistable() );			break;
				case CSP_SOUNDEFFECT:		*vp = INT_TO_JSVAL( gPriv->SoundEffect().Effect() );	break;
				case CSP_ENABLED:			*vp = BOOLEAN_TO_JSVAL( gPriv->Enabled() );				break;
				default:																			break;
			}
		}
		return JS_TRUE;
	}

	JSBool CItemProps_getProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		CItem *gPriv = (CItem *)JS_GetPrivate( cx, obj );
		SERIAL TempSerial = INVALIDSERIAL;

		if( !ValidateObject( gPriv ) )
			return JS_FALSE;
		JSString *tString = NULL;
		CRace *TempRace = NULL;
		if( JSVAL_IS_INT( id ) ) 
		{
			switch( JSVAL_TO_INT( id ) )
			{
				case CIP_NAME:
					tString = JS_NewStringCopyZ( cx, gPriv->GetName().c_str() );
					*vp = STRING_TO_JSVAL( tString );
					break;
				case CIP_TITLE:
					tString = JS_NewStringCopyZ( cx, gPriv->GetTitle().c_str() );
					*vp = STRING_TO_JSVAL( tString );
					break;
				case CIP_X:			*vp = INT_TO_JSVAL( gPriv->GetX() );			break;
				case CIP_Y:			*vp = INT_TO_JSVAL( gPriv->GetY() );			break;
				case CIP_Z:			*vp = INT_TO_JSVAL( gPriv->GetZ() );			break;
				case CIP_ID:		*vp = INT_TO_JSVAL( gPriv->GetID() );			break;
				case CIP_COLOUR:	*vp = INT_TO_JSVAL( gPriv->GetColour() );		break;
				case CIP_OWNER:		
					// The owner property is not the pack's owner, but the item's owner.
					// If you want the container's owner, look that up instead
					if(*vp!=JSVAL_NULL)
					{
						CChar *pOwner = gPriv->GetOwnerObj();

						if( !ValidateObject( pOwner ) )
						{
							*vp = JSVAL_NULL;
						}
						else
						{
							// Otherwise Acquire an object
							cScript *myScript	= JSMapping->GetScript( JS_GetGlobalObject( cx ) );
							JSObject *myObj		= myScript->AcquireObject( IUE_CHAR );
							JS_SetPrivate( cx, myObj, pOwner );
							*vp = OBJECT_TO_JSVAL( myObj );
						}
					}
					else
					{
							gPriv->SetOwner( NULL );
					}
					break;
				case CIP_VISIBLE:		*vp = INT_TO_JSVAL( (UI08)gPriv->GetVisible() );	break;
				case CIP_SERIAL:		
					if( gPriv->GetSerial() != INVALIDSERIAL )
					{
						*vp = INT_TO_JSVAL( gPriv->GetSerial() - BASEITEMSERIAL );
					}
					else
					{
						*vp = INT_TO_JSVAL( INVALIDSERIAL );
					}
					break;
				case CIP_HEALTH:		*vp = INT_TO_JSVAL( gPriv->GetHP() );				break;
				case CIP_SCRIPTTRIGGER:	*vp = INT_TO_JSVAL( gPriv->GetScriptTrigger() );	break;
				case CIP_PRIVATEWORD:
					
					break;
				case CIP_WORLDNUMBER:	*vp = INT_TO_JSVAL( gPriv->WorldNumber() );			break;
				case CIP_AMOUNT:		*vp = INT_TO_JSVAL( gPriv->GetAmount() );			break;
				
				// We should Accquie an object here and return that, so you can do
				// item.container.name
				case CIP_CONTAINER:
					TempSerial = gPriv->GetContSerial();

					if( TempSerial == INVALIDSERIAL )
						// Return a JS_NULL
						*vp = JSVAL_NULL;
					else
					{
						// Otherwise Acquire an object
						cScript *myScript	= JSMapping->GetScript( JS_GetGlobalObject( cx ) );
						if( TempSerial >= BASEITEMSERIAL )	// item's have serials of 0x40000000 and above, and we already know it's not INVALIDSERIAL
						{
							JSObject *myItem = myScript->AcquireObject( IUE_ITEM );
							JS_SetPrivate( cx, myItem, calcItemObjFromSer( TempSerial ) );
							*vp = OBJECT_TO_JSVAL( myItem );
						}
						else
						{
							JSObject *myChar = myScript->AcquireObject( IUE_CHAR );
							JS_SetPrivate( cx, myChar, calcCharObjFromSer( TempSerial ) );
							*vp = OBJECT_TO_JSVAL( myChar );
						}
					}

					break;
				case CIP_TYPE:			*vp = INT_TO_JSVAL( static_cast<UI08>(gPriv->GetType()) );		break;
				case CIP_MORE:			*vp = INT_TO_JSVAL( gPriv->GetTempVar( CITV_MORE ) );			break;
				case CIP_MOREX:			*vp = INT_TO_JSVAL( gPriv->GetTempVar( CITV_MOREX ) );			break;
				case CIP_MOREY:			*vp = INT_TO_JSVAL( gPriv->GetTempVar( CITV_MOREY ) );			break;
				case CIP_MOREZ:			*vp = INT_TO_JSVAL( gPriv->GetTempVar( CITV_MOREZ ) );			break;
				case CIP_MOVABLE:		*vp = INT_TO_JSVAL( gPriv->GetMovable() );			break;
				case CIP_ATT:			*vp = INT_TO_JSVAL( RandomNum( gPriv->GetLoDamage(), gPriv->GetHiDamage() ) );	break;
				case CIP_DEF:			*vp = INT_TO_JSVAL( gPriv->GetDef() );				break;
				case CIP_LAYER:			*vp = INT_TO_JSVAL( gPriv->GetLayer() );			break;
				case CIP_ITEMSINSIDE:	*vp = INT_TO_JSVAL( gPriv->GetContainsList()->Num() );		break;
				case CIP_DECAYABLE:		*vp = BOOLEAN_TO_JSVAL( gPriv->isDecayable() );		break;
				case CIP_DECAYTIME:		*vp = INT_TO_JSVAL( gPriv->GetDecayTime() );		break;
				case CIP_LODAMAGE:		*vp = INT_TO_JSVAL( gPriv->GetLoDamage() );			break;
				case CIP_HIDAMAGE:		*vp = INT_TO_JSVAL( gPriv->GetHiDamage() );			break;
				case CIP_NAME2:
					tString = JS_NewStringCopyZ( cx, gPriv->GetName2() );
					*vp = STRING_TO_JSVAL( tString );
					break;
				case CIP_ISCHAR:	*vp = JSVAL_FALSE;								break;
				case CIP_ISITEM:	*vp = JSVAL_TRUE;								break;
				case CIP_ISSPAWNER:	*vp = BOOLEAN_TO_JSVAL(gPriv->GetObjType() == OT_SPAWNER);	break;
				case CIP_RACE:
					TempRace = Races->Race( gPriv->GetRace() );

					if( TempRace == NULL )
					{
						*vp = JSVAL_NULL;
					}
					else
					{
						// Otherwise Acquire an object
						cScript *myScript	= JSMapping->GetScript( JS_GetGlobalObject( cx ) );
						JSObject *myRace	= myScript->AcquireObject( IUE_RACE );
						JS_SetPrivate( cx, myRace, TempRace );
						*vp = OBJECT_TO_JSVAL( myRace );
					}
					break;
				case CIP_RACEID:		*vp = INT_TO_JSVAL( gPriv->GetRace() );			break;
				case CIP_MAXHP:			*vp = INT_TO_JSVAL( gPriv->GetMaxHP() );		break;
				case CIP_RANK:			*vp = INT_TO_JSVAL( gPriv->GetRank() );			break;
				case CIP_POISON:		*vp = INT_TO_JSVAL( gPriv->GetPoisoned() );		break;
				case CIP_DIR:			*vp = INT_TO_JSVAL( gPriv->GetDir() );			break;
				case CIP_WIPABLE:		*vp = INT_TO_JSVAL( gPriv->isWipeable() );		break;
				case CIP_BUYVALUE:		*vp = INT_TO_JSVAL( gPriv->GetBuyValue() );		break;
				case CIP_SELLVALUE:		*vp = INT_TO_JSVAL( gPriv->GetSellValue() );	break;
				case CIP_RESTOCK:		*vp = INT_TO_JSVAL( gPriv->GetRestock() );		break;
				case CIP_DEVINELOCK:	*vp = INT_TO_JSVAL( gPriv->isDevineLocked() );	break;
				case CIP_WEIGHT:		*vp = INT_TO_JSVAL( gPriv->GetWeight() );		break;
				case CIP_STRENGTH:		*vp = INT_TO_JSVAL( gPriv->GetStrength() );		break;
				case CIP_CORPSE:		*vp = BOOLEAN_TO_JSVAL( gPriv->isCorpse() );	break;
				case CIP_DESC:
					tString = JS_NewStringCopyZ( cx, gPriv->GetDesc().c_str() );
					*vp = STRING_TO_JSVAL( tString );
					break;
				case CIP_MURDERTIME:	*vp = INT_TO_JSVAL( gPriv->GetMurderTime() );		break;
				// The following entries are specifically for CSpawnItem objects
				case CIP_SPAWNSECTION:
					if( gPriv->GetObjType() == OT_SPAWNER )
					{
						tString = JS_NewStringCopyZ( cx, ((CSpawnItem *)gPriv)->GetSpawnSection().c_str() );
						*vp = STRING_TO_JSVAL( tString );
					}
					break;
				case CIP_SECTIONALIST:
					if( gPriv->GetObjType() == OT_SPAWNER )
						*vp = INT_TO_JSVAL( ((CSpawnItem *)gPriv)->IsSectionAList() );
					break;
				case CIP_MININTERVAL:
					if( gPriv->GetObjType() == OT_SPAWNER )
						*vp = INT_TO_JSVAL( ((CSpawnItem *)gPriv)->GetInterval( 0 ) );
					break;
				case CIP_MAXINTERVAL:
					if( gPriv->GetObjType() == OT_SPAWNER )
						*vp = INT_TO_JSVAL( ((CSpawnItem *)gPriv)->GetInterval( 1 ) );
					break;
				default:
					break;
			}
		}
		return JS_TRUE;
	}

	JSBool CCharacterProps_getProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		CItem *TempItem			= NULL;
		CRace *TempRace			= NULL;
		JSObject *TempObject	= NULL;
		UI08 TempTownID			= 0xFF;
		GUILDID TempGuildID		= INVALIDID; // should work, shouldn't it ?
		CChar *gPriv			= (CChar *)JS_GetPrivate( cx, obj );

		if( !ValidateObject( gPriv ) )
			return JS_FALSE;
		JSString *tString = NULL;
		if( JSVAL_IS_INT( id ) ) 
		{
			switch( JSVAL_TO_INT( id ) )
			{
				case CCP_NAME:
					tString = JS_NewStringCopyZ( cx, gPriv->GetName().c_str() );
					*vp = STRING_TO_JSVAL( tString );
					break;
				case CCP_TITLE:
					tString = JS_NewStringCopyZ( cx, gPriv->GetTitle().c_str() );
					*vp = STRING_TO_JSVAL( tString );
					break;
				case CCP_X:			*vp = INT_TO_JSVAL( gPriv->GetX() );			break;
				case CCP_Y:			*vp = INT_TO_JSVAL( gPriv->GetY() );			break;
				case CCP_Z:			*vp = INT_TO_JSVAL( gPriv->GetZ() );			break;
				case CCP_ID:		*vp = INT_TO_JSVAL( gPriv->GetID() );			break;
				case CCP_COLOUR:	*vp = INT_TO_JSVAL( gPriv->GetColour() );		break;
				case CCP_OWNER:
					// The Item-thingy for characters as well (damn what possibiliteis we've got :)
					CBaseObject *TempObj;
					TempObj = gPriv->GetOwnerObj();

					if( !ValidateObject( TempObj ) )
					{
						// Return a JS_NULL
						*vp = JSVAL_NULL;
					}
					else
					{
						// Otherwise Acquire an object
						cScript *myScript	= JSMapping->GetScript( JS_GetGlobalObject( cx ) );
						JSObject *myChar	= myScript->AcquireObject( IUE_CHAR );
						JS_SetPrivate( cx, myChar, TempObj );
						*vp = OBJECT_TO_JSVAL( myChar );
					}

					break;
				case CCP_VISIBLE:		*vp = INT_TO_JSVAL( (UI08)gPriv->GetVisible() );	break;
				case CCP_SERIAL:		*vp = INT_TO_JSVAL( gPriv->GetSerial() );			break;
				case CCP_HEALTH:		*vp = INT_TO_JSVAL( gPriv->GetHP() );				break;
				case CCP_SCRIPTTRIGGER:	*vp = INT_TO_JSVAL( gPriv->GetScriptTrigger() );	break;
				case CCP_PRIVATEWORD:
					
					break;
				case CCP_WORLDNUMBER:	*vp = INT_TO_JSVAL( gPriv->WorldNumber() );			break;
				case CCP_TARGET:
					// Hm Quite funny, same thing as .owner
					CChar *tempChar;
					tempChar = gPriv->GetTarg();

					if( !ValidateObject( tempChar ) )
					{
						// Return a JS_NULL
						*vp = JSVAL_NULL;
					}
					else
					{
						// Otherwise Acquire an object
						cScript *myScript	= JSMapping->GetScript( JS_GetGlobalObject( cx ) );
						JSObject *myChar	= myScript->AcquireObject( IUE_CHAR );
						JS_SetPrivate( cx, myChar, tempChar );
						*vp = OBJECT_TO_JSVAL( myChar );
					}

					break;
				case CCP_DEXTERITY:		*vp = INT_TO_JSVAL( gPriv->GetDexterity() );	break;
				case CCP_INTELLIGENCE:	*vp = INT_TO_JSVAL( gPriv->GetIntelligence() );	break;
				case CCP_STRENGTH:		*vp = INT_TO_JSVAL( gPriv->GetStrength() );		break;
				case CCP_BASESKILLS:
					TempObject = JS_NewObject( cx, &UOXBaseSkills_class, NULL, obj );
					JS_DefineProperties( cx, TempObject, CSkillsProps );
					JS_SetPrivate( cx, TempObject, gPriv );
					*vp = OBJECT_TO_JSVAL( TempObject );
					break;
				case CCP_SKILLS:
					TempObject = JS_NewObject( cx, &UOXSkills_class, NULL, obj );
					JS_DefineProperties( cx, TempObject, CSkillsProps );
					JS_SetPrivate( cx, TempObject, gPriv );
					*vp = OBJECT_TO_JSVAL( TempObject );
					break;
				case CCP_SKILLUSE:
					TempObject = JS_NewObject( cx, &UOXSkillsUsed_class, NULL, obj );
					JS_DefineProperties( cx, TempObject, CSkillsProps );
					JS_SetPrivate( cx, TempObject, gPriv );
					*vp = OBJECT_TO_JSVAL( TempObject );
					break;
				case CCP_MANA:			*vp = INT_TO_JSVAL( gPriv->GetMana() );			break;
				case CCP_STAMINA:		*vp = INT_TO_JSVAL( gPriv->GetStamina() );		break;
				
				// Used that to return either JSVAL_NULL if there is no backpack
				// or to return an item object (which is the backpack)
				// sorry, didn't know what the identifier was meant for
				case CCP_CHARPACK:
					TempItem = gPriv->GetPackItem();

					if( !ValidateObject( TempItem ) )
					{
						// Return a JS_NULL
						*vp = JSVAL_NULL;
					}
					else
					{
						// Otherwise Acquire an object
						cScript *myScript	= JSMapping->GetScript( JS_GetGlobalObject( cx ) );
						JSObject *myItem	= myScript->AcquireObject( IUE_ITEM );
						JS_SetPrivate( cx, myItem, TempItem );
						*vp = OBJECT_TO_JSVAL( myItem );
					}
			
					break;
				case CCP_FAME:			*vp = INT_TO_JSVAL( gPriv->GetFame() );						break;
				case CCP_KARMA:			*vp = INT_TO_JSVAL( gPriv->GetKarma() );					break;
				case CCP_DEFENSE:		*vp = INT_TO_JSVAL( Combat->calcDef( gPriv, 0, true ) );	break;
				case CCP_ATTACK:		*vp = INT_TO_JSVAL( Combat->calcAtt( gPriv ) );				break;
				case CCP_HUNGER:		*vp = INT_TO_JSVAL( gPriv->GetHunger() );					break;
				case CCP_FROZEN:		*vp = BOOLEAN_TO_JSVAL( gPriv->IsFrozen() );				break;
				case CCP_COMMANDLEVEL:	*vp = INT_TO_JSVAL( gPriv->GetCommandLevel() );				break;
				// Allocate a race object here and return that... (a lot nicer, isn't it?)
				case CCP_RACE:
					TempRace = Races->Race( gPriv->GetRace() );

					if( TempRace == NULL )
					{
						*vp = JSVAL_NULL;
					}
					else
					{
						// Otherwise Acquire an object
						cScript *myScript	= JSMapping->GetScript( JS_GetGlobalObject( cx ) );
						JSObject *myRace = myScript->AcquireObject( IUE_RACE );
						JS_SetPrivate( cx, myRace, TempRace );
						*vp = OBJECT_TO_JSVAL( myRace );
					}
					
					break;
				case CCP_CRIMINAL:		*vp = BOOLEAN_TO_JSVAL( gPriv->IsCriminal() );	break;
				case CCP_MURDERER:		*vp = BOOLEAN_TO_JSVAL( gPriv->IsMurderer() );	break;
				case CCP_INNOCENT:		*vp = BOOLEAN_TO_JSVAL( gPriv->IsInnocent() );	break;
				case CCP_MURDERCOUNT:	*vp = INT_TO_JSVAL( gPriv->GetKills() );		break;
				case CCP_GENDER:
					if( gPriv->GetID() == 0x0190 || gPriv->GetID() == 0x0192 )
						*vp = INT_TO_JSVAL( 0 );
					else if( gPriv->GetID() == 0x0191 || gPriv->GetID() == 0x0193 )
						*vp = INT_TO_JSVAL( 1 );
					else
						*vp = INT_TO_JSVAL( 2 );
					break;
				case CCP_DEAD:			*vp = BOOLEAN_TO_JSVAL( gPriv->IsDead() );		break;
				case CCP_NPC:			*vp = BOOLEAN_TO_JSVAL( gPriv->IsNpc() );		break;
				case CCP_ONLINE:		*vp = BOOLEAN_TO_JSVAL( isOnline( (*gPriv) ) );	break;
				case CCP_DIRECTION:		*vp = INT_TO_JSVAL( gPriv->GetDir() );			break;
				
				// 3  objects: regions + towns + guilds
				// Goal: myChar.target.textmessage( "Your target is a member of " + myChar.guild.name );
				case CCP_REGION:
					{
						CTownRegion *myReg = gPriv->GetRegion();
						if( myReg == NULL )
							*vp = JSVAL_NULL;
						else
						{
							cScript *myScript	= JSMapping->GetScript( JS_GetGlobalObject( cx ) );
							JSObject *myTown	= myScript->AcquireObject( IUE_REGION );
				
							JS_SetPrivate( cx, myTown, myReg );
							*vp = OBJECT_TO_JSVAL( myTown );
							break;
						}
					}
				case CCP_TOWN:
					TempTownID = gPriv->GetTown();

					// We need to decide here whether 0xFF is a valid town (wilderness) or not
					// i would say no its not
					if( TempTownID == 0xFF )
					{
						*vp = JSVAL_NULL;
					}
					else
					{
						// Should build the town here
						cScript *myScript	= JSMapping->GetScript( JS_GetGlobalObject( cx ) );
						JSObject *myTown	= myScript->AcquireObject( IUE_REGION );
			
						JS_SetPrivate( cx, myTown, regions[TempTownID] );
						*vp = OBJECT_TO_JSVAL( myTown );
					}
					break;
				case CCP_GUILD:
					TempGuildID = gPriv->GetGuildNumber();

					// Character has no guild
					if( TempGuildID == -1 ) // isn't there a constant or something like?
					{
						*vp = JSVAL_NULL;
					}
					else
					{
						// if he has one, lets build our guild !
						cScript *myScript	= JSMapping->GetScript( JS_GetGlobalObject( cx ) );
						JSObject *myGuild	= myScript->AcquireObject( IUE_GUILD );
			
						JS_SetPrivate( cx, myGuild, GuildSys->Guild( TempGuildID ) );
						*vp = OBJECT_TO_JSVAL( myGuild );
					}
					break;
				case CCP_SOCKET:
					{ // So we can declar the variables here
					cScript *myScript	= JSMapping->GetScript( JS_GetGlobalObject( cx ) );
					JSObject *mySock	= myScript->AcquireObject( IUE_SOCK );
					JS_SetPrivate( cx, mySock, calcSocketObjFromChar( gPriv ) );
					*vp = OBJECT_TO_JSVAL( mySock );
					}
					break;
				case CCP_ISCHAR:		*vp = JSVAL_TRUE;									break;
				case CCP_ISITEM:		*vp = JSVAL_FALSE;									break;
				case CCP_ISSPAWNER:		*vp = JSVAL_FALSE;									break;
				case CCP_RACEID:		*vp = INT_TO_JSVAL( gPriv->GetRace() );				break;
				case CCP_MAXHP:			*vp = INT_TO_JSVAL( gPriv->GetMaxHP() );			break;
				case CCP_MAXSTAMINA:	*vp = INT_TO_JSVAL( gPriv->GetMaxStam() );			break;
				case CCP_MAXMANA:		*vp = INT_TO_JSVAL( gPriv->GetMaxMana() );			break;
				case CCP_WANDERTYPE:	*vp = INT_TO_JSVAL( gPriv->GetNpcWander() );		break;
				case CCP_ISONHORSE:		*vp = BOOLEAN_TO_JSVAL( gPriv->IsOnHorse() );		break;
				case CCP_TDEXTERITY:	*vp = INT_TO_JSVAL( gPriv->GetDexterity2() );		break;
				case CCP_TINTELLIGENCE:	*vp = INT_TO_JSVAL( gPriv->GetIntelligence2() );	break;
				case CCP_TSTRENGTH:		*vp = INT_TO_JSVAL( gPriv->GetStrength2() );		break;
				case CCP_POISON:		*vp = INT_TO_JSVAL( gPriv->GetPoisoned() );			break;
				case CCP_LIGHTLEVEL:	*vp = INT_TO_JSVAL( gPriv->GetFixedLight() );		break;
				case CCP_ARMOUR:		*vp = INT_TO_JSVAL( gPriv->GetDef() );				break;
				case CCP_VULNERABLE:	*vp = BOOLEAN_TO_JSVAL( !gPriv->IsInvulnerable() );	break;
				case CCP_HUNGERSTATUS:	*vp = BOOLEAN_TO_JSVAL( gPriv->WillHunger() );		break;
				case CCP_LODAMAGE:		*vp = INT_TO_JSVAL( gPriv->GetLoDamage() );			break;
				case CCP_HIDAMAGE:		*vp = INT_TO_JSVAL( gPriv->GetHiDamage() );			break;
				case CCP_FLAG:			*vp = INT_TO_JSVAL( gPriv->GetFlag() );				break;
				case CCP_ATWAR:			*vp = BOOLEAN_TO_JSVAL( gPriv->IsAtWar() );			break;
				case CCP_SPELLCAST:		*vp = INT_TO_JSVAL( gPriv->GetSpellCast() );		break;
				case CCP_ISCASTING:		*vp = BOOLEAN_TO_JSVAL( gPriv->IsCasting() || gPriv->IsJSCasting() );		break;

				case CCP_TOWNPRIV:		*vp = INT_TO_JSVAL( gPriv->GetTownPriv() );			break;
				case CCP_GUILDTITLE:	
										tString = JS_NewStringCopyZ( cx, gPriv->GetName().c_str() );
										*vp = STRING_TO_JSVAL( tString );
										break;
				case CCP_FONTTYPE:		*vp = INT_TO_JSVAL( gPriv->GetFontType() );			break;
				case CCP_SAYCOLOUR:		*vp = INT_TO_JSVAL( gPriv->GetSayColour() );		break;
				case CCP_EMOTECOLOUR:	*vp = INT_TO_JSVAL( gPriv->GetEmoteColour() );		break;
				case CCP_ATTACKER:
					{
						// Hm Quite funny, same thing as .owner
						CChar *tempChar	= gPriv->GetAttacker();
						if( !ValidateObject( tempChar ) )
						{	// Return a JS_NULL
							*vp = JSVAL_NULL;
						}
						else
						{
							// Otherwise Acquire an object
							cScript *myScript	= JSMapping->GetScript( JS_GetGlobalObject( cx ) );
							JSObject *myChar	= myScript->AcquireObject( IUE_CHAR );
							JS_SetPrivate( cx, myChar, tempChar );
							*vp = OBJECT_TO_JSVAL( myChar );
						}
					}
					break;
				case CCP_RACEGATE:		*vp = INT_TO_JSVAL( gPriv->GetRaceGate() );			break;
				case CCP_SKILLLOCK:
					TempObject	= JS_NewObject( cx, &UOXSkillsLock_class, NULL, obj );
					JS_DefineProperties( cx, TempObject, CSkillsProps );
					JS_SetPrivate( cx, TempObject, gPriv );
					*vp			= OBJECT_TO_JSVAL( TempObject );
					break;
				case CCP_DEATHS:		*vp = INT_TO_JSVAL( gPriv->GetDeaths() );					break;
				case CCP_NEXTACT:		*vp = INT_TO_JSVAL( gPriv->GetNextAct() );					break;
				case CCP_PETCOUNT:		*vp = INT_TO_JSVAL( gPriv->GetPetList()->Num() );			break;
				case CCP_OWNEDITEMSCOUNT:	*vp = INT_TO_JSVAL( gPriv->GetOwnedItems()->size() );	break;
				case CCP_CELL:			*vp = INT_TO_JSVAL( gPriv->GetCell() );						break;
				case CCP_ALLMOVE:		*vp = BOOLEAN_TO_JSVAL( gPriv->AllMove() );					break;
				case CCP_HOUSEICONS:	*vp = BOOLEAN_TO_JSVAL( gPriv->ViewHouseAsIcon() );			break;
				case CCP_SPATTACK:		*vp = INT_TO_JSVAL( gPriv->GetSpAttack() );					break;
				case CCP_SPDELAY:		*vp = INT_TO_JSVAL( gPriv->GetSpDelay() );					break;
				case CCP_AITYPE:		*vp = INT_TO_JSVAL( gPriv->GetNPCAiType() );				break;
				case CCP_SPLIT:			*vp = INT_TO_JSVAL( gPriv->GetSplit() );					break;
				case CCP_SPLITCHANCE:	*vp = INT_TO_JSVAL( gPriv->GetSplitChance() );				break;
				case CCP_TRAINER:		*vp = BOOLEAN_TO_JSVAL( gPriv->CanTrain() );				break;
				case CCP_WEIGHT:		*vp = INT_TO_JSVAL( gPriv->GetWeight() );					break;
				case CCP_SQUELCH:		*vp = INT_TO_JSVAL( gPriv->GetSquelched() );				break;
				case CCP_ISJAILED:		*vp = BOOLEAN_TO_JSVAL( gPriv->IsJailed() );				break;
				case CCP_MAGICREFLECT:	*vp = BOOLEAN_TO_JSVAL( gPriv->IsPermReflected() );			break;
				case CCP_TAMED:			*vp = BOOLEAN_TO_JSVAL( gPriv->IsTamed() );					break;
				case CCP_USINGPOTION:	*vp = BOOLEAN_TO_JSVAL( gPriv->IsUsingPotion() );			break;
				case CCP_STEALTH:		*vp = INT_TO_JSVAL( gPriv->GetStealth() );					break;
				case CCP_SKILLTOTAME:	*vp = INT_TO_JSVAL( gPriv->GetTaming() );					break;
				default:
					break;
			}
		}
		return JS_TRUE;
	}

	JSBool CCharacterProps_setProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		CChar *gPriv = (CChar *)JS_GetPrivate( cx, obj );
		if( !ValidateObject( gPriv ) )
			return JS_FALSE;

		JSEncapsulate encaps( cx, vp );

		if( JSVAL_IS_INT( id ) ) 
		{
			switch( JSVAL_TO_INT( id ) )
			{
				case CCP_NAME:	gPriv->SetName( encaps.toString() );										break;
				case CCP_TITLE:	gPriv->SetTitle( encaps.toString() );										break;
				case CCP_X:		gPriv->SetLocation( (SI16)encaps.toInt(), gPriv->GetY(), gPriv->GetZ() );	break;
				case CCP_Y:		gPriv->SetLocation( gPriv->GetX(), (SI16)encaps.toInt(), gPriv->GetZ() );	break;
				case CCP_Z:		gPriv->SetZ( (SI08)encaps.toInt() );										break;
				case CCP_ID:		
					gPriv->SetID( (UI16)encaps.toInt() );
					gPriv->SetOrgID( (UI16)encaps.toInt() );
					break;
				case CCP_COLOUR:	
					gPriv->SetColour( (UI16)encaps.toInt() );
					gPriv->SetOrgSkin( (UI16)encaps.toInt() );
					break;
				case CCP_OWNER:		
					if( *vp != JSVAL_NULL )
					{
						CChar *myChar = (CChar*)encaps.toObject();
						if( !ValidateObject( myChar ) )
							break;
						gPriv->SetOwner( myChar );
					}
					else
						gPriv->SetOwner( NULL );
					break;
				case CCP_VISIBLE:	gPriv->SetVisible( (VisibleTypes)encaps.toInt() );									break;
				case CCP_SERIAL:	
					
					break;
					// We want other players to see the change don't we
				case CCP_HEALTH:
					gPriv->SetHP( encaps.toInt() ); 
					break;
				case CCP_SCRIPTTRIGGER:	gPriv->SetScriptTrigger( (UI16)encaps.toInt() );			break;
				case CCP_PRIVATEWORD:
					
					break;
				case CCP_WORLDNUMBER:	
					gPriv->SetLocation( gPriv->GetX(), gPriv->GetY(), gPriv->GetZ(), (UI08)encaps.toInt() );
					break;
				case CCP_TARGET:
					//TODO: Check if the user(admin per jscript) can set the target
					if( *vp != JSVAL_NULL )
					{
						CChar *myChar = (CChar *)encaps.toObject();
						if( !ValidateObject( myChar ) )
							break;
						gPriv->SetTarg( myChar );
					}
					else
						gPriv->SetTarg( NULL );
					break;
				case CCP_DEXTERITY:		gPriv->SetDexterity( encaps.toInt() );				break;
				case CCP_INTELLIGENCE:	gPriv->SetIntelligence( encaps.toInt() );			break;
				case CCP_STRENGTH:		gPriv->SetStrength( encaps.toInt() );				break;
				case CCP_SKILLS:															break;
				case CCP_MANA:			gPriv->SetMana( encaps.toInt() );					break;
				case CCP_STAMINA:		gPriv->SetStamina( encaps.toInt() );				break;
				case CCP_CHARPACK:															break;
				case CCP_FAME:			gPriv->SetFame( (SI16)encaps.toInt() );				break;
				case CCP_KARMA:			gPriv->SetKarma( (SI16)encaps.toInt() );			break;
				case CCP_HUNGER:		gPriv->SetHunger( (SI08)encaps.toInt() );			break;
				case CCP_FROZEN:		gPriv->SetFrozen( encaps.toBool() );				break;
				case CCP_COMMANDLEVEL:	gPriv->SetCommandLevel( (UI08)encaps.toInt() );		break;
				case CCP_RACEID:
				case CCP_RACE:			Races->gate( gPriv, (RACEID)encaps.toInt(), true );	break;
				case CCP_CRIMINAL:		
					if( encaps.toBool() )
						criminal( gPriv );
					else
					{
						gPriv->SetTimer( tCHAR_CRIMFLAG, 0 );
						setcharflag( gPriv );
					}
					break;
				case CCP_INNOCENT:
					if( !encaps.toBool() )
						criminal( gPriv );
					else
					{
						gPriv->SetTimer( tCHAR_CRIMFLAG, 0 );
						setcharflag( gPriv );
					}
					break;
				case CCP_MURDERCOUNT:
					gPriv->SetKills( (SI16)encaps.toInt() );
					setcharflag( gPriv );
					break;
				case CCP_GENDER:
					switch( (SI16)encaps.toInt() )
					{
					case 0:	// male
						if( gPriv->IsDead() )
							gPriv->SetID( 0x0192 );
						else
							gPriv->SetID( 0x0190 );
						break;
					case 1:	// female
						if( gPriv->IsDead() )
							gPriv->SetID( 0x0193 );
						else
							gPriv->SetID( 0x0191 );
						break;
					default:
						break;
					}
					break;
				case CCP_NPC:			gPriv->SetNpc( encaps.toBool() );					break;
				case CCP_DIRECTION:		gPriv->SetDir( (UI08)encaps.toInt() );				break;
				case CCP_REGION:		gPriv->SetRegion( (UI08)encaps.toInt() );			break;
				case CCP_TOWN:
					regions[gPriv->GetTown()]->RemoveTownMember( *gPriv );
					regions[encaps.toInt()]->AddAsTownMember( *gPriv );
					break;
				case CCP_GUILD:
					GuildSys->Resign( calcSocketObjFromChar( gPriv ) );

					if( *vp != JSVAL_NULL )
					{
						CGuild *myGuild = (CGuild*)encaps.toObject();
						myGuild->NewRecruit( gPriv->GetSerial() );
					}
					break;
				case CCP_WANDERTYPE:	gPriv->SetNpcWander( (SI08)encaps.toInt() );		break;
				case CCP_TDEXTERITY:	gPriv->SetDexterity2( encaps.toInt() );				break;
				case CCP_TINTELLIGENCE:	gPriv->SetIntelligence2( encaps.toInt() );			break;
				case CCP_TSTRENGTH:		gPriv->SetStrength2( encaps.toInt() );				break;
				case CCP_LIGHTLEVEL:	
					gPriv->SetFixedLight( (UI08)encaps.toInt() );
					if( calcSocketObjFromChar( gPriv ) != NULL )
					{
						if( (UI08)encaps.toInt() == 255 )
							doLight( calcSocketObjFromChar( gPriv ), cwmWorldState->ServerData()->WorldLightCurrentLevel() );
						else
							doLight( calcSocketObjFromChar( gPriv ), (UI08)encaps.toInt() );
					}
					break;
				case CCP_ARMOUR:		gPriv->SetDef( (UI16)encaps.toInt() );			break;
				case CCP_VULNERABLE:	gPriv->SetInvulnerable( !encaps.toBool() );		break;
				case CCP_HUNGERSTATUS:	gPriv->SetHungerStatus( encaps.toBool() );		break;
				case CCP_LODAMAGE:		gPriv->SetLoDamage( (SI16)encaps.toInt() );		break;
				case CCP_HIDAMAGE:		gPriv->SetHiDamage( (SI16)encaps.toInt() );		break;
				case CCP_ATWAR:			gPriv->SetWar( encaps.toBool() );				break;
				case CCP_SPELLCAST:		gPriv->SetSpellCast( (SI08)encaps.toInt() );	break;
				case CCP_ISCASTING:		
					{
										bool isCasting = encaps.toBool();
										gPriv->SetCasting( isCasting );
										gPriv->SetJSCasting( isCasting );
					}
					break;
				case CCP_TOWNPRIV:		gPriv->SetTownpriv( (SI08)encaps.toInt() );			break;
				case CCP_GUILDTITLE:	gPriv->SetGuildTitle( encaps.toString() );			break;
				case CCP_FONTTYPE:		gPriv->SetFontType( (SI08)encaps.toInt() );			break;
				case CCP_SAYCOLOUR:		gPriv->SetSayColour( (COLOUR)encaps.toInt() );		break;
				case CCP_EMOTECOLOUR:	gPriv->SetEmoteColour( (COLOUR)encaps.toInt() );	break;
				case CCP_ATTACKER:
					if( *vp != JSVAL_NULL )
					{
						CChar *myChar = (CChar *)encaps.toObject();
						if( !ValidateObject( myChar ) )
							break;
						gPriv->SetAttacker( myChar );
					}
					else
						gPriv->SetAttacker( NULL );
					break;
				case CCP_RACEGATE:		gPriv->SetRaceGate( (RACEID)encaps.toInt() );		break;
				case CCP_SKILLLOCK:															break;
				case CCP_DEATHS:		gPriv->SetDeaths( (UI16)encaps.toInt() );			break;
				case CCP_NEXTACT:		gPriv->SetNextAct( (UI08)encaps.toInt() );			break;
				case CCP_CELL:			gPriv->SetCell( (SI08)encaps.toInt() );				break;
				case CCP_ALLMOVE:		gPriv->SetAllMove( encaps.toBool() );				break;
				case CCP_HOUSEICONS:	gPriv->SetViewHouseAsIcon( encaps.toBool() );		break;
				case CCP_SPATTACK:		gPriv->SetSpAttack( (SI16)encaps.toInt() );			break;
				case CCP_SPDELAY:		gPriv->SetSpDelay( (SI08)encaps.toInt() );			break;
				case CCP_AITYPE:		gPriv->SetNPCAiType( (SI16)encaps.toInt() );		break;
				case CCP_SPLIT:			gPriv->SetSplit( (UI08)encaps.toInt() );			break;
				case CCP_SPLITCHANCE:	gPriv->SetSplitChance( (UI08)encaps.toInt() );		break;
				case CCP_TRAINER:		gPriv->SetCanTrain( encaps.toBool() );				break;
				case CCP_WEIGHT:		gPriv->SetWeight( (SI32)encaps.toInt() );			break;
				case CCP_SQUELCH:		gPriv->SetSquelched( (UI08)encaps.toInt() );		break;
				case CCP_MAGICREFLECT:	gPriv->SetPermReflected( encaps.toBool() );			break;
				case CCP_TAMED:			gPriv->SetTamed( encaps.toBool() );					break;
				case CCP_USINGPOTION:	gPriv->SetUsingPotion( encaps.toBool() );			break;
				case CCP_STEALTH:		gPriv->SetStealth( encaps.toInt() );				break;
				case CCP_SKILLTOTAME:	gPriv->SetTaming( encaps.toInt() );					break;
			}
		}
		return JS_TRUE;
	}

	JSBool CRegionProps_getProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		CTownRegion *gPriv = (CTownRegion *)JS_GetPrivate( cx, obj );
		if( gPriv == NULL )
			return JS_FALSE;
		JSString *tString = NULL;
		if( JSVAL_IS_INT( id ) ) 
		{
			switch( JSVAL_TO_INT( id ) )
			{
				case CREGP_NAME:
					tString = JS_NewStringCopyZ( cx, gPriv->GetName().c_str() );
					*vp = STRING_TO_JSVAL( tString );
					break;
				case CREGP_MAYOR:
					break;
				case CREGP_RACE:				*vp = INT_TO_JSVAL( gPriv->GetRace() );					break;
				case CREGP_TAX:					*vp = INT_TO_JSVAL( gPriv->TaxedAmount() );				break;
				case CREGP_TAXRESOURCE:			*vp = INT_TO_JSVAL( gPriv->GetResourceID() );			break;
				case CREGP_CANMARK:				*vp = BOOLEAN_TO_JSVAL( gPriv->CanMark() );				break;
				case CREGP_CANRECALL:			*vp = BOOLEAN_TO_JSVAL( gPriv->CanRecall() );			break;
				case CREGP_CANGATE:				*vp = BOOLEAN_TO_JSVAL( gPriv->CanGate() );				break;
				case CREGP_ISGUARDED:			*vp = BOOLEAN_TO_JSVAL( gPriv->IsGuarded() );			break;
				case CREGP_CANCASTAGGRESSIVE:	*vp = BOOLEAN_TO_JSVAL( gPriv->CanCastAggressive() );	break;
				case CREGP_HEALTH:				*vp = INT_TO_JSVAL( gPriv->GetHealth() );				break;
				case CREGP_ISDUNGEON:			*vp = BOOLEAN_TO_JSVAL( gPriv->IsDungeon() );			break;
				case CREGP_CHANCEBIGORE:		*vp = INT_TO_JSVAL( gPriv->GetChanceBigOre() );			break;
				case CREGP_CHANCECOLOURORE:		*vp = INT_TO_JSVAL( gPriv->GetChanceColourOre() );		break;
				case CREGP_NUMOREPREFERENCES:	*vp = INT_TO_JSVAL( gPriv->GetNumOrePreferences() );	break;
				case CREGP_OREPREFERENCES:
					break;
				case CREGP_POPULATION:			*vp = INT_TO_JSVAL( gPriv->GetPopulation() );			break;
				case CREGP_MEMBERS:
				default:
					break;
			}
		}
		return JS_TRUE;
	}
	JSBool CRegionProps_setProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		CTownRegion *gPriv = (CTownRegion *)JS_GetPrivate( cx, obj );
		if( gPriv == NULL )
			return JS_FALSE;
		JSEncapsulate encaps( cx, vp );
		if( JSVAL_IS_INT( id ) ) 
		{
			switch( JSVAL_TO_INT( id ) )
			{
				case CREGP_NAME:				gPriv->SetName( encaps.toString() );				break;
				case CREGP_MAYOR:
					break;
				case CREGP_RACE:				gPriv->SetRace( (RACEID)encaps.toInt() );			break;
				case CREGP_TAX:					gPriv->TaxedAmount( (SI16)encaps.toInt() );			break;
				case CREGP_TAXRESOURCE:			gPriv->SetResourceID( (UI16)encaps.toInt() );		break;
				case CREGP_CANMARK:				gPriv->CanMark( encaps.toBool() );					break;
				case CREGP_CANRECALL:			gPriv->CanRecall( encaps.toBool() );				break;
				case CREGP_CANGATE:				gPriv->CanGate( encaps.toBool() );					break;
				case CREGP_ISGUARDED:			gPriv->IsGuarded( encaps.toBool() );				break;
				case CREGP_CANCASTAGGRESSIVE:	gPriv->CanCastAggressive( encaps.toBool() );		break;
				case CREGP_HEALTH:				gPriv->SetHealth( (SI16)encaps.toInt() );			break;
				case CREGP_ISDUNGEON:			gPriv->IsDungeon( encaps.toBool() );				break;
				case CREGP_CHANCEBIGORE:		gPriv->SetChanceBigOre( (UI08)encaps.toInt() );		break;
				case CREGP_CHANCECOLOURORE:		gPriv->SetChanceColourOre( (UI08)encaps.toInt() );	break;
				case CREGP_OREPREFERENCES:
				case CREGP_MEMBERS:
				default:
					break;
			}
		}
		return JS_TRUE;
	}
	JSBool CGuildProps_getProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		CGuild *gPriv = (CGuild *)JS_GetPrivate( cx, obj );
		if( gPriv == NULL )
			return JS_FALSE;
		JSString *tString = NULL;
		if( JSVAL_IS_INT( id ) ) 
		{
			switch( JSVAL_TO_INT( id ) )
			{
				case CGP_NAME:
					tString = JS_NewStringCopyZ( cx, gPriv->Name().c_str() );
					*vp = STRING_TO_JSVAL( tString );
					break;
				case CGP_TYPE:			*vp = INT_TO_JSVAL( gPriv->Type() );		break;
				case CGP_MASTER:
				case CGP_STONE:
					break;
				case CGP_NUMMEMBERS:	*vp = INT_TO_JSVAL( gPriv->NumMembers() );	break;
				case CGP_NUMRECRUITS:	*vp = INT_TO_JSVAL( gPriv->NumRecruits() );	break;
				case CGP_MEMBERS:
				case CGP_RECRUITS:
					break;
				case CGP_CHARTER:
					tString = JS_NewStringCopyZ( cx, gPriv->Charter().c_str() );
					*vp = STRING_TO_JSVAL( tString );
					break;
				case CGP_ABBREVIATION:
					tString = JS_NewStringCopyZ( cx, gPriv->Abbreviation() );
					*vp = STRING_TO_JSVAL( tString );
					break;
				case CGP_WEBPAGE:
					tString = JS_NewStringCopyZ( cx, gPriv->Webpage().c_str() );
					*vp = STRING_TO_JSVAL( tString );
					break;
				default:
					break;
			}
		}
		return JS_TRUE;
	}
	JSBool CGuildProps_setProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		CGuild *gPriv = (CGuild *)JS_GetPrivate( cx, obj );
		if( gPriv == NULL )
			return JS_FALSE;
		JSEncapsulate encaps( cx, vp );
		if( JSVAL_IS_INT( id ) ) 
		{
			switch( JSVAL_TO_INT( id ) )
			{
				case CGP_NAME:				gPriv->Name( encaps.toString() );					break;
				case CGP_TYPE:				gPriv->Type( (GuildType)encaps.toInt() );			break;
				case CGP_MASTER:
				case CGP_STONE:
				case CGP_MEMBERS:
				case CGP_RECRUITS:
					break;
				case CGP_CHARTER:			gPriv->Charter( encaps.toString() );				break;
				case CGP_ABBREVIATION:		gPriv->Abbreviation( encaps.toString().c_str() );	break;
				case CGP_WEBPAGE:			gPriv->Webpage( encaps.toString() );				break;
				default:
					break;
			}
		}
		return JS_TRUE;
	}
	JSBool CRaceProps_getProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		CRace *gPriv = (CRace *)JS_GetPrivate( cx, obj );
		UI08 TempRace;

		if( gPriv == NULL )
			return JS_FALSE;
		JSString *tString = NULL;
		if( JSVAL_IS_INT( id ) ) 
		{
			switch( JSVAL_TO_INT( id ) )
			{
				case CRP_ID:
					for( TempRace = 0; TempRace < Races->Count(); ++TempRace )
					{
						if( Races->Race( TempRace ) == gPriv )
						{
							*vp = INT_TO_JSVAL( TempRace );
							break;
						}
					}
					break;
				case CRP_NAME:
					tString = JS_NewStringCopyZ( cx, gPriv->Name().c_str() );
					*vp = STRING_TO_JSVAL( tString );
					break;
				case CRP_WEAKTOWEATHER:
					break;
				case CRP_REQUIRESBEARD:		*vp = BOOLEAN_TO_JSVAL( gPriv->RequiresBeard() );		break;
				case CRP_REQUIRESNOBEARD:	*vp = BOOLEAN_TO_JSVAL( gPriv->NoBeard() );				break;
				case CRP_ISPLAYERRACE:		*vp = BOOLEAN_TO_JSVAL( gPriv->IsPlayerRace() );		break;
				case CRP_GENDERRESTRICT:	*vp = INT_TO_JSVAL( gPriv->GenderRestriction() );		break;
				case CRP_ARMOURCLASS:		*vp = INT_TO_JSVAL( gPriv->ArmourClassRestriction() );	break;
				case CRP_LANGUAGESKILLMIN:	*vp = INT_TO_JSVAL( gPriv->LanguageMin() );				break;
				case CRP_SKILLADJUSTMENT:
					break;
				case CRP_POISONRESISTANCE:	*vp = DOUBLE_TO_JSVAL( gPriv->PoisonResistance() );		break;
				case CRP_MAGICRESISTANCE:	*vp = DOUBLE_TO_JSVAL( gPriv->MagicResistance() );		break;
				case CRP_VISIBLEDISTANCE:	*vp = INT_TO_JSVAL( gPriv->VisibilityRange() );			break;
				case CRP_NIGHTVISION:		*vp = INT_TO_JSVAL( gPriv->NightVision() );				break;
				default:
					break;
			}
		}
		return JS_TRUE;
	}

	JSBool CRaceProps_setProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		CRace *gPriv = (CRace *)JS_GetPrivate( cx, obj );
		if( gPriv == NULL )
			return JS_FALSE;
		JSEncapsulate encaps( cx, vp );
		if( JSVAL_IS_INT( id ) ) 
		{
			switch( JSVAL_TO_INT( id ) )
			{
				case CRP_NAME:	gPriv->Name( encaps.toString() );									break;
				case CRP_WEAKTOWEATHER:
					break;
				case CRP_REQUIRESBEARD:		gPriv->RequiresBeard( encaps.toBool() );				break;
				case CRP_REQUIRESNOBEARD:	gPriv->NoBeard( encaps.toBool() );						break;
				case CRP_ISPLAYERRACE:		gPriv->IsPlayerRace( encaps.toBool() );					break;
				case CRP_GENDERRESTRICT:	gPriv->GenderRestriction( (GENDER)encaps.toInt() );		break;
				case CRP_ARMOURCLASS:		gPriv->ArmourClassRestriction( (UI08)encaps.toInt() );	break;
				case CRP_LANGUAGESKILLMIN:	gPriv->LanguageMin( (UI16)encaps.toInt() );				break;
				case CRP_SKILLADJUSTMENT:
					break;
				case CRP_POISONRESISTANCE:	gPriv->PoisonResistance( encaps.toFloat() );			break;
				case CRP_MAGICRESISTANCE:	gPriv->MagicResistance( encaps.toFloat() );				break;
				case CRP_VISIBLEDISTANCE:	gPriv->VisibilityRange( (SI08)encaps.toInt() );			break;
				case CRP_NIGHTVISION:		gPriv->NightVision( (UI08)encaps.toInt() );				break;
				default:
					break;
			}
		}
		return JS_TRUE;
	}

	JSBool CItemProps_setProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		CItem *gPriv = (CItem *)JS_GetPrivate( cx, obj );
		if( !ValidateObject( gPriv ) )
			return JS_FALSE;
		JSEncapsulate encaps( cx, vp );
		if( JSVAL_IS_INT( id ) ) 
		{
			switch( JSVAL_TO_INT( id ) )
			{
				case CIP_NAME:	gPriv->SetName( encaps.toString() );			break;
				case CIP_X:		gPriv->SetLocation( (SI16)encaps.toInt(), gPriv->GetY(), gPriv->GetZ() );				break;
				case CIP_Y:		gPriv->SetLocation( gPriv->GetX(), (SI16)encaps.toInt(), gPriv->GetZ() );				break;
				case CIP_Z:		gPriv->SetZ( (SI08)encaps.toInt() );			break;
				case CIP_ID:		gPriv->SetID( (UI16)encaps.toInt() );		break;
				case CIP_COLOUR:	gPriv->SetColour( (UI16)encaps.toInt() );	break;
				case CIP_OWNER:		
					if( *vp == JSVAL_NULL ) 
					{	 
						CChar *myChar = (CChar*)encaps.toObject(); 
						if( !ValidateObject( myChar ) ) 
							break; 
						gPriv->SetOwner( myChar ); 
					}
					else
						gPriv->SetOwner( NULL );
					break;
				case CIP_VISIBLE:	gPriv->SetVisible( (VisibleTypes)encaps.toInt() );	break;
				case CIP_SERIAL:	
					
					break;
				case CIP_HEALTH:	gPriv->SetHP( (SI16)encaps.toInt() ); 								break;
				case CIP_SCRIPTTRIGGER:	gPriv->SetScriptTrigger( (UI16)encaps.toInt() );				break;
				case CIP_PRIVATEWORD:
					
					break;
				case CIP_WORLDNUMBER:	
					gPriv->SetLocation( gPriv->GetX(), gPriv->GetY(), gPriv->GetZ(), (UI08)encaps.toInt() );
					break;
				case CIP_AMOUNT:	gPriv->SetAmount( (UI32)encaps.toInt() ); 	break;
				case CIP_CONTAINER:
					if( *vp != JSVAL_NULL )
					{
						CBaseObject *myObj = (CBaseObject*)encaps.toObject();
						if( !ValidateObject( myObj ) )
							break;
						gPriv->SetCont( myObj );
					}
					else
						gPriv->SetCont( NULL );
					break;
				case CIP_TYPE:		gPriv->SetType( static_cast<ItemTypes>(encaps.toInt()) ); 		break;
				case CIP_MORE:		gPriv->SetTempVar( CITV_MORE, encaps.toInt() ); 				break;
				case CIP_MOREX:		gPriv->SetTempVar( CITV_MOREX, encaps.toInt() );				break;
				case CIP_MOREY:		gPriv->SetTempVar( CITV_MOREY, encaps.toInt() );				break;
				case CIP_MOREZ:		gPriv->SetTempVar( CITV_MOREZ, encaps.toInt() );				break;
				case CIP_MOVABLE:	gPriv->SetMovable( (SI08)encaps.toInt() );						break;
				case CIP_ATT:		gPriv->SetLoDamage( (SI16)encaps.toInt() ); 	
									gPriv->SetHiDamage( (SI16)encaps.toInt() ); 	
									break;
				case CIP_DEF:		gPriv->SetDef( (UI16)encaps.toInt() );							break;
				case CIP_LAYER:		gPriv->SetLayer( (SI08)encaps.toInt() ); 						break;
				case CIP_ITEMSINSIDE:
					break;
				case CIP_DECAYABLE:	gPriv->SetDecayable( encaps.toBool() );				 			break;
				case CIP_DECAYTIME:	gPriv->SetDecayTime( encaps.toInt() ); 							break;
				case CIP_LODAMAGE:	gPriv->SetLoDamage( (SI16)encaps.toInt() );						break;
				case CIP_HIDAMAGE:	gPriv->SetHiDamage( (SI16)encaps.toInt() );						break;
				case CIP_NAME2:		gPriv->SetName2( encaps.toString().c_str() );					break;
				case CIP_RACEID:
				case CIP_RACE:		gPriv->SetRace( (RACEID)encaps.toInt() );						break;
				case CIP_MAXHP:		gPriv->SetMaxHP( (SI16)encaps.toInt() );						break;
				case CIP_RANK:		gPriv->SetRank( (SI08)encaps.toInt() );							break;
				case CIP_DIR:		gPriv->SetDir( (SI16)encaps.toInt() );							break;
				case CIP_WIPABLE:	gPriv->SetWipeable( encaps.toBool() );							break;
				case CIP_BUYVALUE:	gPriv->SetBuyValue( (UI32)encaps.toInt() );						break;
				case CIP_SELLVALUE:	gPriv->SetSellValue( (UI32)encaps.toInt() );					break;
				case CIP_RESTOCK:	gPriv->SetRestock( (UI16)encaps.toInt() );						break;
				case CIP_DEVINELOCK:gPriv->SetDevineLock( encaps.toBool() );						break;
				case CIP_WEIGHT:	gPriv->SetWeight( (SI32)encaps.toInt() );						break;
				case CIP_STRENGTH:	gPriv->SetStrength( (SI16)encaps.toInt() );						break;
				case CIP_CORPSE:	gPriv->SetCorpse( encaps.toBool() );							break;
				case CIP_DESC:		gPriv->SetDesc( encaps.toString() );							break;
				case CIP_MURDERTIME:gPriv->SetMurderTime( encaps.toInt() );							break;
				// The following entries are specifically for CSpawnItem objects
				case CIP_SPAWNSECTION:
					if( gPriv->GetObjType() == OT_SPAWNER )
						((CSpawnItem *)gPriv)->SetSpawnSection( encaps.toString() );
					break;
				case CIP_SECTIONALIST:
					if( gPriv->GetObjType() == OT_SPAWNER )
						((CSpawnItem *)gPriv)->IsSectionAList( encaps.toBool() );
					break;
				case CIP_MININTERVAL:
					if( gPriv->GetObjType() == OT_SPAWNER )
						((CSpawnItem *)gPriv)->SetInterval( 0, (UI08)encaps.toInt() );
					break;
				case CIP_MAXINTERVAL:
					if( gPriv->GetObjType() == OT_SPAWNER )
						((CSpawnItem *)gPriv)->SetInterval( 1, (UI08)encaps.toInt() );
					break;
				default:
					break;
			}
		}
		return JS_TRUE;
	}

	JSBool CSocketProps_setProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		CSocket *gPriv = (CSocket *)JS_GetPrivate( cx, obj );
		if( gPriv == NULL )
			return JS_FALSE;
		JSEncapsulate encaps( cx, vp );
		if( JSVAL_IS_INT( id ) ) 
		{
			switch( JSVAL_TO_INT( id ) )
			{
				case CSOCKP_ACCOUNT:
					break;
				case CSOCKP_CURRENTCHAR:
					{
						CChar *mChar = (CChar *)encaps.toObject();
						if( ValidateObject( mChar ) )
							gPriv->CurrcharObj( mChar );
					}
					break;
				case CSOCKP_IDLETIMEOUT:
					break;
				case CSOCKP_WASIDLEWARNED:		gPriv->WasIdleWarned( encaps.toBool() );				break;
				case CSOCKP_TEMPINT:			gPriv->TempInt( encaps.toInt() );						break;
				case CSOCKP_TEMPOBJ:
												if( *vp == JSVAL_NULL )
													gPriv->TempObj( NULL );
												else
													gPriv->TempObj( (CBaseObject *)encaps.toObject() );		break;
				case CSOCKP_BUFFER:
					break;
				case CSOCKP_XTEXT:				gPriv->XText( encaps.toString() );						break;
				case CSOCKP_CLICKZ:				gPriv->ClickZ( (SI08)encaps.toInt() );					break;
				case CSOCKP_ADDID:
					break;
				case CSOCKP_NEWCLIENT:			gPriv->NewClient( encaps.toBool() );					break;
				case CSOCKP_FIRSTPACKET:		gPriv->FirstPacket( encaps.toBool() );					break;
				case CSOCKP_CRYPTCLIENT:		gPriv->CryptClient( encaps.toBool() );					break;
				case CSOCKP_CLIENTIP:
					break;
				case CSOCKP_WALKSEQUENCE:		gPriv->WalkSequence( (SI16)encaps.toInt() );			break;
				case CSOCKP_CURRENTSPELLTYPE:	gPriv->CurrentSpellType( (SI08)encaps.toInt() );		break;
					break;
				case CSOCKP_LOGGING:			gPriv->Logging( encaps.toBool() );						break;
				case CSOCKP_BYTESSENT:
				case CSOCKP_BYTESRECEIVED:
					break;
				case CSOCKP_TARGETOK:			gPriv->TargetOK( encaps.toBool() );						break;
				case CSOCKP_CLICKX:				gPriv->ClickX( (SI16)encaps.toInt() );					break;
				case CSOCKP_CLICKY:				gPriv->ClickY( (SI16)encaps.toInt() );					break;
				case CSOCKP_PICKUPX:			gPriv->PickupX( (SI16)encaps.toInt() );					break;
				case CSOCKP_PICKUPY:			gPriv->PickupY( (SI16)encaps.toInt() );					break;
				case CSOCKP_PICKUPZ:			gPriv->PickupZ( (SI08)encaps.toInt() );					break;
				case CSOCKP_PICKUPSPOT:			gPriv->PickupSpot( (PickupLocations)encaps.toInt() );	break;
				case CSOCKP_PICKUPSERIAL:
					break;
				case CSOCKP_LANGUAGE:			gPriv->Language( (UnicodeTypes)encaps.toInt() );		break;
				case CSOCKP_CLIENTMAJORVER:		gPriv->ClientVersionMajor( (UI08)encaps.toInt() );		break;
				case CSOCKP_CLIENTMINORVER:		gPriv->ClientVersionMinor( (UI08)encaps.toInt() );		break;
				case CSOCKP_CLIENTSUBVER:		gPriv->ClientVersionSub( (UI08)encaps.toInt() );		break;
				case CSOCKP_CLIENTLETTERVER:	gPriv->ClientVersionLetter( (UI08)encaps.toInt() );		break;
				case CSOCKP_CLIENTTYPE:			gPriv->ClientType( (ClientTypes)encaps.toInt() );		break;
				default:
					break;
			}
		}
		return JS_TRUE;
	}

	JSBool CSocketProps_getProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		CSocket *gPriv = (CSocket *)JS_GetPrivate( cx, obj );
		CChar *myChar;
		JSString *tString = NULL;
		if( gPriv == NULL )
			return JS_FALSE;
		if( JSVAL_IS_INT( id ) ) 
		{
			switch( JSVAL_TO_INT( id ) )
			{
				case CSOCKP_ACCOUNT:
				case CSOCKP_CURRENTCHAR:
					myChar = gPriv->CurrcharObj();
					if( !ValidateObject( myChar ) )
					{
						*vp = JSVAL_NULL;
					}
					else
					{
						cScript *myScript	= JSMapping->GetScript( JS_GetGlobalObject( cx ) );
						JSObject *myObj		= myScript->AcquireObject( IUE_CHAR );
						JS_SetPrivate( cx, myObj, gPriv->CurrcharObj() );
						*vp = OBJECT_TO_JSVAL( myObj );
					}

				case CSOCKP_IDLETIMEOUT:
					break;
				case CSOCKP_WASIDLEWARNED:		*vp = BOOLEAN_TO_JSVAL( gPriv->WasIdleWarned() );		break;
				case CSOCKP_TEMPINT:			*vp = INT_TO_JSVAL( gPriv->TempInt() );					break;
				case CSOCKP_TEMPOBJ:
					{
						cScript *myScript	= JSMapping->GetScript( JS_GetGlobalObject( cx ) );
						CBaseObject *mObj	= gPriv->TempObj();
						if( !ValidateObject( mObj ) )
							*vp = JSVAL_NULL;
						else
						{
							JSObject *myObj = NULL;
							if( mObj->CanBeObjType( OT_ITEM ) )
								myObj = myScript->AcquireObject( IUE_ITEM );
							else
								myObj = myScript->AcquireObject( IUE_CHAR );
							JS_SetPrivate( cx, myObj, mObj );
							*vp = OBJECT_TO_JSVAL( myObj );
						}
					}
					break;
				case CSOCKP_BUFFER:
					break;
				case CSOCKP_XTEXT:				tString = JS_NewStringCopyZ( cx, gPriv->XText().c_str() );
												*vp = STRING_TO_JSVAL( tString );						break;
				case CSOCKP_CLICKZ:				*vp = INT_TO_JSVAL( gPriv->ClickZ() );					break;
				case CSOCKP_ADDID:
					break;
				case CSOCKP_NEWCLIENT:			*vp = BOOLEAN_TO_JSVAL( gPriv->NewClient() );			break;
				case CSOCKP_FIRSTPACKET:		*vp = BOOLEAN_TO_JSVAL( gPriv->FirstPacket() );			break;
				case CSOCKP_CRYPTCLIENT:		*vp = BOOLEAN_TO_JSVAL( gPriv->CryptClient() );			break;
				case CSOCKP_CLIENTIP:
					break;
				case CSOCKP_WALKSEQUENCE:		*vp = INT_TO_JSVAL( gPriv->WalkSequence() );			break;
				case CSOCKP_CURRENTSPELLTYPE:	*vp = INT_TO_JSVAL( gPriv->CurrentSpellType() );		break;
				case CSOCKP_LOGGING:			*vp = BOOLEAN_TO_JSVAL( gPriv->Logging() );				break;
				case CSOCKP_BYTESSENT:			*vp = INT_TO_JSVAL( gPriv->BytesSent() );				break;
				case CSOCKP_BYTESRECEIVED:		*vp = INT_TO_JSVAL( gPriv->BytesReceived() );			break;
				case CSOCKP_TARGETOK:			*vp = BOOLEAN_TO_JSVAL( gPriv->TargetOK() );			break;
				case CSOCKP_CLICKX:				*vp = INT_TO_JSVAL( gPriv->ClickX() );					break;
				case CSOCKP_CLICKY:				*vp = INT_TO_JSVAL( gPriv->ClickY() );					break;
				case CSOCKP_PICKUPX:			*vp = INT_TO_JSVAL( gPriv->PickupX() );					break;
				case CSOCKP_PICKUPY:			*vp = INT_TO_JSVAL( gPriv->PickupY() );					break;
				case CSOCKP_PICKUPZ:			*vp = INT_TO_JSVAL( gPriv->PickupZ() );					break;
				case CSOCKP_PICKUPSPOT:			*vp = INT_TO_JSVAL( gPriv->PickupSpot() );				break;
				case CSOCKP_PICKUPSERIAL:
					break;
				case CSOCKP_LANGUAGE:			*vp = INT_TO_JSVAL( gPriv->Language() );				break;
				case CSOCKP_CLIENTMAJORVER:		*vp = INT_TO_JSVAL( gPriv->ClientVersionMajor() );		break;
				case CSOCKP_CLIENTMINORVER:		*vp = INT_TO_JSVAL( gPriv->ClientVersionMinor() );		break;
				case CSOCKP_CLIENTSUBVER:		*vp = INT_TO_JSVAL( gPriv->ClientVersionSub() );		break;
				case CSOCKP_CLIENTLETTERVER:	*vp = INT_TO_JSVAL( gPriv->ClientVersionLetter() );		break;
				case CSOCKP_CLIENTTYPE:			*vp = INT_TO_JSVAL( gPriv->ClientType() );				break;
				case CSOCKP_TARGET:
					{
						SERIAL mySerial		= gPriv->GetDWord( 7 );
						cScript *myScript	= JSMapping->GetScript( JS_GetGlobalObject( cx ) );
						// Item
						if( mySerial >= BASEITEMSERIAL )
						{
							CItem *myItem = calcItemObjFromSer( mySerial );

							if( !ValidateObject( myItem ) )
							{
								*vp = JSVAL_NULL;
								return JS_TRUE;
							}

							JSObject *myObj = myScript->AcquireObject( IUE_ITEM );
							JS_SetPrivate( cx, myObj,  myItem);
							*vp = OBJECT_TO_JSVAL( myObj );
						}
						// Char
						else
						{
							CChar *myChar = calcCharObjFromSer( mySerial );

							if( !ValidateObject( myChar ) )
							{
								*vp = JSVAL_NULL;
								return JS_TRUE;
							}

							JSObject *myObj = myScript->AcquireObject( IUE_CHAR );
							JS_SetPrivate( cx, myObj,  myChar);
							*vp = OBJECT_TO_JSVAL( myObj );
						}

						return JS_TRUE;
					}
					break;
				default:
					break;
			}
		}
		return JS_TRUE;
	}

	JSBool CItemsProps_getProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		// Create a new JS Object representing the accquired object...
		// IF SOMEONE IS ENUMERATING THOSE OBJECTS THIS COULD CAUSE OUT_OF_MEMORY !
		CItem *myItem = (CItem*)JS_GetPrivate( cx, JS_GetParent( cx, obj ) );

		if( !ValidateObject( myItem ) )
			return JS_FALSE;
		
		UI32 Index			= JSVAL_TO_INT( id );
		CItem *mySubItem	= myItem->GetContainsList()->GetCurrent( Index );
		if( !ValidateObject( mySubItem ) )
		{
			*vp = JSVAL_NULL;
			return JS_TRUE;
		}
		
		// Otherwise Acquire an object
		cScript *myScript	= JSMapping->GetScript( JS_GetGlobalObject( cx ) );
		JSObject *myJSItem	= myScript->AcquireObject( IUE_ITEM );
		JS_SetPrivate( cx, myJSItem, mySubItem );
		*vp = OBJECT_TO_JSVAL( myJSItem );
		
		return JS_TRUE;
	}

	JSBool CSkillsProps_getProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		CChar *myChar = (CChar*)JS_GetPrivate( cx, obj );

		if( !ValidateObject( myChar ) )
			return JS_FALSE;
		
		UI08 SkillID		= (UI08)JSVAL_TO_INT( id );
		JSClass *myClass	= JS_GetClass( obj );

		if( !strcmp( myClass->name, "UOXSkills" ) )
			*vp = INT_TO_JSVAL( myChar->GetSkill( SkillID ) );
		else if( !strcmp( myClass->name, "UOXBaseSkills" ) )
			*vp = INT_TO_JSVAL( myChar->GetBaseSkill( SkillID ) );
		else if( !strcmp( myClass->name, "UOXSkillsUsed" ) )
			*vp = BOOLEAN_TO_JSVAL( myChar->SkillUsed( SkillID ) );
		else if( !strcmp( myClass->name, "UOXSkillsLock" ) )
			*vp = INT_TO_JSVAL( myChar->GetSkillLock( SkillID ) );

		return JS_TRUE;
	}

	JSBool CSkillsProps_setProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		CChar *myChar = (CChar*)JS_GetPrivate( cx, obj );

		if( !ValidateObject( myChar ) )
			return JS_FALSE;
		
		JSEncapsulate encaps( cx, vp );
		UI08 SkillID		= (UI08)JSVAL_TO_INT( id );
		JSClass *myClass	= JS_GetClass( obj );	
		SI16 NewSkillValue	= (SI16)encaps.toInt();
		UI08 i				= 0;

		if( !strcmp( myClass->name, "UOXSkills" ) )
		{
			if( SkillID == ALLSKILLS )
			{
				for( i = 0; i < ALLSKILLS; ++i )
				{
					myChar->SetSkill( NewSkillValue, i );
				}
			}
			else
				myChar->SetSkill( NewSkillValue, SkillID );
		}
		else if( !strcmp( myClass->name, "UOXBaseSkills" ) )
		{
			if( SkillID == ALLSKILLS )
			{
				for( i = 0; i < ALLSKILLS; ++i )
				{
					myChar->SetBaseSkill( NewSkillValue, i );
					Skills->updateSkillLevel( myChar, i );
				}
			}
			else
			{
				myChar->SetBaseSkill( NewSkillValue, SkillID );
				Skills->updateSkillLevel( myChar, SkillID );
			}
		}
		else if( !strcmp( myClass->name, "UOXSkillsUsed" ) )
		{
			if( SkillID == ALLSKILLS )
			{
				for( i = 0; i < ALLSKILLS; ++i )
				{
					myChar->SkillUsed( encaps.toBool(), i );
				}
			}
			else
				myChar->SkillUsed( encaps.toBool(), SkillID );
		}
		else if( !strcmp( myClass->name, "UOXSkillsLock" ) )
		{
			if( SkillID == ALLSKILLS )
			{
				for( i = 0; i < ALLSKILLS; ++i )
				{
					myChar->SetSkillLock( (UI08)NewSkillValue, i );
				}
			}
			else
				myChar->SetSkillLock( (UI08)NewSkillValue, SkillID );
		}

		if( !myChar->IsNpc() )
		{
			CSocket *toFind = calcSocketObjFromChar( myChar );
			if( toFind != NULL )
			{
				if( SkillID == ALLSKILLS )
				{
					for( i = 0; i < ALLSKILLS; ++i )
					{
						toFind->updateskill( i );
					}
				}
				else
					toFind->updateskill( SkillID );
			}
		}
		return JS_TRUE;
	}

	JSBool CGumpDataProps_getProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		SEGumpData *gPriv = (SEGumpData *)JS_GetPrivate( cx, obj );
		//SEGump *toDelete = (SEGump*)JS_GetPrivate( cx, obj );
		
		if( gPriv == NULL )
			return JS_FALSE;
		
		if( JSVAL_IS_INT( id ) ) 
		{
			switch( JSVAL_TO_INT( id ) )
			{
				case CGumpData_ID:
					*vp = INT_TO_JSVAL( gPriv->nIDs.size()) ;
					break ;
				case CGumpData_Button:
					*vp = INT_TO_JSVAL( gPriv->nButtons.size()) ;
					break ;
				default:
					break;
			}
		}
		return JS_TRUE;
	}

	JSBool CFileProps_getProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		UOXFileWrapper *gPriv = (UOXFileWrapper *)JS_GetPrivate( cx, obj );
		if( gPriv == NULL || gPriv->mWrap == NULL )
			return JS_FALSE;
		if( JSVAL_IS_INT( id ) ) 
		{
			switch( JSVAL_TO_INT( id ) )
			{
				case CFILE_POS:		*vp = INT_TO_JSVAL( ftell( gPriv->mWrap ) );				break;
				case CFILE_LENGTH:
					{
						long fpos = ftell( gPriv->mWrap );
						fseek( gPriv->mWrap, 0, SEEK_END );
						*vp = INT_TO_JSVAL( ftell( gPriv->mWrap ) );
						fseek( gPriv->mWrap, fpos, SEEK_SET );
					}
					break;
				case CFILE_EOF:		*vp = BOOLEAN_TO_JSVAL( ( feof( gPriv->mWrap ) != 0 ) );	break;
					break;
				default:
					break;
			}
		}
		return JS_TRUE;
	}
	JSBool CFileProps_setProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		UOXFileWrapper *gPriv = (UOXFileWrapper *)JS_GetPrivate( cx, obj );
		if( gPriv == NULL )
			return JS_FALSE;
		JSEncapsulate encaps( cx, vp );
		if( JSVAL_IS_INT( id ) ) 
		{
			switch( JSVAL_TO_INT( id ) )
			{
				case CFILE_POS:	fseek( gPriv->mWrap, encaps.toInt(), SEEK_SET );	break;
				default:
					break;
			}
		}
		return JS_TRUE;
	}

	JSBool CAccountProps_getProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		if( JSVAL_IS_INT( id ) ) 
		{
	/*		switch( JSVAL_TO_INT( id ) )
			{
			default:
				break;
			}*/
		}
		return JS_TRUE;
	}

	JSBool CAccountProps_setProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		return JS_FALSE;
	}

	JSBool CConsoleProps_getProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		if( JSVAL_IS_INT( id ) ) 
		{
			switch( JSVAL_TO_INT( id ) )
			{
			case CCONSOLE_LEFT:		*vp = INT_TO_JSVAL( Console.Left() );			break;
			case CCONSOLE_TOP:		*vp = INT_TO_JSVAL( Console.Top() );			break;
			case CCONSOLE_HEIGHT:	*vp = INT_TO_JSVAL( Console.Height() );			break;
			case CCONSOLE_WIDTH:	*vp = INT_TO_JSVAL( Console.Width() );			break;
			case CCONSOLE_FILTER:	*vp = INT_TO_JSVAL( Console.FilterSetting() );	break;
			case CCONSOLE_MODE:		*vp = INT_TO_JSVAL( Console.CurrentMode() );	break;
			case CCONSOLE_LEVEL:	*vp = INT_TO_JSVAL( Console.CurrentLevel() );	break;
			case CCONSOLE_LOGECHO:	*vp = INT_TO_JSVAL( Console.LogEcho() );		break;
			default:
				break;
			}
		}
		return JS_TRUE;
	}

	JSBool CConsoleProps_setProperty( JSContext *cx, JSObject *obj, jsval id, jsval *vp )
	{
		JSEncapsulate encaps( cx, vp );
		if( JSVAL_IS_INT( id ) ) 
		{
			switch( JSVAL_TO_INT( id ) )
			{
			case CCONSOLE_LEFT:		Console.Left( encaps.toInt() );				break;
			case CCONSOLE_TOP:		Console.Top( encaps.toInt() );				break;
			case CCONSOLE_HEIGHT:	Console.Height( encaps.toInt() );			break;
			case CCONSOLE_WIDTH:	Console.Width( encaps.toInt() );			break;
			case CCONSOLE_FILTER:	Console.FilterSetting( encaps.toInt() );	break;
			case CCONSOLE_MODE:		Console.CurrentMode( encaps.toInt() );		break;
			case CCONSOLE_LEVEL:	Console.CurrentLevel( encaps.toInt() );		break;
			case CCONSOLE_LOGECHO:	Console.LogEcho( encaps.toBool() );			break;
			default:
				break;
			}
		}
		return JS_TRUE;
	}

}
