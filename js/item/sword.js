function onUse( pUser, iUsed )
{
	var socket = pUser.socket;
	if( socket && iUsed && iUsed.isItem )
	{
		var targMsg = GetDictionaryEntry( 462, socket.Language );
		socket.CustomTarget( 1, targMsg );
	}
	return false;
}

function onCallback1( socket, ourObj )
{
	var mChar = socket.currentChar;

	if( mChar && mChar.isChar )
	{
		var tileID = 0;
		if( !ourObj )
			tileID = socket.GetWord( 17 );
		else if( ourObj.isItem )
			tileID = ourObj.id;
		else if( ourObj.isChar )
		{
			if( ourObj.id == 0x00CF )	// Unshorn Sheep
			{
				CreateBlankItem( socket, mChar, 1, "#", 0x0DF8, 0x0000, "ITEM", true );
				ourObj.id = 0x00DF;
				var delay = RollDice( 2, 3, 0 );
				DoTempEffect( 0, mChar, mChar, 43, delay*300, 0, 0 );
			}
			else
				socket.SysMessage( "You should probably kill that before you attempt to skin it." );
			return;
		}

		if( tileID != 0 )
		{
			if( 	tileID == 0x0CD0 || tileID == 0x0CD3 || tileID == 0x0CD6 || tileID == 0x0CD8 || tileID == 0x0CDA || 
				tileID == 0x0CDD || tileID == 0x0CE0 || tileID == 0x0CE3 || tileID == 0x0CE6 || tileID == 0x0D58 || 
				( tileID >= 0x0CCA && tileID <= 0x0CCE ) || ( tileID >= 0x12B8 && tileID <= 0x12BB ) || tileID == 0x0D42 ||
				tileID == 0x0D43 || tileID == 0x0D58 || tileID == 0x0D59 || tileID == 0x0D70 || tileID == 0x0D85 || 
				tileID == 0x0D94 || tileID == 0x0D95 || tileID == 0x0D98 || tileID == 0x0DA4 || tileID == 0x0DA8 ) // Trees
			{
				MakeKindling( socket, mChar );
			}
			else if( ourObj )
			{
				if( tileID >= 0x09CC && tileID <= 0x09CF )	// Fish
					MakeFishSteaks( socket, mChar, ourObj );
				else if( tileID == 0x1be0 || ( tileID >= 0x1BD7 && tileID <= 0x1BE2 ) )	// Bowcraft
					BowCraft( socket, mChar, ourObj, tileID );
				else if( tileID == 0x2006 )
					CarveCorpse( socket, mChar, ourObj );
				else
					socket.SysMessage( "You cannot carve that." );
			}
		}
	}
}

function BowCraft( socket, mChar, ourObj, tileID )
{
	var ownerObj = GetPackOwner( ourObj, 0 );
	if( ownerObj && mChar.serial == ownerObj.serial )
	{
		var countResource = mChar.ResourceCount( tileID );
		if( countResource >= 2 )
			socket.MakeMenu( 49, 8 );
		else
			socket.SysMessage( GetDictionaryEntry( 782, socket.Language ) );
	}
	else
		socket.SysMessage( GetDictionaryEntry( 781, socket.Language ) );
}

function MakeFishSteaks( socket, mChar, ourObj )
{
	var ownerObj = GetPackOwner( ourObj, 0 );
	if( ownerObj && mChar.serial == ownerObj.serial )
	{
		CreateBlankItem( socket, mChar, 4, "raw fish steak", 0x097A, 0x0000, "ITEM", true );
		if( ourObj.amount > 1 )
			ourObj.amount = ourObj.amount-1;
		else
			ourObj.Delete();
	}
	else
		socket.SysMessage( GetDictionaryEntry( 775, socket.Language ) );
}

function MakeKindling( socket, mChar )
{
	var distX = Math.abs( mChar.x - socket.GetWord( 11 ) );
	var distY = Math.abs( mChar.y - socket.GetWord( 13 ) );
	var distZ = Math.abs( mChar.z - socket.GetByte( 16 ) );

	if( distX > 5 || distY > 5 || distZ > 9 )
	{
		socket.SysMessage( GetDictionaryEntry( 393, socket.Language ) );
		return;
	}

	if( mChar.isonhorse )
		mChar.DoAction( 0x1D );
	else
		mChar.DoAction( 0x0D );

	mChar.SoundEffect( 0x013E, true );
	CreateBlankItem( socket, mChar, 1, "#", 0x0DE1, 0x0000, "ITEM", true );
	socket.SysMessage( GetDictionaryEntry( 1049, socket.Language ) );
}

function CarveCorpse( socket, mChar, ourObj )
{
	if( mChar.InRange( ourObj, 3 ) )
	{
		mChar.DoAction( 0x20 );
		if( (ourObj.morey>>24) == 0 )
		{
			var part1 = 1;
			var part2 = ourObj.morey>>16;
			var part3 = ourObj.morey>>8;
			var part4 = ourObj.morey%256;

			ourObj.morey = (part1<<24) + (part2<<16) + (part3<<8) + part4;
			if( part2 != 0 || ourObj.carveSection != -1 )
				ourObj.Carve( socket );
		}
		else
			socket.SysMessage( GetDictionaryEntry( 1051, socket.Language ) );
	}
	else
		socket.SysMessage( GetDictionaryEntry( 393, socket.Language ) );
}