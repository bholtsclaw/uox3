// Scissor-usage; Sheep-shearing, cloth-cutting
// 02/03/2003 Xuri; xuri@sensewave.com
// This script contains all the default functions of the scissors,
// from sheep-shearing to cloth cutting.

function onUse( pUser, iUsed )
{
	var socket = pUser.socket;
	if( socket && iUsed && iUsed.isItem )
	{
		var isInRange = pUser.InRange( iUsed, 3 );
		if( !isInRange )
	 	{
			pUser.SysMessage( GetDictionaryEntry( 389, socket.Language ) ); // That is too far away and you cannot reach it.
			return false;
		}
		socket.tempObj = iUsed;		
		pUser.socket.CustomTarget( 0, "What should I use these scissors on?" );
	}
	return false;
}

function onCallback0( pSock, myTarget ) 
{
	var pUser = pSock.currentChar;
	var StrangeByte   = pSock.GetWord( 1 );
	var tileID = pSock.GetWord( 17 );

	if( myTarget == null || tileID == 0 )
	{ //Target is invalid or a Maptile
		pUser.SysMessage("You can't use the scissors on that.");
		return;
	}
	else if( StrangeByte == 0 && myTarget.isChar )
	{ //Target is a Character
		var isInRange = pUser.InRange( myTarget, 3 );
		if( !isInRange )
	 	{
			pUser.SysMessage( "You are too far away to reach that." );
			return;
		}
		if( tileID == 0x00df )
		{
			pUser.SysMessage( "This sheep is not yet ready to be shorn." );
			return;
		}
		if( tileID == 0x00cf )
		{ //If target is a Sheep
			pSock.SysMessage( "You shear some wool from the sheep." );
			myTarget.SoundEffect( 0x0248, true );
			myTarget.id = 0x00DF; // remove sheep's wool
			var itemMade = CreateDFNItem( pUser.socket, pUser, "0x0df8", 1, "ITEM", true ); //give the player some wool
			myTarget.StartTimer( 60000, 1, true ); //respawn wool on sheep in 60 seconds
			return;
		}
	}
	else if( StrangeByte == 0 )
	{ //Target is a Dynamic Item
		var isInRange = pUser.InRange( myTarget, 3 );
		if( !isInRange )
	 	{
			pUser.SysMessage( "You are too far away to reach that." );
			return;
		}
		if( myTarget.movable == 3 )
		{
			pUser.SysMessage( "Locked down resources cannot be used!" );
			return;
		} 
	
		var iPackOwner = GetPackOwner( myTarget, 0 );
		if( iPackOwner != null ) // Is the target item in a backpack?
		{
			if( iPackOwner.serial != pUser.serial ) //And if so does the pack belong to the user?
			{
				pUser.SysMessage( "That resource is in someone else's backpack!" );
				return;
			}
		}
		else // ...or is the target item on the ground?
		{
			pUser.SysMessage( "This has to be in your backpack before you can use it." );
			return;
		}
		if( tileID >= 3989 && tileID <= 3996 ) // <-- Decimal item-ids
		{ //Cut bolts of cloth into cut cloth
			pUser.SysMessage( "You cut the material and place it into your backpack." );
			var itemMade = CreateDFNItem( pUser.socket, pUser, "0x1766", myTarget.amount*50, "ITEM", true );  //give the player some cut cloth
			myTarget.Delete();
			return;				
		}
		if(( tileID >= 5981 && tileID <= 5992 ) || ( tileID >= 5397 && tileID <= 5400 ) || ( tileID >= 5422 && tileID <= 5444 ) || ( tileID >= 7933 && tileID <= 7940 )) // <-- Decimal item-ids
		{ //Cut folded cloth/clothes into bandages
			pUser.SysMessage( "You cut the material into bandage and place it in your backpack." );
			var itemMade = CreateDFNItem( pUser.socket, pUser, "0x0e21", myTarget.amount, "ITEM", true );  //give the player some bandages
			myTarget.Delete();
			return;
		}
	}
	pUser.SysMessage( "Scissors cannot be used on that to produce anything." );
}

function onTimer( sheep, timerID )
{
	if( timerID == 1 )
	{
		sheep.id = 0x00cf; // give sheep wool again
	}
}
