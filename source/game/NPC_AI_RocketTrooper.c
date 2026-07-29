// Rocket Trooper AI code
// Keep the same in-game notation as the rest of the NPC AI modules.
#include "b_local.h"

qboolean RT_Flying( gentity_t *self );

extern void G_CheckCharmed( gentity_t *self );
extern void NPC_BehaviorSet_Stormtrooper( int bState );
extern void Boba_FlyStart( gentity_t *ent );
extern void Boba_FlyStop( gentity_t *ent );
extern qboolean NPC_CheckEnemyExt( qboolean checkAlerts );
extern gentity_t *FindClosestPlayer( vec3_t position, int enemyTeam );
extern qboolean G_ValidEnemy( gentity_t *self, gentity_t *enemy );




static void RT_TakeOff( void )
{
	if ( !NPC || !NPC->client )
	{
		return;
	}

	if ( RT_Flying( NPC ) )
	{
		return;
	}

	// SP inspiration: rocket troopers will commit to flight when engaged.
	// In MP, enemy may not be set yet, so allow an alert-based acquire.
	if ( NPC->enemy && !G_ValidEnemy( NPC, NPC->enemy ) )
	{
		G_ClearEnemy( NPC );
	}

	if ( !NPC->enemy )
	{
		NPC_CheckEnemyExt( qtrue );
		// Fallback: in some MP modes NPC_CheckEnemyExt may not populate enemy
		// immediately. Use the same helper many MP AIs use.
		if ( !NPC->enemy && NPC->client )
		{
			gentity_t *closestPlayer = FindClosestPlayer( NPC->r.currentOrigin, NPC->client->enemyTeam );
			if ( closestPlayer && G_ValidEnemy( NPC, closestPlayer ) )
			{
				NPC->enemy = closestPlayer;
			}
		}
		if ( !NPC->enemy )
		{
			return;
		}
	}

	if ( !TIMER_Done( NPC, "takeoffDebounce" ) )
	{
		return;
	}

	// Take off whenever engaged (SP-style). This avoids the close-range "stuck" case.
	Boba_FlyStart( NPC );
	TIMER_Set( NPC, "rtTakeoff", Q_irand( 700, 1000 ) );
	TIMER_Set( NPC, "takeoffDebounce", Q_irand( 2000, 4000 ) );
}

// SP-inspired, MP-safe jetpack steering:
// Maintain a height above the enemy, strafe on timers, and keep a distance band.
static void RT_JetpackMove( void )
{
	float distSqr;
	float desiredZ;
	float zDelta;
	vec3_t toEnemy;

	if ( !NPC || !NPC->client )
	{
		return;
	}

	if ( NPC->enemy && !G_ValidEnemy( NPC, NPC->enemy ) )
	{
		G_ClearEnemy( NPC );
	}

	if ( !NPC->enemy )
	{
		NPC_CheckEnemyExt( qtrue );
		if ( !NPC->enemy && NPC->client )
		{
			gentity_t *closestPlayer = FindClosestPlayer( NPC->r.currentOrigin, NPC->client->enemyTeam );
			if ( closestPlayer && G_ValidEnemy( NPC, closestPlayer ) )
			{
				NPC->enemy = closestPlayer;
			}
		}
		if ( !NPC->enemy )
		{
			// No enemy: stop flying so we don't hover forever.
			Boba_FlyStop( NPC );
			return;
		}
	}

	NPC_FaceEnemy( qtrue );
	NPC_UpdateAngles( qtrue, qtrue );
	// Keep jetpack flags alive like MP bots so client renders flames.
	NPC->client->jetPackOn = qtrue;
	NPC->client->ps.pm_type = PM_JETPACK;
	NPC->client->ps.eFlags |= (EF_JETPACK|EF_JETPACK_ACTIVE|EF_JETPACK_FLAMING);
	NPC->client->ps.eFlags2 |= EF2_FLYING;


	static int s_rtHoverSound = 0;
	NPC->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_JETPACK);
	if ( !s_rtHoverSound )
	{
		s_rtHoverSound = G_SoundIndex( "sound/boba/jethover.wav" );
	}
	NPC->s.loopSound = s_rtHoverSound;
	// Maintain height relative to enemy (SP MaintainHeight intent).
	desiredZ = NPC->enemy->r.currentOrigin[2] + 180.0f;
	zDelta = desiredZ - NPC->r.currentOrigin[2];
	if ( zDelta > 24.0f )
	{
		ucmd.upmove = 127;
	}
	else if ( zDelta < -48.0f )
	{
		ucmd.upmove = -127;
	}

	// Strafe direction changes on a timer.
	if ( TIMER_Done( NPC, "rtStrafe" ) )
	{
		if ( Q_irand( 0, 1 ) )
		{
			TIMER_Set( NPC, "rtStrafeLeft", Q_irand( 500, 900 ) );
			TIMER_Set( NPC, "rtStrafeRight", 0 );
		}
		else
		{
			TIMER_Set( NPC, "rtStrafeRight", Q_irand( 500, 900 ) );
			TIMER_Set( NPC, "rtStrafeLeft", 0 );
		}
		TIMER_Set( NPC, "rtStrafe", Q_irand( 800, 1400 ) );
	}
	if ( !TIMER_Done( NPC, "rtStrafeLeft" ) )
	{
		ucmd.rightmove = -127;
	}
	else if ( !TIMER_Done( NPC, "rtStrafeRight" ) )
	{
		ucmd.rightmove = 127;
	}

	// Distance band: keep pressure but don't sit perfectly still.
	VectorSubtract( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, toEnemy );
	distSqr = VectorLengthSquared( toEnemy );
	if ( distSqr > (450.0f * 450.0f) )
	{
		ucmd.forwardmove = 127;
	}
	else if ( distSqr < (200.0f * 200.0f) )
	{
		ucmd.forwardmove = -127;
	}
	else
	{
		// In-band drift so we never hover motionless.
		ucmd.forwardmove = 32;
	}

	// Takeoff commit: force upward thrust briefly after starting flight.
	// If we are still grounded (common at very short distances), extend the commit
	// so we reliably lift off instead of "jetpack on but stuck".
	if ( NPC->client->ps.groundEntityNum != ENTITYNUM_NONE )
	{
		if ( TIMER_Done( NPC, "rtTakeoff" ) )
		{
			TIMER_Set( NPC, "rtTakeoff", 500 );
		}
	}
	if ( !TIMER_Done( NPC, "rtTakeoff" ) )
	{
		ucmd.upmove = 127;
		if ( ucmd.forwardmove < 64 )
		{
			ucmd.forwardmove = 64;
		}
	}
}

void RT_FireDecide( void )
{
	qboolean enemyLOS;
	qboolean enemyInFOV;
	float dot;
	vec3_t enemyDir, shootDir;

	RT_TakeOff();

	if ( !NPC || !NPC->client || !NPC->enemy )
	{
		return;
	}

	enemyLOS = NPC_ClearLOS4( NPC->enemy );
	VectorSubtract( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, enemyDir );
	VectorNormalize( enemyDir );
	AngleVectors( NPC->client->ps.viewangles, shootDir, NULL, NULL );
	dot = DotProduct( enemyDir, shootDir );
	enemyInFOV = ( dot > 0.5f );

	if ( enemyLOS && enemyInFOV )
	{
		ucmd.buttons |= BUTTON_ATTACK;
	}
}

//=====================================================================================
//FLYING behavior 
//=====================================================================================
qboolean RT_Flying( gentity_t *self )
{
	if ( !self || !self->client )
	{
		return qfalse;
	}
	return (qboolean)( (self->client->ps.eFlags2 & EF2_FLYING) ||
		(self->client->ps.pm_type == PM_JETPACK) ||
		(self->client->ps.eFlags & (EF_JETPACK_ACTIVE|EF_JETPACK_FLAMING)) ||
		(self->client->jetPackOn) );
}



// -----------------------------------------------------------------------------
// SP-style behavior wrapper
// -----------------------------------------------------------------------------

void NPC_BehaviorSet_RocketTrooper( int bState )
{
    if ( !NPC || !NPC->client || !NPCInfo )
    {
        return;
    }

	// Acquire an enemy if possible (SP inspiration: rocket troopers stay combat-focused).
	if ( !NPC->enemy )
	{
		NPC_CheckEnemyExt( qtrue );
	}

	// If we're flying or taking off, run SP-inspired jetpack steering.
	if ( RT_Flying( NPC ) || !TIMER_Done( NPC, "rtTakeoff" ) )
	{
		RT_JetpackMove();
		RT_FireDecide();
		// SP-style: don't forcibly land just because the enemy is close.
		// Close-range "stuck" comes from taking off then immediately landing.
	}
	else
	{
		// Ground behavior first, then check for takeoff and overwrite movement if needed.
		NPC_BehaviorSet_Stormtrooper( bState );
		RT_TakeOff();
		if ( !TIMER_Done( NPC, "rtTakeoff" ) )
		{
			// Immediately apply takeoff inputs after ground AI.
			ucmd.upmove = 127;
			ucmd.forwardmove = 64;
		}
	}

    G_CheckCharmed( NPC );
}

