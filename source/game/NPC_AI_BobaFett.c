//[SPPortComplete]
#include "b_local.h"
#include "g_nav.h"
#include "anims.h"
#include "w_saber.h"

/*
** Boba Fett behavior wrapper.
** The core Boba_* implementation remains in NPC_AI_Jedi.c in this codebase.
** We only provide the BehaviorSet entry point here to keep class dispatch clean.
*/

extern void Boba_FireDecide( void );
extern qboolean Boba_Flying( gentity_t *self );
extern void Boba_FlyStart( gentity_t *self );
extern void Boba_FlyStop( gentity_t *self );
extern qboolean NPC_CheckEnemyExt( qboolean checkAlerts );
extern gentity_t *FindClosestPlayer( vec3_t position, int enemyTeam );

// SP-inspired, MP-safe jetpack steering for Boba.
// We mimic SP's MaintainHeight + strafe/hunt timing, but only via usercmd.
static void Boba_JetpackMoveMP( void )
{
	float distSqr;
	float desiredZ;
	float zDelta;
	vec3_t toEnemy;

	if ( !NPC || !NPC->client )
	{
		return;
	}

	// If we lost our enemy, don't keep "silent floating" — land cleanly.
	if ( !NPC->enemy )
	{
		Boba_FlyStop( NPC );
		return;
	}

	// Always face the enemy while flying, like SP.
	NPC_FaceEnemy( qtrue );
	NPC_UpdateAngles( qtrue, qtrue );
	// Keep MP jetpack system alive every frame while flying (weapon switching can clear these).
	NPC->client->jetPackOn = qtrue;
	NPC->client->ps.pm_type = PM_JETPACK;
	NPC->client->ps.eFlags |= (EF_JETPACK|EF_JETPACK_ACTIVE|EF_JETPACK_FLAMING);
	NPC->client->ps.eFlags2 |= EF2_FLYING;
	NPC->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_JETPACK);
	static int s_bobaHoverSound = 0;
	if ( !s_bobaHoverSound )
	{
		s_bobaHoverSound = G_SoundIndex( "sound/boba/jethover.wav" );
	}
	NPC->s.loopSound = s_bobaHoverSound;
	VectorSubtract( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, toEnemy );
	distSqr = VectorLengthSquared( toEnemy );

	// Maintain height relative to enemy. When jetPackTime expires, descend.
	if ( NPC->client->jetPackTime != Q3_INFINITE && NPC->client->jetPackTime < level.time )
	{
		desiredZ = NPC->enemy->r.currentOrigin[2] + 40.0f;
	}
	else
	{
		desiredZ = NPC->enemy->r.currentOrigin[2] + 220.0f;
	}
	zDelta = desiredZ - NPC->r.currentOrigin[2];
	if ( zDelta > 16.0f )
	{
		ucmd.upmove = 127;
	}
	else if ( zDelta < -24.0f )
	{
		ucmd.upmove = -127;
	}

	// Timed strafe/hunt motion (SP style - no hovering still).
	if ( TIMER_Done( NPC, "bobaStrafe" ) )
	{
		if ( Q_irand( 0, 1 ) )
		{
			TIMER_Set( NPC, "bobaStrafeLeft", Q_irand( 400, 800 ) );
			TIMER_Set( NPC, "bobaStrafeRight", 0 );
		}
		else
		{
			TIMER_Set( NPC, "bobaStrafeRight", Q_irand( 400, 800 ) );
			TIMER_Set( NPC, "bobaStrafeLeft", 0 );
		}
		TIMER_Set( NPC, "bobaStrafe", Q_irand( 600, 1000 ) );
	}
	if ( !TIMER_Done( NPC, "bobaStrafeLeft" ) )
	{
		ucmd.rightmove = -127;
	}
	else if ( !TIMER_Done( NPC, "bobaStrafeRight" ) )
	{
		ucmd.rightmove = 127;
	}

		// Preferred distance band like SP (pressure + reposition).
		// IMPORTANT: don't make Boba "retreat" (RocketTrooper doesn't). We'll keep
		// pressure when far, and strafe/hover when close so he keeps engaging.
	if ( distSqr > (450.0f * 450.0f) )
	{
		ucmd.forwardmove = 127;
	}
	else if ( distSqr < (200.0f * 200.0f) )
	{
			ucmd.forwardmove = 0;
	}
	else
	{
			// In-band: keep gentle forward pressure so he stays on the player,
			// while the strafe timers provide lateral movement.
			ucmd.forwardmove = 64;
	}

	// Takeoff commit: force upward thrust briefly after starting flight (or until airborne).
	if ( NPC->client->ps.groundEntityNum != ENTITYNUM_NONE )
	{
		// Still grounded - keep committing to takeoff so we don't get "jetpack on but stuck".
		if ( TIMER_Done( NPC, "bobaTakeoff" ) )
		{
			TIMER_Set( NPC, "bobaTakeoff", 500 );
		}
	}
	if ( !TIMER_Done( NPC, "bobaTakeoff" ) )
	{
		ucmd.upmove = 127;
		if ( ucmd.forwardmove < 64 )
		{
			ucmd.forwardmove = 64;
		}
	}


	// Landing conditions: SP will land when disengaging or when flight time is up.
	if ( NPC->client->jetPackTime != Q3_INFINITE && NPC->client->jetPackTime < level.time )
	{
		// MP safety: never cut the jetpack off while we're still high above ground.
		// That can make Boba drop, play a scream/pain, and die from fall damage.
		// Instead, only stop once we're actually on/near the ground.
		if ( NPC->client->ps.groundEntityNum != ENTITYNUM_NONE )
		{
			Boba_FlyStop( NPC );
		}
		else
		{
			trace_t tr;
			vec3_t start, end;
			float groundDist;

			VectorCopy( NPC->r.currentOrigin, start );
			VectorCopy( NPC->r.currentOrigin, end );
			end[2] -= 2048.0f;
			trap_Trace( &tr, start, NPC->r.mins, NPC->r.maxs, end, NPC->s.number, MASK_SOLID );
			groundDist = (start[2] - tr.endpos[2]);
			if ( tr.fraction < 1.0f && groundDist < 96.0f )
			{
				Boba_FlyStop( NPC );
			}
		}
	}
}
void NPC_BehaviorSet_BobaFett( int bState )
{
	if ( !NPC || !NPC->client || !NPCInfo )
	{
		return;
	}

	// Try to have an enemy so flight/weapon logic can behave like SP.
	if ( !NPC->enemy )
	{
		NPC_CheckEnemyExt( qtrue );
		if ( !NPC->enemy )
		{
			NPC->enemy = FindClosestPlayer( NPC->r.currentOrigin, NPC->client->enemyTeam );
		}
	}

	// SP-inspired: if we have an enemy at range, proactively take off so we can pursue
	// like the RocketTrooper instead of being a mostly-static turret.
	if ( NPC->enemy && !Boba_Flying( NPC ) && TIMER_Done( NPC, "bobaTakeoffDebounce" ) )
	{
		float distSqr = DistanceSquared( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin );
		// Take off at medium range, or sooner if the enemy has a saber (evasion).
		if ( distSqr > (192.0f*192.0f) || (NPC->enemy->s.weapon == WP_SABER && distSqr > (96.0f*96.0f)) )
		{
			Boba_FlyStart( NPC );
			TIMER_Set( NPC, "bobaTakeoff", Q_irand( 700, 1000 ) );
			TIMER_Set( NPC, "bobaTakeoffDebounce", Q_irand( 2500, 4500 ) );
		}
	}

	// Robustness: if custom gravity is still active, don't abruptly stop flying
	// (that can kill jetpack FX mid-air). Instead, re-assert the jetpack state.
	if ( NPC->client->ps.gravity == 0 && NPC->NPC && (NPC->NPC->aiFlags & NPCAI_CUSTOM_GRAVITY) )
	{
		NPC->client->ps.eFlags2 |= EF2_FLYING;
		NPC->client->ps.jetPackOn = qtrue;
		NPC->client->ps.eFlags |= (EF_JETPACK|EF_JETPACK_ACTIVE|EF_JETPACK_FLAMING);
	}

	// If flying, don't let generic ground AI overwrite our ucmd.
	if ( Boba_Flying( NPC ) )
	{
			// Make combat decisions first (weapon selection, flame), then apply jetpack steer/sustain
			// last so weapon switching can't clear jetpack FX/sound.
			Boba_FireDecide();
			Boba_JetpackMoveMP();
			return;
	}

	// Ground behavior first, then Boba decisions (may trigger takeoff).
	NPC_BSST_Default();
	Boba_FireDecide();
}
