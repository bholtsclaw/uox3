// Version History
// 1.0		Abaddon		14th December, 2001
//			Initial implementation
//			Defines the JSClass objects for Char, Item, Race, Guild and Region

#include "UOXJSPropertyFuncs.h"

namespace UOX
{

#ifndef __UOXJSClasses__
#define __UOXJSClasses__

static JSClass UOXSpell_class =
{
	"UOXSpell",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	CSpellProps_getProperty,
	CSpellProps_setProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub 
};

static  JSClass UOXSpells_class =
{
	"UOXSpells",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	CSpellsProps_getProperty,
	JS_PropertyStub,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub 
};

static JSClass UOXChar_class = 
{ 
	"UOXChar",
	JSCLASS_HAS_PRIVATE, 
	JS_PropertyStub,
	JS_PropertyStub,
	CCharacterProps_getProperty,
	CCharacterProps_setProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub 
}; 

static JSClass UOXItem_class = 
{ 
	"UOXItem",
	JSCLASS_HAS_PRIVATE, 
	JS_PropertyStub,
	JS_PropertyStub,
	CItemProps_getProperty,
	CItemProps_setProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub 
};

//
// What this class does:
//  uses JS_GetPrivate to get the assigned item and creates a new JS Item for
//  each "request" made to it. (GetItem)
//

static JSClass UOXItems_class = 
{ 
	"UOXItems",
	JSCLASS_HAS_PRIVATE, 
	JS_PropertyStub,
	JS_PropertyStub,
	CItemsProps_getProperty,
	JS_PropertyStub, 
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub 
};

//
// What this class does:
// Returns the skill-values for a character
// and is able to set them too
//
static JSClass UOXSkills_class = 
{ 
	"UOXSkills",
	JSCLASS_HAS_PRIVATE, 
	JS_PropertyStub,
	JS_PropertyStub,
	CSkillsProps_getProperty,
	CSkillsProps_setProperty, 
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub 
};

static JSClass UOXBaseSkills_class = 
{ 
	"UOXBaseSkills",
	JSCLASS_HAS_PRIVATE, 
	JS_PropertyStub,
	JS_PropertyStub,
	CSkillsProps_getProperty,
	CSkillsProps_setProperty, 
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub 
};

static JSClass UOXSkillsUsed_class = 
{ 
	"UOXSkillsUsed",
	JSCLASS_HAS_PRIVATE, 
	JS_PropertyStub,
	JS_PropertyStub,
	CSkillsProps_getProperty,
	CSkillsProps_setProperty, 
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub 
};

static JSClass UOXSkillsLock_class = 
{ 
	"UOXSkillsLock",
	JSCLASS_HAS_PRIVATE, 
	JS_PropertyStub,
	JS_PropertyStub,
	CSkillsProps_getProperty,
	CSkillsProps_setProperty, 
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub 
};

static JSClass UOXRace_class =
{
	"UOXRace",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	CRaceProps_getProperty,
	CRaceProps_setProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub 
}; 

static JSClass UOXGuild_class =
{
	"UOXGuild",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	CGuildProps_getProperty,
	CGuildProps_setProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub 
}; 

static JSClass UOXGuilds_class =
{
	"UOXGuilds",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	CGuildsProps_getProperty,
	CGuildsProps_setProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub 
}; 

static JSClass UOXRegion_class =
{
	"UOXRegion",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	CRegionProps_getProperty,
	CRegionProps_setProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub 
}; 

static JSClass UOXSocket_class =
{
	"UOXSocket",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	CSocketProps_getProperty,
	CSocketProps_setProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub 
}; 

static JSClass UOXFile_class =
{
	"UOXCFile",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	CFileProps_getProperty,
	CFileProps_setProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub 
}; 

// Added by DarkStorm, 
// var myGump = new Gump; // should be possible
static JSClass UOXGump_class =
{
	"Gump",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	JS_PropertyStub,
	JS_PropertyStub,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub 
}; 

//o--------------------------------------------------------------------------o
//|	Class/Struct	-	static JSClass UOXGumpData_class =
//|	Date					-	1/21/2003 7:35:37 AM
//|	Developers		-	Punt
//|	Organization	-	UOXe Project
//|	Status				-	Currently under development
//o--------------------------------------------------------------------------o
//|	Description		-	
//o--------------------------------------------------------------------------o
//| Modifications	-	
//o--------------------------------------------------------------------------o
static JSClass UOXGumpData_class =
{
	"GumpData",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	CGumpDataProps_getProperty,
	JS_PropertyStub,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub
};

static JSClass UOXAccount_class = 
{
	"CAccountClass",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	CAccountProps_getProperty,
	CAccountProps_setProperty,
	//JS_PropertyStub,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub
};

static JSClass UOXConsole_class = 
{
	"CConsoleClass",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	CConsoleProps_getProperty,
	CConsoleProps_setProperty,
	//JS_PropertyStub,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub
};

static JSClass UOXScriptSection_class =
{
	"UOXScriptSection",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	CScriptSectionProps_getProperty,
	CScriptSectionProps_setProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub 
}; 

}

#endif
