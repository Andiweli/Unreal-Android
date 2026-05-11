/*=============================================================================
	UnConfig.h: INI file reading and writing
	Based on FConfigCacheIni from the public v226 headers.
=============================================================================*/

/*-----------------------------------------------------------------------------
	Definitions.
-----------------------------------------------------------------------------*/

#define MAX_INI_KEY 255
#define MAX_INI_VAL 255
#define MAX_INI_LINE 1023
#define MAX_INI_NAME 1023

/*-----------------------------------------------------------------------------
	FConfigFile.
-----------------------------------------------------------------------------*/

class FConfigFile
{
public:
	char Filename[MAX_INI_NAME + 1];
	UBOOL Dirty = 0;
	UBOOL NoSave = 0;

	FConfigFile( const char* InFilename )
	{
		appStrncpy( Filename, InFilename, MAX_INI_NAME );
	}

	~FConfigFile()
	{
		for( INT i = 0; i < Sections.Num(); ++i )
			delete Sections(i);
		Sections.Empty();
	}

	UBOOL Read( const char* InFilename = NULL );
	UBOOL Write( const char* InFilename = NULL );

	UBOOL GetString( const char* Section, const char* Key, char* Out, INT OutLen );
	UBOOL GetSection( const char* Section, char* Out, INT OutLen );
	UBOOL SetString( const char* Section, const char *Key, const char* Val );

protected:
	struct FKeyValue
	{
		char Key[MAX_INI_KEY + 1] = "";
		char Val[MAX_INI_VAL + 1] = "";
		FKeyValue( const char* InKey, const char* InVal )
		{
			appStrncpy( Key, InKey, MAX_INI_KEY );
			appStrncpy( Val, InVal, MAX_INI_VAL );
		}
	};

	struct FSection
	{
		char Name[MAX_INI_KEY + 1] = "";
		TArray<FKeyValue> KeyValues;
		FSection( const char* InName ) : KeyValues()
		{
			appStrncpy( Name, InName, MAX_INI_KEY  );
		}
		inline FKeyValue* AddKeyValue( const char* Key, const char* Val )
		{
			guard(FSection::AddKeyValue)
			INT i = KeyValues.AddItem( FKeyValue( Key, Val ) );
			return &KeyValues(i);
			unguard;
		}
		inline FKeyValue* FindKeyValue( const char* Key )
		{
			guard(FSection::FindKeyValue)
			for( INT i = 0; i < KeyValues.Num(); ++i )
				if( !appStricmp( KeyValues(i).Key, Key ) )
					return &KeyValues(i);
			return NULL;
			unguard;
		}
	};

	TArray<FSection*> Sections;

	inline FSection* AddSection( const char* Name )
	{
		guard(FConfigFile::AddSection)
		INT i = Sections.AddItem( new FSection( Name ) );
		return Sections(i);
		unguard;
	}

	inline FSection* FindSection( const char* Name )
	{
		guard(FConfigFile::FindSection)
		for( INT i = 0; i < Sections.Num(); ++i )
			if( !appStricmp( Sections(i)->Name, Name ) )
				return Sections(i);
		return NULL;
		unguard;
	}

	inline FKeyValue* FindKeyValue( const char* Sec, const char* Key )
	{
		FSection* SecPtr = FindSection( Sec );
		if( SecPtr )
			return SecPtr->FindKeyValue( Key );
		return NULL;
	}

	inline FKeyValue* AddKeyValue( const char* Sec, const char* Key, const char* Val )
	{
		FSection* SecPtr = FindSection( Sec );
		if( !SecPtr )
			SecPtr = AddSection( Sec );
		if( SecPtr )
			return SecPtr->AddKeyValue( Key, Val );
		return NULL;
	}
};

/*-----------------------------------------------------------------------------
	FConfigCache.
-----------------------------------------------------------------------------*/

class FConfigCache
{
public:
	~FConfigCache()
	{
		Exit();
	}

	void Init( const char* InDefaultIni );
	void Exit();

	UBOOL GetString( const char* Section, const char* Key, char* Out, INT OutLen, const char* Filename = NULL );
	UBOOL GetSection( const char* Section, char* Out, INT OutLen, const char* Filename = NULL );
	UBOOL SetString( const char* Section, const char *Key, const char* Val, const char* Filename = NULL );

	FConfigFile* FindConfig( const char* Filename, UBOOL CreateIfNotFound );

	UBOOL SaveAllConfigs();

protected:
	char DefaultIni[MAX_INI_NAME + 1];
	TArray<FConfigFile*> Configs;
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
