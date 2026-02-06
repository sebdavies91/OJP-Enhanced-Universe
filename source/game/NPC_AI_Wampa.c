#include "b_local.h"
#include "g_nav.h"

// These define the working combat range for these suckers
#define MIN_DISTANCE		48
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

#define MAX_DISTANCE		1024
#define MAX_DISTANCE_SQR	( MAX_DISTANCE * MAX_DISTANCE )

#define LSTATE_CLEAR		0
#define LSTATE_WAITING		1

float enemyDist = 0;

// In MP, NPC_GetEntsNearBolt is used by several creature AIs (e.g. rancor/howler),
// but it is not declared in a shared header for all translation units.
// Provide a local prototype to avoid MSVC implicit-int warnings.
extern int NPC_GetEntsNearBolt( int *radiusEnts, float radius, int boltIndex, vec3_t boltOrg );

// Wampa grab/hold support (MP): modeled after the SP/MP rancor grab system.
static void Wampa_DropVictim( gentity_t *self );
static qboolean Wampa_TryGrab( void );
static void Wampa_HoldVictim( void );

void Wampa_SetBolts( gentity_t *self )
{
	if ( self && self->client )
	{
		renderInfo_t *ri = &self->client->renderInfo;
		ri->headBolt = trap_G2API_AddBolt(self->ghoul2, 0, "*head_eyes");
		//ri->cervicalBolt = trap_G2API_AddBolt(self->ghoul2, 0, "neck_bone" );
		//ri->chestBolt = trap_G2API_AddBolt(self->ghoul2, 0, "upper_spine");
		//ri->gutBolt = trap_G2API_AddBolt(self->ghoul2, 0, "mid_spine");
		ri->torsoBolt = trap_G2API_AddBolt(self->ghoul2, 0, "lower_spine");
		ri->crotchBolt = trap_G2API_AddBolt(self->ghoul2, 0, "rear_bone");
		//ri->elbowLBolt = trap_G2API_AddBolt(self->ghoul2, 0, "*l_arm_elbow");
		//ri->elbowRBolt = trap_G2API_AddBolt(self->ghoul2, 0, "*r_arm_elbow");
		ri->handLBolt = trap_G2API_AddBolt(self->ghoul2, 0, "*l_hand");
		ri->handRBolt = trap_G2API_AddBolt(self->ghoul2, 0, "*r_hand");
		//ri->kneeLBolt = trap_G2API_AddBolt(self->ghoul2, 0, "*hips_l_knee");
		//ri->kneeRBolt = trap_G2API_AddBolt(self->ghoul2, 0, "*hips_r_knee");
		ri->footLBolt = trap_G2API_AddBolt(self->ghoul2, 0, "*l_leg_foot");
		ri->footRBolt = trap_G2API_AddBolt(self->ghoul2, 0, "*r_leg_foot");
	}
}

static void Wampa_DropVictim( gentity_t *self )
{
	gentity_t	*victim;
vec3_t		start, end, dropOrg;
	trace_t		tr;
if ( !self || !self->activator || !self->activator->client )
	{
		if ( self )
		{
			self->count = 0;
			self->activator = NULL;
		}
		return;
	}

	victim = self->activator;

	victim->client->ps.eFlags2 &= ~EF2_HELD_BY_MONSTER;
	victim->client->ps.hasLookTarget = qfalse;
	victim->client->ps.lookTarget = ENTITYNUM_NONE;

	// Clear any held victim anim timers so movement/anim can resume immediately.
	victim->client->ps.legsTimer = 0;
	victim->client->ps.torsoTimer = 0;

	// Clear any "all times" locks so the client regains control immediately.
	victim->client->ps.pm_time = 0;
	victim->client->ps.pm_flags &= ~PMF_ALL_TIMES;
	victim->client->ps.groundEntityNum = ENTITYNUM_NONE;

	// Drop the victim to a safe, non-solid position and land them on real world ground.
	// Important: do not rely on a single forward drop; if the Wampa is near walls/ledges,
	// that can place the victim inside architecture. Try several directions.
	{
		vec3_t yawAngles;
		vec3_t fwd, right;
		vec3_t dirs[4];
		float dists[2] = { 160.0f, 224.0f };
		int dirI, distI;
		qboolean placed = qfalse;

		yawAngles[PITCH] = 0.0f;
		yawAngles[YAW] = self->r.currentAngles[YAW];
		yawAngles[ROLL] = 0.0f;
		AngleVectors( yawAngles, fwd, right, NULL );
		VectorNormalize( fwd );
		VectorNormalize( right );

		VectorCopy( fwd, dirs[0] );
		VectorCopy( right, dirs[1] );
		VectorScale( right, -1.0f, dirs[2] );
		VectorScale( fwd, -1.0f, dirs[3] );

		// Temporarily unlink the victim so we don't collide with their existing (held) link state.
		trap_UnlinkEntity( victim );

		for ( distI = 0; distI < 2 && !placed; distI++ )
		{
			for ( dirI = 0; dirI < 4 && !placed; dirI++ )
			{
				VectorCopy( self->r.currentOrigin, start );
				VectorMA( start, dists[distI], dirs[dirI], start );
				start[2] += 96.0f; // start above so we can fall to ground

				// Is the candidate start position free?
				trap_Trace( &tr, start, victim->r.mins, victim->r.maxs, start, self->s.number, MASK_PLAYERSOLID );
				if ( tr.startsolid || tr.allsolid )
				{
					continue;
				}

				VectorCopy( start, end );
				end[2] -= 1024.0f;
				trap_Trace( &tr, start, victim->r.mins, victim->r.maxs, end, self->s.number, MASK_PLAYERSOLID );
				if ( tr.startsolid || tr.allsolid )
				{
					continue;
				}
				if ( tr.fraction >= 1.0f )
				{
					continue; // no ground found
				}

				VectorCopy( tr.endpos, dropOrg );
				dropOrg[2] += 1.0f; // small epsilon

				// Final occupancy check.
				trap_Trace( &tr, dropOrg, victim->r.mins, victim->r.maxs, dropOrg, self->s.number, MASK_PLAYERSOLID );
				if ( tr.startsolid || tr.allsolid )
				{
					continue;
				}

				VectorCopy( dropOrg, victim->client->ps.origin );
				G_SetOrigin( victim, dropOrg );
				VectorClear( victim->client->ps.velocity );
				victim->client->ps.groundEntityNum = tr.entityNum;
				trap_LinkEntity( victim );
				placed = qtrue;
			}
		}

		if ( !placed )
		{
			// Worst-case fallback: put them at the Wampa origin and let the server fix-up on release handle it.
			VectorCopy( self->r.currentOrigin, dropOrg );
			dropOrg[2] += 96.0f;
			VectorCopy( dropOrg, victim->client->ps.origin );
			G_SetOrigin( victim, dropOrg );
			VectorClear( victim->client->ps.velocity );
			victim->client->ps.groundEntityNum = ENTITYNUM_NONE;
			trap_LinkEntity( victim );
		}
	}
// Prevent instant re-grab loops.
	TIMER_Set( self, "wampaNoGrab", 1500 );

	// If we were focusing our enemy pointer on the held victim, clear it.
	if ( self->enemy == victim )
	{
		self->enemy = NULL;
	}

	// Let go.
	self->count = 0;
	self->activator = NULL;
}

static qboolean Wampa_TryGrab( void )
{
	int     radiusEntNums[128];
	int     numEnts;
	int     i;
	vec3_t  boltOrg;
	const int   boltIndex = NPC->client->renderInfo.handRBolt;
	const float radius = 80.0f;

	if ( NPC->count )
	{
		return qfalse;
	}
	if ( !TIMER_Done( NPC, "wampaNoGrab" ) )
	{
		return qfalse;
	}
	if ( !NPC->enemy || !NPC->enemy->client || NPC->enemy->health <= 0 )
	{
		return qfalse;
	}
	if ( NPC->enemy->client->ps.eFlags2 & EF2_HELD_BY_MONSTER )
	{
		return qfalse;
	}

	// Don't grab "large" classes or special cases (mirrors other monster AI checks).
	if ( NPC->enemy->client->NPC_class == CLASS_RANCOR
		|| NPC->enemy->client->NPC_class == CLASS_WAMPA
		|| NPC->enemy->client->NPC_class == CLASS_ATST )
	{
		return qfalse;
	}

	numEnts = NPC_GetEntsNearBolt( radiusEntNums, radius, boltIndex, boltOrg );

	for ( i = 0; i < numEnts; i++ )
	{
		gentity_t *ent = &g_entities[radiusEntNums[i]];

		if ( !ent->inuse || ent == NPC )
		{
			continue;
		}
		if ( ent != NPC->enemy )
		{
			continue;
		}
		if ( !ent->client || ent->health <= 0 )
		{
			continue;
		}
		if ( ent->client->ps.eFlags2 & EF2_HELD_BY_MONSTER )
		{
			return qfalse;
		}

		NPC->activator = ent;
		NPC->count = 1;

		ent->client->ps.eFlags2 |= EF2_HELD_BY_MONSTER;
		ent->client->ps.hasLookTarget = qtrue;
		ent->client->ps.lookTarget = NPC->s.number;

		// Freeze victim in a safe generic pose while held.
		NPC_SetAnim( ent, SETANIM_BOTH, BOTH_SWIM_IDLE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
		ent->client->ps.torsoTimer = 0;
		ent->client->ps.legsTimer = 0;

		// Bite shortly after grabbing; also auto-drop as a safety net.
		TIMER_Set( NPC, "wampaBite", 1200 );
		TIMER_Set( NPC, "clearGrabbed", 3500 );

		return qtrue;
	}

	return qfalse;
}

static void Wampa_HoldVictim( void )
{
	if ( !NPC->activator || !NPC->activator->inuse || !NPC->activator->client )
	{
		Wampa_DropVictim( NPC );
		return;
	}

	// Don't spin chasing the held victim.
	ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;

	// Periodic bite damage.
	if ( TIMER_Done( NPC, "wampaBite" ) )
	{
		G_Damage( NPC->activator, NPC, NPC, NULL, NPC->activator->r.currentOrigin, 25, DAMAGE_NO_KNOCKBACK, MOD_MELEE );
		TIMER_Set( NPC, "wampaBite", 900 );
	}

	if ( NPC->activator->health <= 0 || TIMER_Done( NPC, "clearGrabbed" ) )
	{
		Wampa_DropVictim( NPC );
	}
}

/*
-------------------------
NPC_Wampa_Precache
-------------------------
*/
void NPC_Wampa_Precache( void )
{
	/*
	int i;
	for ( i = 1; i < 4; i ++ )
	{
		G_SoundIndex( va("sound/chars/wampa/growl%d.wav", i) );
	}
	for ( i = 1; i < 3; i ++ )
	{
		G_SoundIndex( va("sound/chars/wampa/snort%d.wav", i) );
	}
	*/
	G_SoundIndex( "sound/chars/rancor/swipehit.wav" );
	//G_SoundIndex( "sound/chars/wampa/chomp.wav" );
}


/*
-------------------------
Wampa_Idle
-------------------------
*/
void Wampa_Idle( void )
{
	NPCInfo->localState = LSTATE_CLEAR;

	//If we have somewhere to go, then do that
	if ( UpdateGoal() )
	{
		ucmd.buttons &= ~BUTTON_WALKING;
		NPC_MoveToGoal( qtrue );
	}
}

qboolean Wampa_CheckRoar( gentity_t *self )
{
	if ( self->wait < level.time )
	{
		self->wait = level.time + Q_irand( 5000, 20000 );
		NPC_SetAnim(self, SETANIM_BOTH, Q_irand(BOTH_GESTURE1,BOTH_GESTURE2), (SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD));
		TIMER_Set( self, "rageTime", self->client->ps.legsTimer );
		return qtrue;
	}
	return qfalse;
}
/*
-------------------------
Wampa_Patrol
-------------------------
*/
void Wampa_Patrol( void )
{
	NPCInfo->localState = LSTATE_CLEAR;

	//If we have somewhere to go, then do that
	if ( UpdateGoal() )
	{
		ucmd.buttons |= BUTTON_WALKING;
		NPC_MoveToGoal( qtrue );
	}
	else
	{
		if ( TIMER_Done( NPC, "patrolTime" ))
		{
			TIMER_Set( NPC, "patrolTime", crandom() * 5000 + 5000 );
		}
	}

	if ( NPC_CheckEnemyExt( qtrue ) == qfalse )
	{
		Wampa_Idle();
		return;
	}
	Wampa_CheckRoar( NPC );
	TIMER_Set( NPC, "lookForNewEnemy", Q_irand( 5000, 15000 ) );
}
 
/*
-------------------------
Wampa_Move
-------------------------
*/
void Wampa_Move( qboolean visible )
{
	if ( NPCInfo->localState != LSTATE_WAITING )
	{
		NPCInfo->goalEntity = NPC->enemy;

		if ( NPC->enemy )
		{//pick correct movement speed and anim
			//run by default
			ucmd.buttons &= ~BUTTON_WALKING;
			if ( !TIMER_Done( NPC, "runfar" ) 
				|| !TIMER_Done( NPC, "runclose" ) )
			{//keep running with this anim & speed for a bit
			}
			else if ( !TIMER_Done( NPC, "walk" ) )
			{//keep walking for a bit
				ucmd.buttons |= BUTTON_WALKING;
			}
			else if ( visible && enemyDist > 384 && NPCInfo->stats.runSpeed == 180 )
			{//fast run, all fours
				NPCInfo->stats.runSpeed = 300;
				TIMER_Set( NPC, "runfar", Q_irand( 2000, 4000 ) );
			}
			else if ( enemyDist > 256 && NPCInfo->stats.runSpeed == 300 )
			{//slow run, upright
				NPCInfo->stats.runSpeed = 180;
				TIMER_Set( NPC, "runclose", Q_irand( 3000, 5000 ) );
			}
			else if ( enemyDist < 128 )
			{//walk
				NPCInfo->stats.runSpeed = 180;
				ucmd.buttons |= BUTTON_WALKING;
				TIMER_Set( NPC, "walk", Q_irand( 4000, 6000 ) );
			}
		}

		if ( NPCInfo->stats.runSpeed == 300 )
		{//need to use the alternate run - hunched over on all fours
			NPC->client->ps.eFlags2 |= EF2_USE_ALT_ANIM;
		}
		NPC_MoveToGoal( qtrue );
		NPCInfo->goalRadius = MAX_DISTANCE;	// just get us within combat range
	}
}

//---------------------------------------------------------
//[KnockdownSys]
extern void G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );
//extern void G_Knockdown( gentity_t *victim );
//[/KnockdownSys]
extern void G_Dismember( gentity_t *ent, gentity_t *enemy, vec3_t point, int limbType, float limbRollBase, float limbPitchBase, int deathAnim, qboolean postDeath );

void Wampa_Slash( int boltIndex, qboolean backhand )
{
	int			radiusEntNums[128];
	int			numEnts;
	const float	radius = 88;
	const float	radiusSquared = (radius*radius);
	int			i;
	vec3_t		boltOrg;
	int			damage = (backhand)?Q_irand(10,15):Q_irand(20,30);

	numEnts = NPC_GetEntsNearBolt( radiusEntNums, radius, boltIndex, boltOrg );

	for ( i = 0; i < numEnts; i++ )
	{
		gentity_t *radiusEnt = &g_entities[radiusEntNums[i]];
		if ( !radiusEnt->inuse )
		{
			continue;
		}
		
		if ( radiusEnt == NPC )
		{//Skip the wampa ent
			continue;
		}
		
		if ( radiusEnt->client == NULL )
		{//must be a client
			continue;
		}

		if ( DistanceSquared( radiusEnt->r.currentOrigin, boltOrg ) <= radiusSquared )
		{
			//smack
			G_Damage( radiusEnt, NPC, NPC, vec3_origin, radiusEnt->r.currentOrigin, damage, ((backhand)?DAMAGE_NO_ARMOR:(DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK)), MOD_MELEE );
			if ( backhand )
			{
				//actually push the enemy
				vec3_t pushDir;
				vec3_t angs;
				VectorCopy( NPC->client->ps.viewangles, angs );
				angs[YAW] += flrand( 25, 50 );
				angs[PITCH] = flrand( -25, -15 );
				AngleVectors( angs, pushDir, NULL, NULL );
				if ( radiusEnt->client->NPC_class != CLASS_WAMPA
					&& radiusEnt->client->NPC_class != CLASS_RANCOR
					&& radiusEnt->client->NPC_class != CLASS_ATST )
				{
					G_Throw( radiusEnt, pushDir, 65 );
					if ( BG_KnockDownable(&radiusEnt->client->ps) &&
						radiusEnt->health > 0 && Q_irand( 0, 1 ) )
					{//do pain on enemy
						radiusEnt->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
						radiusEnt->client->ps.forceDodgeAnim = 0;
						radiusEnt->client->ps.forceHandExtendTime = level.time + 1100;
						radiusEnt->client->ps.quickerGetup = qfalse;
					}
				}
			}
			else if ( radiusEnt->health <= 0 && radiusEnt->client )
			{//killed them, chance of dismembering
				if ( !Q_irand( 0, 1 ) )
				{//bite something off
					int hitLoc = Q_irand( G2_MODELPART_HEAD, G2_MODELPART_RLEG );
					if ( hitLoc == G2_MODELPART_HEAD )
					{
						NPC_SetAnim(radiusEnt, SETANIM_BOTH, BOTH_DEATH17, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else if ( hitLoc == G2_MODELPART_WAIST )
					{
						NPC_SetAnim(radiusEnt, SETANIM_BOTH, BOTH_DEATHBACKWARD2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					G_Dismember( radiusEnt, NPC, radiusEnt->r.currentOrigin, hitLoc, 90, 0, radiusEnt->client->ps.torsoAnim, qtrue);
				}
			}
			else if ( !Q_irand( 0, 3 ) && radiusEnt->health > 0 )
			{//one out of every 4 normal hits does a knockdown, too
				vec3_t pushDir;
				vec3_t angs;
				VectorCopy( NPC->client->ps.viewangles, angs );
				angs[YAW] += flrand( 25, 50 );
				angs[PITCH] = flrand( -25, -15 );
				AngleVectors( angs, pushDir, NULL, NULL );
				//[KnockdownSys]
				//ported multi-direction knockdowns from SP.
				G_Knockdown( radiusEnt, NPC, pushDir, 35, qtrue );
				//G_Knockdown( radiusEnt );
				//[/KnockdownSys]
			}
			G_Sound( radiusEnt, CHAN_WEAPON, G_SoundIndex( "sound/chars/rancor/swipehit.wav" ) );
		}
	}
}

//------------------------------
void Wampa_Attack( float distance, qboolean doCharge )
{
	if ( !TIMER_Exists( NPC, "attacking" ) )
	{
		if ( Q_irand(0, 2) && !doCharge )
		{//double slash
			NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_ATTACK1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			TIMER_Set( NPC, "attack_dmg", 750 );
		}
		else if ( doCharge || (distance > 270 && distance < 430 && !Q_irand(0, 1)) )
		{//leap
			vec3_t	fwd, yawAng;
			VectorSet( yawAng, 0, NPC->client->ps.viewangles[YAW], 0 );
			NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_ATTACK2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			TIMER_Set( NPC, "attack_dmg", 500 );
			AngleVectors( yawAng, fwd, NULL, NULL );
			VectorScale( fwd, distance*1.5f, NPC->client->ps.velocity );
			NPC->client->ps.velocity[2] = 150;
			NPC->client->ps.groundEntityNum = ENTITYNUM_NONE;
		}
		else
		{//backhand
			NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_ATTACK3, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			TIMER_Set( NPC, "attack_dmg", 250 );
		}

		TIMER_Set( NPC, "attacking", NPC->client->ps.legsTimer + random() * 200 );
		//allow us to re-evaluate our running speed/anim
		TIMER_Set( NPC, "runfar", -1 );
		TIMER_Set( NPC, "runclose", -1 );
		TIMER_Set( NPC, "walk", -1 );
	}

	// Need to do delayed damage since the attack animations encapsulate multiple mini-attacks

	if ( TIMER_Done2( NPC, "attack_dmg", qtrue ) )
	{
		switch ( NPC->client->ps.legsAnim )
		{
		case BOTH_ATTACK1:
				// Occasionally turn a close-range swipe into a grab.
				if ( NPC->count == 0 && enemyDist < 96 && !Q_irand( 0, 2 ) && Wampa_TryGrab() )
				{
					// Cancel follow-up hit; the hold logic will take over.
					TIMER_Remove( NPC, "attack_dmg2" );
					return;
				}
			Wampa_Slash( NPC->client->renderInfo.handRBolt, qfalse );
			//do second hit
			TIMER_Set( NPC, "attack_dmg2", 100 );
			break;
		case BOTH_ATTACK2:
				if ( NPC->count == 0 && enemyDist < 96 && !Q_irand( 0, 2 ) && Wampa_TryGrab() )
				{
					TIMER_Remove( NPC, "attack_dmg2" );
					return;
				}
			Wampa_Slash( NPC->client->renderInfo.handRBolt, qfalse );
			TIMER_Set( NPC, "attack_dmg2", 100 );
			break;
		case BOTH_ATTACK3:
			Wampa_Slash( NPC->client->renderInfo.handLBolt, qtrue );
			break;
		}
	}
	else if ( TIMER_Done2( NPC, "attack_dmg2", qtrue ) )
	{
		switch ( NPC->client->ps.legsAnim )
		{
		case BOTH_ATTACK1:
			Wampa_Slash( NPC->client->renderInfo.handLBolt, qfalse );
			break;
		case BOTH_ATTACK2:
			Wampa_Slash( NPC->client->renderInfo.handLBolt, qfalse );
			break;
		}
	}

	// Just using this to remove the attacking flag at the right time
	TIMER_Done2( NPC, "attacking", qtrue );

	if ( NPC->client->ps.legsAnim == BOTH_ATTACK1 && distance > (NPC->r.maxs[0]+MIN_DISTANCE) )
	{//okay to keep moving
		ucmd.buttons |= BUTTON_WALKING;
		Wampa_Move( 1 );
	}
}

//----------------------------------
void Wampa_Combat( void )
{
	// If we're holding a victim, don't do normal facing/movement logic.
	if ( NPC->count )
	{
		Wampa_HoldVictim();
		return;
	}

	// If we cannot see our target or we have somewhere to go, then do that
	if ( !NPC_ClearLOS( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin ) )
	{
		if ( !Q_irand( 0, 10 ) )
		{
			if ( Wampa_CheckRoar( NPC ) )
			{
				return;
			}
		}
		NPCInfo->combatMove = qtrue;
		NPCInfo->goalEntity = NPC->enemy;
		NPCInfo->goalRadius = MAX_DISTANCE;	// just get us within combat range

		Wampa_Move( 0 );
		return;
	}
	else if ( UpdateGoal() )
	{
		NPCInfo->combatMove = qtrue;
		NPCInfo->goalEntity = NPC->enemy;
		NPCInfo->goalRadius = MAX_DISTANCE;	// just get us within combat range

		Wampa_Move( 1 );
		return;
	}
	else
	{
		float		distance = enemyDist = Distance( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin );	
		qboolean	advance = (qboolean)( distance > (NPC->r.maxs[0]+MIN_DISTANCE) ? qtrue : qfalse  );
		qboolean	doCharge = qfalse;

		// Sometimes I have problems with facing the enemy I'm attacking, so force the issue so I don't look dumb
		//FIXME: always seems to face off to the left or right?!!!!
		NPC_FaceEnemy( qtrue );


		if ( advance )
		{//have to get closer
			vec3_t	yawOnlyAngles;
			VectorSet( yawOnlyAngles, 0, NPC->r.currentAngles[YAW], 0 );
			if ( NPC->enemy->health > 0//enemy still alive
				&& fabs(distance-350) <= 80 //enemy anywhere from 270 to 430 away
				&& InFOV3( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, yawOnlyAngles, 20, 20 ) )//enemy generally in front
			{//10% chance of doing charge anim
				if ( !Q_irand( 0, 9 ) )
				{//go for the charge
					doCharge = qtrue;
					advance = qfalse;
				}
			}
		}

		if (( advance || NPCInfo->localState == LSTATE_WAITING ) && TIMER_Done( NPC, "attacking" )) // waiting monsters can't attack
		{
			if ( TIMER_Done2( NPC, "takingPain", qtrue ))
			{
				NPCInfo->localState = LSTATE_CLEAR;
			}
			else
			{
				Wampa_Move( 1 );
			}
		}
		else
		{
			if ( !Q_irand( 0, 20 ) )
			{//FIXME: only do this if we just damaged them or vice-versa?
				if ( Wampa_CheckRoar( NPC ) )
				{
					return;
				}
			}
			if ( !Q_irand( 0, 1 ) )
			{//FIXME: base on skill
				Wampa_Attack( distance, doCharge );
			}
		}
	}
}

/*
-------------------------
NPC_Wampa_Pain
-------------------------
*/
//void NPC_Wampa_Pain( gentity_t *self, gentity_t *inflictor, gentity_t *other, const vec3_t point, int damage, int mod,int hitLoc ) 
void NPC_Wampa_Pain( gentity_t *self, gentity_t *attacker, int damage ) 
{
	// If we're holding someone, pain forces us to release them (prevents stuck victims).
	if ( self->count && self->activator )
	{
		Wampa_DropVictim( self );
	}

	qboolean hitByWampa = qfalse;
	if ( attacker&&attacker->client&&attacker->client->NPC_class==CLASS_WAMPA )
	{
		hitByWampa = qtrue;
	}
	if ( attacker 
		&& attacker->inuse 
		&& attacker != self->enemy
		&& !(attacker->flags&FL_NOTARGET) )
	{
		if ( (!attacker->s.number&&!Q_irand(0,3))
			|| !self->enemy
			|| self->enemy->health == 0
			|| (self->enemy->client&&self->enemy->client->NPC_class == CLASS_WAMPA)
			|| (!Q_irand(0, 4 ) && DistanceSquared( attacker->r.currentOrigin, self->r.currentOrigin ) < DistanceSquared( self->enemy->r.currentOrigin, self->r.currentOrigin )) ) 
		{//if my enemy is dead (or attacked by player) and I'm not still holding/eating someone, turn on the attacker
			//FIXME: if can't nav to my enemy, take this guy if I can nav to him
			G_SetEnemy( self, attacker );
			TIMER_Set( self, "lookForNewEnemy", Q_irand( 5000, 15000 ) );
			if ( hitByWampa )
			{//stay mad at this Wampa for 2-5 secs before looking for attacker enemies
				TIMER_Set( self, "wampaInfight", Q_irand( 2000, 5000 ) );
			}
		}
	}
	if ( (hitByWampa|| Q_irand( 0, 100 ) < damage )//hit by wampa, hit while holding live victim, or took a lot of damage
		&& self->client->ps.legsAnim != BOTH_GESTURE1
		&& self->client->ps.legsAnim != BOTH_GESTURE2
		&& TIMER_Done( self, "takingPain" ) )
	{
		if ( !Wampa_CheckRoar( self ) )
		{
			if ( self->client->ps.legsAnim != BOTH_ATTACK1
				&& self->client->ps.legsAnim != BOTH_ATTACK2
				&& self->client->ps.legsAnim != BOTH_ATTACK3 )
			{//cant interrupt one of the big attack anims
				if ( self->health > 100 || hitByWampa )
				{
					TIMER_Remove( self, "attacking" );

					VectorCopy( self->NPC->lastPathAngles, self->s.angles );

					if ( !Q_irand( 0, 1 ) )
					{
						NPC_SetAnim(self, SETANIM_BOTH, BOTH_PAIN2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						NPC_SetAnim(self, SETANIM_BOTH, BOTH_PAIN1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					TIMER_Set( self, "takingPain", self->client->ps.legsTimer+Q_irand(0, 500) );
					//allow us to re-evaluate our running speed/anim
					TIMER_Set( self, "runfar", -1 );
					TIMER_Set( self, "runclose", -1 );
					TIMER_Set( self, "walk", -1 );

					if ( self->NPC )
					{
						self->NPC->localState = LSTATE_WAITING;
					}
				}
			}
		}
	}
}

/*
-------------------------
NPC_BSWampa_Default
-------------------------
*/
void NPC_BSWampa_Default( void )
{
	NPC->client->ps.eFlags2 &= ~EF2_USE_ALT_ANIM;
	//NORMAL ANIMS
	//	stand1 = normal stand
	//	walk1 = normal, non-angry walk
	//	walk2 = injured
	//	run1 = far away run
	//	run2 = close run
	//VICTIM ANIMS
	//	grabswipe = melee1 - sweep out and grab
	//	stand2 attack = attack4 - while holding victim, swipe at him
	//	walk3_drag = walk5 - walk with drag
	//	stand2 = hold victim
	//	stand2to1 = drop victim
	if ( !TIMER_Done( NPC, "rageTime" ) )
	{//do nothing but roar first time we see an enemy
		NPC_FaceEnemy( qtrue );
		return;
	}
	if ( NPC->enemy )
	{
		if ( !TIMER_Done(NPC,"attacking") )
		{//in middle of attack
			//face enemy
			NPC_FaceEnemy( qtrue );
			//continue attack logic
			enemyDist = Distance( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin );
			Wampa_Attack( enemyDist, qfalse );
			return;
		}
		else
		{
			if ( TIMER_Done(NPC,"angrynoise") )
			{
				G_Sound( NPC, CHAN_VOICE, G_SoundIndex( va("sound/chars/wampa/misc/anger%d.wav", Q_irand(1, 2)) ) );

				TIMER_Set( NPC, "angrynoise", Q_irand( 5000, 10000 ) );
			}
			//else, if he's in our hand, we eat, else if he's on the ground, we keep attacking his dead body for a while
			if( NPC->enemy->client && NPC->enemy->client->NPC_class == CLASS_WAMPA )
			{//got mad at another Wampa, look for a valid enemy
				if ( TIMER_Done( NPC, "wampaInfight" ) )
				{
					NPC_CheckEnemyExt( qtrue );
				}
			}
			else
			{
				if ( ValidEnemy( NPC->enemy ) == qfalse )
				{
					TIMER_Remove( NPC, "lookForNewEnemy" );//make them look again right now
					if ( !NPC->enemy->inuse || level.time - NPC->enemy->s.time > Q_irand( 10000, 15000 ) )
					{//it's been a while since the enemy died, or enemy is completely gone, get bored with him
						NPC->enemy = NULL;
						Wampa_Patrol();
						NPC_UpdateAngles( qtrue, qtrue );
						//just lost my enemy
						if ( (NPC->spawnflags&2) )
						{//search around me if I don't have an enemy
							NPC_BSSearchStart( NPC->waypoint, BS_SEARCH );
							NPCInfo->tempBehavior = BS_DEFAULT;
						}
						else if ( (NPC->spawnflags&1) )
						{//wander if I don't have an enemy
							NPC_BSSearchStart( NPC->waypoint, BS_WANDER );
							NPCInfo->tempBehavior = BS_DEFAULT;
						}
						return;
					}
				}
				if ( TIMER_Done( NPC, "lookForNewEnemy" ) )
				{
					gentity_t *newEnemy, *sav_enemy = NPC->enemy;//FIXME: what about NPC->lastEnemy?
					NPC->enemy = NULL;
					newEnemy = NPC_CheckEnemy( NPCInfo->confusionTime < level.time, qfalse, qfalse );
					NPC->enemy = sav_enemy;
					if ( newEnemy && newEnemy != sav_enemy )
					{//picked up a new enemy!
						NPC->lastEnemy = NPC->enemy;
						G_SetEnemy( NPC, newEnemy );
						//hold this one for at least 5-15 seconds
						TIMER_Set( NPC, "lookForNewEnemy", Q_irand( 5000, 15000 ) );
					}
					else
					{//look again in 2-5 secs
						TIMER_Set( NPC, "lookForNewEnemy", Q_irand( 2000, 5000 ) );
					}
				}
			}
			Wampa_Combat();
			return;
		}
	}
	else 
	{
		if ( TIMER_Done(NPC,"idlenoise") )
		{
			G_Sound( NPC, CHAN_AUTO, G_SoundIndex( "sound/chars/wampa/misc/anger3.wav" ) );

			TIMER_Set( NPC, "idlenoise", Q_irand( 2000, 4000 ) );
		}
		if ( (NPC->spawnflags&2) )
		{//search around me if I don't have an enemy
			if ( NPCInfo->homeWp == WAYPOINT_NONE )
			{//no homewap, initialize the search behavior
				NPC_BSSearchStart( WAYPOINT_NONE, BS_SEARCH );
				NPCInfo->tempBehavior = BS_DEFAULT;
			}
			ucmd.buttons |= BUTTON_WALKING;
			NPC_BSSearch();//this automatically looks for enemies
		}
		else if ( (NPC->spawnflags&1) )
		{//wander if I don't have an enemy
			if ( NPCInfo->homeWp == WAYPOINT_NONE )
			{//no homewap, initialize the wander behavior
				NPC_BSSearchStart( WAYPOINT_NONE, BS_WANDER );
				NPCInfo->tempBehavior = BS_DEFAULT;
			}
			ucmd.buttons |= BUTTON_WALKING;
			NPC_BSWander();
			if ( NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES )
			{
				if ( NPC_CheckEnemyExt( qtrue ) == qfalse )
				{
					Wampa_Idle();
				}
				else
				{
					Wampa_CheckRoar( NPC );
					TIMER_Set( NPC, "lookForNewEnemy", Q_irand( 5000, 15000 ) );
				}
			}
		}
		else
		{
			if ( NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES )
			{
				Wampa_Patrol();
			}
			else
			{
				Wampa_Idle();
			}
		}
	}

	NPC_UpdateAngles( qtrue, qtrue );
}
