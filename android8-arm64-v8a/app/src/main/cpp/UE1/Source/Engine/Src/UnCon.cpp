/*=============================================================================
	UnCon.cpp: Implementation of UConsole class
	Copyright 1997 Epic MegaGames, Inc. This software is a trade secret.
=============================================================================*/

#include "EnginePrivate.h"
#include "UnRender.h"

#ifdef PLATFORM_ANDROID
#ifndef UNREAL_ANDROID_VERSION_NAME
#define UNREAL_ANDROID_VERSION_NAME unknown
#endif
#define UE1_ANDROID_STRINGIZE_INNER(x) #x
#define UE1_ANDROID_STRINGIZE(x) UE1_ANDROID_STRINGIZE_INNER(x)

static const char* UE1AndroidVersionNameText()
{
	static char CleanVersion[128];
	static UBOOL Initialized = 0;
	if( !Initialized )
	{
		const char* Raw = UE1_ANDROID_STRINGIZE(UNREAL_ANDROID_VERSION_NAME);
		INT Len = appStrlen(Raw);
		if( Len >= 2 && Raw[0]=='"' && Raw[Len-1]=='"' )
		{
			appStrncpy( CleanVersion, Raw+1, Min<INT>(Len-1,(INT)ARRAY_COUNT(CleanVersion)) );
			CleanVersion[Min<INT>(Len-2,(INT)ARRAY_COUNT(CleanVersion)-1)] = 0;
		}
		else
		{
			appStrncpy( CleanVersion, Raw, ARRAY_COUNT(CleanVersion) );
			CleanVersion[ARRAY_COUNT(CleanVersion)-1] = 0;
		}
		Initialized = 1;
	}
	return CleanVersion;
}

static const char* UE1AndroidAbiText()
{
#if PLATFORM_64BIT
	return "64bit (arm64-v8a)";
#else
	return "32bit (armeabi-v7a)";
#endif
}

static UBOOL UE1AndroidIsTitleFlybyLevel( UViewport* Viewport )
{
	guard(UE1AndroidIsTitleFlybyLevel);
	if( !Viewport || !Viewport->Actor )
		return 0;
	ULevel* RealLevel = Viewport->Actor->GetLevel();
	UGameEngine* GameEngine = Viewport->Client ? Cast<UGameEngine>(Viewport->Client->Engine) : NULL;
	if( RealLevel && GameEngine && RealLevel==GameEngine->GEntry )
		return 1;
	if( Viewport->Actor->Level && Viewport->Actor->Level->Game && Viewport->Actor->Level->Game->GetClass() && !appStricmp(Viewport->Actor->Level->Game->GetClass()->GetName(),"Intro") )
		return 1;
	if( Viewport->Actor->XLevel )
	{
		const char* Map = *Viewport->Actor->XLevel->URL.Map;
		if( Map && (!appStricmp(Map,"Entry") || !appStricmp(Map,"Entry.unr") || !appStricmp(Map,"Unreal.unr")) )
			return 1;
	}
	return 0;
	unguard;
}

static UBOOL UE1AndroidIsRootMainMenu( UViewport* Viewport )
{
	guard(UE1AndroidIsRootMainMenu);
	if( !Viewport || !Viewport->Actor || !Viewport->Actor->bShowMenu || !Viewport->Actor->myHUD || !Viewport->Actor->myHUD->MainMenu )
		return 0;
	AMenu* Menu = Viewport->Actor->myHUD->MainMenu;
	return Menu->ParentMenu==NULL && Menu->GetClass() && !appStricmp(Menu->GetClass()->GetName(),"UnrealMainMenu");
	unguard;
}

static void UE1AndroidDrawVersionAbiOverlay( UViewport* Viewport )
{
	guard(UE1AndroidDrawVersionAbiOverlay);
	if( !Viewport || !Viewport->Actor || !Viewport->Canvas )
		return;
	if( !Viewport->Actor->Level || Viewport->Actor->Level->LevelAction==LEVACT_Loading || Viewport->Actor->Level->LevelAction==LEVACT_Saving || Viewport->Actor->Level->LevelAction==LEVACT_Connecting )
		return;

	// Show exactly on the intro/title flyby and on the root main menu.  Do not
	// leak into OPTIONS/Customize Controls, Prioritize Weapons, game levels, or
	// multiplayer setup submenus.
	if( !UE1AndroidIsTitleFlybyLevel(Viewport) && !UE1AndroidIsRootMainMenu(Viewport) )
		return;

	UFont* Font = Viewport->Canvas->SmallFont ? Viewport->Canvas->SmallFont : Viewport->Canvas->MedFont;
	if( !Font || Viewport->Canvas->ClipX<=0 || Viewport->Canvas->ClipY<=0 )
		return;

	char Text[192];
	appSprintf( Text, "%s %s", UE1AndroidVersionNameText(), UE1AndroidAbiText() );
	INT XL=0, YL=0;
	Viewport->Canvas->StrLen( Font, XL, YL, Text );
	INT X = Max<INT>( 4, (INT)Viewport->Canvas->ClipX - XL - 8 );
	INT Y = Max<INT>( 4, (INT)Viewport->Canvas->ClipY - YL - 6 );
	Viewport->Canvas->Printf( Font, X, Y, "%s", Text );
	unguard;
}
#endif


/*------------------------------------------------------------------------------
	UConsole object implementation.
------------------------------------------------------------------------------*/

IMPLEMENT_CLASS(UConsole);

/*------------------------------------------------------------------------------
	Console.
------------------------------------------------------------------------------*/

//
// Constructor.
//
UConsole::UConsole()
{}

//
// Init console.
//
void UConsole::_Init( UViewport* InViewport )
{
	guard(UConsole::_Init);
	check(sizeof(UConsole)==UConsole::StaticClass->GetPropertiesSize());

	// Set properties.
	Viewport		= InViewport;
	TopLine			= MAX_LINES-1;
	BorderSize		= 1; 

	// Init scripting.
	InitExecution();

	// Start console log.
	Logf(LocalizeGeneral("Engine","Core"));
	Logf(LocalizeGeneral("Copyright","Core"));
	Logf(" ");
	Logf(" ");

	unguard;
}

/*------------------------------------------------------------------------------
	Viewport console output.
------------------------------------------------------------------------------*/

//
// Print a message on the playing screen.
// Time = time to keep message going, or 0=until next message arrives, in 60ths sec
//
void UConsole::WriteBinary( const void* Data, INT Length, EName ThisType )
{
	guard(UConsole::WriteBinary);
	eventMessage( (const char*)Data, ThisType );
	unguard;
}

void UConsole::execConsoleCommand( FFrame& Stack, BYTE*& Result )
{
	guardSlow(UConsole::execLog);

	P_GET_STRING(S);
	P_FINISH;

	*(DWORD*)Result = Viewport->Exec( S, this );

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( UConsole, INDEX_NONE, execConsoleCommand );

/*------------------------------------------------------------------------------
	Rendering.
------------------------------------------------------------------------------*/

//
// Called before rendering the world view.  Here, the
// Viewport console code can affect the screen's Viewport,
// for example by shrinking the view according to the
// size of the status bar.
//
FSceneNode SavedFrame;
void UConsole::PreRender( FSceneNode* Frame )
{
	guard(UConsole::PreRender);

	// Prevent status redraw due to changing.
	eventTick( Viewport->CurrentTime - Viewport->LastUpdateTime );

	// Save the Viewport.
	SavedFrame = *Frame;

	// Compute new status info.
	BorderLines		= 0;
	BorderPixels	= 0;
	ConsoleLines	= 0;

	// Compute sizing of all visible status bar components.
	if( ConsolePos > 0.0 )
	{
		// Show console.
		ConsoleLines = Min(ConsolePos * (FLOAT)Frame->Y, (FLOAT)Frame->Y);
		Frame->Y -= ConsoleLines;
	}
	if( BorderSize>=2 )
	{
		// Encroach on screen area.
		FLOAT Fraction = (FLOAT)(BorderSize-1) / (FLOAT)(MAX_BORDER-1);

		BorderLines = (int)Min((FLOAT)Frame->Y * 0.25f * Fraction,(FLOAT)Frame->Y);
		BorderLines = ::Max(0,BorderLines - ConsoleLines);
		Frame->Y -= 2 * BorderLines;

		BorderPixels = (int)Min((FLOAT)Frame->X * 0.25f * Fraction,(FLOAT)Frame->X) & ~3;
		Frame->X -= 2 * BorderPixels;
	}
	Frame->XB += BorderPixels;
	Frame->YB += ConsoleLines + BorderLines;
	Frame->ComputeRenderSize();

	unguard;
}

//
// Refresh the player console on the specified Viewport.  This is called after
// all in-game graphics are drawn in the rendering loop, and it overdraws stuff
// with the status bar, menus, and chat text.
//
void UConsole::PostRender( FSceneNode* Frame )
{
	guard(UConsole::PostRender);
	check(Viewport->Client->Engine->Client);
	*Frame = SavedFrame;

	// Big status message.
	UFont* LargeFont = Viewport->Canvas->LargeFont;
	char BigMessage[256]="";
	if( Viewport->Actor->bShowMenu )
		appStrcpy( BigMessage, "" );		
	else if( Viewport->Actor->Level->LevelAction==LEVACT_Loading )
		appStrcpy( BigMessage, LocalizeProgress("Loading") );
	else if( Viewport->Actor->Level->LevelAction==LEVACT_Saving )
		appStrcpy( BigMessage, LocalizeProgress("Saving") );
	else if( Viewport->Actor->Level->LevelAction==LEVACT_Connecting )
		appStrcpy( BigMessage, LocalizeProgress("Connecting") );
	else if( Viewport->Actor->Level->Pauser[0] )
	{
		LargeFont = Viewport->Canvas->MedFont;
		appSprintf( BigMessage, LocalizeProgress("Paused"), Viewport->Actor->Level->Pauser );
	}
	if( BigMessage[0] )
	{
		appStrupr( BigMessage );
		INT XL, YL;
		Viewport->Canvas->StrLen( LargeFont, XL, YL, BigMessage );
#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_CENTER_CONSOLE_BIGMESSAGE
		Viewport->Canvas->Printf( LargeFont, (INT)(Viewport->Canvas->ClipX * 0.5f) - XL/2, (INT)(Viewport->Canvas->ClipY * 0.5f) - YL/2, "%s", BigMessage );
#else
		Viewport->Canvas->Printf( LargeFont, Frame->X/2-XL/2, Frame->Y/2-YL/2, "%s", BigMessage );
#endif
	}

	// If the console has changed since the previous frame, draw it.
	INT YStart	   = BorderLines;
	INT YEnd	   = Frame->Y - BorderLines;
	if( ConsoleLines > 0 )
		Viewport->Canvas->DrawPattern( ConBackground, 0.0, 0.0, Frame->X, ConsoleLines, 1.0, 0.0, ConsoleLines, NULL, 1.0, FPlane(0.7,0.7,0.7,0), FPlane(0,0,0,0), 0 );

	// Draw border.
	if( BorderLines>0 || BorderPixels>0 )
	{
		YStart += ConsoleLines;
		FLOAT V = ConsoleLines/2;
		if( BorderLines > 0 )
		{
			Viewport->Canvas->DrawPattern( Border, 0, 0, Frame->X, BorderLines, 1.0, 0.0, 0.0, NULL, 1.0, FPlane(1,1,1,0), FPlane(0,0,0,0), 0 );
			Viewport->Canvas->DrawPattern( Border, 0, YEnd, Frame->X, BorderLines, 1.0, 0.0, 0.0, NULL, 1.0, FPlane(1,1,1,0), FPlane(0,0,0,0), 0 );
		}
		if( BorderPixels > 0 )
		{
			Viewport->Canvas->DrawPattern( Border, 0, YStart, BorderPixels, YEnd-YStart, 1.0, 0.0, 0.0, NULL, 1.0, FPlane(1,1,1,0), FPlane(0,0,0,0), 0 );
			Viewport->Canvas->DrawPattern( Border, Frame->X-BorderPixels, YStart, BorderPixels, YEnd-YStart, 1.0, 0.0, 0.0, NULL, 1.0, FPlane(1,1,1,0), FPlane(0,0,0,0), 0 );
		}
	}

	// Draw console text.
	if( ConsoleLines )
	{
		// Console is visible; display console view.
		INT Y = ConsoleLines-1;
		appSprintf(MsgText[(TopLine + 1 + MAX_LINES) % MAX_LINES],"(> %s_",TypedStr);
		for( INT i=Scrollback; i<(NumLines+1); i++ )
		{
			// Display all text in the buffer.
			INT Line = (TopLine + MAX_LINES*2 - (i-1)) % MAX_LINES;

			INT XL,YL;
			Viewport->Canvas->WrappedStrLen( Viewport->Canvas->MedFont, XL, YL, Frame->X-8, MsgText[Line] );

			// Half-space blank lines.
			if( YL == 0 )
				YL = 5;

			Y -= YL;
			if( (Y+YL)<0 )
				break;
			Viewport->Canvas->CurX = 4;
			Viewport->Canvas->CurY = Y;
			Viewport->Canvas->WrappedPrintf( Viewport->Canvas->MedFont, 0, "%s", MsgText[Line] );
		}
	}
	else
	{
		// Console is hidden; display single-line view.
		if( TextLines>0 && MsgType!=NAME_None && (!Viewport->Actor->bShowMenu || Viewport->Actor->bShowScores) )
		{
			int iLine=TopLine;
			int i;
			for( i=0; i<NumLines; i++ )
			{
				if( *MsgText[iLine] )
					break;
				iLine = (iLine-1+MAX_LINES)%MAX_LINES;
			}
			Viewport->Canvas->CurX = 4;
			Viewport->Canvas->CurY = 2;
			Viewport->Canvas->WrappedPrintf( Viewport->Canvas->MedFont, 1, "%s", MsgText[iLine] );
			if ( TextLines > 1 )
			{
				iLine = (iLine-1+MAX_LINES)%MAX_LINES;
				for ( int j=0; j<i; j++ )
				{
					if( *MsgText[iLine] )
						break;
					iLine = (iLine-1+MAX_LINES)%MAX_LINES;
				}
				Viewport->Canvas->CurY = 12;
				Viewport->Canvas->WrappedPrintf( Viewport->Canvas->MedFont, 1, "%s", MsgText[iLine] );
			}
		}
		if( GetMainFrame()->Node && GetMainFrame()->Node->GetFName()=="Typing" )
		{
			// Draw stuff being typed.
			int XL,YL;
			char S[256];
			appSprintf( S, "(> %s_", TypedStr );
			Viewport->Canvas->WrappedStrLen( Viewport->Canvas->MedFont, XL, YL, Frame->X-8, S );
			Viewport->Canvas->CurX = 2;
			Viewport->Canvas->CurY = Frame->Y - ConsoleLines - YL - 1;
			Viewport->Canvas->WrappedPrintf( Viewport->Canvas->MedFont, 0, "%s", S );
		}
	}
#ifdef PLATFORM_ANDROID
	UE1AndroidDrawVersionAbiOverlay( Viewport );
#endif
	unguard;
}

/*------------------------------------------------------------------------------
	The End.
------------------------------------------------------------------------------*/
