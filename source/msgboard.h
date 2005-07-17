#ifndef __MSGBOARD_H__
#define __MSGBOARD_H__

namespace UOX
{

// msgboard.h
//

// Maximum number of posts per board
// BUFFER SIZE = 2560 
// Therefore 0x3c max size = 2560 - 5 ( 0x3c header info ) 
//                         = 2550 
//
//                           2550 / 19 ( item segment size per msg ) 
//                         = 134 
// Round down to 128 Messages allowable on a message board (better safe than sorry)

// Maximum number of entries in a ESCORTS list in the MSGBOARD.SCP file
#define MAXENTRIES      256

// Different types of user posts
enum PostTypes
{
	LOCALPOST =		0,
	REGIONALPOST,
	GLOBALPOST
};

// Different types of QUESTS (nQuestType)
// Added for Quests (ESCORTS)
// Reason for starting high and counting down, is that I store the entire user
// posted message as is (which includes the message type).  The message type
// for a user post is 0x05 and I also use this field to determine whether the
// post is marked for deletion (0x00).  In order to allow for the maximum number
// of different quest types, I opted to start high and count down.
const UI08 ESCORTQUEST = 0xFF;
const UI08 BOUNTYQUEST = 0xFE;
const UI08 ITEMQUEST = 0xFD;


// Function Prototypes 
void    MsgBoardEvent( CSocket *s );
void    MsgBoardSetPostType( CSocket *s, UI08 nPostType );
void    MsgBoardGetPostType( CSocket *s );
void    MsgBoardQuestEscortCreate( CChar *npcIndex );
void    MsgBoardQuestEscortArrive( CChar *npcIndex, CSocket *mSock );
void    MsgBoardQuestEscortDelete( CChar *npcIndex );
void    MsgBoardQuestEscortRemovePost( CChar *npcIndex );
void    MsgBoardMaintenance( void );

}

#endif
