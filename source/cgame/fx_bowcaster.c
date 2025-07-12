// Bowcaster Weapon

#include "cg_local.h"

/*
---------------------------
FX_BowcasterProjectileThink
---------------------------
*/

void FX_BowcasterProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap_FX_PlayEffectID( cgs.effects.greenShotEffect, cent->lerpOrigin, forward, -1, -1 );
	

}
void FX_BowcasterProjectileThink2( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap_FX_PlayEffectID( cgs.effects.redShotEffect, cent->lerpOrigin, forward, -1, -1 );


}
void FX_BowcasterProjectileThink3( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap_FX_PlayEffectID( cgs.effects.blueShotEffect, cent->lerpOrigin, forward, -1, -1 );


}
void FX_BowcasterProjectileThink4( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap_FX_PlayEffectID( cgs.effects.yellowShotEffect, cent->lerpOrigin, forward, -1, -1 );
	

}
void FX_BowcasterProjectileThink5( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap_FX_PlayEffectID( cgs.effects.purpleShotEffect, cent->lerpOrigin, forward, -1, -1 );
	

}
void FX_BowcasterProjectileThink6( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap_FX_PlayEffectID( cgs.effects.orangeShotEffect, cent->lerpOrigin, forward, -1, -1 );
	

}
/*
---------------------------
FX_BowcasterHitWall
---------------------------
*/

void FX_BowcasterHitWall( vec3_t origin, vec3_t normal )
{
	trap_FX_PlayEffectID( cgs.effects.blasterWallImpactEffect, origin, normal, -1, -1 );
}

/*
---------------------------
FX_BowcasterHitPlayer
---------------------------
*/

void FX_BowcasterHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid )
{
	trap_FX_PlayEffectID( cgs.effects.blasterFleshImpactEffect, origin, normal, -1, -1 );
}

/*
------------------------------
FX_BowcasterAltProjectileThink
------------------------------
*/

void FX_BowcasterAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}


	trap_FX_PlayEffectID( cgs.effects.greenShotEffect, cent->lerpOrigin, forward, -1, -1 );
	
}
void FX_BowcasterAltProjectileThink2( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}


	trap_FX_PlayEffectID( cgs.effects.redShotEffect, cent->lerpOrigin, forward, -1, -1 );

}
void FX_BowcasterAltProjectileThink3( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}


	trap_FX_PlayEffectID( cgs.effects.blueShotEffect, cent->lerpOrigin, forward, -1, -1 );

}
void FX_BowcasterAltProjectileThink4( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}


	trap_FX_PlayEffectID( cgs.effects.yellowShotEffect, cent->lerpOrigin, forward, -1, -1 );

}
void FX_BowcasterAltProjectileThink5( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}


	trap_FX_PlayEffectID( cgs.effects.purpleShotEffect, cent->lerpOrigin, forward, -1, -1 );

}
void FX_BowcasterAltProjectileThink6( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}


	trap_FX_PlayEffectID( cgs.effects.orangeShotEffect, cent->lerpOrigin, forward, -1, -1 );

}