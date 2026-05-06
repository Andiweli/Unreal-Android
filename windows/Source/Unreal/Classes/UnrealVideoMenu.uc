//=============================================================================
// UnrealVideoMenu
//=============================================================================
class UnrealVideoMenu expands UnrealMenu
	localized;

var float brightness;
var float Gamma;
var string[32] CurrentRes;
var string[255] AvailableRes;
var string[64] MenuValues[20];
var string[32] Resolutions[16];
var int resNum;
var int SoundVol, MusicVol;
var bool bLowTextureDetail, bLowSoundQuality;

function bool ProcessLeft()
{
	if ( Selection == 1 )
	{
		Brightness = FMax(0.2, Brightness - 0.1);
		PlayerOwner.ConsoleCommand("set ini:Engine.Engine.ViewportManager Brightness "$Brightness);
		PlayerOwner.ConsoleCommand("FLUSH");
		return true;
	}
	else if ( Selection == 2 )
	{
		Gamma = FMax(1.0, Gamma - 0.05);
		PlayerOwner.ConsoleCommand("set ini:Engine.Engine.ViewportManager Gamma "$Gamma);
		return true;
	}
	else if ( Selection == 4 )
	{
		ResNum--;
		if ( ResNum < 0 )
		{
			ResNum = ArrayCount(Resolutions);
			While ( Resolutions[ResNum] == "" )
				ResNum--;
		}
		MenuValues[4] = Resolutions[ResNum];
		return true;
	}	
	else if ( Selection == 6 )
	{
		MusicVol = Max(0, MusicVol - 32);
		PlayerOwner.ConsoleCommand("set ini:Engine.Engine.AudioDevice MusicVolume "$MusicVol);
		return true;
	}
	else if ( Selection == 7 )
	{
		SoundVol = Max(0, SoundVol - 32);
		PlayerOwner.ConsoleCommand("set ini:Engine.Engine.AudioDevice SoundVolume "$SoundVol);
		return true;
	}	
	else if ( Selection == 5 )
	{
		bLowTextureDetail = !bLowTextureDetail;
		PlayerOwner.ConsoleCommand("set ini:Engine.Engine.ViewportManager LowDetailTextures "$bLowTextureDetail);
		return true;
	}
	else if ( Selection == 8 )
	{
		bLowSoundQuality = !bLowSoundQuality;
		PlayerOwner.ConsoleCommand("set ini:Engine.Engine.AudioDevice LowSoundQuality "$bLowSoundQuality);
		return true;
	}
	return false;
}

function bool ProcessRight()
{
	local string[255] ParseString;
	local string[32] FirstString;
	local int p;

	if ( Selection == 1 )
	{
		Brightness = FMin(1, Brightness + 0.1);
		PlayerOwner.ConsoleCommand("set ini:Engine.Engine.ViewportManager Brightness "$Brightness);
		PlayerOwner.ConsoleCommand("FLUSH");
		return true;
	}
	else if ( Selection == 2 )
	{
		Gamma = FMin(1.4, Gamma + 0.05);
		PlayerOwner.ConsoleCommand("set ini:Engine.Engine.ViewportManager Gamma "$Gamma);
		return true;
	}
	else if ( Selection == 4 )
	{
		ResNum++;
		if ( (ResNum >= ArrayCount(Resolutions)) || (Resolutions[ResNum] == "") )
			ResNum = 0;
		MenuValues[4] = Resolutions[ResNum];
		return true;
	}	
	else if ( Selection == 6 )
	{
		MusicVol = Min(255, MusicVol + 32);
		PlayerOwner.ConsoleCommand("set ini:Engine.Engine.AudioDevice MusicVolume "$MusicVol);
		return true;
	}
	else if ( Selection == 7 )
	{
		SoundVol = Min(255, SoundVol + 32);
		PlayerOwner.ConsoleCommand("set ini:Engine.Engine.AudioDevice SoundVolume "$SoundVol);
		return true;
	}
	else if ( Selection == 5 )
	{
		bLowTextureDetail = !bLowTextureDetail;
		PlayerOwner.ConsoleCommand("set ini:Engine.Engine.ViewportManager LowDetailTextures "$bLowTextureDetail);
		return true;
	}
	else if ( Selection == 8 )
	{
		bLowSoundQuality = !bLowSoundQuality;
		PlayerOwner.ConsoleCommand("set ini:Engine.Engine.AudioDevice LowSoundQuality "$bLowSoundQuality);
		return true;
	}
	return false;
}		

function bool ProcessSelection()
{
	if ( Selection == 3 )
	{
		PlayerOwner.ConsoleCommand("TOGGLEFULLSCREEN");
		CurrentRes = PlayerOwner.ConsoleCommandResult("GetCurrentRes");
		GetAvailableRes();
		return true;
	}
	else if ( Selection == 4 )
	{
		PlayerOwner.ConsoleCommand("SetRes "$MenuValues[4]);
		CurrentRes = PlayerOwner.ConsoleCommandResult("GetCurrentRes");
		GetAvailableRes();
		return true;
	}
	else if ( Selection == 5 )
	{
		bLowTextureDetail = !bLowTextureDetail;
		PlayerOwner.ConsoleCommand("set ini:Engine.Engine.ViewportManager LowDetailTextures "$bLowTextureDetail);
		return true;
	}
	else if ( Selection == 8 )
	{
		bLowSoundQuality = !bLowSoundQuality;
		PlayerOwner.ConsoleCommand("set ini:Engine.Engine.AudioDevice LowSoundQuality "$bLowSoundQuality);
		return true;
	}
		
	return false;
}


function DrawMenu(canvas Canvas)
{
	local int StartX, StartY, Spacing, i, HelpPanelX;

	DrawBackGround(Canvas, (Canvas.ClipY < 250));
	HelpPanelX = 228;

	Spacing = Clamp(0.04 * Canvas.ClipY, 16, 32);
	StartX = Max(40, 0.5 * Canvas.ClipX - 120);

	DrawTitle(Canvas);
	StartY = Max(36, 0.5 * (Canvas.ClipY - MenuLength * Spacing - 128));

	// draw text
	DrawList(Canvas, false, Spacing, StartX, StartY);  

	// draw icons
	Brightness = float(PlayerOwner.ConsoleCommandResult("get ini:Engine.Engine.ViewportManager Brightness"));
	DrawSlider(Canvas, StartX + 155, StartY + 1, (10 * Brightness - 2), 0, 1);

	Gamma = float(PlayerOwner.ConsoleCommandResult("get ini:Engine.Engine.ViewportManager Gamma"));
	if ( Gamma < 1.0 )
		Gamma = 1.0;
	else if ( Gamma > 1.4 )
		Gamma = 1.4;
	DrawSlider(Canvas, StartX + 155, StartY + Spacing + 1, (20 * Gamma - 20), 0, 1);

	SoundVol = int(PlayerOwner.ConsoleCommandResult("get ini:Engine.Engine.AudioDevice SoundVolume"));
	MusicVol = int(PlayerOwner.ConsoleCommandResult("get ini:Engine.Engine.AudioDevice MusicVolume"));
	DrawSlider(Canvas, StartX + 155, StartY + 5*Spacing + 1, MusicVol, 0, 32);
	DrawSlider(Canvas, StartX + 155, StartY + 6*Spacing + 1, SoundVol, 0, 32);

	if ( CurrentRes == "" )
		GetAvailableRes();
	else if ( AvailableRes == "" )
		GetAvailableRes();

	SetFontBrightness( Canvas, (Selection == 4) );
	Canvas.SetPos(StartX + 152, StartY + Spacing * 3);
	if ( MenuValues[4] ~= CurrentRes )
		Canvas.DrawText("["$MenuValues[4]$"]", false);
	else
		Canvas.DrawText(" "$MenuValues[4], false);
	Canvas.DrawColor = Canvas.Default.DrawColor;

	bLowTextureDetail = bool(PlayerOwner.ConsoleCommandResult("get ini:Engine.Engine.ViewportManager LowDetailTextures"));
	SetFontBrightness( Canvas, (Selection == 5) );
	Canvas.SetPos(StartX + 152, StartY + Spacing * 4);
	if ( bLowTextureDetail )
		Canvas.DrawText("Low", false);
	else
		Canvas.DrawText("High", false);
	Canvas.DrawColor = Canvas.Default.DrawColor;

	bLowSoundQuality = bool(PlayerOwner.ConsoleCommandResult("get ini:Engine.Engine.AudioDevice LowSoundQuality"));
	SetFontBrightness( Canvas, (Selection == 8) );
	Canvas.SetPos(StartX + 152, StartY + Spacing * 7);
	if ( bLowSoundQuality )
		Canvas.DrawText("Low", false);
	else
		Canvas.DrawText("High", false);
	Canvas.DrawColor = Canvas.Default.DrawColor;
	
	// Draw help panel
	DrawHelpPanel(Canvas, StartY + MenuLength * Spacing, HelpPanelX);
}

function GetAvailableRes()
{
	local int p,i;
	local string[255] ParseString;

	AvailableRes = PlayerOwner.ConsoleCommandResult("GetRes");
	resNum = 0;
	ParseString = AvailableRes;
	p = InStr(ParseString, " ");
	while ( (ResNum < ArrayCount(Resolutions)) && (p != -1) ) 
	{
		Resolutions[ResNum] = Left(ParseString, p);
		ParseString = Right(ParseString, Len(ParseString) - p - 1);
		p = InStr(ParseString, " ");
		ResNum++;
	}

	Resolutions[ResNum] = ParseString;
	for ( i=ResNum+1; i< ArrayCount(Resolutions); i++ )
		Resolutions[i] = "";

	CurrentRes = PlayerOwner.ConsoleCommandResult("GetCurrentRes");
	MenuValues[4] = CurrentRes;
	for ( i=0; i< ResNum+1; i++ )
		if ( MenuValues[4] ~= Resolutions[i] )
		{
			ResNum = i;
			return;
		}

	ResNum = 0;
	MenuValues[4] = Resolutions[0];
}

defaultproperties
{
	 MenuTitle="AUDIO/VIDEO"
	 MenuList(1)="Brightness"
	 MenuList(2)="Gamma"
	 MenuList(3)="Toggle Fullscreen Mode"
	 MenuList(4)="Select Resolution"
	 MenuList(5)="Texture Detail"
	 MenuList(6)="Music Volume"
	 MenuList(7)="Sound Volume"
	 MenuList(8)="Sound Quality"
     HelpMessage(1)="Adjust display brightness using the left and right arrow keys."
     HelpMessage(2)="Adjust world gamma for dark lightmapped areas. Android/GLES effects, sprites, HUD and menus are not affected."
     HelpMessage(3)="Display Unreal in a window. Note that going to a software display mode may remove high detail actors that were visible with hardware acceleration."
	 HelpMessage(4)="Use the left and right arrows to select a resolution, and press enter to select this resolution."
	 HelpMessage(5)="Use the low texture detail option to improve performance.  Changes to this setting will take effect on the next level change."
     HelpMessage(6)="Adjust the volume of the music using the left and right arrow keys."
     HelpMessage(7)="Adjust the volume of sound effects in the game using the left and right arrow keys."
	 HelpMessage(8)="Use the low sound quality option to improve performance on machines with 32 megabytes or less of memory.  Changes to this setting will take effect on the next level change."
     MenuLength=8
}
