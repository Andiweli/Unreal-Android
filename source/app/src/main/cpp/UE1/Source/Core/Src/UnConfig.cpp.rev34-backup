/*=============================================================================
	UnConfig.cpp: Config cache implementation.
=============================================================================*/

#include "CorePrivate.h"

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

	Dirty = false;
	FString Text;
	for( TIterator<FSection*> It(Sections); It; ++It )
	{
		Text.Appendf( "[%s]\r\n", It.GetCurrent()->Name );
		for( TIterator<FKeyValue> It2(It.GetCurrent()->KeyValues); It2; ++It2 )
			Text.Appendf( "%s=%s\r\n", It2.GetCurrent().Key, It2.GetCurrent().Val );
		Text.Appendf( "\r\n" );
	}

	return appSaveStringToFile( Text, InFilename );

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
	if( Cfg )
		return Cfg->SetString( Section, Key, Val );

	return false;

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
