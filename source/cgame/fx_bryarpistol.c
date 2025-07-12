// Bryar Pistol Weapon Effects

#include "cg_local.h"
#include "fx_local.h"

/*
-------------------------

	MAIN FIRE

-------------------------
FX_BryarProjectileThink
-------------------------
*/
void FX_BryarProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}
	

	trap_FX_PlayEffectID( cgs.effects.orangeShotEffect, cent->lerpOrigin, forward, -1, -1 );
	
}
void FX_BryarProjectileThink2(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}
	

	trap_FX_PlayEffectID( cgs.effects.greenShotEffect, cent->lerpOrigin, forward, -1, -1 );

}
void FX_BryarProjectileThink3(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}
	

	trap_FX_PlayEffectID( cgs.effects.blueShotEffect, cent->lerpOrigin, forward, -1, -1 );

}
void FX_BryarProjectileThink4(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}
	

	trap_FX_PlayEffectID( cgs.effects.yellowShotEffect, cent->lerpOrigin, forward, -1, -1 );

}
void FX_BryarProjectileThink5(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}
	

	trap_FX_PlayEffectID( cgs.effects.purpleShotEffect, cent->lerpOrigin, forward, -1, -1 );

}
void FX_BryarProjectileThink6(  centity_t *cent, const struct weaponInfo_s *weapon )
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
FX_BryarHitWall
-------------------------
*/
void FX_BryarHitWall( vec3_t origin, vec3_t normal )
{
	trap_FX_PlayEffectID( cgs.effects.blasterWallImpactEffect, origin, normal, -1, -1 );
}

/*
-------------------------
FX_BryarHitPlayer
-------------------------
*/
void FX_BryarHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid )
{

		trap_FX_PlayEffectID( cgs.effects.blasterFleshImpactEffect, origin, normal, -1, -1 );

}


/*
-------------------------

	ALT FIRE

-------------------------
FX_BryarAltProjectileThink
-------------------------
*/
void FX_BryarAltProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;
	int t;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	// see if we have some sort of extra charge going on
	for (t = 1; t < cent->currentState.generic1; t++ )
	{

			// just add ourselves over, and over, and over when we are charged
		trap_FX_PlayEffectID( cgs.effects.orangeShotEffect, cent->lerpOrigin, forward, -1, -1 );
	

	}

	//	for ( int t = 1; t < cent->gent->count; t++ )	// The single player stores the charge in count, which isn't accessible on the client


	trap_FX_PlayEffectID( cgs.effects.orangeShotEffect, cent->lerpOrigin, forward, -1, -1 );
	
}
void FX_BryarAltProjectileThink2(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;
	int t;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	// see if we have some sort of extra charge going on
	for (t = 1; t < cent->currentState.generic1; t++ )
	{

			// just add ourselves over, and over, and over when we are charged
		trap_FX_PlayEffectID( cgs.effects.greenShotEffect, cent->lerpOrigin, forward, -1, -1 );


	}

	//	for ( int t = 1; t < cent->gent->count; t++ )	// The single player stores the charge in count, which isn't accessible on the client


	trap_FX_PlayEffectID( cgs.effects.greenShotEffect, cent->lerpOrigin, forward, -1, -1 );

}
void FX_BryarAltProjectileThink3(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;
	int t;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	// see if we have some sort of extra charge going on
	for (t = 1; t < cent->currentState.generic1; t++ )
	{

			// just add ourselves over, and over, and over when we are charged
		trap_FX_PlayEffectID( cgs.effects.blueShotEffect, cent->lerpOrigin, forward, -1, -1 );


	}

	//	for ( int t = 1; t < cent->gent->count; t++ )	// The single player stores the charge in count, which isn't accessible on the client


	trap_FX_PlayEffectID( cgs.effects.blueShotEffect, cent->lerpOrigin, forward, -1, -1 );

}
void FX_BryarAltProjectileThink4(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;
	int t;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	// see if we have some sort of extra charge going on
	for (t = 1; t < cent->currentState.generic1; t++ )
	{

			// just add ourselves over, and over, and over when we are charged
		trap_FX_PlayEffectID( cgs.effects.yellowShotEffect, cent->lerpOrigin, forward, -1, -1 );


	}

	//	for ( int t = 1; t < cent->gent->count; t++ )	// The single player stores the charge in count, which isn't accessible on the client


	trap_FX_PlayEffectID( cgs.effects.yellowShotEffect, cent->lerpOrigin, forward, -1, -1 );

}
void FX_BryarAltProjectileThink5(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;
	int t;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	// see if we have some sort of extra charge going on
	for (t = 1; t < cent->currentState.generic1; t++ )
	{

			// just add ourselves over, and over, and over when we are charged
		trap_FX_PlayEffectID( cgs.effects.purpleShotEffect, cent->lerpOrigin, forward, -1, -1 );


	}

	//	for ( int t = 1; t < cent->gent->count; t++ )	// The single player stores the charge in count, which isn't accessible on the client


	trap_FX_PlayEffectID( cgs.effects.purpleShotEffect, cent->lerpOrigin, forward, -1, -1 );

}
void FX_BryarAltProjectileThink6(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;
	int t;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	// see if we have some sort of extra charge going on
	for (t = 1; t < cent->currentState.generic1; t++ )
	{

			// just add ourselves over, and over, and over when we are charged
		trap_FX_PlayEffectID( cgs.effects.redShotEffect, cent->lerpOrigin, forward, -1, -1 );


	}

	//	for ( int t = 1; t < cent->gent->count; t++ )	// The single player stores the charge in count, which isn't accessible on the client


	trap_FX_PlayEffectID( cgs.effects.redShotEffect, cent->lerpOrigin, forward, -1, -1 );

}
/*
-------------------------
FX_BryarAltHitWall
-------------------------
*/
void FX_BryarAltHitWall( vec3_t origin, vec3_t normal, int power )
{

		trap_FX_PlayEffectID( cgs.effects.blasterWallImpactEffect, origin, normal, -1, -1 );

	
}

/*
-------------------------
FX_BryarAltHitPlayer
-------------------------
*/
void FX_BryarAltHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid )
{

		trap_FX_PlayEffectID( cgs.effects.blasterFleshImpactEffect, origin, normal, -1, -1 );

}

/*
-------------------------

	MAIN FIRE

-------------------------
FX_BryarOldProjectileThink
-------------------------
*/
void FX_BryarOldProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap_FX_PlayEffectID( cgs.effects.greenShotEffect, cent->lerpOrigin, forward, -1, -1 );
	

}
void FX_BryarOldProjectileThink2(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap_FX_PlayEffectID( cgs.effects.orangeShotEffect, cent->lerpOrigin, forward, -1, -1 );


}
void FX_BryarOldProjectileThink3(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap_FX_PlayEffectID( cgs.effects.redShotEffect, cent->lerpOrigin, forward, -1, -1 );


}
void FX_BryarOldProjectileThink4(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap_FX_PlayEffectID( cgs.effects.redShotEffect, cent->lerpOrigin, forward, -1, -1 );


}
void FX_BryarOldProjectileThink5(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap_FX_PlayEffectID( cgs.effects.whiteShotEffect, cent->lerpOrigin, forward, -1, -1 );


}
void FX_BryarOldProjectileThink6(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap_FX_PlayEffectID( cgs.effects.orangeShotEffect, cent->lerpOrigin, forward, -1, -1 );


}

/*
-------------------------
FX_BryarOldHitWall
-------------------------
*/
void FX_BryarOldHitWall( vec3_t origin, vec3_t normal )
{
	trap_FX_PlayEffectID( cgs.effects.blasterWallImpactEffect, origin, normal, -1, -1 );
}

/*
-------------------------
FX_BryarOldHitPlayer
-------------------------
*/
void FX_BryarOldHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid )
{

		trap_FX_PlayEffectID( cgs.effects.blasterFleshImpactEffect, origin, normal, -1, -1 );

}


/*
-------------------------

	ALT FIRE

-------------------------
FX_BryarOldAltProjectileThink
-------------------------
*/
void FX_BryarOldAltProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;
	int t;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	// see if we have some sort of extra charge going on
	for (t = 1; t < cent->currentState.generic1; t++ )
	{
		// just add ourselves over, and over, and over when we are charged

	
		trap_FX_PlayEffectID( cgs.effects.greenShotEffect, cent->lerpOrigin, forward, -1, -1 );
	

	}

	//	for ( int t = 1; t < cent->gent->count; t++ )	// The single player stores the charge in count, which isn't accessible on the client


	trap_FX_PlayEffectID( cgs.effects.greenShotEffect, cent->lerpOrigin, forward, -1, -1 );
	
}
void FX_BryarOldAltProjectileThink2(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;
	int t;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	// see if we have some sort of extra charge going on
	for (t = 1; t < cent->currentState.generic1; t++ )
	{
		// just add ourselves over, and over, and over when we are charged

		trap_FX_PlayEffectID( cgs.effects.orangeShotEffect, cent->lerpOrigin, forward, -1, -1 );


	}

	//	for ( int t = 1; t < cent->gent->count; t++ )	// The single player stores the charge in count, which isn't accessible on the client


	trap_FX_PlayEffectID( cgs.effects.orangeShotEffect, cent->lerpOrigin, forward, -1, -1 );

}
void FX_BryarOldAltProjectileThink3(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;
	int t;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	// see if we have some sort of extra charge going on
	for (t = 1; t < cent->currentState.generic1; t++ )
	{
		// just add ourselves over, and over, and over when we are charged

		trap_FX_PlayEffectID( cgs.effects.redShotEffect, cent->lerpOrigin, forward, -1, -1 );


	}

	//	for ( int t = 1; t < cent->gent->count; t++ )	// The single player stores the charge in count, which isn't accessible on the client


	trap_FX_PlayEffectID( cgs.effects.redShotEffect, cent->lerpOrigin, forward, -1, -1 );

}
void FX_BryarOldAltProjectileThink4(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;
	int t;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	// see if we have some sort of extra charge going on
	for (t = 1; t < cent->currentState.generic1; t++ )
	{
		// just add ourselves over, and over, and over when we are charged

		trap_FX_PlayEffectID( cgs.effects.redShotEffect, cent->lerpOrigin, forward, -1, -1 );


	}

	//	for ( int t = 1; t < cent->gent->count; t++ )	// The single player stores the charge in count, which isn't accessible on the client


	trap_FX_PlayEffectID( cgs.effects.redShotEffect, cent->lerpOrigin, forward, -1, -1 );

}
void FX_BryarOldAltProjectileThink5(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;
	int t;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	// see if we have some sort of extra charge going on
	for (t = 1; t < cent->currentState.generic1; t++ )
	{
		// just add ourselves over, and over, and over when we are charged

		trap_FX_PlayEffectID( cgs.effects.whiteShotEffect, cent->lerpOrigin, forward, -1, -1 );


	}

	//	for ( int t = 1; t < cent->gent->count; t++ )	// The single player stores the charge in count, which isn't accessible on the client


	trap_FX_PlayEffectID( cgs.effects.whiteShotEffect, cent->lerpOrigin, forward, -1, -1 );

}
void FX_BryarOldAltProjectileThink6(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;
	int t;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	// see if we have some sort of extra charge going on
	for (t = 1; t < cent->currentState.generic1; t++ )
	{
		// just add ourselves over, and over, and over when we are charged

		trap_FX_PlayEffectID( cgs.effects.orangeShotEffect, cent->lerpOrigin, forward, -1, -1 );


	}

	//	for ( int t = 1; t < cent->gent->count; t++ )	// The single player stores the charge in count, which isn't accessible on the client


	trap_FX_PlayEffectID( cgs.effects.orangeShotEffect, cent->lerpOrigin, forward, -1, -1 );

}
/*
-------------------------
FX_BryarOldAltHitWall
-------------------------
*/
void FX_BryarOldAltHitWall( vec3_t origin, vec3_t normal, int power )
{

		trap_FX_PlayEffectID( cgs.effects.blasterWallImpactEffect, origin, normal, -1, -1 );

}

/*
-------------------------
FX_BryarOldAltHitPlayer
-------------------------
*/
void FX_BryarOldAltHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid )
{

		trap_FX_PlayEffectID( cgs.effects.blasterFleshImpactEffect, origin, normal, -1, -1 );

}
//TURRET
/*
-------------------------
FX_TurretProjectileThink
-------------------------
*/
void FX_TurretProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap_FX_PlayEffectID( cgs.effects.turretShotEffect, cent->lerpOrigin, forward, -1, -1 );
}

/*
-------------------------
FX_TurretHitWall
-------------------------
*/
void FX_TurretHitWall( vec3_t origin, vec3_t normal )
{
	trap_FX_PlayEffectID( cgs.effects.blasterWallImpactEffect, origin, normal, -1, -1 );
}

/*
-------------------------
FX_TurretHitPlayer
-------------------------
*/
void FX_TurretHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid )
{

		trap_FX_PlayEffectID( cgs.effects.blasterFleshImpactEffect, origin, normal, -1, -1 );

}



//CONCUSSION (yeah, should probably make a new file for this.. or maybe just move all these stupid semi-redundant fx_ functions into one file)
/*
-------------------------
FX_ConcussionHitWall
-------------------------
*/
void FX_ConcussionHitWall( vec3_t origin, vec3_t normal )
{
	trap_FX_PlayEffectID( cgs.effects.concussionImpactEffect, origin, normal, -1, -1 );		
}
void FX_ConcussionHitWall2( vec3_t origin, vec3_t normal )
{
	trap_FX_PlayEffectID( cgs.effects.concussionImpactEffect2, origin, normal, -1, -1 );		
}
void FX_ConcussionHitWall3( vec3_t origin, vec3_t normal )
{	
	trap_FX_PlayEffectID( cgs.effects.concussionImpactEffect3, origin, normal, -1, -1 );	
}
void FX_ConcussionHitWall4( vec3_t origin, vec3_t normal )
{
	trap_FX_PlayEffectID( cgs.effects.concussionImpactEffect4, origin, normal, -1, -1 );	
}
void FX_ConcussionHitWall5( vec3_t origin, vec3_t normal )
{
	trap_FX_PlayEffectID( cgs.effects.repeaterWallImpactEffect, origin, normal, -1, -1 );		
}
void FX_ConcussionHitWall6( vec3_t origin, vec3_t normal )
{
	trap_FX_PlayEffectID( cgs.effects.repeaterWallImpactEffect, origin, normal, -1, -1 );		
}
/*
-------------------------
FX_ConcussionHitPlayer
-------------------------
*/
void FX_ConcussionHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid )
{
	trap_FX_PlayEffectID( cgs.effects.concussionImpactEffect, origin, normal, -1, -1 );	
}
void FX_ConcussionHitPlayer2( vec3_t origin, vec3_t normal, qboolean humanoid )
{
	trap_FX_PlayEffectID( cgs.effects.concussionImpactEffect2, origin, normal, -1, -1 );		
}
void FX_ConcussionHitPlayer3( vec3_t origin, vec3_t normal, qboolean humanoid )
{
	trap_FX_PlayEffectID( cgs.effects.concussionImpactEffect3, origin, normal, -1, -1 );
}
void FX_ConcussionHitPlayer4( vec3_t origin, vec3_t normal, qboolean humanoid )
{
	trap_FX_PlayEffectID( cgs.effects.concussionImpactEffect4, origin, normal, -1, -1 );
}
void FX_ConcussionHitPlayer5( vec3_t origin, vec3_t normal, qboolean humanoid )
{
	trap_FX_PlayEffectID( cgs.effects.repeaterWallImpactEffect, origin, normal, -1, -1 );	
}
void FX_ConcussionHitPlayer6( vec3_t origin, vec3_t normal, qboolean humanoid )
{
	trap_FX_PlayEffectID( cgs.effects.repeaterWallImpactEffect, origin, normal, -1, -1 );	
}
/*
-------------------------
FX_ConcussionProjectileThink
-------------------------
*/
void FX_ConcussionProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}
	

	trap_FX_PlayEffectID( cgs.effects.concussionShotEffect, cent->lerpOrigin, forward, -1, -1 );
	
	
}
void FX_ConcussionProjectileThink2(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}
	

	trap_FX_PlayEffectID( cgs.effects.concussionShotEffect2, cent->lerpOrigin, forward, -1, -1 );

	
}
void FX_ConcussionProjectileThink3(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}
	

	trap_FX_PlayEffectID( cgs.effects.concussionShotEffect3, cent->lerpOrigin, forward, -1, -1 );

	
}
void FX_ConcussionProjectileThink4(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}
	

	trap_FX_PlayEffectID( cgs.effects.concussionShotEffect4, cent->lerpOrigin, forward, -1, -1 );

	
}
void FX_ConcussionProjectileThink5(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}
	

	trap_FX_PlayEffectID( cgs.effects.orangeShotEffect, cent->lerpOrigin, forward, -1, -1 );

	
}
void FX_ConcussionProjectileThink6(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}
	

	trap_FX_PlayEffectID( cgs.effects.repeaterProjectileEffect, cent->lerpOrigin, forward, -1, -1 );

	
}
/*
---------------------------
FX_ConcAltShot
---------------------------
*/
static vec3_t WHITE	={1.0f,1.0f,1.0f};
static vec3_t BRIGHT={0.75f,0.5f,1.0f};

void FX_ConcAltShot( vec3_t start, vec3_t end )
{
	//"concussion/beam"
	trap_FX_AddLine( start, end, 0.1f, 10.0f, 0.0f, 
							1.0f, 0.0f, 0.0f,
							WHITE, WHITE, 0.0f,
							175, trap_R_RegisterShader( "gfx/effects/blueLine" ), 
							FX_SIZE_LINEAR | FX_ALPHA_LINEAR );

	// add some beef
	trap_FX_AddLine( start, end, 0.1f, 7.0f, 0.0f, 
						1.0f, 0.0f, 0.0f,
						BRIGHT, BRIGHT, 0.0f,
						150, trap_R_RegisterShader( "gfx/misc/whiteline2" ), 
						FX_SIZE_LINEAR | FX_ALPHA_LINEAR );
}



//DESTRUCTION (yeah, should probably make a new file for this.. or maybe just move all these stupid semi-redundant fx_ functions into one file)
/*
-------------------------
FX_DestructionHitWall
-------------------------
*/
void FX_DestructionHitWall( vec3_t origin, vec3_t normal )
{
	trap_FX_PlayEffectID( cgs.effects.concussionImpactEffect2, origin, normal, -1, -1 );
}

/*
-------------------------
FX_DestructionHitPlayer
-------------------------
*/
void FX_DestructionHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid )
{
	trap_FX_PlayEffectID( cgs.effects.concussionImpactEffect2, origin, normal, -1, -1 );
}

/*
-------------------------
FX_DestructionProjectileThink
-------------------------
*/
void FX_DestructionProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap_FX_PlayEffectID( cgs.effects.concussionShotEffect2, cent->lerpOrigin, forward, -1, -1 );
}
