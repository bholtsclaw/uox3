// simple lightsource switching script
// 17/06/2001 Yeshe; yeshe@manofmystery.org

function onUseChecked( pUser, iUsed )
{
    pUser.SoundEffect( 0x0047, true );
    iUsed.id = 0x142C;
    return false;
}