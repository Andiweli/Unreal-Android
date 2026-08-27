/*=============================================================================
	UnConfig.cpp: Config cache implementation.
=============================================================================*/

#include "CorePrivate.h"

#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
#include <android/log.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int GUE1AndroidConfigAutosaveGuard = 0;

static const char* UE1AndroidConfigBaseName( const char* Filename )
{
	const char* Base = Filename ? Filename : "";
	for( const char* P = Base; *P; ++P )
	{
		if( *P == '/' || *P == '\\' || *P == ':' )
			Base = P + 1;
	}
	return Base;
}

static UBOOL UE1AndroidConfigHasIniExtension( const char* Filename )
{
	const char* Base = UE1AndroidConfigBaseName( Filename );
	const char* Dot = strrchr( Base, '.' );
	return Dot && appStricmp( Dot, ".ini" ) == 0;
}

static UBOOL UE1AndroidConfigIsAbsolutePath( const char* Filename )
{
	return Filename && ( Filename[0] == '/' || ( Filename[0] && Filename[1] == ':' ) );
}

static const char* UE1AndroidRemapConfigFilename( const char* Filename, char* Out, size_t OutSize, UBOOL ForWrite )
{
	if( !Filename || !Filename[0] || OutSize < 16 )
		return Filename;

	if( !UE1AndroidConfigHasIniExtension( Filename ) )
		return Filename;

	if( UE1AndroidConfigIsAbsolutePath( Filename ) )
		return Filename;

	snprintf( Out, OutSize, "%s%s", appAndroidUnrealSystemDir(), UE1AndroidConfigBaseName( Filename ) );
	Out[OutSize - 1] = 0;

	__android_log_print( ANDROID_LOG_INFO, "UE1Config", "Config %s path: %s -> %s", ForWrite ? "save" : "load", Filename, Out );

	return Out;
}
#endif

/*-----------------------------------------------------------------------------
	Global variables.
-----------------------------------------------------------------------------*/

CORE_API FConfigCache GConfigCache;

/*-----------------------------------------------------------------------------
	FConfigFile.
-----------------------------------------------------------------------------*/

UBOOL FConfigFile::Read( const char* InFilename )
{
	guard(FConfigFile::Read);

	if( !InFilename )
		InFilename = Filename;

#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
	char AndroidConfigPath[4096];
	InFilename = UE1AndroidRemapConfigFilename( InFilename, AndroidConfigPath, sizeof(AndroidConfigPath), false );
#endif

	FString Text;
	if( appLoadFileToString( Text, InFilename ? InFilename : Filename ) )
	{
		char* Ptr = &Text.GetCharArray()(0);
		FSection* CurrentSection = NULL;
		UBOOL Done = 0;
		while( !Done )
		{
			while( *Ptr=='\r' || *Ptr=='\n' )
				Ptr++;
			char* Start = Ptr;
			while( *Ptr && *Ptr!='\r' && *Ptr!='\n' )
				Ptr++;
			if( *Ptr==0 )
				Done = 1;
			*Ptr++ = 0;
			if( *Start=='[' && Start[appStrlen(Start)-1]==']' )
			{
				Start++;
				Start[appStrlen(Start)-1] = 0;
				CurrentSection = FindSection( Start );
				if( !CurrentSection )
					CurrentSection = AddSection( Start );
			}
			else if( CurrentSection && *Start )
			{
				char* Value = appStrchr(Start,'=');
				if( Value )
				{
					*Value++ = 0;
					if( *Value=='\"' && Value[appStrlen(Value)-1]=='\"' )
					{
						Value++;
						Value[appStrlen(Value)-1]=0;
					}
					CurrentSection->AddKeyValue( Start, Value );
				}
			}
		}
		return true;
	}

	return false;

	unguard;
}


UBOOL FConfigFile::Write( const char* InFilename )
{
	guard(FConfigFile::Write);

	if( !Dirty || NoSave )
		return true;

	if( !InFilename )
		InFilename = Filename;

#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
	char AndroidConfigPath[4096];
	InFilename = UE1AndroidRemapConfigFilename( InFilename, AndroidConfigPath, sizeof(AndroidConfigPath), true );
#endif

	Dirty = false;
	FString Text;
	for( TIterator<FSection*> It(Sections); It; ++It )
	{
		Text.Appendf( "[%s]\r\n", It.GetCurrent()->Name );
		for( TIterator<FKeyValue> It2(It.GetCurrent()->KeyValues); It2; ++It2 )
			Text.Appendf( "%s=%s\r\n", It2.GetCurrent().Key, It2.GetCurrent().Val );
		Text.Appendf( "\r\n" );
	}

	UBOOL Ok = appSaveStringToFile( Text, InFilename );

#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
	__android_log_print( ANDROID_LOG_INFO, "UE1Config", "FConfigFile::Write %s: %s", Ok ? "OK" : "FAILED", InFilename ? InFilename : "(null)" );
#endif

	return Ok;

	unguard;
}


UBOOL FConfigFile::GetString( const char* Section, const char* Key, char* Out, INT OutLen )
{
	guard(FConfigFile::GetString);

	if( !OutLen )
		return false;

	*Out = '\0';

	FKeyValue* KeyVal = FindKeyValue( Section, Key );
	if( KeyVal )
	{
		appStrncpy( Out, KeyVal->Val, OutLen - 1 );
		return true;
	}

	return false;

	unguard;
}

UBOOL FConfigFile::GetSection( const char* Section, char* Out, INT OutLen )
{
	guard(FConfigFile::GetSection);

	if( !OutLen )
		return false;

	*Out = '\0';

	FSection* Sec = FindSection( Section );
	if( !Sec )
		return false;

	char* End = Out;
	for( TIterator<FKeyValue> It(Sec->KeyValues); It; ++It )
	{
		const char* Key = It.GetCurrent().Key;
		if( ( End - Out ) + appStrlen( Key ) + 1 >= OutLen )
			break;
		End += appSprintf( End, "%s=%s", Key, It.GetCurrent().Val ) + 1;
	}

	*End++ = '\0';

	return true;

	unguard;
}

UBOOL FConfigFile::SetString( const char* Section, const char *Key, const char* Val )
{
	guard(FConfigFile::SetString);

	FKeyValue* KeyVal = FindKeyValue( Section, Key );
	if( KeyVal )
	{
		if( appStrcmp( KeyVal->Val, Val ) != 0 )
			Dirty = true;
		appStrncpy( KeyVal->Val, Val, MAX_INI_VAL );
		return true;
	}

	if( AddKeyValue( Section, Key, Val ) )
	{
		Dirty = true;
		return true;
	}

	return false;

	unguard;
}

/*-----------------------------------------------------------------------------
	FConfigCache.
-----------------------------------------------------------------------------*/

void FConfigCache::Init( const char* InDefaultIni )
{
	guard(FConfigCache::Init);

	appStrncpy( DefaultIni, InDefaultIni, MAX_INI_NAME );

	unguard;
}

void FConfigCache::Exit()
{
#if defined(PLATFORM_ANDROID) || defined(__ANDROID__)
// Android shutdown path: force-write all cached configs.
    if( !GUE1AndroidConfigAutosaveGuard )
    {
        GUE1AndroidConfigAutosaveGuard = 1;
        __android_log_print(ANDROID_LOG_INFO, "UE1Config", "FConfigCache::Exit forcing SaveAllConfigs");
        SaveAllConfigs();
        GUE1AndroidConfigAutosaveGuard = 0;
    }
#endif

	guard(FConfigCache::Exit);

	for( TIterator<FConfigFile*> It(Configs); It; ++It )
	{
		It.GetCurrent()->Write();
		delete It.GetCurrent();
	}

	Configs.Empty();

	unguard;
}

UBOOL FConfigCache::GetString( const char* Section, const char* Key, char* Out, INT OutLen, const char* Filename )
{
	guard(FConfigCache::GetString);

	*Out = '\0';

	FConfigFile* Cfg = FindConfig( Filename, false );
	if( Cfg )
		return Cfg->GetString( Section, Key, Out, OutLen );

	return false;

	unguard;
}

UBOOL FConfigCache::GetSection( const char* Section, char* Out, INT OutLen, const char* Filename )
{
	guard(FConfigCache::GetSection);

	*Out = '\0';

	FConfigFile* Cfg = FindConfig( Filename, false );
	if( Cfg )
		return Cfg->GetSection( Section, Out, OutLen );

	return false;

	unguard;
}

UBOOL FConfigCache::SetString( const char* Section, const char *Key, const char* Val, const char* Filename )
{
	guard(FConfigCache::SetString);

	FConfigFile* Cfg = FindConfig( Filename, true );
	UBOOL Result = false;

	if( Cfg )
		Result = Cfg->SetString( Section, Key, Val );

#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
	// Android can kill or background the process without the classic desktop shutdown path.
	// Save the touched ini immediately so Options/Preferences changes survive even abrupt exits.
	if( Result && Cfg && !GUE1AndroidConfigAutosaveGuard )
	{
		GUE1AndroidConfigAutosaveGuard = 1;
		__android_log_print( ANDROID_LOG_INFO, "UE1Config", "Autosave touched config: %s", Cfg->Filename );
		Cfg->Write( Cfg->Filename );
		GUE1AndroidConfigAutosaveGuard = 0;
	}
#endif

	return Result;

	unguard;
}


FConfigFile* FConfigCache::FindConfig( const char* InFilename, UBOOL CreateIfNotFound )
{
	guard(FConfigCache::FindConfig);

	// If filename not specified, use default.
	char Filename[MAX_INI_NAME + 1];
	appStrncpy( Filename, InFilename ? InFilename : DefaultIni, MAX_INI_NAME );

	// Add .ini extension.
	INT Len = appStrlen(Filename);
	if( Len < 5 || ( Filename[Len - 4] != '.' && Filename[Len - 5] != '.' ) )
		appStrcat( Filename, ".ini" );

	// Get file.
	FConfigFile* Cfg = NULL;
	for( INT i = 0; i < Configs.Num(); ++i )
	{
		if( !appStricmp( Configs(i)->Filename, Filename ) )
		{
			Cfg = Configs(i);
			break;
		}
	}

	// Create if we don't have it cached yet.
	if( !Cfg && ( CreateIfNotFound || appFSize( Filename ) >= 0 ) )
	{
		INT i = Configs.AddItem( new FConfigFile( Filename ) );
		Cfg = Configs(i);
		Cfg->Read( Filename );
	}

	return Cfg;

	unguard;
}

UBOOL FConfigCache::SaveAllConfigs()
{
#if defined(PLATFORM_ANDROID) || defined(__ANDROID__)
// Trace every global config save attempt on Android.
    __android_log_print(ANDROID_LOG_INFO, "UE1Config", "FConfigCache::SaveAllConfigs called");
#endif

	guard(FConfigCache::SaveAllConfigs);
	UBOOL Ret = true;
	for( TIterator<FConfigFile*> It(Configs); It; ++It )
	{
		if ( !It.GetCurrent()->Write() )
			Ret = false;
	}
	return Ret;
	unguard;
}

/*-----------------------------------------------------------------------------
   The End.
 -----------------------------------------------------------------------------*/
