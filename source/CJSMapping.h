#ifndef __CJSMAPPING_H__
#define __CJSMAPPING_H__

namespace UOX
{

	enum SCRIPTTYPE
	{
		SCPT_NORMAL		= 0,
		SCPT_COMMAND,
		SCPT_MAGIC,
		SCPT_SKILLUSE,
		SCPT_COUNT
	};

	const std::string ScriptNames[SCPT_COUNT] = { "SCRIPT_LIST", "COMMAND_SCRIPTS", "MAGIC_SCRIPTS", "SKILLUSE_SCRIPTS" };

	class CJSMappingSection
	{
	private:
		std::map< UI16, cScript * >				scriptIDMap;
		std::map< JSObject *, UI16 >			scriptJSMap;

		std::map< UI16, cScript * >::iterator	scriptIDIter;

		SCRIPTTYPE								scriptType;
	public:
					CJSMappingSection( SCRIPTTYPE sT );
					~CJSMappingSection();


		void		Reload( UI16 toLoad );
		void		Parse( Script *fileAssocData );

		bool		IsInMap( UI16 scriptID );

		UI16		GetScriptID( JSObject *toFind );
		cScript *	GetScript( UI16 toFind );
		cScript *	GetScript( JSObject *toFind );

		cScript *	First( void );
		cScript *	Next( void );
		bool		Finished( void );
	};

	class CJSMapping
	{
	private:
		CJSMappingSection *				mapSection[SCPT_COUNT];

		CEnvoke *						envokeByID;
		CEnvoke *						envokeByType;

		void				ResetDefaults( void );
		void				Parse( void );

	public:
							CJSMapping( void );
							~CJSMapping();

		void				Cleanup( void );
		void				Reload( UI16 scriptID = 0xFFFF );

		CJSMappingSection *	GetSection( SCRIPTTYPE toGet );

		UI16				GetScriptID( JSObject *toFind );

		cScript *			GetScript( UI16 toFind );
		cScript *			GetScript( JSObject *toFind );

		CEnvoke *			GetEnvokeByID( void );
		CEnvoke *			GetEnvokeByType( void );
	};

	class CEnvoke
	{
	private:
		std::map< UI16, UI16 >	envokeList;
		std::string				envokeType;

	public:
				CEnvoke( std::string envokeType );
				~CEnvoke();

		void	Parse( void );
		bool	Check( UI16 envokeID ) const;
		UI16	GetScript( UI16 envokeID ) const;
	};

	extern CJSMapping *JSMapping;

}

#endif  // __CJSMAPPING_H__