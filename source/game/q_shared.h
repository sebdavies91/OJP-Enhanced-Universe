// Copyright (C) 1999-2000 Id Software, Inc.
//
#ifndef __Q_SHARED_H__
#define __Q_SHARED_H__

// q_shared.h -- included first by ALL program modules.
// A user mod should never modify this file

//NOTENOTE: Only change this to re-point ICARUS to a new script directory
#define Q3_SCRIPT_DIR	"scripts"

#define MAX_TEAMNAME 32

//[SaberLockSys]
//moved here to support ShortestLineSegBewteen2LineSegs
#define Q3_INFINITE			16777216 
//[/SaberLockSys]

#include "../qcommon/disablewarnings.h"

#include "teams.h" //npc team stuff

#define MAX_WORLD_COORD		( 64 * 1024 )
#define MIN_WORLD_COORD		( -64 * 1024 )
#define WORLD_SIZE			( MAX_WORLD_COORD - MIN_WORLD_COORD )

//Pointer safety utilities
#define VALID( a )		( a != NULL )
#define	VALIDATE( a )	( assert( a ) )

#define	VALIDATEV( a )	if ( a == NULL ) {	assert(0);	return;			}
#define	VALIDATEB( a )	if ( a == NULL ) {	assert(0);	return qfalse;	}
#define VALIDATEP( a )	if ( a == NULL ) {	assert(0);	return NULL;	}

#define VALIDSTRING( a )	( ( a != 0 ) && ( a[0] != 0 ) )

/*
#define G2_EHNANCEMENTS

#ifdef G2_EHNANCEMENTS
//these two will probably explode if they're defined independant of one another.
//rww - RAGDOLL_BEGIN
#define JK2_RAGDOLL
//rww - RAGDOLL_END
//rww - Bone cache for multiplayer base.
#define MP_BONECACHE
#endif
*/

#ifndef FINAL_BUILD
#define G2_PERFORMANCE_ANALYSIS
#define _FULL_G2_LEAK_CHECKING
extern int g_Ghoul2Allocations;
extern int g_G2ServerAlloc;
extern int g_G2ClientAlloc;
extern int g_G2AllocServer;
#endif

#if defined __LCC__

#define Q_INLINE
#define QDECL
#define Q_EXPORT
#define Q_NORETURN
#define Q_PTR_NORETURN
#define Q_UNUSED
#define Q_UNREACHABLE()
#define __attribute__(x)

#elif defined _MSC_VER							// Microsoft Visual C++

#define Q_INLINE __inline
#define	QDECL	__cdecl
#define Q_EXPORT __declspec(dllexport)
#define Q_NORETURN __declspec(noreturn)
#define Q_PTR_NORETURN // MSVC doesn't support noreturn function pointers
#define Q_UNUSED
#define Q_UNREACHABLE() abort()
#define __attribute__(x)

#elif (defined __GNUC__ || defined __clang__)	// GCC & Clang

#define Q_INLINE inline
#define QDECL
#define Q_EXPORT __attribute__((visibility("default")))
#define Q_NORETURN __attribute__((noreturn))
#define Q_PTR_NORETURN Q_NORETURN
#define Q_UNUSED __attribute__((unused))
#define Q_UNREACHABLE() __builtin_unreachable()

#else											// Any ANSI C compiler

#define Q_INLINE inline
#define QDECL
#define Q_EXPORT
#define Q_NORETURN
#define Q_PTR_NORETURN
#define Q_UNUSED
#define Q_UNREACHABLE() abort()
#define __attribute__(x)

#endif

#ifdef __cplusplus
#define ID_INLINE Q_INLINE
#else
#define ID_INLINE static Q_INLINE
#endif

/**********************************************************************
  VM Considerations

  The VM can not use the standard system headers because we aren't really
  using the compiler they were meant for.  We use bg_lib.h which contains
  prototypes for the functions we define for our own use in bg_lib.c.

  When writing mods, please add needed headers HERE, do not start including
  stuff like <stdio.h> in the various .c files that make up each of the VMs
  since you will be including system headers files can will have issues.

  Remember, if you use a C library function that is not defined in bg_lib.c,
  you will have to add your own version for support in the VM.

 **********************************************************************/

#ifdef Q3_VM

#include "bg_lib.h"

#define assert(exp)     ((void)0)

#else

#include <assert.h>
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include <stddef.h>

#if defined (_MSC_VER)
	#if _MSC_VER >= 1600
		#include <stdint.h>
	#else
		typedef signed __int64 int64_t;
		typedef signed __int32 int32_t;
		typedef signed __int16 int16_t;
		typedef signed __int8  int8_t;
		typedef unsigned __int64 uint64_t;
		typedef unsigned __int32 uint32_t;
		typedef unsigned __int16 uint16_t;
		typedef unsigned __int8  uint8_t;
	#endif
#else // not using MSVC
	#if !defined(__STDC_LIMIT_MACROS)
		#define __STDC_LIMIT_MACROS
	#endif
	#include <stdint.h>
#endif

#endif

// this is the define for determining if we have an asm version of a C function
#if defined(ARCH_X86)  && !defined(Q3_VM)
#define id386	1
#else
#define id386	0
#endif

#if defined(ARCH_X86_64)  && !defined(Q3_VM)
#define idx64	1
#else
#define idx64	0
#endif

#if defined(ARCH_ARM32) && !defined(Q3_VM)
#define idarm32	1
#else
#define idarm32	0
#endif

#if defined(ARCH_ARM64) && !defined(Q3_VM)
#define idarm64	1
#else
#define idarm64	0
#endif

#if (defined(powerc) || defined(powerpc) || defined(ppc) || defined(__ppc) || defined(__ppc__)) && !defined(Q3_VM)
#define idppc	1
#else
#define idppc	0
#endif

short   ShortSwap (short l);
int		LongSwap (int l);
float	FloatSwap (const float *f);

//======================= WIN32 DEFINES =================================

#ifdef WIN32

#define	PATH_SEP '\\'

#endif

//======================= MAC OS X DEFINES =====================

#if defined(MACOS_X)

#define	PATH_SEP	'/'

#endif

//======================= LINUX DEFINES =================================

#ifdef __linux__

#define	PATH_SEP '/'

#endif

//======================= FreeBSD DEFINES =====================
#ifdef __FreeBSD__ // rb010123

#define	PATH_SEP '/'

#endif

//=============================================================

#if id386
#define Q_LITTLE_ENDIAN
#elif idx64
#define Q_LITTLE_ENDIAN
#elif idarm32
#define Q_LITTLE_ENDIAN
#elif idarm64
#define Q_LITTLE_ENDIAN
#elif idppc
#define Q_BIG_ENDIAN
#else
#error "Architecture not supported"
#endif

#if defined(Q_LITTLE_ENDIAN)
#define BigShort(x) ShortSwap(x)
#define BigLong(x) LongSwap(x)
#define BigFloat(x) FloatSwap(x)
#define LittleShort
#define LittleLong
#define LittleFloat
#endif

#if defined(Q_BIG_ENDIAN)
#define LittleShort(x) ShortSwap(x)
#define LittleLong(x) LongSwap(x)
#define LittleFloat(x) FloatSwap(x)
#define BigShort
#define BigLong
#define BigFloat
#endif

//=============================================================

typedef unsigned char 		byte;
typedef unsigned short		word;
typedef unsigned long		ulong;

typedef enum {qfalse, qtrue}	qboolean;

typedef int		qhandle_t;
typedef int		thandle_t; //rwwRMG - inserted
typedef int		fxHandle_t;
typedef int		sfxHandle_t;
typedef int		fileHandle_t;
typedef int		clipHandle_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

#define	MAX_QINT			0x7fffffff
#define	MIN_QINT			(-MAX_QINT-1)


// angle indexes
#define	PITCH				0		// up / down
#define	YAW					1		// left / right
#define	ROLL				2		// fall over

// the game guarantees that no string from the network will ever
// exceed MAX_STRING_CHARS
#define	MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString
#define	MAX_STRING_TOKENS	1024	// max tokens resulting from Cmd_TokenizeString
#define	MAX_TOKEN_CHARS		1024	// max length of an individual token

#define	MAX_INFO_STRING		1024
#define	MAX_INFO_KEY		1024
#define	MAX_INFO_VALUE		1024

#define	BIG_INFO_STRING		8192  // used for system info key only
#define	BIG_INFO_KEY		  8192
#define	BIG_INFO_VALUE		8192

#define MAX_ADDRESSLENGTH		256//64
#define MAX_HOSTNAMELENGTH		256//22
#define MAX_MAPNAMELENGTH		256//16
#define MAX_STATUSLENGTH		256//64


#define	MAX_MODELPATH		256 //SERENITY  max length of a EoC game model pathname
#define	MAX_QPATH			64		// max length of a quake game pathname
#ifdef PATH_MAX
#define MAX_OSPATH			PATH_MAX
#else
#define	MAX_OSPATH			256		// max length of a filesystem pathname
#endif

#define	MAX_NAME_LENGTH		32		// max length of a client name

#define	MAX_SAY_TEXT	150

// paramters for command buffer stuffing
typedef enum {
	EXEC_NOW,			// don't return until completed, a VM should NEVER use this,
						// because some commands might cause the VM to be unloaded...
	EXEC_INSERT,		// insert at current position, but don't run yet
	EXEC_APPEND			// add to end of the command buffer (normal case)
} cbufExec_t;

#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))
// todo, ctrl, pad, padlen, padp, array_len ?

//
// these aren't needed by any of the VMs.  put in another header?
//
#define	MAX_MAP_AREA_BYTES		32		// bit vector of area visibility


#define LS_STYLES_START			0
#define LS_NUM_STYLES			32
#define	LS_SWITCH_START			(LS_STYLES_START+LS_NUM_STYLES)
#define LS_NUM_SWITCH			32
#if !defined MAX_LIGHT_STYLES
#define MAX_LIGHT_STYLES		64
#endif

//For system-wide prints
enum WL_e {
	WL_ERROR=1,
	WL_WARNING,
	WL_VERBOSE,
	WL_DEBUG
};

extern float forceSpeedLevels[4];

// print levels from renderer (FIXME: set up for game / cgame?)
typedef enum {
	PRINT_ALL,
	PRINT_DEVELOPER,		// only print when "developer 1"
	PRINT_WARNING,
	PRINT_ERROR
} printParm_t;


#ifdef ERR_FATAL
#undef ERR_FATAL			// this is be defined in malloc.h
#endif

// parameters to the main Error routine
typedef enum {
	ERR_FATAL,					// exit the entire game with a popup window
	ERR_DROP,					// print to console and disconnect from game
	ERR_SERVERDISCONNECT,		// don't kill server
	ERR_DISCONNECT,				// client disconnected from the server
	ERR_NEED_CD					// pop up the need-cd dialog
} errorParm_t;


// font rendering values used by ui and cgame

/*#define PROP_GAP_WIDTH			3
#define PROP_SPACE_WIDTH		8
#define PROP_HEIGHT				27
#define PROP_SMALL_SIZE_SCALE	0.75*/

#define PROP_GAP_WIDTH			2
//#define PROP_GAP_WIDTH			3
#define PROP_SPACE_WIDTH		4
#define PROP_HEIGHT				16

#define PROP_TINY_SIZE_SCALE	1
#define PROP_SMALL_SIZE_SCALE	1
#define PROP_BIG_SIZE_SCALE		1
#define PROP_GIANT_SIZE_SCALE	2

#define PROP_TINY_HEIGHT		10
#define PROP_GAP_TINY_WIDTH		1
#define PROP_SPACE_TINY_WIDTH	3

#define PROP_BIG_HEIGHT			24
#define PROP_GAP_BIG_WIDTH		3
#define PROP_SPACE_BIG_WIDTH	6

#define BLINK_DIVISOR			200
#define PULSE_DIVISOR			75

#define UI_LEFT			0x00000000	// default
#define UI_CENTER		0x00000001
#define UI_RIGHT		0x00000002
#define UI_FORMATMASK	0x00000007
#define UI_SMALLFONT	0x00000010
#define UI_BIGFONT		0x00000020	// default
//#define UI_GIANTFONT	0x00000040
#define UI_DROPSHADOW	0x00000800
#define UI_BLINK		0x00001000
#define UI_INVERSE		0x00002000
#define UI_PULSE		0x00004000

#if defined(_DEBUG) && !defined(BSPC)
	#define HUNK_DEBUG
#endif

typedef enum {
	h_high,
	h_low,
	h_dontcare
} ha_pref;

void *Hunk_Alloc( int size, ha_pref preference );

void Com_Memset (void* dest, const int val, const size_t count);
void Com_Memcpy (void* dest, const void* src, const size_t count);

#define CIN_system	1
#define CIN_loop	2
#define	CIN_hold	4
#define CIN_silent	8
#define CIN_shader	16

/*
==============================================================

MATHLIB

==============================================================
*/


typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef vec_t vec5_t[5];

//rwwRMG - new vec types
typedef vec3_t	vec3pair_t[2];

typedef int ivec3_t[3];
typedef int ivec4_t[4];
typedef int ivec5_t[5];

typedef	int	fixed4_t;
typedef	int	fixed8_t;
typedef	int	fixed16_t;

#ifndef M_PI
#define M_PI		3.14159265358979323846f	// matches value in gcc v2 math.h
#endif


typedef enum {
	BLK_NO,
	BLK_TIGHT,		// Block only attacks and shots around the saber itself, a bbox of around 12x12x12
	BLK_WIDE		// Block all attacks in an area around the player in a rough arc of 180 degrees
} saberBlockType_t;

typedef enum {
	BLOCKED_NONE,
	BLOCKED_BOUNCE_MOVE,	//[RACC] This is used for situations where you want to manually 
							//set a animation (normally deflections.)  To use, set your 
							//sabermove to the move you want and then set this saberBlocked
							//flag.  The animation will be set with the correct timers and
							//weapontime.
	BLOCKED_PARRY_BROKEN,
	BLOCKED_ATK_BOUNCE,
	BLOCKED_UPPER_RIGHT,
	BLOCKED_UPPER_LEFT,
	BLOCKED_LOWER_RIGHT,
	BLOCKED_LOWER_LEFT,
	BLOCKED_TOP,
	BLOCKED_UPPER_RIGHT_PROJ,
	BLOCKED_UPPER_LEFT_PROJ,
	BLOCKED_LOWER_RIGHT_PROJ,
	BLOCKED_LOWER_LEFT_PROJ,
	BLOCKED_TOP_PROJ,
	BLOCKED_BACK
} saberBlockedType_t;


typedef enum
{
	SABER_RED,
	SABER_ORANGE,
	SABER_YELLOW,
	SABER_GREEN,
	SABER_BLUE,
	SABER_PURPLE,
	SABER_CYAN,
	//[RGBSabers]
	SABER_WHITE,
	SABER_BLACK,
	SABER_RGB,
	SABER_PIMP,
	SABER_SCRIPTED,
	//[/RGBSabers]
	NUM_SABER_COLORS

} saber_colors_t;

typedef enum
{
	FP_FIRST = 0,//marker
	FP_HEAL = 0,//instant
	FP_LEVITATION,//hold/duration
	FP_SPEED,//duration
	FP_PUSH,//hold/duration
	FP_PULL,//hold/duration
	FP_TELEPATHY,//instant
	FP_GRIP,//hold/duration
	FP_LIGHTNING,//hold/duration
	FP_RAGE,//duration
	FP_PROTECT,
	FP_ABSORB,
	FP_TEAM_HEAL,
	FP_TEAM_FORCE,
	FP_DRAIN,
	FP_SEE,
	FP_SABER_OFFENSE,
	FP_SABER_DEFENSE,
	FP_SABERTHROW,
	NUM_FORCE_POWERS
} forcePowers_t;

//[ExpSys]
typedef enum
{
	SK_JETPACK,		//jetpack skill
	SK_PISTOL,		//blaster pistol
	SK_BLASTER,		//blaster rifle skill
	SK_THERMAL,		//thermal detenator skill
	SK_ROCKET,		//rocket launcher skill
	SK_BACTA,		//bacta tank skill
	SK_FLAMETHROWER,//flamethrower skill
	SK_BOWCASTER,	//bowcaster skill
	SK_FORCEFIELD,	//forcefield skill
	SK_CLOAK,		//cloaking device skill
	SK_SEEKER,		//seeker droid skill
	SK_SENTRY,		//sentry gun skill
	SK_DETPACK,		//detpack skill
	SK_REPEATER,	//Repeater/clone Rifle skill
	SK_DISRUPTOR,	//Disruptor/sniper rifle skill
	//[StanceSelection]
	SK_BLUESTYLE,	//Yellow lightsaber style
	SK_REDSTYLE,	//Red lightsaber style
	SK_PURPLESTYLE,	//Purple lightsaber style
	SK_GREENSTYLE,	//Green lightsaber style
	SK_DUALSTYLE,	//Dual lightsaber style
	SK_STAFFSTYLE,	//Staff lightsaber style
	//[/StanceSelection]
	SK_FLECHETTE,//[Flechette]
	SK_TRIPMINE,			//trip mine
	SK_DEMP2,			//Concussion Rifle
	SK_CONCUSSION,			//Concussion Rifle
	SK_OLD,			//old blaster pistol
	SK_EWEB,		//EWEB cannon
	SK_BINOCULARS,		//Binoculars
	SK_WRIST,			//Wrist
	SK_HEALTH,			//Health
	SK_SHIELDS,			//Shields
	SK_REPAIR,//shieldbooster skill
	SK_ELECTROSHOCKER,//Electroshocker skill
	SK_SPHERESHIELD,		//sphere shield device skill	
	SK_OVERLOAD,		//overload device skill
	SK_SQUADTEAM,
	SK_SQUADTEAMA,
	SK_SQUADTEAMB,	
	SK_SQUADTEAMC, 
	SK_VEHICLEMOUNT,
	SK_LIGHTVEHICLEA,
	SK_MEDIUMVEHICLEA,
	SK_HEAVYVEHICLEA,
	SK_FIGHTERSHIPA,
	SK_BOMBERSHIPA,
	SK_TRANSPORTSHIPA,
	SK_LIGHTVEHICLEB,
	SK_MEDIUMVEHICLEB,
	SK_HEAVYVEHICLEB,	
	SK_FIGHTERSHIPB,
	SK_BOMBERSHIPB,
	SK_TRANSPORTSHIPB, 
	SK_GRAPPLE,	
	SK_AGILITY,	
	SK_STRENGTH,
	SK_BACKPACKROCKET,	
	SK_BACKPACKROCKETA,
	SK_SPECIALCHARACTER,
	SK_JETPACKA,
	SK_JETPACKB,
	SK_JETPACKC,
	SK_PISTOLA,
	SK_PISTOLB,
	SK_BLASTERA,
	SK_BLASTERB,
	SK_DISRUPTORA,
	SK_DISRUPTORB,
	SK_BOWCASTERA,
	SK_BOWCASTERB,
	SK_REPEATERA,
	SK_REPEATERB,
	SK_DEMP2A,
	SK_DEMP2B,
	SK_FLECHETTEA,
	SK_FLECHETTEB,
	SK_CONCUSSIONA,
	SK_CONCUSSIONB,
	SK_ROCKETA,
	SK_ROCKETB,
	SK_THERMALA,
	SK_THERMALB,
	SK_TRIPMINEA,
	SK_TRIPMINEB,
	SK_DETPACKA,
	SK_DETPACKB,
	SK_OLDA,
	SK_OLDB,
	SK_FLAMETHROWERA,
	SK_ELECTROSHOCKERA,
	SK_LIGHTNINGA,
	SK_DRAINA,
	SK_GRIPA,
	SK_ABSORBA,
	SK_PROTECTA,
	SK_DESTRUCTIONA,
	SK_HEALA,
	SK_RAGEA,
	SK_TELEPATHYA,
	SK_STASISA,	
	SK_PUSHA,
	SK_PULLA,	
	SK_POWER,	
	SK_RESISTANCE,
	NUM_SKILLS
} skills_t;

#define NUM_TOTAL_SKILLS	(NUM_FORCE_POWERS+NUM_SKILLS)
//[/ExpSys]

typedef enum
{
	SABER_NONE = 0,
	SABER_SINGLE,
	SABER_STAFF,
	SABER_DAGGER,
	SABER_BROAD,
	SABER_PRONG,
	SABER_ARC,
	SABER_SAI,
	SABER_CLAW,
	SABER_LANCE,
	SABER_STAR,
	SABER_TRIDENT,
	SABER_SITH_SWORD,
	NUM_SABERS
} saberType_t;

typedef struct 
{
	// Actual trail stuff
	int		inAction;	// controls whether should we even consider starting one
	int		duration;	// how long each trail seg stays in existence
	int		lastTime;	// time a saber segement was last stored
	vec3_t	base;
	vec3_t	tip;

	vec3_t	dualbase;
	vec3_t	dualtip;

	// Marks stuff
	qboolean	haveOldPos[2];
	vec3_t		oldPos[2];		
	vec3_t		oldNormal[2];	// store this in case we don't have a connect-the-dots situation
							//	..then we'll need the normal to project a mark blob onto the impact point
} saberTrail_t;

typedef struct
{
	qboolean	active;
	saber_colors_t	color;
	float		radius;
	float		length;
	float		lengthMax;
	float		lengthOld;
	float		desiredLength;
	vec3_t		muzzlePoint;		//racc - not updated on cgame side.		
	vec3_t		muzzlePointOld;		//racc - not updated on cgame side.	
	vec3_t		muzzleDir;			//racc - not updated on cgame side.	
	vec3_t		muzzleDirOld;		//racc - not updated on cgame side.	
	saberTrail_t	trail;
	int			hitWallDebounceTime;
	int			storageTime;
	int			extendDebounce;
} bladeInfo_t;
#define MAX_BLADES 8

typedef enum
{
	SS_NONE = 0,
	SS_FAST,
	SS_MEDIUM,
	SS_STRONG,
	SS_DESANN,
	SS_TAVION,
	SS_DUAL,
	SS_STAFF,
	SS_NUM_SABER_STYLES
} saber_styles_t;

//SABER FLAGS
//Old bools converted to a flag now
#define SFL_NOT_LOCKABLE			(1<<0)//can't get into a saberlock
#define SFL_NOT_THROWABLE			(1<<1)//can't be thrown - FIXME: maybe make this a max level of force saber throw that can be used with this saber?
#define SFL_NOT_DISARMABLE			(1<<2)//can't be dropped
#define SFL_NOT_ACTIVE_BLOCKING		(1<<3)//don't to try to block incoming shots with this saber
#define SFL_TWO_HANDED				(1<<4)//uses both hands
#define SFL_SINGLE_BLADE_THROWABLE	(1<<5)//can throw this saber if only the first blade is on
#define SFL_RETURN_DAMAGE			(1<<6)//when returning from a saber throw, it keeps spinning and doing damage
//NEW FLAGS
#define SFL_ON_IN_WATER				(1<<7)//if set, weapon stays active even in water
#define SFL_BOUNCE_ON_WALLS			(1<<8)//if set, the saber will bounce back when it hits solid architecture (good for real-sword type mods)
#define SFL_BOLT_TO_WRIST			(1<<9)//if set, saber model is bolted to wrist, not in hand... useful for things like claws & shields, etc.
//#define SFL_STICK_ON_IMPACT		(1<<?)//if set, the saber will stick in the wall when thrown and hits solid architecture (good for sabers that are meant to be thrown).
//#define SFL_NO_ATTACK				(1<<?)//if set, you cannot attack with the saber (for sabers/weapons that are meant to be thrown only, not used as melee weapons).
//Move Restrictions
#define SFL_NO_PULL_ATTACK			(1<<10)//if set, cannot do pull+attack move (move not available in MP anyway)
#define SFL_NO_BACK_ATTACK			(1<<11)//if set, cannot do back-stab moves
#define SFL_NO_STABDOWN				(1<<12)//if set, cannot do stabdown move (when enemy is on ground)
#define SFL_NO_WALL_RUNS			(1<<13)//if set, cannot side-run or forward-run on walls
#define SFL_NO_WALL_FLIPS			(1<<14)//if set, cannot do backflip off wall or side-flips off walls
#define SFL_NO_WALL_GRAB			(1<<15)//if set, cannot grab wall & jump off
#define SFL_NO_ROLLS				(1<<16)//if set, cannot roll
#define SFL_NO_FLIPS				(1<<17)//if set, cannot do flips
#define SFL_NO_CARTWHEELS			(1<<18)//if set, cannot do cartwheels
#define SFL_NO_KICKS				(1<<19)//if set, cannot do kicks (can't do kicks anyway if using a throwable saber/sword)
#define SFL_NO_MIRROR_ATTACKS		(1<<20)//if set, cannot do the simultaneous attack left/right moves (only available in Dual Lightsaber Combat Style)
#define SFL_NO_ROLL_STAB			(1<<21)//if set, cannot do roll-stab move at end of roll
//SABER FLAGS2
//Primary Blade Style
#define SFL2_NO_WALL_MARKS			(1<<0)//if set, stops the saber from drawing marks on the world (good for real-sword type mods)
#define SFL2_NO_DLIGHT				(1<<1)//if set, stops the saber from drawing a dynamic light (good for real-sword type mods)
#define SFL2_NO_BLADE				(1<<2)//if set, stops the saber from drawing a blade (good for real-sword type mods)
#define SFL2_NO_CLASH_FLARE			(1<<3)//if set, the saber will not do the big, white clash flare with other sabers
#define SFL2_NO_DISMEMBERMENT		(1<<4)//if set, the saber never does dismemberment (good for pointed/blunt melee weapons)
#define SFL2_NO_IDLE_EFFECT			(1<<5)//if set, the saber will not do damage or any effects when it is idle (not in an attack anim).  (good for real-sword type mods)
#define SFL2_ALWAYS_BLOCK			(1<<6)//if set, the blades will always be blocking (good for things like shields that should always block)
#define SFL2_NO_MANUAL_DEACTIVATE	(1<<7)//if set, the blades cannot manually be toggled on and off
#define SFL2_TRANSITION_DAMAGE		(1<<8)//if set, the blade does damage in start, transition and return anims (like strong style does)
//Secondary Blade Style
#define SFL2_NO_WALL_MARKS2			(1<<9)//if set, stops the saber from drawing marks on the world (good for real-sword type mods)
#define SFL2_NO_DLIGHT2				(1<<10)//if set, stops the saber from drawing a dynamic light (good for real-sword type mods)
#define SFL2_NO_BLADE2				(1<<11)//if set, stops the saber from drawing a blade (good for real-sword type mods)
#define SFL2_NO_CLASH_FLARE2		(1<<12)//if set, the saber will not do the big, white clash flare with other sabers
#define SFL2_NO_DISMEMBERMENT2		(1<<13)//if set, the saber never does dismemberment (good for pointed/blunt melee weapons)
#define SFL2_NO_IDLE_EFFECT2		(1<<14)//if set, the saber will not do damage or any effects when it is idle (not in an attack anim).  (good for real-sword type mods)
#define SFL2_ALWAYS_BLOCK2			(1<<15)//if set, the blades will always be blocking (good for things like shields that should always block)
#define SFL2_NO_MANUAL_DEACTIVATE2	(1<<16)//if set, the blades cannot manually be toggled on and off
#define SFL2_TRANSITION_DAMAGE2		(1<<17)//if set, the blade does damage in start, transition and return anims (like strong style does)

typedef struct
{
	char		name[64];						//entry in sabers.cfg, if any
	char		fullName[64];				//the "Proper Name" of the saber, shown in UI
	saberType_t	type;						//none, single or staff
	char		model[MAX_QPATH];						//hilt model
	qhandle_t	skin;						//registered skin id
	int			soundOn;					//game soundindex for turning on sound
	int			soundLoop;					//game soundindex for hum/loop sound
	int			soundOff;					//game soundindex for turning off sound
	int			numBlades;
	bladeInfo_t	blade[MAX_BLADES];			//blade info - like length, trail, origin, dir, etc.
	int			stylesLearned;				//styles you get when you get this saber, if any
	int			stylesForbidden;			//styles you cannot use with this saber, if any
	int			maxChain;					//how many moves can be chained in a row with this weapon (-1 is infinite, 0 is use default behavior)
	int			forceRestrictions;			//force powers that cannot be used while this saber is on (bitfield) - FIXME: maybe make this a limit on the max level, per force power, that can be used with this type?
	int			lockBonus;					//in saberlocks, this type of saber pushes harder or weaker
	int			parryBonus;					//added to strength of parry with this saber
	int			breakParryBonus;			//added to strength when hit a parry
	int			breakParryBonus2;			//for bladeStyle2 (see bladeStyle2Start below)
	int			disarmBonus;				//added to disarm chance when win saberlock or have a good parry (knockaway)
	int			disarmBonus2;				//for bladeStyle2 (see bladeStyle2Start below)
	saber_styles_t	singleBladeStyle;		//makes it so that you use a different style if you only have the first blade active
//	char		*brokenSaber1;				//if saber is actually hit by another saber, it can be cut in half/broken and will be replaced with this saber in your right hand
//	char		*brokenSaber2;				//if saber is actually hit by another saber, it can be cut in half/broken and will be replaced with this saber in your left hand
//===NEW========================================================================================
	//these values are global to the saber, like all of the ones above
	int			saberFlags;					//from SFL_ list above
	int			saberFlags2;				//from SFL2_ list above

	//done in cgame (client-side code)
	qhandle_t	spinSound;					//none - if set, plays this sound as it spins when thrown
	qhandle_t	swingSound[3];				//none - if set, plays one of these 3 sounds when swung during an attack - NOTE: must provide all 3!!!

	//done in game (server-side code)
	float		moveSpeedScale;				//1.0 - you move faster/slower when using this saber
	float		animSpeedScale;				//1.0 - plays normal attack animations faster/slower

	//done in both cgame and game (BG code)
	int	kataMove;				//LS_INVALID - if set, player will execute this move when they press both attack buttons at the same time 
	int	lungeAtkMove;			//LS_INVALID - if set, player will execute this move when they crouch+fwd+attack 
	int	jumpAtkUpMove;			//LS_INVALID - if set, player will execute this move when they jump+attack 
	int	jumpAtkFwdMove;			//LS_INVALID - if set, player will execute this move when they jump+fwd+attack 
	int	jumpAtkBackMove;		//LS_INVALID - if set, player will execute this move when they jump+back+attack
	int	jumpAtkRightMove;		//LS_INVALID - if set, player will execute this move when they jump+rightattack
	int	jumpAtkLeftMove;		//LS_INVALID - if set, player will execute this move when they jump+left+attack
	int	readyAnim;				//-1 - anim to use when standing idle
	int	drawAnim;				//-1 - anim to use when drawing weapon
	int	putawayAnim;			//-1 - anim to use when putting weapon away
	int	tauntAnim;				//-1 - anim to use when hit "taunt"
	int	bowAnim;				//-1 - anim to use when hit "bow"
	int	meditateAnim;			//-1 - anim to use when hit "meditate"
	int	flourishAnim;			//-1 - anim to use when hit "flourish"
	int	gloatAnim;				//-1 - anim to use when hit "gloat"

	//***NOTE: you can only have a maximum of 2 "styles" of blades, so this next value, "bladeStyle2Start" is the number of the first blade to use these value on... all blades before this use the normal values above, all blades at and after this number use the secondary values below***
	int			bladeStyle2Start;			//0 - if set, blades from this number and higher use the following values (otherwise, they use the normal values already set)

	//***The following can be different for the extra blades - not setting them individually defaults them to the value for the whole saber (and first blade)***
	
	//===PRIMARY BLADES=====================
	//done in cgame (client-side code)
	int			trailStyle;					//0 - default (0) is normal, 1 is a motion blur and 2 is no trail at all (good for real-sword type mods)
	int			g2MarksShader;				//none - if set, the game will use this shader for marks on enemies instead of the default "gfx/damage/saberglowmark"
	int			g2WeaponMarkShader;			//none - if set, the game will ry to project this shader onto the weapon when it damages a person (good for a blood splatter on the weapon)
	//int		bladeShader;				//none - if set, overrides the shader used for the saber blade?
	//int		trailShader;				//none - if set, overrides the shader used for the saber trail?
	qhandle_t	hitSound[3];				//none - if set, plays one of these 3 sounds when saber hits a person - NOTE: must provide all 3!!!
	qhandle_t	blockSound[3];				//none - if set, plays one of these 3 sounds when saber/sword hits another saber/sword - NOTE: must provide all 3!!!
	qhandle_t	bounceSound[3];				//none - if set, plays one of these 3 sounds when saber/sword hits a wall and bounces off (must set bounceOnWall to 1 to use these sounds) - NOTE: must provide all 3!!!
	int			blockEffect;				//none - if set, plays this effect when the saber/sword hits another saber/sword (instead of "saber/saber_block.efx")
	int			hitPersonEffect;			//none - if set, plays this effect when the saber/sword hits a person (instead of "saber/blood_sparks_mp.efx")
	int			hitOtherEffect;				//none - if set, plays this effect when the saber/sword hits something else damagable (instead of "saber/saber_cut.efx")
	int			bladeEffect;				//none - if set, plays this effect at the blade tag

	//done in game (server-side code)
	float		knockbackScale;				//0 - if non-zero, uses damage done to calculate an appropriate amount of knockback
	float		damageScale;				//1 - scale up or down the damage done by the saber
	float		splashRadius;				//0 - radius of splashDamage
	int			splashDamage;				//0 - amount of splashDamage, 100% at a distance of 0, 0% at a distance = splashRadius
	float		splashKnockback;			//0 - amount of splashKnockback, 100% at a distance of 0, 0% at a distance = splashRadius
	
	//===SECONDARY BLADES===================
	//done in cgame (client-side code)
	int			trailStyle2;				//0 - default (0) is normal, 1 is a motion blur and 2 is no trail at all (good for real-sword type mods)
	int			g2MarksShader2;				//none - if set, the game will use this shader for marks on enemies instead of the default "gfx/damage/saberglowmark"
	int			g2WeaponMarkShader2;		//none - if set, the game will ry to project this shader onto the weapon when it damages a person (good for a blood splatter on the weapon)
	//int		bladeShader2;				//none - if set, overrides the shader used for the saber blade?
	//int		trailShader2;				//none - if set, overrides the shader used for the saber trail?
	qhandle_t	hit2Sound[3];				//none - if set, plays one of these 3 sounds when saber hits a person - NOTE: must provide all 3!!!
	qhandle_t	block2Sound[3];				//none - if set, plays one of these 3 sounds when saber/sword hits another saber/sword - NOTE: must provide all 3!!!
	qhandle_t	bounce2Sound[3];			//none - if set, plays one of these 3 sounds when saber/sword hits a wall and bounces off (must set bounceOnWall to 1 to use these sounds) - NOTE: must provide all 3!!!
	int			blockEffect2;				//none - if set, plays this effect when the saber/sword hits another saber/sword (instead of "saber/saber_block.efx")
	int			hitPersonEffect2;			//none - if set, plays this effect when the saber/sword hits a person (instead of "saber/blood_sparks_mp.efx")
	int			hitOtherEffect2;			//none - if set, plays this effect when the saber/sword hits something else damagable (instead of "saber/saber_cut.efx")
	int			bladeEffect2;				//none - if set, plays this effect at the blade tag

	//done in game (server-side code)
	float		knockbackScale2;			//0 - if non-zero, uses damage done to calculate an appropriate amount of knockback
	float		damageScale2;				//1 - scale up or down the damage done by the saber
	float		splashRadius2;				//0 - radius of splashDamage
	int			splashDamage2;				//0 - amount of splashDamage, 100% at a distance of 0, 0% at a distance = splashRadius
	float		splashKnockback2;			//0 - amount of splashKnockback, 100% at a distance of 0, 0% at a distance = splashRadius
//=========================================================================================================================================

} saberInfo_t;
#define MAX_SABERS 2

typedef enum
{
	FORCE_LEVEL_0,
	FORCE_LEVEL_1,
	FORCE_LEVEL_2,
	FORCE_LEVEL_3,
	NUM_FORCE_POWER_LEVELS
} forcePowerLevels_t;

#define	FORCE_LEVEL_4 (FORCE_LEVEL_3+1)
#define	FORCE_LEVEL_5 (FORCE_LEVEL_4+1)

//rww - a C-ified structure version of the class which fires off callbacks and gives arguments to update ragdoll status.
enum sharedERagPhase
{
	RP_START_DEATH_ANIM,
	RP_END_DEATH_ANIM,
	RP_DEATH_COLLISION,
	RP_CORPSE_SHOT,
	RP_GET_PELVIS_OFFSET,  // this actually does nothing but set the pelvisAnglesOffset, and pelvisPositionOffset
	RP_SET_PELVIS_OFFSET,  // this actually does nothing but set the pelvisAnglesOffset, and pelvisPositionOffset
	RP_DISABLE_EFFECTORS  // this removes effectors given by the effectorsToTurnOff member
};

enum sharedERagEffector
{
	RE_MODEL_ROOT=			0x00000001, //"model_root"
	RE_PELVIS=				0x00000002, //"pelvis"
	RE_LOWER_LUMBAR=		0x00000004, //"lower_lumbar"
	RE_UPPER_LUMBAR=		0x00000008, //"upper_lumbar"
	RE_THORACIC=			0x00000010, //"thoracic"
	RE_CRANIUM=				0x00000020, //"cranium"
	RE_RHUMEROUS=			0x00000040, //"rhumerus"
	RE_LHUMEROUS=			0x00000080, //"lhumerus"
	RE_RRADIUS=				0x00000100, //"rradius"
	RE_LRADIUS=				0x00000200, //"lradius"
	RE_RFEMURYZ=			0x00000400, //"rfemurYZ"
	RE_LFEMURYZ=			0x00000800, //"lfemurYZ"
	RE_RTIBIA=				0x00001000, //"rtibia"
	RE_LTIBIA=				0x00002000, //"ltibia"
	RE_RHAND=				0x00004000, //"rhand"
	RE_LHAND=				0x00008000, //"lhand"
	RE_RTARSAL=				0x00010000, //"rtarsal"
	RE_LTARSAL=				0x00020000, //"ltarsal"
	RE_RTALUS=				0x00040000, //"rtalus"
	RE_LTALUS=				0x00080000, //"ltalus"
	RE_RRADIUSX=			0x00100000, //"rradiusX"
	RE_LRADIUSX=			0x00200000, //"lradiusX"
	RE_RFEMURX=				0x00400000, //"rfemurX"
	RE_LFEMURX=				0x00800000, //"lfemurX"
	RE_CEYEBROW=			0x01000000 //"ceyebrow"
};

typedef struct
{
	vec3_t angles;
	vec3_t position;
	vec3_t scale;
	vec3_t pelvisAnglesOffset;    // always set on return, an argument for RP_SET_PELVIS_OFFSET
	vec3_t pelvisPositionOffset; // always set on return, an argument for RP_SET_PELVIS_OFFSET

	float fImpactStrength; //should be applicable when RagPhase is RP_DEATH_COLLISION
	float fShotStrength; //should be applicable for setting velocity of corpse on shot (probably only on RP_CORPSE_SHOT)
	int me; //index of entity giving this update

	//rww - we have convenient animation/frame access in the game, so just send this info over from there.
	int startFrame;
	int endFrame;

	int collisionType; // 1 = from a fall, 0 from effectors, this will be going away soon, hence no enum 

	qboolean CallRagDollBegin; // a return value, means that we are now begininng ragdoll and the NPC stuff needs to happen

	int RagPhase;

// effector control, used for RP_DISABLE_EFFECTORS call

	int effectorsToTurnOff;  // set this to an | of the above flags for a RP_DISABLE_EFFECTORS

} sharedRagDollParams_t;

//And one for updating during model animation.
typedef struct
{
	vec3_t angles;
	vec3_t position;
	vec3_t scale;
	vec3_t velocity;
	int	me;
	int settleFrame;
} sharedRagDollUpdateParams_t;

//rww - update parms for ik bone stuff
typedef struct
{
	char boneName[512]; //name of bone
	vec3_t desiredOrigin; //world coordinate that this bone should be attempting to reach
	vec3_t origin; //world coordinate of the entity who owns the g2 instance that owns the bone
	float movementSpeed; //how fast the bone should move toward the destination
} sharedIKMoveParams_t;


typedef struct
{
	vec3_t pcjMins; //ik joint limit
	vec3_t pcjMaxs; //ik joint limit
	vec3_t origin; //origin of caller
	vec3_t angles; //angles of caller
	vec3_t scale; //scale of caller
	float radius; //bone rad
	int blendTime; //bone blend time
	int pcjOverrides; //override ik bone flags
	int startFrame; //base pose start
	int endFrame; //base pose end
	qboolean forceAnimOnBone; //normally if the bone has specified start/end frames already it will leave it alone.. if this is true, then the animation will be restarted on the bone with the specified frames anyway.
} sharedSetBoneIKStateParams_t;

enum sharedEIKMoveState
{
	IKS_NONE = 0,
	IKS_DYNAMIC
};

//material stuff needs to be shared
typedef enum //# material_e
{
	MAT_METAL = 0,	// scorched blue-grey metal
	MAT_GLASS,		// not a real chunk type, just plays an effect with glass sprites
	MAT_ELECTRICAL,	// sparks only
	MAT_ELEC_METAL,	// sparks/electrical type metal
	MAT_DRK_STONE,	// brown
	MAT_LT_STONE,	// tan
	MAT_GLASS_METAL,// glass sprites and METAl chunk
	MAT_METAL2,		// electrical metal type
	MAT_NONE,		// no chunks
	MAT_GREY_STONE,	// grey
	MAT_METAL3,		// METAL and METAL2 chunks
	MAT_CRATE1,		// yellow multi-colored crate chunks
	MAT_GRATE1,		// grate chunks
	MAT_ROPE,		// for yavin trial...no chunks, just wispy bits
	MAT_CRATE2,		// read multi-colored crate chunks
	MAT_WHITE_METAL,// white angular chunks
	MAT_SNOWY_ROCK,	// gray & brown chunks

	NUM_MATERIALS

} material_t;

//rww - bot stuff that needs to be shared
#define MAX_WPARRAY_SIZE 4096
#define MAX_NEIGHBOR_SIZE 32

#define MAX_NEIGHBOR_LINK_DISTANCE 128
#define MAX_NEIGHBOR_FORCEJUMP_LINK_DISTANCE 400

#define DEFAULT_GRID_SPACING 400

typedef struct wpneighbor_s
{
	int num;
	int forceJumpTo;
} wpneighbor_t;

typedef struct wpobject_s
{
	vec3_t origin;
	int inuse;
	int index;
	float weight;
	float disttonext;
	int flags;
	int associated_entity;

	int forceJumpTo;

	int neighbornum;
	wpneighbor_t neighbors[MAX_NEIGHBOR_SIZE];
} wpobject_t;


#define NUMVERTEXNORMALS	162
extern	vec3_t	bytedirs[NUMVERTEXNORMALS];

// all drawing is done to a 640*480 virtual screen size
// and will be automatically scaled to the real resolution
#define	SCREEN_WIDTH		640
#define	SCREEN_HEIGHT		480

#define TINYCHAR_WIDTH		(SMALLCHAR_WIDTH)
#define TINYCHAR_HEIGHT		(SMALLCHAR_HEIGHT/2)

#define SMALLCHAR_WIDTH		8
#define SMALLCHAR_HEIGHT	16

#define BIGCHAR_WIDTH		16
#define BIGCHAR_HEIGHT		16

#define	GIANTCHAR_WIDTH		32
#define	GIANTCHAR_HEIGHT	48

typedef enum
{
CT_NONE,
CT_BLACK,
CT_RED,
CT_GREEN,
CT_BLUE,
CT_YELLOW,
CT_MAGENTA,
CT_CYAN,
CT_WHITE,
CT_LTGREY,
CT_MDGREY,
CT_DKGREY,
CT_DKGREY2,

CT_VLTORANGE,
CT_LTORANGE,
CT_DKORANGE,
CT_VDKORANGE,

CT_VLTBLUE1,
CT_LTBLUE1,
CT_DKBLUE1,
CT_VDKBLUE1,

CT_VLTBLUE2,
CT_LTBLUE2,
CT_DKBLUE2,
CT_VDKBLUE2,

CT_VLTBROWN1,
CT_LTBROWN1,
CT_DKBROWN1,
CT_VDKBROWN1,

CT_VLTGOLD1,
CT_LTGOLD1,
CT_DKGOLD1,
CT_VDKGOLD1,

CT_VLTPURPLE1,
CT_LTPURPLE1,
CT_DKPURPLE1,
CT_VDKPURPLE1,

CT_VLTPURPLE2,
CT_LTPURPLE2,
CT_DKPURPLE2,
CT_VDKPURPLE2,

CT_VLTPURPLE3,
CT_LTPURPLE3,
CT_DKPURPLE3,
CT_VDKPURPLE3,

CT_VLTRED1,
CT_LTRED1,
CT_DKRED1,
CT_VDKRED1,
CT_VDKRED,
CT_DKRED,

CT_VLTAQUA,
CT_LTAQUA,
CT_DKAQUA,
CT_VDKAQUA,

CT_LTPINK,
CT_DKPINK,
CT_LTCYAN,
CT_DKCYAN,
CT_LTBLUE3,
CT_BLUE3,
CT_DKBLUE3,

CT_HUD_GREEN,
CT_HUD_RED,
CT_ICON_BLUE,
CT_NO_AMMO_RED,
CT_HUD_ORANGE,

CT_MAX
} ct_table_t;

extern vec4_t colorTable[CT_MAX];

//[CoOp]
//added clear color for easy of use.
extern	vec4_t		colorClear;
//[/CoOp]

extern	vec4_t		colorBlack;
extern	vec4_t		colorRed;
extern	vec4_t		colorGreen;
extern	vec4_t		colorBlue;
extern	vec4_t		colorYellow;
extern	vec4_t		colorMagenta;
extern	vec4_t		colorCyan;
extern	vec4_t		colorWhite;
extern	vec4_t		colorLtGrey;
extern	vec4_t		colorMdGrey;
extern	vec4_t		colorDkGrey;
extern	vec4_t		colorLtBlue;
extern	vec4_t		colorDkBlue;

#define Q_COLOR_ESCAPE	'^'
// you MUST have the last bit on here about colour strings being less than 7 or taiwanese strings register as colour!!!!
#define Q_IsColorString(p)	( p && *(p) == Q_COLOR_ESCAPE && *((p)+1) && *((p)+1) != Q_COLOR_ESCAPE && *((p)+1) <= '7' && *((p)+1) >= '0' )


#define COLOR_BLACK		'0'
#define COLOR_RED		'1'
#define COLOR_GREEN		'2'
#define COLOR_YELLOW	'3'
#define COLOR_BLUE		'4'
#define COLOR_CYAN		'5'
#define COLOR_MAGENTA	'6'
#define COLOR_WHITE		'7'
#define ColorIndex(c)	( ( (c) - '0' ) & 7 )

#define S_COLOR_BLACK	"^0"
#define S_COLOR_RED		"^1"
#define S_COLOR_GREEN	"^2"
#define S_COLOR_YELLOW	"^3"
#define S_COLOR_BLUE	"^4"
#define S_COLOR_CYAN	"^5"
#define S_COLOR_MAGENTA	"^6"
#define S_COLOR_WHITE	"^7"

extern vec4_t	g_color_table[8];

#define	MAKERGB( v, r, g, b ) v[0]=r;v[1]=g;v[2]=b
#define	MAKERGBA( v, r, g, b, a ) v[0]=r;v[1]=g;v[2]=b;v[3]=a

#define DEG2RAD( a ) ( ( (a) * M_PI ) / 180.0F )
#define RAD2DEG( a ) ( ( (a) * 180.0f ) / M_PI )

struct cplane_s;

extern	vec3_t	vec3_origin;
extern	vec3_t	axisDefault[3];

#define	nanmask (255<<23)

#define	IS_NAN(x) (((*(int *)&x)&nanmask)==nanmask)

float Q_fabs( float f );
float Q_rsqrt( float f );		// reciprocal square root

#define SQRTFAST( x ) ( (x) * Q_rsqrt( x ) )

signed char ClampChar( int i );
signed short ClampShort( int i );

float JKA_powf ( float x, int y );

// this isn't a real cheap function to call!
int DirToByte( vec3_t dir );
void ByteToDir( int b, vec3_t dir );

#if	1

#define DotProduct(x,y)					((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
#define VectorSubtract(a,b,c)			((c)[0]=(a)[0]-(b)[0],(c)[1]=(a)[1]-(b)[1],(c)[2]=(a)[2]-(b)[2])
#define VectorAdd(a,b,c)				((c)[0]=(a)[0]+(b)[0],(c)[1]=(a)[1]+(b)[1],(c)[2]=(a)[2]+(b)[2])
#define	VectorScale(v, s, o)			((o)[0]=(v)[0]*(s),(o)[1]=(v)[1]*(s),(o)[2]=(v)[2]*(s))

#define VectorCopy(a,b)					((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])
#define VectorCopy4(a,b)				((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2],(b)[3]=(a)[3])
#define	VectorMA(v, s, b, o)			((o)[0]=(v)[0]+(b)[0]*(s),(o)[1]=(v)[1]+(b)[1]*(s),(o)[2]=(v)[2]+(b)[2]*(s))
#define VectorInc(v)					((v)[0] += 1.0f,(v)[1] += 1.0f,(v)[2] +=1.0f)
#define VectorDec(v)					((v)[0] -= 1.0f,(v)[1] -= 1.0f,(v)[2] -=1.0f)
#define VectorInverseScaleVector(a,b,c)	((c)[0]=(a)[0]/(b)[0],(c)[1]=(a)[1]/(b)[1],(c)[2]=(a)[2]/(b)[2])
#define VectorScaleVectorAdd(c,a,b,o)	((o)[0]=(c)[0]+((a)[0]*(b)[0]),(o)[1]=(c)[1]+((a)[1]*(b)[1]),(o)[2]=(c)[2]+((a)[2]*(b)[2]))
#define VectorAdvance(a,s,b,c)			(((c)[0]=(a)[0] + s * ((b)[0] - (a)[0])),((c)[1]=(a)[1] + s * ((b)[1] - (a)[1])),((c)[2]=(a)[2] + s * ((b)[2] - (a)[2])))
#define VectorAverage(a,b,c)			(((c)[0]=((a)[0]+(b)[0])*0.5f),((c)[1]=((a)[1]+(b)[1])*0.5f),((c)[2]=((a)[2]+(b)[2])*0.5f))
#define VectorScaleVector(a,b,c)		(((c)[0]=(a)[0]*(b)[0]),((c)[1]=(a)[1]*(b)[1]),((c)[2]=(a)[2]*(b)[2]))

#else

#define DotProduct(x,y)			_DotProduct(x,y)
#define VectorSubtract(a,b,c)	_VectorSubtract(a,b,c)
#define VectorAdd(a,b,c)		_VectorAdd(a,b,c)
#define VectorCopy(a,b)			_VectorCopy(a,b)
#define	VectorScale(v, s, o)	_VectorScale(v,s,o)
#define	VectorMA(v, s, b, o)	_VectorMA(v,s,b,o)

#endif

#ifdef __LCC__
#ifdef VectorCopy
#undef VectorCopy
// this is a little hack to get more efficient copies in our interpreter
typedef struct {
	float	v[3];
} vec3struct_t;
#define VectorCopy(a,b)	*(vec3struct_t *)b=*(vec3struct_t *)a;
#define ID_INLINE static
#endif
#endif

#define VectorClear(a)			((a)[0]=(a)[1]=(a)[2]=0)
#define VectorNegate(a,b)		((b)[0]=-(a)[0],(b)[1]=-(a)[1],(b)[2]=-(a)[2])
#define VectorSet(v, x, y, z)	((v)[0]=(x), (v)[1]=(y), (v)[2]=(z))
#define VectorSet5(v,x,y,z,a,b)	((v)[0]=(x), (v)[1]=(y), (v)[2]=(z), (v)[3]=(a), (v)[4]=(b)) //rwwRMG - added
#define Vector4Copy(a,b)		((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2],(b)[3]=(a)[3])

#define	SnapVector(v) {v[0]=((int)(v[0]));v[1]=((int)(v[1]));v[2]=((int)(v[2]));}

// just in case you do't want to use the macros
vec_t _DotProduct( const vec3_t v1, const vec3_t v2 );
void _VectorSubtract( const vec3_t veca, const vec3_t vecb, vec3_t out );
void _VectorAdd( const vec3_t veca, const vec3_t vecb, vec3_t out );
void _VectorCopy( const vec3_t in, vec3_t out );
void _VectorScale( const vec3_t in, float scale, vec3_t out );
void _VectorMA( const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc );

unsigned ColorBytes3 (float r, float g, float b);
unsigned ColorBytes4 (float r, float g, float b, float a);

float NormalizeColor( const vec3_t in, vec3_t out );

float RadiusFromBounds( const vec3_t mins, const vec3_t maxs );
void ClearBounds( vec3_t mins, vec3_t maxs );
vec_t DistanceHorizontal( const vec3_t p1, const vec3_t p2 );
vec_t DistanceHorizontalSquared( const vec3_t p1, const vec3_t p2 );
void AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs );

#ifndef __LCC__
ID_INLINE int VectorCompare( const vec3_t v1, const vec3_t v2 ) {
	if (v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2]) {
		return 0;
	}			
	return 1;
}

ID_INLINE vec_t VectorLength( const vec3_t v ) {
	return (vec_t)sqrt (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

ID_INLINE vec_t VectorLengthSquared( const vec3_t v ) {
	return (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

ID_INLINE vec_t Distance( const vec3_t p1, const vec3_t p2 ) {
	vec3_t	v;

	VectorSubtract (p2, p1, v);
	return VectorLength( v );
}

ID_INLINE vec_t DistanceSquared( const vec3_t p1, const vec3_t p2 ) {
	vec3_t	v;

	VectorSubtract (p2, p1, v);
	return v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
}

// fast vector normalize routine that does not check to make sure
// that length != 0, nor does it return length, uses rsqrt approximation
ID_INLINE void VectorNormalizeFast( vec3_t v )
{
	float ilength;

	ilength = Q_rsqrt( DotProduct( v, v ) );

	v[0] *= ilength;
	v[1] *= ilength;
	v[2] *= ilength;
}

ID_INLINE void VectorInverse( vec3_t v ){
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

ID_INLINE void CrossProduct( const vec3_t v1, const vec3_t v2, vec3_t cross ) {
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

#else
int VectorCompare( const vec3_t v1, const vec3_t v2 );

vec_t VectorLength( const vec3_t v );

vec_t VectorLengthSquared( const vec3_t v );

vec_t Distance( const vec3_t p1, const vec3_t p2 );

vec_t DistanceSquared( const vec3_t p1, const vec3_t p2 );
 
void VectorNormalizeFast( vec3_t v );

void VectorInverse( vec3_t v );

void CrossProduct( const vec3_t v1, const vec3_t v2, vec3_t cross );

#endif

vec_t VectorNormalize (vec3_t v);		// returns vector length
vec_t VectorNormalize2( const vec3_t v, vec3_t out );
void Vector4Scale( const vec4_t in, vec_t scale, vec4_t out );
void VectorRotate( vec3_t in, vec3_t matrix[3], vec3_t out );
int Q_log2(int val);

float Q_acos(float c);
float Q_asin(float c);

int		Q_rand( int *seed );
float	Q_random( int *seed );
float	Q_crandom( int *seed );

#define random()	((rand () & 0x7fff) / ((float)0x7fff))
#define crandom()	(2.0 * (random() - 0.5))

void vectoangles( const vec3_t value1, vec3_t angles);
void AnglesToAxis( const vec3_t angles, vec3_t axis[3] );

void AxisClear( vec3_t axis[3] );
void AxisCopy( vec3_t in[3], vec3_t out[3] );

void SetPlaneSignbits( struct cplane_s *out );
int BoxOnPlaneSide (vec3_t emins, vec3_t emaxs, struct cplane_s *plane);

double	fmod( double x, double y );
float	AngleMod(float a);
float	LerpAngle (float from, float to, float frac);
float	AngleSubtract( float a1, float a2 );
void	AnglesSubtract( vec3_t v1, vec3_t v2, vec3_t v3 );

float AngleNormalize360 ( float angle );
float AngleNormalize180 ( float angle );
float AngleDelta ( float angle1, float angle2 );

qboolean PlaneFromPoints( vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c );
void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal );
void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees );
void RotateAroundDirection( vec3_t axis[3], float yaw );
void MakeNormalVectors( const vec3_t forward, vec3_t right, vec3_t up );
// perpendicular vector could be replaced by this

//int	PlaneTypeForNormal (vec3_t normal);

void MatrixMultiply(float in1[3][3], float in2[3][3], float out[3][3]);
void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
void PerpendicularVector( vec3_t dst, const vec3_t src );
void NormalToLatLong( const vec3_t normal, byte bytes[2] ); //rwwRMG - added

//=============================================

int Com_Clampi( int min, int max, int value ); //rwwRMG - added
float Com_Clamp( float min, float max, float value );

char	*COM_SkipPath( char *pathname );
void	COM_StripExtension( const char *in, char *out );
void	COM_DefaultExtension( char *path, int maxSize, const char *extension );

void	COM_BeginParseSession( const char *name );
int		COM_GetCurrentParseLine( void );
const char	*SkipWhitespace( const char *data, qboolean *hasNewLines );
char	*COM_Parse( const char **data_p );
char	*COM_ParseExt( const char **data_p, qboolean allowLineBreak );
int		COM_Compress( char *data_p );
void	COM_ParseError( char *format, ... );
void	COM_ParseWarning( char *format, ... );
qboolean COM_ParseString( const char **data, const char **s );
qboolean COM_ParseInt( const char **data, int *i );
qboolean COM_ParseFloat( const char **data, float *f );
qboolean COM_ParseVec3( const char **buffer, vec3_t *c);
qboolean COM_ParseVec4( const char **buffer, vec4_t *c);
//int		COM_ParseInfos( char *buf, int max, char infos[][MAX_INFO_STRING] );

#define MAX_TOKENLENGTH		1024

#ifndef TT_STRING
//token types
#define TT_STRING					1			// string
#define TT_LITERAL					2			// literal
#define TT_NUMBER					3			// number
#define TT_NAME						4			// name
#define TT_PUNCTUATION				5			// punctuation
#endif

typedef struct pc_token_s
{
	int type;
	int subtype;
	int intvalue;
	float floatvalue;
	char string[MAX_TOKENLENGTH];
} pc_token_t;

// data is an in/out parm, returns a parsed out token

void	COM_MatchToken( const char**buf_p, char *match );

void SkipBracedSection (const char **program);
void SkipRestOfLine ( const char **data );

void Parse1DMatrix (const char **buf_p, int x, float *m);
void Parse2DMatrix (const char **buf_p, int y, int x, float *m);
void Parse3DMatrix (const char **buf_p, int z, int y, int x, float *m);

void	QDECL Com_sprintf (char *dest, int size, const char *fmt, ...);


// mode parm for FS_FOpenFile
typedef enum {
	FS_READ,
	FS_WRITE,
	FS_APPEND,
	FS_APPEND_SYNC
} fsMode_t;

typedef enum {
	FS_SEEK_CUR,
	FS_SEEK_END,
	FS_SEEK_SET
} fsOrigin_t;

//=============================================

int Q_isprint( int c );
int Q_islower( int c );
int Q_isupper( int c );
int Q_isalpha( int c );

// portable case insensitive compare
int		Q_stricmp (const char *s1, const char *s2);
int		Q_strncmp (const char *s1, const char *s2, int n);
int		Q_stricmpn (const char *s1, const char *s2, int n);
char	*Q_strlwr( char *s1 );
char	*Q_strupr( char *s1 );
char	*Q_strrchr( const char* string, int c );

// buffer size safe library replacements
void	Q_strncpyz( char *dest, const char *src, int destsize );
void	Q_strcat( char *dest, int size, const char *src );

// strlen that discounts Quake color sequences
int Q_PrintStrlen( const char *string );
// removes color sequences from string
char *Q_CleanStr( char *string );

char	*Q_stristr( const char *s, const char *find);
char	*Q_StrReplace(char *haystack, const char *needle, const char *newp);

qboolean COM_BitCheck( const int array[], unsigned int bitNum );
void COM_BitSet( int array[], unsigned int bitNum );
void COM_BitClear( int array[], unsigned int bitNum );

//[OverflowProtection]
int Q_vsnprintf( char *dest, int size, const char *fmt, va_list argptr );
//[/OverflowProtection]

//=============================================

// 64-bit integers for global rankings interface
// implemented as a struct for qvm compatibility
typedef struct
{
	byte	b0;
	byte	b1;
	byte	b2;
	byte	b3;
	byte	b4;
	byte	b5;
	byte	b6;
	byte	b7;
} qint64;

//=============================================
/*
short	BigShort(short l);
short	LittleShort(short l);
int		BigLong (int l);
int		LittleLong (int l);
qint64  BigLong64 (qint64 l);
qint64  LittleLong64 (qint64 l);
float	BigFloat (const float *l);
float	LittleFloat (const float *l);

void	Swap_Init (void);
*/
//[OverflowProtection]
char	* QDECL va( char *format, ... );
//char	* QDECL va(const char *format, ...);
//[/OverflowProtection]


//=============================================

//
// key / value info strings
//
char *Info_ValueForKey( const char *s, const char *key );
void Info_RemoveKey( char *s, const char *key );
void Info_RemoveKey_big( char *s, const char *key );
void Info_SetValueForKey( char *s, const char *key, const char *value );
void Info_SetValueForKey_Big( char *s, const char *key, const char *value );
qboolean Info_Validate( const char *s );
void Info_NextPair( const char **s, char *key, char *value );

// this is only here so the functions in q_shared.c and bg_*.c can link
void	QDECL Com_Error( int level, const char *error, ... );
void	QDECL Com_Printf( const char *msg, ... );


/*
==========================================================

CVARS (console variables)

Many variables can be used for cheating purposes, so when
cheats is zero, force all unspecified variables to their
default values.
==========================================================
*/

#define	CVAR_ARCHIVE		0x00000001		// set to cause it to be saved to vars.rc
											// used for system variables, not for player
											// specific configurations
#define	CVAR_USERINFO		0x00000002		// sent to server on connect or change
#define	CVAR_SERVERINFO		0x00000004		// sent in response to front end requests
#define	CVAR_SYSTEMINFO		0x00000008		// these cvars will be duplicated on all clients
#define	CVAR_INIT			0x00000010		// don't allow change from console at all,
											// but can be set from the command line
#define	CVAR_LATCH			0x00000020		// will only change when C code next does
											// a Cvar_Get(), so it can't be changed
											// without proper initialization.  modified
											// will be set, even though the value hasn't
											// changed yet
#define	CVAR_ROM			0x00000040		// display only, cannot be set by user at all (can be set by code)
#define	CVAR_USER_CREATED	0x00000080		// created by a set command
#define	CVAR_TEMP			0x00000100		// can be set even when cheats are disabled, but is not archived
#define CVAR_CHEAT			0x00000200		// can not be changed if cheats are disabled
#define CVAR_NORESTART		0x00000400		// do not clear when a cvar_restart is issued
#define CVAR_INTERNAL		0x00000800		// cvar won't be displayed, ever (for passwords and such)
#define	CVAR_PARENTAL		0x00001000		// lets cvar system know that parental stuff needs to be updated

// nothing outside the Cvar_*() functions should modify these fields!
typedef struct cvar_s {
	char		*name;
	char		*string;
	char		*resetString;		// cvar_restart will reset to this value
	char		*latchedString;		// for CVAR_LATCH vars
	int			flags;
	qboolean	modified;			// set each time the cvar is changed
	int			modificationCount;	// incremented each time the cvar is changed
	float		value;				// atof( string )
	int			integer;			// atoi( string )
	struct cvar_s *next;
	struct cvar_s *hashNext;
} cvar_t;

#define	MAX_CVAR_VALUE_STRING	256

typedef int	cvarHandle_t;

// the modules that run in the virtual machine can't access the cvar_t directly,
// so they must ask for structured updates
typedef struct {
	cvarHandle_t	handle;
	int			modificationCount;
	float		value;
	int			integer;
	char		string[MAX_CVAR_VALUE_STRING];
} vmCvar_t;

/*
==============================================================

COLLISION DETECTION

==============================================================
*/

#include "surfaceflags.h"			// shared with the q3map utility

// plane types are used to speed some tests
// 0-2 are axial planes
#define	PLANE_X			0
#define	PLANE_Y			1
#define	PLANE_Z			2
#define	PLANE_NON_AXIAL	3


/*
=================
PlaneTypeForNormal
=================
*/

#define PlaneTypeForNormal(x) (x[0] == 1.0 ? PLANE_X : (x[1] == 1.0 ? PLANE_Y : (x[2] == 1.0 ? PLANE_Z : PLANE_NON_AXIAL) ) )

// plane_t structure
// !!! if this is changed, it must be changed in asm code too !!!
typedef struct cplane_s {
	vec3_t	normal;
	float	dist;
	byte	type;			// for fast side tests: 0,1,2 = axial, 3 = nonaxial
	byte	signbits;		// signx + (signy<<1) + (signz<<2), used as lookup during collision
	byte	pad[2];
} cplane_t;
/*
Ghoul2 Insert Start
*/
typedef struct
{
	float		mDistance;
	int			mEntityNum;
	int			mModelIndex;
	int			mPolyIndex;
	int			mSurfaceIndex;
	vec3_t		mCollisionPosition;
	vec3_t		mCollisionNormal;
	int			mFlags;
	int			mMaterial;
	int			mLocation;
	float		mBarycentricI; // two barycentic coodinates for the hit point
	float		mBarycentricJ; // K = 1-I-J
}CollisionRecord_t;

#define MAX_G2_COLLISIONS 16

typedef CollisionRecord_t G2Trace_t[MAX_G2_COLLISIONS];	// map that describes all of the parts of ghoul2 models that got hit

/*
Ghoul2 Insert End
*/
// a trace is returned when a box is swept through the world
typedef struct {
	byte		allsolid;	// if true, plane is not valid
	byte		startsolid;	// if true, the initial point was in a solid area
	short		entityNum;	// entity the contacted sirface is a part of

	float		fraction;	// time completed, 1.0 = didn't hit anything
	vec3_t		endpos;		// final position
	cplane_t	plane;		// surface normal at impact, transformed to world space
	int			surfaceFlags;	// surface hit
	int			contents;	// contents on other side of surface hit
/*
Ghoul2 Insert Start
*/
	//rww - removed this for now, it's just wasting space in the trace structure.
//	CollisionRecord_t G2CollisionMap[MAX_G2_COLLISIONS];	// map that describes all of the parts of ghoul2 models that got hit
/*
Ghoul2 Insert End
*/
} trace_t;

// trace->entityNum can also be 0 to (MAX_GENTITIES-1)
// or ENTITYNUM_NONE, ENTITYNUM_WORLD


// markfragments are returned by CM_MarkFragments()
typedef struct {
	int		firstPoint;
	int		numPoints;
} markFragment_t;



typedef struct {
	vec3_t		origin;
	vec3_t		axis[3];
} orientation_t;

//=====================================================================


// in order from highest priority to lowest
// if none of the catchers are active, bound key strings will be executed
#define KEYCATCH_CONSOLE		0x0001
#define	KEYCATCH_UI					0x0002
#define	KEYCATCH_MESSAGE		0x0004
#define	KEYCATCH_CGAME			0x0008


// sound channels
// channel 0 never willingly overrides
// other channels will allways override a playing sound on that channel
typedef enum {
	CHAN_AUTO,	//## %s !!"W:\game\base\!!sound\*.wav;*.mp3" # Auto-picks an empty channel to play sound on
	CHAN_LOCAL,	//## %s !!"W:\game\base\!!sound\*.wav;*.mp3" # menu sounds, etc
	CHAN_WEAPON,//## %s !!"W:\game\base\!!sound\*.wav;*.mp3" 
	CHAN_VOICE, //## %s !!"W:\game\base\!!sound\voice\*.wav;*.mp3" # Voice sounds cause mouth animation
	CHAN_VOICE_ATTEN, //## %s !!"W:\game\base\!!sound\voice\*.wav;*.mp3" # Causes mouth animation but still use normal sound falloff 
	CHAN_ITEM,  //## %s !!"W:\game\base\!!sound\*.wav;*.mp3"
	CHAN_BODY,	//## %s !!"W:\game\base\!!sound\*.wav;*.mp3"
	CHAN_AMBIENT,//## %s !!"W:\game\base\!!sound\*.wav;*.mp3" # added for ambient sounds
	CHAN_LOCAL_SOUND,	//## %s !!"W:\game\base\!!sound\*.wav;*.mp3" #chat messages, etc
	CHAN_ANNOUNCER,		//## %s !!"W:\game\base\!!sound\*.wav;*.mp3" #announcer voices, etc
	CHAN_LESS_ATTEN,	//## %s !!"W:\game\base\!!sound\*.wav;*.mp3" #attenuates similar to chan_voice, but uses empty channel auto-pick behaviour
	CHAN_MENU1,		//## %s !!"W:\game\base\!!sound\*.wav;*.mp3" #menu stuff, etc
	CHAN_VOICE_GLOBAL,  //## %s !!"W:\game\base\!!sound\voice\*.wav;*.mp3" # Causes mouth animation and is broadcast, like announcer
	CHAN_MUSIC,	//## %s !!"W:\game\base\!!sound\*.wav;*.mp3" #music played as a looping sound - added by BTO (VV)
} soundChannel_t;


/*
========================================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

========================================================================
*/

#define	ANGLE2SHORT(x)	((int)((x)*65536/360) & 65535)
#define	SHORT2ANGLE(x)	((x)*(360.0/65536))

#define	SNAPFLAG_RATE_DELAYED	1
#define	SNAPFLAG_NOT_ACTIVE		2	// snapshot used during connection and for zombies
#define SNAPFLAG_SERVERCOUNT	4	// toggled every map_restart so transitions can be detected

//
// per-level limits
//
#define	MAX_CLIENTS			32		// absolute limit
#define MAX_RADAR_ENTITIES	MAX_GENTITIES
#define MAX_TERRAINS		1//32 //rwwRMG: inserted
#define MAX_LOCATIONS		64

#define	GENTITYNUM_BITS	10		// don't need to send any more
#define	MAX_GENTITIES	(1<<GENTITYNUM_BITS)

//I am reverting. I guess. For now.
/*
#define	GENTITYNUM_BITS		11
							//rww - I am raising this 1 bit. SP actually has room for 1024 ents - none - world - 1 client.
							//Which means 1021 useable entities. However we have 32 clients.. so if we keep our limit
							//at 1024 we are not going to be able to load any SP levels at the edge of the ent limit.
#define		MAX_GENTITIES	(1024+(MAX_CLIENTS-1))
							//rww - we do have enough room to send over 2048 ents now. However, I cannot live with the guilt of
							//actually increasing the entity limit to 2048 (as it would slow down countless things, and
							//there are tons of ent list traversals all over the place). So I am merely going to give enough
							//to compensate for our larger maxclients.
*/

// entitynums are communicated with GENTITY_BITS, so any reserved
// values thatare going to be communcated over the net need to
// also be in this range
#define	ENTITYNUM_NONE		(MAX_GENTITIES-1)
#define	ENTITYNUM_WORLD		(MAX_GENTITIES-2)
#define	ENTITYNUM_MAX_NORMAL	(MAX_GENTITIES-2)


// these are also in be_aas_def.h - argh (rjr)
#define	MAX_MODELS			512		// these are sent over the net as -12 bits
#define	MAX_SOUNDS			256		// so they cannot be blindly increased
#define MAX_ICONS			64		// max registered icons you can have per map 
#define MAX_FX				64	// max effects strings, I'm hoping that 64 will be plenty

#define MAX_SUB_BSP			32 //rwwRMG - added

/*
Ghoul2 Insert Start
*/
#define	MAX_G2BONES		64		//rww - changed from MAX_CHARSKINS to MAX_G2BONES. value still equal.
/*
Ghoul2 Insert End
*/

#define MAX_AMBIENT_SETS		256 //rww - ambient soundsets must be sent over in config strings.

#define	MAX_CONFIGSTRINGS	1700 //this is getting pretty high. Try not to raise it anymore than it already is.

// these are the only configstrings that the system reserves, all the
// other ones are strictly for servergame to clientgame communication
#define	CS_SERVERINFO		0		// an info string with all the serverinfo cvars
#define	CS_SYSTEMINFO		1		// an info string for server system to client system configuration (timescale, etc)

#define	RESERVED_CONFIGSTRINGS	2	// game can't modify below this, only the system can

#define	MAX_GAMESTATE_CHARS	64000
typedef struct {
	int			stringOffsets[MAX_CONFIGSTRINGS];
	char		stringData[MAX_GAMESTATE_CHARS];
	int			dataCount;
} gameState_t;

//=========================================================

// all the different tracking "channels"
typedef enum {
	TRACK_CHANNEL_NONE = 50,
	TRACK_CHANNEL_1,
	TRACK_CHANNEL_2,
	TRACK_CHANNEL_3,
	TRACK_CHANNEL_4,
	TRACK_CHANNEL_5,
	NUM_TRACK_CHANNELS
} trackchan_t;

#define TRACK_CHANNEL_MAX (NUM_TRACK_CHANNELS-50)

typedef struct forcedata_s {
	int			forcePowerDebounce[NUM_FORCE_POWERS];	//for effects that must have an interval
	int			forcePowersKnown;
	int			forcePowersActive;
	int			forcePowerSelected;
	int			forceButtonNeedRelease;
	int			forcePowerDuration[NUM_FORCE_POWERS];
	int			forcePower;
	int			forcePowerMax;
	int			forcePowerRegenDebounceTime;
	int			forcePowerLevel[NUM_FORCE_POWERS];		//so we know the max forceJump power you have
	int			forcePowerBaseLevel[NUM_FORCE_POWERS];
	int			forceUsingAdded;
	float		forceJumpZStart;					//So when you land, you don't get hurt as much
	float		forceJumpCharge;					//you're current forceJump charge-up level, increases the longer you hold the force jump button down
	int			forceJumpSound;
	int			forceJumpAddTime;
	int			forceGripEntityNum;				//what entity I'm gripping
	int			forceGripDamageDebounceTime;		//debounce for grip damage
	float		forceGripBeingGripped;				//if > level.time then client is in someone's grip
	int			forceGripCripple;					//if != 0 then make it so this client can't move quickly (he's being gripped)
	int			forceGripUseTime;					//can't use if > level.time
	float		forceGripSoundTime;
	float		forceGripStarted;					//level.time when the grip was activated
	int			forceHealTime;
	int			forceHealAmount;

	//This hurts me somewhat to do, but there's no other real way to allow completely "dynamic" mindtricking.
	int			forceMindtrickTargetIndex; //0-15
	int			forceMindtrickTargetIndex2; //16-32
	int			forceMindtrickTargetIndex3; //33-48
	int			forceMindtrickTargetIndex4; //49-64

	int			forceRageRecoveryTime;
	int			forceDrainEntNum;
	float		forceDrainTime;
	int			forceDoInit;

	int			forceSide;
	int			forceRank;		//racc - stores the force mastery rank of the player.  Is set but doesn't seem to be used.

	int			forceDeactivateAll;

	int			killSoundEntIndex[TRACK_CHANNEL_MAX]; //this goes here so it doesn't get wiped over respawn

	//[SentryGun]
	//racc - This variable isn't used anymore since we allow multiple sentry guns now.
	//[/SentryGun]
	//racc - flag for indicating when a player has a sentry gun deployed.  This prevents the player from deploying multiple sentry guns.
	qboolean	sentryDeployed;	

	int			saberAnimLevelBase;//sigh... //racc - This id's the saber style. SS_DUAL for dual sabers, SS_STAFF for staff sabers, and anything else for single saber. 
	int			saberAnimLevel;
	int			saberDrawAnimLevel;

	int			suicides;

	int			privateDuelTime;
} forcedata_t;


typedef enum {
	SENTRY_NOROOM = 1,
	SENTRY_ALREADYPLACED,
	SHIELD_NOROOM,
	SEEKER_ALREADYDEPLOYED
} itemUseFail_t;

// bit field limits
#define	MAX_STATS				16
#define	MAX_PERSISTANT			16
#define	MAX_POWERUPS			16
#define	MAX_WEAPONS				19		

#define	MAX_PS_EVENTS			2

#define PS_PMOVEFRAMECOUNTBITS	6

#define FORCE_LIGHTSIDE			1
#define FORCE_DARKSIDE			2

#define MAX_FORCE_RANK			7

#define FALL_FADE_TIME			3000

//#define _ONEBIT_COMBO
//Crazy optimization attempt to take all those 1 bit values and shove them into a single
//send. May help us not have to send so many 1/0 bits to acknowledge modified values. -rww

#define _OPTIMIZED_VEHICLE_NETWORKING
//Instead of sending 2 full playerStates for the pilot and the vehicle, send a smaller,
//specialized pilot playerState and vehicle playerState.  Also removes some vehicle
//fields from the normal playerState -mcg

// playerState_t is the information needed by both the client and server
// to predict player motion and actions
// nothing outside of pmove should modify these, or some degree of prediction error
// will occur

// you can't add anything to this without modifying the code in msg.c

// playerState_t is a full superset of entityState_t as it is used by players,
// so if a playerState_t is transmitted, the entityState_t can be fully derived
// from it.
typedef struct playerState_s {
	int			commandTime;	// cmd->serverTime of last executed command
	int			pm_type;
	int			bobCycle;		// for view bobbing and footstep generation
	int			pm_flags;		// ducked, jump_held, etc
	int			pm_time;

	vec3_t		origin;
	vec3_t		velocity;

	vec3_t		moveDir; //NOT sent over the net - nor should it be.

	int			weaponTime;
	int			weaponChargeTime;
	int			weaponChargeSubtractTime;
	int			gravity;
	float		speed;
	int			basespeed; //used in prediction to know base server g_speed value when modifying speed between updates
	int			delta_angles[3];	// add to command angles to get view direction
									// changed by spawns, rotating objects, and teleporters

	int			slopeRecalcTime; //this is NOT sent across the net and is maintained seperately on game and cgame in pmove code.

	int			useTime;

	int			groundEntityNum;// ENTITYNUM_NONE = in air

	int			legsTimer;		// don't change low priority animations until this runs out
	int			legsAnim;

	int			torsoTimer;		// don't change low priority animations until this runs out
	int			torsoAnim;

	qboolean	legsFlip; //set to opposite when the same anim needs restarting, sent over in only 1 bit. Cleaner and makes porting easier than having that god forsaken ANIM_TOGGLEBIT.
	qboolean	torsoFlip;

	int			movementDir;	// a number 0 to 7 that represents the reletive angle
								// of movement to the view angle (axial and diagonals)
								// when at rest, the value will remain unchanged
								// used to twist the legs during strafing

	int			eFlags;			// copied to entityState_t->eFlags
	int			eFlags2;		// copied to entityState_t->eFlags2, EF2_??? used much less frequently

	int			eventSequence;	// pmove generated events
	int			events[MAX_PS_EVENTS];
	int			eventParms[MAX_PS_EVENTS];

	int			externalEvent;	// events set on player from another source
	int			externalEventParm;
	int			externalEventTime;

	int			clientNum;		// ranges from 0 to MAX_CLIENTS-1
	int			weapon;			// copied to entityState_t->weapon
	int			weaponstate;

	vec3_t		viewangles;		// for fixed views
	int			viewheight;

	// damage feedback
	int			damageEvent;	// when it changes, latch the other parms
	int			damageYaw;
	int			damagePitch;
	int			damageCount;
	int			damageType;

	int			painTime;		// used for both game and client side to process the pain twitch - NOT sent across the network
	int			painDirection;	// NOT sent across the network
	float		yawAngle;		// NOT sent across the network
	qboolean	yawing;			// NOT sent across the network
	float		pitchAngle;		// NOT sent across the network
	qboolean	pitching;		// NOT sent across the network

	int			stats[MAX_STATS];
	int			persistant[MAX_PERSISTANT];	// stats that aren't cleared on death
	int			powerups[MAX_POWERUPS];	// level.time that the powerup runs out
	int			ammo[MAX_WEAPONS];

	int			generic1;
	int			loopSound;
	int			jumppad_ent;	// jumppad entity hit this frame

	// not communicated over the net at all
	int			ping;			// server to game info for scoreboard
	int			pmove_framecount;	// FIXME: don't transmit over the network
	int			jumppad_frame;
	int			entityEventSequence;

	int			lastOnGround;	//last time you were on the ground

	qboolean	saberInFlight;

	int			saberMove;
	int			saberBlocking;
	int			saberBlocked;

	int			saberLockTime;
	int			saberLockEnemy;
	int			saberLockFrame; //since we don't actually have the ability to get the current anim frame
	int			saberLockHits; //every x number of buttons hits, allow one push forward in a saber lock (server only)
	int			saberLockHitCheckTime; //so we don't allow more than 1 push per server frame
	int			saberLockHitIncrementTime; //so we don't add a hit per attack button press more than once per server frame
	qboolean	saberLockAdvance; //do an advance (sent across net as 1 bit)

	int			saberEntityNum;
	float		saberEntityDist;
	int			saberEntityState;	//racc - this keeps track of the saber's throw state.  
									//0 = not thrown 1 = thrown.
	int			saberThrowDelay;
	qboolean	saberCanThrow;
	//racc - indicates the time when the saber was first tossed.  We use this to determine
	//when the saber should be able to return (to prevent throw spamming).
	int			saberDidThrowTime; 
	int			saberDamageDebounceTime;
	int			saberHitWallSoundDebounceTime;
	int			saberEventFlags;

	int			rocketLockIndex;
	float		rocketLastValidTime;
	float		rocketLockTime;
	float		rocketTargetTime;

	int			emplacedIndex;
	float		emplacedTime;

	qboolean	isJediMaster;
	qboolean	forceRestricted;
	qboolean	trueJedi;
	qboolean	trueNonJedi;
	int			saberIndex;

	int			genericEnemyIndex;
	float		droneFireTime;
	float		droneExistTime;

	int			activeForcePass;

	qboolean	hasDetPackPlanted; //better than taking up an eFlag isn't it?

	float		holocronsCarried[NUM_FORCE_POWERS];
	int			holocronCantTouch;
	float		holocronCantTouchTime; //for keeping track of the last holocron that just popped out of me (if any)
	int			holocronBits;

	int			electrifyTime;

	int			saberAttackSequence;
	int			saberIdleWound;			//[RACC] - Idle damage debouncer, also used for 
										//timing saber clash sparks in the middle of a 
										//saber lock
	int			saberAttackWound;
	int			saberBlockTime;

	int			otherKiller;
	int			otherKillerTime;
	int			otherKillerDebounceTime;

	forcedata_t	fd;
	qboolean	forceJumpFlip;
	int			forceHandExtend;
	int			forceHandExtendTime;

	int			forceRageDrainTime;

	int			forceDodgeAnim;
	qboolean	quickerGetup;

	int			groundTime;		// time when first left ground

	int			footstepTime;

	int			otherSoundTime;
	float		otherSoundLen;

	int			forceGripMoveInterval;
	int			forceGripChangeMovetype;

	int			forceKickFlip;

	int			duelIndex;
	int			duelTime;
	qboolean	duelInProgress;

	//racc - this isn't used anymore in Enhanced thanks to the new saber system.
	int			saberAttackChainCount;

	int			saberHolstered;

	int			forceAllowDeactivateTime;

	// zoom key
	int			zoomMode;		// 0 - not zoomed, 1 - disruptor weapon
	int			zoomTime;
	qboolean	zoomLocked;
	float		zoomFov;
	int			zoomLockTime;

	int			fallingToDeath;

	int			useDelay;

	qboolean	inAirAnim;

	vec3_t		lastHitLoc;

	int			heldByClient; //can only be a client index - this client should be holding onto my arm using IK stuff.

	int			ragAttach; //attach to ent while ragging

	int			iModelScale;

	int			brokenLimbs;

	//for looking at an entity's origin (NPCs and players)
	qboolean	hasLookTarget;
	int			lookTarget;

	int			customRGBA[4];

	int			standheight;
	int			crouchheight;

	//If non-0, this is the index of the vehicle a player/NPC is riding.
	int			m_iVehicleNum;

	//lovely hack for keeping vehicle orientation in sync with prediction
	vec3_t		vehOrientation;
	qboolean	vehBoarding;
	int			vehSurfaces;

	//vehicle turnaround stuff (need this in ps so it doesn't jerk too much in prediction)
	int			vehTurnaroundIndex;
	int			vehTurnaroundTime;

	//vehicle has weapons linked
	qboolean	vehWeaponsLinked;

	//when hyperspacing, you just go forward really fast for HYPERSPACE_TIME
	int			hyperSpaceTime;
	vec3_t		hyperSpaceAngles;

	//hacking when > time
	int			hackingTime;
	//actual hack amount - only for the proper percentage display when
	//drawing progress bar (is there a less bandwidth-eating way to do
	//this without a lot of hassle?)
	int			hackingBaseTime;

	//keeps track of jetpack fuel
	int			jetpackFuel;

	//keeps track of cloak fuel
	int			cloakFuel;

	//rww - spare values specifically for use by mod authors.
	//See psf_overrides.txt if you want to increase the send
	//amount of any of these above 1 bit.
	int			userInt1;
	int			userInt2;
	int			userInt3;
	float		userFloat1;
	float		userFloat2;
	float		userFloat3;
	vec3_t		userVec1;
	vec3_t		userVec2;

#ifdef _ONEBIT_COMBO
	int			deltaOneBits;
	int			deltaNumBits;
#endif
} playerState_t;

typedef struct siegePers_s
{
	qboolean	beatingTime;
	int			lastTeam;
	int			lastTime;
} siegePers_t;

//====================================================================


//
// usercmd_t->button bits, many of which are generated by the client system,
// so they aren't game/cgame only definitions
//
#define	BUTTON_ATTACK			1
//[RACC] - +button0
//+attack

#define	BUTTON_TALK				2			// displays talk balloon and disables actions
//[RACC] - +button1

#define	BUTTON_USE_HOLDABLE		4
//[RACC] - +button2

#define	BUTTON_GESTURE			8
//[RACC] - +button3

#define	BUTTON_WALKING			16			// walking can't just be infered from MOVE_RUN
										// because a key pressed late in the frame will
										// only generate a small move value for that frame
										// walking will use different animations and
										// won't generate footsteps
//[RACC] - +button4?

#define	BUTTON_USE				32			// the ol' use key returns!
//[RACC] - +button5
//+use

#define BUTTON_FORCEGRIP		64			
//[RACC] - +button6

#define BUTTON_ALT_ATTACK		128
//[RACC] - +button7
//+altattack

#define	BUTTON_ANY				256			// any key whatsoever
//[RACC] - +button8?

#define BUTTON_FORCEPOWER		512			// use the "active" force power
//[RACC] - +button9
//+useforce

#define BUTTON_FORCE_LIGHTNING	1024
//[RACC] - +button10
//+force_lightning

#define BUTTON_FORCE_DRAIN		2048
//[RACC] - +button11
//+force_drain

//[SaberSys]
//[SaberDefines]
//new button defines.
#define	BUTTON_SABERTHROW		4096		//+button12
//[SnapThrow]
#define	BUTTON_THERMALTHROW		8192		//+button13
//[/SnapThrow]
#define	BUTTON_GRAPPLE						16384		//+button14
#define	BUTTON_15				32768
//NOTE: As far as I've been able to test, the buttons seen below don't work because they 
//aren't set to command buttons in the engine.
#define	BUTTON_16				32768
#define	BUTTON_17				65536	//seems to trigger after landing on some and then randomly after that.
#define	BUTTON_18				131072
#define	BUTTON_19				262144
#define	BUTTON_20				524288
#define	BUTTON_21				1048576
#define	BUTTON_22				2097152
#define	BUTTON_23				4194304
#define	BUTTON_24				8388608
#define	BUTTON_25				16777216
#define	BUTTON_26				33554432
#define	BUTTON_27				67108864
#define	BUTTON_28				134217728
#define	BUTTON_29				268435456
#define	BUTTON_30				536870912
#define	BUTTON_31				1073741824
#define	BUTTON_32				2147483648
//[/SaberDefines]
//[/SaberSys]

// Here's an interesting bit.  The bots in TA used buttons to do additional gestures.
// I ripped them out because I didn't want too many buttons given the fact that I was already adding some for JK2.
// We can always add some back in if we want though.
/*
#define BUTTON_AFFIRMATIVE	32
#define	BUTTON_NEGATIVE		64

#define BUTTON_GETFLAG		128
#define BUTTON_GUARDBASE	256
#define BUTTON_PATROL		512
#define BUTTON_FOLLOWME		1024
*/

#define	MOVE_RUN			120			// if forwardmove or rightmove are >= MOVE_RUN,
										// then BUTTON_WALKING should be set

//[SaberSys]
//playerstate userint1
//Bitmasks for view locking
#define	LOCK_RIGHT			1
#define	LOCK_LEFT			2
#define LOCK_UP				4
#define LOCK_DOWN			8

//Bitmasks for move locking
#define LOCK_MOVERIGHT		16
#define LOCK_MOVELEFT		32
#define LOCK_MOVEFORWARD	64
#define LOCK_MOVEBACK		128
#define LOCK_MOVEUP			256
#define LOCK_MOVEDOWN		512
//[/SaberSys]

//entitystate/playerstate userint3

//Flag bits in bit number form
//use "(1 << flag)"
typedef enum {
	//[/FatigueSys]
	FLAG_FATIGUED_LIGHT = 1	
	, FLAG_FATIGUED_HEAVY		//Badly Fatigued Flag
	, FLAG_DODGEROLL		//Dodge low flag
	//indicates that the current attack/transition is
	//part of a fake.  This makes the attack much stronger
	//forbreaking thru other attacks and blocks.
	, FLAG_ATTACKFAKE
	//[/FatigueSys]

	//[SaberSys]
	//This flag indicates that the player should have a slower than usual bounce since they just avoided a mishap by
	//having enough FP/DP.
	, FLAG_SLOWBOUNCE
	//This flag indicates at the player is going into a older, more vulnerable slow bounce animations
	//this must be used in conjunction with the FLAG_SLOWBOUNCE to work right.
	, FLAG_OLDSLOWBOUNCE

	//this flag indicates that this player is current on offense in the current saberlock.
	, FLAG_SABERLOCK_ATTACKER

	//This flag indicates that the current saber lock direction was the last one selected and can't be used again.
	, FLAG_SABERLOCK_OLD_DIR

	//Saber lock directions, valid on both saber lockees
	, FLAG_SABERLOCK_UP
	, FLAG_SABERLOCK_DOWN
	, FLAG_SABERLOCK_LEFT
	, FLAG_SABERLOCK_RIGHT

	//flag indicates that the player was parried.  
	//They won't be able to launch into a combo from the bounce.
	, FLAG_PARRIED

	//flag indicates that this block is a pre-block and interruptable
	, FLAG_PREBLOCK

	//[QuickParry]
//	, FLAG_QUICKPARRY
	//[/QuickParry]
	, FLAG_BLOCKING
	//[/SaberSys]

	//[Flamethrower]
	//flag indicating that the player's flamethrower is active.
	, FLAG_THROWER
	, FLAG_THROWER2
	//[/Flamethrower]

//[Electroshocker]
//flag indicating that the player's electroshocker is active.
	,	FLAG_ADVANCEDTHROWER
	,	FLAG_ADVANCEDTHROWER2
//[/Electroshocker]

	//[DodgeSys]
//	, FLAG_DODGE_LIGHT
//	, FLAG_DODGE_CRITICAL
	, FLAG_JETPACK2
	, FLAG_JETPACK3
	, FLAG_JETPACK4
	, FLAG_JETPACK5
	, FLAG_LIGHTNING2
	, FLAG_DRAIN2
	, FLAG_ABSORB2
	, FLAG_PROTECT2
	, FLAG_RAGE2
	, FLAG_PUSH2
	, FLAG_PULL2
	, FLAG_DESTRUCTION2
	//[/DodgeSys]
} userInt3Flags_t;

//[SaberLockSys]
#define SABERLOCK_DIR_FLAG_MASK	((1 << FLAG_SABERLOCK_UP) | (1 << FLAG_SABERLOCK_DOWN) | (1 << FLAG_SABERLOCK_LEFT) | (1 << FLAG_SABERLOCK_RIGHT))
//[/SaberLockSys]

//[SaberDefines]
//scaler to the walkspeed
#define WALKSPEED			1.75//1.35
//[/SaberDefines]
//[/SaberSys]




typedef enum
{
	GENCMD_SABERSWITCH = 1,
	GENCMD_ENGAGE_DUEL,
	GENCMD_FORCE_HEAL,
	GENCMD_FORCE_SPEED,
	GENCMD_FORCE_THROW,
	GENCMD_FORCE_PULL,
	GENCMD_FORCE_DISTRACT,
	GENCMD_FORCE_RAGE,
	GENCMD_FORCE_PROTECT,
	GENCMD_FORCE_ABSORB,
	GENCMD_FORCE_HEALOTHER,
	GENCMD_FORCE_FORCEPOWEROTHER,
	GENCMD_FORCE_SEEING,
	GENCMD_USE_SEEKER,
	GENCMD_USE_FIELD,
	GENCMD_USE_BACTA,
	GENCMD_USE_OVERLOAD,
	GENCMD_USE_ELECTROBINOCULARS,
	GENCMD_USE_SPHERESHIELD,
	GENCMD_USE_SENTRY,
	GENCMD_USE_JETPACK,
	GENCMD_USE_SQUADTEAM,
	GENCMD_USE_VEHICLEMOUNT,
	GENCMD_USE_EWEB,
	GENCMD_USE_CLOAK,
	GENCMD_SABERATTACKCYCLE,
	GENCMD_TAUNT,
	GENCMD_BOW,
	GENCMD_MEDITATE,
	GENCMD_FLOURISH,
	GENCMD_GLOAT,
	GENCMD_USE_REPAIR,
} genCmds_t;

// usercmd_t is sent to the server each client frame
typedef struct usercmd_s {
	int				serverTime;
	int				angles[3];
	int 			buttons;
	byte			weapon;           // weapon 
	byte			forcesel;		  //racc - force power selection command.
	byte			invensel;
	byte			generic_cmd;
	signed char	forwardmove, rightmove, upmove;

} usercmd_t;

//===================================================================

//rww - unsightly hack to allow us to make an FX call that takes a horrible amount of args
typedef struct addpolyArgStruct_s {
	vec3_t				p[4];
	vec2_t				ev[4];
	int					numVerts;
	vec3_t				vel;
	vec3_t				accel;
	float				alpha1;
	float				alpha2;
	float				alphaParm;
	vec3_t				rgb1;
	vec3_t				rgb2;
	float				rgbParm;
	vec3_t				rotationDelta;
	float				bounce;
	int					motionDelay;
	int					killTime;
	qhandle_t			shader;
	int					flags;
} addpolyArgStruct_t;

typedef struct addbezierArgStruct_s {
	vec3_t start;
	vec3_t end;
	vec3_t control1;
	vec3_t control1Vel;
	vec3_t control2;
	vec3_t control2Vel;
	float size1;
	float size2;
	float sizeParm;
	float alpha1;
	float alpha2;
	float alphaParm;
	vec3_t sRGB;
	vec3_t eRGB;
	float rgbParm;
	int killTime;
	qhandle_t shader;
	int flags;
} addbezierArgStruct_t;

typedef struct addspriteArgStruct_s
{
	vec3_t origin;
	vec3_t vel;
	vec3_t accel;
	float scale;
	float dscale;
	float sAlpha;
	float eAlpha;
	float rotation;
	float bounce;
	int life;
	qhandle_t shader;
	int flags;
} addspriteArgStruct_t;

typedef struct
{
	vec3_t	origin;

	// very specifc case, we can modulate the color and the alpha
	vec3_t	rgb;
	vec3_t	destrgb;
	vec3_t	curRGB;

	float	alpha;
	float	destAlpha;
	float	curAlpha;

	// this is a very specific case thing...allow interpolating the st coords so we can map the texture
	//	properly as this segement progresses through it's life
	float	ST[2];
	float	destST[2];
	float	curST[2];
} effectTrailVertStruct_t;

typedef struct effectTrailArgStruct_s {
	effectTrailVertStruct_t		mVerts[4];
	qhandle_t					mShader;
	int							mSetFlags;
	int							mKillTime;
} effectTrailArgStruct_t;

typedef struct
{
	vec3_t start;
	vec3_t end;
	float size1;
	float size2;
	float sizeParm;
	float alpha1;
	float alpha2;
	float alphaParm;
	vec3_t sRGB;
	vec3_t eRGB;
	float rgbParm;
	float chaos;
	int killTime;
	qhandle_t shader;
	int flags;
} addElectricityArgStruct_t;

// if entityState->solid == SOLID_BMODEL, modelindex is an inline model number
#define	SOLID_BMODEL	0xffffff

typedef enum {
	TR_STATIONARY,
	TR_INTERPOLATE,				// non-parametric, but interpolate between snapshots
	TR_LINEAR,
	TR_LINEAR_STOP,
	TR_NONLINEAR_STOP,
	TR_SINE,					// value = base + sin( time / duration ) * delta
	TR_GRAVITY
} trType_t;

typedef struct {
	trType_t	trType;
	int		trTime;
	int		trDuration;			// if non 0, trTime + trDuration = stop time
	vec3_t	trBase;
	vec3_t	trDelta;			// velocity, etc
} trajectory_t;

// entityState_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
// Different eTypes may use the information in different ways
// The messages are delta compressed, so it doesn't really matter if
// the structure size is fairly large

typedef struct entityState_s {
	int		number;			// entity index
	int		eType;			// entityType_t
	int		eFlags;
	int		eFlags2;		// EF2_??? used much less frequently

	trajectory_t	pos;	// for calculating position
	trajectory_t	apos;	// for calculating angles

	int		time;
	int		time2;

	vec3_t	origin;			//racc - this variable isn't updated when the entity is a playing player, 
							//but it is used when a player is spectating.
	vec3_t	origin2;

	vec3_t	angles;
	vec3_t	angles2;

	//rww - these were originally because we shared g2 info client and server side. Now they
	//just get used as generic values everywhere.
	int		bolt1;
	int		bolt2;

	//rww - this is necessary for determining player visibility during a jedi mindtrick
	int		trickedentindex; //0-15
	int		trickedentindex2; //16-32
	int		trickedentindex3; //33-48
	int		trickedentindex4; //49-64

	float	speed;

	int		fireflag;

	int		genericenemyindex;

	int		activeForcePass;

	int		emplacedOwner;

	int		otherEntityNum;	// shotgun sources, etc
	int		otherEntityNum2;

	int		groundEntityNum;	// -1 = in air

	int		constantLight;	// r + (g<<8) + (b<<16) + (intensity<<24)
	int		loopSound;		// constantly loop this sound
	qboolean	loopIsSoundset; //qtrue if the loopSound index is actually a soundset index

	int		soundSetIndex;

	int		modelGhoul2;
	int		g2radius;
	int		modelindex;
	int		modelindex2;
	int		clientNum;		// 0 to (MAX_CLIENTS - 1), for players and corpses
	int		frame;

	qboolean	saberInFlight;
	int			saberEntityNum;
	int			saberMove;
	int			forcePowersActive;
	int			saberHolstered;//sent in only only 2 bits - should be 0, 1 or 2

	qboolean	isJediMaster;

	qboolean	isPortalEnt; //this needs to be seperate for all entities I guess, which is why I couldn't reuse another value.

	int		solid;			// for client side prediction, trap_linkentity sets this properly

	int		event;			// impulse events -- muzzle flashes, footsteps, etc
	int		eventParm;

	// so crosshair knows what it's looking at
	int			owner;
	int			teamowner;
	qboolean	shouldtarget;

	// for players
	int		powerups;		// bit flags
	int		weapon;			// determines weapon and flash model, etc
	int		legsAnim;
	int		torsoAnim;
	
	qboolean	legsFlip; //set to opposite when the same anim needs restarting, sent over in only 1 bit. Cleaner and makes porting easier than having that god forsaken ANIM_TOGGLEBIT.
	qboolean	torsoFlip;

	int		forceFrame;		//if non-zero, force the anim frame

	int		generic1;

	int		heldByClient; //can only be a client index - this client should be holding onto my arm using IK stuff.

	int		ragAttach; //attach to ent while ragging

	int		iModelScale; //rww - transfer a percentage of the normal scale in a single int instead of 3 x-y-z scale values

	int		brokenLimbs;

	int		boltToPlayer; //set to index of a real client+1 to bolt the ent to that client. Must be a real client, NOT an NPC.

	//for looking at an entity's origin (NPCs and players)
	qboolean	hasLookTarget;
	int			lookTarget;

	int			customRGBA[4];

	//I didn't want to do this, but I.. have no choice. However, we aren't setting this for all ents or anything,
	//only ones we want health knowledge about on cgame (like siege objective breakables) -rww
	int			health;
	int			maxhealth; //so I know how to draw the stupid health bar

	//NPC-SPECIFIC FIELDS
	//------------------------------------------------------------
	int		npcSaber1;
	int		npcSaber2;

	//index values for each type of sound, gets the folder the sounds
	//are in. I wish there were a better way to do this,
	int		csSounds_Std;
	int		csSounds_Combat;
	int		csSounds_Extra;
	int		csSounds_Jedi;

	int		surfacesOn; //a bitflag of corresponding surfaces from a lookup table. These surfaces will be forced on.
	int		surfacesOff; //same as above, but forced off instead.

	//Allow up to 4 PCJ lookup values to be stored here.
	//The resolve to configstrings which contain the name of the
	//desired bone.
	int		boneIndex1;
	int		boneIndex2;
	int		boneIndex3;
	int		boneIndex4;

	//packed with x, y, z orientations for bone angles
	int		boneOrient;

	//I.. feel bad for doing this, but NPCs really just need to
	//be able to control this sort of thing from the server sometimes.
	//At least it's at the end so this stuff is never going to get sent
	//over for anything that isn't an NPC.
	vec3_t	boneAngles1; //angles of boneIndex1
	vec3_t	boneAngles2; //angles of boneIndex2
	vec3_t	boneAngles3; //angles of boneIndex3
	vec3_t	boneAngles4; //angles of boneIndex4

	int		NPC_class; //we need to see what it is on the client for a few effects.

	//If non-0, this is the index of the vehicle a player/NPC is riding.
	int		m_iVehicleNum;

	//rww - spare values specifically for use by mod authors.
	//See netf_overrides.txt if you want to increase the send
	//amount of any of these above 1 bit.
	int			userInt1;
	int			userInt2;
	int			userInt3;
	float		userFloat1;
	float		userFloat2;
	float		userFloat3;
	vec3_t		userVec1;
	vec3_t		userVec2;
} entityState_t;

typedef enum {
	CA_UNINITIALIZED,
	CA_DISCONNECTED, 	// not talking to a server
	CA_AUTHORIZING,		// not used any more, was checking cd key 
	CA_CONNECTING,		// sending request packets to the server
	CA_CHALLENGING,		// sending challenge packets to the server
	CA_CONNECTED,		// netchan_t established, getting gamestate
	CA_LOADING,			// only during cgame initialization, never during main loop
	CA_PRIMED,			// got gamestate, waiting for first frame
	CA_ACTIVE,			// game views should be displayed
	CA_CINEMATIC		// playing a cinematic or a static pic, not connected to a server
} connstate_t;


#define Square(x) ((x)*(x))

// real time
//=============================================


typedef struct qtime_s {
	int tm_sec;     /* seconds after the minute - [0,59] */
	int tm_min;     /* minutes after the hour - [0,59] */
	int tm_hour;    /* hours since midnight - [0,23] */
	int tm_mday;    /* day of the month - [1,31] */
	int tm_mon;     /* months since January - [0,11] */
	int tm_year;    /* years since 1900 */
	int tm_wday;    /* days since Sunday - [0,6] */
	int tm_yday;    /* days since January 1 - [0,365] */
	int tm_isdst;   /* daylight savings time flag */
} qtime_t;


// server browser sources
#define AS_LOCAL			0
#define AS_GLOBAL			1
#define AS_FAVORITES		2

#define AS_MPLAYER			3 // (Obsolete)

// cinematic states
typedef enum {
	FMV_IDLE,
	FMV_PLAY,		// play
	FMV_EOF,		// all other conditions, i.e. stop/EOF/abort
	FMV_ID_BLT,
	FMV_ID_IDLE,
	FMV_LOOPED,
	FMV_ID_WAIT
} e_status;

typedef enum flagStatus_e {
	FLAG_ATBASE = 0,
	FLAG_TAKEN,			// CTF
	FLAG_TAKEN_RED,		// One Flag CTF
	FLAG_TAKEN_BLUE,	// One Flag CTF
	FLAG_DROPPED
} flagStatus_t;


#define	MAX_GLOBAL_SERVERS			2048
#define	MAX_OTHER_SERVERS			128
#define MAX_PINGREQUESTS			32
#define MAX_SERVERSTATUSREQUESTS	16

#define SAY_ALL		0
#define SAY_TEAM	1
#define SAY_TELL	2

#define CDKEY_LEN 16
#define CDCHKSUM_LEN 2

void Rand_Init(int seed);
float flrand(float min, float max);
int irand(int min, int max);
int Q_irand(int value1, int value2);

//[AotCAI]
float VectorDistance(vec3_t v1, vec3_t v2);
//[/AotCAI]

/*
Ghoul2 Insert Start
*/

typedef struct {
	float		matrix[3][4];
} mdxaBone_t;

// For ghoul2 axis use

typedef enum Eorientations
{
	ORIGIN = 0, 
	POSITIVE_X,
	POSITIVE_Z,
	POSITIVE_Y,
	NEGATIVE_X,
	NEGATIVE_Z,
	NEGATIVE_Y
} orientations_t;
/*
Ghoul2 Insert End
*/


// define the new memory tags for the zone, used by all modules now
//
#define TAGDEF(blah) TAG_ ## blah
typedef enum {
	#include "../qcommon/tags.h"
} memtag;
typedef unsigned int memtag_t;

//rww - conveniently toggle "gore" code, for model decals and stuff.
#define _G2_GORE

typedef struct SSkinGoreData_s
{
	vec3_t			angles;
	vec3_t			position;
	int				currentTime;
	int				entNum;
	vec3_t			rayDirection;	// in world space
	vec3_t			hitLocation;	// in world space
	vec3_t			scale;
	float			SSize;			// size of splotch in the S texture direction in world units
	float			TSize;			// size of splotch in the T texture direction in world units
	float			theta;			// angle to rotate the splotch

	// growing stuff
	int				growDuration;			// time over which we want this to scale up, set to -1 for no scaling
	float			goreScaleStartFraction; // fraction of the final size at which we want the gore to initially appear

	qboolean		frontFaces;
	qboolean		backFaces;
	qboolean		baseModelOnly;
	int				lifeTime;				// effect expires after this amount of time
	int				fadeOutTime;			//specify the duration of fading, from the lifeTime (e.g. 3000 will start fading 3 seconds before removal and be faded entirely by removal)
	int				shrinkOutTime;			// unimplemented
	float			alphaModulate;			// unimplemented
	vec3_t			tint;					// unimplemented
	float			impactStrength;			// unimplemented

	int				shader; // shader handle 

	int				myIndex; // used internally

	qboolean		fadeRGB; //specify fade method to modify RGB (by default, the alpha is set instead)
} SSkinGoreData;

/*
========================================================================

String ID Tables

========================================================================
*/
#define ENUM2STRING(arg)   { #arg,arg }
typedef struct stringID_table_s
{
	char	*name;
	int		id;
} stringID_table_t;

int GetIDForString ( stringID_table_t *table, const char *string );
const char *GetStringForID( stringID_table_t *table, int id );


// stuff to help out during development process, force reloading/uncacheing of certain filetypes...
//
typedef enum
{
	eForceReload_NOTHING,
//	eForceReload_BSP,	// not used in MP codebase
	eForceReload_MODELS,
	eForceReload_ALL

} ForceReload_e;


enum {
	FONT_NONE,
	FONT_SMALL=1,
	FONT_MEDIUM,
	FONT_LARGE,
	FONT_SMALL2
};

typedef intptr_t (QDECL *dllSyscall_t)( intptr_t callNum, ... );

#endif	// __Q_SHARED_H__
