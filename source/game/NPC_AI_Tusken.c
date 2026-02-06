//
// NPC_AI_Tusken.c
// Tusken Raider AI (MP) - SP-inspired, but kept in MP "in-game" style.
//

#include "b_local.h"
#include "anims.h"

// Implemented in NPC.c. Needed here for SP-parity fallback behavior.
// (Fixes MSVC C4013: implicit declaration/assumed int.)
extern void NPC_BehaviorSet_Default( int bState );

extern int PM_AnimLength( int index, animNumber_t anim );
extern void NPC_BSST_Patrol( void );

static void Tusken_StaffHitSound( gentity_t *ent )
{
	G_SoundOnEnt( ent, CHAN_WEAPON, va( "sound/weapons/tusken_staff/stickhit%d.wav", Q_irand( 1, 4 ) ) );
}

void NPC_Tusken_Precache( void )
{
	int i;
	for ( i = 1; i <= 4; i++ )
	{
		G_SoundIndex( va( "sound/weapons/tusken_staff/stickhit%d.wav", i ) );
	}
}

// Returns qtrue when the current tusken attack anim is within its damage window.
static qboolean Tusken_InAttackDamageWindow( gentity_t *self )
{
	int animLen, elapsed;

	if ( !self || !self->client )
	{
		return qfalse;
	}

	switch ( self->client->ps.torsoAnim )
	{
	case BOTH_TUSKENATTACK1:
	case BOTH_TUSKENATTACK2:
	case BOTH_TUSKENATTACK3:
	case BOTH_TUSKENLUNGE1:
		break;
	default:
		return qfalse;
	}

	animLen = PM_AnimLength( 0, (animNumber_t)self->client->ps.torsoAnim );
	if ( animLen <= 0 )
	{
		return qfalse;
	}

	// torsoAnimTimer counts down.
	elapsed = animLen - self->client->ps.torsoTimer;
	if ( elapsed < 0 )
	{
		elapsed = 0;
	}

	// SP-like rough windows (kept conservative).
	switch ( self->client->ps.torsoAnim )
	{
	case BOTH_TUSKENATTACK1:
	case BOTH_TUSKENATTACK2:
		return ( elapsed > (int)( animLen * 0.30f ) && elapsed < (int)( animLen * 0.70f ) );
	case BOTH_TUSKENATTACK3:
		return ( elapsed > (int)( animLen * 0.10f ) && elapsed < (int)( animLen * 0.50f ) );
	case BOTH_TUSKENLUNGE1:
		return ( elapsed > (int)( animLen * 0.25f ) && elapsed < (int)( animLen * 0.70f ) );
	default:
		break;
	}

	return qfalse;
}

static void Tusken_StaffTrace( void )
{
	vec3_t fwd, start, end, mins, maxs;
	trace_t tr;

	if ( !NPC || !NPC->client )
	{
		return;
	}

	if ( !NPC->enemy || NPC->enemy->health <= 0 )
	{
		return;
	}

	// Only apply damage once per swing window.
	if ( !Tusken_InAttackDamageWindow( NPC ) )
	{
		return;
	}
	if ( !TIMER_Done( NPC, "tuskenStaffDmg" ) )
	{
		return;
	}
	TIMER_Set( NPC, "tuskenStaffDmg", 200 );

	AngleVectors( NPC->r.currentAngles, fwd, NULL, NULL );
	VectorCopy( NPC->r.currentOrigin, start );
	start[2] += NPC->client->ps.viewheight;

	VectorMA( start, 72.0f, fwd, end );

	VectorSet( mins, -4, -4, -4 );
	VectorSet( maxs,  4,  4,  4 );

	trap_Trace( &tr, start, mins, maxs, end, NPC->s.number, MASK_SHOT );

	if ( tr.fraction < 1.0f && tr.entityNum < ENTITYNUM_WORLD )
	{
		gentity_t *hitEnt = &g_entities[tr.entityNum];
		if ( hitEnt->takedamage )
		{
			int dmg = Q_irand( 5, 10 ) * ( g_spskill.integer + 1 );
			Tusken_StaffHitSound( hitEnt );
			G_Damage( hitEnt, NPC, NPC, fwd, tr.endpos, dmg, DAMAGE_NO_KNOCKBACK, MOD_MELEE );
		}
	}
}

void NPC_BSTusken_Default( void )
{
	// Simple SP-style loop: patrol until enemy, then close and melee.
	if ( !NPC || !NPC->client )
	{
		return;
	}

	if ( !NPC->enemy )
	{
		NPC_BSST_Patrol();
		return;
	}

	// Face and move toward enemy.
	NPC_FaceEnemy( qtrue );
	NPC_MoveToGoal( qtrue );

	// Use attack anims when close.
	if ( DistanceSquared( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin ) < ( 96.0f * 96.0f ) )
	{
		if ( TIMER_Done( NPC, "tuskenSwing" ) )
		{
			int anim = Q_irand( 0, 3 );
			switch ( anim )
			{
			case 0: anim = BOTH_TUSKENATTACK1; break;
			case 1: anim = BOTH_TUSKENATTACK2; break;
			case 2: anim = BOTH_TUSKENATTACK3; break;
			default: anim = BOTH_TUSKENLUNGE1; break;
			}
			NPC_SetAnim(NPC, SETANIM_TORSO, anim, SETANIM_FLAG_OVERRIDE);
			TIMER_Set( NPC, "tuskenSwing", Q_irand( 600, 1100 ) );
		}
	}

	Tusken_StaffTrace();
}

void NPC_BehaviorSet_Tusken( int bState )
{
	// SP parity: Tusken uses the standard bState wrapper and falls back to Default behavior set.
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSTusken_Default();
		break;

	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}