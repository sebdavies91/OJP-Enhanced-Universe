// Blaster Weapon

#include "cg_local.h"

/*
-------------------------
FX_BlasterProjectileThink
-------------------------
*/

void FX_BlasterProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}
	

	trap_FX_PlayEffectID( cgs.effects.redShotEffect, cent->lerpOrigin, forward, -1, -1 );
	
	
}
void FX_BlasterProjectileThink2( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}
	

	trap_FX_PlayEffectID( cgs.effects.greenShotEffect, cent->lerpOrigin, forward, -1, -1 );

	
}
void FX_BlasterProjectileThink3( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}
	

	trap_FX_PlayEffectID( cgs.effects.blueShotEffect, cent->lerpOrigin, forward, -1, -1 );

	
}
void FX_BlasterProjectileThink4( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}
	

	trap_FX_PlayEffectID( cgs.effects.orangeShotEffect, cent->lerpOrigin, forward, -1, -1 );

	
}
void FX_BlasterProjectileThink5( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}
	

	trap_FX_PlayEffectID( cgs.effects.purpleShotEffect, cent->lerpOrigin, forward, -1, -1 );

	
}
void FX_BlasterProjectileThink6( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}
	

	trap_FX_PlayEffectID( cgs.effects.redShotEffect, cent->lerpOrigin, forward, -1, -1 );

	
}
/*
-------------------------
FX_BlasterAltFireThink
-------------------------
*/
void FX_BlasterAltFireThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap_FX_PlayEffectID( cgs.effects.redShotEffect, cent->lerpOrigin, forward, -1, -1 );
}

/*
-------------------------
FX_BlasterWeaponHitWall
-------------------------
*/
void FX_BlasterWeaponHitWall( vec3_t origin, vec3_t normal )
{
	trap_FX_PlayEffectID( cgs.effects.blasterWallImpactEffect, origin, normal, -1, -1 );
}

/*
-------------------------
FX_BlasterWeaponHitPlayer
-------------------------
*/
void FX_BlasterWeaponHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid )
{

		trap_FX_PlayEffectID( cgs.effects.blasterFleshImpactEffect, origin, normal, -1, -1 );

}

