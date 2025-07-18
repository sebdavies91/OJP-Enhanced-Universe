#include "q_shared.h"
#include "bg_public.h"
#include "bg_local.h"
#include "w_saber.h"


//[SaberLockSys]
//moved this up here to allow more debug messages earlier in the file.
#ifdef QAGAME
#include "g_local.h"
extern stringID_table_t animTable [MAX_ANIMATIONS+1];
extern stringID_table_t SaberMoveTable[];
#endif
//[/SaberLockSys]

extern qboolean BG_SabersOff( playerState_t *ps );
saberInfo_t *BG_MySaber( int clientNum, int saberNum );

int PM_irand_timesync(int val1, int val2)
{
	int i;

	i = (val1-1) + (Q_random( &pm->cmd.serverTime )*(val2 - val1)) + 1;
	if (i < val1)
	{
		i = val1;
	}
	if (i > val2)
	{
		i = val2;
	}

	return i;
}

//[FatigueSys]
void UpdateFatigueFlags( playerState_t *ps )
{//Update fatigue flags
	if(ps->fd.forcePower < FATIGUELEVEL_HEAVY)
	{
		ps->userInt3 |= ( 1 << FLAG_FATIGUED_HEAVY );
		ps->userInt3 &= ~(1 << FLAG_FATIGUED_LIGHT);
	}
	else if(ps->fd.forcePower < FATIGUELEVEL_LIGHT)
	{
		ps->userInt3 &= ~( 1 << FLAG_FATIGUED_HEAVY );
		ps->userInt3 |= (1 << FLAG_FATIGUED_LIGHT);
	}
	else
	{
		ps->userInt3 &= ~( 1 << FLAG_FATIGUED_HEAVY );
		ps->userInt3 &= ~(1 << FLAG_FATIGUED_LIGHT);
	}

	//[/FatigueSys]
}

void BG_ForcePowerDrain( playerState_t *ps, forcePowers_t forcePower, int overrideAmt )
{
	//take away the power
	int	drain = overrideAmt;

	/*
	if (ps->powerups[PW_FORCE_BOON])
	{
		return;
	}
	*/
	//No longer grant infinite force with boon.

	if ( !drain )
	{
		drain = forcePowerNeeded[ps->fd.forcePowerLevel[forcePower]][forcePower];
	}
	if ( !drain )
	{
		return;
	}

	//[FatigueSys]
	//want consistant drain now, but don't drain FP when the player is dropping from the jump
	if (forcePower == FP_LEVITATION && ps->velocity[2] <= 0)
	{//don't drain FP after the top of the arc since the player is just dropping anyway.
		return;
	}

	/* basejka
	if (forcePower == FP_LEVITATION)
	{ //special case
		int jumpDrain = 0;

		if (ps->velocity[2] > 250)
		{
			jumpDrain = 20;
		}
		else if (ps->velocity[2] > 200)
		{
			jumpDrain = 16;
		}
		else if (ps->velocity[2] > 150)
		{
			jumpDrain = 12;
		}
		else if (ps->velocity[2] > 100)
		{
			jumpDrain = 8;
		}
		else if (ps->velocity[2] > 50)
		{
			jumpDrain = 6;
		}
		else if (ps->velocity[2] > 0)
		{
			jumpDrain = 4;
		}

		if (jumpDrain)
		{
			if (ps->fd.forcePowerLevel[FP_LEVITATION])
			{ //don't divide by 0!
				jumpDrain /= ps->fd.forcePowerLevel[FP_LEVITATION];
			}
		}

		ps->fd.forcePower -= jumpDrain;
		if ( ps->fd.forcePower < 0 )
		{
			ps->fd.forcePower = 0;
		}

		return;
	}
	*/
	//[/FatigueSys]

	ps->fd.forcePower -= drain;
	if ( ps->fd.forcePower < 0 )
	{
		ps->fd.forcePower = 0;
	}

	UpdateFatigueFlags(ps);
}

qboolean BG_EnoughForcePowerForMove( int cost )
{
	if ( pm->ps->fd.forcePower < cost )
	{
		PM_AddEvent( EV_NOAMMO );
		return qfalse;
	}

	return qtrue;
}

// Silly, but I'm replacing these macros so they are shorter!
#define AFLAG_IDLE	(SETANIM_FLAG_NORMAL)
#define AFLAG_ACTIVE (SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS)
#define AFLAG_WAIT (SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS)
//[SaberSys]
//We probably shouldn't use SETANIM_FLAG_HOLD without SETANIM_FLAG_HOLDLESS because it doesn't account for animation speed differences.
#define AFLAG_FINISH (SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS)
//#define AFLAG_FINISH (SETANIM_FLAG_HOLD)
//[/SaberSys]

//FIXME: add the alternate anims for each style?
saberMoveData_t	saberMoveData[LS_MOVE_MAX] = {//							NB:randomized
	// name			anim(do all styles?)startQ	endQ	setanimflag		blend,	blocking	chain_idle		chain_attack	trailLen
	{"None",		BOTH_STAND1,		Q_R,	Q_R,	AFLAG_IDLE,		350,	BLK_NO,		LS_NONE,		LS_NONE,		0	},	// LS_NONE		= 0,

	// General movements with saber
	{"Ready",		BOTH_STAND2,		Q_R,	Q_R,	AFLAG_IDLE,		350,	BLK_WIDE,	LS_READY,		LS_S_R2L,		0	},	// LS_READY,
	{"Draw",		BOTH_STAND1TO2,		Q_R,	Q_R,	AFLAG_FINISH,	350,	BLK_NO,		LS_READY,		LS_S_R2L,		0	},	// LS_DRAW,
	{"Putaway",		BOTH_STAND2TO1,		Q_R,	Q_R,	AFLAG_FINISH,	350,	BLK_NO,		LS_READY,		LS_S_R2L,		0	},	// LS_PUTAWAY,

	// Attacks
	//UL2LR
	{"TL2BR Att",	BOTH_A1_TL_BR,		Q_TL,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_R_TL2BR,		LS_R_TL2BR,		200	},	// LS_A_TL2BR
	//SLASH LEFT
	{"L2R Att",		BOTH_A1__L__R,		Q_L,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_R_L2R,		LS_R_L2R,		200 },	// LS_A_L2R
	//LL2UR
	{"BL2TR Att",	BOTH_A1_BL_TR,		Q_BL,	Q_TR,	AFLAG_ACTIVE,	50,		BLK_TIGHT,	LS_R_BL2TR,		LS_R_BL2TR,		200	},	// LS_A_BL2TR
	//LR2UL
	{"BR2TL Att",	BOTH_A1_BR_TL,		Q_BR,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_R_BR2TL,		LS_R_BR2TL,		200	},	// LS_A_BR2TL
	//SLASH RIGHT
	{"R2L Att",		BOTH_A1__R__L,		Q_R,	Q_L,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_R_R2L,		LS_R_R2L,		200 },// LS_A_R2L
	//UR2LL
	{"TR2BL Att",	BOTH_A1_TR_BL,		Q_TR,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_R_TR2BL,		LS_R_TR2BL,		200	},	// LS_A_TR2BL
	//SLASH DOWN
	{"T2B Att",		BOTH_A1_T__B_,		Q_T,	Q_B,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_R_T2B,		LS_R_T2B,		200	},	// LS_A_T2B
	//special attacks
	{"Back Stab",	BOTH_A2_STABBACK1,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_A_BACKSTAB
	{"Back Att",	BOTH_ATTACK_BACK,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_A_BACK
	{"CR Back Att",	BOTH_CROUCHATTACKBACK1,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_A_BACK_CR
	{"RollStab",	BOTH_ROLL_STAB,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_ROLL_STAB
	{"Lunge Att",	BOTH_LUNGE2_B__T_,	Q_B,	Q_T,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_A_LUNGE
	{"Jump Att",	BOTH_FORCELEAP2_T__B_,Q_T,	Q_B,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_A_JUMP_T__B_
	{"Flip Stab",	BOTH_JUMPFLIPSTABDOWN,Q_R,	Q_T,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1_T___R,	200	},	// LS_A_FLIP_STAB
	{"Flip Slash",	BOTH_JUMPFLIPSLASHDOWN1,Q_L,Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1__R_T_,	200	},	// LS_A_FLIP_SLASH
	{"DualJump Atk",BOTH_JUMPATTACK6,	Q_R,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1_BL_TR,	200	},	// LS_JUMPATTACK_DUAL

	{"DualJumpAtkL_A",BOTH_ARIAL_LEFT,	Q_R,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_A_TL2BR,		200	},	// LS_JUMPATTACK_ARIAL_LEFT
	{"DualJumpAtkR_A",BOTH_ARIAL_RIGHT,	Q_R,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_A_TR2BL,		200	},	// LS_JUMPATTACK_ARIAL_RIGHT

	{"DualJumpAtkL_A",BOTH_CARTWHEEL_LEFT,	Q_R,Q_TL,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1_TL_BR,	200	},	// LS_JUMPATTACK_CART_LEFT
	{"DualJumpAtkR_A",BOTH_CARTWHEEL_RIGHT,	Q_R,Q_TR,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1_TR_BL,	200	},	// LS_JUMPATTACK_CART_RIGHT
	
	{"DualJumpAtkLStaff", BOTH_BUTTERFLY_FL1,Q_R,Q_L,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1__L__R,	200	},	// LS_JUMPATTACK_STAFF_LEFT
	{"DualJumpAtkRStaff", BOTH_BUTTERFLY_FR1,Q_R,Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1__R__L,	200	},	// LS_JUMPATTACK_STAFF_RIGHT

	{"ButterflyLeft", BOTH_BUTTERFLY_LEFT,Q_R,Q_L,		AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1__L__R,	200	},	// LS_BUTTERFLY_LEFT
	{"ButterflyRight", BOTH_BUTTERFLY_RIGHT,Q_R,Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1__R__L,	200	},	// LS_BUTTERFLY_RIGHT
	
	{"BkFlip Atk",	BOTH_JUMPATTACK7,	Q_B,	Q_T,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1_T___R,	200	},	// LS_A_BACKFLIP_ATK
	{"DualSpinAtk",	BOTH_SPINATTACK6,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_SPINATTACK_DUAL
	{"StfSpinAtk",	BOTH_SPINATTACK7,	Q_L,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_SPINATTACK
	{"LngLeapAtk",	BOTH_FORCELONGLEAP_ATTACK,Q_R,Q_L,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_LEAP_ATTACK
	{"SwoopAtkR",	BOTH_VS_ATR_S,		Q_R,	Q_T,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_SWOOP_ATTACK_RIGHT
	{"SwoopAtkL",	BOTH_VS_ATL_S,		Q_L,	Q_T,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_SWOOP_ATTACK_LEFT
	{"TauntaunAtkR",BOTH_VT_ATR_S,		Q_R,	Q_T,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_TAUNTAUN_ATTACK_RIGHT
	{"TauntaunAtkL",BOTH_VT_ATL_S,		Q_L,	Q_T,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_TAUNTAUN_ATTACK_LEFT
	{"StfKickFwd",	BOTH_A7_KICK_F,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200	},	// LS_KICK_F
	{"StfKickBack",	BOTH_A7_KICK_B,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200	},	// LS_KICK_B
	{"StfKickRight",BOTH_A7_KICK_R,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200	},	// LS_KICK_R
	{"StfKickLeft",	BOTH_A7_KICK_L,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200	},	// LS_KICK_L
	{"StfKickSpin",	BOTH_A7_KICK_S,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,		LS_READY,		LS_S_R2L,		200	},	// LS_KICK_S
	{"StfKickBkFwd",BOTH_A7_KICK_BF,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,		LS_READY,		LS_S_R2L,		200	},	// LS_KICK_BF
	{"StfKickSplit",BOTH_A7_KICK_RL,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,		LS_READY,		LS_S_R2L,		200	},	// LS_KICK_RL
	{"StfKickFwdAir",BOTH_A7_KICK_F_AIR,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200	},	// LS_KICK_F_AIR
	{"StfKickBackAir",BOTH_A7_KICK_B_AIR,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200	},	// LS_KICK_B_AIR
	{"StfKickRightAir",BOTH_A7_KICK_R_AIR,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200	},	// LS_KICK_R_AIR
	{"StfKickLeftAir",BOTH_A7_KICK_L_AIR,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200	},	// LS_KICK_L_AIR
	{"StabDown",	BOTH_STABDOWN,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200	},	// LS_STABDOWN
	{"StabDownStf",	BOTH_STABDOWN_STAFF,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200	},	// LS_STABDOWN_STAFF
	{"StabDownDual",BOTH_STABDOWN_DUAL,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200	},	// LS_STABDOWN_DUAL
	{"dualspinprot",BOTH_A6_SABERPROTECT,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		500	},	// LS_DUAL_SPIN_PROTECT
	{"StfSoulCal",	BOTH_A7_SOULCAL,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		500	},	// LS_STAFF_SOULCAL
	{"specialfast",	BOTH_A1_SPECIAL,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		2000},	// LS_A1_SPECIAL
	{"specialmed",	BOTH_A2_SPECIAL,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		2000},	// LS_A2_SPECIAL
	{"specialstr",	BOTH_A3_SPECIAL,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		2000},	// LS_A3_SPECIAL
	{"upsidedwnatk",BOTH_FLIP_ATTACK7,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200},	// LS_UPSIDE_DOWN_ATTACK
	{"pullatkstab",	BOTH_PULL_IMPALE_STAB,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200},	// LS_PULL_ATTACK_STAB
	{"pullatkswing",BOTH_PULL_IMPALE_SWING,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200},	// LS_PULL_ATTACK_SWING
	{"AloraSpinAtk",BOTH_ALORA_SPIN_SLASH,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_SPINATTACK_ALORA
	{"Dual FB Atk",	BOTH_A6_FB,			Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_DUAL_FB
	{"Dual LR Atk",	BOTH_A6_LR,			Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_DUAL_LR
	{"StfHiltBash",	BOTH_A7_HILT,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_HILT_BASH

	//starts
	{"TL2BR St",	BOTH_S1_S1_TL,		Q_R,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_A_TL2BR,		LS_A_TL2BR,		200	},	// LS_S_TL2BR
	{"L2R St",		BOTH_S1_S1__L,		Q_R,	Q_L,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_A_L2R,		LS_A_L2R,		200	},	// LS_S_L2R
	{"BL2TR St",	BOTH_S1_S1_BL,		Q_R,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_A_BL2TR,		LS_A_BL2TR,		200	},	// LS_S_BL2TR
	{"BR2TL St",	BOTH_S1_S1_BR,		Q_R,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_A_BR2TL,		LS_A_BR2TL,		200	},	// LS_S_BR2TL
	{"R2L St",		BOTH_S1_S1__R,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_A_R2L,		LS_A_R2L,		200	},	// LS_S_R2L
	{"TR2BL St",	BOTH_S1_S1_TR,		Q_R,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_A_TR2BL,		LS_A_TR2BL,		200	},	// LS_S_TR2BL
	{"T2B St",		BOTH_S1_S1_T_,		Q_R,	Q_T,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_A_T2B,		LS_A_T2B,		200	},	// LS_S_T2B
	
	//returns
	{"TL2BR Ret",	BOTH_R1_BR_S1,		Q_BR,	Q_R,	AFLAG_FINISH,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_R_TL2BR
	{"L2R Ret",		BOTH_R1__R_S1,		Q_R,	Q_R,	AFLAG_FINISH,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_R_L2R
	{"BL2TR Ret",	BOTH_R1_TR_S1,		Q_TR,	Q_R,	AFLAG_FINISH,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_R_BL2TR
	{"BR2TL Ret",	BOTH_R1_TL_S1,		Q_TL,	Q_R,	AFLAG_FINISH,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_R_BR2TL
	{"R2L Ret",		BOTH_R1__L_S1,		Q_L,	Q_R,	AFLAG_FINISH,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_R_R2L
	{"TR2BL Ret",	BOTH_R1_BL_S1,		Q_BL,	Q_R,	AFLAG_FINISH,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_R_TR2BL
	{"T2B Ret",		BOTH_R1_B__S1,		Q_B,	Q_R,	AFLAG_FINISH,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200	},	// LS_R_T2B

	//Transitions
	{"BR2R Trans",	BOTH_T1_BR__R,		Q_BR,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_L2R,		LS_A_R2L,		150	},	//# Fast arc bottom right to right
	{"BR2TR Trans",	BOTH_T1_BR_TR,		Q_BR,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_TR2BL,		150	},	//# Fast arc bottom right to top right		(use: BOTH_T1_TR_BR)
	{"BR2T Trans",	BOTH_T1_BR_T_,		Q_BR,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_T2B,		150	},	//# Fast arc bottom right to top			(use: BOTH_T1_T__BR)
	{"BR2TL Trans",	BOTH_T1_BR_TL,		Q_BR,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BR2TL,		LS_A_TL2BR,		150	},	//# Fast weak spin bottom right to top left
	{"BR2L Trans",	BOTH_T1_BR__L,		Q_BR,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_R2L,		LS_A_L2R,		150	},	//# Fast weak spin bottom right to left
	{"BR2BL Trans",	BOTH_T1_BR_BL,		Q_BR,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TR2BL,		LS_A_BL2TR,		150	},	//# Fast weak spin bottom right to bottom left
	{"R2BR Trans",	BOTH_T1__R_BR,		Q_R,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TL2BR,		LS_A_BR2TL,		150	},	//# Fast arc right to bottom right			(use: BOTH_T1_BR__R)
	{"R2TR Trans",	BOTH_T1__R_TR,		Q_R,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_TR2BL,		150	},	//# Fast arc right to top right
	{"R2T Trans",	BOTH_T1__R_T_,		Q_R,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_T2B,		150	},	//# Fast ar right to top				(use: BOTH_T1_T___R)
	{"R2TL Trans",	BOTH_T1__R_TL,		Q_R,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BR2TL,		LS_A_TL2BR,		150	},	//# Fast arc right to top left
	{"R2L Trans",	BOTH_T1__R__L,		Q_R,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_R2L,		LS_A_L2R,		150	},	//# Fast weak spin right to left
	{"R2BL Trans",	BOTH_T1__R_BL,		Q_R,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TR2BL,		LS_A_BL2TR,		150	},	//# Fast weak spin right to bottom left
	{"TR2BR Trans",	BOTH_T1_TR_BR,		Q_TR,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TL2BR,		LS_A_BR2TL,		150	},	//# Fast arc top right to bottom right
	{"TR2R Trans",	BOTH_T1_TR__R,		Q_TR,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_L2R,		LS_A_R2L,		150	},	//# Fast arc top right to right			(use: BOTH_T1__R_TR)
	{"TR2T Trans",	BOTH_T1_TR_T_,		Q_TR,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_T2B,		150	},	//# Fast arc top right to top				(use: BOTH_T1_T__TR)
	{"TR2TL Trans",	BOTH_T1_TR_TL,		Q_TR,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BR2TL,		LS_A_TL2BR,		150	},	//# Fast arc top right to top left
	{"TR2L Trans",	BOTH_T1_TR__L,		Q_TR,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_R2L,		LS_A_L2R,		150	},	//# Fast arc top right to left
	{"TR2BL Trans",	BOTH_T1_TR_BL,		Q_TR,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TR2BL,		LS_A_BL2TR,		150	},	//# Fast weak spin top right to bottom left
	{"T2BR Trans",	BOTH_T1_T__BR,		Q_T,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TL2BR,		LS_A_BR2TL,		150	},	//# Fast arc top to bottom right
	{"T2R Trans",	BOTH_T1_T___R,		Q_T,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_L2R,		LS_A_R2L,		150	},	//# Fast arc top to right
	{"T2TR Trans",	BOTH_T1_T__TR,		Q_T,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_TR2BL,		150	},	//# Fast arc top to top right
	{"T2TL Trans",	BOTH_T1_T__TL,		Q_T,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BR2TL,		LS_A_TL2BR,		150	},	//# Fast arc top to top left
	{"T2L Trans",	BOTH_T1_T___L,		Q_T,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_R2L,		LS_A_L2R,		150	},	//# Fast arc top to left
	{"T2BL Trans",	BOTH_T1_T__BL,		Q_T,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TR2BL,		LS_A_BL2TR,		150	},	//# Fast arc top to bottom left
	{"TL2BR Trans",	BOTH_T1_TL_BR,		Q_TL,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TL2BR,		LS_A_BR2TL,		150	},	//# Fast weak spin top left to bottom right
	{"TL2R Trans",	BOTH_T1_TL__R,		Q_TL,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_L2R,		LS_A_R2L,		150	},	//# Fast arc top left to right			(use: BOTH_T1__R_TL)
	{"TL2TR Trans",	BOTH_T1_TL_TR,		Q_TL,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_TR2BL,		150	},	//# Fast arc top left to top right			(use: BOTH_T1_TR_TL)
	{"TL2T Trans",	BOTH_T1_TL_T_,		Q_TL,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_T2B,		150	},	//# Fast arc top left to top				(use: BOTH_T1_T__TL)
	{"TL2L Trans",	BOTH_T1_TL__L,		Q_TL,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_R2L,		LS_A_L2R,		150	},	//# Fast arc top left to left				(use: BOTH_T1__L_TL)
	{"TL2BL Trans",	BOTH_T1_TL_BL,		Q_TL,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TR2BL,		LS_A_BL2TR,		150	},	//# Fast arc top left to bottom left
	{"L2BR Trans",	BOTH_T1__L_BR,		Q_L,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TL2BR,		LS_A_BR2TL,		150	},	//# Fast weak spin left to bottom right
	{"L2R Trans",	BOTH_T1__L__R,		Q_L,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_L2R,		LS_A_R2L,		150	},	//# Fast weak spin left to right
	{"L2TR Trans",	BOTH_T1__L_TR,		Q_L,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_TR2BL,		150	},	//# Fast arc left to top right			(use: BOTH_T1_TR__L)
	{"L2T Trans",	BOTH_T1__L_T_,		Q_L,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_T2B,		150	},	//# Fast arc left to top				(use: BOTH_T1_T___L)
	{"L2TL Trans",	BOTH_T1__L_TL,		Q_L,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BR2TL,		LS_A_TL2BR,		150	},	//# Fast arc left to top left
	{"L2BL Trans",	BOTH_T1__L_BL,		Q_L,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TR2BL,		LS_A_BL2TR,		150	},	//# Fast arc left to bottom left			(use: BOTH_T1_BL__L)
	{"BL2BR Trans",	BOTH_T1_BL_BR,		Q_BL,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TL2BR,		LS_A_BR2TL,		150	},	//# Fast weak spin bottom left to bottom right
	{"BL2R Trans",	BOTH_T1_BL__R,		Q_BL,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_L2R,		LS_A_R2L,		150	},	//# Fast weak spin bottom left to right
	{"BL2TR Trans",	BOTH_T1_BL_TR,		Q_BL,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_TR2BL,		150	},	//# Fast weak spin bottom left to top right
	{"BL2T Trans",	BOTH_T1_BL_T_,		Q_BL,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_T2B,		150	},	//# Fast arc bottom left to top			(use: BOTH_T1_T__BL)
	{"BL2TL Trans",	BOTH_T1_BL_TL,		Q_BL,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BR2TL,		LS_A_TL2BR,		150	},	//# Fast arc bottom left to top left		(use: BOTH_T1_TL_BL)
	{"BL2L Trans",	BOTH_T1_BL__L,		Q_BL,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_R2L,		LS_A_L2R,		150	},	//# Fast arc bottom left to left

	//Bounces
	{"Bounce BR",	BOTH_B1_BR___,		Q_BR,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TL2BR,		LS_T1_BR_TR,	150	},	
	{"Bounce R",	BOTH_B1__R___,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_L2R,		LS_T1__R__L,	150	},	
	{"Bounce TR",	BOTH_B1_TR___,		Q_TR,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_T1_TR_TL,	150	},	
	{"Bounce T",	BOTH_B1_T____,		Q_T,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_T1_T__BL,	150	},	
	{"Bounce TL",	BOTH_B1_TL___,		Q_TL,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BR2TL,		LS_T1_TL_TR,	150	},	
	{"Bounce L",	BOTH_B1__L___,		Q_L,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_R2L,		LS_T1__L__R,	150	},	
	{"Bounce BL",	BOTH_B1_BL___,		Q_BL,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TR2BL,		LS_T1_BL_TR,	150	},	

	//Deflected attacks (like bounces, but slide off enemy saber, not straight back)
	{"Deflect BR",	BOTH_D1_BR___,		Q_BR,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TL2BR,		LS_T1_BR_TR,	150	},	
	{"Deflect R",	BOTH_D1__R___,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_L2R,		LS_T1__R__L,	150	},	
	{"Deflect TR",	BOTH_D1_TR___,		Q_TR,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_T1_TR_TL,	150	},	
	{"Deflect T",	BOTH_B1_T____,		Q_T,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_T1_T__BL,	150	},	
	{"Deflect TL",	BOTH_D1_TL___,		Q_TL,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BR2TL,		LS_T1_TL_TR,	150	},	
	{"Deflect L",	BOTH_D1__L___,		Q_L,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_R2L,		LS_T1__L__R,	150	},	
	{"Deflect BL",	BOTH_D1_BL___,		Q_BL,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TR2BL,		LS_T1_BL_TR,	150	},	
	{"Deflect B",	BOTH_D1_B____,		Q_B,	Q_B,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_T1_T__BL,	150	},	

	//Reflected attacks
	{"Reflected BR",BOTH_V1_BR_S1,		Q_BR,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_READY,		LS_READY,	150	},//	LS_V1_BR
	{"Reflected R",	BOTH_V1__R_S1,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_READY,		LS_READY,	150	},//	LS_V1__R
	{"Reflected TR",BOTH_V1_TR_S1,		Q_TR,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_READY,		LS_READY,	150	},//	LS_V1_TR
	{"Reflected T",	BOTH_V1_T__S1,		Q_T,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_READY,		LS_READY,	150	},//	LS_V1_T_
	{"Reflected TL",BOTH_V1_TL_S1,		Q_TL,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_READY,		LS_READY,	150	},//	LS_V1_TL
	{"Reflected L",	BOTH_V1__L_S1,		Q_L,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_READY,		LS_READY,	150	},//	LS_V1__L
	{"Reflected BL",BOTH_V1_BL_S1,		Q_BL,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_READY,		LS_READY,	150	},//	LS_V1_BL
	{"Reflected B",	BOTH_V1_B__S1,		Q_B,	Q_B,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_READY,		LS_READY,	150	},//	LS_V1_B_

	// Broken parries
	{"BParry Top",	BOTH_H1_S1_T_,		Q_T,	Q_B,	AFLAG_ACTIVE,	50,		BLK_NO,	LS_READY,		LS_READY,		150	},	// LS_PARRY_UP,
	{"BParry UR",	BOTH_H1_S1_TR,		Q_TR,	Q_BL,	AFLAG_ACTIVE,	50,		BLK_NO,	LS_READY,		LS_READY,		150	},	// LS_PARRY_UR,
	{"BParry UL",	BOTH_H1_S1_TL,		Q_TL,	Q_BR,	AFLAG_ACTIVE,	50,		BLK_NO,	LS_READY,		LS_READY,		150	},	// LS_PARRY_UL,
	//[BugFix33]
	//the bottom right and bottom left animations were accidently switched.
	{"BParry LR",	BOTH_H1_S1_BR,		Q_BL,	Q_TR,	AFLAG_ACTIVE,	50,		BLK_NO,	LS_READY,		LS_READY,		150	},	// LS_PARRY_LR,
	{"BParry Bot",	BOTH_H1_S1_B_,		Q_B,	Q_T,	AFLAG_ACTIVE,	50,		BLK_NO,	LS_READY,		LS_READY,		150	},	// LS_PARRY_LR
	{"BParry LL",	BOTH_H1_S1_BL,		Q_BR,	Q_TL,	AFLAG_ACTIVE,	50,		BLK_NO,	LS_READY,		LS_READY,		150	},	// LS_PARRY_LL
	//{"BParry LR",	BOTH_H1_S1_BL,		Q_BL,	Q_TR,	AFLAG_ACTIVE,	50,		BLK_NO,	LS_READY,		LS_READY,		150	},	// LS_PARRY_LR,
	//{"BParry Bot",	BOTH_H1_S1_B_,		Q_B,	Q_T,	AFLAG_ACTIVE,	50,		BLK_NO,	LS_READY,		LS_READY,		150	},	// LS_PARRY_LL
	//{"BParry LL",	BOTH_H1_S1_BR,		Q_BR,	Q_TL,	AFLAG_ACTIVE,	50,		BLK_NO,	LS_READY,		LS_READY,		150	},	// LS_PARRY_LL
	//[/BugFix33]

	// Knockaways
	{"Knock Top",	BOTH_K1_S1_T_,		Q_R,	Q_T,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BL2TR,		LS_T1_T__BR,		150	},	// LS_PARRY_UP,
	{"Knock UR",	BOTH_K1_S1_TR,		Q_R,	Q_TR,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BL2TR,		LS_T1_TR__R,		150	},	// LS_PARRY_UR,
	{"Knock UL",	BOTH_K1_S1_TL,		Q_R,	Q_TL,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BR2TL,		LS_T1_TL__L,		150	},	// LS_PARRY_UL,
	//[BugFix33]
	//the bottom right and bottom left animations were accidently switched.
	{"Knock LR",	BOTH_K1_S1_BR,		Q_R,	Q_BL,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_TL2BR,		LS_T1_BL_TL,		150	},	// LS_PARRY_LR,
	{"Knock LL",	BOTH_K1_S1_BL,		Q_R,	Q_BR,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_TR2BL,		LS_T1_BR_TR,		150	},	// LS_PARRY_LL
	//{"Knock LR",	BOTH_K1_S1_BL,		Q_R,	Q_BL,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_TL2BR,		LS_T1_BL_TL,		150	},	// LS_PARRY_LR,
	//{"Knock LL",	BOTH_K1_S1_BR,		Q_R,	Q_BR,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_TR2BL,		LS_T1_BR_TR,		150	},	// LS_PARRY_LL
	//[/BugFix33]

	// Parry
	{"Parry Top",	BOTH_P1_S1_T_,		Q_R,	Q_T,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BL2TR,		LS_A_T2B,		150	},	// LS_PARRY_UP,
	{"Parry UR",	BOTH_P1_S1_TR,		Q_R,	Q_TL,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BL2TR,		LS_A_TR2BL,		150	},	// LS_PARRY_UR,
	{"Parry UL",	BOTH_P1_S1_TL,		Q_R,	Q_TR,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BR2TL,		LS_A_TL2BR,		150	},	// LS_PARRY_UL,
	//[BugFix33]
	//the bottom right and bottom left animations were accidently switched.
	{"Parry LR",	BOTH_P1_S1_BR,		Q_R,	Q_BR,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_TL2BR,		LS_A_BR2TL,		150	},	// LS_PARRY_LR,
	{"Parry LL",	BOTH_P1_S1_BL,		Q_R,	Q_BL,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_TR2BL,		LS_A_BL2TR,		150	},	// LS_PARRY_LL
	//{"Parry LR",	BOTH_P1_S1_BL,		Q_R,	Q_BR,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_TL2BR,		LS_A_BR2TL,		150	},	// LS_PARRY_LR,
	//{"Parry LL",	BOTH_P1_S1_BR,		Q_R,	Q_BL,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_TR2BL,		LS_A_BL2TR,		150	},	// LS_PARRY_LL
	//[/BugFix33]

	// Reflecting a missile
	{"Reflect Top",	BOTH_P1_S1_T_,		Q_R,	Q_T,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BL2TR,		LS_A_T2B,		300	},	// LS_PARRY_UP,
	{"Reflect UR",	BOTH_P1_S1_TL,		Q_R,	Q_TR,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BR2TL,		LS_A_TL2BR,		300	},	// LS_PARRY_UR,
	{"Reflect UL",	BOTH_P1_S1_TR,		Q_R,	Q_TL,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BL2TR,		LS_A_TR2BL,		300	},	// LS_PARRY_UL,
	{"Reflect LR",	BOTH_P1_S1_BR,		Q_R,	Q_BL,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_TR2BL,		LS_A_BL2TR,		300	},	// LS_PARRY_LR
	{"Reflect LL",	BOTH_P1_S1_BL,		Q_R,	Q_BR,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_TL2BR,		LS_A_BR2TL,		300	},	// LS_PARRY_LL,
};


int transitionMove[Q_NUM_QUADS][Q_NUM_QUADS] = 
{
	{	LS_NONE,		LS_T1_BR__R,	LS_T1_BR_TR,	LS_T1_BR_T_,	LS_T1_BR_TL,	LS_T1_BR__L,	LS_T1_BR_BL,	LS_NONE		},
	{	LS_T1__R_BR,	LS_NONE,		LS_T1__R_TR,	LS_T1__R_T_,	LS_T1__R_TL,	LS_T1__R__L,	LS_T1__R_BL,	LS_NONE		},
	{	LS_T1_TR_BR,	LS_T1_TR__R,	LS_NONE,		LS_T1_TR_T_,	LS_T1_TR_TL,	LS_T1_TR__L,	LS_T1_TR_BL,	LS_NONE		},
	{	LS_T1_T__BR,	LS_T1_T___R,	LS_T1_T__TR,	LS_NONE,		LS_T1_T__TL,	LS_T1_T___L,	LS_T1_T__BL,	LS_NONE		},
	{	LS_T1_TL_BR,	LS_T1_TL__R,	LS_T1_TL_TR,	LS_T1_TL_T_,	LS_NONE,		LS_T1_TL__L,	LS_T1_TL_BL,	LS_NONE		},
	{	LS_T1__L_BR,	LS_T1__L__R,	LS_T1__L_TR,	LS_T1__L_T_,	LS_T1__L_TL,	LS_NONE,		LS_T1__L_BL,	LS_NONE		},
	{	LS_T1_BL_BR,	LS_T1_BL__R,	LS_T1_BL_TR,	LS_T1_BL_T_,	LS_T1_BL_TL,	LS_T1_BL__L,	LS_NONE,		LS_NONE		},
	{	LS_T1_BL_BR,	LS_T1_BR__R,	LS_T1_BR_TR,	LS_T1_BR_T_,	LS_T1_BR_TL,	LS_T1_BR__L,	LS_T1_BR_BL,	LS_NONE		},
};


saberMoveName_t PM_AttackMoveForQuad( int quad )
{
	switch ( quad )
	{
	case Q_B:
	case Q_BR:
		return LS_A_BR2TL;
		break;
	case Q_R:
		return LS_A_R2L;
		break;
	case Q_TR:
		return LS_A_TR2BL;
		break;
	case Q_T:
		return LS_A_T2B;
		break;
	case Q_TL:
		return LS_A_TL2BR;
		break;
	case Q_L:
		return LS_A_L2R;
		break;
	case Q_BL:
		return LS_A_BL2TR;
		break;
	}
	return LS_NONE;
}

//[SaberSys]
//qboolean PM_SaberKataDone(int curmove, int newmove);

int PM_ReturnforQuad( int quad );
//[/SaberSys]

int PM_SaberAnimTransitionAnim( int curmove, int newmove )
{//racc - find the transition move from the current move to the new move.
	int retmove = newmove;
	if ( curmove == LS_READY )
	{//just standing there
		switch ( newmove )
		{
		case LS_A_TL2BR:
		case LS_A_L2R:
		case LS_A_BL2TR:
		case LS_A_BR2TL:
		case LS_A_R2L:
		case LS_A_TR2BL:
		case LS_A_T2B:
			//transition is the start
			retmove = LS_S_TL2BR + (newmove-LS_A_TL2BR);
			break;
		}
	}
	else
	{
		switch ( newmove )
		{
		//transitioning to ready pose
		case LS_READY:
			switch ( curmove )
			{
			//transitioning from an attack
			case LS_A_TL2BR:
			case LS_A_L2R:
			case LS_A_BL2TR:
			case LS_A_BR2TL:
			case LS_A_R2L:
			case LS_A_TR2BL:
			case LS_A_T2B:
				//transition is the return
				retmove = LS_R_TL2BR + (newmove-LS_A_TL2BR);
				break;
			//[SaberSys]
			//bounces
			case LS_B1_BR:
			case LS_B1__R:
			case LS_B1_TR:
			case LS_B1_T_:
			case LS_B1_TL:
			case LS_B1__L:
			case LS_B1_BL:
			//transitioning from a parry/reflection/knockaway/broken parry
			case LS_PARRY_UP:
			case LS_PARRY_UR:
			case LS_PARRY_UL:
			case LS_PARRY_LR:
			case LS_PARRY_LL:
			case LS_REFLECT_UP:
			case LS_REFLECT_UR:
			case LS_REFLECT_UL:
			case LS_REFLECT_LR:
			case LS_REFLECT_LL:
			case LS_K1_T_:
			case LS_K1_TR:
			case LS_K1_TL:
			case LS_K1_BR:
			case LS_K1_BL:
			case LS_V1_BR:
			case LS_V1__R:
			case LS_V1_TR:
			case LS_V1_T_:
			case LS_V1_TL:
			case LS_V1__L:
			case LS_V1_BL:
			case LS_V1_B_:
			case LS_H1_T_:
			case LS_H1_TR:
			case LS_H1_TL:
			case LS_H1_BR:
			case LS_H1_BL:
				retmove = PM_ReturnforQuad(saberMoveData[curmove].endQuad);
				break;
			//[/SaberSys]
			}
			break;
		//transitioning to an attack
		case LS_A_TL2BR:
		case LS_A_L2R:
		case LS_A_BL2TR:
		case LS_A_BR2TL:
		case LS_A_R2L:
		case LS_A_TR2BL:
		case LS_A_T2B:
			if ( newmove == curmove )
			{
				//[SaberSys]
				//racc - removing the chaining requirements on the saber system.
				retmove = transitionMove[saberMoveData[curmove].endQuad][saberMoveData[newmove].startQuad];

				/* racc - basejka code.
				//going into an attack
				if ( PM_SaberKataDone( curmove, newmove ) )
				{//done with this kata, must return to ready before attack again
					retmove = LS_R_TL2BR + (newmove-LS_A_TL2BR);
				}
				else
				{//okay to chain to another attack
					retmove = transitionMove[saberMoveData[curmove].endQuad][saberMoveData[newmove].startQuad];
				}
				*/
				//[/SaberSys]
			}
			else if ( saberMoveData[curmove].endQuad == saberMoveData[newmove].startQuad )
			{//new move starts from same quadrant
				retmove = newmove;
			}
			else
			{
				switch ( curmove )
				{
				//transitioning from an attack
				case LS_A_TL2BR:
				case LS_A_L2R:
				case LS_A_BL2TR:
				case LS_A_BR2TL:
				case LS_A_R2L:
				case LS_A_TR2BL:
				case LS_A_T2B:
				case LS_D1_BR:
				case LS_D1__R:
				case LS_D1_TR:
				case LS_D1_T_:
				case LS_D1_TL:
				case LS_D1__L:
				case LS_D1_BL:
				case LS_D1_B_:
					retmove = transitionMove[saberMoveData[curmove].endQuad][saberMoveData[newmove].startQuad];
					break;
				//transitioning from a return
				case LS_R_TL2BR:
				case LS_R_L2R:
				case LS_R_BL2TR:
				case LS_R_BR2TL:
				case LS_R_R2L:
				case LS_R_TR2BL:
				case LS_R_T2B:
					//transition is the start
					retmove = LS_S_TL2BR + (newmove-LS_A_TL2BR);
					break;
				//transitioning from a bounce
				//[SaberSys]
				//bounces should transition to transitions before attacks.
				case LS_B1_BR:
				case LS_B1__R:
				case LS_B1_TR:
				case LS_B1_T_:
				case LS_B1_TL:
				case LS_B1__L:
				case LS_B1_BL:
				//[/SaberSys]
				/*
				case LS_BOUNCE_UL2LL:
				case LS_BOUNCE_LL2UL:
				case LS_BOUNCE_L2LL:
				case LS_BOUNCE_L2UL:
				case LS_BOUNCE_UR2LR:
				case LS_BOUNCE_LR2UR:
				case LS_BOUNCE_R2LR:
				case LS_BOUNCE_R2UR:
				case LS_BOUNCE_TOP:
				case LS_OVER_UR2UL:
				case LS_OVER_UL2UR:
				case LS_BOUNCE_UR:
				case LS_BOUNCE_UL:
				case LS_BOUNCE_LR:
				case LS_BOUNCE_LL:
				*/
				//transitioning from a parry/reflection/knockaway/broken parry
				case LS_PARRY_UP:
				case LS_PARRY_UR:
				case LS_PARRY_UL:
				case LS_PARRY_LR:
				case LS_PARRY_LL:
				case LS_REFLECT_UP:
				case LS_REFLECT_UR:
				case LS_REFLECT_UL:
				case LS_REFLECT_LR:
				case LS_REFLECT_LL:
				case LS_K1_T_:
				case LS_K1_TR:
				case LS_K1_TL:
				case LS_K1_BR:
				case LS_K1_BL:
				case LS_V1_BR:
				case LS_V1__R:
				case LS_V1_TR:
				case LS_V1_T_:
				case LS_V1_TL:
				case LS_V1__L:
				case LS_V1_BL:
				case LS_V1_B_:
				case LS_H1_T_:
				case LS_H1_TR:
				case LS_H1_TL:
				case LS_H1_BR:
				case LS_H1_BL:
					retmove = transitionMove[saberMoveData[curmove].endQuad][saberMoveData[newmove].startQuad];
					break;
				//NB: transitioning from transitions is fine
				}
			}
			break;
		//transitioning to any other anim is not supported
		}
	}

	if ( retmove == LS_NONE )
	{
		return newmove;
	}

	return retmove;
}

extern qboolean BG_InKnockDown( int anim );
saberMoveName_t PM_CheckStabDown( void )
{
	vec3_t faceFwd, facingAngles;
	vec3_t fwd;
	bgEntity_t *ent = NULL;
	trace_t tr;
	//yeah, vm's may complain, but.. who cares!
	vec3_t trmins = {-15, -15, -15};
	vec3_t trmaxs = {15, 15, 15};

	saberInfo_t *saber1 = BG_MySaber( pm->ps->clientNum, 0 );
	saberInfo_t *saber2 = BG_MySaber( pm->ps->clientNum, 1 );
	if ( saber1
		&& (saber1->saberFlags&SFL_NO_STABDOWN) )
	{
		return LS_NONE;
	}
	if ( saber2
		&& (saber2->saberFlags&SFL_NO_STABDOWN) )
	{
		return LS_NONE;
	}

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE )
	{//sorry must be on ground!
		return LS_NONE;
	}
	if ( pm->ps->clientNum < MAX_CLIENTS )
	{//player
		pm->ps->velocity[2] = 0;
		pm->cmd.upmove = 0;
	}

	VectorSet(facingAngles, 0, pm->ps->viewangles[YAW], 0);
	AngleVectors( facingAngles, faceFwd, NULL, NULL );

	//FIXME: need to only move forward until we bump into our target...?
	VectorMA(pm->ps->origin, 164.0f, faceFwd, fwd);

	pm->trace(&tr, pm->ps->origin, trmins, trmaxs, fwd, pm->ps->clientNum, MASK_PLAYERSOLID);

	if (tr.entityNum < ENTITYNUM_WORLD)
	{
		ent = PM_BGEntForNum(tr.entityNum);
	}

	if ( ent &&
		(ent->s.eType == ET_PLAYER || ent->s.eType == ET_NPC) &&
		BG_InKnockDown( ent->s.legsAnim ) )
	{//guy is on the ground below me, do a top-down attack
		if ( pm->ps->fd.saberAnimLevel == SS_DUAL )
		{
			return LS_STABDOWN_DUAL;
		}
		else if ( pm->ps->fd.saberAnimLevel == SS_STAFF )
		{
			return LS_STABDOWN_STAFF;
		}
		else
		{
			return LS_STABDOWN;
		}
	}
	return LS_NONE;
}

int PM_SaberMoveQuadrantForMovement( usercmd_t *ucmd )
{
	if ( ucmd->rightmove > 0 )
	{//moving right
		if ( ucmd->forwardmove > 0 )
		{//forward right = TL2BR slash
			return Q_TL;
		}
		else if ( ucmd->forwardmove < 0 )
		{//backward right = BL2TR uppercut
			return Q_BL;
		}
		else
		{//just right is a left slice
			return Q_L;
		}
	}
	else if ( ucmd->rightmove < 0 )
	{//moving left
		if ( ucmd->forwardmove > 0 )
		{//forward left = TR2BL slash
			return Q_TR;
		}
		else if ( ucmd->forwardmove < 0 )
		{//backward left = BR2TL uppercut
			return Q_BR;
		}
		else
		{//just left is a right slice
			return Q_R;
		}
	}
	else
	{//not moving left or right
		if ( ucmd->forwardmove > 0 )
		{//forward= T2B slash
			return Q_T;
		}
		else if ( ucmd->forwardmove < 0 )
		{//backward= T2B slash	//or B2T uppercut?
			return Q_T;
		}
		else
		{//Not moving at all
			return Q_R;
		}
	}
}

//===================================================================
qboolean PM_SaberInBounce( int move )
{
	if ( move >= LS_B1_BR && move <= LS_B1_BL )
	{
		return qtrue;
	}
	if ( move >= LS_D1_BR && move <= LS_D1_BL )
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberInTransition( int move );

int saberMoveTransitionAngle[Q_NUM_QUADS][Q_NUM_QUADS] = 
{
//		Q_BR,Q_BR,	Q_BR,Q_R,	Q_BR,Q_TR,	Q_BR,Q_T,	Q_BR,Q_TL,	Q_BR,Q_L,	Q_BR,Q_BL,	Q_BR,Q_B,
	{	0,			45,			90,			135,		180,		215,		270,		45			},

//		Q_R,Q_BR,	Q_R,Q_R,	Q_R,Q_TR,	Q_R,Q_T,	Q_R,Q_TL,	Q_R,Q_L,	Q_R,Q_BL,	Q_R,Q_B,
	{	45,			0,			45,			90,			135,		180,		215,		90			},

//		Q_TR,Q_BR,	Q_TR,Q_R,	Q_TR,Q_TR,	Q_TR,Q_T,	Q_TR,Q_TL,	Q_TR,Q_L,	Q_TR,Q_BL,	Q_TR,Q_B,
	{	90,			45,			0,			45,			90,			135,		180,		135			},

//		Q_T,Q_BR,	Q_T,Q_R,	Q_T,Q_TR,	Q_T,Q_T,	Q_T,Q_TL,	Q_T,Q_L,	Q_T,Q_BL,	Q_T,Q_B,
	{	135,		90,			45,			0,			45,			90,			135,		180			},

//		Q_TL,Q_BR,	Q_TL,Q_R,	Q_TL,Q_TR,	Q_TL,Q_T,	Q_TL,Q_TL,	Q_TL,Q_L,	Q_TL,Q_BL,	Q_TL,Q_B,
	{	180,		135,		90,			45,			0,			45,			90,			135			},

//		Q_L,Q_BR,	Q_L,Q_R,	Q_L,Q_TR,	Q_L,Q_T,	Q_L,Q_TL,	Q_L,Q_L,	Q_L,Q_BL,	Q_L,Q_B,
	{	215,		180,		135,		90,			45,			0,			45,			90			},

//		Q_BL,Q_BR,	Q_BL,Q_R,	Q_BL,Q_TR,	Q_BL,Q_T,	Q_BL,Q_TL,	Q_BL,Q_L,	Q_BL,Q_BL,	Q_BL,Q_B,
	{	270,		215,		180,		135,		90,			45,			0,			45			},

//		Q_B,Q_BR,	Q_B,Q_R,	Q_B,Q_TR,	Q_B,Q_T,	Q_B,Q_TL,	Q_B,Q_L,	Q_B,Q_BL,	Q_B,Q_B,
	{	45,			90,			135,		180,		135,		90,			45,			0			},
};

//[SaberSys]
//racc - removing the attack chain (kata) stuff.  The OJP saber code now handles it.
/*
int PM_SaberAttackChainAngle( int move1, int move2 )
{
	if ( move1 == -1 || move2 == -1 )
	{
		return -1;
	}
	return saberMoveTransitionAngle[saberMoveData[move1].endQuad][saberMoveData[move2].startQuad];
}

qboolean PM_SaberKataDone(int curmove, int newmove)
{
	if (pm->ps->m_iVehicleNum)
	{ //never continue kata on vehicle
		if (pm->ps->saberAttackChainCount > 0)
		{
			return qtrue;
		}
	}

	if ( pm->ps->fd.saberAnimLevel == SS_DESANN || pm->ps->fd.saberAnimLevel == SS_TAVION )
	{//desann and tavion can link up as many attacks as they want
		return qfalse;
	}

	if ( pm->ps->fd.saberAnimLevel == SS_STAFF )
	{
		//TEMP: for now, let staff attacks infinitely chain
		return qfalse;
	}
	else if ( pm->ps->fd.saberAnimLevel == SS_DUAL )
	{
		//TEMP: for now, let staff attacks infinitely chain
		return qfalse;
	}
	else if ( pm->ps->fd.saberAnimLevel == FORCE_LEVEL_3 )
	{
		if ( curmove == LS_NONE || newmove == LS_NONE )
		{
			if ( pm->ps->fd.saberAnimLevel >= FORCE_LEVEL_3 && pm->ps->saberAttackChainCount > PM_irand_timesync( 0, 1 ) )
			{
				return qtrue;
			}
		}
		else if ( pm->ps->saberAttackChainCount > PM_irand_timesync( 2, 3 ) )
		{
			return qtrue;
		}
		else if ( pm->ps->saberAttackChainCount > 0 )
		{
			int chainAngle = PM_SaberAttackChainAngle( curmove, newmove );
			if ( chainAngle < 135 || chainAngle > 215 )
			{//if trying to chain to a move that doesn't continue the momentum
				return qtrue;
			}
			else if ( chainAngle == 180 )
			{//continues the momentum perfectly, allow it to chain 66% of the time
				if ( pm->ps->saberAttackChainCount > 1 )
				{
					return qtrue;
				}
			}
			else
			{//would continue the movement somewhat, 50% chance of continuing
				if ( pm->ps->saberAttackChainCount > 2 )
				{
					return qtrue;
				}
			}
		}
	}
	else 
	{//Perhaps have chainAngle influence fast and medium chains as well? For now, just do level 3.
		if (newmove == LS_A_TL2BR ||
			newmove == LS_A_L2R ||
			newmove == LS_A_BL2TR ||
			newmove == LS_A_BR2TL ||
			newmove == LS_A_R2L ||
			newmove == LS_A_TR2BL )
		{ //lower chaining tolerance for spinning saber anims
			int chainTolerance;

			if (pm->ps->fd.saberAnimLevel == FORCE_LEVEL_1)
			{
				chainTolerance = 5;
			}
			else
			{
				chainTolerance = 3;
			}

			if (pm->ps->saberAttackChainCount >= chainTolerance && PM_irand_timesync(1, pm->ps->saberAttackChainCount) > chainTolerance)
			{
				return qtrue;
			}
		}
		if ( pm->ps->fd.saberAnimLevel == FORCE_LEVEL_2 && pm->ps->saberAttackChainCount > PM_irand_timesync( 2, 5 ) )
		{
			return qtrue;
		}
	}
	return qfalse;
}
*/
//[/SaberSys]

void PM_SetAnimFrame( playerState_t *gent, int frame, qboolean torso, qboolean legs )
{
	gent->saberLockFrame = frame;
}

int PM_SaberLockWinAnim( qboolean victory, qboolean superBreak )
{//racc - determines which saberlock animation to use a player won/lost a saberlock.  
	//This function only applies to single on single saberlocks.
	int winAnim = -1;
	switch ( pm->ps->torsoAnim )
	{
/*
	default:
#ifndef FINAL_BUILD
		Com_Printf( S_COLOR_RED"ERROR-PM_SaberLockBreak: %s not in saberlock anim, anim = (%d)%s\n", pm->gent->NPC_type, pm->ps->torsoAnim, animTable[pm->ps->torsoAnim].name );
#endif
*/
	case BOTH_BF2LOCK:
		if ( superBreak )
		{
			winAnim = BOTH_LK_S_S_T_SB_1_W;
		}
		else if ( !victory )
		{
			winAnim = BOTH_BF1BREAK;
		}
		else
		{
			pm->ps->saberMove = LS_A_T2B;
			winAnim = BOTH_A3_T__B_;
		}
		break;
	case BOTH_BF1LOCK:
		if ( superBreak )
		{
			winAnim = BOTH_LK_S_S_T_SB_1_W;
		}
		else if ( !victory )
		{
			winAnim = BOTH_KNOCKDOWN4;
		}
		else
		{
			pm->ps->saberMove = LS_K1_T_;
			winAnim = BOTH_K1_S1_T_;
		}
		break;
	case BOTH_CWCIRCLELOCK:
		if ( superBreak )
		{
			winAnim = BOTH_LK_S_S_S_SB_1_W;
		}
		else if ( !victory )
		{
			pm->ps->saberMove = LS_V1_BL;//pm->ps->saberBounceMove = 
			pm->ps->saberBlocked = BLOCKED_PARRY_BROKEN;
			winAnim = BOTH_V1_BL_S1;
		}
		else
		{
			winAnim = BOTH_CWCIRCLEBREAK;
		}
		break;
	case BOTH_CCWCIRCLELOCK:
		if ( superBreak )
		{
			winAnim = BOTH_LK_S_S_S_SB_1_W;
		}
		else if ( !victory )
		{
			pm->ps->saberMove = LS_V1_BR;//pm->ps->saberBounceMove = 
			pm->ps->saberBlocked = BLOCKED_PARRY_BROKEN;
			winAnim = BOTH_V1_BR_S1;
		}
		else
		{
			winAnim = BOTH_CCWCIRCLEBREAK;
		}
		break;
	default:
		//must be using new system:
		break;
	}
	if ( winAnim != -1 )
	{
		PM_SetAnim( SETANIM_BOTH, winAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );
		pm->ps->weaponTime = pm->ps->torsoTimer;
		pm->ps->saberBlocked = BLOCKED_NONE;
		pm->ps->weaponstate = WEAPON_FIRING;
		/*
		if ( superBreak 
			&& winAnim != BOTH_LK_ST_DL_T_SB_1_W )
		{//going to attack with saber, do a saber trail
			pm->ps->SaberActivateTrail( 200 );
		}
		*/
	}

//[SaberLockSys]
/*
#ifdef QAGAME
	G_Printf("%i: %i:  SaberLock Win Animation TorsoTime %i LegsTimer %i\n", level.time, 
		pm->ps->clientNum, pm->ps->torsoTimer, pm->ps->legsTimer);
#endif
*/
//[/SaberLockSys]

	return winAnim;
}

//[SaberSys]
extern void PM_DoPunch(void);
//[/SaberSys]

#ifdef QAGAME //including game headers on cgame is FORBIDDEN ^_^

//[SaberLockSys]
//moved this up to allow for additional debugger messages.
//#include "g_local.h"
//[/SaberLockSys]
extern void NPC_SetAnim(gentity_t *ent, int setAnimParts, int anim, int setAnimFlags);
extern gentity_t g_entities[];

#elif defined CGAME

#include "../cgame/cg_local.h" //ahahahahhahahaha@$!$!

#endif

int PM_SaberLockLoseAnim( playerState_t *genemy, qboolean victory, qboolean superBreak )
{ 
	int loseAnim = -1;
	switch ( genemy->torsoAnim )
	{
/*
	default:
#ifndef FINAL_BUILD
		Com_Printf( S_COLOR_RED"ERROR-PM_SaberLockBreak: %s not in saberlock anim, anim = (%d)%s\n", genemy->NPC_type, genemy->client->ps.torsoAnim, animTable[genemy->client->ps.torsoAnim].name );
#endif
*/
	case BOTH_BF2LOCK:
		if ( superBreak )
		{
			loseAnim = BOTH_LK_S_S_T_SB_1_L;
		}
		else if ( !victory )
		{
			loseAnim = BOTH_BF1BREAK;
		}
		else
		{
			if ( !victory )
			{//no-one won
				genemy->saberMove = LS_K1_T_;
				loseAnim = BOTH_K1_S1_T_;
			}
			else
			{//FIXME: this anim needs to transition back to ready when done
				loseAnim = BOTH_BF1BREAK;
			}
		}
		break;
	case BOTH_BF1LOCK:
		if ( superBreak )
		{
			loseAnim = BOTH_LK_S_S_T_SB_1_L;
		}
		else if ( !victory )
		{
			loseAnim = BOTH_KNOCKDOWN4;
		}
		else
		{
			if ( !victory )
			{//no-one won
				genemy->saberMove = LS_A_T2B;
				loseAnim = BOTH_A3_T__B_;
			}
			else
			{
				loseAnim = BOTH_KNOCKDOWN4;
			}
		}
		break;
	case BOTH_CWCIRCLELOCK:
		if ( superBreak )
		{
			loseAnim = BOTH_LK_S_S_S_SB_1_L;
		}
		else if ( !victory )
		{
			genemy->saberMove = LS_V1_BL;//genemy->saberBounceMove = 
			genemy->saberBlocked = BLOCKED_PARRY_BROKEN;
			loseAnim = BOTH_V1_BL_S1;
		}
		else
		{
			if ( !victory )
			{//no-one won
				loseAnim = BOTH_CCWCIRCLEBREAK;
			}
			else
			{
				genemy->saberMove = LS_V1_BL;//genemy->saberBounceMove = 
				genemy->saberBlocked = BLOCKED_PARRY_BROKEN;
				loseAnim = BOTH_V1_BL_S1;
				/*
				genemy->client->ps.saberMove = genemy->client->ps.saberBounceMove = LS_H1_BR;
				genemy->client->ps.saberBlocked = BLOCKED_PARRY_BROKEN;
				loseAnim = BOTH_H1_S1_BL;
				*/
			}
		}
		break;
	case BOTH_CCWCIRCLELOCK:
		if ( superBreak )
		{
			loseAnim = BOTH_LK_S_S_S_SB_1_L;
		}
		else if ( !victory )
		{
			genemy->saberMove = LS_V1_BR;//genemy->saberBounceMove = 
			genemy->saberBlocked = BLOCKED_PARRY_BROKEN;
			loseAnim = BOTH_V1_BR_S1;
		}
		else
		{
			if ( !victory )
			{//no-one won
				loseAnim = BOTH_CWCIRCLEBREAK;
			}
			else
			{
				genemy->saberMove = LS_V1_BR;//genemy->saberBounceMove = 
				genemy->saberBlocked = BLOCKED_PARRY_BROKEN;
				loseAnim = BOTH_V1_BR_S1;
				/*
				genemy->client->ps.saberMove = genemy->client->ps.saberBounceMove = LS_H1_BL;
				genemy->client->ps.saberBlocked = BLOCKED_PARRY_BROKEN;
				loseAnim = BOTH_H1_S1_BR;
				*/
			}
		}
		break;
	}
	if ( loseAnim != -1 )
	{
#ifdef QAGAME
		NPC_SetAnim( &g_entities[genemy->clientNum], SETANIM_BOTH, loseAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		genemy->weaponTime = genemy->torsoTimer;// + 250;
#endif
		genemy->saberBlocked = BLOCKED_NONE;
		genemy->weaponstate = WEAPON_READY;
	}
	return loseAnim;
}

int PM_SaberLockResultAnim( playerState_t *duelist, qboolean superBreak, qboolean won )
{
	int baseAnim = duelist->torsoAnim;
	switch ( baseAnim )
	{
	case BOTH_LK_S_S_S_L_2:		//lock if I'm using single vs. a single and other intitiated
		baseAnim = BOTH_LK_S_S_S_L_1;
		break;
	case BOTH_LK_S_S_T_L_2:		//lock if I'm using single vs. a single and other initiated
		baseAnim = BOTH_LK_S_S_T_L_1;
		break;
	case BOTH_LK_DL_DL_S_L_2:	//lock if I'm using dual vs. dual and other initiated
		baseAnim = BOTH_LK_DL_DL_S_L_1;
		break;
	case BOTH_LK_DL_DL_T_L_2:	//lock if I'm using dual vs. dual and other initiated
		baseAnim = BOTH_LK_DL_DL_T_L_1;
		break;
	case BOTH_LK_ST_ST_S_L_2:	//lock if I'm using staff vs. a staff and other initiated
		baseAnim = BOTH_LK_ST_ST_S_L_1;
		break;
	case BOTH_LK_ST_ST_T_L_2:	//lock if I'm using staff vs. a staff and other initiated
		baseAnim = BOTH_LK_ST_ST_T_L_1;
		break;
	}
	//what kind of break?
	if ( !superBreak )
	{ 
		baseAnim -= 2;
	}
	else if ( superBreak )
	{
		baseAnim += 1;
	}
	else
	{//WTF?  Not a valid result
		return -1;
	}
	//win or lose?
	if ( won )
	{
		baseAnim += 1;
	}

	//play the anim and hold it
#ifdef QAGAME
	//server-side: set it on the other guy, too
	if ( duelist->clientNum == pm->ps->clientNum )
	{//me
		PM_SetAnim( SETANIM_BOTH, baseAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );
	}
	else
	{//other guy
		NPC_SetAnim( &g_entities[duelist->clientNum], SETANIM_BOTH, baseAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	}
#else
	PM_SetAnim( SETANIM_BOTH, baseAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );
#endif

	if ( superBreak
		&& !won )
	{//if you lose a superbreak, you're defenseless
		/*
		//Taken care of in SetSaberBoxSize()
		//make saberent not block
		gentity_t *saberent = &g_entities[duelist->client->ps.saberEntityNum];
		if ( saberent )
		{
			VectorClear(saberent->mins);
			VectorClear(saberent->maxs);
			G_SetOrigin(saberent, duelist->currentOrigin);
		}
		*/
#ifdef QAGAME
		if ( 1 )
#else
		if ( duelist->clientNum == pm->ps->clientNum )
#endif
		{
			//set sabermove to none
			duelist->saberMove = LS_NONE;
			//Hold the anim a little longer than it is
			duelist->torsoTimer += 250;
		}
	}

#ifdef QAGAME
	if ( 1 )
#else
	if ( duelist->clientNum == pm->ps->clientNum )
#endif
	{
		//no attacking during this anim
		duelist->weaponTime = duelist->torsoTimer;
		duelist->saberBlocked = BLOCKED_NONE;
		/*
		if ( superBreak
			&& won
			&& baseAnim != BOTH_LK_ST_DL_T_SB_1_W )
		{//going to attack with saber, do a saber trail
			duelist->client->ps.SaberActivateTrail( 200 );
		}
		*/
	}
	return baseAnim;
}


//[KnockdownSys]
#ifdef QAGAME
extern void G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );
#endif
//[/KnockdownSys]
void PM_SaberLockBreak( playerState_t *genemy, qboolean victory, int strength )
{//racc - breaks a saber lock.  PM is the current player, who is always the winner of the duel if victory is true.
	int	winAnim = BOTH_STAND1, loseAnim = BOTH_STAND1;
	//qboolean punishLoser = qfalse;
	//[SaberLockSys]
	//qboolean noKnockdown = qfalse;
	//[/SaberLockSys]
	qboolean singleVsSingle = qtrue;
	//[SaberLockSys]
	qboolean superBreak = victory;
	//qboolean superBreak = (strength+pm->ps->saberLockHits > Q_irand(2,4));

	//remove the saber lock winner flag.
	pm->ps->userInt3 &= ~( 1 << FLAG_SABERLOCK_ATTACKER );
	genemy->userInt3 &= ~( 1 << FLAG_SABERLOCK_ATTACKER );

	pm->ps->userInt3 &= ~( 1 << FLAG_SABERLOCK_OLD_DIR );
	genemy->userInt3 &= ~( 1 << FLAG_SABERLOCK_OLD_DIR );

	pm->ps->userInt3 &= ~( SABERLOCK_DIR_FLAG_MASK );
	genemy->userInt3 &= ~( SABERLOCK_DIR_FLAG_MASK );
	//[/SaberLockSys]

	winAnim = PM_SaberLockWinAnim( victory, superBreak );
	if ( winAnim != -1 )
	{//a single vs. single break
		loseAnim = PM_SaberLockLoseAnim( genemy, victory, superBreak );
	}
	else
	{//must be a saberlock that's not between single and single...
		singleVsSingle = qfalse;
		winAnim = PM_SaberLockResultAnim( pm->ps, superBreak, qtrue );
		pm->ps->weaponstate = WEAPON_FIRING;
		loseAnim = PM_SaberLockResultAnim( genemy, superBreak, qfalse );
		genemy->weaponstate = WEAPON_READY;
	}

	if ( victory )
	{ //someone lost the lock, so punish them by knocking them down
		//[SaberLockSys]
		if(!superBreak)
		{//if we're not in a superbreak, force the loser to mishap.
			pm->checkDuelLoss = genemy->clientNum+1;
		}

		//racc - this seems to override the actual saberlock completion animations I don't want that.
		/*
		if ( pm->ps->saberLockHits && !superBreak )
		{//there was some over-power in the win, but not enough to superbreak
			vec3_t oppDir;

			int strength = 8;

			VectorSubtract(genemy->origin, pm->ps->origin, oppDir);
			VectorNormalize(oppDir);

			if (noKnockdown)
			{
				if (!genemy->saberEntityNum)
				{ //if he has already lost his saber then just knock him down
					noKnockdown = qfalse;
				}
			}

			if (!noKnockdown && BG_KnockDownable(genemy))
			{
				//[KnockdownSys]
				//had to shift this to game side to make it work with the SP knockdown method.
#ifdef QAGAME
				gentity_t *self = &g_entities[pm->ps->clientNum];
				gentity_t *enemy = &g_entities[genemy->clientNum];
				
				G_Knockdown( enemy, self, oppDir, strength*40, qtrue );

				// replaced with SP knockdown method.
				//genemy->forceHandExtend = HANDEXTEND_KNOCKDOWN;
				//genemy->forceHandExtendTime = pm->cmd.serverTime + 1100;
				//genemy->forceDodgeAnim = 0; //this toggles between 1 and 0, when it's 1 we should play the get up anim
				
				genemy->otherKiller = pm->ps->clientNum;
				genemy->otherKillerTime = pm->cmd.serverTime + 5000;
				genemy->otherKillerDebounceTime = pm->cmd.serverTime + 100;

				//[Asteroids]
				enemy->client->otherKillerMOD = MOD_UNKNOWN;
				enemy->client->otherKillerVehWeapon = 0;
				enemy->client->otherKillerWeaponType = WP_NONE;
#endif
				//[/Asteroids]
				//[/KnockdownSys]

				genemy->velocity[0] = oppDir[0]*(strength*40);
				genemy->velocity[1] = oppDir[1]*(strength*40);
				genemy->velocity[2] = 100;
			}

			pm->checkDuelLoss = genemy->clientNum+1;

			pm->ps->saberEventFlags |= SEF_LOCK_WON;
		}
		*/
		//[/SaberLockSys]
	}
	else
	{ //If no one lost, then shove each player away from the other
		vec3_t oppDir;

		//[SaberLockSys]
		int strength = 10;
		//int strength = 4;
		//[/SaberLockSys]

		VectorSubtract(genemy->origin, pm->ps->origin, oppDir);
		VectorNormalize(oppDir);
		genemy->velocity[0] = oppDir[0]*(strength*40);
		genemy->velocity[1] = oppDir[1]*(strength*40);
		//[SaberLockSys]
		genemy->velocity[2] = 0;
		//genemy->velocity[2] = 150;
		//[/SaberLockSys]

		VectorSubtract(pm->ps->origin, genemy->origin, oppDir);
		VectorNormalize(oppDir);
		pm->ps->velocity[0] = oppDir[0]*(strength*40);
		pm->ps->velocity[1] = oppDir[1]*(strength*40);
		//[SaberLockSys]
		pm->ps->velocity[2] = 0;
		//pm->ps->velocity[2] = 150;

		//genemy->forceHandExtend = HANDEXTEND_WEAPONREADY;
		//[/SaberLockSys]
	}

	//[SaberLockSys]
	//racc - we need to hold our new weapontime so that our lose/win animations can't be interrupted by new attacks.
	/*
	pm->ps->weaponTime = 0;
	genemy->weaponTime = 0;
	*/
	//[/SaberLockSys]

	pm->ps->saberLockTime = genemy->saberLockTime = 0;
	pm->ps->saberLockFrame = genemy->saberLockFrame = 0;
	pm->ps->saberLockEnemy = genemy->saberLockEnemy = 0;
	
	//[SaberLockSys]
	//pm->ps->forceHandExtend = HANDEXTEND_WEAPONREADY;
	//[/SaberLockSys]

	PM_AddEvent( EV_JUMP );
	if ( !victory )
	{//no-one won
		BG_AddPredictableEventToPlayerstate(EV_JUMP, 0, genemy);
	}
	else
	{
		if ( PM_irand_timesync( 0, 1 ) )
		{
			BG_AddPredictableEventToPlayerstate(EV_JUMP, PM_irand_timesync( 0, 75 ), genemy);
		}
	}

//[SaberLockSys]
/*
#ifdef QAGAME
	G_Printf("%i: %i: Broke saberlock Win Animation %s.\n", level.time, pm->ps->clientNum, 
		GetStringForID(animTable, pm->ps->torsoAnim), GetStringForID(animTable, genemy->torsoAnim));
#endif
*/
//[/SaberLockSys]
}

qboolean BG_CheckIncrementLockAnim( int anim, int winOrLose )
{//racc - This function indications that a saberlock animation needs to increment before going to the win or lose position.
	qboolean increment = qfalse;//???
	//RULE: if you are the first style in the lock anim, you advance from LOSING position to WINNING position
	//		if you are the second style in the lock anim, you advance from WINNING position to LOSING position
	switch ( anim )
	{
	//increment to win:
	case BOTH_LK_DL_DL_S_L_1:	//lock if I'm using dual vs. dual and I initiated
	case BOTH_LK_DL_DL_S_L_2:	//lock if I'm using dual vs. dual and other initiated
	case BOTH_LK_DL_DL_T_L_1:	//lock if I'm using dual vs. dual and I initiated
	case BOTH_LK_DL_DL_T_L_2:	//lock if I'm using dual vs. dual and other initiated
	case BOTH_LK_DL_S_S_L_1:		//lock if I'm using dual vs. a single
	case BOTH_LK_DL_S_T_L_1:		//lock if I'm using dual vs. a single
	case BOTH_LK_DL_ST_S_L_1:	//lock if I'm using dual vs. a staff
	case BOTH_LK_DL_ST_T_L_1:	//lock if I'm using dual vs. a staff
	case BOTH_LK_S_S_S_L_1:		//lock if I'm using single vs. a single and I initiated
	case BOTH_LK_S_S_T_L_2:		//lock if I'm using single vs. a single and other initiated
	case BOTH_LK_ST_S_S_L_1:		//lock if I'm using staff vs. a single
	case BOTH_LK_ST_S_T_L_1:		//lock if I'm using staff vs. a single
	case BOTH_LK_ST_ST_T_L_1:	//lock if I'm using staff vs. a staff and I initiated
	case BOTH_LK_ST_ST_T_L_2:	//lock if I'm using staff vs. a staff and other initiated
		if ( winOrLose == SABERLOCK_WIN )
		{
			increment = qtrue;
		}
		else
		{
			increment = qfalse;
		}
		break;

	//decrement to win:
	case BOTH_LK_S_DL_S_L_1:		//lock if I'm using single vs. a dual
	case BOTH_LK_S_DL_T_L_1:		//lock if I'm using single vs. a dual
	case BOTH_LK_S_S_S_L_2:		//lock if I'm using single vs. a single and other intitiated
	case BOTH_LK_S_S_T_L_1:		//lock if I'm using single vs. a single and I initiated
	case BOTH_LK_S_ST_S_L_1:		//lock if I'm using single vs. a staff
	case BOTH_LK_S_ST_T_L_1:		//lock if I'm using single vs. a staff
	case BOTH_LK_ST_DL_S_L_1:	//lock if I'm using staff vs. dual
	case BOTH_LK_ST_DL_T_L_1:	//lock if I'm using staff vs. dual
	case BOTH_LK_ST_ST_S_L_1:	//lock if I'm using staff vs. a staff and I initiated
	case BOTH_LK_ST_ST_S_L_2:	//lock if I'm using staff vs. a staff and other initiated
		if ( winOrLose == SABERLOCK_WIN )
		{
			increment = qfalse;
		}
		else
		{
			increment = qtrue;
		}
		break;
	default:
		break;
	}
	return increment;
}


//[SaberLockSys]
extern void PM_AddEventWithParm( int newEvent, int parm );
//[/SaberLockSys]
extern qboolean ValidAnimFileIndex ( int index );
void PM_SaberLocked( void )
{
	int	remaining = 0;
	playerState_t *genemy;
	bgEntity_t *eGenemy = PM_BGEntForNum(pm->ps->saberLockEnemy);

	if (!eGenemy)
	{
		return;
	}

	genemy = eGenemy->playerState;

	if ( !genemy )
	{
		return;
	}

	//[SaberLockSys]
	//do saberlock effect
	PM_AddEventWithParm(EV_SABERLOCK, pm->ps->saberLockEnemy);
	//[/SaberLockSys]

	/*if ( ( (pm->ps->torsoAnim) == BOTH_BF2LOCK ||
			(pm->ps->torsoAnim) == BOTH_BF1LOCK ||
			(pm->ps->torsoAnim) == BOTH_CWCIRCLELOCK ||
			(pm->ps->torsoAnim) == BOTH_CCWCIRCLELOCK )
		&& ( (genemy->torsoAnim) == BOTH_BF2LOCK ||
			(genemy->torsoAnim) == BOTH_BF1LOCK ||
			(genemy->torsoAnim) == BOTH_CWCIRCLELOCK ||
			(genemy->torsoAnim) == BOTH_CCWCIRCLELOCK )
		)
		*/ //yeah..
	if (pm->ps->saberLockFrame &&
		genemy->saberLockFrame &&
		BG_InSaberLock(pm->ps->torsoAnim) &&
		BG_InSaberLock(genemy->torsoAnim))
	{
		float dist = 0;

		pm->ps->torsoTimer = 0;
		pm->ps->weaponTime = 0;
		genemy->torsoTimer = 0;
		genemy->weaponTime = 0;

		dist = DistanceSquared(pm->ps->origin,genemy->origin);
		if ( dist < 64 || dist > 6400 )
		{//between 8 and 80 from each other
			PM_SaberLockBreak( genemy, qfalse, 0 );
			return;
		}
		/*
		//NOTE: time-out is handled around where PM_SaberLocked is called
		if ( pm->ps->saberLockTime <= pm->cmd.serverTime + 500 )
		{//lock just ended
			PM_SaberLockBreak( genemy, qfalse, 0 );
			return;
		}
		*/
		if ( pm->ps->saberLockAdvance )
		{//holding attack
			animation_t *anim;
			float		currentFrame;
			int			curFrame;
			int			strength = 1;

			pm->ps->saberLockAdvance = qfalse;

			anim = &pm->animations[pm->ps->torsoAnim];

			currentFrame = pm->ps->saberLockFrame;

			//[SaberLockSys]
			//advance the saberlock moves more slowly (the defualt speed of 1 frame)
			//strength = pm->ps->fd.forcePowerLevel[FP_SABER_OFFENSE]+1;
			//[/SaberLockSys]

			//advance/decrement my frame number
			if ( BG_InSaberLockOld( pm->ps->torsoAnim ) )
			{ //old locks
				if ( (pm->ps->torsoAnim) == BOTH_CCWCIRCLELOCK ||
					(pm->ps->torsoAnim) == BOTH_BF2LOCK )
				{
					curFrame = floor( currentFrame )-strength;
					//drop my frame one
					if ( curFrame <= anim->firstFrame )
					{//I won!  Break out
						PM_SaberLockBreak( genemy, qtrue, strength );
						return;
					}
					else
					{
						PM_SetAnimFrame( pm->ps, curFrame, qtrue, qtrue );
						remaining = curFrame-anim->firstFrame;
					}
				}
				else
				{
					curFrame = ceil( currentFrame )+strength;
					//advance my frame one
					if ( curFrame >= anim->firstFrame+anim->numFrames )
					{//I won!  Break out
						PM_SaberLockBreak( genemy, qtrue, strength );
						return;
					}
					else
					{
						PM_SetAnimFrame( pm->ps, curFrame, qtrue, qtrue );
						remaining = anim->firstFrame+anim->numFrames-curFrame;
					}
				}
			}
			else
			{ //new locks
				if ( BG_CheckIncrementLockAnim( pm->ps->torsoAnim, SABERLOCK_WIN ) )
				{
					curFrame = ceil( currentFrame )+strength;
					//advance my frame one
					if ( curFrame >= anim->firstFrame+anim->numFrames )
					{//I won!  Break out
						PM_SaberLockBreak( genemy, qtrue, strength );
						return;
					}
					else
					{
						PM_SetAnimFrame( pm->ps, curFrame, qtrue, qtrue );
						remaining = anim->firstFrame+anim->numFrames-curFrame;
					}
				}
				else
				{
					curFrame = floor( currentFrame )-strength;
					//drop my frame one
					if ( curFrame <= anim->firstFrame )
					{//I won!  Break out
						PM_SaberLockBreak( genemy, qtrue, strength );
						return;
					}
					else
					{
						PM_SetAnimFrame( pm->ps, curFrame, qtrue, qtrue );
						remaining = curFrame-anim->firstFrame;
					}
				}
			}
			if ( !PM_irand_timesync( 0, 2 ) )
			{
				PM_AddEvent( EV_JUMP );
			}
			//advance/decrement enemy frame number
			anim = &pm->animations[(genemy->torsoAnim)];

			if ( BG_InSaberLockOld( genemy->torsoAnim ) )
			{
				if ( (genemy->torsoAnim) == BOTH_CWCIRCLELOCK ||
					(genemy->torsoAnim) == BOTH_BF1LOCK )
				{
					if ( !PM_irand_timesync( 0, 2 ) )
					{//racc - throw in some grunts of pain!
						BG_AddPredictableEventToPlayerstate(EV_PAIN, floor((float)80/100*100.0f), genemy);
					}
					PM_SetAnimFrame( genemy, anim->firstFrame+remaining, qtrue, qtrue );
				}
				else
				{
					PM_SetAnimFrame( genemy, anim->firstFrame+anim->numFrames-remaining, qtrue, qtrue );
				}
			}
			else
			{//new locks
				if ( BG_CheckIncrementLockAnim( genemy->torsoAnim, SABERLOCK_LOSE ) )
				{
					if ( !PM_irand_timesync( 0, 2 ) )
					{
						BG_AddPredictableEventToPlayerstate(EV_PAIN, floor((float)80/100*100.0f), genemy);
					}
					PM_SetAnimFrame( genemy, anim->firstFrame+anim->numFrames-remaining, qtrue, qtrue );
				}
				else
				{
					PM_SetAnimFrame( genemy, anim->firstFrame+remaining, qtrue, qtrue );
				}
			}
		}
	}
	else
	{//something broke us out of it
		PM_SaberLockBreak( genemy, qfalse, 0 );
	}
}

qboolean PM_SaberInBrokenParry( int move )
{
	if(move==139||move==133)
	{
		return qfalse;
	}
	if ( move >= LS_V1_BR && move <= LS_V1_B_ )
	{
		return qtrue;
	}
	if ( move >= LS_H1_T_ && move <= LS_H1_BL )
	{
		return qtrue;
	}
	return qfalse;
}


int PM_BrokenParryForParry( int move )
{
	switch ( move )
	{
	case LS_PARRY_UP:
		return LS_H1_T_;
		break;
	case LS_PARRY_UR:
		return LS_H1_TR;
		break;
	case LS_PARRY_UL:
		return LS_H1_TL;
		break;
	case LS_PARRY_LR:
		return LS_H1_BL;
		break;
	case LS_PARRY_LL:
		return LS_H1_BR;
		break;
	case LS_READY:
		return LS_H1_B_;
		break;
	}
	return LS_NONE;
}

#define BACK_STAB_DISTANCE 128

qboolean PM_CanBackstab(void)
{
	trace_t tr;
	vec3_t flatAng;
	vec3_t fwd, back;
	vec3_t trmins = {-15, -15, -8};
	vec3_t trmaxs = {15, 15, 8};

	VectorCopy(pm->ps->viewangles, flatAng);
	flatAng[PITCH] = 0;

	AngleVectors(flatAng, fwd, 0, 0);

	back[0] = pm->ps->origin[0] - fwd[0]*BACK_STAB_DISTANCE;
	back[1] = pm->ps->origin[1] - fwd[1]*BACK_STAB_DISTANCE;
	back[2] = pm->ps->origin[2] - fwd[2]*BACK_STAB_DISTANCE;

	pm->trace(&tr, pm->ps->origin, trmins, trmaxs, back, pm->ps->clientNum, MASK_PLAYERSOLID);

	if (tr.fraction != 1.0 && tr.entityNum >= 0 && tr.entityNum < ENTITYNUM_NONE)
	{
		bgEntity_t *bgEnt = PM_BGEntForNum(tr.entityNum);

		if (bgEnt && (bgEnt->s.eType == ET_PLAYER || bgEnt->s.eType == ET_NPC))
		{
			return qtrue;
		}
	}

	return qfalse;
}

saberMoveName_t PM_SaberFlipOverAttackMove(void)
{ 
	vec3_t fwdAngles, jumpFwd;
//	float zDiff = 0;
//	playerState_t *psData;
//	bgEntity_t *bgEnt;

	saberInfo_t *saber1 = BG_MySaber( pm->ps->clientNum, 0 );
	saberInfo_t *saber2 = BG_MySaber( pm->ps->clientNum, 1 );
	//see if we have an overridden (or cancelled) lunge move
	if ( saber1
		&& saber1->jumpAtkFwdMove != LS_INVALID )
	{
		if ( saber1->jumpAtkFwdMove != LS_NONE )
		{
			return (saberMoveName_t)saber1->jumpAtkFwdMove;
		}
	}
	if ( saber2
		&& saber2->jumpAtkFwdMove != LS_INVALID )
	{
		if ( saber2->jumpAtkFwdMove != LS_NONE )
		{
			return (saberMoveName_t)saber2->jumpAtkFwdMove;
		}
	}
	//no overrides, cancelled?
	if ( saber1
		&& saber1->jumpAtkFwdMove == LS_NONE )
	{
		return LS_A_T2B;//LS_NONE;
	}
	if ( saber2 
		&& saber2->jumpAtkFwdMove == LS_NONE )
	{
		return LS_A_T2B;//LS_NONE;
	}
	//just do it
	VectorCopy( pm->ps->viewangles, fwdAngles );
	fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
	AngleVectors( fwdAngles, jumpFwd, NULL, NULL );
	VectorScale( jumpFwd, 150, pm->ps->velocity );//was 50
	pm->ps->velocity[2] = 400;

	/*
	bgEnt = PM_BGEntForNum(tr->entityNum);

	if (!bgEnt)
	{
		return LS_A_FLIP_STAB;
	}

    psData = bgEnt->playerState;

	//go higher for enemies higher than you, lower for those lower than you
	if (psData)
	{
		zDiff = psData->origin[2] - pm->ps->origin[2];
	}
	else
	{
		zDiff = 0;
	}
	pm->ps->velocity[2] += (zDiff)*1.5f;

	//clamp to decent-looking values
	if ( zDiff <= 0 && pm->ps->velocity[2] < 200 )
	{//if we're on same level, don't let me jump so low, I clip into the ground
		pm->ps->velocity[2] = 200;
	}
	else if ( pm->ps->velocity[2] < 100 )
	{
		pm->ps->velocity[2] = 100;
	}
	else if ( pm->ps->velocity[2] > 400 )
	{
		pm->ps->velocity[2] = 400;
	}
	*/

	PM_SetForceJumpZStart(pm->ps->origin[2]);//so we don't take damage if we land at same height

	PM_AddEvent( EV_JUMP );
	pm->ps->fd.forceJumpSound = 1;
	pm->cmd.upmove = 0;

	/*
	if ( PM_irand_timesync( 0, 1 ) )
	{
		return LS_A_FLIP_STAB;
	}
	else
	*/
	//[SaberSys]
	if( pm->ps->fd.saberAnimLevel == SS_FAST
		|| pm->ps->fd.saberAnimLevel == SS_MEDIUM
		|| pm->ps->fd.saberAnimLevel == SS_TAVION )
	{
		return LS_A_FLIP_STAB;
	}
	else
	//[/SaberSys]
	{
		return LS_A_FLIP_SLASH;
	}
}


//[FatigueSys]
//Moved from bg_panimate.c
qboolean PM_SaberInDeflect( int move )
{
	if ( move >= LS_D1_BR && move <= LS_D1_B_ )
	{
		return qtrue;
	}
	return qfalse;
}

//BG version of SaberAttacking
qboolean BG_SaberAttacking( playerState_t *ps )
{
	if (PM_SaberInParry(ps->saberMove))
	{
		return qfalse;
	}
	if (PM_SaberInBrokenParry(ps->saberMove))
	{
		return qfalse;
	}
	if (PM_SaberInDeflect(ps->saberMove))
	{
		return qfalse;
	}
	if (PM_SaberInBounce(ps->saberMove))
	{
		return qfalse;
	}
	if (PM_SaberInKnockaway(ps->saberMove))
	{
		return qfalse;
	}

	if (BG_SaberInAttack(ps->saberMove))
	{
		if (ps->weaponstate == WEAPON_FIRING && ps->saberBlocked == BLOCKED_NONE)
		{ //if we're firing and not blocking, then we're attacking.
			return qtrue;
		}
	}

	if (BG_SaberInSpecial(ps->saberMove))
	{
		return qtrue;
	}

	return qfalse;
}
//[/FatigueSys]


int PM_SaberBackflipAttackMove( void )
{
	saberInfo_t *saber1 = BG_MySaber( pm->ps->clientNum, 0 );
	saberInfo_t *saber2 = BG_MySaber( pm->ps->clientNum, 1 );
	//see if we have an overridden (or cancelled) lunge move
	if ( saber1
		&& saber1->jumpAtkBackMove != LS_INVALID )
	{
		if ( saber1->jumpAtkBackMove != LS_NONE )
		{
			return (saberMoveName_t)saber1->jumpAtkBackMove;
		}
	}
	if ( saber2
		&& saber2->jumpAtkBackMove != LS_INVALID )
	{
		if ( saber2->jumpAtkBackMove != LS_NONE )
		{
			return (saberMoveName_t)saber2->jumpAtkBackMove;
		}
	}
	//no overrides, cancelled?
	if ( saber1
		&& saber1->jumpAtkBackMove == LS_NONE )
	{
		return LS_A_T2B;//LS_NONE;
	}
	if ( saber2
		&& saber2->jumpAtkBackMove == LS_NONE )
	{
		return LS_A_T2B;//LS_NONE;
	}
	//just do it
	pm->cmd.upmove = 127;
	pm->ps->velocity[2] = 500;
	return LS_A_BACKFLIP_ATK;
}

int PM_SaberDualJumpAttackMove( void )
{
	//FIXME: to make this move easier to execute, should be allowed to do it 
	//		after you've already started your jump... but jump is delayed in
	//		this anim, so how do we undo the jump?
	pm->cmd.upmove = 0;//no jump just yet
	return LS_JUMPATTACK_DUAL;
}

#define FLIPHACK_DISTANCE 200

qboolean PM_SomeoneInFront(trace_t *tr)
{ //Also a very simplified version of the sp counterpart
	vec3_t flatAng;
	vec3_t fwd, back;
	vec3_t trmins = {-15, -15, -8};
	vec3_t trmaxs = {15, 15, 8};

	VectorCopy(pm->ps->viewangles, flatAng);
	flatAng[PITCH] = 0;

	AngleVectors(flatAng, fwd, 0, 0);

	back[0] = pm->ps->origin[0] + fwd[0]*FLIPHACK_DISTANCE;
	back[1] = pm->ps->origin[1] + fwd[1]*FLIPHACK_DISTANCE;
	back[2] = pm->ps->origin[2] + fwd[2]*FLIPHACK_DISTANCE;

	pm->trace(tr, pm->ps->origin, trmins, trmaxs, back, pm->ps->clientNum, MASK_PLAYERSOLID);

	if (tr->fraction != 1.0 && tr->entityNum >= 0 && tr->entityNum < ENTITYNUM_NONE)
	{
		bgEntity_t *bgEnt = PM_BGEntForNum(tr->entityNum);

		if (bgEnt && (bgEnt->s.eType == ET_PLAYER || bgEnt->s.eType == ET_NPC))
		{
			return qtrue;
		}
	}

	return qfalse;
}

saberMoveName_t PM_SaberLungeAttackMove( qboolean noSpecials )
{
	vec3_t fwdAngles, jumpFwd;
	saberInfo_t *saber1 = BG_MySaber( pm->ps->clientNum, 0 );
	saberInfo_t *saber2 = BG_MySaber( pm->ps->clientNum, 1 );
	//see if we have an overridden (or cancelled) lunge move
	if ( saber1
		&& saber1->lungeAtkMove != LS_INVALID )
	{
		if ( saber1->lungeAtkMove != LS_NONE )
		{
			return (saberMoveName_t)saber1->lungeAtkMove;
		}
	}
	if ( saber2
		&& saber2->lungeAtkMove != LS_INVALID )
	{
		if ( saber2->lungeAtkMove != LS_NONE )
		{
			return (saberMoveName_t)saber2->lungeAtkMove;
		}
	}
	//no overrides, cancelled?
	if ( saber1
		&& saber1->lungeAtkMove == LS_NONE )
	{
		return LS_A_T2B;//LS_NONE;
	}
	if ( saber2
	&& saber2->lungeAtkMove == LS_NONE )
	{
		return LS_A_T2B;//LS_NONE;
	}
	//just do it
	//[SaberSys]
	//all single saber styles can now do lunges.
	if (pm->ps->fd.saberAnimLevel >= SS_FAST && pm->ps->fd.saberAnimLevel <= SS_TAVION)
	//if (pm->ps->fd.saberAnimLevel == SS_FAST)
	//[/SaberSys]
	{
		VectorCopy( pm->ps->viewangles, fwdAngles );
		fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
		//do the lunge
		AngleVectors( fwdAngles, jumpFwd, NULL, NULL );
		VectorScale( jumpFwd, 150, pm->ps->velocity );
		PM_AddEvent( EV_JUMP );

/*
#ifdef QAGAME
		G_Printf("%i: %i: Started Lunge Move.\n", level.time, pm->ps->clientNum);
#endif
*/
		return LS_A_LUNGE;
	}
	else if(!noSpecials && pm->ps->fd.saberAnimLevel == SS_STAFF && saber1->numBlades == 1)
	{
						VectorCopy( pm->ps->viewangles, fwdAngles );
		fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
		//do the lunge
		AngleVectors( fwdAngles, jumpFwd, NULL, NULL );
		VectorScale( jumpFwd, 150, pm->ps->velocity );
		PM_AddEvent( EV_JUMP );
		return LS_A_LUNGE;
	}
	else if ( !noSpecials && pm->ps->fd.saberAnimLevel == SS_STAFF)
	{
		return LS_SPINATTACK;
	}
	else if(!noSpecials && (!saber2 || saber2->numBlades == 0))
	{
						VectorCopy( pm->ps->viewangles, fwdAngles );
		fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
		//do the lunge
		AngleVectors( fwdAngles, jumpFwd, NULL, NULL );
		VectorScale( jumpFwd, 150, pm->ps->velocity );
		PM_AddEvent( EV_JUMP );
		return LS_A_LUNGE;
	}
	else if ( !noSpecials )
	{
		return LS_SPINATTACK_DUAL;
	}
	return LS_A_T2B;
}


//[AotCAI]
#ifdef QAGAME
//#ifndef CLIENTCOMPILE
#define JM_BACK_STAB_DISTANCE 256

qboolean PM_JMCanBackstab(void)
{
	trace_t tr;
	vec3_t flatAng;
	vec3_t fwd, back;
	vec3_t trmins = {-15, -15, -8};
	vec3_t trmaxs = {15, 15, 8};

	VectorCopy(pm->ps->viewangles, flatAng);
	flatAng[PITCH] = 0;

	AngleVectors(flatAng, fwd, 0, 0);

	back[0] = pm->ps->origin[0] - fwd[0]*JM_BACK_STAB_DISTANCE;
	back[1] = pm->ps->origin[1] - fwd[1]*JM_BACK_STAB_DISTANCE;
	back[2] = pm->ps->origin[2] - fwd[2]*JM_BACK_STAB_DISTANCE;

	pm->trace(&tr, pm->ps->origin, trmins, trmaxs, back, pm->ps->clientNum, MASK_PLAYERSOLID);

	if ( tr.fraction != 1.0 && tr.entityNum >= 0 && ( tr.entityNum < MAX_CLIENTS ))
	{ //We don't have real entity access here so we can't do an indepth check. But if it's a client and it's behind us, I guess that's reason enough to stab backward
		return qtrue;
	}

	return qfalse;
}


#define LUNGE_DISTANCE 128

qboolean PM_CanLunge(void)
{//AotC AI check for lunge attacks.
	//gentity_t *ent;
	trace_t tr;
	vec3_t flatAng;
	vec3_t fwd, back;
	vec3_t trmins = {-15, -15, -8};
	vec3_t trmaxs = {15, 15, 8};

	VectorCopy(pm->ps->viewangles, flatAng);
	flatAng[PITCH] = 0;

	AngleVectors(flatAng, fwd, 0, 0);

	back[0] = pm->ps->origin[0] + fwd[0]*LUNGE_DISTANCE;
	back[1] = pm->ps->origin[1] + fwd[1]*LUNGE_DISTANCE;
	back[2] = pm->ps->origin[2] + fwd[2]*LUNGE_DISTANCE;

	pm->trace(&tr, pm->ps->origin, trmins, trmaxs, back, pm->ps->clientNum, MASK_PLAYERSOLID);

	if ( tr.fraction != 1.0 && tr.entityNum >= 0 && ( tr.entityNum < MAX_CLIENTS ))
	{ //We don't have real entity access here so we can't do an indepth check. But if it's a client and it's behind us, I guess that's reason enough to stab backward
		return qtrue;
	}

	return qfalse;
}


#define JM_LUNGE_DISTANCE 512

qboolean PM_JMCanLunge(void)
{
	trace_t tr;
	vec3_t flatAng;
	vec3_t fwd, back;
	vec3_t trmins = {-15, -15, -8};
	vec3_t trmaxs = {15, 15, 8};

	VectorCopy(pm->ps->viewangles, flatAng);
	flatAng[PITCH] = 0;

	AngleVectors(flatAng, fwd, 0, 0);

	back[0] = pm->ps->origin[0] + fwd[0]*JM_LUNGE_DISTANCE;
	back[1] = pm->ps->origin[1] + fwd[1]*JM_LUNGE_DISTANCE;
	back[2] = pm->ps->origin[2] + fwd[2]*JM_LUNGE_DISTANCE;

	pm->trace(&tr, pm->ps->origin, trmins, trmaxs, back, pm->ps->clientNum, MASK_PLAYERSOLID);

	if (tr.fraction != 1.0 && tr.entityNum >= 0 && tr.entityNum < MAX_CLIENTS )
	{ //We don't have real entity access here so we can't do an indepth check. But if it's a client and it's behind us, I guess that's reason enough to stab backward
		return qtrue;
	}

	return qfalse;
}
#endif
//[/AotCAI]

saberMoveName_t PM_SaberJumpAttackMove2( void )
{
	saberInfo_t *saber1 = BG_MySaber( pm->ps->clientNum, 0 );
	saberInfo_t *saber2 = BG_MySaber( pm->ps->clientNum, 1 );
	//see if we have an overridden (or cancelled) lunge move
	if ( saber1
		&& saber1->jumpAtkFwdMove != LS_INVALID )
	{
		if ( saber1->jumpAtkFwdMove != LS_NONE )
		{
			return (saberMoveName_t)saber1->jumpAtkFwdMove;
		}
	}
	if ( saber2
	&& saber2->jumpAtkFwdMove != LS_INVALID )
	{
		if ( saber2->jumpAtkFwdMove != LS_NONE )
		{
			return (saberMoveName_t)saber2->jumpAtkFwdMove;
		}
	}
	//no overrides, cancelled?
	if ( saber1
		&& saber1->jumpAtkFwdMove == LS_NONE )
	{
		return LS_A_T2B;//LS_NONE;
	}
	if ( saber2
		&& saber2->jumpAtkFwdMove == LS_NONE )
	{
		return LS_A_T2B;//LS_NONE;
	}
	//just do it
	if (pm->ps->fd.saberAnimLevel == SS_DUAL)
	{
		return PM_SaberDualJumpAttackMove();
	}
	else
	{
		//rwwFIXMEFIXME I don't like randomness for this sort of thing, gives people reason to
		//complain combat is unpredictable. Maybe do something more clever to determine
		//if we should do a left or right?
		/*
		if (PM_irand_timesync(0, 1))
		{
			newmove = LS_JUMPATTACK_STAFF_LEFT;
		}
		else
		*/
		{
			return LS_JUMPATTACK_STAFF_RIGHT;
		}
	}
	return LS_A_T2B;
}

saberMoveName_t PM_SaberJumpAttackMove( void )
{
	vec3_t fwdAngles, jumpFwd;
	saberInfo_t *saber1 = BG_MySaber( pm->ps->clientNum, 0 );
	saberInfo_t *saber2 = BG_MySaber( pm->ps->clientNum, 1 );
	//see if we have an overridden (or cancelled) lunge move
	if ( saber1
		&& saber1->jumpAtkFwdMove != LS_INVALID )
	{
		if ( saber1->jumpAtkFwdMove != LS_NONE )
		{
			return (saberMoveName_t)saber1->jumpAtkFwdMove;
		}
	}
	if ( saber2
		&& saber2->jumpAtkFwdMove != LS_INVALID )
	{
		if ( saber2->jumpAtkFwdMove != LS_NONE )
		{
			return (saberMoveName_t)saber2->jumpAtkFwdMove;
		}
	}
	//no overrides, cancelled?
	if ( saber1
		&& saber1->jumpAtkFwdMove == LS_NONE )
	{
		return LS_A_T2B;//LS_NONE;
	}
	if ( saber2
		&& saber2->jumpAtkFwdMove == LS_NONE )
	{
		return LS_A_T2B;//LS_NONE;
	}
	//just do it
	VectorCopy( pm->ps->viewangles, fwdAngles );
	fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
	AngleVectors( fwdAngles, jumpFwd, NULL, NULL );
	VectorScale( jumpFwd, 300, pm->ps->velocity );
	pm->ps->velocity[2] = 280;
	PM_SetForceJumpZStart(pm->ps->origin[2]);//so we don't take damage if we land at same height

	PM_AddEvent( EV_JUMP );
	pm->ps->fd.forceJumpSound = 1;
	pm->cmd.upmove = 0;

	return LS_A_JUMP_T__B_;
}

float PM_GroundDistance(void)
{
	trace_t tr;
	vec3_t down;

	VectorCopy(pm->ps->origin, down);

	down[2] -= 4096;

	pm->trace(&tr, pm->ps->origin, pm->mins, pm->maxs, down, pm->ps->clientNum, MASK_SOLID);

	VectorSubtract(pm->ps->origin, tr.endpos, down);

	return VectorLength(down);
}

float PM_WalkableGroundDistance(void)
{
	trace_t tr;
	vec3_t down;

	VectorCopy(pm->ps->origin, down);

	down[2] -= 4096;

	pm->trace(&tr, pm->ps->origin, pm->mins, pm->maxs, down, pm->ps->clientNum, MASK_SOLID);

	if ( tr.plane.normal[2] < MIN_WALK_NORMAL )
	{//can't stand on this plane
		return 4096;
	}

	VectorSubtract(pm->ps->origin, tr.endpos, down);

	return VectorLength(down);
}

qboolean BG_SaberInTransitionAny( int move );
static qboolean PM_CanDoDualDoubleAttacks(void)
{
	if ( pm->ps->weapon == WP_SABER )
	{
		saberInfo_t *saber = BG_MySaber( pm->ps->clientNum, 0 );
		if ( saber
			&& (saber->saberFlags&SFL_NO_MIRROR_ATTACKS) )
		{
			return qfalse;
		}
		saber = BG_MySaber( pm->ps->clientNum, 1 );
		if ( saber
			&& (saber->saberFlags&SFL_NO_MIRROR_ATTACKS) )
		{
			return qfalse;
		}
	}
	if (BG_SaberInSpecialAttack(pm->ps->torsoAnim) ||
		BG_SaberInSpecialAttack(pm->ps->legsAnim))
	{
		return qfalse;
	}
	return qtrue;
}

static qboolean PM_CheckEnemyPresence( int dir, float radius )
{ //anyone in this dir?
	vec3_t angles;
	vec3_t checkDir;
	vec3_t tTo;
	vec3_t tMins, tMaxs;
	trace_t tr;
	const float tSize = 12.0f;
	//sp uses a bbox ent list check, but.. that's not so easy/fast to
	//do in predicted code. So I'll just do a single box trace in the proper direction,
	//and take whatever is first hit.

	VectorSet(tMins, -tSize, -tSize, -tSize);
	VectorSet(tMaxs, tSize, tSize, tSize);

	VectorCopy(pm->ps->viewangles, angles);
	angles[PITCH] = 0.0f;

	switch( dir )
	{
	case DIR_RIGHT:
		AngleVectors( angles, NULL, checkDir, NULL );
		break;
	case DIR_LEFT:
		AngleVectors( angles, NULL, checkDir, NULL );
		VectorScale( checkDir, -1, checkDir );
		break;
	case DIR_FRONT:
		AngleVectors( angles, checkDir, NULL, NULL );
		break;
	case DIR_BACK:
		AngleVectors( angles, checkDir, NULL, NULL );
		VectorScale( checkDir, -1, checkDir );
		break;
	}

	VectorMA(pm->ps->origin, radius, checkDir, tTo);
	pm->trace(&tr, pm->ps->origin, tMins, tMaxs, tTo, pm->ps->clientNum, MASK_PLAYERSOLID);

	if (tr.fraction != 1.0f && tr.entityNum < ENTITYNUM_WORLD)
	{ //let's see who we hit
		bgEntity_t *bgEnt = PM_BGEntForNum(tr.entityNum);

		if (bgEnt &&
			(bgEnt->s.eType == ET_PLAYER || bgEnt->s.eType == ET_NPC))
		{ //this guy can be considered an "enemy"... if he is on the same team, oh well. can't bg-check that (without a whole lot of hassle).
			return qtrue;
		}
	}

	//no one in the trace
	return qfalse;
}

#define SABER_ALT_ATTACK_POWER		100//75?

//[SaberSys]
//for now, I've dramatically reduced the cost of the saber special moves to
//keep them from screwing up the fatigue system balance.

//racc - force cost of doing cartwheels.
#define SABER_ALT_ATTACK_POWER_LR	FATIGUE_CARTWHEEL
#define SABER_ALT_ATTACK_POWER_LRA	FATIGUE_CARTWHEEL_ATARU
//#define SABER_ALT_ATTACK_POWER_LR	10//30?

//racc - force cost of doing all the special saber attacks (other than the katas and cartwheels)
//#define SABER_ALT_ATTACK_POWER_FB	3 NUAM
//#define SABER_ALT_ATTACK_POWER_FB	25//30/50?
//[/SaberSys]

extern qboolean PM_SaberInReturn( int move ); //bg_panimate.c
saberMoveName_t PM_CheckPullAttack( void )
{
#if 0 //disabling these for MP, they aren't useful
	if (!(pm->cmd.buttons & BUTTON_ATTACK))
	{
		return LS_NONE;
	}

	if ( (pm->ps->saberMove == LS_READY||PM_SaberInReturn(pm->ps->saberMove)||PM_SaberInReflect(pm->ps->saberMove))//ready
		//&& (pm->ps->clientNum < MAX_CLIENTS||PM_ControlledByPlayer())//PLAYER ONLY
		&& pm->ps->fd.saberAnimLevel >= SS_FAST//single saber styles - FIXME: Tavion?
		&& pm->ps->fd.saberAnimLevel <= SS_STRONG//single saber styles - FIXME: Tavion?
		//&& G_TryingPullAttack( pm->gent, &pm->cmd, qfalse )
		//&& pm->ps->fd.forcePowerLevel[FP_PULL] 
		//rwwFIXMEFIXME: rick has the damn msg.cpp file checked out exclusively so I can't update the bloody psf to send this for prediction
		&& pm->ps->powerups[PW_DISINT_4] > pm->cmd.serverTime
		&& !(pm->ps->fd.forcePowersActive & (1<<FP_GRIP))
		&& pm->ps->powerups[PW_PULL] > pm->cmd.serverTime
		//&& pm->cmd.forwardmove<0//pulling back
		&& (pm->cmd.buttons&BUTTON_ATTACK)//attacking
		&& BG_EnoughForcePowerForMove( SABER_ALT_ATTACK_POWER_FB ) )//pm->ps->forcePower >= SABER_ALT_ATTACK_POWER_FB//have enough power
	{//FIXME: some NPC logic to do this?
		qboolean doMove = qtrue;
//		if ( g_saberNewControlScheme->integer
//			|| g_crosshairEntNum < ENTITYNUM_WORLD )//in old control scheme, there has to be someone there
		{	
			saberMoveName_t pullAttackMove = LS_NONE;
			if ( pm->ps->fd.saberAnimLevel == SS_FAST )
			{
				pullAttackMove = LS_PULL_ATTACK_STAB;
			}
			else
			{
				pullAttackMove = LS_PULL_ATTACK_SWING;
			}

			/*
			if ( g_crosshairEntNum < ENTITYNUM_WORLD 
				&& pm->gent && pm->gent->client )
			{
				gentity_t *targEnt = &g_entities[g_crosshairEntNum];
				if ( targEnt->client
					&& targEnt->health > 0
					//FIXME: check other things like in knockdown, saberlock, uninterruptable anims, etc.
					&& !PM_InOnGroundAnim( &targEnt->client->ps )
					&& !PM_LockedAnim( targEnt->client->ps.legsAnim ) 
					&& !PM_SuperBreakLoseAnim( targEnt->client->ps.legsAnim )
					&& !PM_SuperBreakWinAnim( targEnt->client->ps.legsAnim )
					&& targEnt->client->ps.saberLockTime <= 0
					&& WP_ForceThrowable( targEnt, targEnt, pm->gent, qtrue, 1.0f, 0.0f, NULL ) )
				{
					if ( !g_saberNewControlScheme->integer )
					{//in old control scheme, make sure they're close or far enough away for the move we'll be doing
						float targDist = Distance( targEnt->currentOrigin, pm->ps->origin );
						if ( pullAttackMove == LS_PULL_ATTACK_STAB )
						{//must be closer than 512
							if ( targDist > 384.0f )
							{
								return LS_NONE;
							}
						}
						else//if ( pullAttackMove == LS_PULL_ATTACK_SWING )
						{//must be farther than 256
							if ( targDist > 512.0f )
							{
								return LS_NONE;
							}
							if ( targDist < 192.0f )
							{
								return LS_NONE;
							}
						}
					}

					vec3_t targAngles = {0,targEnt->client->ps.viewangles[YAW],0};
					if ( InFront( pm->ps->origin, targEnt->currentOrigin, targAngles ) )
					{
						NPC_SetAnim( targEnt, SETANIM_BOTH, BOTH_PULLED_INAIR_F, SETANIM_FLAG_OVERRIDE, SETANIM_FLAG_HOLD );
					}
					else
					{
						NPC_SetAnim( targEnt, SETANIM_BOTH, BOTH_PULLED_INAIR_B, SETANIM_FLAG_OVERRIDE, SETANIM_FLAG_HOLD );
					}
					//hold the anim until I'm with done pull anim
					targEnt->client->ps.legsAnimTimer = targEnt->client->ps.torsoAnimTimer = PM_AnimLength( pm->gent->client->clientInfo.animFileIndex, (animNumber_t)saberMoveData[pullAttackMove].animToUse );
					//set pullAttackTime
					pm->gent->client->ps.pullAttackTime = targEnt->client->ps.pullAttackTime = level.time+targEnt->client->ps.legsAnimTimer;
					//make us know about each other
					pm->gent->client->ps.pullAttackEntNum = g_crosshairEntNum;
					targEnt->client->ps.pullAttackEntNum = pm->ps->clientNum;
					//do effect and sound on me
					pm->ps->powerups[PW_FORCE_PUSH] = level.time + 1000;
					if ( pm->gent )
					{
						G_Sound( pm->gent, G_SoundIndex( "sound/weapons/force/pull.wav" ) );
					}
					doMove = qtrue;
				}
			}
			*/
			if ( doMove )
			{
				BG_ForcePowerDrain(pm->ps, FP_GRIP, SABER_ALT_ATTACK_POWER_FB );
				return pullAttackMove;
			}
		

		}
	}
#endif
	return LS_NONE;
}

qboolean PM_InSecondaryStyle( void )
{
	if ( pm->ps->fd.saberAnimLevelBase == SS_STAFF 
		|| pm->ps->fd.saberAnimLevelBase == SS_DUAL )
	{
		if ( pm->ps->fd.saberAnimLevel != pm->ps->fd.saberAnimLevelBase )
		{
			return qtrue;
		}
	}
	return qfalse;
}


//[AotCAI]
#ifdef QAGAME
//#ifndef CLIENTCOMPILE
int nextcheck[MAX_CLIENTS]; // Next special move check.
extern int FindBotType(int clientNum);
#endif
//[/AotCAI]
//[SaberSys]
extern qboolean PM_InCartwheel( int anim );
//[/SaberSys]
saberMoveName_t PM_SaberAttackForMovement(saberMoveName_t curmove)
{
//	saberInfo_t *saber = BG_MySaber( pm->ps->clientNum, 0 );
	saberMoveName_t newmove = LS_NONE;
	//[SaberSys]
	//can't launch a special while in a cartwheel (prevents possible FP exploit)
	qboolean noSpecials = PM_InSecondaryStyle() || PM_InCartwheel(pm->ps->legsAnim);
	//qboolean noSpecials = PM_InSecondaryStyle();
	//[/SaberSys]
	qboolean allowCartwheels = qtrue;
	saberMoveName_t overrideJumpRightAttackMove = LS_INVALID;
	saberMoveName_t overrideJumpLeftAttackMove = LS_INVALID;

	if ( pm->ps->weapon == WP_SABER )
	{
		saberInfo_t *saber1 = BG_MySaber( pm->ps->clientNum, 0 );
		saberInfo_t *saber2 = BG_MySaber( pm->ps->clientNum, 1 );

		if ( saber1
			&& saber1->jumpAtkRightMove != LS_INVALID )
		{
			if ( saber1->jumpAtkRightMove != LS_NONE )
			{//actually overriding
				overrideJumpRightAttackMove = (saberMoveName_t)saber1->jumpAtkRightMove;
			}
			else if ( saber2
				&& saber2->jumpAtkRightMove > LS_NONE )
			{//would be cancelling it, but check the second saber, too
				overrideJumpRightAttackMove = (saberMoveName_t)saber2->jumpAtkRightMove;
			}
			else
			{//nope, just cancel it
				overrideJumpRightAttackMove = LS_NONE;
			}
		}
		else if ( saber2
			&& saber2->jumpAtkRightMove != LS_INVALID )
		{//first saber not overridden, check second
			overrideJumpRightAttackMove = (saberMoveName_t)saber2->jumpAtkRightMove;
		}

		if ( saber1
			&& saber1->jumpAtkLeftMove != LS_INVALID )
		{
			if ( saber1->jumpAtkLeftMove != LS_NONE )
			{//actually overriding
				overrideJumpLeftAttackMove = (saberMoveName_t)saber1->jumpAtkLeftMove;
			}
			else if ( saber2
				&& saber2->jumpAtkLeftMove > LS_NONE )
			{//would be cancelling it, but check the second saber, too
				overrideJumpLeftAttackMove = (saberMoveName_t)saber2->jumpAtkLeftMove;
			}
			else
			{//nope, just cancel it
				overrideJumpLeftAttackMove = LS_NONE;
			}
		}
		else if ( saber2
			&& saber2->jumpAtkLeftMove != LS_INVALID )
		{//first saber not overridden, check second
			overrideJumpLeftAttackMove = (saberMoveName_t)saber1->jumpAtkLeftMove;
		}

		if ( saber1
			&& (saber1->saberFlags&SFL_NO_CARTWHEELS) )
		{
			allowCartwheels = qfalse;
		}
		if ( saber2
			&& (saber2->saberFlags&SFL_NO_CARTWHEELS) )
		{
			allowCartwheels = qfalse;
		}
	}

	if ( pm->cmd.rightmove > 0 )
	{//moving right
		saberInfo_t *saber1 = BG_MySaber( pm->ps->clientNum, 0 );
		saberInfo_t *saber2 = BG_MySaber( pm->ps->clientNum, 1 );
		if ( !noSpecials
			&& overrideJumpRightAttackMove != LS_NONE
			&& pm->ps->velocity[2] > 20.0f //pm->ps->groundEntityNum != ENTITYNUM_NONE//on ground
			&& (pm->cmd.buttons&BUTTON_ATTACK)//hitting attack
			&& PM_GroundDistance() < 70.0f //not too high above ground
			&& ( pm->cmd.upmove > 0 || (pm->ps->pm_flags & PMF_JUMP_HELD) )//focus-holding player
			&& BG_EnoughForcePowerForMove( SABER_ALT_ATTACK_POWER_LR ) )//have enough power
		{//cartwheel right
			if(saber1 && !saber2 && pm->ps->fd.saberAnimLevel == SS_DUAL)//for a part of single dual/ataru's. 1 point cartwheels
			{
				BG_ForcePowerDrain(pm->ps, FP_GRIP, SABER_ALT_ATTACK_POWER_LRA);

			}
			else
			{
				BG_ForcePowerDrain(pm->ps, FP_GRIP, SABER_ALT_ATTACK_POWER_LR);
			}

			if ( overrideJumpRightAttackMove != LS_INVALID )
			{//overridden with another move
				return overrideJumpRightAttackMove;
			}
			else
			
			{
				vec3_t right, fwdAngles;

				VectorSet(fwdAngles, 0.0f, pm->ps->viewangles[YAW], 0.0f);

				AngleVectors( fwdAngles, NULL, right, NULL );
				pm->ps->velocity[0] = pm->ps->velocity[1] = 0.0f; 
				VectorMA( pm->ps->velocity, 190.0f, right, pm->ps->velocity );
				if ( pm->ps->fd.saberAnimLevel == SS_STAFF )
				{
					newmove = LS_BUTTERFLY_RIGHT;
					pm->ps->velocity[2] = 350.0f;
				}
				else if ( allowCartwheels )
				{
					//PM_SetJumped( JUMP_VELOCITY, qtrue );
					PM_AddEvent( EV_JUMP );
					pm->ps->velocity[2] = 300.0f;

					//if ( !Q_irand( 0, 1 ) )
					//if (PM_GroundDistance() >= 25.0f)
					if (1)
					{
						newmove = LS_JUMPATTACK_ARIAL_RIGHT;
					}
					else
					{
						newmove = LS_JUMPATTACK_CART_RIGHT;
					}
				}
			}
		}
		else if ( pm->cmd.forwardmove > 0 )
		{//forward right = TL2BR slash
			newmove = LS_A_TL2BR;
		}
		else if ( pm->cmd.forwardmove < 0 )
		{//backward right = BL2TR uppercut
			newmove = LS_A_BL2TR;
		}
		else
		{//just right is a left slice
			newmove = LS_A_L2R;
		}
	}
	else if ( pm->cmd.rightmove < 0 )
	{//moving left
		saberInfo_t *saber1 = BG_MySaber( pm->ps->clientNum, 0 );
		saberInfo_t *saber2 = BG_MySaber( pm->ps->clientNum, 1 );
		if ( !noSpecials
			&& overrideJumpLeftAttackMove != LS_NONE
			&& pm->ps->velocity[2] > 20.0f //pm->ps->groundEntityNum != ENTITYNUM_NONE//on ground
			&& (pm->cmd.buttons&BUTTON_ATTACK)//hitting attack
			&& PM_GroundDistance() < 70.0f //not too high above ground
			&& ( pm->cmd.upmove > 0 || (pm->ps->pm_flags & PMF_JUMP_HELD) )//focus-holding player
			&& BG_EnoughForcePowerForMove( SABER_ALT_ATTACK_POWER_LR ) )//have enough power
		{//cartwheel left
			if(saber1 && !saber2 && pm->ps->fd.saberAnimLevel == SS_DUAL)//for a part of single dual/ataru's. 1 point cartwheels
			{
				BG_ForcePowerDrain(pm->ps, FP_GRIP, SABER_ALT_ATTACK_POWER_LRA);

			}
			else
			{
				BG_ForcePowerDrain(pm->ps, FP_GRIP, SABER_ALT_ATTACK_POWER_LR);
			}

			if ( overrideJumpLeftAttackMove != LS_INVALID )
			{//overridden with another move
				return overrideJumpLeftAttackMove;
			}
			else
			{
				vec3_t right, fwdAngles;

				VectorSet(fwdAngles, 0.0f, pm->ps->viewangles[YAW], 0.0f);
				AngleVectors( fwdAngles, NULL, right, NULL );
				pm->ps->velocity[0] = pm->ps->velocity[1] = 0.0f; 
				VectorMA( pm->ps->velocity, -190.0f, right, pm->ps->velocity );
				if ( pm->ps->fd.saberAnimLevel == SS_STAFF )
				{
					newmove = LS_BUTTERFLY_LEFT;
					pm->ps->velocity[2] = 250.0f;
				}
				else if ( allowCartwheels )
				{
					//PM_SetJumped( JUMP_VELOCITY, qtrue );
					PM_AddEvent( EV_JUMP );
					pm->ps->velocity[2] = 350.0f;

					//if ( !Q_irand( 0, 1 ) )
					//if (PM_GroundDistance() >= 25.0f)
					if (1)
					{
						newmove = LS_JUMPATTACK_ARIAL_LEFT;
					}
					else
					{
						newmove = LS_JUMPATTACK_CART_LEFT;
					}
				}
			}
		}
		else if ( pm->cmd.forwardmove > 0 )
		{//forward left = TR2BL slash
			newmove = LS_A_TR2BL;
		}
		else if ( pm->cmd.forwardmove < 0 )
		{//backward left = BR2TL uppercut
			newmove = LS_A_BR2TL;
		}
		else
		{//just left is a right slice
			newmove = LS_A_R2L;
		}
	}
	else
	{//not moving left or right
		if ( pm->cmd.forwardmove > 0 )
		{//forward= T2B slash
			if (!noSpecials&&
				(pm->ps->fd.saberAnimLevel == SS_DUAL || pm->ps->fd.saberAnimLevel == SS_STAFF) &&
				pm->ps->fd.forceRageRecoveryTime < pm->cmd.serverTime &&
				//pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1 &&
				(pm->ps->groundEntityNum != ENTITYNUM_NONE || PM_GroundDistance() <= 40) &&
				pm->ps->velocity[2] >= 0 &&
				(pm->cmd.upmove > 0 || pm->ps->pm_flags & PMF_JUMP_HELD) &&
				!BG_SaberInTransitionAny(pm->ps->saberMove) &&
				!BG_SaberInAttack(pm->ps->saberMove) &&
				pm->ps->weaponTime <= 0 &&
				pm->ps->forceHandExtend == HANDEXTEND_NONE &&
				(pm->cmd.buttons & BUTTON_ATTACK)&&
				//[SaberSys]
				BG_EnoughForcePowerForMove(FATIGUE_JUMPATTACK) )
				//BG_EnoughForcePowerForMove(SABER_ALT_ATTACK_POWER_FB) )
				//[/SaberSys]
			{ //DUAL/STAFF JUMP ATTACK
				newmove = PM_SaberJumpAttackMove2();
				if ( newmove != LS_A_T2B 
					&& newmove != LS_NONE )
				{
					//[SaberSys]
					BG_ForcePowerDrain(pm->ps, FP_GRIP, FATIGUE_JUMPATTACK);
					//BG_ForcePowerDrain(pm->ps, FP_GRIP, SABER_ALT_ATTACK_POWER_FB);
					//[/SaberSys]
				}
				//[/SaberSys]
				
			}
			else if (!noSpecials&&
				//[SaberSys]
				//all single saber styles now have an overhead slash move
				(pm->ps->fd.saberAnimLevel >= SS_FAST &&
				pm->ps->fd.saberAnimLevel <= SS_TAVION) &&
				//pm->ps->fd.saberAnimLevel == SS_MEDIUM &&
				//[/SaberSys]
				pm->ps->velocity[2] > 100 &&
				PM_GroundDistance() < 32 &&
				!BG_InSpecialJump(pm->ps->legsAnim) &&
				!BG_SaberInSpecialAttack(pm->ps->torsoAnim)&&
				//[SaberSys]
				BG_EnoughForcePowerForMove(FATIGUE_JUMPATTACK))
				//BG_EnoughForcePowerForMove(SABER_ALT_ATTACK_POWER_FB))
				//[/SaberSys]
			{ //FLIP AND DOWNWARD ATTACK
				//trace_t tr;

				//if (PM_SomeoneInFront(&tr))
				{
					newmove = PM_SaberFlipOverAttackMove();
					if ( newmove != LS_A_T2B
						&& newmove != LS_NONE )
					{
						//[SaberSys]
						BG_ForcePowerDrain(pm->ps, FP_GRIP, FATIGUE_JUMPATTACK);
						//BG_ForcePowerDrain(pm->ps, FP_GRIP, SABER_ALT_ATTACK_POWER_FB);
						//[/SaberSys]
					}

				}
			}
			//[SaberSys]
			/* basejka - moved up to the saber attack code so that DFA's have to be preceeded by a windup swing.
			else if (!noSpecials&&
				pm->ps->fd.saberAnimLevel == SS_STRONG &&
				pm->ps->velocity[2] > 100 &&
				PM_GroundDistance() < 32 &&
				!BG_InSpecialJump(pm->ps->legsAnim) &&
				!BG_SaberInSpecialAttack(pm->ps->torsoAnim)&&
				BG_EnoughForcePowerForMove( SABER_ALT_ATTACK_POWER_FB ))
			{ //DFA
				//trace_t tr;

				//if (PM_SomeoneInFront(&tr))
				{
					newmove = PM_SaberJumpAttackMove();
					if ( newmove != LS_A_T2B
						&& newmove != LS_NONE )
					{
						BG_ForcePowerDrain(pm->ps, FP_GRIP, SABER_ALT_ATTACK_POWER_FB);
					}
				}
			}
			*/
			//[/SaberSys]
			//[SaberSys]
			//all single saber styles can now do lunges.
			else if (
			//else if ((pm->ps->fd.saberAnimLevel == SS_FAST || pm->ps->fd.saberAnimLevel == SS_DUAL || pm->ps->fd.saberAnimLevel == SS_STAFF) &&
			//[/SaberSys]
				pm->ps->groundEntityNum != ENTITYNUM_NONE &&
				(pm->ps->pm_flags & PMF_DUCKED) &&
				pm->ps->weaponTime <= 0 &&
				!BG_SaberInSpecialAttack(pm->ps->torsoAnim)&&
				//[SaberSys]
				BG_EnoughForcePowerForMove(FATIGUE_GROUNDATTACK) &&
				!PM_SaberInBounce(curmove)) //can't combo into a lunge move 
				//BG_EnoughForcePowerForMove(SABER_ALT_ATTACK_POWER_FB))
				//[/SaberSys]
			{ //LUNGE (weak)
				newmove = PM_SaberLungeAttackMove( noSpecials );
				if ( newmove != LS_A_T2B
					&& newmove != LS_NONE )
				{
					//[SaberSys]
					BG_ForcePowerDrain(pm->ps, FP_GRIP, FATIGUE_GROUNDATTACK);
					//BG_ForcePowerDrain(pm->ps, FP_GRIP, SABER_ALT_ATTACK_POWER_FB);
					//[/SaberSys]
				}

			}
			else if ( !noSpecials )
			{
				saberMoveName_t stabDownMove = PM_CheckStabDown();
				if (stabDownMove != LS_NONE 
					//[SaberSys]
					&& BG_EnoughForcePowerForMove(FATIGUE_GROUNDATTACK) )
					//&& BG_EnoughForcePowerForMove(SABER_ALT_ATTACK_POWER_FB) )
					//[/SaberSys]
				{//racc - stab down at someone on the ground.
					newmove = stabDownMove;
					//[SaberSys]
					BG_ForcePowerDrain(pm->ps, FP_GRIP, FATIGUE_GROUNDATTACK);

					//BG_ForcePowerDrain(pm->ps, FP_GRIP, SABER_ALT_ATTACK_POWER_FB);
					//[/SaberSys]
				}
				else
				{
					newmove = LS_A_T2B;
				}
			}
		}
		else if ( pm->cmd.forwardmove < 0 )
		{//backward= T2B slash//B2T uppercut?
			if (!noSpecials&&
				pm->ps->fd.saberAnimLevel == SS_STAFF &&
				pm->ps->fd.forceRageRecoveryTime < pm->cmd.serverTime &&
				pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1 &&
				(pm->ps->groundEntityNum != ENTITYNUM_NONE || PM_GroundDistance() <= 40) &&
				pm->ps->velocity[2] >= 0 &&
				(pm->cmd.upmove > 0 || pm->ps->pm_flags & PMF_JUMP_HELD) &&
				!BG_SaberInTransitionAny(pm->ps->saberMove) &&
				!BG_SaberInAttack(pm->ps->saberMove) &&
				pm->ps->weaponTime <= 0 &&
				pm->ps->forceHandExtend == HANDEXTEND_NONE &&
				(pm->cmd.buttons & BUTTON_ATTACK))
			{ //BACKFLIP ATTACK
				newmove = PM_SaberBackflipAttackMove();
			}
			else if (PM_CanBackstab() && !BG_SaberInSpecialAttack(pm->ps->torsoAnim))
			{ //BACKSTAB (attack varies by level)
				if (pm->ps->fd.saberAnimLevel >= FORCE_LEVEL_2 && pm->ps->fd.saberAnimLevel != SS_STAFF)
				{//medium and higher attacks
					if ( (pm->ps->pm_flags&PMF_DUCKED) || pm->cmd.upmove < 0 )
					{
						newmove = LS_A_BACK_CR;
					}
					else
					{
						newmove = LS_A_BACK;
					}
				}
				else
				{ //weak attack
					newmove = LS_A_BACKSTAB;
				}
			}
			else
			{
				newmove = LS_A_T2B;
			}
		}
		//[SaberSys]
		else if ( PM_SaberInBounce( curmove ) 
			|| PM_SaberInParry( curmove ) 
			|| PM_SaberInBrokenParry( curmove )
			|| PM_SaberInKnockaway( curmove ) )
		{//bounces, parries, etc return to the start position if a direction isn't given.
			newmove = LS_READY;
		}
		/*
		else if ( PM_SaberInBounce( curmove ) )
		{//bounces should go to their default attack if you don't specify a direction but are attacking
			newmove = saberMoveData[curmove].chain_attack;

			if ( PM_SaberKataDone(curmove, newmove) )
			{
				newmove = saberMoveData[curmove].chain_idle;
			}
			else
			{
				newmove = saberMoveData[curmove].chain_attack;
			}
		}
		*/
		//[/SaberSys]
		else if ( curmove == LS_READY )
		{//Not moving at all, shouldn't have gotten here...?
			//for now, just pick a random attack
			//newmove = Q_irand( LS_A_TL2BR, LS_A_T2B );
			//rww - If we don't seed with a "common" value, the client and server will get mismatched
			//prediction values. Under laggy conditions this will cause the appearance of rapid swing
			//sequence changes.

			newmove = LS_A_T2B; //decided we don't like random attacks when idle, use an overhead instead.
		}
	}

	if (pm->ps->fd.saberAnimLevel == SS_DUAL)
	{
		if ( ( newmove == LS_A_R2L || newmove == LS_S_R2L
					|| newmove == LS_A_L2R  || newmove == LS_S_L2R )
			&& PM_CanDoDualDoubleAttacks()
			&& PM_CheckEnemyPresence( DIR_RIGHT, 100.0f )
			&& PM_CheckEnemyPresence( DIR_LEFT, 100.0f ) )
		{//enemy both on left and right
			newmove = LS_DUAL_LR;
			//probably already moved, but...
			pm->cmd.rightmove = 0;
		}
		else if ( (newmove == LS_A_T2B || newmove == LS_S_T2B
					|| newmove == LS_A_BACK || newmove == LS_A_BACK_CR )
			&& PM_CanDoDualDoubleAttacks()
			&& PM_CheckEnemyPresence( DIR_FRONT, 100.0f )
			&& PM_CheckEnemyPresence( DIR_BACK, 100.0f ) )
		{//enemy both in front and back
			newmove = LS_DUAL_FB;
			//probably already moved, but...
			pm->cmd.forwardmove = 0;
		}
	}

//[AotCAI]
#ifdef QAGAME
//#ifndef CLIENTCOMPILE
	if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT 
		&& FindBotType(pm->ps->clientNum) == BOT_AOTC)
	{// Some special bot stuff. (AIMod).
		if ( nextcheck[pm->ps->clientNum] <= level.time && bot_thinklevel.integer >= 0 )
		{
			int check_val = 0; // Times 500 for next check interval.

			if (PM_JMCanBackstab())
			{ //Bot BACKSTAB (attack varies by level) (This one for JM and above bots!)
				if ( (pm->ps->pm_flags&PMF_DUCKED) || pm->cmd.upmove < 0 )
				{
					newmove = LS_A_BACK_CR;
				}
				else
				{
					int choice = rand()%3;

					if ( choice == 1 )
					{
						newmove = LS_A_BACK;
					}
					else if ( choice == 2 )
					{
						newmove = PM_SaberBackflipAttackMove();
					}
					else if ( choice == 3 )
					{
						newmove = LS_A_BACKFLIP_ATK;
					}
					else
					{
						newmove = LS_A_BACKSTAB;
					}
				}
			}
			else if (PM_CanBackstab())
			{ //Bot BACKSTAB (attack varies by level)
				if ( (pm->ps->pm_flags&PMF_DUCKED) || pm->cmd.upmove < 0 )
				{
					newmove = LS_A_BACK_CR;
				}
				else
				{
					newmove = LS_A_BACK;
				}
			}
			else if (PM_JMCanLunge())
			{ //Bot Lunge (attack varies by level)  (This one for JM and above bots!)
				int choice = rand()%12;

				if (choice == 1
					&& pm->ps->fd.saberAnimLevel == SS_DUAL)
				{
					newmove = PM_SaberDualJumpAttackMove();
				}
				else if (choice == 2
					&& pm->ps->fd.saberAnimLevel == SS_DUAL)
				{
					newmove = LS_SPINATTACK_DUAL;
				}
				else if (choice == 1)
				{
					newmove = LS_SPINATTACK_ALORA;
				}
				else if (choice == 2)
				{
					newmove = LS_A1_SPECIAL;
				}
				else if (choice == 3)
				{
					newmove = LS_A2_SPECIAL;
				}
				else if (choice == 4)
				{
					newmove = LS_A3_SPECIAL;
				}
				else if (choice == 5)
				{
					newmove = LS_SPINATTACK;
				}
				else if (choice == 6)
				{
					newmove = LS_BUTTERFLY_RIGHT;
				}
				else if (choice == 7)
				{
					newmove = LS_BUTTERFLY_LEFT;
				}
				else if (choice == 8)
				{
					newmove = LS_JUMPATTACK_STAFF_RIGHT;
				}
				else if (choice == 9)
				{
					newmove = LS_JUMPATTACK_STAFF_LEFT;
				}
				else if (choice == 10)
				{
					newmove = PM_SaberFlipOverAttackMove();
				}
				else if (choice == 11)
				{
					newmove = PM_SaberJumpAttackMove2();
				}
				else if ( ((pm->ps->pm_flags&PMF_DUCKED) || pm->cmd.upmove < 0)
					&& choice == 12) // Less often when close.
				{
					newmove = PM_SaberJumpAttackMove();
				}
				else
				{
					newmove = PM_SaberLungeAttackMove(qfalse);
				}
			}
			else if (PM_CanLunge())
			{ //Bot Lunge (attack varies by level)
				int choice = rand()%4;

				if (choice == 1)
				{
					newmove = PM_SaberDualJumpAttackMove();
				}
				else if (choice == 2)
				{
					newmove = PM_SaberFlipOverAttackMove();
				}
				else if (choice == 3)
				{
					newmove = PM_SaberJumpAttackMove2();
				}
				else if ( ((pm->ps->pm_flags&PMF_DUCKED) || pm->cmd.upmove < 0)
					&& choice == 4) // Less often when close.
				{
					newmove = PM_SaberLungeAttackMove(qfalse);
				}
				else
				{
					newmove = PM_SaberJumpAttackMove();					
				}
			}

			//check_val = (7 - bot_thinklevel.integer);

			check_val = bot_thinklevel.integer;
			
			if (check_val <= 0)
				check_val = 1;

			nextcheck[pm->ps->clientNum] = level.time + (20000 / check_val); // 20 secs / bot_thinklevel
		}
	}
#endif
//[/AotCAI]

	return newmove;
}

int PM_KickMoveForConditions(void)
{
	int kickMove = -1;

	//FIXME: only if FP_SABER_OFFENSE >= 3
	if ( pm->cmd.rightmove )
	{//kick to side
		if ( pm->cmd.rightmove > 0 )
		{//kick right
			kickMove = LS_KICK_R;
		}
		else
		{//kick left
			kickMove = LS_KICK_L;
		}
		pm->cmd.rightmove = 0;
	}
	else if ( pm->cmd.forwardmove )
	{//kick front/back
		if ( pm->cmd.forwardmove > 0 )
		{//kick fwd
			//[MELEE]
			//Added back in the hilt bash.
			if (pm->ps->groundEntityNum != ENTITYNUM_NONE &&
				//[SaberSys]
				//don't use the hilt bash when we're not using our saber.
				pm->ps->weapon == WP_SABER &&
				pm->ps->fd.saberAnimLevel == SS_STAFF && //don't hilt bash without the staff
				//[/SaberSys]
				PM_CheckEnemyPresence( DIR_FRONT, 64.0f ))
			{
				kickMove = LS_HILT_BASH;
			}
			else
			//[/MELEE]
			{
				kickMove = LS_KICK_F;
			}
		}
		else
		{//kick back
			kickMove = LS_KICK_B;
		}
		pm->cmd.forwardmove = 0;
	}
	else
	{
		//if (pm->cmd.buttons & BUTTON_ATTACK)
		//if (pm->ps->pm_flags & PMF_JUMP_HELD)
		if (0)
		{ //ok, let's try some fancy kicks
			//qboolean is actually of type int anyway, but just for safeness.
			int front = (int)PM_CheckEnemyPresence( DIR_FRONT, 100.0f );
			int back = (int)PM_CheckEnemyPresence( DIR_BACK, 100.0f );
			int right = (int)PM_CheckEnemyPresence( DIR_RIGHT, 100.0f );
			int left = (int)PM_CheckEnemyPresence( DIR_LEFT, 100.0f );
			int numEnemy = front+back+right+left;

			if (numEnemy >= 3 ||
				((!right || !left) && numEnemy >= 2))
			{ //> 2 enemies near, or, >= 2 enemies near and they are not to the right and left.
                kickMove = LS_KICK_S;
			}
			else if (right && left)
			{ //enemies on both sides
				kickMove = LS_KICK_RL;
			}
			else
			{ //oh well, just do a forward kick
				kickMove = LS_KICK_F;
			}

			pm->cmd.upmove = 0;
		}
	}

	return kickMove;
}

qboolean BG_InSlopeAnim( int anim );
qboolean PM_RunningAnim( int anim );

qboolean PM_SaberMoveOkayForKata( void )
{
	if ( pm->ps->saberMove == LS_READY
		|| PM_SaberInStart( pm->ps->saberMove ) )
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

//[RACC] - check to see if you can do this kata or not.
qboolean PM_CanDoKata( void )
{
	if ( PM_InSecondaryStyle() )
	{
		return qfalse;
	}

	if ( !pm->ps->saberInFlight//not throwing saber
		&& PM_SaberMoveOkayForKata()
		&& !BG_SaberInKata(pm->ps->saberMove)
		&& !BG_InKataAnim(pm->ps->legsAnim)
		&& !BG_InKataAnim(pm->ps->torsoAnim)
		/*
		&& pm->ps->saberAnimLevel >= SS_FAST//fast, med or strong style
		&& pm->ps->saberAnimLevel <= SS_STRONG//FIXME: Tavion, too?
		*/
		&& pm->ps->groundEntityNum != ENTITYNUM_NONE//not in the air
		&& (pm->cmd.buttons&BUTTON_ATTACK)//pressing attack
		&& (pm->cmd.buttons&BUTTON_ALT_ATTACK)//pressing alt attack
		&& !pm->cmd.forwardmove//not moving f/b
		&& !pm->cmd.rightmove//not moving r/l
		&& pm->cmd.upmove <= 0//not jumping...?
		&& BG_EnoughForcePowerForMove(SABER_ALT_ATTACK_POWER) )// have enough power
	{//FIXME: check rage, etc...
		saberInfo_t *saber = BG_MySaber( pm->ps->clientNum, 0 );
		if ( saber
			&& saber->kataMove == LS_NONE )
		{//kata move has been overridden in a way that should stop you from doing it at all
			return qfalse;
		}
		saber = BG_MySaber( pm->ps->clientNum, 1 );
		if ( saber
			&& saber->kataMove == LS_NONE )
		{//kata move has been overridden in a way that should stop you from doing it at all
			return qfalse;
		}
		return qtrue;
	}
	return qfalse;
}


//[SaberSys]
//ported from SP code
qboolean PM_SaberThrowable( void )
{
	saberInfo_t *saber = BG_MySaber( pm->ps->clientNum, 0 );
	if(!saber)
	{//this is bad, just drop out.
		return qfalse;
	}

	/* racc - popped this off to see if we don't need it.
	//ugh, hard-coding this is bad...
	if ( pm->ps->saberAnimLevel == SS_STAFF )
	{
		return qfalse;
	}
	*/
	
	if ( !(saber->saberFlags&SFL_NOT_THROWABLE) )
	{//yes, this saber is always throwable
		return qtrue;
	}
	
	//saber is not normally throwable
	if ( (saber->saberFlags&SFL_SINGLE_BLADE_THROWABLE) )
	{//it is throwable if only one blade is on
		if ( saber->numBlades > 1 )
		{//it has more than one blade
			int i = 0;
			int numBladesActive = 0;
			for (; i < saber->numBlades; i++ )
			{
				if ( saber->blade[i].active )
				{
					numBladesActive++;
				}
			}
			if ( numBladesActive == 1 )
			{//only 1 blade is on
				return qtrue;
			}
		}
	}
	//nope, can't throw it
	return qtrue;
}
//[/SaberSys]


qboolean PM_CheckAltKickAttack( void )
{
	if ( pm->ps->weapon == WP_SABER )
	{
		saberInfo_t *saber = BG_MySaber( pm->ps->clientNum, 0 );
		if ( saber
			&& (saber->saberFlags&SFL_NO_KICKS) )
		{
			return qfalse;
		}
		saber = BG_MySaber( pm->ps->clientNum, 1 );
		if ( saber
			&& (saber->saberFlags&SFL_NO_KICKS) )
		{
			return qfalse;
		}
	}
	if ( (pm->cmd.buttons&BUTTON_ALT_ATTACK) 
		//&& (!(pm->ps->pm_flags&PMF_ALT_ATTACK_HELD)||PM_SaberInReturn(pm->ps->saberMove))
		&& (!BG_FlippingAnim(pm->ps->legsAnim)||pm->ps->legsTimer<=250)
		&& (pm->ps->fd.saberAnimLevel == SS_STAFF/*||!pm->ps->saber[0].throwable*/) && !pm->ps->saberHolstered )
	{
		return qtrue;
	}
	return qfalse;
}

int bg_parryDebounce[NUM_FORCE_POWER_LEVELS] =
{
	500,//if don't even have defense, can't use defense!
	300,
	150,
	50
};

//[RACC] - PM check to make sure you have force power to do a saber throw.
qboolean PM_SaberPowerCheck(void)
{
	if (pm->ps->saberInFlight)
	{ //so we don't keep doing stupid force out thing while guiding saber.
		if (pm->ps->fd.forcePower > forcePowerNeeded[pm->ps->fd.forcePowerLevel[FP_SABERTHROW]][FP_SABERTHROW])
		{
			return qtrue;
		}
	}
	else
	{
		return BG_EnoughForcePowerForMove(forcePowerNeeded[pm->ps->fd.forcePowerLevel[FP_SABERTHROW]][FP_SABERTHROW]);
	}

	return qfalse;
}

qboolean PM_CanDoRollStab( void )
{
	//[DodgeSys]
	//Can't roll stab in a Dodge Roll.
	if ( pm->ps->weapon == WP_SABER && !(pm->ps->userInt3 & (1 << FLAG_DODGEROLL)) )
	//if ( pm->ps->weapon == WP_SABER )
	{
		saberInfo_t *saber = BG_MySaber( pm->ps->clientNum, 0 );
		if ( saber
			&& (saber->saberFlags&SFL_NO_ROLL_STAB) )
		{
			return qfalse;
		}
		saber = BG_MySaber( pm->ps->clientNum, 1 );
		if ( saber
			&& (saber->saberFlags&SFL_NO_ROLL_STAB) )
		{
			return qfalse;
		}

		return qtrue;
	}
	//you shouldn't be able to roll stab with a non-saber weapon selected.
	else
	{
		return qfalse;
	}
	//[/DodgeSys]
	
}


//[CoOp]
void PM_SaberDroidWeapon( void )
{
	// make weapon function
	if ( pm->ps->weaponTime > 0 ) {
		pm->ps->weaponTime -= pml.msec;
		if ( pm->ps->weaponTime <= 0 )
		{
			pm->ps->weaponTime = 0;
		}
	}

	// Now we react to a block action by the player's lightsaber.
	if ( pm->ps->saberBlocked )
	{
		switch ( pm->ps->saberBlocked )
		{
			case BLOCKED_PARRY_BROKEN:
				PM_SetAnim( SETANIM_BOTH, Q_irand(BOTH_PAIN1,BOTH_PAIN3), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );
				pm->ps->weaponTime = pm->ps->legsTimer;
				break;
			case BLOCKED_ATK_BOUNCE:
				PM_SetAnim( SETANIM_BOTH, Q_irand(BOTH_PAIN1,BOTH_PAIN3), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );
				pm->ps->weaponTime = pm->ps->legsTimer;
				break;
			case BLOCKED_UPPER_RIGHT:
			case BLOCKED_UPPER_RIGHT_PROJ:
			case BLOCKED_LOWER_RIGHT:
			case BLOCKED_LOWER_RIGHT_PROJ:
				PM_SetAnim( SETANIM_BOTH, BOTH_P1_S1_TR, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );
				pm->ps->legsTimer += Q_irand( 200, 1000 );
				pm->ps->weaponTime = pm->ps->legsTimer;
				break;
			case BLOCKED_UPPER_LEFT:
			case BLOCKED_UPPER_LEFT_PROJ:
			case BLOCKED_LOWER_LEFT:
			case BLOCKED_LOWER_LEFT_PROJ:
				PM_SetAnim( SETANIM_BOTH, BOTH_P1_S1_TL, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );
				pm->ps->legsTimer += Q_irand( 200, 1000 );
				pm->ps->weaponTime = pm->ps->legsTimer;
				break;
			case BLOCKED_TOP:
			case BLOCKED_TOP_PROJ:
				PM_SetAnim( SETANIM_BOTH, BOTH_P1_S1_T_, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );
				pm->ps->legsTimer += Q_irand( 200, 1000 );
				pm->ps->weaponTime = pm->ps->legsTimer;
				break;
			default:
				pm->ps->saberBlocked = BLOCKED_NONE;
				break;
		}

		pm->ps->saberBlocked = BLOCKED_NONE;
		pm->ps->saberMove = LS_NONE;	//pm->ps->saberBounceMove = LS_NONE;
		pm->ps->weaponstate = WEAPON_READY;
		
		// Done with block, so stop these active weapon branches.
		return;
	}
}
//[/CoOp]


/*
=================
PM_WeaponLightsaber

Consults a chart to choose what to do with the lightsaber.
While this is a little different than the Quake 3 code, there is no clean way of using the Q3 code for this kind of thing.
=================
*/
// Ultimate goal is to set the sabermove to the proper next location
// Note that if the resultant animation is NONE, then the animation is essentially "idle", and is set in WP_TorsoAnim
qboolean PM_WalkingAnim( int anim );
qboolean PM_SwimmingAnim( int anim );
int PM_SaberBounceForAttack( int move );
qboolean BG_SuperBreakLoseAnim( int anim );
qboolean BG_SuperBreakWinAnim( int anim );

//[CoOp]
extern bgEntity_t *pm_entSelf;
//[/CoOp]

//[SaberSys]
/*  NUAM
//Sets your saber block position based on your current movement commands.
//Be careful as this assumes that you have already done all the nessicary prechecks.
void PM_SetBlock(void)
{
	//for now I'll assume that we're using an inverted control system.
	if( pm->cmd.forwardmove < 0 )
	{
		if( pm->cmd.rightmove < 0 )
		{//upper left block
			pm->ps->saberBlocked = BLOCKED_UPPER_LEFT;
		}
		else if ( pm->cmd.rightmove > 0 )
		{//upper right block
			pm->ps->saberBlocked = BLOCKED_UPPER_RIGHT;
		}
		else
		{//Top Block
			pm->ps->saberBlocked = BLOCKED_TOP;
		}
	}
	else if ( pm->cmd.forwardmove > 0 )
	{				
		if( pm->cmd.rightmove < 0 )
		{//lower left block
			pm->ps->saberBlocked = BLOCKED_LOWER_LEFT;
		}
		else if ( pm->cmd.rightmove > 0 )
		{//lower right block
			pm->ps->saberBlocked = BLOCKED_LOWER_RIGHT;
		}
		else
		{//This would be a bottom block, but that doesn't exist at the moment.  Just 
			//do nothing
		}
	}
	else
	{
		if( pm->cmd.rightmove < 0 )
		{//left block doesn't exist so we just use the upper blocks for now.
			pm->ps->saberBlocked = BLOCKED_UPPER_LEFT;
		}
		else if ( pm->cmd.rightmove > 0 )
		{//right block doesn't exist so we just use the upper blocks for now.
			pm->ps->saberBlocked = BLOCKED_UPPER_RIGHT;
		}
		else
		{//centered block.  This is just the ready stance I guess.
		}
	}
}
*/


int PM_ReturnforQuad( int quad )
{
	switch (quad)
	{
		case Q_BR:
			return LS_R_TL2BR;
			break;
		case Q_R:
			return LS_R_L2R;
			break;
		case Q_TR:
			return LS_R_BL2TR;
			break;
		case Q_T:
		//no attacks ever end here (IE no animation) so just wing it.
			//[SaberSys]
			//hack this to use a similar quad so that it had consistant animation timing.
			return LS_R_BL2TR;
			//return LS_READY;
			//[/SaberSys]
			break;
		case Q_TL:
			return LS_R_BR2TL;
			break;
		case Q_L:
			return LS_R_R2L;
			break;
		case Q_BL:
			return LS_R_TR2BL;
			break;
		case Q_B:
			return LS_R_T2B;
			break;
		default:
			return LS_READY;
	};
}


//[SaberSys]
int BlockedforQuad(int quad)
{//returns the saberBlocked direction for given quad.
	switch (quad)
	{
		case Q_BR:
			return BLOCKED_LOWER_RIGHT;
			break;
		case Q_R:
			return BLOCKED_UPPER_RIGHT;
			break;
		case Q_TR:
			return BLOCKED_UPPER_RIGHT;
			break;
		case Q_T:
			return BLOCKED_TOP;
			break;
		case Q_TL:
			return BLOCKED_UPPER_LEFT;
			break;
		case Q_L:
			return BLOCKED_UPPER_LEFT;
			break;
		case Q_BL:
			return BLOCKED_LOWER_LEFT;
			break;
		case Q_B:
			return BLOCKED_LOWER_LEFT;
			break;
		default:
			return BLOCKED_TOP;
	};
}
//[/SaberSys]



int PM_DoFake(int curmove)
{
	int newQuad = -1;

	if(pm->ps->userInt3 & (1 << FLAG_ATTACKFAKE)) 
	{//already attack faking, can't do another one until this one is over.
		return LS_NONE;
	}

	if ( pm->cmd.rightmove > 0 )
	{//moving right
		if ( pm->cmd.forwardmove > 0 )
		{//forward right = TL2BR slash
			newQuad = Q_TL;
		}
		else if ( pm->cmd.forwardmove < 0 )
		{//backward right = BL2TR uppercut
			newQuad = Q_BL;
		}
		else
		{//just right is a left slice
			newQuad = Q_L;
		}
	}
	else if ( pm->cmd.rightmove < 0 )
	{//moving left
		if ( pm->cmd.forwardmove > 0 )
		{//forward left = TR2BL slash
			newQuad = Q_TR;
		}
		else if ( pm->cmd.forwardmove < 0 )
		{//backward left = BR2TL uppercut
			newQuad = Q_BR;
		}
		else
		{//just left is a right slice
			newQuad = Q_R;
		}
	}
	else
	{//not moving left or right
		if ( pm->cmd.forwardmove > 0 )
		{//forward= T2B slash
			newQuad = Q_T;
		}
		else if ( pm->cmd.forwardmove < 0 )
		{//backward= T2B slash	//or B2T uppercut?
			newQuad = Q_T;
		}
		else
		{//Not moving at all
		}
	}

	if(newQuad == -1)
	{//assume that we're trying to fake in our current direction so we'll automatically fake 
		//in the completely opposite direction.  This allows the player to do a fake while standing still.
		newQuad = saberMoveData[curmove].endQuad;
	}

	if ( newQuad == saberMoveData[curmove].endQuad )
	{//player is attempting to do a fake move to the same quadrant 
		//as such, fake to the completely opposite quad
		newQuad += 4;
		if(newQuad > Q_B)
		{//rotated past Q_B, shift back to get the proper quadrant
			newQuad -= Q_NUM_QUADS;
		}
	}

	if(newQuad == Q_B)
	{//attacks can't be launched from this quad, just randomly fake to the bottom left/right
		if(PM_irand_timesync(0, 9) <= 4 )
		{
			newQuad = Q_BL;
		}
		else
		{
			newQuad = Q_BR;
		}

	}

	//add faking flag
	pm->ps->userInt3 |= ( 1 << FLAG_ATTACKFAKE );
	return transitionMove[saberMoveData[curmove].endQuad][newQuad];
}


//[SaberSys]
void BG_AddFatigue( playerState_t * ps, int Fatigue);
//[/SaberSys]
void PM_WeaponLightsaber(void)
{
	int			addTime,amount;
	qboolean	delayed_fire = qfalse;
	int			anim=-1, curmove, newmove=LS_NONE;

	qboolean checkOnlyWeap = qfalse;


	//[CoOp]
	if ( pm_entSelf->s.NPC_class == CLASS_SABER_DROID )
	{//Saber droid does it's own attack logic
		PM_SaberDroidWeapon();
		return;
	}
	//[/CoOp]

	if ( PM_InKnockDown( pm->ps ) || BG_InRoll( pm->ps, pm->ps->legsAnim ))
	{//in knockdown
		// make weapon function
		if ( pm->ps->weaponTime > 0 ) {
			pm->ps->weaponTime -= pml.msec;
			if ( pm->ps->weaponTime <= 0 )
			{
				pm->ps->weaponTime = 0;
			}
		}

		if ( pm->ps->legsAnim == BOTH_ROLL_F 
			&& pm->ps->legsTimer <= 250 )
		{
			if ( (pm->cmd.buttons&BUTTON_ATTACK) )
			{
				//[SaberSys]
				if ( BG_EnoughForcePowerForMove(FATIGUE_GROUNDATTACK) && !pm->ps->saberInFlight )
				//if ( BG_EnoughForcePowerForMove(SABER_ALT_ATTACK_POWER_FB) && !pm->ps->saberInFlight )
				//[SaberSys]
				{
					if ( PM_CanDoRollStab() )
					{
						//make sure the saber is on for this move!
						if ( pm->ps->saberHolstered == 2 )
						{//all the way off
							pm->ps->saberHolstered = 0;
							PM_AddEvent(EV_SABER_UNHOLSTER);
						}
						PM_SetSaberMove( LS_ROLL_STAB );
						//[SaberSys]
						BG_ForcePowerDrain(pm->ps, FP_GRIP, FATIGUE_GROUNDATTACK);

						//BG_ForcePowerDrain(pm->ps, FP_GRIP, SABER_ALT_ATTACK_POWER_FB);
						//[/SaberSys]
					}
				}
			}
		}
		return;
	}

 	if ( pm->ps->saberLockTime > pm->cmd.serverTime )
	{
		pm->ps->saberMove = LS_NONE;
		PM_SaberLocked();
		return;
	}
	else
	{
		if ( /*( (pm->ps->torsoAnim) == BOTH_BF2LOCK ||
				(pm->ps->torsoAnim) == BOTH_BF1LOCK ||
				(pm->ps->torsoAnim) == BOTH_CWCIRCLELOCK ||
				(pm->ps->torsoAnim) == BOTH_CCWCIRCLELOCK ||*/
				pm->ps->saberLockFrame
			)
		{
			if (pm->ps->saberLockEnemy < ENTITYNUM_NONE &&
				pm->ps->saberLockEnemy >= 0)
			{
				bgEntity_t *bgEnt;
				playerState_t *en;

				bgEnt = PM_BGEntForNum(pm->ps->saberLockEnemy);

				if (bgEnt)
				{
					en = bgEnt->playerState;

					if (en)
					{
						PM_SaberLockBreak(en, qfalse, 0);
						return;
					}
				}
			}

			if (/* ( (pm->ps->torsoAnim) == BOTH_BF2LOCK ||
					(pm->ps->torsoAnim) == BOTH_BF1LOCK ||
					(pm->ps->torsoAnim) == BOTH_CWCIRCLELOCK ||
					(pm->ps->torsoAnim) == BOTH_CCWCIRCLELOCK ||*/
					pm->ps->saberLockFrame
				)
			{
				pm->ps->torsoTimer = 0;
				PM_SetAnim(SETANIM_TORSO,BOTH_STAND1,SETANIM_FLAG_OVERRIDE, 100);
				pm->ps->saberLockFrame = 0;
			}
		}
	}

	if ( BG_KickingAnim( pm->ps->legsAnim ) ||
		BG_KickingAnim( pm->ps->torsoAnim ))
	{
		if ( pm->ps->legsTimer > 0 )
		{//you're kicking, no interruptions
			return;
		}
		//done?  be immeditately ready to do an attack
		pm->ps->saberMove = LS_READY;
		pm->ps->weaponTime = 0;
	}

	if ( BG_SuperBreakLoseAnim( pm->ps->torsoAnim )
		|| BG_SuperBreakWinAnim( pm->ps->torsoAnim ) )
	{
		if ( pm->ps->torsoTimer > 0 )
		{//never interrupt these
			return;
		}
	}

	if (BG_SabersOff( pm->ps ))
	{
		if (pm->ps->saberMove != LS_READY)
		{
			PM_SetSaberMove( LS_READY );
		}

		//[RACC] - if your saber is off, set the torso animation to the legs animation unless
		//you are on a slope.
		if ((pm->ps->legsAnim) != (pm->ps->torsoAnim) && !BG_InSlopeAnim(pm->ps->legsAnim) &&
			pm->ps->torsoTimer <= 0)
		{
			PM_SetAnim(SETANIM_TORSO,(pm->ps->legsAnim),SETANIM_FLAG_OVERRIDE, 100);
		}
		else if (BG_InSlopeAnim(pm->ps->legsAnim) && pm->ps->torsoTimer <= 0)
		{
			PM_SetAnim(SETANIM_TORSO,PM_GetSaberStance(),SETANIM_FLAG_OVERRIDE, 100);
		}

		//[RACC]
		//The unholster saber by attacking code.
		//[SaberThrowSys]
		//changed so that we can't activate saber while it's in flight/not in our hand
		if (pm->ps->weaponTime < 1 
			//[MELEE]
			//don't active saber on altattack.
			&& (pm->cmd.buttons & BUTTON_ATTACK)
			//&& ((pm->cmd.buttons & BUTTON_ALT_ATTACK) || (pm->cmd.buttons & BUTTON_ATTACK))
			//[/MELEE]
			&& !pm->ps->saberInFlight)
		//if (pm->ps->weaponTime < 1 && ((pm->cmd.buttons & BUTTON_ALT_ATTACK) || (pm->cmd.buttons & BUTTON_ATTACK)))
		//[/SaberThrowSys]
		{
			if (pm->ps->duelTime < pm->cmd.serverTime)
			{
				if (!pm->ps->m_iVehicleNum)
				{ //don't let em unholster the saber by attacking while on vehicle
					pm->ps->saberHolstered = 0;
					PM_AddEvent(EV_SABER_UNHOLSTER);
				}
				else
				{
					pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;
					pm->cmd.buttons &= ~BUTTON_ATTACK;
				}
			}
		}

		if ( pm->ps->weaponTime > 0 )
		{
			pm->ps->weaponTime -= pml.msec;
		}

		checkOnlyWeap = qtrue;
		goto weapChecks;
	}

	if (!pm->ps->saberEntityNum && pm->ps->saberInFlight)
	{ //this means our saber has been knocked away
		/*
		if (pm->ps->saberMove != LS_READY)
		{
			PM_SetSaberMove( LS_READY );
		}

		if ((pm->ps->legsAnim) != (pm->ps->torsoAnim) && !BG_InSlopeAnim(pm->ps->legsAnim))
		{
			PM_SetAnim(SETANIM_TORSO,(pm->ps->legsAnim),SETANIM_FLAG_OVERRIDE, 100);
		}

		if (BG_InSaberStandAnim(pm->ps->torsoAnim) || pm->ps->torsoAnim == BOTH_SABERPULL)
		{
			PM_SetAnim(SETANIM_TORSO,BOTH_STAND1,SETANIM_FLAG_OVERRIDE, 100);
		}

		return;
		*/
		//Old method, don't want to do this now because we want to finish up reflected attacks and things
		//if our saber is pried out of our hands from one.
		if ( pm->ps->fd.saberAnimLevel == SS_DUAL )
		{
			//[SaberThrowSys]
			if ( pm->ps->saberHolstered > 1 || !pm->ps->saberHolstered  )
			//if ( pm->ps->saberHolstered > 1 )
			{
				pm->ps->saberHolstered = 1;
			}
			//[/SaberThrowSys]
		}
		else
		{
			pm->cmd.buttons &= ~BUTTON_ATTACK;
		}
		pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;
	}

	//[SaberSys]
	//preblocks can be interrupted
	if(PM_SaberInParry(pm->ps->saberMove) && pm->ps->userInt3 & (1 << FLAG_PREBLOCK) // in a pre-block
		&& ((pm->cmd.buttons & BUTTON_ALT_ATTACK) || (pm->cmd.buttons & BUTTON_ATTACK))) //and attempting an attack
		 
	{//interrupting a preblock
		pm->ps->weaponTime = 0;
		pm->ps->torsoTimer = 0;
	}
	
	//Added additional ways of using saber throw.
	if ( (pm->cmd.buttons & BUTTON_SABERTHROW) || ((pm->cmd.buttons & BUTTON_FORCEPOWER) && pm->ps->fd.forcePowerSelected == FP_SABERTHROW) )
	//if ( (pm->cmd.buttons & BUTTON_ALT_ATTACK) )
	//attempting to saber throw.  handle it.
	//[/SaberSys]
	{ //might as well just check for a saber throw right here
		//[SaberSys]
		//this really should be based on weither the saber is throwable or not.
		if(!PM_SaberThrowable())
		//if (pm->ps->fd.saberAnimLevel == SS_STAFF)
		//[/SaberSys]
		{ //kick instead of doing a throw 
			//[Melee]
			if(PM_DoKick())
			{
				return;
			}
			/*
			//if in a saber attack return anim, can interrupt it with a kick
			if ( pm->ps->weaponTime > 0//can't fire yet
				&& PM_SaberInReturn( pm->ps->saberMove )//in a saber return move - FIXME: what about transitions?
				//&& pm->ps->weaponTime <= 250//should be able to fire soon
				//&& pm->ps->torsoTimer <= 250//torso almost done
				&& pm->ps->saberBlocked == BLOCKED_NONE//not interacting with any other saber
				&& !(pm->cmd.buttons&BUTTON_ATTACK) )//not trying to swing the saber
			{
				if ( (pm->cmd.forwardmove||pm->cmd.rightmove)//trying to kick in a specific direction
					&& PM_CheckAltKickAttack() )//trying to do a kick
				{//allow them to do the kick now!
					int kickMove = PM_KickMoveForConditions();
					if (kickMove != -1)
					{
						pm->ps->weaponTime = 0;
						PM_SetSaberMove( kickMove );
						return;
					}
				}
			}
			*/
			//[/Melee]
		}
		//[SaberThrowSys]
		else if ( pm->ps->saberInFlight && pm->ps->saberEntityNum )
		{//saber is already in flight continue moving it with the force.
			PM_SetAnim(SETANIM_TORSO, BOTH_SABERTHROW1START, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 100);
			pm->ps->torsoTimer = 1;
			return;
		}
		//[/SaberThrowSys]
		else if ( pm->ps->weaponTime < 1&&
				pm->ps->saberCanThrow &&
				//pm->ps->fd.forcePower >= forcePowerNeeded[pm->ps->fd.forcePowerLevel[FP_SABERTHROW]][FP_SABERTHROW] &&
				!BG_HasYsalamiri(pm->gametype, pm->ps) &&
				BG_CanUseFPNow(pm->gametype, pm->ps, pm->cmd.serverTime, FP_SABERTHROW) &&
				pm->ps->fd.forcePowerLevel[FP_SABERTHROW] > 0 &&
				PM_SaberPowerCheck() )
		{
			trace_t sabTr;
			vec3_t	fwd, minFwd, sabMins, sabMaxs;

			VectorSet( sabMins, SABERMINS_X, SABERMINS_Y, SABERMINS_Z );
			VectorSet( sabMaxs, SABERMAXS_X, SABERMAXS_Y, SABERMAXS_Z );

			AngleVectors( pm->ps->viewangles, fwd, NULL, NULL );
			VectorMA( pm->ps->origin, SABER_MIN_THROW_DIST, fwd, minFwd );

			pm->trace(&sabTr, pm->ps->origin, sabMins, sabMaxs, minFwd, pm->ps->clientNum, MASK_PLAYERSOLID);

			/*
#ifdef QAGAME
			if(pm->ps->fd.forcePowersActive & (1<<FP_GRIP))
				WP_ForcePowerStop(&g_entities[pm->ps->clientNum],FP_GRIP);
#endif
				*/

			if ( sabTr.allsolid || sabTr.startsolid || sabTr.fraction < 1.0f )
			{//not enough room to throw
			}
			else
			{//throw it
				//This will get set to false again once the saber makes it back to its owner game-side
				if (!pm->ps->saberInFlight)
				{
					pm->ps->fd.forcePower -= forcePowerNeeded[pm->ps->fd.forcePowerLevel[FP_SABERTHROW]][FP_SABERTHROW];
				}

				pm->ps->saberInFlight = qtrue;
			}
		}
	}



	//[SaberThrowSys]
	//replaced by above code.
	/* basejka code
	if ( pm->ps->saberInFlight && pm->ps->saberEntityNum )
	{//guiding saber
		if ( (pm->ps->fd.saberAnimLevel != SS_DUAL //not using 2 sabers
			  || pm->ps->saberHolstered //left one off - FIXME: saberHolstered 1 should be left one off, 0 should be both on, 2 should be both off
			  || (!(pm->cmd.buttons&BUTTON_ATTACK)//not trying to start an attack AND...
				  && (pm->ps->torsoAnim == BOTH_SABERDUAL_STANCE//not already attacking
					  || pm->ps->torsoAnim == BOTH_SABERPULL//not already attacking
					  || pm->ps->torsoAnim == BOTH_STAND1//not already attacking
					  || PM_RunningAnim( pm->ps->torsoAnim ) //not already attacking
					  || PM_WalkingAnim( pm->ps->torsoAnim ) //not already attacking
					  || PM_JumpingAnim( pm->ps->torsoAnim )//not already attacking
					  || PM_SwimmingAnim( pm->ps->torsoAnim ))//not already attacking
				)
			  )
			)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_SABERPULL, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 100);
			pm->ps->torsoTimer = 1;
			return;
		}
	}
	*/
	//[/SaberThrowSys]

   // don't allow attack until all buttons are up
	//This is bad. It freezes the attack state and the animations if you hold the button after respawning, and it looks strange.
	/*
	if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
		return;
	}
	*/

	// check for dead player
	if ( pm->ps->stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	/*

	if (pm->ps->weaponstate == WEAPON_READY ||
		pm->ps->weaponstate == WEAPON_IDLE)
	{
		if (pm->ps->saberMove != LS_READY && pm->ps->weaponTime <= 0 && !pm->ps->saberBlocked)
		{
			PM_SetSaberMove( LS_READY );
		}
	}

	if(PM_RunningAnim(pm->ps->torsoAnim))
	{
		if ((pm->ps->torsoAnim) != (pm->ps->legsAnim))
		{
			PM_SetAnim(SETANIM_TORSO,(pm->ps->legsAnim),SETANIM_FLAG_OVERRIDE, 100);
		}
	}
	*/

	// make weapon function
	if ( pm->ps->weaponTime > 0 )
	{
		//check for special pull move while busy
		saberMoveName_t pullmove = PM_CheckPullAttack();
		if (pullmove != LS_NONE)
		{
			pm->ps->weaponTime = 0;
			pm->ps->torsoTimer = 0;
			pm->ps->legsTimer = 0;
			pm->ps->forceHandExtend = HANDEXTEND_NONE;
			pm->ps->weaponstate = WEAPON_READY;
			PM_SetSaberMove(pullmove);
			return;
		}

		pm->ps->weaponTime -= pml.msec;

		//This was stupid and didn't work right. Looks like things are fine without it.
	//	if (pm->ps->saberBlocked && pm->ps->torsoAnim != saberMoveData[pm->ps->saberMove].animToUse)
	//	{ //rww - keep him in the blocking pose until he can attack again
	//		PM_SetAnim(SETANIM_TORSO,saberMoveData[pm->ps->saberMove].animToUse,saberMoveData[pm->ps->saberMove].animSetFlags|SETANIM_FLAG_HOLD, saberMoveData[pm->ps->saberMove].blendTime);
	//		return;
	//	}
	}
	else
	{
		pm->ps->weaponstate = WEAPON_READY;
	}

	// Now we react to a block action by the player's lightsaber.
	if ( pm->ps->saberBlocked )
	{
		//[RACC] - Hold saber blocks for a bit.
		if ( pm->ps->saberBlocked >= BLOCKED_UPPER_RIGHT 
			&& pm->ps->saberBlocked < BLOCKED_UPPER_RIGHT_PROJ)
		{//hold the parry for a bit
			//[SaberSys]
			//make these blocks be consistent length
			pm->ps->weaponTime = 350;
			//pm->ps->weaponTime = bg_parryDebounce[pm->ps->fd.forcePowerLevel[FP_SABER_DEFENSE]]+200;
			//[/SaberSys]
		}
		switch ( pm->ps->saberBlocked )
		{
			case BLOCKED_BOUNCE_MOVE:
				{ //act as a bounceMove and reset the saberMove instead of using a seperate value for it
					pm->ps->torsoTimer = 0;
					PM_SetSaberMove( pm->ps->saberMove );
					pm->ps->weaponTime = pm->ps->torsoTimer;
					pm->ps->saberBlocked = 0;
				}
				break;
			case BLOCKED_PARRY_BROKEN:
				//whatever parry we were is in now broken, play the appropriate knocked-away anim
				{
					int nextMove;

					if ( PM_SaberInBrokenParry( pm->ps->saberMove ) )
					{//already have one...?
						nextMove = pm->ps->saberMove;
					}
					else
					{
						nextMove = PM_BrokenParryForParry( pm->ps->saberMove );
					}
					if ( nextMove != LS_NONE )
					{
						PM_SetSaberMove( nextMove );
						pm->ps->weaponTime = pm->ps->torsoTimer;
					}
					else
					{//Maybe in a knockaway?
					}
				}
				break;
			case BLOCKED_ATK_BOUNCE:
				// If there is absolutely no blocked move in the chart, don't even mess with the animation.
				// OR if we are already in a block or parry.
				//[SaberSys]
				//only cancel an attack bounce if we're already in a bounce/return, which shouldn't really happen.
				if ( PM_SaberInBounce(pm->ps->saberMove) || PM_SaberInReturn(pm->ps->saberMove) )
				//if (pm->ps->saberMove >= LS_T1_BR__R)
				//[/SaberSys]
				{//an actual bounce?  Other bounces before this are actually transitions?
					pm->ps->saberBlocked = BLOCKED_NONE;
				}
				else
				{
					int bounceMove;

					//[SaberSys]
					//removed the bounce check since it was redundent
					if ( (pm->ps->userInt3 & (1 << FLAG_SLOWBOUNCE) 
						&& !(pm->ps->userInt3 & (1 << FLAG_OLDSLOWBOUNCE)))  //starting a new style slow bounce, always use a return
						|| (!BG_SaberInAttack( pm->ps->saberMove ) && !PM_SaberInStart( pm->ps->saberMove )) )
					//if ( PM_SaberInBounce( pm->ps->saberMove ) || !BG_SaberInAttack( pm->ps->saberMove ) )
					//[/SaberSys]
					{
						//[SaberSys]
						/* This stuff can cause instant attacks/multi hits.
						if ( pm->cmd.buttons & BUTTON_ATTACK )
						{//transition to a new attack
							//racc - this doesn't seem to happen very often since BLOCKED_ATK_BOUNCE
							//terminates after the bounce animation is started. 
							int newQuad = PM_SaberMoveQuadrantForMovement( &pm->cmd );		
							while ( newQuad == saberMoveData[pm->ps->saberMove].startQuad )
							{//player is still in same attack quad, don't repeat that attack because it looks bad, 
								//FIXME: try to pick one that might look cool?
								//newQuad = Q_irand( Q_BR, Q_BL );
								newQuad = PM_irand_timesync( Q_BR, Q_BL );
								//FIXME: sanity check, just in case?
							}//else player is switching up anyway, take the new attack dir
							bounceMove = transitionMove[saberMoveData[pm->ps->saberMove].startQuad][newQuad];
						}
						else
						*/
						//[/SaberSys]
						{//return to ready
							if ( saberMoveData[pm->ps->saberMove].startQuad == Q_T )
							{
								bounceMove = LS_R_BL2TR;
							}
							else if ( saberMoveData[pm->ps->saberMove].startQuad < Q_T )
							{
								bounceMove = LS_R_TL2BR+saberMoveData[pm->ps->saberMove].startQuad-Q_BR;
							}
							else// if ( saberMoveData[pm->ps->saberMove].startQuad > Q_T )
							{
								bounceMove = LS_R_BR2TL+saberMoveData[pm->ps->saberMove].startQuad-Q_TL;
							}

							//[SaberSys]
							//override current animation since returns don't normally override
							pm->ps->torsoTimer = 0;
							//[/SaberSys]
						}
					}
					else
					{//start the bounce
						bounceMove = PM_SaberBounceForAttack( (saberMoveName_t)pm->ps->saberMove );
					}

//[SaberLockSys]
/*
#ifdef QAGAME
					G_Printf("%i: %i: Started Bounce %s from %s:%i:%s\n", level.time, pm->ps->clientNum, 
						GetStringForID( SaberMoveTable, bounceMove ),
						GetStringForID( animTable, pm->ps->torsoAnim ),
						pm->ps->saberMove,
						GetStringForID( SaberMoveTable, pm->ps->saberMove ));
#endif
*/
//[/SaberLockSys]

					PM_SetSaberMove( bounceMove );

					pm->ps->weaponTime = pm->ps->torsoTimer;//+saberMoveData[bounceMove].blendTime+SABER_BLOCK_DUR;

//[SaberLockSys]
/*
#ifdef QAGAME
					G_Printf("%i: %i: Post Bounce Set: %s:%s\n", level.time, pm->ps->clientNum,
						GetStringForID( SaberMoveTable, pm->ps->saberMove ),
						GetStringForID( animTable, pm->ps->torsoAnim ));
#endif
*/
//[/SaberLockSys]
				}
				break;
			//[SaberSys]
			//Added previous SaberMove check to prevent Manual Blocking from doing 
			//a lot of restart animation calls for when the animation is already set.
			case BLOCKED_UPPER_RIGHT:
				if( pm->ps->saberMove != LS_PARRY_UR )
				{
					PM_SetSaberMove( LS_PARRY_UR );
				}
				break;
			case BLOCKED_UPPER_RIGHT_PROJ:
				PM_SetSaberMove( LS_REFLECT_UR );
				break;
			case BLOCKED_UPPER_LEFT:
				if( pm->ps->saberMove != LS_PARRY_UL )
				{
					PM_SetSaberMove( LS_PARRY_UL );
				}
				break;
			case BLOCKED_UPPER_LEFT_PROJ:
				PM_SetSaberMove( LS_REFLECT_UL );
				break;
			case BLOCKED_LOWER_RIGHT:
				if( pm->ps->saberMove != LS_PARRY_LR )
				{
					PM_SetSaberMove( LS_PARRY_LR );
				}
				break;
			case BLOCKED_LOWER_RIGHT_PROJ:
				PM_SetSaberMove( LS_REFLECT_LR );
				break;
			case BLOCKED_LOWER_LEFT:
				if( pm->ps->saberMove != LS_PARRY_LL )
				{				
					PM_SetSaberMove( LS_PARRY_LL );
				}
				break;
			case BLOCKED_LOWER_LEFT_PROJ:
				PM_SetSaberMove( LS_REFLECT_LL);
				break;
			case BLOCKED_TOP:
				if( pm->ps->saberMove != LS_PARRY_UP )
				{
					PM_SetSaberMove( LS_PARRY_UP );
				}
			//[/SaberSys]
				break;
			case BLOCKED_TOP_PROJ:
				PM_SetSaberMove( LS_REFLECT_UP );
				break;
			default:
				pm->ps->saberBlocked = BLOCKED_NONE;
				break;
		}
		if ( pm->ps->saberBlocked >= BLOCKED_UPPER_RIGHT 
			&& pm->ps->saberBlocked < BLOCKED_UPPER_RIGHT_PROJ)
		{//hold the parry for a bit
			if ( pm->ps->torsoTimer < pm->ps->weaponTime )
			{
				pm->ps->torsoTimer = pm->ps->weaponTime;
			}
		}

		//what the? I don't know why I was doing this.
		/*
		if (pm->ps->saberBlocked != BLOCKED_ATK_BOUNCE && pm->ps->saberBlocked != BLOCKED_PARRY_BROKEN && pm->ps->weaponTime < 1)
		{
			pm->ps->torsoTimer = SABER_BLOCK_DUR;
			pm->ps->weaponTime = pm->ps->torsoTimer;
		}
		*/

		//clear block
		pm->ps->saberBlocked = 0;

		// Charging is like a lead-up before attacking again.  This is an appropriate use, or we can create a new weaponstate for blocking
		pm->ps->weaponstate = WEAPON_READY;

		// Done with block, so stop these active weapon branches.
		return;
	}

weapChecks:
	
	//[SaberThrowSys]
	//No longer need the saber with us to change weapons
	// check for weapon change
	if (pm->ps->weaponTime <= 0 && pm->ps->torsoTimer <= 0)
	{			
		if ( pm->ps->weapon != pm->cmd.weapon ) 
		{
			PM_BeginWeaponChange( pm->cmd.weapon );
		}
	}

	/* basejka code
	if (pm->ps->saberEntityNum)
	{ //only check if we have our saber with us
		// check for weapon change
		// can't change if weapon is firing, but can change again if lowering or raising
		//if ( pm->ps->weaponTime <= 0 || pm->ps->weaponstate != WEAPON_FIRING ) {
		if (pm->ps->weaponTime <= 0 && pm->ps->torsoTimer <= 0)
		{
			if ( pm->ps->weapon != pm->cmd.weapon ) {
				PM_BeginWeaponChange( pm->cmd.weapon );
			}
		}
	}
	*/
	//[/SaberThrowSys]


	//[SaberSys]
	//Removed Katas.
	/*
	if((pm->cmd.buttons & BUTTON_ALT_ATTACK) && (pm->cmd.buttons & BUTTON_ATTACK))
	{//counter attack code
		if(PM_SaberInStart(pm->ps->saberMove))
		{
			pm->ps->weaponTime = 0;
		}

		if(pm->ps->weaponTime <= 0)
		{
			PM_DoPunch();
			return;
		}
	}
	*/

	/*
	if ( PM_CanDoKata() )
	{
		saberMoveName_t overrideMove = LS_INVALID;
		saberInfo_t *saber1 = BG_MySaber( pm->ps->clientNum, 0 );
		saberInfo_t *saber2 = BG_MySaber( pm->ps->clientNum, 1 );
		//see if we have an overridden (or cancelled) kata move
		if ( saber1 && saber1->kataMove != LS_INVALID )
		{
			if ( saber1->kataMove != LS_NONE )
			{
				overrideMove = (saberMoveName_t)saber1->kataMove;
			}
		}
		if ( overrideMove == LS_INVALID )
		{//not overridden by first saber, check second
			if ( saber2
				&& saber2->kataMove != LS_INVALID )
			{
				if ( saber2->kataMove != LS_NONE )
				{
					overrideMove = (saberMoveName_t)saber2->kataMove;
				}
			}
		}
		//no overrides, cancelled?
		if ( overrideMove == LS_INVALID )
		{
			if ( saber2
				&& saber2->kataMove == LS_NONE )
			{
				overrideMove = LS_NONE;
			}
			else if ( saber2
				&& saber2->kataMove == LS_NONE )
			{
				overrideMove = LS_NONE;
			}
		}
		if ( overrideMove == LS_INVALID )
		{//not overridden
			//FIXME: make sure to turn on saber(s)!
			switch ( pm->ps->fd.saberAnimLevel )
			{
			case SS_FAST:
			case SS_TAVION:
				PM_SetSaberMove( LS_A1_SPECIAL );
				break;
			case SS_MEDIUM:
				PM_SetSaberMove( LS_A2_SPECIAL );
				break;
			case SS_STRONG:
			case SS_DESANN:
				PM_SetSaberMove( LS_A3_SPECIAL );
				break;
			case SS_DUAL:
				PM_SetSaberMove( LS_DUAL_SPIN_PROTECT );//PM_CheckDualSpinProtect();
				break;
			case SS_STAFF:
				PM_SetSaberMove( LS_STAFF_SOULCAL );
				break;
			}
			pm->ps->weaponstate = WEAPON_FIRING;
			//G_DrainPowerForSpecialMove( pm->gent, FP_SABER_OFFENSE, SABER_ALT_ATTACK_POWER );//FP_SPEED, SINGLE_SPECIAL_POWER );
			BG_ForcePowerDrain(pm->ps, FP_GRIP, SABER_ALT_ATTACK_POWER);
		}
		else if ( overrideMove != LS_NONE )
		{
			PM_SetSaberMove( overrideMove );
			pm->ps->weaponstate = WEAPON_FIRING;
			BG_ForcePowerDrain(pm->ps, FP_GRIP, SABER_ALT_ATTACK_POWER);
		}
		if ( overrideMove != LS_NONE )
		{//not cancelled
			return;
		}
	}
	*/
	//[/SaberSys]

	if ( pm->ps->weaponTime > 0 ) 
	{
		return;
	}

	// *********************************************************
	// WEAPON_DROPPING
	// *********************************************************

	// change weapon if time
	if ( pm->ps->weaponstate == WEAPON_DROPPING ) {
		PM_FinishWeaponChange();
		return;
	}

	// *********************************************************
	// WEAPON_RAISING
	// *********************************************************

	if ( pm->ps->weaponstate == WEAPON_RAISING ) 
	{//Just selected the weapon
		pm->ps->weaponstate = WEAPON_IDLE;
		if((pm->ps->legsAnim) == BOTH_WALK1 )
		{
			PM_SetAnim(SETANIM_TORSO,BOTH_WALK1,SETANIM_FLAG_NORMAL, 100);
		}
		else if((pm->ps->legsAnim) == BOTH_RUN1 )
		{
			PM_SetAnim(SETANIM_TORSO,BOTH_RUN1,SETANIM_FLAG_NORMAL, 100);
		}
		else if((pm->ps->legsAnim) == BOTH_RUN2 )
		{
			PM_SetAnim(SETANIM_TORSO,BOTH_RUN2,SETANIM_FLAG_NORMAL, 100);
		}
		else if( pm->ps->legsAnim == BOTH_RUN_STAFF)
		{
			PM_SetAnim(SETANIM_TORSO,BOTH_RUN_STAFF,SETANIM_FLAG_NORMAL, 100);
		}
		else if( pm->ps->legsAnim == BOTH_RUN_DUAL)
		{
			PM_SetAnim(SETANIM_TORSO,BOTH_RUN_DUAL,SETANIM_FLAG_NORMAL, 100);
		}
		else if((pm->ps->legsAnim) == BOTH_WALK1 )
		{
			PM_SetAnim(SETANIM_TORSO,BOTH_WALK1,SETANIM_FLAG_NORMAL, 100);
		}
		else if( pm->ps->legsAnim == BOTH_WALK2)
		{
			PM_SetAnim(SETANIM_TORSO,BOTH_WALK2,SETANIM_FLAG_NORMAL, 100);
		}
		else if( pm->ps->legsAnim == BOTH_WALK_STAFF)
		{
			PM_SetAnim(SETANIM_TORSO,BOTH_WALK_STAFF,SETANIM_FLAG_NORMAL, 100);
		}
		else if( pm->ps->legsAnim == BOTH_WALK_DUAL)
		{
			PM_SetAnim(SETANIM_TORSO,BOTH_WALK_DUAL,SETANIM_FLAG_NORMAL, 100);
		}
		else
		{
			PM_SetAnim(SETANIM_TORSO,PM_GetSaberStance(),SETANIM_FLAG_NORMAL, 100);
		}

		if (pm->ps->weaponstate == WEAPON_RAISING)
		{
			return;
		}

	}

	//[MELEE]
	//moved the kick code to here so the player can kick while their saber is off.
	if((pm->cmd.buttons & BUTTON_ALT_ATTACK) && !(pm->cmd.buttons & BUTTON_ATTACK) && PM_DoKick())
	{
		return;
	}
	//[/MELEE]
	//[SaberThrowSys]
	else if(pm->ps->saberInFlight && pm->ps->forceHandExtend != HANDEXTEND_SABERPULL 
		&& pm->ps->fd.saberAnimLevel != SS_DUAL && (pm->cmd.buttons & BUTTON_ATTACK))
	{//don't have our saber so we can punch instead.
		PM_DoPunch();
		return;
	}
	//[/SaberThrowSys]

	if (checkOnlyWeap)
	{
		return;
	}

	// *********************************************************
	// Check for WEAPON ATTACK
	// *********************************************************
	//[MELEE]
	/* bumped up so that players can still kick while their saber is off.
	if (pm->ps->fd.saberAnimLevel == SS_STAFF &&
		(pm->cmd.buttons & BUTTON_ALT_ATTACK))
	{ //ok, try a kick I guess.
		int kickMove = -1;

		if ( !BG_KickingAnim(pm->ps->torsoAnim) &&
			!BG_KickingAnim(pm->ps->legsAnim) &&
			!BG_InRoll(pm->ps, pm->ps->legsAnim) &&
//			!BG_KickMove( pm->ps->saberMove )//not already in a kick
			pm->ps->saberMove == LS_READY
			&& !(pm->ps->pm_flags&PMF_DUCKED)//not ducked
			&& (pm->cmd.upmove >= 0 ) //not trying to duck
			)//&& pm->ps->groundEntityNum != ENTITYNUM_NONE)
		{//player kicks
			kickMove = PM_KickMoveForConditions();
		}

		if (kickMove != -1)
		{
			if ( pm->ps->groundEntityNum == ENTITYNUM_NONE )
			{//if in air, convert kick to an in-air kick
				float gDist = PM_GroundDistance();
				//let's only allow air kicks if a certain distance from the ground
				//it's silly to be able to do them right as you land.
				//also looks wrong to transition from a non-complete flip anim...
				if ((!BG_FlippingAnim( pm->ps->legsAnim ) || pm->ps->legsTimer <= 0) &&
					gDist > 64.0f && //strict minimum
					gDist > (-pm->ps->velocity[2])-64.0f //make sure we are high to ground relative to downward velocity as well
					)
				{
					switch ( kickMove )
					{
					case LS_KICK_F:
						kickMove = LS_KICK_F_AIR;
						break;
					case LS_KICK_B:
						kickMove = LS_KICK_B_AIR;
						break;
					case LS_KICK_R:
						kickMove = LS_KICK_R_AIR;
						break;
					case LS_KICK_L:
						kickMove = LS_KICK_L_AIR;
						break;
					default: //oh well, can't do any other kick move while in-air
						kickMove = -1;
						break;
					}
				}
				else
				{//leave it as a normal kick unless we're too high up
					if ( gDist > 128.0f || pm->ps->velocity[2] >= 0 )
					{ //off ground, but too close to ground
						kickMove = -1;
					}
				}
			}

			if (kickMove != -1)
			{
				PM_SetSaberMove( kickMove );
				return;
			}
		}
	}
	*/
	//[/MELEE]


	//[SaberSys]
	//this is never a valid regular saber attack button
	//pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;
	//[/SaberSys]

	if(!delayed_fire)
	{
		// Start with the current move, and cross index it with the current control states.
		if ( pm->ps->saberMove > LS_NONE && pm->ps->saberMove < LS_MOVE_MAX )
		{
			curmove = pm->ps->saberMove;
		}
		else
		{
			curmove = LS_READY;
		}

		if ( curmove == LS_A_JUMP_T__B_ || pm->ps->torsoAnim == BOTH_FORCELEAP2_T__B_ )
		{//must transition back to ready from this anim
			newmove = LS_R_T2B;
		}
		// check for fire
		//[SaberSys]
		//Cleaning up the faking code.  This section dictates want happens when you
		//quit holding down attack.
		else if ( !(pm->cmd.buttons & (BUTTON_ATTACK)) )
		//else if ( !(pm->cmd.buttons & (BUTTON_ATTACK|BUTTON_ALT_ATTACK)) )
		//[/SaberSys]
		{//not attacking
			//[SaberSys]
			if ( pm->ps->weaponstate != WEAPON_READY )
			/* redundant code?  We already know that the weaponTime is 0.
			pm->ps->weaponTime = 0;
			
			if ( pm->ps->weaponTime > 0 )
			{//Still firing
				pm->ps->weaponstate = WEAPON_FIRING;
			}
			else if ( pm->ps->weaponstate != WEAPON_READY )
			*/
			//[/SaberSys]
			{
				pm->ps->weaponstate = WEAPON_IDLE;
			}

			//[SaberSys]
			//return to ready if not pressing button
			//Check for finishing an anim if necc.
			if ( curmove >= LS_S_TL2BR && curmove <= LS_S_T2B )
			{//started a swing, must continue from here
#ifdef QAGAME
				if(pm_entSelf->s.NPC_class != CLASS_NONE)
				{//NPCs never do attack fakes, just follow thru with attack.
					newmove = LS_A_TL2BR + (curmove-LS_S_TL2BR);
				}
				else
#endif
				{//perform attack fake
					newmove = PM_ReturnforQuad(saberMoveData[curmove].endQuad);
					//attack fakes now cost FP
					BG_AddFatigue(pm->ps, 1);
				}
				//newmove = LS_A_TL2BR + (curmove-LS_S_TL2BR);
			}
			else if ( curmove >= LS_A_TL2BR && curmove <= LS_A_T2B )
			{//finished an attack, must continue from here
				newmove = LS_R_TL2BR + (curmove-LS_A_TL2BR);
			}
			else if ( PM_SaberInTransition( curmove ) )
			{//in a transition, must play sequential attack
#ifdef QAGAME
				if(pm_entSelf->s.NPC_class != CLASS_NONE)
				{//NPCs never stop attacking mid-attack, just follow thru with attack.
					newmove = saberMoveData[curmove].chain_attack;
				}
				else
#endif
				{//exit out of transition without attacking
					newmove = PM_ReturnforQuad(saberMoveData[curmove].endQuad);
				}
				//newmove = saberMoveData[curmove].chain_attack;
			//[/SaberSys]
			}
			else if ( PM_SaberInBounce( curmove ) )
			{//in a bounce
				newmove = saberMoveData[curmove].chain_idle;//oops, not attacking, so don't chain
			}
			else 
			{//FIXME: what about returning from a parry?
				//PM_SetSaberMove( LS_READY );
				//if ( pm->ps->saberBlockingTime > pm->cmd.serverTime )
				{
					PM_SetSaberMove( LS_READY );
				}
				return;
			}
		}
		//[SaberSys]
		else if( (pm->cmd.buttons & BUTTON_ALT_ATTACK) && (pm->cmd.buttons & BUTTON_ATTACK))
		{//do some fancy faking stuff.
			if ( pm->ps->weaponstate != WEAPON_READY )
			{
				pm->ps->weaponstate = WEAPON_IDLE;
			}

			//Check for finishing an anim if necc.
			if ( curmove >= LS_S_TL2BR && curmove <= LS_S_T2B )
			{//allow the player to fake into another transition
				newmove = PM_DoFake(curmove);
				if(newmove == LS_NONE)
				{//no movement, just do the attack
					newmove = LS_A_TL2BR + (curmove-LS_S_TL2BR);
				}
			}
			else if ( curmove >= LS_A_TL2BR && curmove <= LS_A_T2B )
			{//finished attack, let attack code handle the next step.
			}
			else if ( PM_SaberInTransition( curmove ) )
			{//in a transition, must play sequential attack
				newmove = PM_DoFake(curmove);
				if(newmove == LS_NONE)
				{//no movement, just let the normal attack code handle it
					newmove = saberMoveData[curmove].chain_attack;
				}
			}
			else if ( PM_SaberInBounce( curmove ) )
			{//in a bounce
			}
			else
			{//returning from a parry I think.
			}
		}
		//[/SaberSys]


		// ***************************************************
		// Pressing attack, so we must look up the proper attack move.

		if ( pm->ps->weaponTime > 0 )
		{	// Last attack is not yet complete.
			pm->ps->weaponstate = WEAPON_FIRING;
			return;
		}
		else
		{
			int	both = qfalse;
			if ( pm->ps->torsoAnim == BOTH_FORCELONGLEAP_ATTACK
				|| pm->ps->torsoAnim == BOTH_FORCELONGLEAP_LAND )
			{//can't attack in these anims
				return;
			}
			else if ( pm->ps->torsoAnim == BOTH_FORCELONGLEAP_START )
			{//only 1 attack you can do from this anim
				if ( pm->ps->torsoTimer >= 200 )
				{//hit it early enough to do the attack
					PM_SetSaberMove( LS_LEAP_ATTACK );
				}
				return;
			}
			//[SaberSys]
			/*
			if ( curmove >= LS_PARRY_UP && curmove <= LS_REFLECT_LL )
			{//from a parry or reflection, can go directly into an attack
				switch ( saberMoveData[curmove].endQuad )
				{
				case Q_T:
					newmove = LS_A_T2B;
					break;
				case Q_TR:
					newmove = LS_A_TR2BL;
					break;
				case Q_TL:
					newmove = LS_A_TL2BR;
					break;
				case Q_BR:
					newmove = LS_A_BR2TL;
					break;
				case Q_BL:
					newmove = LS_A_BL2TR;
					break;
				//shouldn't be a parry that ends at L, R or B
				}
			}
			*/
			//[/SaberSys]


			if ( newmove != LS_NONE )
			{//have a valid, final LS_ move picked, so skip findingt he transition move and just get the anim
				anim = saberMoveData[newmove].animToUse;
			}

			//FIXME: diagonal dirs use the figure-eight attacks from ready pose?
			if ( anim == -1 )
			{
				//FIXME: take FP_SABER_OFFENSE into account here somehow?
				if ( PM_SaberInTransition( curmove ) )
				{//in a transition, must play sequential attack
					newmove = saberMoveData[curmove].chain_attack;
				}
				else if ( curmove >= LS_S_TL2BR && curmove <= LS_S_T2B )
				{//started a swing, must continue from here
					//[SaberSys]
					//moved the DFA code to here so we can require the 
					if (curmove == LS_S_T2B &&//need to be winding up for a T2B attack
						(pm->ps->fd.saberAnimLevel == SS_STRONG ||
						pm->ps->fd.saberAnimLevel == SS_DESANN) &&
						!pm->cmd.rightmove &&
						pm->cmd.forwardmove > 0 &&
						pm->ps->velocity[2] > 100 &&
						VectorLengthSquared(pm->ps->velocity)>=40000 &&
						//PM_GroundDistance() < 32 &&
						!BG_InSpecialJump(pm->ps->legsAnim) &&
						!BG_SaberInSpecialAttack(pm->ps->torsoAnim)&&
						//[SaberSys]
						BG_EnoughForcePowerForMove( FATIGUE_JUMPATTACK ))
 						//BG_EnoughForcePowerForMove( SABER_ALT_ATTACK_POWER_FB ))
						//[SaberSys]
					{ //attempt to do a DFA
						newmove = PM_SaberJumpAttackMove();
						if ( newmove != LS_A_T2B
							&& newmove != LS_NONE )
						{
							//[SaberSys]
							BG_ForcePowerDrain(pm->ps, FP_GRIP, FATIGUE_JUMPATTACK);

							//BG_ForcePowerDrain(pm->ps, FP_GRIP, SABER_ALT_ATTACK_POWER_FB);
							//[/SaberSys]
						}
					}
					else
					{
						newmove = LS_A_TL2BR + (curmove-LS_S_TL2BR);
					}
					//newmove = LS_A_TL2BR + (curmove-LS_S_TL2BR);
					//[/SaberSys]
				}
				//[SaberSys]
				else if ( PM_SaberInBounce( curmove ) && pm->ps->userInt3 & (1 << FLAG_PARRIED) )
				{//can't combo if we were parried.
					newmove = LS_READY;
				}
				/*
				else if ( PM_SaberInParry( curmove ) )
				{//can't attack straight from a block animation.
					newmove = LS_READY;
				}
				else if ( PM_SaberInBrokenParry( curmove ) )
				{//broken parries must always return to ready
					newmove = LS_READY;
				}
				*/
				//[/SaberSys]
				else//if ( pm->cmd.buttons&BUTTON_ATTACK && !(pm->ps->pm_flags&PMF_ATTACK_HELD) )//only do this if just pressed attack button?
				{//get attack move from movement command
					/*
					if ( PM_SaberKataDone() )
					{//we came from a bounce and cannot chain to another attack because our kata is done
						newmove = saberMoveData[curmove].chain_idle;
					}
					else */
					//racc - determine where we want to go next.
					newmove = PM_SaberAttackForMovement( curmove );

					//[SaberLockSys]
					/* racc - debugger message I was using to debug the super break saber animations.
					#ifdef QAGAME
					G_Printf("%i: %i: starting new attack move %i:%s %s.\n", level.time, pm->ps->clientNum, curmove, GetStringForID(SaberMoveTable, curmove), GetStringForID(animTable, pm->ps->torsoAnim));
					#endif
					*/
					//[/SaberLockSys]

					//[SaberSys]
					if ( (PM_SaberInBounce( curmove ) || PM_SaberInParry( curmove ))
						&& newmove >= LS_A_TL2BR && newmove <= LS_A_T2B )
					{//prevent similar attack directions to prevent lightning-like bounce attacks.
						if(saberMoveData[newmove].startQuad == saberMoveData[curmove].endQuad)
						{//can't attack in the same direction
							newmove = LS_READY;
						}
						/* old method blocks adjacent attack quads as well.  I don't think we need this anymore.
						if(saberMoveData[newmove].startQuad >= saberMoveData[curmove].endQuad
							&& saberMoveData[newmove].startQuad < saberMoveData[curmove].endQuad + 2)
						{
							newmove = LS_READY;
						}
						else if(saberMoveData[newmove].startQuad <= saberMoveData[curmove].endQuad
							&& saberMoveData[newmove].startQuad > saberMoveData[curmove].endQuad - 2)
						{
							newmove = LS_READY;
						}
						*/
					}

					//starting a new attack, as such, remove the attack fake flag.
					pm->ps->userInt3 &= ~( 1 << FLAG_ATTACKFAKE );

					/* basejka code
					if ( (PM_SaberInBounce( curmove )||PM_SaberInBrokenParry( curmove ))
						&& saberMoveData[newmove].startQuad == saberMoveData[curmove].endQuad )
					{//this attack would be a repeat of the last (which was blocked), so don't actually use it, use the default chain attack for this bounce
						newmove = saberMoveData[curmove].chain_attack;
					}

					if ( PM_SaberKataDone( curmove, newmove ) )
					{//cannot chain this time
						newmove = saberMoveData[curmove].chain_idle;
					}
					*/
					//[/SaberSys]
				}
				/*
				if ( newmove == LS_NONE )
				{//FIXME: should we allow this?  Are there some anims that you should never be able to chain into an attack?
					//only curmove that might get in here is LS_NONE, LS_DRAW, LS_PUTAWAY and the LS_R_ returns... all of which are in Q_R
					newmove = PM_AttackMoveForQuad( saberMoveData[curmove].endQuad );
				}
				*/
				if ( newmove != LS_NONE )
				{//racc - find the transition move for this attack/move
					//Now get the proper transition move
					newmove = PM_SaberAnimTransitionAnim( curmove, newmove );
					anim = saberMoveData[newmove].animToUse;
				}
			}

			if (anim == -1)
			{//not side-stepping, pick neutral anim
				// Add randomness for prototype?
				newmove = saberMoveData[curmove].chain_attack;

				anim= saberMoveData[newmove].animToUse;

				if ( !pm->cmd.forwardmove && !pm->cmd.rightmove && pm->cmd.upmove >= 0 && pm->ps->groundEntityNum != ENTITYNUM_NONE )
				{//not moving at all, so set the anim on entire body
					both = qtrue;
				}
			
			}

			if ( anim == -1)
			{
				switch ( pm->ps->legsAnim )
				{
				case BOTH_WALK1:
				case BOTH_WALK2:
				case BOTH_WALK_STAFF:
				case BOTH_WALK_DUAL:
				case BOTH_WALKBACK1:
				case BOTH_WALKBACK2:
				case BOTH_WALKBACK_STAFF:
				case BOTH_WALKBACK_DUAL:
				case BOTH_RUN1:
				case BOTH_RUN2:
				case BOTH_RUN_STAFF:
				case BOTH_RUN_DUAL:
				case BOTH_RUNBACK1:
				case BOTH_RUNBACK2:
				case BOTH_RUNBACK_STAFF:
					anim = pm->ps->legsAnim;
					break;
				default:
					anim = PM_GetSaberStance();
					break;
				}

//				if (PM_RunningAnim(anim) && !pm->cmd.forwardmove && !pm->cmd.rightmove)
//				{ //semi-hacky (if not moving on x-y and still playing the running anim, force the player out of it)
//					anim = PM_GetSaberStance();
//				}
				newmove = LS_READY;
			}

			PM_SetSaberMove( newmove );

			if ( both && pm->ps->torsoAnim == anim )
			{
				PM_SetAnim(SETANIM_LEGS,anim,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 100);
			}

			//don't fire again until anim is done
			pm->ps->weaponTime = pm->ps->torsoTimer;
		}
	}

	// *********************************************************
	// WEAPON_FIRING
	// *********************************************************

	pm->ps->weaponstate = WEAPON_FIRING;

	amount = weaponData[pm->ps->weapon].energyPerShot;

	addTime = pm->ps->weaponTime;

	pm->ps->saberAttackSequence = pm->ps->torsoAnim;
	if ( !addTime )
	{
		addTime = weaponData[pm->ps->weapon].fireTime;
	}
	pm->ps->weaponTime = addTime;
}


//[FatigueSys]
//Add Fatigue to a player
void BG_AddFatigue( playerState_t * ps, int Fatigue)
{
	//For now, all saber attacks cost one FP.
	if(ps->fd.forcePower > Fatigue)
	{
		ps->fd.forcePower -= Fatigue;
	}
	else
	{//don't have enough so just completely drain FP then.
		ps->fd.forcePower = 0;
	}
	
	UpdateFatigueFlags(ps);
}


int Fatigue_SaberAttack( playerState_t * ps )
{//returns the FP cost for a saber attack by this player.
	/* racc - all saber styles use 1 fp for attacks.
	if( ps->fd.saberAnimLevel == SS_DUAL )
	{//duals cost more to attack
		return 2*FATIGUE_SABERATTACK;
	}

	if( ps->fd.saberAnimLevel == SS_STAFF )
	{//doubles cost more to attack
		return 2*FATIGUE_SABERATTACK;
	}
	*/
			if (ps->fd.forcePowersActive & (1 << FP_RAGE))
			{
				return FATIGUE_SABERATTACK/2;	
			}
			else
			{
				return FATIGUE_SABERATTACK;
			}
	
}


extern qboolean BG_SaberInAttackPure( int move );
//Add Fatigue for all the sabermoves.
void BG_SaberFatigue( playerState_t * ps, int newMove, int anim )
{
	if ( ps->saberMove != newMove )
	{//wasn't playing that attack before
		if ( BG_KickMove( newMove ) )
		{//melee move
			BG_AddFatigue( ps, FATIGUE_MELEE );
		}
		else if ( BG_SaberInAttackPure( newMove ) )
		{//simple saber attack
			BG_AddFatigue( ps, Fatigue_SaberAttack(ps) );
		}
		else if( PM_SaberInTransition( newMove ) && pm->ps->userInt3 & (1 << FLAG_ATTACKFAKE) )
		{//attack fakes cost FP as well
			if( ps->fd.saberAnimLevel == SS_DUAL ) 
			{//dual sabers don't have transition/FP costs.
			}
			else
			{//single sabers
				BG_AddFatigue( ps, FATIGUE_SABERTRANS);
			}
		}
	}

	return;
}
//[/FatigueSys]

//[SaberSys]
void PM_SaberFakeFlagUpdate(playerState_t *ps, int newMove, int currentMove);
//[/SaberSys]

void PM_SetSaberMove(short newMove)
{
	unsigned int setflags = saberMoveData[newMove].animSetFlags;
	int	anim = saberMoveData[newMove].animToUse;
	int parts = SETANIM_TORSO;

	//[SaberSys]
	//removed the attackchain requirements for saber moves.
	/*
	if ( newMove == LS_READY || newMove == LS_A_FLIP_STAB || newMove == LS_A_FLIP_SLASH )
	{//finished with a kata (or in a special move) reset attack counter
		pm->ps->saberAttackChainCount = 0;
	}
	else if ( BG_SaberInAttack( newMove ) )
	{//continuing with a kata, increment attack counter
		pm->ps->saberAttackChainCount++;
	}

	if (pm->ps->saberAttackChainCount > 16)
	{ //for the sake of being able to send the value over the net within a reasonable bit count
		pm->ps->saberAttackChainCount = 16;
	}
	*/
	//[SaberSys]

	if ( newMove == LS_DRAW )
	{
		saberInfo_t *saber1 = BG_MySaber( pm->ps->clientNum, 0 );
		saberInfo_t *saber2 = BG_MySaber( pm->ps->clientNum, 1 );
		if ( saber1 
			&& saber1->drawAnim != -1 )
		{
			anim = saber1->drawAnim;
		}
		else if ( saber2 
			&& saber2->drawAnim != -1 )
		{
			anim = saber2->drawAnim;
		}
		else if ( pm->ps->fd.saberAnimLevel == SS_STAFF )
		{
			anim = BOTH_S1_S7;
		}
		else if ( pm->ps->fd.saberAnimLevel == SS_DUAL )
		{
			anim = BOTH_S1_S6;
		}
	}
	else if ( newMove == LS_PUTAWAY )
	{
		saberInfo_t *saber1 = BG_MySaber( pm->ps->clientNum, 0 );
		saberInfo_t *saber2 = BG_MySaber( pm->ps->clientNum, 1 );
		if ( saber1 
			&& saber1->putawayAnim != -1 )
		{
			anim = saber1->putawayAnim;
		}
		else if ( saber2 
			&& saber2->putawayAnim != -1 )
		{
			anim = saber2->putawayAnim;
		}
		else if ( pm->ps->fd.saberAnimLevel == SS_STAFF )
		{
			anim = BOTH_S7_S1;
		}
		else if ( pm->ps->fd.saberAnimLevel == SS_DUAL )
		{
			anim = BOTH_S6_S1;
		}
	}
	//[SaberSys]
	//different styles use different animations for the DFA move.
	else if ( newMove == LS_A_JUMP_T__B_ && pm->ps->fd.saberAnimLevel == SS_DESANN )
	{
		anim = BOTH_FJSS_TR_BL;
	}	
	//[/SaberSys]
	else if ( pm->ps->fd.saberAnimLevel == SS_STAFF && newMove >= LS_S_TL2BR && newMove < LS_REFLECT_LL )
	{//staff has an entirely new set of anims, besides special attacks
		//FIXME: include ready and draw/putaway?
		//FIXME: get hand-made bounces and deflections?
		if ( newMove >= LS_V1_BR && newMove <= LS_REFLECT_LL )
		{//there aren't 1-7, just 1, 6 and 7, so just set it
			//anim = BOTH_P7_S7_T_ + (anim-BOTH_P1_S1_T_);//shift it up to the proper set
			saberInfo_t *saber1 = BG_MySaber( pm->ps->clientNum, 0 );//new block anims code for Niman
			if(saber1->numBlades == 1)
				anim = saberMoveData[newMove].animToUse;
			else
				anim = BOTH_P7_S7_T_ + (anim-BOTH_P1_S1_T_);//shift it up to the proper set
		}
		else
		{//add the appropriate animLevel
			anim += (pm->ps->fd.saberAnimLevel-FORCE_LEVEL_1) * SABER_ANIM_GROUP_SIZE;
		}
	}
	else if ( pm->ps->fd.saberAnimLevel == SS_DUAL && newMove >= LS_S_TL2BR && newMove < LS_REFLECT_LL )
	{ //akimbo has an entirely new set of anims, besides special attacks
		//FIXME: include ready and draw/putaway?
		//FIXME: get hand-made bounces and deflections?
		if ( newMove >= LS_V1_BR && newMove <= LS_REFLECT_LL )
		{//there aren't 1-7, just 1, 6 and 7, so just set it
			//anim = BOTH_P6_S6_T_ + (anim-BOTH_P1_S1_T_);//shift it up to the proper set//old code
			saberInfo_t *saber1 = BG_MySaber( pm->ps->clientNum, 0 );//new ataru block anims
			saberInfo_t *saber2 = BG_MySaber( pm->ps->clientNum, 1 );
			if(saber1->numBlades == 1 && (!saber2 || saber2->numBlades == 0))
				anim = saberMoveData[newMove].animToUse;
			else
			anim = BOTH_P6_S6_T_ + (anim-BOTH_P1_S1_T_);//shift it up to the proper set

		}
		else
		{//add the appropriate animLevel
			anim += (pm->ps->fd.saberAnimLevel-FORCE_LEVEL_1) * SABER_ANIM_GROUP_SIZE;
		}
	}
	/*
	else if ( newMove == LS_DRAW && pm->ps->SaberStaff() )
	{//hold saber out front as we turn it on
		//FIXME: need a real "draw" anim for this (and put-away)
		anim = BOTH_SABERSTAFF_STANCE;
	}
	*/
	else if ( pm->ps->fd.saberAnimLevel > FORCE_LEVEL_1 &&
		 !BG_SaberInIdle( newMove ) && !PM_SaberInParry( newMove ) && !PM_SaberInKnockaway( newMove ) && !PM_SaberInBrokenParry( newMove ) && !PM_SaberInReflect( newMove ) && !BG_SaberInSpecial(newMove))
	{//readies, parries and reflections have only 1 level 
		anim += (pm->ps->fd.saberAnimLevel-FORCE_LEVEL_1) * SABER_ANIM_GROUP_SIZE;
	}

	// If the move does the same animation as the last one, we need to force a restart...
	if ( saberMoveData[pm->ps->saberMove].animToUse == anim && newMove > LS_PUTAWAY)
	{
		setflags |= SETANIM_FLAG_RESTART;
	}

	//saber torso anims should always be highest priority (4/12/02 - for special anims only)
	if (!pm->ps->m_iVehicleNum)
	{ //if not riding a vehicle
		if (BG_SaberInSpecial(newMove))
		{
			setflags |= SETANIM_FLAG_OVERRIDE;
		}
		/*
		if ( newMove == LS_A_LUNGE 
			|| newMove == LS_A_JUMP_T__B_ 
			|| newMove == LS_A_BACKSTAB
			|| newMove == LS_A_BACK
			|| newMove == LS_A_BACK_CR
			|| newMove == LS_A_FLIP_STAB
			|| newMove == LS_A_FLIP_SLASH
			|| newMove == LS_JUMPATTACK_DUAL
			|| newMove == LS_A_BACKFLIP_ATK)
		{
			setflags |= SETANIM_FLAG_OVERRIDE;
		}
		*/
	}
	if ( BG_InSaberStandAnim(anim) || anim == BOTH_STAND1 )
	{
		//[SaberLockSys]
		//The original method here was causing a problem where along animation (like a superbreak) would end up repeating on the
		//torso a sequence of animations returned the torso to the stand/stance faster than the original animation can finish
		//on the legs.
		//In addition, I've compressed all the statements into one if statement.
		if(PM_JumpingAnim(pm->ps->legsAnim) 
			|| (pm->ps->legsAnim >= BOTH_RUN1 && pm->ps->legsAnim <= BOTH_RUNBACK_DUAL)
			|| BG_FlippingAnim( pm->ps->legsAnim ))
		{//play the leg animation on the body.
			anim = pm->ps->legsAnim;
		}
		else if (//if legs are in stand animation
			(pm->ps->legsAnim >= BOTH_STAND1 && pm->ps->legsAnim <= BOTH_STAND4TOATTACK2) ||
			(pm->ps->legsAnim >= TORSO_DROPWEAP1 && pm->ps->legsAnim <= TORSO_WEAPONIDLE10)
			//or the player is ducked
			|| pm->ps->pm_flags & PMF_DUCKED
			//or the player is walking backwards
			|| pm->ps->legsAnim == BOTH_WALKBACK1 || pm->ps->legsAnim == BOTH_WALKBACK2 
			|| (pm->ps->legsAnim >= BOTH_WALK1 && pm->ps->legsAnim <= BOTH_WALKBACK_DUAL)
			|| BG_InSlopeAnim(pm->ps->legsAnim)
			|| BG_InSaberStandAnim(pm->ps->legsAnim) )
		{//override the generic stance/stand animation with the approprate one for the current situation
			anim = PM_GetSaberStance();
			parts = SETANIM_TORSO;
		}

		/* basejka method
		anim = (pm->ps->legsAnim);
		
		if ((anim >= BOTH_STAND1 && anim <= BOTH_STAND4TOATTACK2) ||
			(anim >= TORSO_DROPWEAP1 && anim <= TORSO_WEAPONIDLE10))
		{ //If standing then use the special saber stand anim
			anim = PM_GetSaberStance();
		}

		if (pm->ps->pm_flags & PMF_DUCKED)
		{ //Playing torso walk anims while crouched makes you look like a monkey
			anim = PM_GetSaberStance();
		}

		//[SaberSys]
		//use saber stance instead of BOTH_WALK2 (walking forward with single saber lit) on the torso.
		if (anim == BOTH_WALKBACK1 || anim == BOTH_WALKBACK2 || anim == BOTH_WALK1 || anim == BOTH_WALK2 )
		//if (anim == BOTH_WALKBACK1 || anim == BOTH_WALKBACK2 || anim == BOTH_WALK1)
		//[/SaberSys]
		{ //normal stance when walking backward so saber doesn't look like it's cutting through leg
			anim = PM_GetSaberStance();
		}

		if (BG_InSlopeAnim( anim ))
		{
			anim = PM_GetSaberStance();
		}

		parts = SETANIM_TORSO;
		*/
		//[/SaberLockSys]
	}

	if (!pm->ps->m_iVehicleNum)
	{ //if not riding a vehicle
		if (newMove == LS_JUMPATTACK_ARIAL_RIGHT ||
				newMove == LS_JUMPATTACK_ARIAL_LEFT)
		{ //force only on legs
			parts = SETANIM_LEGS;
		}
		else if ( newMove == LS_A_LUNGE 
				|| newMove == LS_A_JUMP_T__B_ 
				|| newMove == LS_A_BACKSTAB
				|| newMove == LS_A_BACK
				|| newMove == LS_A_BACK_CR
				|| newMove == LS_ROLL_STAB
				|| newMove == LS_A_FLIP_STAB
				|| newMove == LS_A_FLIP_SLASH
				|| newMove == LS_JUMPATTACK_DUAL
				|| newMove == LS_JUMPATTACK_ARIAL_LEFT
				|| newMove == LS_JUMPATTACK_ARIAL_RIGHT
				|| newMove == LS_JUMPATTACK_CART_LEFT
				|| newMove == LS_JUMPATTACK_CART_RIGHT
				|| newMove == LS_JUMPATTACK_STAFF_LEFT
				|| newMove == LS_JUMPATTACK_STAFF_RIGHT
				|| newMove == LS_A_BACKFLIP_ATK
				|| newMove == LS_STABDOWN
				|| newMove == LS_STABDOWN_STAFF
				|| newMove == LS_STABDOWN_DUAL
				|| newMove == LS_DUAL_SPIN_PROTECT
				|| newMove == LS_STAFF_SOULCAL
				|| newMove == LS_A1_SPECIAL
				|| newMove == LS_A2_SPECIAL
				|| newMove == LS_A3_SPECIAL
				|| newMove == LS_UPSIDE_DOWN_ATTACK
				|| newMove == LS_PULL_ATTACK_STAB
				|| newMove == LS_PULL_ATTACK_SWING
				//[SaberSys]
				//play full body on the broken parries
				|| PM_SaberInBrokenParry( newMove )
				//[/SaberSys]
				|| BG_KickMove( newMove ) )
		{
			parts = SETANIM_BOTH;
		}
		else if ( BG_SpinningSaberAnim( anim ) )
		{//spins must be played on entire body
			parts = SETANIM_BOTH;
		}
		//[SaberSys]
		//coming out of a spin, force full body setting
		else if ( BG_SpinningSaberAnim( pm->ps->legsAnim ) )
		{//spins must be played on entire body
			parts = SETANIM_BOTH;
			pm->ps->legsTimer = pm->ps->torsoTimer = 0;
		}
		//[/SaberSys]
		//[MoveSys]
		//override our leg animation if we're restricted from moving (like during a viewlock)
		else if ( (!pm->cmd.forwardmove&&!pm->cmd.rightmove&&!pm->cmd.upmove) || pm->ps->speed == 0)
		//else if ( (!pm->cmd.forwardmove&&!pm->cmd.rightmove&&!pm->cmd.upmove))
		//[/MoveSys]
		{//not trying to run, duck or jump
			if ( !BG_FlippingAnim( pm->ps->legsAnim ) && 
				!BG_InRoll( pm->ps, pm->ps->legsAnim ) && 
				!PM_InKnockDown( pm->ps ) && 
				!PM_JumpingAnim( pm->ps->legsAnim ) &&
				!BG_InSpecialJump( pm->ps->legsAnim ) &&
				anim != PM_GetSaberStance() &&
				pm->ps->groundEntityNum != ENTITYNUM_NONE &&
				!(pm->ps->pm_flags & PMF_DUCKED))
			{
				parts = SETANIM_BOTH;
			}
			else if ( !(pm->ps->pm_flags & PMF_DUCKED) 
				&& ( newMove == LS_SPINATTACK_DUAL || newMove == LS_SPINATTACK ) )
			{
				parts = SETANIM_BOTH;
			}
		}

		PM_SetAnim(parts, anim, setflags, saberMoveData[newMove].blendTime);
		if (parts != SETANIM_LEGS &&
			(pm->ps->legsAnim == BOTH_ARIAL_LEFT ||
			pm->ps->legsAnim == BOTH_ARIAL_RIGHT))
		{
			if (pm->ps->legsTimer > pm->ps->torsoTimer)
			{
				pm->ps->legsTimer = pm->ps->torsoTimer;
			}
		}

	}

	if ( (pm->ps->torsoAnim) == anim )
	{//successfully changed anims
	//special check for *starting* a saber swing
		//playing at attack

//[SaberLockSys]
/*
#ifdef QAGAME
		if(pm->ps->saberMove != newMove)
		{

			G_Printf("%i: %i: Set New SaberMove: %i:%s:%s torsoTimer: %i\n", level.time, pm->ps->clientNum,
				newMove,
				GetStringForID( SaberMoveTable, newMove ),
				GetStringForID( animTable, pm->ps->torsoAnim ),
				pm->ps->torsoTimer);
		}
#endif
*/
//[/SaberLockSys]

		//[FatigueSys]
		BG_SaberFatigue( pm->ps, newMove, anim );
		//[/FatgieSys]

		//[SaberSys]
		//update the attack fake flag
		PM_SaberFakeFlagUpdate(pm->ps, newMove, anim);
		
		if(!PM_SaberInBounce(newMove) && !PM_SaberInReturn(newMove) ) //or new move isn't slowbounce move
		{//switched away from a slow bounce move, remove the flags.
			pm->ps->userInt3 &= ~( 1 << FLAG_SLOWBOUNCE );
			pm->ps->userInt3 &= ~( 1 << FLAG_OLDSLOWBOUNCE );
			pm->ps->userInt3 &= ~( 1 << FLAG_PARRIED );
			//[QuickParry]
//			pm->ps->userInt3 &= ~( 1 << FLAG_QUICKPARRY);
			//[/QuickParry]
		}

		if(!PM_SaberInParry(newMove))
		{//cancel out pre-block flag
			pm->ps->userInt3 &= ~( 1 << FLAG_PREBLOCK );
		}
		//[/SaberSys]

		if ( BG_SaberInAttack( newMove ) || BG_SaberInSpecialAttack( anim ) )
		{
			if ( pm->ps->saberMove != newMove )
			{//wasn't playing that attack before
				if ( newMove != LS_KICK_F
					&& newMove != LS_KICK_B
					&& newMove != LS_KICK_R
					&& newMove != LS_KICK_L
					&& newMove != LS_KICK_F_AIR
					&& newMove != LS_KICK_B_AIR
					&& newMove != LS_KICK_R_AIR
					&& newMove != LS_KICK_L_AIR )
				{
                    PM_AddEvent(EV_SABER_ATTACK);
				}

				if (pm->ps->brokenLimbs)
				{ //randomly make pain sounds with a broken arm because we are suffering.
					int iFactor = -1;

					if (pm->ps->brokenLimbs & (1<<BROKENLIMB_RARM))
					{ //You're using it more. So it hurts more.
						iFactor = 5;
					}
					else if (pm->ps->brokenLimbs & (1<<BROKENLIMB_LARM))
					{
						iFactor = 10;
					}

					if (iFactor != -1)
					{
						if ( !PM_irand_timesync( 0, iFactor ) )
						{
							BG_AddPredictableEventToPlayerstate(EV_PAIN, PM_irand_timesync( 1, 100 ), pm->ps);
						}
					}
				}
			}
		}

		if (BG_SaberInSpecial(newMove) &&
			pm->ps->weaponTime < pm->ps->torsoTimer)
		{ //rww 01-02-03 - I think this will solve the issue of special attacks being interruptable, hopefully without side effects
			pm->ps->weaponTime = pm->ps->torsoTimer;
		}

		pm->ps->saberMove = newMove;
		pm->ps->saberBlocking = saberMoveData[newMove].blocking;

		pm->ps->torsoAnim = anim;

		if (pm->ps->weaponTime <= 0)
		{
			pm->ps->saberBlocked = BLOCKED_NONE;
		}
	}
}

saberInfo_t *BG_MySaber( int clientNum, int saberNum )
{
	//returns a pointer to the requested saberNum
#ifdef QAGAME
	gentity_t *ent = &g_entities[clientNum];
	if ( ent->inuse && ent->client )
	{
		if ( !ent->client->saber[saberNum].model[0] )
		{ //don't have saber anymore!
			return NULL;
		}
		return &ent->client->saber[saberNum];
	}
#elif defined CGAME
	clientInfo_t *ci = NULL;
	if (clientNum < MAX_CLIENTS)
	{
		ci = &cgs.clientinfo[clientNum];
	}
	else
	{
		centity_t *cent = &cg_entities[clientNum];
		if (cent->npcClient)
		{
			ci = cent->npcClient;
		}
	}
	if ( ci 
		&& ci->infoValid )
	{
		if ( !ci->saber[saberNum].model[0] )
		{ //don't have sabers anymore!
			return NULL;
		}
		return &ci->saber[saberNum];
	}
#endif

	return NULL;
}


//[MELEE]
//converted all the convulted kick code into one function for easy upgrading.
qboolean PM_DoKick(void)
{//perform a kick.
	int kickMove = -1;

	if ( !BG_KickingAnim(pm->ps->torsoAnim) &&
		!BG_KickingAnim(pm->ps->legsAnim) &&
		!BG_InRoll(pm->ps, pm->ps->legsAnim) &&
//			!BG_KickMove( pm->ps->saberMove )//not already in a kick
		pm->ps->weaponTime <= 0
		//pm->ps->saberMove == LS_READY //racc - don't use this since Dodge resets this flag and the saber system doesn't go back to LS_READY unless the saber is being used.
		//&& !(pm->ps->pm_flags&PMF_DUCKED)//not ducked
		//&& (pm->cmd.upmove >= 0 ) //not trying to duck
		)
	{//player kicks
		kickMove = PM_KickMoveForConditions();
	}

	if (kickMove != -1)
	{
		if ( pm->ps->groundEntityNum == ENTITYNUM_NONE )
		{//if in air, convert kick to an in-air kick
			float gDist = PM_GroundDistance();
			//let's only allow air kicks if a certain distance from the ground
			//it's silly to be able to do them right as you land.
			//also looks wrong to transition from a non-complete flip anim...
			if ((!BG_FlippingAnim( pm->ps->legsAnim ) || pm->ps->legsTimer <= 0) &&
				gDist > 64.0f && //strict minimum
				gDist > (-pm->ps->velocity[2])-64.0f //make sure we are high to ground relative to downward velocity as well
				)
			{
				switch ( kickMove )
				{
				case LS_KICK_F:
					kickMove = LS_KICK_F_AIR;
					break;
				case LS_KICK_B:
					kickMove = LS_KICK_B_AIR;
					break;
				case LS_KICK_R:
					kickMove = LS_KICK_R_AIR;
					break;
				case LS_KICK_L:
					kickMove = LS_KICK_L_AIR;
					break;
				default: //oh well, can't do any other kick move while in-air
					kickMove = -1;
					break;
				}
			}
			else
			{//leave it as a normal kick unless we're too high up
				if ( gDist > 128.0f || pm->ps->velocity[2] >= 0 )
				{ //off ground, but too close to ground
					kickMove = -1;
				}
			}
		}

		if (kickMove != -1 && BG_EnoughForcePowerForMove(FATIGUE_SABERATTACK))
		{
			PM_SetSaberMove( kickMove );
			return qtrue;
		}
	}

	return qfalse;
}
//[/MELEE]


//[SaberSys]
qboolean BG_SaberInNonIdleDamageMove(playerState_t *ps, int AnimIndex);
void PM_SaberFakeFlagUpdate(playerState_t *ps, int newMove, int currentMove)
{//checks to see if the attack fake flag needs to be removed.
	if(!PM_SaberInTransition(newMove) && !PM_SaberInStart(newMove) && !BG_SaberInAttackPure(newMove))
	{//not going into an attack move, clear the flag
		pm->ps->userInt3 &= ~( 1 << FLAG_ATTACKFAKE );
	}
}


//saber status utility tools
qboolean BG_SaberInFullDamageMove( playerState_t *ps, int AnimIndex )
{//The player is attacking with a saber attack that does full damage
	if( (BG_SaberInAttack(ps->saberMove) && !BG_KickMove(ps->saberMove) && !BG_InSaberLock(ps->torsoAnim))
		|| BG_SuperBreakWinAnim(ps->torsoAnim) )
	{//in attack animation
		if( (ps->saberMove == LS_A_FLIP_STAB || ps->saberMove == LS_A_FLIP_SLASH)
			&& (BG_GetTorsoAnimPoint(ps, AnimIndex) <= .5 || BG_GetTorsoAnimPoint(ps, AnimIndex) >= .87)) //assumes that the dude is 
		{//flip attacks shouldn't do damage during the whole move.
			return qfalse;
		}

		if(ps->saberMove == BOTH_ROLL_STAB && BG_GetTorsoAnimPoint(ps, AnimIndex) <= .5)
		{//don't do damage during the follow thru part of the roll stab.
			return qfalse;
		}

		if(ps->saberBlocked == BLOCKED_NONE)
		{//and not attempting to do some sort of block animation
			return qtrue;
		}
	}
	return qfalse;
}

qboolean BG_SaberInTransitionDamageMove( playerState_t *ps )
{//player is in a saber move where it does transitional damage
	if(PM_SaberInTransition(ps->saberMove)) 
	{
		if(ps->saberBlocked == BLOCKED_NONE)
		{//and not attempting to do some sort of block animation
			return qtrue;
		}
	}
	return qfalse;
}


qboolean BG_SaberInNonIdleDamageMove(playerState_t *ps, int AnimIndex) 
{//player is in a saber move that does something more than idle saber damage
	return BG_SaberInFullDamageMove(ps, AnimIndex);
}


extern qboolean BG_BounceAnim( int anim );
extern qboolean PM_SaberReturnAnim( int anim );
qboolean BG_InSlowBounce(playerState_t *ps)
{//checks for a bounce/return animation in combination with the slow bounce flag
	if(ps->userInt3 & (1 << FLAG_SLOWBOUNCE)
		&& (BG_BounceAnim(ps->torsoAnim) || PM_SaberReturnAnim(ps->torsoAnim)) )
	{//in slow bounce
		return qtrue;
	}
	return qfalse;
}
//[/SaberSys]




