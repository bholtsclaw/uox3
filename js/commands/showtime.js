function CommandRegistration()
{
	RegisterCommand( "showtime", 1, true );
}

function command_SHOWTIME( socket, cmdString )
{
	var hour 	= GetHour();
	var minute	= GetMinute();
	var ampm	= " AM";
	if( hour >= 12 )
	{
		ampm = " PM";
		hour = hour - 12;
	}
	var hDisp = NumToString( hour );
	var mDisp = NumToString( minute );
	if( minute < 10 )
		mDisp = "0" + mDisp;

	var toDisplay	= "Time: " + hDisp + ":" + mDisp + ampm;
	socket.SysMessage( toDisplay );
}