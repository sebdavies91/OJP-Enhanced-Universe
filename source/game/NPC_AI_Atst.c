#include "b_local.h"

// NOTE (MP safety): We intentionally do NOT use WP_ATST_MAIN/WP_ATST_SIDE here.
// Those indices are not part of MP client weapon tables and can cause client-side
// weaponInfo indexing crashes. To preserve the SP outcome (AT-ST fires from its
// head guns), we spawn the projectiles directly from the correct model bolts.

// These values match the local ATST defines in MP g_weapon.c.
#define ATST_MAIN_VEL                   3000
#define ATST_MAIN_DAMAGE                150
 

#define ATST_SIDE_ALT_DAMAGE            300
#define ATST_SIDE_ALT_NPC_VELOCITY      2000
#define ATST_SIDE_ALT_ROCKET_SIZE       5
#define ATST_SIDE_ALT_SPLASH_DAMAGE     150
#define ATST_SIDE_ALT_SPLASH_RADIUS     512

extern void WP_FireTurretMissile( gentity_t *ent, vec3_t start, vec3_t dir, qboolean altFire,
                                 int damage, int velocity, int mod, gentity_t *ignore );
extern void G_GetBoltPosition( gentity_t *self, int boltIndex, vec3_t pos, int modelIndex );

#define	MIN_MELEE_RANGE		640
#define	MIN_MELEE_RANGE_SQR	( MIN_MELEE_RANGE * MIN_MELEE_RANGE )

#define MIN_DISTANCE		128
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

#define TURN_OFF			0x00000100//G2SURFACEFLAG_NODESCENDANTS

#define LEFT_ARM_HEALTH 40
#define RIGHT_ARM_HEALTH 40

extern void G_SoundOnEnt( gentity_t *ent, int channel, const char *soundPath );
extern gentity_t *FindClosestPlayer( vec3_t position, int enemyTeam );
/*
-------------------------
NPC_ATST_Precache
-------------------------
*/
void NPC_ATST_Precache(void)
{
	G_SoundIndex( "sound/chars/atst/atst_damaged1" );
	G_SoundIndex( "sound/chars/atst/atst_damaged2" );

//	RegisterItem( BG_FindItemForWeapon( WP_ATST_MAIN ));	//precache the weapon
	//rwwFIXMEFIXME: add this weapon
	RegisterItem( BG_FindItemForWeapon( WP_BOWCASTER ));	//precache the weapon
	RegisterItem( BG_FindItemForWeapon( WP_ROCKET_LAUNCHER ));	//precache the weapon

	G_EffectIndex( "env/med_explode2" );
//	G_EffectIndex( "smaller_chunks" );
	G_EffectIndex( "blaster/smoke_bolton" );
	G_EffectIndex( "explosions/droidexplosion1" );
}

// Cache the bolts we need for muzzle positions (SP sets these up in g_client.cpp;
// MP does not for NPC AT-STs).
static void ATST_EnsureBolts( void )
{
	renderInfo_t *ri;

	if ( !NPC || !NPC->client )
	{
		return;
	}

	ri = &NPC->client->renderInfo;

	// If the ghoul2 instance changed, previously cached bolt indices are invalid.
	if ( ri->lastG2 != NPC->ghoul2 )
	{
		ri->lastG2 = NPC->ghoul2;
		ri->headBolt = -1;
		ri->handLBolt = -1;
		ri->handRBolt = -1;
		NPCInfo->genericBolt1 = -1;
		NPCInfo->genericBolt2 = -1;
	}

	if ( !NPC->ghoul2 )
	{
		return;
	}

	if ( ri->headBolt < 0 )
	{
		ri->headBolt = trap_G2API_AddBolt( NPC->ghoul2, 0, "*head" );
	}
	if ( ri->handLBolt < 0 )
	{
		ri->handLBolt = trap_G2API_AddBolt( NPC->ghoul2, 0, "*flash1" ); // front left gun
	}
	if ( ri->handRBolt < 0 )
	{
		ri->handRBolt = trap_G2API_AddBolt( NPC->ghoul2, 0, "*flash2" ); // front right gun
	}
	if ( NPCInfo->genericBolt1 < 0 )
	{
		NPCInfo->genericBolt1 = (short)trap_G2API_AddBolt( NPC->ghoul2, 0, "*flash3" ); // side blaster
	}
	if ( NPCInfo->genericBolt2 < 0 )
	{
		NPCInfo->genericBolt2 = (short)trap_G2API_AddBolt( NPC->ghoul2, 0, "*flash4" ); // concussion/rocket
	}
}

/*
============================
ATST_LockUpperToLower

SP outcome: the AT-ST cockpit/head should visually track with the walker body.

In MP, independent look offsets (head/torso yaw ranges + lookTarget) can leave
the cockpit appearing slightly off-axis while the legs/body are correctly
turning. For AT-STs we want the upper section to follow the body, so we clamp
these ranges to zero and clear look targeting.
============================
*/
static void ATST_LockUpperToLower( void )
{
    if ( !NPC || !NPC->client )
    {
        return;
    }

    // Prevent any independent yaw/pitch offsets from being applied to the model.
    NPC->client->renderInfo.headYawRangeLeft   = 0;
    NPC->client->renderInfo.headYawRangeRight  = 0;
    NPC->client->renderInfo.headPitchRangeUp   = 0;
    NPC->client->renderInfo.headPitchRangeDown = 0;

    NPC->client->renderInfo.torsoYawRangeLeft  = 0;
    NPC->client->renderInfo.torsoYawRangeRight = 0;
    NPC->client->renderInfo.torsoPitchRangeUp  = 0;
    NPC->client->renderInfo.torsoPitchRangeDown= 0;

    // Clear look targeting so the cockpit doesn't try to "look" independently.
    NPC->client->renderInfo.lookMode   = LM_ENT;
    NPC->client->renderInfo.lookTarget = ENTITYNUM_NONE;
    NPC->client->renderInfo.lockYaw    = 0.0f;
}

static void ATST_GetMuzzle( int bolt, vec3_t out )
{
	if ( bolt >= 0 )
	{
		G_GetBoltPosition( NPC, bolt, out, 0 );
	}
	else
	{
		// Safe fallback: head-ish, never feet.
		CalcEntitySpot( NPC, SPOT_HEAD, out );
	}
}

/*
============================
ATST_GetCentralCannonMuzzle

The drivable AT-ST in MP effectively fires its main weapon from a central,
body-facing walker muzzle instead of from the side/head attachments. For NPCs
we emulate that with a stable point near the front-center of the cockpit so the
main blaster bolts visually come from the central cannon and always travel in
body-facing direction.
============================
*/
static void ATST_GetCentralCannonMuzzle( vec3_t out )
{
	vec3_t head, fwd, right, up;

	CalcEntitySpot( NPC, SPOT_HEAD, head );
	AngleVectors( NPC->r.currentAngles, fwd, right, up );

	VectorCopy( head, out );
	VectorMA( out, 40.0f, fwd, out );
	VectorMA( out, -14.0f, up, out );
}

static void ATST_GetFireDir( const vec3_t start, vec3_t dirOut )
{
	vec3_t target;

	if ( NPC->enemy )
	{
		CalcEntitySpot( NPC->enemy, SPOT_HEAD, target );
	}
	else
	{
		vec3_t fwd;
		AngleVectors( NPC->r.currentAngles, fwd, NULL, NULL );
		VectorMA( start, 1024.0f, fwd, target );
	}

	VectorSubtract( target, start, dirOut );
	VectorNormalize( dirOut );
}

static qboolean ATST_FacingEnemyForFire( float maxYawErrorDeg )
{
    vec3_t selfSpot, targetSpot, dir, ang;
    float yawErr;

    if ( !NPC || !NPC->enemy )
    {
        return qfalse;
    }

    // Compare *body* yaw to the actual yaw-to-enemy.
    // This prevents firing while the cockpit/head is twisted off to the side.
    CalcEntitySpot( NPC, SPOT_ORIGIN, selfSpot );
    CalcEntitySpot( NPC->enemy, SPOT_HEAD, targetSpot );
    VectorSubtract( targetSpot, selfSpot, dir );
    vectoangles( dir, ang );

    yawErr = AngleDelta( AngleNormalize360( ang[YAW] ), NPC->r.currentAngles[YAW] );
    return ( fabs( yawErr ) <= maxYawErrorDeg );
}

static void ATST_FireEnergyNoSplash( int bolt, int damage, int velocity )
{
	vec3_t start, dir;
	ATST_GetMuzzle( bolt, start );
	ATST_GetFireDir( start, dir );
	WP_FireTurretMissile( NPC, start, dir, qfalse, damage, velocity, MOD_TURBLAST, NULL );
}

static void ATST_FireRocket( int bolt, int damage, int velocity, int splashDmg, int splashRadius )
{
	gentity_t *missile;
	vec3_t start, dir;

	ATST_GetMuzzle( bolt, start );
	ATST_GetFireDir( start, dir );

	missile = CreateMissile( start, dir, velocity, 10000, NPC, qfalse );
	missile->classname = "rocket_proj";
	missile->s.weapon = WP_ROCKET_LAUNCHER;

	VectorSet( missile->r.maxs, ATST_SIDE_ALT_ROCKET_SIZE, ATST_SIDE_ALT_ROCKET_SIZE, ATST_SIDE_ALT_ROCKET_SIZE );
	VectorScale( missile->r.maxs, -1, missile->r.mins );

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_ROCKET;
	missile->splashMethodOfDeath = MOD_ROCKET_SPLASH;
	missile->clipmask = MASK_SHOT;
	missile->splashDamage = splashDmg;
	missile->splashRadius = splashRadius;
	missile->bounceCount = 0;
}

static void ATST_FireMainBurst( int damage, int velocity )
{
	vec3_t start, dir, right;
	vec3_t shotStart;

	ATST_GetCentralCannonMuzzle( start );
	ATST_GetFireDir( start, dir );
	AngleVectors( NPC->r.currentAngles, NULL, right, NULL );

	/* Fire a tight 2-bolt burst from the central cannon so it feels like the
	 * vehicle blaster stream, while still clearly originating from the front-center. */
	VectorMA( start, -3.0f, right, shotStart );
	WP_FireTurretMissile( NPC, shotStart, dir, qfalse, damage, velocity, MOD_TURBLAST, NULL );
	VectorMA( start, 3.0f, right, shotStart );
	WP_FireTurretMissile( NPC, shotStart, dir, qfalse, damage, velocity, MOD_TURBLAST, NULL );
}

//-----------------------------------------------------------------
#if 0
static void ATST_PlayEffect( gentity_t *self, const int boltID, const char *fx )
{
	if ( boltID >=0 && fx && fx[0] )
	{
		mdxaBone_t	boltMatrix;
		vec3_t		org, dir;

		trap_G2API_GetBoltMatrix( self->ghoul2, 0, 
					boltID,
					&boltMatrix, self->r.currentAngles, self->r.currentOrigin, level.time,
					NULL, self->modelScale );

		BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, org );
		BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_Y, dir );

		G_PlayEffectID( G_EffectIndex((char *)fx), org, dir );
	}
}
#endif

/*
-------------------------
G_ATSTCheckPain

Called by NPC's and player in an ATST
-------------------------
*/

void G_ATSTCheckPain( gentity_t *self, gentity_t *other, int damage )
{
	//int newBolt;
	//int hitLoc = gPainHitLoc;
	
	if ( rand() & 1 )
	{
		G_SoundOnEnt( self, CHAN_LESS_ATTEN, "sound/chars/atst/atst_damaged1" );
	}
	else
	{
		G_SoundOnEnt( self, CHAN_LESS_ATTEN, "sound/chars/atst/atst_damaged2" );
	}

	/*
	if ((hitLoc==HL_ARM_LT) && (self->locationDamage[HL_ARM_LT] > LEFT_ARM_HEALTH))
	{
		if (self->locationDamage[hitLoc] >= LEFT_ARM_HEALTH)	// Blow it up?
		{
			newBolt = trap_G2API_AddBolt( self->ghoul2, 0, "*flash3" );
			if ( newBolt != -1 )
			{
//				G_PlayEffect( "small_chunks", self->playerModel, self->genericBolt1, self->s.number);
				ATST_PlayEffect( self, trap_G2API_AddBolt(self->ghoul2, 0, "*flash3"), "env/med_explode2" );
				//G_PlayEffectID( G_EffectIndex("blaster/smoke_bolton"), self->playerModel, newBolt, self->s.number);
				//Maybe bother with this some other time.
			}

			NPC_SetSurfaceOnOff( self, "head_light_blaster_cann", TURN_OFF );
		}
	}
	else if ((hitLoc==HL_ARM_RT) && (self->locationDamage[HL_ARM_RT] > RIGHT_ARM_HEALTH))	// Blow it up?
	{
		if (self->locationDamage[hitLoc] >= RIGHT_ARM_HEALTH)
		{			
			newBolt = trap_G2API_AddBolt( self->ghoul2, 0, "*flash4" );
			if ( newBolt != -1 )
			{
//				G_PlayEffect( "small_chunks", self->playerModel, self->genericBolt2, self->s.number);
				ATST_PlayEffect( self, trap_G2API_AddBolt(self->ghoul2, 0, "*flash4"), "env/med_explode2" );
				//G_PlayEffect( "blaster/smoke_bolton", self->playerModel, newBolt, self->s.number);
			}

			NPC_SetSurfaceOnOff( self, "head_concussion_charger", TURN_OFF );
		}
	}
	*/
}
/*
-------------------------
NPC_ATST_Pain
-------------------------
*/
void NPC_ATST_Pain(gentity_t *self, gentity_t *attacker, int damage) 
{
	G_ATSTCheckPain( self, attacker, damage );
	NPC_Pain( self, attacker, damage );
}

/*
-------------------------
ATST_Hunt
-------------------------`
*/
void ATST_Hunt( qboolean visible, qboolean advance )
{
	if ( !NPC || !NPCInfo )
	{
		return;
	}

	ATST_LockUpperToLower();

	if ( NPCInfo->goalEntity == NULL )
	{
		NPCInfo->goalEntity = NPC->enemy;
	}

	if ( NPC->enemy )
	{
		vec3_t selfSpot, targetSpot, dir, ang;
		CalcEntitySpot( NPC, SPOT_ORIGIN, selfSpot );
		CalcEntitySpot( NPC->enemy, SPOT_HEAD, targetSpot );
		VectorSubtract( targetSpot, selfSpot, dir );
		vectoangles( dir, ang );
		NPCInfo->desiredYaw = AngleNormalize360( ang[YAW] );
		NPCInfo->desiredPitch = 0.0f;
	}

	NPCInfo->combatMove = qtrue;
	NPC_MoveToGoal( qtrue );
	NPC_UpdateAngles( qtrue, qtrue );
}

/*
-------------------------
ATST_Ranged
-------------------------
*/
void ATST_Ranged( qboolean visible, qboolean advance, qboolean altAttack, qboolean useMainGuns )
{
	qboolean canFire = qfalse;

	ATST_LockUpperToLower();

	if ( visible )
	{
		canFire = ATST_FacingEnemyForFire( 12.0f );
	}

	if ( TIMER_Done( NPC, "atkDelay" ) && visible && canFire )
	{
		TIMER_Set( NPC, "atkDelay", Q_irand( 500, 1500 ) );
		ATST_EnsureBolts();

		if ( altAttack )
		{
			ucmd.buttons |= (BUTTON_ATTACK|BUTTON_ALT_ATTACK);
		}
		else
		{
			ucmd.buttons |= BUTTON_ATTACK;
		}

		if ( useMainGuns )
		{
			ATST_FireMainBurst( ATST_MAIN_DAMAGE, ATST_MAIN_VEL );
		}
		else
		{
			ATST_FireRocket( (int)NPCInfo->genericBolt2, ATST_SIDE_ALT_DAMAGE, ATST_SIDE_ALT_NPC_VELOCITY,
						ATST_SIDE_ALT_SPLASH_DAMAGE, ATST_SIDE_ALT_SPLASH_RADIUS );
		}
	}

	if ( NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
	{
		ATST_Hunt( visible, advance );
	}
}

/*
-------------------------
ATST_Attack
-------------------------
*/
void ATST_Attack( void )
{
	qboolean	altAttack=qfalse;
	qboolean	useMainGuns=qfalse;
	int		chargerTest;
	float		distance;	
	distance_e	distRate;
	qboolean	visible;
	qboolean	advance;

	// Static analysis: protect against unexpected NULL globals in MP.
	if ( !NPC || !NPCInfo || !NPC->client )
	{
		return;
	}

	if ( NPC_CheckEnemyExt(qfalse) == qfalse )
	{
		NPC->enemy = NULL;
		return;
	}
	if ( !NPC->enemy )
	{
		return;
	}

	// Ensure our head bolt exists so facing/looking behaves correctly (SP sets this up at spawn).
	ATST_EnsureBolts();
	if ( !NPC->enemy )
	{
		return;
	}

	ATST_LockUpperToLower();

	if ( NPC->enemy )
	{
		vec3_t selfOrigin, target, dir, ang;
		CalcEntitySpot( NPC, SPOT_ORIGIN, selfOrigin );
		CalcEntitySpot( NPC->enemy, SPOT_HEAD, target );
		VectorSubtract( target, selfOrigin, dir );
		vectoangles( dir, ang );
		NPCInfo->desiredYaw = AngleNormalize360( ang[YAW] );
		NPCInfo->desiredPitch = 0.0f;
	}

	NPC_UpdateAngles( qtrue, qtrue );

	// Keep the AT-ST out of the normal player weapon pipeline.
	NPC_ChangeWeapon( WP_NONE );

	// Rate our distance to the target, and our visibility
	distance	= (int) DistanceHorizontalSquared( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin );	
	distRate	= ( distance > MIN_MELEE_RANGE_SQR ) ? DIST_LONG : DIST_MELEE;
	{
		vec3_t losStart, losEnd;
		CalcEntitySpot( NPC, SPOT_HEAD, losStart );
		CalcEntitySpot( NPC->enemy, SPOT_HEAD, losEnd );
		// Lift the start a bit so the trace doesn't immediately hit the walker hull.
		losStart[2] += 24.0f;
		visible = NPC_ClearLOS( losStart, losEnd );
	}
	advance		= (qboolean)(distance > MIN_DISTANCE_SQR);

	// If we cannot see our target, move to see it
	if ( visible == qfalse )
	{
		if ( NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
		{
			ATST_Hunt( visible, advance );
			return;
		}
	}

	// Decide what type of attack to do
	switch ( distRate )
	{
	case DIST_MELEE:
		// SP uses WP_ATST_MAIN here (front guns). We emulate that outcome directly.
		useMainGuns = qtrue;
		altAttack = qfalse;
		break;

	case DIST_LONG:
		// Long range: keep using the main cannons, but occasionally fire a rocket
		// if the launcher is still present.
		useMainGuns = qtrue;
		altAttack = qfalse;

		chargerTest = trap_G2API_GetSurfaceRenderStatus( NPC->ghoul2, 0, "head_concussion_charger" );
		if ( chargerTest != -1 && !(chargerTest & TURN_OFF) )
		{
			// Roughly 25% chance to use the rocket launcher.
			if ( Q_irand( 0, 3 ) == 0 )
			{
				altAttack = qtrue;
				useMainGuns = qfalse;
			}
		}
		break;
	}


	ATST_Ranged( visible, advance, altAttack, useMainGuns );
}

/*
-------------------------
ATST_Patrol
-------------------------
*/
void ATST_Patrol( void )
{
	// Keep cockpit/head aligned with the walker body while patrolling.
	ATST_LockUpperToLower();
	if ( NPC_CheckPlayerTeamStealth() )
	{
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

	//If we have somewhere to go, then do that
	if (!NPC->enemy)
	{
		if ( UpdateGoal() )
		{
			ucmd.buttons |= BUTTON_WALKING;
			NPC_MoveToGoal( qtrue );

			// Apply the same conservative unstick logic used in ATST_Hunt().
			// Patrol routes often go through doorways and narrow turns where walkers
			// can hang up on geometry.
			{
				navInfo_t info;
				int blockedFor;
				memset( &info, 0, sizeof( info ) );
				NAV_GetLastMove( &info );
				blockedFor = 0;
				if ( ( NPCInfo->aiFlags & NPCAI_BLOCKED ) && NPCInfo->blockedDebounceTime )
				{
					blockedFor = level.time - NPCInfo->blockedDebounceTime;
				}
				if ( ( info.flags & ( NIF_BLOCKED | NIF_COLLISION ) ) || blockedFor > 1000 )
				{
					if ( TIMER_Done( NPC, "atstUnstick" ) )
					{
						NPCInfo->localState = ( Q_irand( 0, 1 ) ? 1 : -1 );
						TIMER_Set( NPC, "atstUnstick", Q_irand( 700, 1100 ) );
						TIMER_Set( NPC, "atstUnstickBack", Q_irand( 250, 400 ) );
					}
					if ( !TIMER_Done( NPC, "atstUnstickBack" ) )
					{
						ucmd.forwardmove = -127;
					}
					else
					{
						ucmd.forwardmove = 127;
					}
					ucmd.rightmove = 0;
					NPCInfo->desiredPitch = 0.0f;
					NPCInfo->desiredYaw = AngleNormalize360( ( NPC->client ? NPC->client->ps.viewangles[YAW] : NPC->r.currentAngles[YAW] ) + ( (float)NPCInfo->localState * 35.0f ) );
				}
			}
			NPC_UpdateAngles( qtrue, qtrue );
		}
	}

}

/*
-------------------------
ATST_Idle
-------------------------
*/
void ATST_Idle( void )
{
	// Keep cockpit/head aligned with the walker body while idle.
	ATST_LockUpperToLower();

	NPC_BSIdle();

	NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_STAND1, SETANIM_FLAG_NORMAL );
}

/*
-------------------------
NPC_BSDroid_Default
-------------------------
*/
void NPC_BSATST_Default( void )
{
	// MP: some maps spawn AT-STs without SCF_LOOK_FOR_ENEMIES/SCF_CHASE_ENEMIES.
	// If we are allowed to engage, keep trying to acquire a target so the walker
	// doesn't sit idle until it is attacked.
	if ( !NPC->enemy && !(NPCInfo->scriptFlags & (SCF_IGNORE_ENEMIES|SCF_IGNORE_ALERTS)) )
	{
		NPC_CheckEnemyExt( qtrue );
		if ( !NPC->enemy && NPC->client )
		{
			int team = ( NPC->client->enemyTeam != NPCTEAM_FREE ) ? NPC->client->enemyTeam : -1;
			NPC->enemy = FindClosestPlayer( NPC->r.currentOrigin, team );
		}
	}

	if ( NPC->enemy )
	{
		if( (NPCInfo->scriptFlags & SCF_CHASE_ENEMIES) )
		{
			NPCInfo->goalEntity = NPC->enemy;
		}
		ATST_Attack();
	}
	else if ( NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES )
	{
		ATST_Patrol();
	} 
	else
	{
		ATST_Idle();
	}
}
