// Copyright (C) 2000-2002 Raven Software, Inc.
//
#include "../win32/AutoVersion.h"

// Current version of the multi player game
#ifdef _DEBUG
	#define	Q3_VERSION		"(debug)Open Battlefront Project: v"VERSION_STRING_DOTTED
#elif defined FINAL_BUILD
	#define	Q3_VERSION		"Open Battlefront Project: v"VERSION_STRING_DOTTED
#else
	#define	Q3_VERSION		"(internal)Open Battlefront Project: v"VERSION_STRING_DOTTED
#endif

//end
