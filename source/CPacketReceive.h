#ifndef __CPACKETRECEIVE_H__
#define __CPACKETRECEIVE_H__

#include "network.h"

namespace UOX
{

class CPIFirstLogin : public CPInputBuffer
{
protected:
	virtual void		InternalReset( void );
	std::string			userID;
	std::string			password;
	UI08				unknown;
public:
	virtual				~CPIFirstLogin()
	{
	}
						CPIFirstLogin();
						CPIFirstLogin( CSocket *s );
	virtual void		Receive( void );
	virtual const std::string Name( void );
	virtual const std::string Pass( void );
	virtual UI08		Unknown( void );
	virtual bool		Handle( void );
	virtual void		Log( std::ofstream &outStream, bool fullHeader = true );
};

class CPIServerSelect : public CPInputBuffer
{
protected:
	virtual void	InternalReset( void );
public:
	virtual			~CPIServerSelect()
	{
	}
					CPIServerSelect();
					CPIServerSelect( CSocket *s );
	virtual void	Receive( void );
	virtual SI16	ServerNum( void );
	virtual bool	Handle( void );
	virtual void	Log( std::ofstream &outStream, bool fullHeader = true );
};

class CPISecondLogin : public CPInputBuffer
{
protected:
	virtual void		InternalReset( void );
	UI32				keyUsed;
	std::string			sid;
	std::string			password;
public:
	virtual				~CPISecondLogin()
	{
	}
						CPISecondLogin();
						CPISecondLogin( CSocket *s );
	virtual void		Receive( void );
	virtual UI32		Account( void );
	virtual const std::string Name( void );
	virtual const std::string Pass( void );
	virtual bool		Handle( void );
	virtual void		Log( std::ofstream &outStream, bool fullHeader = true );
};

class CPIClientVersion : public CPInputBuffer
{
protected:
	virtual void	InternalReset( void );
	char *			Offset( void );
	SI16			len;
public:
	virtual			~CPIClientVersion()
	{
	}
					CPIClientVersion();
					CPIClientVersion( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
	virtual void	Log( std::ofstream &outStream, bool fullHeader = true );
};

class CPIUpdateRangeChange : public CPInputBuffer
{
protected:
	virtual void	InternalReset( void );
public:
	virtual			~CPIUpdateRangeChange()
	{
	}
					CPIUpdateRangeChange();
					CPIUpdateRangeChange( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
	virtual void	Log( std::ofstream &outStream, bool fullHeader = true );
};

class CPITips : public CPInputBuffer
{
protected:
	virtual void	InternalReset( void );
public:
	virtual			~CPITips()
	{
	}
					CPITips();
					CPITips( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
};

class CPIRename : public CPInputBuffer
{
protected:
	virtual void	InternalReset( void );
public:
	virtual			~CPIRename()
	{
	}
					CPIRename();
					CPIRename( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
};

class CPIKeepAlive : public CPInputBuffer
{
public:
	virtual			~CPIKeepAlive()
	{
	}
					CPIKeepAlive();
					CPIKeepAlive( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
};

class CPIStatusRequest : public CPInputBuffer
{
	UI32			pattern;
	UI08			getType;
	SERIAL			playerID;
public:
	virtual			~CPIStatusRequest()
	{
	}
					CPIStatusRequest();
					CPIStatusRequest( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
	virtual void	Log( std::ofstream &outStream, bool fullHeader = true );
};

class CPISpy : public CPInputBuffer
{
public:
	virtual			~CPISpy()
	{
	}
					CPISpy();
					CPISpy( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
};

class CPIGodModeToggle : public CPInputBuffer
{
public:
	virtual			~CPIGodModeToggle()
	{
	}
					CPIGodModeToggle();
					CPIGodModeToggle( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
};

class CPIDblClick : public CPInputBuffer
{
protected:
	SERIAL			objectID;
public:
	virtual			~CPIDblClick()
	{
	}
					CPIDblClick();
					CPIDblClick( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
	virtual void	Log( std::ofstream &outStream, bool fullHeader = true );
};

class CPISingleClick : public CPInputBuffer
{
protected:
	SERIAL			objectID;
public:
	virtual			~CPISingleClick()
	{
	}
					CPISingleClick();
					CPISingleClick( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
	virtual void	Log( std::ofstream &outStream, bool fullHeader = true );
};

class CPIMoveRequest : public CPInputBuffer
{
public:
	virtual			~CPIMoveRequest()
	{
	}
					CPIMoveRequest();
					CPIMoveRequest( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
};

class CPIResyncReq : public CPInputBuffer
{
public:
	virtual			~CPIResyncReq()
	{
	}
					CPIResyncReq();
					CPIResyncReq( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
};

class CPIResMenuChoice : public CPInputBuffer
{
public:
	virtual			~CPIResMenuChoice()
	{
	}
					CPIResMenuChoice();
					CPIResMenuChoice( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
};

class CPIAttack : public CPInputBuffer
{
public:
	virtual			~CPIAttack()
	{
	}
					CPIAttack();
					CPIAttack( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
};

class CPITargetCursor : public CPInputBuffer
{
public:
	virtual			~CPITargetCursor()
	{
	}
					CPITargetCursor();
					CPITargetCursor( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
};

class CPIEquipItem : public CPInputBuffer
{
public:
	virtual			~CPIEquipItem()
	{
	}
					CPIEquipItem();
					CPIEquipItem( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
};

class CPIGetItem : public CPInputBuffer
{
public:
	virtual			~CPIGetItem()
	{
	}
					CPIGetItem();
					CPIGetItem( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
};

class CPIDropItem : public CPInputBuffer
{
public:
	virtual			~CPIDropItem()
	{
	}
					CPIDropItem();
					CPIDropItem( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
};

class CPIGumpMenuSelect : public CPInputBuffer
{
public:
					CPIGumpMenuSelect();
					CPIGumpMenuSelect( CSocket *s );
	virtual			~CPIGumpMenuSelect()
	{
	}
	virtual void	Receive( void );
	virtual bool	Handle( void );

	SERIAL			ButtonID( void ) const;
	SERIAL			GumpID( void ) const;
	SERIAL			ID( void ) const;
	UI32			SwitchCount( void ) const;
	UI32			TextCount( void ) const;
	UI32			SwitchValue( UI32 index ) const;

	UI16			GetTextID( UI08 number ) const;
	UI16			GetTextLength( UI08 number ) const;
	std::string		GetTextString( UI08 number ) const;
	std::string		GetTextUString( UI08 number ) const;

protected:
	SERIAL				id, buttonID, gumpID;
	UI32				switchCount, textCount;
	size_t				textOffset;

	std::vector< UI16 >	textLocationOffsets;

	void				BuildTextLocations( void );
};


class CPITalkRequest : public CPInputBuffer
{
protected:
	COLOUR				textColour;
	UI16				strLen;
	UI16				fontUsed;
	char				txtSaid[4096];
	SpeechType			typeUsed;
	bool				isUnicode;
	virtual void		InternalReset( void );
	bool				HandleCommon( void );
public:
						CPITalkRequest();
						CPITalkRequest( CSocket *s );
	virtual				~CPITalkRequest()
	{
	}
	virtual void		Receive( void ) = 0;
	virtual bool		Handle( void );

	COLOUR				TextColour( void )	const;
	UI16				Length( void )		const;
	UI16				Font( void )		const;
	SpeechType			Type( void )		const;
	bool				IsUnicode( void )	const;
	virtual std::string	TextString( void )	const;
	virtual char *		Text( void )		const;

};

class CPITalkRequestAscii : public CPITalkRequest
{
public:
						CPITalkRequestAscii();
						CPITalkRequestAscii( CSocket *s );
	virtual				~CPITalkRequestAscii()
	{
	}
	virtual void		Receive( void );
};

class CPITalkRequestUnicode : public CPITalkRequest
{
protected:
	char unicodeTxt[8192];
	char langCode[4];
public:
						CPITalkRequestUnicode();
						CPITalkRequestUnicode( CSocket *s );
	virtual				~CPITalkRequestUnicode()
	{
	}
	virtual void		Receive( void );

	virtual std::string	UnicodeTextString( void )	const;
	virtual char *		UnicodeText( void )			const;
	char *				Language( void )			const;

};

class CPIAllNames3D : public CPInputBuffer
{
public:
	virtual			~CPIAllNames3D()
	{
	}
					CPIAllNames3D();
					CPIAllNames3D( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
};

class CPIGumpChoice : public CPInputBuffer
{
public:
	virtual			~CPIGumpChoice()
	{
	}
					CPIGumpChoice();
					CPIGumpChoice( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
};

class CPIBuyItem : public CPInputBuffer
{
public:
	virtual			~CPIBuyItem()
	{
	}
					CPIBuyItem();
					CPIBuyItem( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
};

class CPISellItem : public CPInputBuffer
{
public:
	virtual			~CPISellItem()
	{
	}
					CPISellItem();
					CPISellItem( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
};

class CPIDeleteCharacter : public CPInputBuffer
{
public:
	virtual			~CPIDeleteCharacter()
	{
	}
					CPIDeleteCharacter();
					CPIDeleteCharacter( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
};

class CPICreateCharacter : public CPInputBuffer
{
protected:
	UI32	pattern1;
	UI32	pattern2;
	UI08	pattern3;
	char	charName[30];
	char	password[30];
	UI08	sex;
	UI08	str;
	UI08	dex;
	UI08	intel;
	UI08	skill[3];
	UI08	skillValue[3];
	COLOUR	skinColour;
	UI16	hairStyle;
	COLOUR	hairColour;
	UI16	facialHair;
	COLOUR	facialHairColour;
	UI16	locationNumber;
	UI16	unknown;
	UI16	slot;
	UI32	ipAddress;
	COLOUR	shirtColour;
	COLOUR	pantsColour;

	void	newbieItems( CChar *mChar );
public:
	virtual			~CPICreateCharacter()
	{
	}
					CPICreateCharacter();
					CPICreateCharacter( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
	virtual void	Log( std::ofstream &outStream, bool fullHeader = true );
};

class CPIPlayCharacter : public CPInputBuffer
{
protected:
	UI32			pattern;
	char			charName[30];
	char			unknown[33];
	UI08			slotChosen;
	UI32			ipAddress;
public:
	virtual			~CPIPlayCharacter()
	{
	}
					CPIPlayCharacter();
					CPIPlayCharacter( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
	virtual void	Log( std::ofstream &outStream, bool fullHeader = true );
};

class CPIGumpInput : public CPInputBuffer
{
protected:
	UI32			id;
	UI08			type;
	UI08			index;
	UI08			unk[3];
	UString			reply;

	void			HandleTweakItemText( UI08 index );
	void			HandleTweakCharText( UI08 index );
	void			HandleTownstoneText( UI08 index );
public:
	virtual			~CPIGumpInput()
	{
	}
					CPIGumpInput();
					CPIGumpInput( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );

	UI32				ID( void ) const;
	UI08				Type( void ) const;
	UI08				Index( void ) const;
	UI08				Unk( int offset ) const;
	const UString		Reply( void ) const;
	virtual void		Log( std::ofstream &outStream, bool fullHeader = true );
};

class CPIHelpRequest : public CPInputBuffer
{
protected:
	UI16			menuNum;
public:
	virtual			~CPIHelpRequest()
	{
	}
					CPIHelpRequest( CSocket *s, UI16 menuVal );
					CPIHelpRequest( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
};

class CPITradeMessage : public CPInputBuffer
{
public:
	virtual			~CPITradeMessage()
	{
	}
					CPITradeMessage();
					CPITradeMessage( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
};

class CPIDyeWindow : public CPInputBuffer
{
protected:
	SERIAL			changing;
	COLOUR			newValue;
	UI16			modelID;
public:
	virtual			~CPIDyeWindow()
	{
	}
					CPIDyeWindow();
					CPIDyeWindow( CSocket *s );
	virtual void	Receive( void );
	virtual bool	Handle( void );
	virtual void	Log( std::ofstream &outStream, bool fullHeader = true );
};

class CPIMsgBoardEvent : public CPInputBuffer
{
public:
	virtual			~CPIMsgBoardEvent()
	{
	}
					CPIMsgBoardEvent();
					CPIMsgBoardEvent( CSocket *s );
					CPIMsgBoardEvent( CSocket *s, bool receive );

	virtual void	Receive( void );
	virtual bool	Handle( void );
};

}

#endif

