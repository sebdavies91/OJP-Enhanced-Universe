//[SPPortComplete]
#include "b_local.h"
#include "g_nav.h"
#include "anims.h"
#include "w_saber.h"

extern void Boba_FireDecide( void );
extern qboolean Boba_Flying( gentity_t *self );
extern void Boba_FlyStart( gentity_t *self );
extern void Boba_FlyStop( gentity_t *self );
extern gentity_t *FindClosestPlayer( vec3_t position, int enemyTeam );
extern qboolean G_ValidEnemy( gentity_t *self, gentity_t *enemy );
extern float G_GroundDistance( gentity_t *self );

void NPC_BehaviorSet_BobaFett( int bState )
{
	// SP intent: Boba has a dedicated flight "mode" and should keep hunting/orbiting while in the air.
	// MP reality: we must drive that via ucmd after weapon logic so movement isn't stomped.

	if ( !NPC || !NPC->client || !NPCInfo )
	{
		return;
	}

	// Aggressive enemy reacquire (RocketTrooper-style). Avoid "hover idle" when enemy pointer drops.
	if ( NPC->enemy && !G_ValidEnemy( NPC, NPC->enemy ) )
	{
		G_ClearEnemy( NPC );
	}

	if ( !NPC->enemy )
	{
		NPC_CheckEnemyExt( qtrue );
		if ( !NPC->enemy )
		{
			gentity_t *closestPlayer = FindClosestPlayer( NPC->r.currentOrigin, NPC->client->enemyTeam );
			if ( closestPlayer && G_ValidEnemy( NPC, closestPlayer ) )
			{
				NPC->enemy = closestPlayer;
			}
		}
	}

	// If we're not flying, use normal humanoid ground behavior.
	if ( !Boba_Flying( NPC ) )
	{
		NPC_BSST_Default();

		if ( NPC->enemy )
		{
			vec3_t flat;
			VectorSubtract( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, flat );
			flat[2] = 0.0f;
			const float distSq2D = VectorLengthSquared( flat );

			// SP-like: take off once engaged unless point-blank. Debounced.
			// In SP, Boba will also jet to reposition even at relatively short range.
			if ( TIMER_Done( NPC, "takeoffDebounce" )
				&& ( distSq2D > ( 32.0f * 32.0f ) || Q_irand( 0, 3 ) == 0 ) )
			{
				Boba_FlyStart( NPC );
				TIMER_Set( NPC, "takeoffDebounce", Q_irand( 2000, 3500 ) );
			}

			Boba_FireDecide();
		}
		G_CheckCharmed( NPC );
		return;
	}

	// --- Flying behavior ---
	if ( NPC->enemy )
	{
		Boba_FireDecide();
	}

	// Apply jetpack steering LAST (RocketTrooper pattern) so we don't hover-stall.
	{
		vec3_t toEnemy, flat;
		float distSq2D = 0.0f;
		float desiredZ = NPC->r.currentOrigin[2];

		if ( NPC->enemy )
		{
			VectorSubtract( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, toEnemy );
			VectorCopy( toEnemy, flat );
			flat[2] = 0.0f;
			distSq2D = VectorLengthSquared( flat );

			NPC_FaceEnemy( qtrue );
			NPC_UpdateAngles( qtrue, qtrue );

			// Hover around enemy height (SP-like), with a small per-NPC wobble.
			desiredZ = NPC->enemy->r.currentOrigin[2] + 96.0f + ( ( (NPC->s.number + (level.time / 1000)) & 1 ) ? 24.0f : -24.0f );
		}
		else
		{
			// No enemy: land after a short grace period (don't hover frozen).
			const int t = TIMER_Get( NPC, "bobaNoEnemyLand" );
			if ( t == -1 )
			{
				TIMER_Set( NPC, "bobaNoEnemyLand", 1200 );
			}
			else if ( t < level.time )
			{
				TIMER_Remove( NPC, "bobaNoEnemyLand" );
				Boba_FlyStop( NPC );
			}
			ucmd.forwardmove = 0;
			ucmd.rightmove = 0;
			ucmd.upmove = 0;
			G_CheckCharmed( NPC );
			return;
		}

		// Cap altitude above ground to avoid flying off into the sky.
		{
			extern float G_GroundDistance( gentity_t *self );
			const float groundDist = G_GroundDistance( NPC );
			if ( groundDist > 520.0f )
			{
				ucmd.upmove = -127;
			}
			else
			{
				const float dz = desiredZ - NPC->r.currentOrigin[2];
				if ( dz > 24.0f )
					ucmd.upmove = 127;
				else if ( dz < -24.0f )
					ucmd.upmove = -127;
				else
					ucmd.upmove = 0;
			}
		}

		// Timed strafe orbit.
		ucmd.rightmove = ( ((level.time / 600) + NPC->s.number) & 1 ) ? 127 : -127;

		// Horizontal pursuit band.
		if ( distSq2D > ( 700.0f * 700.0f ) )
			ucmd.forwardmove = 127;
		else if ( distSq2D < ( 220.0f * 220.0f ) )
			ucmd.forwardmove = -64;
		else
			ucmd.forwardmove = 80;

		// Add mild ranged pressure like SP Boba: if we have bowcaster and target is visible, shoot in bursts.
		if ( NPC->enemy
			&& (NPC->client->ps.stats[STAT_WEAPONS] & (1 << WP_BOWCASTER))
			&& NPC->client->ps.weapon == WP_BOWCASTER
			&& NPC_ClearLOS4( NPC->enemy )
			&& !(NPCInfo->aiFlags & NPCAI_FLAMETHROW) )
		{
			if ( ((level.time / 250) & 1) == 0 )
			{
				ucmd.buttons |= BUTTON_ATTACK;
			}
		}
	}

	G_CheckCharmed( NPC );
}
