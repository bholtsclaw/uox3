#include "uox3.h"
#include "msgboard.h"
#include "townregion.h"
#include "cServerDefinitions.h"
#include "ssection.h"
#include "cEffects.h"
#include "CPacketSend.h"
#include "classes.h"
#include "Dictionary.h"

namespace UOX
{

//////////////////////////////////////////////////////////////////////////////
// LOCAL - GLOBAL DATA
//////////////////////////////////////////////////////////////////////////////

// Char array for messages to client. Message body (when entering body of post)
// can hold a maximum of 1975 chars (approx)
UI08 msg[MAXBUFFER];         

// Buffer to be used when posting messages
//                                   |Pid|sz1|sz2|mTy|b1 |b2 |b3 |b4 |m1 |m2 |m3 |m4 |
UI08 msg2Post[MAXBUFFER] = "\x71\xFF\xFF\x05\x40\x00\x00\x19\x00\x00\x00\x00";


//////////////////////////////////////////////////////////////////////////////
// FUNCTION:    MsgBoardGetPostType( int s )
//
// PURPOSE:     Used to retrieve the current post type in order to tell the
//              user what type of mode they are in.
// 
// PARAMETERS:  s           Player serial number
//
// RETURNS:     void
//////////////////////////////////////////////////////////////////////////////
void MsgBoardGetPostType( CSocket *s )
{
	CChar *mChar = s->CurrcharObj();
	if( !ValidateObject( mChar ) )
		return;

	switch( mChar->GetPostType() )
	{
		case LOCALPOST:
			// Feed back to operator that post type has been changed to LOCAL
			s->sysmessage( 722 );
			break;
		case REGIONALPOST:
			// Feed back to operator that post type has been changed to REGIONAL
			s->sysmessage( 723 );
			break;
		case GLOBALPOST:
			// Feed back to operator that post type has been changed to GLOBAL
			s->sysmessage( 724 );
			break;
		default:
			// Feed back to operator that post type has been changed to LOCAL
			s->sysmessage( 725 );
			break;
	}
}

//////////////////////////////////////////////////////////////////////////////
// FUNCTION:    MsgBoardSetPostType( int s, int Type )
//
// PURPOSE:     Used to set the postType for the current user (Typically a GM)
//              There is a local array that holds each players curreny posting
//              type.  Using the command to set the post type updates the
//              value in the array for that player so that they can post 
//              different types of messages.  Array can hold any of the
//              following:
// 
//              Type = 0 = LOCAL
//                     1 = REGIONAL
//                     2 = GLOBAL
//
// PARAMETERS:  s           Player serial number
//              Type        Type of post
//
// RETURNS:     void
//////////////////////////////////////////////////////////////////////////////
void MsgBoardSetPostType( CSocket *s, int Type )
{
	CChar *mChar = s->CurrcharObj();
	if( !ValidateObject( mChar ) )
		return;

	mChar->SetPostType( Type );
	
	switch ( Type )
	{
		case LOCALPOST:
			// Feed back to operator that post type has been changed to LOCAL
			s->sysmessage( 726 );
			break;
		case REGIONALPOST:
			// Feed back to operator that post type has been changed to REGIONAL
			s->sysmessage( 727 );
			break;
		case GLOBALPOST:
			// Feed back to operator that post type has been changed to GLOBAL
			s->sysmessage( 728 );
			break;
		default:
			// Feed back to operator that post type has been changed to LOCAL
			s->sysmessage( 725 );
			break;
	}
}

//////////////////////////////////////////////////////////////////////////////
// FUNCTION:    MsgBoardOpen( int s )
//
// PURPOSE:     Called when player dbl-clicks on a Message Board thereby
//              requesting a list of messages posted on the board.
//
// PARAMETERS:  s           Player serial number
//
// RETURNS:     void
//////////////////////////////////////////////////////////////////////////////
void MsgBoardOpen( CSocket *s )
{
	// In Response to a doubleclick() message from a client
	// [SEND:5] 06 40 07 ba 3d 
	// Message to client     |Pak|sz1|sz2|mTy|sn1|sn2|sn3|sn4| b | u | l | l | e | t | i | n |   | b | o | a | r | d |<------------  Unknown, don't care right now ----------------->|                                                     
	char msgBoardHeader[] = "\x71\x00\x26\x00\xFF\xFF\xFF\xFF\x62\x75\x6c\x6c\x65\x74\x69\x6e\x20\x62\x6f\x61\x72\x64\x00\x00\x00\x00\x00\x00\x00\x00\x40\x20\x00\xff\x00\x00\x00\x00";
	
	// Extract the Bulletin Board serial number from client buffer and update msgBoardHeader
	msgBoardHeader[4]     = s->GetByte( 1 ); // From doubleclick() buffer[s]
	msgBoardHeader[5]     = s->GetByte( 2 );
	msgBoardHeader[6]     = s->GetByte( 3 );
	msgBoardHeader[7]     = s->GetByte( 4 );
	
	// Can place up to 20 customizable chars in Message Board header to give Message Board a unique name
	// Might be able to do more, but why, it usually overruns the area designated for the name anyway
	CItem *msgBoard = calcItemObjFromSer( s->GetDWord( 1 ) );
	
	// If the name the item (Bulletin Board) has been defined, display it
	// instead of the default "Bulletin Board" title.
	if( msgBoard->GetName() != "#" )
		strncpy( &msgBoardHeader[8], msgBoard->GetName().c_str(), 20);
	
	// Send Message Board header to client
	s->Send( msgBoardHeader, (sizeof(msgBoardHeader)-1) );
	
	
	// Send draw item message to client with required info to draw the message board
	// Base size plus however many messages are in the list
	// Example:
	// s1 s2 s3 s4 m# m# ?? st st xx xx yy yy b1 b2 b3 b4 c1 c2
	// 40 1c 53 eb 0e b0 00 00 00 00 3a 00 3a 40 07 ba 3d 00 00    Size = 19 (min size for msg = 24 bytes)
	
	// Standard header :        |Pak|sz1|sz2|sg1|sg2|<--------------- See above ------------------->                            |
	//                         "\x3c\x00\x18\x00\x01\x40\x1c\x53\xeb\x0e\xb0\x00\x00\x00\x00\x3a\x00\x3a\x40\x00\x00\x19\x00\x00";
	
	// Read bbi file to determine messages on boards list
	// Get Message Board serial number from message buffer
	FILE *file = NULL;
	// 50 chars for prefix and 4 for the extension plus the ending NULL
	UString fileName;
	
	msg[0] = 0x3c;  // Packet type (Items in Container)
	msg[1] = 0x00;  // High byte of packet size
	msg[2] = 0x00;  // Low byte of packet size
	msg[3] = 0x00;  // High byte of number of items
	msg[4] = 0x00;  // Low byte of number of items
	
	int offset = 5; // Offset to next msg[] value
	int count  = 0; // Number of messages (times through while loop)
	
	SI32 currentFile = 1;  // Starting file to open and iterate through (1=GLOBAL.bbp, 2=REGIONAL.bbp, 3=LOCAL.bbp)
	
	// Determine what type of message this is in order to determine which file to open
	// GLOBAL   Posts start at 01 00 00 00 -> 01 FF FF FF
	// REGIONAL Posts start at 02 00 00 00 -> 02 FF FF FF
	// LOCAL    Posts start at 03 00 00 00 -> 03 FF FF FF
	
	// GLOBAL post file
	UString fileName1 = "global.bbi";
	
	// REGIONAL post file
	CTownRegion *mbRegion = calcRegionFromXY( msgBoard->GetX(), msgBoard->GetY(), msgBoard->WorldNumber() );
	UString fileName2 = "region" + UString::number( mbRegion->GetRegionNum() ) + ".bbi";
	
	// LOCAL post file
	UString fileName3 = UString::number( s->GetByte( 1 ), 16 ) + UString::number( s->GetByte( 2 ), 16 ) + UString::number( s->GetByte( 3 ), 16 ) + UString::number( s->GetByte( 4 ), 16 ) + ".bbi";
	
	while( currentFile <= 3 )
	{
		// If a MSBBOARDPATH has been define in the SERVER.SCP file, then use it
		if( !cwmWorldState->ServerData()->Directory( CSDDP_MSGBOARD ).empty() )
			fileName = cwmWorldState->ServerData()->Directory( CSDDP_MSGBOARD );
		
		// Open the next file to process
		switch ( currentFile )
		{
			case 1:
				// Start with the GLOBAL.bbp file first
				//s->sysmessage( "Opening GLOBAL.bbi messages" );
				fileName	+= fileName1;
				file		= fopen( fileName.c_str(), "rb" );
				break;
			case 2:
				// Set fileName to REGIONAL.bbi
				//s->sysmessage( "Opening REGIONAL.bbi messages" );
				fileName	+= fileName2;
				file		= fopen( fileName.c_str(), "rb" );
				break;
			case 3:
				// Set fileName to LOCAL.bbi
				//s->sysmessage( "Opening LOCAL.bbi messages" );
				fileName	+= fileName3;
				file		= fopen( fileName.c_str(), "rb" );
				break;
			default:
				Console << "MsgBoardOpen() Unhandle case value: " << currentFile << myendl;
				return;
		}
		
		// If the file doesn't exist, increment the currenFile count and move onto the next file
		if( file != NULL )
		{
			// Ignore first 4 bytes of bbi file as this is reserverd for the current max message serial number being used
			
			if( fseek( file, 4, SEEK_SET ) )
			{
				Console << "MsgBoardOpen() failed to seek to first message segment in bbi file" << myendl;
				fclose(file);
				return;
			}
			
			// Loop until we have reached the end of the file or the maximum number of posts allowed
			// to be displayed
			while( (!feof(file)) && (count<=MAXPOSTS) )
			{
				// Fill up the msg with data from the bbi file
				if( fread( &msg[offset], sizeof(char), 19, file ) != 19 )
				{
					if( feof(file) )
						break;
				}
				//  |Off| 1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16  17  18
				//  |mg1|mg2|mg3|mg4|mo1|mo2|???|sg1|sg2|xx1|xx2|yy1|yy2|bn1|bn2|bn3|bn4|co1|co2|
				// "\x40\x1c\x53\xeb\x0e\xb0\x00\x00\x00\x00\x3a\x00\x3a\x40\x00\x00\x19\x00\x00";
				
				// If the segment 6 is 0x00 then the message is marked for deletion so skip it 
				if( msg[offset+6] ) 
				{
					// Set the Board SN fields the proper value for the board clicked on
					msg[offset+13] = s->GetByte( 1 );
					msg[offset+14] = s->GetByte( 2 );
					msg[offset+15] = s->GetByte( 3 );
					msg[offset+16] = s->GetByte( 4 );
					
					// Store message ID into array for later acknowledgement
					s->PostAcked( count, 0, msg[offset] );
					s->PostAcked( count, 1, msg[offset+1] );
					s->PostAcked( count, 2, msg[offset+2] );
					s->PostAcked( count, 3, msg[offset+3] );
					
					// Increment the offset by 19 bytes for next message index
					offset += 19;

					++count;
				}
			}
		}

		if( file )
			fclose( file );

		++currentFile;
	}

	if( file )
		fclose( file );
	
	// Update size fields of message with new values
	msg[1] = (UI08)(offset>>8);
	msg[2] = (UI08)(offset%256);
	msg[3] = (UI08)(count>>8);
	msg[4] = (UI08)(count%256);
	
	// Set global variable that holds the count of the number of posts being sent 
	// to this particular client
	s->PostCount( count );
	
	// Set the postAckCount to zero in preparation of the client ACKing the message
	// about to be sent
	s->PostAckCount( 0 );
	
	// Send Draw Item message to client
	s->Send( msg, offset );
#if defined( _MSC_VER )
#pragma note( "Flush location" )
#endif
	s->FlushBuffer();
}

//////////////////////////////////////////////////////////////////////////////
// FUNCTION:    MsgBoardList( int s )
//
// PURPOSE:     After Bulletin Board is displayed and client ACK's all posted
//              items this is called to send the details of the posted item to
//              the Bulletin Board so that it can be listed on the board.
//
// PARAMETERS:  s           Player serial number
//
// RETURNS:     void
//////////////////////////////////////////////////////////////////////////////
void MsgBoardList( CSocket *s )
{
	// READ IN bbp FILE (for list on message board)
	
	// Client sends:
	// Example  [SEND:12] 71 00 0c 04 40 07 ba 3d 40 1c 53 eb 
	//
	// Server responds with:
	// Sample Response format for message board message list item
	//                                                         |sA|au|sS| Subject                                                                                                                                                |sD| Date                                    |
	//[RECV:85] 71 00 55 01 40 07 ba 3d 40 1c 53 eb 00 00 00 00 01 00 33 46 6f 58 20 74 68 65 20 44 65 6d 65 6e 74 65 64 3a 20 20 34 30 30 30 67 6f 6c 64 2e 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 00 0e 44 61 79 20 33 20 40 20 32 33 3a 32 30 00 
	
	// 50 chars for prefix and 4 for the extension plus the ending NULL
	FILE         *file = NULL;
	
	int			msgBytes     = 0;  // Number of bytes to send to client (message size)
	int			msgOffset    = 0;  // Total number of bytes between messages from start of file
	UI32		segmentSize  = 0;  // Size of a segment (Author, Subject, Date)
	int			foundMsg     = 0;  // Flag when message has been found
	SI32		currentFile  = 1;  // Starting file to open and iterate through (1=GLOBAL.bbp, 2=REGIONAL.bbp, 3=LOCAL.bbp)
	int			w            = 0;  // Counter
	SI32		x            = 0;  // Counter
	
	// Determine what type of message this is in order to determine which file to open
	// GLOBAL   Posts start at 01 00 00 00 -> 01 FF FF FF
	// REGIONAL Posts start at 02 00 00 00 -> 02 FF FF FF
	// LOCAL    Posts start at 03 00 00 00 -> 03 FF FF FF
	
	// Determine the Bulletin Boards serial number
	CItem *msgBoard = calcItemObjFromSer( s->GetDWord( 4 ) );
	
	UString fileName;
	// GLOBAL post file
	UString fileName1 = "global.bbp";

	// REGIONAL post file
	CTownRegion *mbRegion	= calcRegionFromXY( msgBoard->GetX(), msgBoard->GetY(), msgBoard->WorldNumber() );
	UString fileName2		= "region" + UString::number( mbRegion->GetRegionNum() ) + ".bbp";
	
	// LOCAL post file
	UString fileName3 = UString::number( s->GetByte( 4 ), 16 ) + UString::number( s->GetByte( 5 ), 16 ) + UString::number( s->GetByte( 6 ), 16 ) + UString::number( s->GetByte( 7 ), 16 ) + ".bbp";
	
	while( currentFile <= 3 )
	{
		// If a MSBBOARDPATH has been define in the SERVER.SCP file, then use it
		if( !cwmWorldState->ServerData()->Directory( CSDDP_MSGBOARD ).empty() )
			fileName = cwmWorldState->ServerData()->Directory( CSDDP_MSGBOARD );
		
		// Open the next file to process
		switch( currentFile )
		{
			case 1:
				// Start with the GLOBAL.bbp file first
				//s->sysmessage( "Opening GLOBAL.bbp messages" );
				fileName	+= fileName1;
				file		= fopen( fileName.c_str(), "rb");
				break;
			case 2:
				// Set fileName to REGIONAL.bbi
				//s->sysmessage( "Opening REGIONAL.bbp messages" );
				fileName	+= fileName2;
				file		= fopen( fileName.c_str(), "rb" );
				break;
			case 3:
				// Set fileName to LOCAL.bbi
				//s->sysmessage( "Opening LOCAL.bbp messages" );
				fileName	+= fileName3;
				file		= fopen( fileName.c_str(), "rb" );
				break;
			default:
				Console << "MsgBoardOpen() Unhandle case value: " << currentFile << myendl;
				return;
		}
		msgOffset = 0;
		
		// If the file doesn't exist, increment the currenFile count and move onto the next file
		if( file != NULL )
		{
			while( w < s->PostCount() )
			{
				foundMsg = 0;
				
				// Find Message ID that has been requested
				while( !foundMsg ) 
				{
					x = 0;
					//                                0       1     2      3      4   5   6      7   8    9    10   11   12   13   14   15
					// Read In the first 12 bytes |PacketID|Size1|Size2|MsgType|bSn1|bSn2|bSn3|bSn4|mSn1|mSn2|mSn3|mSn4|pmSN|pmSN|pmSN|pmSN|
					fread( &msg[x], sizeof(char), 16, file );
					msgBytes = 16;
					
					// If we have reached the EOF then stop searching
					if( feof( file ) ) 
						break;
					
					// Check buffered message SN with currently read message SN
					if( ( msg[8]  == s->PostAcked( w, 0 ) ) &&
						( msg[9]  == s->PostAcked( w, 1 ) ) &&
						( msg[10] == s->PostAcked( w, 2 ) ) &&
						( msg[11] == s->PostAcked( w, 3 ) ) )
					{
						// Don't forget to set the flag to stop searching for the message when we find it
						foundMsg = 1;
						
						// Increment While loop counter tracking the number of posts we have replied too
						++w;
						
						// Set the board SN values to the board that was just double-clicked on
						msg[4] = s->GetByte( 4 );
						msg[5] = s->GetByte( 5 );
						msg[6] = s->GetByte( 6 );
						msg[7] = s->GetByte( 7 );
						
						// Read in  author, subject and date info to pass back to client (DO NOT SEND BODY of msg)
						// Count the total number of bytes in posting (not including body as it isn't sent to client)
						
						// Author, Subject, and Date segments can all be retrieved in the same fashion
						// | size | data........................... 0x00 |
						//
						// size equals DATA + NULL
						// Segments:
						//             0 = Author
						//             1 = Subject
						//             2 = Date
						for( x = 0; x <= 2; ++x )
						{
							// Get the size of this segment and store it in the message
							msg[msgBytes] = fgetc( file );
							
							// Put the size into a variable
							segmentSize = msg[msgBytes];
							++msgBytes;
							
							// Read in the number of bytes give by the segment size
							if( segmentSize != fread( &msg[msgBytes], sizeof( char ), segmentSize, file ) )
							{
								// If we are unable to read in the number of bytes specified by the segmentSize, ABORT!
								Console << "MsgBoardList() couldn't read in entire segment(" << x << ")" << myendl;
								fclose( file );
								return;
							}
							
							// Increase msgBytes to the new size
							msgBytes += segmentSize;
						}
						
						msgOffset += (msg[1]<<8) + msg[2];
						
						// Jump to next message
						if( fseek(file, msgOffset, SEEK_SET) )
							Console << "MsgBoardEvent() case 4 : failed to seek start of next message" << myendl;
						
						// Calculate new message size
						msg[1] = (UI08)(msgBytes>>8);
						msg[2] = (UI08)(msgBytes%256);
						
						// Set packet 0x71 message type to 0x01 (send post item to message board list)
						msg[3] = 1;
						
						// Send message to client
						s->Send( msg, msgBytes );
#if defined( _MSC_VER )
#pragma note( "Flush location" )
#endif
						s->FlushBuffer();
					}
					else // If this isn't the message were looking for, jump ahead to next message
					{
						// Since we didn't find the message in this pass, get this messages size and jump ahead
						msgOffset += (msg[1]<<8) + msg[2];
						
						// Jump to next message
						if( fseek(file, msgOffset, SEEK_SET) )
						{
							Console << "MsgBoardEvent() case 4 : failed to seek next message" << myendl;
							break;
						}
					}
				}
				
				// If we broke out of the loop  because EOF was reached then break out again
				if( feof( file ) )
				{
					fclose(file);
					break;
				}
			}
		}
		if( file )
			fclose( file );
		++currentFile;
	}
	if( file )
		fclose( file );
}


//////////////////////////////////////////////////////////////////////////////
// FUNCTION:    MsgBoardGetMaxMsgSN( int msgType, int autoPost=0 )
//
// PURPOSE:     Used during posting operation to determine the current maximum
//              message serial number used on the board.  It then increments
//              the number and updates the bbi file.  The new message serial
//              number is returned.
//
// PARAMETERS:  msgType     Type of post (LOCAL, REGIONAL, GLOBAL)
//              autoPost    0 = message posted by a user (default)
//                          1 = message posted by system
//
// RETURNS:     int         0 = Failed to get maximum serial number 
//                          1 = Found and updated maximum serial number
//////////////////////////////////////////////////////////////////////////////
int MsgBoardGetMaxMsgSN( int msgType, int autoPost = 0 )
{
	FILE        *pFile            = NULL;
	
	// 50 chars for prefix and 4 for the extension plus the ending NULL
	char        msgbbiSegment[20] = "\x00\x00\x00\x00\x0e\x0b\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
	char        maxMsgSN[5]       = "";

	int         maxSN             = 0;
	
	struct tm   timeOfPost;
	time_t      now;
	// If a MSBBOARDPATH has been define in the SERVER.SCP file, then use it
	UString fileName;
	if( !cwmWorldState->ServerData()->Directory( CSDDP_MSGBOARD ).empty() )
		fileName = cwmWorldState->ServerData()->Directory( CSDDP_MSGBOARD );

	char		temp[1024];
	switch ( msgType )
	{
		case LOCALPOST:
			// Get Message Board serial number from message buffer
			sprintf( temp, "%02x%02x%02x%02x.bbi", msg2Post[4], msg2Post[5], msg2Post[6], msg2Post[7]);
			break;
		case REGIONALPOST:
			// set the Message Board fileName to the proper region number
			if( autoPost )
				sprintf( temp, "region%d.bbi", calcCharObjFromSer( calcserial(msg2Post[4], msg2Post[5], msg2Post[6], msg2Post[7]) )->GetRegionNum() );
			else
			{
				CItem *msgBoard = calcItemObjFromSer( calcserial( msg2Post[4], msg2Post[5], msg2Post[6], msg2Post[7] ) );
				CTownRegion *mbRegion = calcRegionFromXY( msgBoard->GetX(), msgBoard->GetY(), msgBoard->WorldNumber() );
				sprintf( temp, "region%d.bbi", mbRegion->GetRegionNum() );
			}
			break;
		case GLOBALPOST:
			sprintf( temp, "global.bbi" );
			break;
		default:
			Console << "MsgBoardGetMaxMsgSN() Invalid post type, aborting post" << myendl;
			return 0;
	}
	
	// Append file name to path
	fileName += temp;
	
	// Get the current maximum message s/n from the bbi file
	pFile = fopen( fileName.c_str(), "rb" );
	if( pFile == NULL )
	{
		Console << "MsgBoardGetMaxMsgSN() bbi not found. Creating file " << fileName << myendl;
		maxSN = 0;
	}
	else // bbi file exists so read in first 4 bytes to get current maximum serial number
	{
		// Get the first 4 bytes from each message index segment in the bbi file
		if( fread(maxMsgSN, sizeof(char), 4, pFile) != 4 )
		{
			Console << "MsgBoardGetMaxMsgSN() Could not get MaxSN from " << fileName << myendl;
			
			fclose( pFile );
			return 0;
		}
		
		// Calculate the maxSN in decimal
		maxSN = (maxMsgSN[0]<<24) + (maxMsgSN[1]<<16) + (maxMsgSN[2]<<8) + maxMsgSN[3];
		++maxSN;
	}

	if( pFile )
		fclose( pFile );
	
	// If the maxSN == 0 then the file does not exist yet so create the file
	if( maxSN == 0 )
	{
		pFile = fopen( fileName.c_str(), "ab+" );
		
		if( pFile == NULL )
		{
			Console << "MsgBoardGetMaxMsgSN() Error creating bbi file" << fileName << ", aborting post" << myendl;
			return 0;
		}
		else
		{
			// GLOBAL   Posts start at 01 00 00 00 -> 01 FF FF FF
			// REGIONAL Posts start at 02 00 00 00 -> 02 FF FF FF
			// LOCAL    Posts start at 03 00 00 00 -> 03 FF FF FF
			switch ( msgType )
			{
				case LOCALPOST:
					// Write out the serial number as 4 bytes
					// Write 03 00 00 00 to bbi file (can't start at 00 00 00 00 because client crashes if this is true)
					if( fwrite("\x03\x00\x00\x00", sizeof( char ), 4, pFile ) != 4 )
					{
						Console.Error( 1, "MsgBoardGetMaxMsgSN() Error writing to bbi file, aborting post" );
						fclose( pFile );
						return 0;
					}
					// Set maxSN to its startup default
					maxSN = 0x3000000;
					break;
				case REGIONALPOST:
					// Write 02 00 00 00 to bbi file (can't start at 00 00 00 00 because client crashes if this is true)
					if( fwrite( "\x02\x00\x00\x00", sizeof(char), 4, pFile ) != 4 )
					{
						Console.Error( 1, "MsgBoardGetMaxMsgSN() Error writing to bbi file, aborting post" );
						fclose( pFile );
						return 0;
					}
					// Set maxSN to its startup default
					maxSN = 0x2000000;
					break;
				case GLOBALPOST:
					// Write 01 00 00 00 to bbi file (can't start at 00 00 00 00 because client crashes if this is true)
					if( fwrite("\x01\x00\x00\x00", sizeof( char ), 4, pFile ) != 4 )
					{
						Console.Error( 1, "MsgBoardGetMaxMsgSN() Error writing to bbi file, aborting post" );
						fclose( pFile );
						return 0;
					}
					
					// Set maxSN to its startup default
					maxSN = 0x1000000;
					break;
				default:
					Console.Error( 1, "MsgBoardGetMaxMsgSN() Invalid post type, aborting post" );
					fclose( pFile );
					return 0;
			}
		}
	}
	else
	{
		// File must have existed already if maxSN > 0 so open the file as read/write
		pFile = fopen( fileName.c_str(), "rb+" );
		
		if( pFile == NULL )
		{
			Console.Error( 1, "MsgBoardGetMaxMsgSN() Failed to create bbi file %s, aborting post", fileName.c_str() );
			return 0;
		}
		else
		{
			// Set file pointer to BOF
			if( fseek( pFile, 0, SEEK_SET ) )
			{
				Console.Error( 1, "MsgBoardGetMaxMsgSN() Failed to set pFile to BOF in bbi file" );
				fclose( pFile );
				return 0;
			}
			else
			{
				// Convert maxSN to an char array
				maxMsgSN[0] = (UI08)(maxSN>>24);
				maxMsgSN[1] = (UI08)(maxSN>>16);
				maxMsgSN[2] = (UI08)(maxSN>>8);
				maxMsgSN[3] = (UI08)(maxSN%256);
				
				// Write out new maxSN for this post
				if( fwrite( maxMsgSN, sizeof( char ), 4, pFile ) != 4 )
				{
					Console.Error( 1, "MsgBoardGetMaxMsgSN() Error writing to bbi file, aborting post" );
					fclose( pFile );
					return 0;
				}
				
				// Now jump to EOF to write next msgbbiSegment info
				if( fseek( pFile, 0, SEEK_END ) )
				{
					Console.Error( 1, "MsgBoardGetMaxMsgSN() Failed to set pFile to EOF in bbi file");
					fclose( pFile );
					return 0;
				}
			}
		}
	}
	
	// Set bytes to proper values in bbi message array
	msgbbiSegment[0]  = (UI08)(maxSN>>24);  // Message ID 1
	msgbbiSegment[1]  = (UI08)(maxSN>>16);     // Message ID 2
	msgbbiSegment[2]  = (UI08)(maxSN>>8);       // Message ID 3
	msgbbiSegment[3]  = (UI08)(maxSN%256);       // Message ID 4
	msgbbiSegment[6]  = msg2Post[3];     // 05 = user posted message, 0xFF and lower  is a quest post (0xFF is escort quest)
	
	// Calculate current time and date ( for later bulletin board maintenance routine )
	time( &now );
	timeOfPost = *localtime( &now );
	
	msgbbiSegment[7]  = (timeOfPost.tm_yday+1)/256;
	msgbbiSegment[8]  = (timeOfPost.tm_yday+1)%256;
	
	// If this is an autoPost then set the CHAR or ITEM serial number in order to mark it for deletion
	// after the quest is done.
	if( autoPost )
	{
		msgbbiSegment[13]  = msg2Post[4];  // Char or Item SN1
		msgbbiSegment[14]  = msg2Post[5];  // Char or Item SN2
		msgbbiSegment[15]  = msg2Post[6];  // Char or Item SN3
		msgbbiSegment[16]  = msg2Post[7];  // Char or Item SN4
	}
	
	// GLOBAL   Posts start at 01 00 00 00 -> 01 FF FF FF
	// REGIONAL Posts start at 02 00 00 00 -> 02 FF FF FF
	// LOCAL    Posts start at 03 00 00 00 -> 03 FF FF FF
	switch ( msgType )
	{
		case LOCALPOST:
			// Check to see if Maximum number of posts have been reached
			//if( ((maxSN-0x03000000) >= MAXPOSTS) || (maxSN >= 0xFFFFFFFF) )
			if( maxSN >= 0x03FFFFFF )
			{
				Console.Error( 3, "MsgBoardGetMaxMsgSN() Max posts reached in %s", fileName.c_str() );
				fclose( pFile );
				return 0;
			}
			break;
		case REGIONALPOST:
			// Check to see if Maximum number of posts have been reached
			//if( ((maxSN-0x02000000) >= MAXPOSTS) || (maxSN >= 0x02FFFFFF) )
			if( maxSN >= 0x02FFFFFF )
			{
				Console.Error( 3, "MsgBoardGetMaxMsgSN() Max posts reached in %s", fileName.c_str() );
				fclose( pFile );
				return 0;
			}
			break;
		case GLOBALPOST:
			// Check to see if Maximum number of posts have been reached
			//if( ((maxSN-0x01000000) >= MAXPOSTS) || (maxSN >= 0x01FFFFFF) )
			if( maxSN >= 0x01FFFFFF )
			{
				Console.Error( 3, "MsgBoardGetMaxMsgSN() Max posts reached in %s", fileName.c_str() );
				fclose( pFile );
				return 0;
			}
			break;
		default:
			Console.Error( 3, "MsgBoardGetMaxMsgSN() Invalid post type, aborting post" );
			fclose( pFile );
			return 0;
	}
	
	// Write out bbi message array to file
	if( fwrite(msgbbiSegment, sizeof(char), (sizeof(msgbbiSegment)-1), pFile) != (sizeof(msgbbiSegment)-1) )
	{
		Console.Error( 1, "MsgBoardGetMaxMsgSN() Error writing to bbi file, aborting post" );
		fclose( pFile );
		return 0;
	}

	fclose( pFile );

	return maxSN;
}

//////////////////////////////////////////////////////////////////////////////
// FUNCTION:    MsgBoardPost( int s, int msgType, int autoPost )
//
// PURPOSE:     Called when a user clicks on the Post button after typing in a
//              message or the server is creating a quest and wants to post
//              a message to a regional board
//              if message posted by a user, then autoPost = 0
//              if message posted by system, then autoPost = 1

//
// PARAMETERS:  s           Players/NPC serial number
//              msgType     Type of post (LOCAL, REGIONAL, GLOBAL)
//              autoPost    0 = message posted by a user
//                          1 = message posted by system
//
// RETURNS:     int         0 = Failed to post message
//                          1 = Post successful
//////////////////////////////////////////////////////////////////////////////
bool MsgBoardPost( CSocket *s, int msgType, int autoPost )
{
	// WRITE FILE OUT (POST MESSAGE)
	
	// 50 chars for prefix and 4 for the extension plus the ending NULL
	FILE        *pFile = NULL;

	int         newMsgSize            = 0;
	int         newMsgSN              = 0;
	int         maxMsgSN              = 0;
	int         isReply               = 0;
	int         x, y, z, pos, offset;
	
	char        msgHeader[17]         = "";
	char        msgSubject[257]       = "";
	char        msgBody[MAXBUFFER]    = "";
	char        msgAuthor[52]         = "";   // Maximum name size from char_st (Size + Name)
	char        msgDate[17]           = "";   // Maximum date size based on Size + "Day ### @ hh:mm" format
	CChar *		mChar				  = s->CurrcharObj();
	
	struct tm   timeOfPost;
	time_t      now;
	char		temp[1024];
	
	// If this is a users post (done from the client) then copy the client buffer[s] into our buffer
	if( !autoPost )
	{
		memcpy( msg2Post, s->Buffer(), s->GetWord( 1 ) );
		
		// Determine what type of post this is supposed to be and then set the proper file name
		// Also, if this is a reply to a base post, then abort posting if the reply is to a
		// GLOBAL or REGIONAL message.  No one can reply to GLOBAL or REGIONAL messages as they
		// as for informational purposes only (discussions should be taken offline).  There is no
		// reason to reply to a quest post execpt to fill up the message board.
		isReply = s->GetDWord( 8 ); // (msg2Post[8]*16777216) + (msg2Post[9]*65536) + (msg2Post[10]*256) + msg2Post[11];
		
		// If this is a reply to anything other than a LOCAL post, abort
		if( (isReply>0) && (isReply<0x03000000) )
		{
#if defined( UOX_DEBUG_MODE )
			Console << "MsgBoardPost() Attempted reply to a global or regional post" << myendl;
#endif
			s->sysmessage( 729 );
			return false;
		}
	}
	
	// If everything passed the check above then,
	// Determine the new messages serial number for the type of post being done
	maxMsgSN = MsgBoardGetMaxMsgSN( msgType, autoPost );
	
	// If the value returned is zero, then abort the posting
	if( maxMsgSN == 0 )
	{
		Console.Error( 1, "MsgBoardPost() Could not retrieve a valid message serial number" );
		s->sysmessage( 730 );
		return false;
	}
	
	std::string fileName;
	// If a MSBBOARDPATH has been define in the SERVER.SCP file, then use it
	if( !cwmWorldState->ServerData()->Directory( CSDDP_MSGBOARD ).empty() )
		fileName = cwmWorldState->ServerData()->Directory( CSDDP_MSGBOARD );
	
	
	switch( msgType )
	{
		case LOCALPOST:
			// Get Message Board serial number from message buffer
			sprintf( temp, "%02x%02x%02x%02x.bbp", msg2Post[4], msg2Post[5], msg2Post[6], msg2Post[7]);
			break;
		case REGIONALPOST:
			// set the Message Board fileName to the proper region number
			if( autoPost )
				sprintf( temp, "region%d.bbp", calcCharObjFromSer( s->GetDWord( 4 ) )->GetRegionNum() );
			else
			{
				CItem *msgBoard = calcItemObjFromSer( s->GetDWord( 4 ) );
				CTownRegion *mbRegion = calcRegionFromXY( msgBoard->GetX(), msgBoard->GetY(), msgBoard->WorldNumber() );
				sprintf( temp, "region%d.bbp", mbRegion->GetRegionNum() );
			}
			break;
		case GLOBALPOST:
			sprintf( temp, "global.bbp" );
			break;
		default:
			Console.Error( 1, "MsgBoardPost() Invalid post type, aborting post" );
			s->sysmessage( 725 );
			return false;
	}
	
	// Append file name to end of path
	fileName += temp;
	
	// Open the file for appending
	pFile = fopen( fileName.c_str(), "ab+" );
	
	// If we couldn't open the file, send an error message to client
	if( pFile == NULL )
	{
		Console.Error( 1, "MsgBoardPost() Unable to open bbp file %s, aborting post", fileName.c_str() );
		return false;
	}
	
	// Calculate original size of the message that the client sent to us
	UI16 origMsgSize = s->GetWord( 1 );
	
	// Get the messages header info (packet type, size, type, board S/N, parent msg S/N(replies only))
	for( x = 0; x < 12; ++x )
		msgHeader[x] = msg2Post[x];
	
	
	// Set new messages serial number to maxMsgSN from the bbi file
	msgHeader[8]  = (UI08)(maxMsgSN>>24);
	msgHeader[9]  = (UI08)(maxMsgSN>>16);
	msgHeader[10] = (UI08)(maxMsgSN>>8);
	msgHeader[11] = (UI08)(maxMsgSN%256);
	
	// Get the new messages serial number (which is its post position on the board- anything other than 00 00 00 00 
	// (base post) is a reply to a specific message ID )
	newMsgSN = s->GetDWord( 8 ); // (buffer[s][8]<<24) + (buffer[s][9]<<16) + (buffer[s][10]<<8) + buffer[s][11];
	
	// If the newMsgSN is 0 then it is a base post, other wise it is a reply to a previous post
	if( newMsgSN )
	{
		// Create the proper parent message ID segment for the new post
		msgHeader[12] = msg2Post[8];
		msgHeader[13] = msg2Post[9];
		msgHeader[14] = msg2Post[10];
		msgHeader[15] = msg2Post[11];
	}
	else
	{
		// If this isn't a reply post, the parent message SN fields should be 0x00
		msgHeader[12] = 0;
		msgHeader[13] = 0;
		msgHeader[14] = 0;
		msgHeader[15] = 0;
	}
	
	newMsgSize = origMsgSize + 4;  // added 4 bytes for parent msg ID
	
	// This is the position within the msg2Post[] array
	pos = x; // Should equal to 12 (pointing to size of Subject field)
	
	// Get the messages subject info (size, subject)
	y = msg2Post[pos];  // get the size of the subject
	
	for( x = 0; x <= y; ++x )    // Do while we get all bytes
		msgSubject[x] = msg2Post[pos+x];  // get the subject message (size and data)
	
	pos += x;
	
	// Get the messages body info (body section, body size, body data)
	z = msg2Post[pos];     // Total number of NULL's in Body of message
	offset = 0;
	
	// Check if body of post is empty (NULL)
	if( z )
	{
		// Get the FIRST Body segment size + 2 for pre segment size bytes
		y = msg2Post[pos+1] + 2;
		
		// Loop until number of remaining NULLS equal zero
		while( z )  
		{
			// Store Body segment into msgBody ( continue until NULL reached )
			for( x = 0; x < y; ++x )
				msgBody[x+offset] = msg2Post[pos+x];
			
			offset += x;
			pos += x;
			
			// Size of NEXT Body segment
			y = msg2Post[pos] + 1;
			
			// Decrement NULL count (processed one segment of the Body)
			--z;
		}
	}
	else  // Body is empty (NULL) so write out set msgBody[] = "\x00"
	{
		msgBody[0] = msg2Post[pos];
		++offset;
		//++pos;
	}
	
	// Get the Authors info and create msgAuthor packet
	// if this was a user posting, get the characters name, other wise leave it blank
	if( !autoPost )
		strncpy( &msgAuthor[1], mChar->GetName().c_str(), (sizeof(msgAuthor)-1) );
	msgAuthor[0] = strlen(&msgAuthor[1]) + 1;  // get the length of the name + 1 for null
	
	newMsgSize += (msgAuthor[0]+1);   // Update the new total length of the message
	// + 1 is for the byte giving the size of Author segment
	
	// Calculate current time and date
	time( &now );
	timeOfPost = *localtime( &now );
	
	// Create msgDate data packet
	sprintf( &msgDate[1], "Day %i @ %i:%02i\0",	(timeOfPost.tm_yday+1), timeOfPost.tm_hour, timeOfPost.tm_min );

	// get the length of the date + 1 for null
	msgDate[0] = strlen(&msgDate[1]) + 1;       
	
	// Update the new total length of the message (+ 1 is for the byte giving the size of Date segment)
	newMsgSize += (msgDate[0]+1);
	
	// Start writing information out to a file
	// msgHeader + sizeof(msgAuthor) + msgAuthor
	
	// Write out the msgHeader
	msgHeader[1] = (UI08)(newMsgSize>>8);
	msgHeader[2] = (UI08)(newMsgSize%256);
	fwrite( msgHeader, sizeof(char), (sizeof(msgHeader)-1), pFile );
	
	// Write out the msgAuthor
	fwrite( msgAuthor, sizeof(char), (msgAuthor[0]+1), pFile );
	
	// Write out the msgSubject
	fwrite( msgSubject, sizeof(char), (msgSubject[0]+1), pFile );
	
	// Write out the msgDate
	fwrite( msgDate, sizeof(char), (msgDate[0]+1), pFile );
	
	// Write out the msgBody
	fwrite( msgBody, sizeof(char), offset, pFile );
	
	// Close the file
	fclose( pFile );
	
	// if this was a user post, then immediately update the message board with the newly created message
	if( !autoPost )
	{
		CPAddItemToCont toAdd;
		toAdd.Serial( calcserial( msgHeader[8], msgHeader[9], msgHeader[10], msgHeader[11] ) );
		toAdd.Container( calcserial( msgHeader[4], msgHeader[5], msgHeader[6], msgHeader[7] ) );
		
		// Setup buffer to expect to receive an ACK from the client for this posting
		s->PostCount( 1 );
		s->PostAckCount( 0 );
		s->PostAcked( 0, 0, msgHeader[8] );
		s->PostAcked( 0, 1, msgHeader[9] );
		s->PostAcked( 0, 2, msgHeader[10] );
		s->PostAcked( 0, 3, msgHeader[11] );
		
		// Send "Add Item to Container" message to client
		s->Send( &toAdd );
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////
// FUNCTION:    MsgBoardOpenPost( int s )
//
// PURPOSE:     Opens a posting when double-clicked in order to view the
//              full message.
//
// PARAMETERS:  s           Players serial number         
//
// RETURNS:     void
//////////////////////////////////////////////////////////////////////////////
void MsgBoardOpenPost( CSocket *s )
{
	// READ IN bbp FILE  (Client dbl-clicked on posted message on message board list)
	// Get Message Board serial number from message buffer
	
	// 50 chars for prefix and 4 for the extension plus the ending NULL
	std::string fileName;
	FILE *file = NULL;
	
	UI32	msgSN           = 0;
	int		msgBytes        = 0;
	int		dateBytes       = 0;
	int		authorBytes     = 0;
	int		subjectBytes    = 0;
	int		foundMsg        = 0;
	int		x, y, z;
	char	temp[1024];
	
	
	// Calculate the messages SN to determine which file to open
	// GLOBAL   Posts start at 01 00 00 00 -> 01 FF FF FF
	// REGIONAL Posts start at 02 00 00 00 -> 02 FF FF FF
	// LOCAL    Posts start at 03 00 00 00 -> 03 FF FF FF
	
	// If a MSBBOARDPATH has been define in the SERVER.SCP file, then use it
	if( !cwmWorldState->ServerData()->Directory( CSDDP_MSGBOARD ).empty() )
		fileName = cwmWorldState->ServerData()->Directory( CSDDP_MSGBOARD );
	
	
	msgSN = s->GetDWord( 8 );
	
	// Is msgSN within the GLOBAL post range
	if( (msgSN>=0x01000000) && (msgSN<=0x01FFFFFF) )
	{
#if defined( UOX_DEBUG_MODE )
		s->sysmessage( "Opening GLOBAL.bbp posting" );
#endif
		fileName += "global.bbp";
		file = fopen( fileName.c_str(), "rb" );
	}
	// Is msgSN within the REGIONAL post range
	else if( (msgSN>=0x02000000) && (msgSN<=0x02FFFFFF) )
	{
#if defined( UOX_DEBUG_MODE )
		s->sysmessage( "Opening REGIONAL.bbp posting" );
#endif
		CItem *msgBoard = calcItemObjFromSer( s->GetDWord( 4 ) );
		CTownRegion *mbRegion = calcRegionFromXY( msgBoard->GetX(), msgBoard->GetY(), msgBoard->WorldNumber() );
		sprintf( temp, "region%d.bbp", mbRegion->GetRegionNum() );
		fileName	+= temp;
		file		= fopen( fileName.c_str(), "rb" );
	}
	// Is msgSN within the LOCAL post range
	else if( msgSN >= 0x03000000 && msgSN <= 0xFFFFFFFF )
	{
#if defined( UOX_DEBUG_MODE )
		s->sysmessage( "Opening LOCAL.bbp posting" );
#endif
		sprintf( temp, "%02x%02x%02x%02x.bbp", s->GetByte( 4 ), s->GetByte( 5 ), s->GetByte( 6 ), s->GetByte( 7 ) );
		fileName	+= temp;
		file		= fopen( fileName.c_str(), "rb" );
	}
	// This msgSN does not fall within a valid range
	else
	{
		Console.Error( 1, "MsgBoardOpenPost() Invalid message SN: %02x%02x%02x%02x", s->GetByte( 8 ), s->GetByte( 9 ), s->GetByte( 10 ), s->GetByte( 11 ) );
		s->sysmessage( 732 );
		return;
	}
	
	if( file == NULL )
	{
		Console.Error( 1, "Failed to open %s for reading", fileName.c_str() );
		return;   // Put file not found error control here
	}
	if( feof( file ) )
	{
		fclose( file );
		return;     // Put end of file error control here
	}
	
	// Find Message ID that has been requested
	while( !foundMsg ) 
	{
		//                                0       1     2      3      4   5   6      7   8    9    10   11   12   13   14   15
		// Read In the first 12 bytes |PacketID|Size1|Size2|MsgType|bSn1|bSn2|bSn3|bSn4|mSn1|mSn2|mSn3|mSn4|pmSN|pmSN|pmSN|pmSN|
		for( x = 0; x < 12; ++x )
			msg[x] = fgetc( file );
		
		// If we have reached the EOF then stop searching
		if( feof( file ) )
		{
			Console.Error( 1, "MsgBoardEvent() case 3: message not found ");
			break;
		}
		
		// Find post that was ACK'd by client
		if( ( msg[8]  == s->GetByte( 8 ) ) &&
			( msg[9]  == s->GetByte( 9 ) ) &&
			( msg[10] == s->GetByte( 10 ) ) &&
			( msg[11] == s->GetByte( 11 ) ) )
		{
			// Don't forget to set the flag to stop searching for the message when we find it
			foundMsg = 1;
			
			// Jump ahead 4 bytes in bbp file to skip 
			// the parent message serial number section as it is not required
			if( fseek( file, 4, SEEK_CUR ) )
				Console.Error( 1, "MsgBoardEvent() case 3 : failed to seek Author segment" );
			
			// Read in  author, subject and date info to pass back to client (DO NOT SEND BODY of msg)
			// Count the total number of bytes in posting (not including body as it isn't sent to client)
			msgBytes = x;
			
			// Get size of Author segment
			msg[msgBytes] = fgetc( file );
			if( feof( file ) )
				return;
			++msgBytes;
			
			// Store size of Author segment
			authorBytes = msg[msgBytes-1];
			
			// Fill in msg[] with Author data
			for( x = 0; x < authorBytes; ++x )
			{
				msg[msgBytes+x] = fgetc( file );
				if( feof( file ) )
					return;
			}
			msgBytes += x;
			// Get size of Subject segment
			msg[msgBytes] = fgetc( file );
			if( feof( file ) )
				return;
			++msgBytes;
			
			// Store size of Subject segment
			subjectBytes = msg[msgBytes-1];
			
			// Fill in msg[] with Subject data
			for( x = 0; x < subjectBytes; ++x )
			{
				msg[msgBytes+x] = fgetc( file );
				if( feof( file ) )
					return;
			}
			msgBytes += x;
			
			// Get size of Date segment
			msg[msgBytes] = fgetc( file );
			if( feof( file ) )
				return;
			++msgBytes;
			
			// Store size of Date segment
			dateBytes = msg[msgBytes-1];
			
			// Fill in msg[] with Date data
			for( x = 0; x < dateBytes; ++x )
			{
				msg[msgBytes+x] = fgetc( file );
				if( feof( file ) )
					return;
			}
			msgBytes += x;
			
			// Weird stuff ???  don't know what it does...  Always has to be some thing to screw stuff up.
			// but if it isn't inserted into the message..... KABOOM!!!  no more client (page faults the client!)
			// 29 bytes
			char weird[30]="\x01\x90\x83\xea\x06\x15\x2e\x07\x1d\x17\x0f\x07\x37\x1f\x7b\x05\xeb\x20\x3d\x04\x66\x20\x4d\x04\x66\x0e\x75\x00\x00";
			
			// Fill in weird portion between DATE and BODY
			for( x = 0; x < 29; ++x )
				msg[msgBytes+x] = weird[x];
			
			msgBytes += x;
			// Hope this works!!!
			// Yup it worked!   Whew, thats a load off...
			
			
			// Get the messages body info (body section, body size, body data)
			z = fgetc( file );     // Total number of NULL's in Body of message
			msg[msgBytes] = z;
			++msgBytes;
			
			while( z )  // Loop until number of remaining NULLS equal zero
			{
				y = fgetc( file );     // Get the FIRST Body segment size
				if( feof( file ) )
				{
					fclose( file );
					break;
				}
				
				msg[msgBytes] = y;
				++msgBytes;
				
				// Get Body segment into msg[] ( continue until 0x00 reached )
				for( x = 0; x < y; ++x )
				{
					msg[msgBytes+x] = fgetc( file );
				}
				
				msgBytes += x;
				
				// Decrement NULL count (processed one segment of the Body)
				--z;
			}
			
			msg[1] = (UI08)(msgBytes>>8);
			msg[2] = (UI08)(msgBytes%256);
			msg[3] = 2;              // Set packet 0x71 message type to 0x02 (send full message)
		}
		else // If this isn't the message were looking for, jump ahead to next message
		{
			// Since we didn't find the message in this pass, get this messages size and jump ahead
			msgBytes += (msg[1]*256) + msg[2];

			// Jump to next message
			if( fseek( file, msgBytes, SEEK_SET ) )
				Console << "MsgBoardEvent() case 3 : failed to seek next message" << myendl;
		}
	}

	fclose(file);
	s->Send( msg, msgBytes );
#if defined( _MSC_VER )
#pragma note( "Flush location" )
#endif
	s->FlushBuffer();
}


//////////////////////////////////////////////////////////////////////////////
// FUNCTION:    MsgBoardRemovePost( int s )
//
// PURPOSE:     Marks the posting for a specific message for deletion, thereby
//              removing it from the bulletin boards viewable list.
//
// PARAMETERS:  s        Serial number of the player removing the post
//
// RETURNS:     void
//////////////////////////////////////////////////////////////////////////////
void MsgBoardRemovePost( CSocket *s )
{
	
	// Sample REMOVE POST message from client
	//             | 0| 1| 2| 3| 4| 5| 6| 7| 8| 9|10|11|
	//             |p#|s1|s2|mt|b1|b2|b3|b4|m1|m2|m3|m4| 
	// Client sends 71  0  c  6 40  0  0 18  1  0  0  4
	
	// Read bbi file to determine messages on boards list
	// Find the post and mark it for deletion
	// thereby removing it from the bulletin boards list
	
	FILE *file = NULL;
	// 50 chars for prefix and 4 for the extension plus the ending NULL
	UString fileName;
	char temp[1024];
	
	// Get the integer value of the message serial number
//	SI32 msgSN = s->GetDWord( 8 );
	
	// Calculate the Bulletin Boards serial number
	CItem *msgBoard = calcItemObjFromSer( s->GetDWord( 4 ) );
	
	// Switch depending on what type of message this is:
	// GLOBAL = 0x01000000 -> 0x01FFFFFF
	// REGION = 0x02000000 -> 0x02FFFFFF
	// LOCAL  = 0x03000000 -> 0x03FFFFFF
	switch( s->GetByte( 8 ) )
	{
		case 0x01:
			// GLOBAL post file
			sprintf( temp, "global.bbi" );
			break;
		case 0x02:
			// REGIONAL post file
			CTownRegion *mbRegion;
			mbRegion = calcRegionFromXY( msgBoard->GetX(), msgBoard->GetY(), msgBoard->WorldNumber() );
			sprintf( temp, "region%d.bbi", mbRegion->GetRegionNum() );
			break;
		default:
			// LOCAL post file
			sprintf( temp, "%02x%02x%02x%02x.bbi", s->GetByte( 4 ), s->GetByte( 5 ), s->GetByte( 6 ), s->GetByte( 7 ) );
			break;
	}
	
	// If a MSBBOARDPATH has been define in the SERVER.SCP file, then use it
	if( !cwmWorldState->ServerData()->Directory( CSDDP_MSGBOARD ).empty() )
		fileName = cwmWorldState->ServerData()->Directory( CSDDP_MSGBOARD );
	
	// Create the full path to the file we need to open
	fileName	+= temp;
	file		= fopen( fileName.c_str(), "rb+" );
	
	// If the file exists continue, othewise abort with an error
	if( file != NULL )
	{
		// Ignore first 4 bytes of bbi file as this is reserverd for the current max message serial number being used
		if( fseek( file, 4, SEEK_SET ) )
		{
			Console.Error( 1, "MsgBoardRemovePost() failed to seek first message seg in bbi");
			s->sysmessage( 733 );
			return;
		}
		
		// Loop until we have reached the end of the file
		while( !feof(file)  )
		{
			//  | 0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16  17  18 
			//  |mg1|mg2|mg3|mg4|mo1|mo2|DEL|sg1|sg2|xx1|xx2|yy1|yy2|NS1|NS2|NS3|NS4|co1|co2|
			// "\x40\x1c\x53\xeb\x0e\xb0\x00\x00\x00\x00\x3a\x00\x3a\x40\x00\x00\x19\x00\x00";
			
			// Fill up the msg with data from the bbi file
			if( fread( msg, sizeof(char), 19, file ) != 19 )
			{
				Console.Print( "MsgBoardRemovePost() Could not find message to mark deleted\n" );
				if( feof( file ) ) 
					break;
			}
			
			if( ( msg[0] == s->GetByte( 8 ) )  &&
				( msg[1] == s->GetByte( 9 ) )  &&
				( msg[2] == s->GetByte( 10 ) ) &&
				( msg[3] == s->GetByte( 11 ) )    )
			{
				// Jump back to the DEL segment in order to mark the post for deletion
				fseek( file, -13, SEEK_CUR );
				
				// Write out the mark for deletion value (0x00)
				fputc( 0, file );
				
				// Inform user that the post has been removed
				s->sysmessage( 734 );
				
				// We found the message we wanted so break out and close the file
				break;
			}
			
		}
	}
	else
	{
		Console.Error( 1, "Could not open file %s for reading", fileName.c_str() );
	}

	if( file )
		fclose( file );
	
	// Put code to actually remove the post from the bulletin board here.
	// Posted messages use serial numbers from 0x01000000 to 0x03FFFFFF so they
	// will not interfere with other worldly objects that start at serial number 0x40000000
	// If, however, this is a problem, then simply remove this portion of code and the
	// messages will not be removed on the client but will still be marked for removal
	// in the message board files.
	CPRemoveItem toRemove;
	toRemove.Serial( s->GetDWord( 8 ) );
	s->Send( &toRemove );
	// Remove code above to prevent problems with client if necessary
}


//////////////////////////////////////////////////////////////////////////////
// FUNCTION:    MsgBoardEvent( int s )
//
// PURPOSE:     Handle all of the different Bulletin Board message types.
//
// PARAMETERS:  s           Players serial number         
//
// RETURNS:     void
//////////////////////////////////////////////////////////////////////////////
void MsgBoardEvent( CSocket *s )
{
	// Message \x71 has numerous uses for the Bulletin Board
	// so we need to get the type of message from the client first.
	int msgType = s->GetByte( 3 ); 
	
	// If this was due to a double click event
	if( s->GetByte( 0 ) == 0x06 )
		msgType = 0;
	
	CChar *mChar = s->CurrcharObj();
	switch( msgType )
	{
		case 0:  // Server->Client: Prepare to draw message board, send title + misc info to client
			{        // show message board code goes here (called by doubleclick())
				MsgBoardOpen( s );
				break;
			}
		case 1:  // Server->Client: Send list of message postings (subjects only)
			{        // message sent out by CASE 4: below 
				break;
			}
		case 2:  // Server->Client: Sending body of message to client after subject dbl-clicked
			{        // message sent out by CASE 3: below
				break;
			}
		case 3:  // Client->Server: Client has dbl-clicked on subject, requesting body of message
			{        // Example  [SEND:12] 71 00 0c 03 40 07 ba 3d 40 1c 53 eb
				MsgBoardOpenPost( s );
				break;
			}
		case 4:  // Client->Server: Client has ACK'ed servers download of posting serial numbers
			{
				// Check to see whether client has ACK'd all of our message ID's before proceeding
				s->PostAckCount( s->PostAckCount() + 1 );
				//printf(" pstAckCont=%d        postCount=%d\n", postAckCount[s], postCount[s]);
				if( s->PostAckCount() != s->PostCount() )
					return;
				
				// Server needs to handle ACK from client that contains the posting serial numbers
				MsgBoardList( s );
				break;
			}
		case 5:  // Client->Server: Client clicked on Post button (either from the main board or after pressing the Reply)
			{    //                 Reply just switches to the Post item.
				
				// Check privledge level against server.scp msgpostaccess
				if( mChar->IsGM() || cwmWorldState->ServerData()->MsgBoardPostingLevel() )
					MsgBoardPost( s, mChar->GetPostType(), 0 );
				else
					s->sysmessage( 1640 );
				
				break;
			}
		case 6:  // Remove post from Bulletin board
			{
				//             |p#|s1|s2|mt|b1|b2|b3|b4|m1|m2|m3|m4| 
				// Client sends 71  0  c  6 40  0  0 18  1  0  0  4
				if( mChar->IsGM() || cwmWorldState->ServerData()->MsgBoardPostRemovalLevel() )
					MsgBoardRemovePost( s );
				break;
			}
		default:
			Console.Error( 1, "MsgBoardEvent() Unknown msgType:%x for message: %x", s->GetByte( 3 ), s->GetByte( 0 ) );
			break;
	}
}

//////////////////////////////////////////////////////////////////////////////
// FUNCTION:    MsgBoardPostQuest( int serial, int questType )
//
// PURPOSE:     Used to read in the template for displaying a typical quest
//              message based on the type of quest and the serial number of
//              the NPC or Item.  Certain parameters can be used as variables
//              to replace certain NPC such as %n for NPC name, %t NPC title
//              etc.  See the MSGBOARD.SCP file for an example.
//
// PARAMETERS:  serial      NPC or Item serial number         
//              questType   Type of quest being posted (used to determine
//                          if the item_st or char_st should be used to 
//                          for replacing paramters in the script.
//
// RETURNS:     0           Failed to post message
//              1           Post was successfull
//
// NOTES:       Currently only escort quests work so this function us still
//              in its early stages in regards to the questType parameter.
//////////////////////////////////////////////////////////////////////////////
bool MsgBoardPostQuest( SERIAL serial, int questType )
{
	std::string subjectEscort		= Dictionary->GetEntry( 735 );  // Default escort message 
	std::string subjectBounty		= Dictionary->GetEntry( 736 );  // Default bounty message 
	std::string subjectItem			= Dictionary->GetEntry( 737 );          // Default item message 
	char		subject[50]			= "";                             // String that will hold the default subject
	int			sectionEntrys[MAXENTRIES];                            // List of SECTION items to store for randomizing
	
	int         listCount           = 0;  // Number of entries under the ESCORTS section, used to randomize selection
	int         entryToUse          = 0;  // Entry of the list that will be used to create random message
	
	int         linesInBody         = 0;  // Count of number of lines in body of post
	int         lineLength          = 0;  // Length of the line just read in including terminating NULL
	int         offset              = 0;  // Offset to next line in buffer
	int         numLinesOffset      = 0;  // Offset to the number of lines in body field
	UString		sect;
	
	// msg2Post[] Buffer initialization
	msg2Post[0]   = 0x71;   // Packet ID
	msg2Post[1]   = 0x00;   // Size of packet (High byte)
	msg2Post[2]   = 0x00;   // Size of packet (Low byte)
	
	// This is the type of quest being posted:
	// The value will start arbitrarily at 0xFF and count down
	//    ESCORT = 0xFF (defined in msgboard.h)
	//    BOUNTY = 0xFE
	//    ITEM   = 0xFD
	switch ( questType )
	{
		case ESCORTQUEST:	msg2Post[3]   = (UI08)(ESCORTQUEST);	break;
		case BOUNTYQUEST:	msg2Post[3]   = (UI08)(BOUNTYQUEST);	break;
		case ITEMQUEST:		msg2Post[3]   = (UI08)(ITEMQUEST);		break;
		default:
			Console.Error( 1, "MsgBoardPostQuest() undefined questType, aborting quest!" );
			return false;
	}
	
	// Since quest posts can only be regional or global, can use the BullBoard SN fields as Char or Item fields
	msg2Post[4]   = (UI08)(serial>>24);		// Normally Bulletin Board SN1 but used for quests as Char or Item SN1
	msg2Post[5]   = (UI08)(serial>>16);		// Normally Bulletin Board SN2 but used for quests as Char or Item SN2
	msg2Post[6]   = (UI08)(serial>>8);		// Normally Bulletin Board SN3 but used for quests as Char or Item SN3
	msg2Post[7]   = (UI08)(serial%256);		// Normally Bulletin Board SN4 but used for quests as Char or Item SN4
	msg2Post[8]   = 0x00;					// Reply to message serial number ( 00 00 00 00 for base post )
	msg2Post[9]   = 0x00;					// Reply to message serial number ( 00 00 00 00 for base post )
	msg2Post[10]  = 0x00;					// Reply to message serial number ( 00 00 00 00 for base post )
	msg2Post[11]  = 0x00;					// Reply to message serial number ( 00 00 00 00 for base post )
	
	ScriptSection *EscortData = NULL;
	UString tag;
	UString data;
	
	switch( questType )
	{
		case ESCORTQUEST:
			{
				ScriptSection *Escort = FileLookup->FindEntry( "ESCORTS", msgboard_def );
				if( Escort == NULL )
					return false;
				for( tag = Escort->First(); !Escort->AtEnd(); tag = Escort->Next() )
				{
					data = Escort->GrabData();
					if( tag.upper() == "ESCORT" )
					{
						if( listCount >= MAXENTRIES )
						{
							Console.Error( 1, "MsgBoardPostQuest() Too many entries in ESCORTS list [MAXENTRIES=%d]", MAXENTRIES );
							break;
						}
						sectionEntrys[listCount] = data.toLong();
						++listCount;
					}
				}
				// If no entries are found in the list, then there must be no entries at all.
				if( listCount == 0 )
				{
					Console.Error( 1, "MsgBoardPostQuest() No msgboard dfn entries found" );
					return false;
				}
				
				// Choose a random number between 1 and listCount to use as a message
				entryToUse = RandomNum( 1, listCount );
#if defined( UOX_DEBUG_MODE )
				Console.Error( 1, "MsgBoardPostQuest() listCount=%d  entryToUse=%d", listCount, entryToUse );
#endif
				sect		= "ESCORT " + UString::number( sectionEntrys[entryToUse-1] );
				EscortData	= FileLookup->FindEntry( sect, msgboard_def );
				if( EscortData == NULL )
				{
					Console.Error( 1, "MsgBoardPostQuest() Couldn't find entry %s", sect.c_str() );
					return false;
				}
				break;
			}
			
		default:
			Console.Error( 1, "MsgBoardPostQuest() Invalid questType %d", questType );
			return false;
	}
	
	////////////////////////////////////////////////////////////////////////////////////
	//  Randomly picked a message, now get the message data and fill in up the buffer //
	////////////////////////////////////////////////////////////////////////////////////
	
	char  *flagPos = NULL;
	char  flag;
	char  tempString[64];
	
	// Insert the default subject line depending on the type of quest selected
	switch( questType )
	{
		case ESCORTQUEST:
			// Copy the default subject to the generic subject string
			strncpy( subject, subjectEscort.c_str(), sizeof(subject) );
			break;
			
		case BOUNTYQUEST:
			// Copy the default subject to the generic subject string
			strncpy( subject, subjectBounty.c_str(), sizeof(subject) );
			break;
			
		case ITEMQUEST:
			// Copy the default subject to the generic subject string
			strncpy( subject, subjectItem.c_str(), sizeof(subject) );
			break;
			
		default:
			Console.Error( 2, "MsgBoardPostQuest() invalid quest type" );
			return false;
	}
	
	// Set the SizeOfSubject field in the buffer and copy the subject string to the buffer
	msg2Post[12] = strlen(subject) + 1;
	strncpy( (char *)&msg2Post[13], subject, msg2Post[12] );
	
	// Set the offset to one past linesInBody count value of the buffer
	// Point to the Size of the line segment
	offset += ( msg2Post[12] + 13 + 1 );
	
	// Set the subject
	numLinesOffset = offset - 1;
	// Read in the random post message choosen above and fill in buffer body for posting
	char fullLine[1024];
	for( tag = EscortData->First(); !EscortData->AtEnd(); tag = EscortData->Next() )
	{
		data = EscortData->Next();
		sprintf( fullLine, "%s %s", tag.c_str(), data.c_str() );
		
		flagPos = strchr( fullLine, '%' );
		
		// Loop until we don't find anymore replaceable parameters
		while( flagPos )
		{
			if( flagPos )
			{
				// Move the the letter indicating what text to insert
				flag = *(flagPos + 1);
				
				// Save the remainder of the original string temporarily
				strcpy( tempString, (flagPos+2) );
				
				// Replace the flag with the requested text
				CChar *npc = calcCharObjFromSer( serial );
				switch( flag )
				{
					case 'n':	// NPC Name
						strcpy( flagPos, npc->GetName().c_str() );
						strcat( fullLine, tempString );
						break;
					case 'l':	// LOCATION in X, Y coords
						sprintf( flagPos, "%d, %d", npc->GetX(), npc->GetY() );
						strcat( fullLine, tempString );
						break;
					case 't':	// NPC title
						strcpy( flagPos, npc->GetTitle().c_str() );
						strcat( fullLine, tempString );
						break;
					case 'r':	// Destination Region Name
						strcpy( flagPos, regions[npc->GetQuestDestRegion()]->GetName().c_str() ); 
						strcat( fullLine, tempString );
						break;
					case 'R':	// Region Name
						strcpy( flagPos, npc->GetRegion()->GetName().c_str() ); 
						strcat( fullLine, tempString );
						break;
					default:
						break;
				}
				
				// Look for another replaceable parameter
				flagPos = strchr( flagPos, '%' );
			}
		}
		// Get the length of the line read into 'temp'
		// after being modified with any extra info due to flags (plus one for the terminating NULL)
		lineLength = ( sect.length() + 1 );
		
		msg2Post[offset] = lineLength;
		++offset;
		
		// Append the string in the msg2Post buffer
		memcpy( &msg2Post[offset], sect.c_str(), (lineLength+1) );
		offset += lineLength;
		
		// Increment the total number of lines read in
		++linesInBody;
	}
	msg2Post[1] = (UI08)(offset>>8);
	msg2Post[2] = (UI08)(offset%256);
	msg2Post[numLinesOffset] = linesInBody;
	
	// If the message is posted to the message board successfully
	// RETURN 1 otherwise RETURN 0 to indicate a failure of some sort
	if( MsgBoardPost( 0, REGIONALPOST, 1 ) )
		return true;  // Post succeeded

	return false;
}

//////////////////////////////////////////////////////////////////////////////
// FUNCTION:    MsgBoardQuestEscortCreate( int npcIndex )
//
// PURPOSE:     Used to generate an escort quest based on the currently
//              NPC's serial number
//
// PARAMETERS:  npcIndex     Character
//
// RETURNS:     void
//////////////////////////////////////////////////////////////////////////////
void MsgBoardQuestEscortCreate( CChar *npcIndex )
{
	// Choose a random region as a destination for the escort quest (except for the same region as the NPC was spawned in)
	do 
	{
		if( !cwmWorldState->escortRegions.empty() )
		{
			// If the number of escort regions is 1, check to make sure that the only 
			// valid escort region is not the NPC's current location - if it is Abort
			if( cwmWorldState->escortRegions.size() == 1 && cwmWorldState->escortRegions[0] == npcIndex->GetRegionNum() )
			{
				npcIndex->SetQuestDestRegion( 0 );
				break;
			}
			npcIndex->SetQuestDestRegion( cwmWorldState->escortRegions[RandomNum( static_cast<size_t>(0), (cwmWorldState->escortRegions.size()-1) )] );
		}
		else
		{
			npcIndex->SetQuestDestRegion( 0 );  // If no escort regions have been defined in REGIONS.SCP then we can't do it!!
			break;
		}
	} while( npcIndex->GetQuestDestRegion() == npcIndex->GetRegionNum() );
	
	// Set quest type to escort
	npcIndex->SetQuestType( ESCORTQUEST );
	
	// Make sure they don't move until an player accepts the quest
	npcIndex->SetNpcWander( 0 );                // Don't want our escort quest object to wander off.
	npcIndex->SetNPCAiType( aiNOAI );                // Remove any AI from the escort (should be dumb, just follow please :)
	npcIndex->SetQuestOrigRegion( npcIndex->GetRegionNum() );  // Store this in order to remeber where the original message was posted
	
	// Set the expirey time on the NPC if no body accepts the quest
	if( cwmWorldState->ServerData()->SystemTimer( tSERVER_ESCORTWAIT ) )
		npcIndex->SetTimer( tNPC_SUMMONTIME, cwmWorldState->ServerData()->BuildSystemTimeValue( tSERVER_ESCORTWAIT ) );
	
	// Make sure the questDest is valid otherwise don't post and delete the NPC
	if( !npcIndex->GetQuestDestRegion() )
	{
		Console.Error( 2, "MsgBoardQuestEscortCreate() No valid regions defined for escort quests" );
		npcIndex->Delete();
		return;
	}
	
	// Post the message to the message board in the same REGION as the NPC
	if( !MsgBoardPostQuest( npcIndex->GetSerial(), ESCORTQUEST) )
	{
		Console.Error( 3, "MsgBoardQuestEscortCreate() Failed to add quest post for %s", npcIndex->GetName().c_str() );
		Console.Error( 3, "MsgBoardQuestEscortCreate() Deleting NPC %s", npcIndex->GetName().c_str() );
		npcIndex->Delete();
		return;
	}

#if defined( UOX_DEBUG_MODE )
	Console.Error( 2, "MsgBoardQuestEscortCreate() Escort quest for:\n       %s to be escorted to %s", npcIndex->GetName(), regions[npcIndex->GetQuestDestRegion()]->GetName().c_str() );
#endif
}

//////////////////////////////////////////////////////////////////////////////
// FUNCTION:    MsgBoardQuestEscortArrive( int npcIndex, int pcIndex )
//
// PURPOSE:     Called when escorted NPC reaches its destination
//
// PARAMETERS:  npcIndex   NPC
//              pcIndex    Player
//
// RETURNS:     void
//////////////////////////////////////////////////////////////////////////////
void MsgBoardQuestEscortArrive( CChar *npcIndex, CSocket *mSock )
{
	CChar *mChar = mSock->CurrcharObj();
	if( !ValidateObject( mChar ) )
		return;

	// Calculate payment for services rendered
	UI16 servicePay = ( RandomNum(0, 20) * RandomNum(1, 30) );  // Equals a range of 0 to 600 possible gold with a 5% chance of getting 0 gold

	// If they have no money, well, oops!
	if( servicePay == 0 )
		npcIndex->talk( mSock, 738, false, mChar->GetName().c_str(), regions[npcIndex->GetQuestDestRegion()]->GetName().c_str() );
	else // Otherwise pay the poor sod for his time
	{
		// Less than 75 gold for a escort is pretty cheesey, so if its between 1 and 75, add a randum amount of between 75 to 100 gold
		if( servicePay < 75 ) 
			servicePay += RandomNum( 75, 100 );
		Items->CreateItem( mSock, mChar, 0x0EED, servicePay, 0, OT_ITEM, true );
		Effects->goldSound( mSock, servicePay );
		npcIndex->talk( mSock, 739, false, mChar->GetName().c_str(), regions[npcIndex->GetQuestDestRegion()]->GetName().c_str() );
	}
	
	// Inform the PC of what he has just been given as payment
	
	mSock->sysmessage( 740, servicePay, npcIndex->GetName().c_str(), npcIndex->GetTitle().c_str() );
	
	// Take the NPC out of quest mode
	npcIndex->SetNpcWander( 2 );         // Wander freely
	npcIndex->SetFTarg( NULL );			 // Reset follow target
	npcIndex->SetQuestType( 0 );         // Reset quest type
	npcIndex->SetQuestDestRegion( 0 );   // Reset quest destination region
	
	// Set a timer to automatically delete the NPC
	npcIndex->SetTimer( tNPC_SUMMONTIME, cwmWorldState->ServerData()->BuildSystemTimeValue( tSERVER_ESCORTDONE ) );
	npcIndex->SetOwner( NULL );
}


//////////////////////////////////////////////////////////////////////////////
// FUNCTION:    MsgBoardQuestEscortDelete( int npcIndex )
//
// PURPOSE:     Called when escorted NPC needs to be deleted from the world
//              (just a wrapper in case some additional logic needs to be added)
//
// PARAMETERS:  npcIndex   NPC
//
// RETURNS:     void
//////////////////////////////////////////////////////////////////////////////
void MsgBoardQuestEscortDelete( CChar *npcIndex )
{
	npcIndex->SetDead( true );
	npcIndex->Delete();
}


//////////////////////////////////////////////////////////////////////////////
// FUNCTION:    MsgBoardQuestEscortRemovePost( int npcIndex )
//
// PURPOSE:     Marks the posting for a specific NPC for deletion, thereby
//              removing it from the bulletin boards viewable list.
//
// PARAMETERS:  npcIndex   NPC
//
// RETURNS:     void
//////////////////////////////////////////////////////////////////////////////
void MsgBoardQuestEscortRemovePost( CChar *npcIndex )
{
	// Read bbi file to determine messages on boards list
	// Find the post related to this NPC's quest and mark it for deletion
	// thereby removing it from the bulletin boards list
	
	SERIAL s	= npcIndex->GetSerial();
	FILE *file	= NULL;
	// 50 chars for prefix and 4 for the extension plus the ending NULL
	UString fileName;
	
	// REGIONAL post file
	UString temp = "region" + UString::number( npcIndex->GetQuestOrigRegion() ) + ".bbi";
	
	// If a MSBBOARDPATH has been define in the SERVER.SCP file, then use it
	if( !cwmWorldState->ServerData()->Directory( CSDDP_MSGBOARD ).empty() )
		fileName = cwmWorldState->ServerData()->Directory( CSDDP_MSGBOARD );
	
	// Set fileName to REGIONAL.bbi
	//s->sysmessage( "Opening REGIONAL.bbi messages" );
	fileName	+= temp;
	file		= fopen( fileName.c_str(), "rb+" );
	
	// If the file exists continue, othewise abort with an error
	if( file != NULL )
	{
		// Ignore first 4 bytes of bbi file as this is reserverd for the current max message serial number being used
		if( fseek( file, 4, SEEK_SET ) )
		{
			Console.Error( 1, "MsgBoardQuestEscortRemovePost() failed to seek first message seg in bbi" );
			return;
		}
		
		// Loop until we have reached the end of the file
		while( !feof( file )  )
		{
			//  | 0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16  17  18     NS = NPC Serial
			//  |mg1|mg2|mg3|mg4|mo1|mo2|DEL|sg1|sg2|xx1|xx2|yy1|yy2|NP1|NS2|NS3|NS4|co1|co2|
			// "\x40\x1c\x53\xeb\x0e\xb0\x00\x00\x00\x00\x3a\x00\x3a\x40\x00\x00\x19\x00\x00";
			
			// Fill up the msg with data from the bbi file
			if( fread( msg, sizeof(char), 19, file ) != 19 )
			{
				Console.Error( 1, "MsgBoardQuestEscortRemovePost() Could not find message to mark deleted");
				if( feof( file ) ) 
					break;
			}
			
			if( ( msg[13] == (s>>24) ) &&
				( msg[14] == (s>>16) ) &&
				( msg[15] == (s>>8)  ) &&
				( msg[16] == s%256   ) )
			{
				// Jump back to the DEL segment in order to mark the post for deletion
				fseek( file, -13, SEEK_CUR );
				
				// Write out the mark for deletion value (0x00)
				fputc( 0, file );
				
				// We found the message we wanted so break out and close the file
				break;
			}
		}
	}
	else
	{
		Console.Error( 1, "Failed to open file %s for reading", temp.c_str() );
	}
	if( file )
		fclose( file );
}

//////////////////////////////////////////////////////////////////////////////
// FUNCTION:    MsgBoardMaintenance( void )
//
// PURPOSE:     Cleans out old posts which are older than the MSGRETENTION
//              period set in SERVER.SCP and any posts that have been marked
//              for deletion (such as escort quests after they have been 
//              accepted or posts that have been "removed" through the user
//              interface.  This is called as a cleanup routine on server
//              startup.  Compacts and reassigns message serial numbers.
//
// PARAMETERS:  void
//
// RETURNS:     void
//
// NOTES:       This function uses the _findfirst() and _findnext() functions
//              which are OS specific.  There will definetly be an issue in 
//              compiling this on LINUX because I have no idea what the
//              structure to be passed to the functions needs to be for LINUX.
//              This will definetly have to be #ifdef'd to handle this.
//              Anyone with LINUX experience please feel free to fix it up.
//////////////////////////////////////////////////////////////////////////////
void MsgBoardMaintenance( void )
{
   return;
}

}
