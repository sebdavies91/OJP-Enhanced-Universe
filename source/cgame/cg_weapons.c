// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_weapons.c -- events and effects dealing with weapons
#include "cg_local.h"
#include "fx_local.h"

extern vec4_t	bluehudtint;
extern vec4_t	redhudtint;
extern float	*hudTintColor;

/*
Ghoul2 Insert Start
*/
// set up the appropriate ghoul2 info to a refent
void CG_SetGhoul2InfoRef( refEntity_t *ent, refEntity_t	*s1)
{
	ent->ghoul2 = s1->ghoul2;
	VectorCopy( s1->modelScale, ent->modelScale);
	ent->radius = s1->radius;
	VectorCopy( s1->angles, ent->angles);
}

/*
Ghoul2 Insert End
*/
char *showWeaponsName[] = 
{
	"NONE2",//WP_NONE
	"STUN_BATON2",//WP_STUN_BATON
	"MELEE2",//WP_MELEE
	"SABER2",//WP_SABER
	"BRYAR_PISTOL2",//WP_BRYAR_PISTOL
	"BLASTER2",//WP_BLASTER
	"DISRUPTOR2",//WP_DISRUPTOR
	"BOWCASTER2",//WP_BOWCASTER
	"REPEATER2",//WP_REPEATER
	"DEMP22",//WP_DEMP2
	"FLECHETTE2",//WP_FLECHETTE
	"ROCKET_LAUNCHER2",//WP_ROCKET_LAUNCHER
	"THERMAL2",//WP_THERMAL
	"TRIP_MINE2",//WP_TRIP_MINE
	"DET_PACK2",//WP_DET_PACK
	"CONCUSSION2",//WP_CONCUSSION
	"BRYAR_OLD2",//WP_BRYAR_OLD
	"EMPLACED_GUN2",//WP_EMPLACED_GUN
	"TURRET2",//WT_TURRET
	NULL
};

char *showWeaponsName2[] = 
{
	"NONE3",//WP_NONE
	"STUN_BATON3",//WP_STUN_BATON
	"MELEE3",//WP_MELEE
	"SABER3",//WP_SABER
	"BRYAR_PISTOL3",//WP_BRYAR_PISTOL
	"BLASTER3",//WP_BLASTER
	"DISRUPTOR3",//WP_DISRUPTOR
	"BOWCASTER3",//WP_BOWCASTER
	"REPEATER3",//WP_REPEATER
	"DEMP23",//WP_DEMP2
	"FLECHETTE3",//WP_FLECHETTE
	"ROCKET_LAUNCHER3",//WP_ROCKET_LAUNCHER
	"THERMAL3",//WP_THERMAL
	"TRIP_MINE3",//WP_TRIP_MINE
	"DET_PACK3",//WP_DET_PACK
	"CONCUSSION3",//WP_CONCUSSION
	"BRYAR_OLD3",//WP_BRYAR_OLD
	"EMPLACED_GUN3",//WP_EMPLACED_GUN
	"TURRET3",//WT_TURRET
	NULL
};

char *showWeaponsName3[] = 
{
	"NONE4",//WP_NONE
	"STUN_BATON4",//WP_STUN_BATON
	"MELEE4",//WP_MELEE
	"SABER4",//WP_SABER
	"BRYAR_PISTOL4",//WP_BRYAR_PISTOL
	"BLASTER4",//WP_BLASTER
	"DISRUPTOR4",//WP_DISRUPTOR
	"BOWCASTER4",//WP_BOWCASTER
	"REPEATER4",//WP_REPEATER
	"DEMP24",//WP_DEMP2
	"FLECHETTE4",//WP_FLECHETTE
	"ROCKET_LAUNCHER4",//WP_ROCKET_LAUNCHER
	"THERMAL4",//WP_THERMAL
	"TRIP_MINE4",//WP_TRIP_MINE
	"DET_PACK4",//WP_DET_PACK
	"CONCUSSION4",//WP_CONCUSSION
	"BRYAR_OLD4",//WP_BRYAR_OLD
	"EMPLACED_GUN4",//WP_EMPLACED_GUN
	"TURRET4",//WT_TURRET
	NULL
};

char *showWeaponsName4[] = 
{
	"NONE5",//WP_NONE
	"STUN_BATON5",//WP_STUN_BATON
	"MELEE5",//WP_MELEE
	"SABER5",//WP_SABER
	"BRYAR_PISTOL5",//WP_BRYAR_PISTOL
	"BLASTER5",//WP_BLASTER
	"DISRUPTOR5",//WP_DISRUPTOR
	"BOWCASTER5",//WP_BOWCASTER
	"REPEATER5",//WP_REPEATER
	"DEMP25",//WP_DEMP2
	"FLECHETTE5",//WP_FLECHETTE
	"ROCKET_LAUNCHER5",//WP_ROCKET_LAUNCHER
	"THERMAL5",//WP_THERMAL
	"TRIP_MINE5",//WP_TRIP_MINE
	"DET_PACK5",//WP_DET_PACK
	"CONCUSSION5",//WP_CONCUSSION
	"BRYAR_OLD5",//WP_BRYAR_OLD
	"EMPLACED_GUN5",//WP_EMPLACED_GUN
	"TURRET5",//WT_TURRET
	NULL
};

char *showWeaponsName5[] = 
{
	"NONE6",//WP_NONE
	"STUN_BATON6",//WP_STUN_BATON
	"MELEE6",//WP_MELEE
	"SABER6",//WP_SABER
	"BRYAR_PISTOL6",//WP_BRYAR_PISTOL
	"BLASTER6",//WP_BLASTER
	"DISRUPTOR6",//WP_DISRUPTOR
	"BOWCASTER6",//WP_BOWCASTER
	"REPEATER6",//WP_REPEATER
	"DEMP26",//WP_DEMP2
	"FLECHETTE6",//WP_FLECHETTE
	"ROCKET_LAUNCHER6",//WP_ROCKET_LAUNCHER
	"THERMAL6",//WP_THERMAL
	"TRIP_MINE6",//WP_TRIP_MINE
	"DET_PACK6",//WP_DET_PACK
	"CONCUSSION6",//WP_CONCUSSION
	"BRYAR_OLD6",//WP_BRYAR_OLD
	"EMPLACED_GUN6",//WP_EMPLACED_GUN
	"TURRET6",//WT_TURRET
	NULL
};

char *showWeaponsName6[] = 
{
	"NONE7",//WP_NONE
	"STUN_BATON7",//WP_STUN_BATON
	"MELEE7",//WP_MELEE
	"SABER7",//WP_SABER
	"BRYAR_PISTOL7",//WP_BRYAR_PISTOL
	"BLASTER7",//WP_BLASTER
	"DISRUPTOR7",//WP_DISRUPTOR
	"BOWCASTER7",//WP_BOWCASTER
	"REPEATER7",//WP_REPEATER
	"DEMP27",//WP_DEMP2
	"FLECHETTE7",//WP_FLECHETTE
	"ROCKET_LAUNCHER7",//WP_ROCKET_LAUNCHER
	"THERMAL7",//WP_THERMAL
	"TRIP_MINE7",//WP_TRIP_MINE
	"DET_PACK7",//WP_DET_PACK
	"CONCUSSION7",//WP_CONCUSSION
	"BRYAR_OLD7",//WP_BRYAR_OLD
	"EMPLACED_GUN7",//WP_EMPLACED_GUN
	"TURRET7",//WT_TURRET
	NULL
};

/*
=================
CG_RegisterItemVisuals

The server says this item is used on this level
=================
*/
void CG_RegisterItemVisuals( int itemNum ) {
	itemInfo_t		*itemInfo;
	gitem_t			*item;
	int				handle;

	if ( itemNum < 0 || itemNum >= bg_numItems ) {
		CG_Error( "CG_RegisterItemVisuals: itemNum %d out of range [0-%d]", itemNum, bg_numItems-1 );
	}

	itemInfo = &cg_items[ itemNum ];
	if ( itemInfo->registered ) {
		return;
	}

	item = &bg_itemlist[ itemNum ];

	memset( itemInfo, 0, sizeof( &itemInfo ) );
	itemInfo->registered = qtrue;

	if (item->giType == IT_TEAM &&
		(item->giTag == PW_REDFLAG || item->giTag == PW_BLUEFLAG) &&
		cgs.gametype == GT_CTY)
	{ //in CTY the flag model is different
		itemInfo->models[0] = trap_R_RegisterModel( item->world_model[1] );
	}
	else if (item->giType == IT_WEAPON &&
		(item->giTag == WP_THERMAL || item->giTag == WP_TRIP_MINE || item->giTag == WP_DET_PACK))
	{
		itemInfo->models[0] = trap_R_RegisterModel( item->world_model[1] );
	}
	else
	{
		itemInfo->models[0] = trap_R_RegisterModel( item->world_model[0] );
	}
/*
Ghoul2 Insert Start
*/
	if (!Q_stricmp(&item->world_model[0][strlen(item->world_model[0]) - 4], ".glm"))
	{
		handle = trap_G2API_InitGhoul2Model(&itemInfo->g2Models[0], item->world_model[0], 0 , 0, 0, 0, 0);
		if (handle<0)
		{
			itemInfo->g2Models[0] = NULL;
		}
		else
		{
			itemInfo->radius[0] = 60;
		}
	}
/*
Ghoul2 Insert End
*/
	if (item->icon)
	{
		if (item->giType == IT_HEALTH)
		{ //medpack gets nomip'd by the ui or something I guess.
			itemInfo->icon = trap_R_RegisterShaderNoMip( item->icon );
		}
		else
		{
			itemInfo->icon = trap_R_RegisterShader( item->icon );
		}
	}
	else
	{
		itemInfo->icon = 0;
	}

	if ( item->giType == IT_WEAPON ) {
		CG_RegisterWeapon( item->giTag );
	}

	//
	// powerups have an accompanying ring or sphere
	//
	if ( item->giType == IT_POWERUP || item->giType == IT_HEALTH || 
		item->giType == IT_ARMOR || item->giType == IT_HOLDABLE ) {
		if ( item->world_model[1] ) {
			itemInfo->models[1] = trap_R_RegisterModel( item->world_model[1] );
		}
	}
}


/*
========================================================================================

VIEW WEAPON

========================================================================================
*/

#define WEAPON_FORCE_BUSY_HOLSTER

#ifdef WEAPON_FORCE_BUSY_HOLSTER
//rww - this was done as a last resort. Forgive me.
static int cgWeapFrame = 0;
static int cgWeapFrameTime = 0;
#endif

/*
=================
CG_MapTorsoToWeaponFrame

=================
*/
static int CG_MapTorsoToWeaponFrame( clientInfo_t *ci, int frame, int animNum ) {
	animation_t *animations = bgHumanoidAnimations;
#ifdef WEAPON_FORCE_BUSY_HOLSTER
	if (cg.snap->ps.forceHandExtend != HANDEXTEND_NONE || cgWeapFrameTime > cg.time)
	{ //the reason for the after delay is so that it doesn't snap the weapon frame to the "idle" (0) frame
		//for a very quick moment
		if (cgWeapFrame < 6)
		{
			cgWeapFrame = 6;
			cgWeapFrameTime = cg.time + 10;
		}

		if (cgWeapFrameTime < cg.time && cgWeapFrame < 10)
		{
			cgWeapFrame++;
			cgWeapFrameTime = cg.time + 10;
		}

		if (cg.snap->ps.forceHandExtend != HANDEXTEND_NONE &&
			cgWeapFrame == 10)
		{
			cgWeapFrameTime = cg.time + 100;
		}

		return cgWeapFrame;
	}
	else
	{
		cgWeapFrame = 0;
		cgWeapFrameTime = 0;
	}
#endif

	switch( animNum )
	{
	case TORSO_DROPWEAP1:
		if ( frame >= animations[animNum].firstFrame && frame < animations[animNum].firstFrame + 5 ) 
		{
			return frame - animations[animNum].firstFrame + 6;
		}
		break;

	case TORSO_RAISEWEAP1:
		if ( frame >= animations[animNum].firstFrame && frame < animations[animNum].firstFrame + 4 ) 
		{
			return frame - animations[animNum].firstFrame + 6 + 4;
		}
		break;
	case BOTH_ATTACK1:
	case BOTH_ATTACK2:
	case BOTH_ATTACK3:
	case BOTH_ATTACK4:
	case BOTH_ATTACK10:
	case BOTH_THERMAL_THROW:
		if ( frame >= animations[animNum].firstFrame && frame < animations[animNum].firstFrame + 6 ) 
		{
			return 1 + ( frame - animations[animNum].firstFrame );
		}

		break;
	}	
	return -1;
}


/*
==============
CG_CalculateWeaponPosition
==============
*/
static void CG_CalculateWeaponPosition( vec3_t origin, vec3_t angles ) {
	float	scale;
	int		delta;
	float	fracsin;

	VectorCopy( cg.refdef.vieworg, origin );
	VectorCopy( cg.refdef.viewangles, angles );

	// on odd legs, invert some angles
	if ( cg.bobcycle & 1 ) {
		scale = -cg.xyspeed;
	} else {
		scale = cg.xyspeed;
	}

	// gun angles from bobbing
	angles[ROLL] += scale * cg.bobfracsin * 0.005;
	angles[YAW] += scale * cg.bobfracsin * 0.01;
	angles[PITCH] += cg.xyspeed * cg.bobfracsin * 0.005;

	// drop the weapon when landing
	delta = cg.time - cg.landTime;
	if ( delta < LAND_DEFLECT_TIME ) {
		origin[2] += cg.landChange*0.25 * delta / LAND_DEFLECT_TIME;
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		origin[2] += cg.landChange*0.25 * 
			(LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;
	}

#if 0
	// drop the weapon when stair climbing
	delta = cg.time - cg.stepTime;
	if ( delta < STEP_TIME/2 ) {
		origin[2] -= cg.stepChange*0.25 * delta / (STEP_TIME/2);
	} else if ( delta < STEP_TIME ) {
		origin[2] -= cg.stepChange*0.25 * (STEP_TIME - delta) / (STEP_TIME/2);
	}
#endif

	// idle drift
	scale = cg.xyspeed + 40;
	fracsin = sin( cg.time * 0.001 );
	angles[ROLL] += scale * fracsin * 0.01;
	angles[YAW] += scale * fracsin * 0.01;
	angles[PITCH] += scale * fracsin * 0.01;
}


/*
===============
CG_LightningBolt

Origin will be the exact tag point, which is slightly
different than the muzzle point used for determining hits.
The cent should be the non-predicted cent if it is from the player,
so the endpoint will reflect the simulated strike (lagging the predicted
angle)
===============
*/
static void CG_LightningBolt( centity_t *cent, vec3_t origin ) {
//	trace_t  trace;
	refEntity_t  beam;
//	vec3_t   forward;
//	vec3_t   muzzlePoint, endPoint;

	//Must be a durational weapon that continuously generates an effect.
	if ( cent->currentState.weapon == WP_DEMP2 && cent->currentState.eFlags & EF_ALT_FIRING ) 
	{ /*nothing*/ }
	else
	{
		return;
	}

	memset( &beam, 0, sizeof( beam ) );

	// NOTENOTE No lightning gun-ish stuff yet.
/*
	// CPMA  "true" lightning
	if ((cent->currentState.number == cg.predictedPlayerState.clientNum) && (cg_trueLightning.value != 0)) {
		vec3_t angle;
		int i;

		for (i = 0; i < 3; i++) {
			float a = cent->lerpAngles[i] - cg.refdef.viewangles[i];
			if (a > 180) {
				a -= 360;
			}
			if (a < -180) {
				a += 360;
			}

			angle[i] = cg.refdef.viewangles[i] + a * (1.0 - cg_trueLightning.value);
			if (angle[i] < 0) {
				angle[i] += 360;
			}
			if (angle[i] > 360) {
				angle[i] -= 360;
			}
		}

		AngleVectors(angle, forward, NULL, NULL );
		VectorCopy(cent->lerpOrigin, muzzlePoint );
//		VectorCopy(cg.refdef.vieworg, muzzlePoint );
	} else {
		// !CPMA
		AngleVectors( cent->lerpAngles, forward, NULL, NULL );
		VectorCopy(cent->lerpOrigin, muzzlePoint );
	}

	// FIXME: crouch
	muzzlePoint[2] += DEFAULT_VIEWHEIGHT;

	VectorMA( muzzlePoint, 14, forward, muzzlePoint );

	// project forward by the lightning range
	VectorMA( muzzlePoint, LIGHTNING_RANGE, forward, endPoint );

	// see if it hit a wall
	CG_Trace( &trace, muzzlePoint, vec3_origin, vec3_origin, endPoint, 
		cent->currentState.number, MASK_SHOT );

	// this is the endpoint
	VectorCopy( trace.endpos, beam.oldorigin );

	// use the provided origin, even though it may be slightly
	// different than the muzzle origin
	VectorCopy( origin, beam.origin );

	beam.reType = RT_LIGHTNING;
	beam.customShader = cgs.media.lightningShader;
	trap_R_AddRefEntityToScene( &beam );
*/

	// NOTENOTE No lightning gun-ish stuff yet.
/*
	// add the impact flare if it hit something
	if ( trace.fraction < 1.0 ) {
		vec3_t	angles;
		vec3_t	dir;

		VectorSubtract( beam.oldorigin, beam.origin, dir );
		VectorNormalize( dir );

		memset( &beam, 0, sizeof( beam ) );
		beam.hModel = cgs.media.lightningExplosionModel;

		VectorMA( trace.endpos, -16, dir, beam.origin );

		// make a random orientation
		angles[0] = rand() % 360;
		angles[1] = rand() % 360;
		angles[2] = rand() % 360;
		AnglesToAxis( angles, beam.axis );
		trap_R_AddRefEntityToScene( &beam );
	}
*/
}


/*
========================
CG_AddWeaponWithPowerups
========================
*/
static void CG_AddWeaponWithPowerups( refEntity_t *gun, int powerups ) {
	// add powerup effects
	trap_R_AddRefEntityToScene( gun );

	if (cg.predictedPlayerState.electrifyTime > cg.time)
	{ //add electrocution shell
		int preShader = gun->customShader;
		if ( rand() & 1 )
		{
			gun->customShader = cgs.media.electricBodyShader;	
		}
		else
		{
			gun->customShader = cgs.media.electricBody2Shader;
		}
		trap_R_AddRefEntityToScene( gun );
		gun->customShader = preShader; //set back just to be safe
	}

}


/*
=============
CG_AddPlayerWeapon

Used for both the view weapon (ps is valid) and the world modelother character models (ps is NULL)
The main player will have this called for BOTH cases, so effects like light and
sound should only be done on the world model case.
=============
*/
void CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent, int team, vec3_t newAngles, qboolean thirdPerson,qboolean leftweap ) {
	refEntity_t	gun;
	refEntity_t	barrel;
	vec3_t		angles;
	weapon_t	weaponNum;
	weaponInfo_t	*weapon;
	centity_t	*nonPredictedCent;
	refEntity_t	flash;

	weaponNum = cent->currentState.weapon;

	if (cent->currentState.weapon == WP_EMPLACED_GUN)
	{
		return;
	}

	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR &&
		cent->currentState.number == cg.predictedPlayerState.clientNum)
	{ //spectator mode, don't draw it...
		return;
	}

	CG_RegisterWeapon( weaponNum );
	weapon = &cg_weapons[weaponNum];
/*
Ghoul2 Insert Start
*/

	memset( &gun, 0, sizeof( gun ) );

	// only do this if we are in first person, since world weapons are now handled on the server by Ghoul2
	if (!thirdPerson)
	{

		// add the weapon
		VectorCopy( parent->lightingOrigin, gun.lightingOrigin );
		gun.shadowPlane = parent->shadowPlane;
		gun.renderfx = parent->renderfx;

		if (ps)
		{	// this player, in first person view
			//gun.hModel = weapon->viewModel;
			if (cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_4  )
			{
				if(cent->currentState.weapon == WP_BRYAR_PISTOL)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/SE-44C/blaster_pistol.md3");
				}
				else if(cent->currentState.weapon == WP_BLASTER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/f11d_blaster/blaster.md3");
				}
				else if(cent->currentState.weapon == WP_DISRUPTOR)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/psg/disruptor.md3");
				}
				else if(cent->currentState.weapon == WP_BOWCASTER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/STCompRifle/bowcaster.md3");
				}
				else if(cent->currentState.weapon == WP_REPEATER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/FWMB-10/heavy_repeater.md3");
				}
				else if(cent->currentState.weapon == WP_DEMP2)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons/CarboniteRifle/model.md3");
				}
				else if(cent->currentState.weapon == WP_FLECHETTE)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/junglerifle/c_rifle.md3");
				}
				else if(cent->currentState.weapon == WP_CONCUSSION)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/minigun/minigun.md3");
				}
				else if(cent->currentState.weapon == WP_ROCKET_LAUNCHER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons/Packered_MortarGun/model.md3");
				}
				else if(cent->currentState.weapon == WP_THERMAL)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/Stormi_TD/stormiTD.md3");
				}
				else if(cent->currentState.weapon == WP_BRYAR_OLD)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/concussion_new/c_rifle.md3");
				}
				else
				{
				gun.hModel = weapon->viewModel;
				}
			
			}
			else if (cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_3  )
			{
				if(cent->currentState.weapon == WP_BRYAR_PISTOL)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/Glie-44/blaster_pistol.md3");
				}
				else if(cent->currentState.weapon == WP_BLASTER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/EL-16/blaster.md3");
				}
				else if(cent->currentState.weapon == WP_DISRUPTOR)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/tusken_rifle/tusken_rifle.md3");
				}
				else if(cent->currentState.weapon == WP_BOWCASTER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons/Bowcaster_heavy/model.md3");
				}
				else if(cent->currentState.weapon == WP_REPEATER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/dc-17m/dc-17m.md3");
				}
				else if(cent->currentState.weapon == WP_DEMP2)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/CR-25_icerifle/rifle.md3");
				}
				else if(cent->currentState.weapon == WP_FLECHETTE)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/CR-1_cannon/rifle.md3");
				}
				else if(cent->currentState.weapon == WP_CONCUSSION)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/z6_rotary/rotary_cannon.md3");
				}
				else if(cent->currentState.weapon == WP_ROCKET_LAUNCHER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons/Packered_MortarGun/model.md3");
				}
				else if(cent->currentState.weapon == WP_THERMAL)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/sonic_detonator/sonic_det.md3");
				}
				else if(cent->currentState.weapon == WP_BRYAR_OLD)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/noweap/noweap.md3");
				}
				else
				{
				gun.hModel = weapon->viewModel;
				}
			
			}
			else if (cent->currentState.eFlags  & EF_WP_OPTION_4  )
			{
				if(cent->currentState.weapon == WP_BRYAR_PISTOL)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/westar_if/blaster_pistol.md3");
				}
				else if(cent->currentState.weapon == WP_BLASTER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/sad_e5/blaster.md3");
				}
				else if(cent->currentState.weapon == WP_DISRUPTOR)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/proj_rifle/proj_rifle.md3");
				}
				else if(cent->currentState.weapon == WP_BOWCASTER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/canderous_blaster/cand_blaster.md3");
				}
				else if(cent->currentState.weapon == WP_REPEATER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/t-21/t-21.md3");
				}
				else if(cent->currentState.weapon == WP_DEMP2)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/CR-24_flamerifle/rifle.md3");
				}
				else if(cent->currentState.weapon == WP_FLECHETTE)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons/ACP_ArrayGun/model.md3");
				}
				else if(cent->currentState.weapon == WP_CONCUSSION)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/geonosian/sonicrifle.md3");
				}
				else if(cent->currentState.weapon == WP_ROCKET_LAUNCHER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/e60r_launcher/e60r_launcher.md3");
				}
				else if(cent->currentState.weapon == WP_THERMAL)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/V-59_Concussion/V-59_conc.md3");
				}
				else if(cent->currentState.weapon == WP_BRYAR_OLD)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/noweap/noweap.md3");
				}
				else
				{
				gun.hModel = weapon->viewModel;
				}
			
			}
			else if (cent->currentState.eFlags  & EF_WP_OPTION_3  )
			{

				if(cent->currentState.weapon == WP_BRYAR_PISTOL)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/dc-17/dc-17.md3");
				}
				else if(cent->currentState.weapon == WP_BLASTER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/dc-15s/blaster.md3");
				}
				else if(cent->currentState.weapon == WP_DISRUPTOR)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/clone_disruptor/clone_disruptor.md3");
				}
				else if(cent->currentState.weapon == WP_BOWCASTER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/wbowcaster/wbowcaster1.md3");
				}
				else if(cent->currentState.weapon == WP_REPEATER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/dc-15a/dc-15a.md3");
				}
				else if(cent->currentState.weapon == WP_DEMP2)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/dc-17p/demp2.md3");
				}
				else if(cent->currentState.weapon == WP_FLECHETTE)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons/DoubleBarrel_ArrayGun/model.md3");
				}
				else if(cent->currentState.weapon == WP_CONCUSSION)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons/PulseCannon/model.md3");
				}
				else if(cent->currentState.weapon == WP_ROCKET_LAUNCHER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/cw_launcher/cw_launcher.md3");
				}
				else if(cent->currentState.weapon == WP_THERMAL)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/fraggrenade/thermal.md3");
				}
				else if(cent->currentState.weapon == WP_BRYAR_OLD)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/sc-10_holdout/sc-10_holdout.md3");
				}
				else
				{
				gun.hModel = weapon->viewModel;
				}
			
			}
			else if (cent->currentState.eFlags  & EF_WP_OPTION_2  )
			{

				if(cent->currentState.weapon == WP_BRYAR_PISTOL)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/dh-17/dh-17_FA.md3");
				}
				else if(cent->currentState.weapon == WP_BLASTER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/a280/a280.md3");
				}
				else if(cent->currentState.weapon == WP_DISRUPTOR)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/DLT20A/dlt20a.md3");
				}
				else if(cent->currentState.weapon == WP_BOWCASTER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/ee-3/ee-3.md3");
				}
				else if(cent->currentState.weapon == WP_REPEATER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/dlt-18_repeater/dlt-18_repeater.md3");
				}
				else if(cent->currentState.weapon == WP_DEMP2)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/beamRIFLE/c_rifle.md3");
				}
				else if(cent->currentState.weapon == WP_FLECHETTE)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons/Bryar_Rifle/model.md3");
				}
				else if(cent->currentState.weapon == WP_CONCUSSION)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons/LJ-70_ConcRifle/model.md3");
				}
				else if(cent->currentState.weapon == WP_ROCKET_LAUNCHER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/MiniMag_launcher/launcher.md3");
				}
				else if(cent->currentState.weapon == WP_THERMAL)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/plasma/plasma.md3");
				}
				else if(cent->currentState.weapon == WP_BRYAR_OLD)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/s5_heavy_pistol/s5_pistol.md3");
				}
				else
				{
				gun.hModel = weapon->viewModel;
				}
			
			}
			else
			{
			gun.hModel = weapon->viewModel;
			}
		}
		else
		{
			//gun.hModel = weapon->weaponModel;
			if (cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_4  )
			{
				if(cent->currentState.weapon == WP_BRYAR_PISTOL)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/SE-44C/blaster_pistol.md3");
				}
				else if(cent->currentState.weapon == WP_BLASTER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/f11d_blaster/blaster.md3");
				}
				else if(cent->currentState.weapon == WP_DISRUPTOR)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/psg/disruptor.md3");
				}
				else if(cent->currentState.weapon == WP_BOWCASTER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons/STCompRifle/bowcaster.md3");
				}
				else if(cent->currentState.weapon == WP_REPEATER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/FWMB-10/heavy_repeater.md3");
				}
				else if(cent->currentState.weapon == WP_DEMP2)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons/CarboniteRifle/model.md3");
				}
				else if(cent->currentState.weapon == WP_FLECHETTE)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/junglerifle/c_rifle.md3");
				}
				else if(cent->currentState.weapon == WP_CONCUSSION)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/minigun/minigun.md3");
				}
				else if(cent->currentState.weapon == WP_ROCKET_LAUNCHER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons/Packered_MortarGun/model.md3");
				}
				else if(cent->currentState.weapon == WP_THERMAL)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/Stormi_TD/stormiTD.md3");
				}
				else if(cent->currentState.weapon == WP_BRYAR_OLD)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/concussion_new/c_rifle.md3");
				}
				else
				{
				gun.hModel = weapon->weaponModel;
				}
			
			}
			else if (cent->currentState.eFlags  & EF_WP_OPTION_2  && cent->currentState.eFlags  & EF_WP_OPTION_3  )
			{
				if(cent->currentState.weapon == WP_BRYAR_PISTOL)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/Glie-44/blaster_pistol.md3");
				}
				else if(cent->currentState.weapon == WP_BLASTER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/EL-16/blaster.md3");
				}
				else if(cent->currentState.weapon == WP_DISRUPTOR)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/tusken_rifle/tusken_rifle.md3");
				}
				else if(cent->currentState.weapon == WP_BOWCASTER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons/Bowcaster_heavy/model.md3");
				}
				else if(cent->currentState.weapon == WP_REPEATER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/dc-17m/dc-17m.md3");
				}
				else if(cent->currentState.weapon == WP_DEMP2)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/CR-25_icerifle/rifle.md3");
				}
				else if(cent->currentState.weapon == WP_FLECHETTE)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/CR-1_cannon/rifle.md3");
				}
				else if(cent->currentState.weapon == WP_CONCUSSION)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/z6_rotary/rotary_cannon.md3");
				}
				else if(cent->currentState.weapon == WP_ROCKET_LAUNCHER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons/Packered_MortarGun/model.md3");
				}
				else if(cent->currentState.weapon == WP_THERMAL)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/sonic_detonator/sonic_det.md3");
				}
				else if(cent->currentState.weapon == WP_BRYAR_OLD)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/noweap/noweap.md3");
				}
				else
				{
				gun.hModel = weapon->weaponModel;
				}
			
			}
			else if (cent->currentState.eFlags  & EF_WP_OPTION_4  )
			{
				if(cent->currentState.weapon == WP_BRYAR_PISTOL)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/westar_if/blaster_pistol.md3");
				}
				else if(cent->currentState.weapon == WP_BLASTER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/sad_e5/blaster.md3");
				}
				else if(cent->currentState.weapon == WP_DISRUPTOR)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/proj_rifle/proj_rifle.md3");
				}
				else if(cent->currentState.weapon == WP_BOWCASTER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/canderous_blaster/cand_blaster.md3");
				}
				else if(cent->currentState.weapon == WP_REPEATER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/t-21/t-21.md3");
				}
				else if(cent->currentState.weapon == WP_DEMP2)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/CR-24_flamerifle/rifle.md3");
				}
				else if(cent->currentState.weapon == WP_FLECHETTE)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons/ACP_ArrayGun/model.md3");
				}
				else if(cent->currentState.weapon == WP_CONCUSSION)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/geonosian/sonicrifle.md3");
				}
				else if(cent->currentState.weapon == WP_ROCKET_LAUNCHER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/e60r_launcher/e60r_launcher.md3");
				}
				else if(cent->currentState.weapon == WP_THERMAL)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/V-59_Concussion/V-59_conc.md3");
				}
				else if(cent->currentState.weapon == WP_BRYAR_OLD)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/noweap/noweap.md3");
				}
				else
				{
				gun.hModel = weapon->weaponModel;
				}
			
			}
			else if (cent->currentState.eFlags  & EF_WP_OPTION_3  )
			{

				if(cent->currentState.weapon == WP_BRYAR_PISTOL)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/dc-17/dc-17.md3");
				}
				else if(cent->currentState.weapon == WP_BLASTER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/dc-15s/blaster.md3");
				}
				else if(cent->currentState.weapon == WP_DISRUPTOR)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/clone_disruptor/clone_disruptor.md3");
				}
				else if(cent->currentState.weapon == WP_BOWCASTER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/wbowcaster/wbowcaster1.md3");
				}
				else if(cent->currentState.weapon == WP_REPEATER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/dc-15a/dc-15a.md3");
				}
				else if(cent->currentState.weapon == WP_DEMP2)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/dc-17p/demp2.md3");
				}
				else if(cent->currentState.weapon == WP_FLECHETTE)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons/DoubleBarrel_ArrayGun/model.md3");
				}
				else if(cent->currentState.weapon == WP_CONCUSSION)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons/PulseCannon/model.md3");
				}
				else if(cent->currentState.weapon == WP_ROCKET_LAUNCHER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/cw_launcher/cw_launcher.md3");
				}
				else if(cent->currentState.weapon == WP_THERMAL)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/fraggrenade/thermal.md3");
				}
				else if(cent->currentState.weapon == WP_BRYAR_OLD)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/sc-10_holdout/sc-10_holdout.md3");
				}
				else
				{
				gun.hModel = weapon->weaponModel;
				}
			
			}
			else if (cent->currentState.eFlags  & EF_WP_OPTION_2  )
			{

				if(cent->currentState.weapon == WP_BRYAR_PISTOL)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/dh-17/dh-17_FA.md3");
				}
				else if(cent->currentState.weapon == WP_BLASTER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/a280/a280.md3");
				}
				else if(cent->currentState.weapon == WP_DISRUPTOR)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/DLT20A/dlt20a.md3");
				}
				else if(cent->currentState.weapon == WP_BOWCASTER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/ee-3/ee-3.md3");
				}
				else if(cent->currentState.weapon == WP_REPEATER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/dlt-18_repeater/dlt-18_repeater.md3");
				}
				else if(cent->currentState.weapon == WP_DEMP2)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/beamRIFLE/c_rifle.md3");
				}
				else if(cent->currentState.weapon == WP_FLECHETTE)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons/Bryar_Rifle/model.md3");
				}
				else if(cent->currentState.weapon == WP_CONCUSSION)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons/LJ-70_ConcRifle/model.md3");
				}
				else if(cent->currentState.weapon == WP_ROCKET_LAUNCHER)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/MiniMag_launcher/launcher.md3");
				}
				else if(cent->currentState.weapon == WP_THERMAL)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/plasma/plasma.md3");
				}
				else if(cent->currentState.weapon == WP_BRYAR_OLD)
				{
				gun.hModel = trap_R_RegisterModel("models/weapons2/s5_heavy_pistol/s5_pistol.md3");
				}
				else
				{
				gun.hModel = weapon->weaponModel;
				}
			
			}
			else
			{
			gun.hModel = weapon->weaponModel;
			}
		}
		if (!gun.hModel) {
			return;
		}

		if ( !ps ) {
			// add weapon ready sound
			cent->pe.lightningFiring = qfalse;
			if ( ( cent->currentState.eFlags & EF_FIRING ) && weapon->firingSound ) {
				// lightning gun and guantlet make a different sound when fire is held down
				trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->firingSound );
				cent->pe.lightningFiring = qtrue;
			} else if ( weapon->readySound ) {
				trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->readySound );
			}
		}

		//gun.angles[1]=20;
		//gun.origin[1]=20;
		//gun.axis[1][1]=20;
		//gun.axis[0][1]=20;
		//gun.axis[2][1]=20;
		CG_PositionEntityOnTag( &gun, parent, parent->hModel, "tag_weapon");

		if (!CG_IsMindTricked(cent->currentState.trickedentindex,
			cent->currentState.trickedentindex2,
			cent->currentState.trickedentindex3,
			cent->currentState.trickedentindex4,
			cg.snap->ps.clientNum))
		{
			CG_AddWeaponWithPowerups( &gun, cent->currentState.powerups ); //don't draw the weapon if the player is invisible
			/*
			if ( weaponNum == WP_STUN_BATON )
			{
				gun.shaderRGBA[0] = gun.shaderRGBA[1] = gun.shaderRGBA[2] = 25;
	
				gun.customShader = trap_R_RegisterShader( "gfx/effects/stunPass" );
				gun.renderfx = RF_RGB_TINT | RF_FIRST_PERSON | RF_DEPTHHACK;
				trap_R_AddRefEntityToScene( &gun );
			}
			*/
		}

		if (weaponNum == WP_STUN_BATON)
		{
			int i = 0;

			while (i < 3)
			{
				memset( &barrel, 0, sizeof( barrel ) );
				VectorCopy( parent->lightingOrigin, barrel.lightingOrigin );
				barrel.shadowPlane = parent->shadowPlane;
				barrel.renderfx = parent->renderfx;

				if (i == 0)
				{
					barrel.hModel = trap_R_RegisterModel("models/weapons2/stun_baton/baton_barrel.md3");
				}
				else if (i == 1)
				{
					barrel.hModel = trap_R_RegisterModel("models/weapons2/stun_baton/baton_barrel2.md3");
				}
				else
				{
					barrel.hModel = trap_R_RegisterModel("models/weapons2/stun_baton/baton_barrel3.md3");
				}
				angles[YAW] = 0;
				angles[PITCH] = 0;
				angles[ROLL] = 0;

				AnglesToAxis( angles, barrel.axis );
				
				if (i == 0)
				{
					CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if (i == 1)
				{
					CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel2" );
				}
				else
				{
					CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel3" );
				}
				CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups );

				i++;
			}
		}
		else
		{
			// add the spinning barrel
			if ( weapon->barrelModel ) {
				memset( &barrel, 0, sizeof( barrel ) );
				VectorCopy( parent->lightingOrigin, barrel.lightingOrigin );
				barrel.shadowPlane = parent->shadowPlane;
				barrel.renderfx = parent->renderfx;
			if (cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_4  )
			{
				if(cent->currentState.weapon == WP_BRYAR_PISTOL)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/SE-44C/blaster_pistol.md3");
				}
				else if(cent->currentState.weapon == WP_BLASTER)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/f11d_blaster/blaster.md3");
				}
				else if(cent->currentState.weapon == WP_DISRUPTOR)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/psg/disruptor.md3");
				}
				else if(cent->currentState.weapon == WP_BOWCASTER)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons/STCompRifle/bowcaster.md3");
				}
				else if(cent->currentState.weapon == WP_REPEATER)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/FWMB-10/heavy_repeater.md3");
				}
				else if(cent->currentState.weapon == WP_DEMP2)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons/CarboniteRifle/model.md3");
				}
				else if(cent->currentState.weapon == WP_FLECHETTE)
				{				
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/junglerifle/c_rifle.md3");
				}
				else if(cent->currentState.weapon == WP_CONCUSSION)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/minigun/minigun.md3");
				}
				else if(cent->currentState.weapon == WP_ROCKET_LAUNCHER)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons/Packered_MortarGun/model.md3");
				}
				else if(cent->currentState.weapon == WP_BRYAR_OLD)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/concussion_new/c_rifle.md3");
				}
				else
				{
				barrel.hModel = weapon->barrelModel;
				}
			
			}
			else if (cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_3  )
			{
				if(cent->currentState.weapon == WP_BRYAR_PISTOL)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/Glie-44/blaster_pistol.md3");
				}
				else if(cent->currentState.weapon == WP_BLASTER)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/EL-16/blaster.md3");
				}
				else if(cent->currentState.weapon == WP_DISRUPTOR)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/tusken_rifle/tusken_rifle.md3");
				}
				else if(cent->currentState.weapon == WP_BOWCASTER)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/Bowcaster_heavy/model.md3");
				}
				else if(cent->currentState.weapon == WP_REPEATER)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/dc-17m/dc-17m.md3");
				}
				else if(cent->currentState.weapon == WP_DEMP2)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/CR-25_icerifle/rifle.md3");
				}
				else if(cent->currentState.weapon == WP_FLECHETTE)
				{				
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/CR-1_cannon/rifle.md3");
				}
				else if(cent->currentState.weapon == WP_CONCUSSION)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/z6_rotary/rotary_cannon.md3");
				}
				else if(cent->currentState.weapon == WP_ROCKET_LAUNCHER)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons/Packered_MortarGun/model.md3");
				}
				else if(cent->currentState.weapon == WP_BRYAR_OLD)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/noweap/noweap.md3");
				}
				else
				{
				barrel.hModel = weapon->barrelModel;
				}
			
			}
			else if (cent->currentState.eFlags  & EF_WP_OPTION_4  )
			{
				if(cent->currentState.weapon == WP_BRYAR_PISTOL)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/westar_if/blaster_pistol.md3");
				}
				else if(cent->currentState.weapon == WP_BLASTER)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/sad_e5/blaster.md3");
				}
				else if(cent->currentState.weapon == WP_DISRUPTOR)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/proj_rifle/proj_rifle.md3");
				}
				else if(cent->currentState.weapon == WP_BOWCASTER)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/canderous_blaster/cand_blaster.md3");
				}
				else if(cent->currentState.weapon == WP_REPEATER)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/t-21/t-21.md3");
				}
				else if(cent->currentState.weapon == WP_DEMP2)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/CR-24_flamerifle/rifle.md3");
				}
				else if(cent->currentState.weapon == WP_FLECHETTE)
				{				
				//barrel.hModel = trap_R_RegisterModel("models/weapons/ACP_ArrayGun/model.md3");
				}
				else if(cent->currentState.weapon == WP_CONCUSSION)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/geonosian/sonicrifle.md3");
				}
				else if(cent->currentState.weapon == WP_ROCKET_LAUNCHER)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/e60r_launcher/e60r_launcher.md3");
				}
				else if(cent->currentState.weapon == WP_BRYAR_OLD)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/noweap/noweap.md3");
				}
				else
				{
				barrel.hModel = weapon->barrelModel;
				}
			
			}
			else if (cent->currentState.eFlags  & EF_WP_OPTION_3  )
			{
				if(cent->currentState.weapon == WP_BRYAR_PISTOL)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/dc-17/dc-17.md3");
				}
				else if(cent->currentState.weapon == WP_BLASTER)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/dc-15s/blaster.md3");
				}
				else if(cent->currentState.weapon == WP_DISRUPTOR)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/clone_disruptor/clone_disruptor.md3");
				}
				else if(cent->currentState.weapon == WP_BOWCASTER)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/wbowcaster/wbowcaster1.md3");
				}
				else if(cent->currentState.weapon == WP_REPEATER)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/dc-15a/dc-15a.md3");
				}
				else if(cent->currentState.weapon == WP_DEMP2)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/dc-17p/demp2.md3");
				}
				else if(cent->currentState.weapon == WP_FLECHETTE)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons/DoubleBarrel_ArrayGun/model.md3");
				}
				else if(cent->currentState.weapon == WP_CONCUSSION)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons/PulseCannon/model.md3");
				}
				else if(cent->currentState.weapon == WP_ROCKET_LAUNCHER)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/cw_launcher/cw_launcher.md3");
				}
				else if(cent->currentState.weapon == WP_THERMAL)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/sonic_detonator/sonic_det.md3");
				}
				else if(cent->currentState.weapon == WP_BRYAR_OLD)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/sc-10_holdout/sc-10_holdout.md3");
				}
				else
				{
				barrel.hModel = weapon->barrelModel;
				}
			
			}
			else if (cent->currentState.eFlags  & EF_WP_OPTION_2  )
			{

				if(cent->currentState.weapon == WP_BRYAR_PISTOL)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/dh-17/dh-17_FA.md3");
				}
				else if(cent->currentState.weapon == WP_BLASTER)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/a280/a280.md3");
				}
				else if(cent->currentState.weapon == WP_DISRUPTOR)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/DLT20A/dlt20a.md3");
				}
				else if(cent->currentState.weapon == WP_BOWCASTER)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/ee-3/ee-3.md3");
				}
				else if(cent->currentState.weapon == WP_REPEATER)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/dlt-18_repeater/dlt-18_repeater.md3");
				}
				else if(cent->currentState.weapon == WP_DEMP2)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/beamRIFLE/c_rifle.md3");
				}
				else if(cent->currentState.weapon == WP_FLECHETTE)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons/Bryar_Rifle/model.md3");
				}
				else if(cent->currentState.weapon == WP_CONCUSSION)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons/LJ-70_ConcRifle/model.md3");
				}
				else if(cent->currentState.weapon == WP_ROCKET_LAUNCHER)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/MiniMag_launcher/launcher.md3");
				}
				else if(cent->currentState.weapon == WP_THERMAL)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/plasma/plasma.md3");
				}
				else if(cent->currentState.weapon == WP_BRYAR_OLD)
				{
				//barrel.hModel = trap_R_RegisterModel("models/weapons2/s5_heavy_pistol/s5_pistol.md3");
				}
				else
				{
				barrel.hModel = weapon->barrelModel;
				}
			
			}
			else
			{
				barrel.hModel = weapon->barrelModel;
			}
				angles[YAW] = 0;
				angles[PITCH] = 0;
				angles[ROLL] = 0;

				AnglesToAxis( angles, barrel.axis );

			if (cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_4)
			{
				if(cent->currentState.weapon == WP_BRYAR_PISTOL)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_BLASTER)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_DISRUPTOR)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_BOWCASTER)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_REPEATER)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_DEMP2)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_FLECHETTE)
				{				
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_CONCUSSION)
				{				
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_ROCKET_LAUNCHER)
				{				
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_BRYAR_OLD)
				{				
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else
				{
				CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
			
			}
			else if (cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_3  )
			{
				if(cent->currentState.weapon == WP_BRYAR_PISTOL)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_BLASTER)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_DISRUPTOR)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_BOWCASTER)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_REPEATER)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_DEMP2)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_FLECHETTE)
				{				
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_CONCUSSION)
				{				
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_ROCKET_LAUNCHER)
				{				
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_BRYAR_OLD)
				{				
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else
				{
				CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
			
			}
			else if (cent->currentState.eFlags  & EF_WP_OPTION_4  )
			{
				if(cent->currentState.weapon == WP_BRYAR_PISTOL)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_BLASTER)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_DISRUPTOR)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_BOWCASTER)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_REPEATER)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_DEMP2)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_FLECHETTE)
				{				
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_CONCUSSION)
				{				
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_ROCKET_LAUNCHER)
				{				
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_BRYAR_OLD)
				{				
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else
				{
				CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
			
			}
			else if (cent->currentState.eFlags  & EF_WP_OPTION_3  )
			{
				if(cent->currentState.weapon == WP_BRYAR_PISTOL)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_BLASTER)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_DISRUPTOR)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_BOWCASTER)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_REPEATER)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_DEMP2)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_FLECHETTE)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_CONCUSSION)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_ROCKET_LAUNCHER)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else
				{
				CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
			
			}
			else if (cent->currentState.eFlags  & EF_WP_OPTION_2  )
			{			
				if(cent->currentState.weapon == WP_BRYAR_PISTOL)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_BLASTER)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_DISRUPTOR)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_BOWCASTER)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_REPEATER)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_DEMP2)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_FLECHETTE)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_CONCUSSION)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_ROCKET_LAUNCHER)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if(cent->currentState.weapon == WP_BRYAR_OLD)
				{
				//CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else
				{
				CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
			
			}
			else
			{
				CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
			}

				CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups );
			}
		}
	}
/*
Ghoul2 Insert End
*/

	memset (&flash, 0, sizeof(flash));
	CG_PositionEntityOnTag( &flash, &gun, gun.hModel, "tag_flash");

	VectorCopy(flash.origin, cg.lastFPFlashPoint);

	// Do special charge bits
	//-----------------------
	//[TrueView]
	//Make the guns do their charging visual in True View.
	if ( (ps || cg.renderingThirdPerson || cg.predictedPlayerState.clientNum != cent->currentState.number || cg_trueguns.integer) &&
	//if ( (ps || cg.renderingThirdPerson || cg.predictedPlayerState.clientNum != cent->currentState.number) &&
	//[/TrueView]
		( ( cent->currentState.modelindex2 == WEAPON_CHARGING_ALT && cent->currentState.weapon == WP_BRYAR_PISTOL ) ||
		  ( cent->currentState.modelindex2 == WEAPON_CHARGING_ALT && cent->currentState.weapon == WP_BRYAR_OLD ) ||
		  ( cent->currentState.weapon == WP_BOWCASTER && cent->currentState.modelindex2 == WEAPON_CHARGING ) ||
		  ( cent->currentState.weapon == WP_DEMP2 && cent->currentState.modelindex2 == WEAPON_CHARGING_ALT) ) )
	{
		int		shader = 0;
		float	val = 0.0f;
		float	scale = 1.0f;
		addspriteArgStruct_t fxSArgs;
		vec3_t flashorigin, flashdir;

		//[DualPistols]
		int	wpmdlidx = 2;

		if (leftweap == qfalse)
			wpmdlidx = 1;
		//[/DualPistols]

		if (!thirdPerson)
		{
			VectorCopy(flash.origin, flashorigin);
			VectorCopy(flash.axis[0], flashdir);
		}
		else
		{
			mdxaBone_t 		boltMatrix;

			if (!trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), wpmdlidx))
			{ //it's quite possible that we may have have no weapon model and be in a valid state, so return here if this is the case
				return;
			}

			// go away and get me the bolt position for this frame please
 			if (!(trap_G2API_GetBoltMatrix(cent->ghoul2, wpmdlidx, 0, &boltMatrix, newAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale)))
			{	// Couldn't find bolt point.
				return;
			}
			
			BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, flashorigin);
			BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_X, flashdir);
		}

		if ( cent->currentState.weapon == WP_BRYAR_PISTOL )
		{
		val = ( cg.time - cent->currentState.constantLight ) * 0.001f;	
		if (cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_4  )
		{
			shader = cgs.media.redFrontFlash;
		}
		else if (cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_3  )
		{
			shader = cgs.media.purpleFrontFlash;
		}
		else if (cent->currentState.eFlags  & EF_WP_OPTION_4  )
		{
			shader = cgs.media.yellowFrontFlash;
		}
		else if (cent->currentState.eFlags & EF_WP_OPTION_3  )
		{
			shader = cgs.media.blueFrontFlash;
		}
		else if (cent->currentState.eFlags  & EF_WP_OPTION_2  )
		{
			shader = cgs.media.greenFrontFlash;
		}
		else
		{
			shader = cgs.media.orangeFrontFlash;
		}			
			
			
		}
		if (
			cent->currentState.weapon == WP_BRYAR_OLD)
		{
			// Hardcoded max charge time of 1 second
		val = ( cg.time - cent->currentState.constantLight ) * 0.001f;
		if (cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_4  )
		{
			shader = cgs.media.orangeFrontFlash;
		}			
		else if (cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_3  )
		{
			shader = cgs.media.whiteFrontFlash;
		}
		else if (cent->currentState.eFlags  & EF_WP_OPTION_4  )
		{
			shader = cgs.media.redFrontFlash;
		}
		else if (cent->currentState.eFlags & EF_WP_OPTION_3  )
		{
			shader = cgs.media.redFrontFlash;
		}
		else if (cent->currentState.eFlags  & EF_WP_OPTION_2  )
		{
			shader = cgs.media.orangeFrontFlash;
		}
		else
		{
			shader = cgs.media.greenFrontFlash;
		}
		}
		else if ( cent->currentState.weapon == WP_BOWCASTER )
		{
			// Hardcoded max charge time of 1 second
			val = ( cg.time - cent->currentState.constantLight ) * 0.001f;
		if (cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_4  )
		{
			/* shader = cgs.media.orangeFrontFlash; */
		}
		else if (cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_3  )
		{
			shader = cgs.media.purpleFrontFlash;
		}
		else if (cent->currentState.eFlags  & EF_WP_OPTION_4  )
		{
			shader = cgs.media.yellowFrontFlash;
		}
		else if (cent->currentState.eFlags & EF_WP_OPTION_3  )
		{
			shader = cgs.media.blueFrontFlash;
		}
		else if (cent->currentState.eFlags  & EF_WP_OPTION_2  )
		{
			shader = cgs.media.redFrontFlash;
		}
		else
		{
			shader = cgs.media.greenFrontFlash;
		}
			
		}
		else if ( cent->currentState.weapon == WP_DEMP2 )
		{
			val = ( cg.time - cent->currentState.constantLight ) * 0.001f;
		if (cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_4  )
		{
//			shader = cgs.media.blueFrontFlash;
		}
		else if (cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_3  )
		{
//			shader = cgs.media.greenFrontFlash;
		}
		else if (cent->currentState.eFlags  & EF_WP_OPTION_4  )
		{
//			shader = cgs.media.yellowFrontFlash;
		}
		else if (cent->currentState.eFlags & EF_WP_OPTION_3  )
		{
			shader = cgs.media.lightningFlash;
		}
		else if (cent->currentState.eFlags  & EF_WP_OPTION_2  )
		{
			shader = cgs.media.lightningFlash;
		}
		else
		{
			shader = cgs.media.lightningFlash;
		}
			scale = 1.75f;
		}

		if ( val < 0.0f )
		{
			val = 0.0f;
		}
		else if ( val > 1.0f )
		{
			val = 1.0f;
			if (ps && cent->currentState.number == ps->clientNum)
			{
				CGCam_Shake( /*0.1f*/0.1f, 1 );
			}
		}
		else
		{
			if (ps && cent->currentState.number == ps->clientNum)
			{
				CGCam_Shake( val * val * /*0.3f*/0.3f, 100 );
			}
		}

		val += random() * 0.5f;

		VectorCopy(flashorigin, fxSArgs.origin);
		VectorClear(fxSArgs.vel);
		VectorClear(fxSArgs.accel);
		fxSArgs.scale = 3.0f*val*scale;
		fxSArgs.dscale = 0.0f;
		fxSArgs.sAlpha = 0.7f;
		fxSArgs.eAlpha = 0.7f;
		fxSArgs.rotation = random()*360;
		fxSArgs.bounce = 0.0f;
		fxSArgs.life = 1.0f;
		fxSArgs.shader = shader;
		fxSArgs.flags = 0x08000000;

		//FX_AddSprite( flash.origin, NULL, NULL, 3.0f * val, 0.0f, 0.7f, 0.7f, WHITE, WHITE, random() * 360, 0.0f, 1.0f, shader, FX_USE_ALPHA );
		trap_FX_AddSprite(&fxSArgs);
	}

	// make sure we aren't looking at cg.predictedPlayerEntity for LG
	nonPredictedCent = &cg_entities[cent->currentState.clientNum];

	// if the index of the nonPredictedCent is not the same as the clientNum
	// then this is a fake player (like on teh single player podiums), so
	// go ahead and use the cent
	if( ( nonPredictedCent - cg_entities ) != cent->currentState.clientNum ) {
		nonPredictedCent = cent;
	}

	// add the flash
	if ( ( weaponNum == WP_DEMP2)
		&& ( nonPredictedCent->currentState.eFlags & EF_FIRING ) ) 
	{
		// continuous flash
	} else {
		// impulse flash
		if ( cg.time - cent->muzzleFlashTime > MUZZLE_FLASH_TIME) {
			return;
		}
	}

	//[TrueView]
	if ( ps || cg.renderingThirdPerson || cg_trueguns.integer 
		|| cent->currentState.number != cg.predictedPlayerState.clientNum ) 
	//if ( ps || cg.renderingThirdPerson ||
	//		cent->currentState.number != cg.predictedPlayerState.clientNum ) 
	//[/TrueView]
	{	// Make sure we don't do the thirdperson model effects for the local player if we're in first person
		vec3_t flashorigin, flashdir;
		refEntity_t	flash;

		//[DualPistols]
		int	wpmdlidx = 2;

		if (leftweap == qfalse)
			wpmdlidx = 1;
		//[/DualPistols]

		memset (&flash, 0, sizeof(flash));

		if (!thirdPerson)
		{
			CG_PositionEntityOnTag( &flash, &gun, gun.hModel, "tag_flash");
			VectorCopy(flash.origin, flashorigin);
			VectorCopy(flash.axis[0], flashdir);
		}
		else
		{
			mdxaBone_t 		boltMatrix;

			if (!trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), wpmdlidx))
			{ //it's quite possible that we may have have no weapon model and be in a valid state, so return here if this is the case
				return;
			}

			// go away and get me the bolt position for this frame please
 			if (!(trap_G2API_GetBoltMatrix(cent->ghoul2, wpmdlidx, 0, &boltMatrix, newAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale)))
			{	// Couldn't find bolt point.
				return;
			}
			
			BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, flashorigin);
			BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_X, flashdir);
		}

		if ( (cent->currentState.eFlags & EF_ALT_FIRING && cg.predictedPlayerState.weapon != WP_BOWCASTER) &&
			cg.time - cent->muzzleFlashTime <= MUZZLE_FLASH_TIME + 10 )
		{	// Handle muzzle flashes
			if ( cent->currentState.eFlags & EF_ALT_FIRING )
			{	// Check the alt firing first.

					if (cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_4  )
					{
					if(weapon->altMuzzle6Effect)
					{
					if (!thirdPerson)
					{
						trap_FX_PlayEntityEffectID(weapon->altMuzzle6Effect, flashorigin, flash.axis, -1, -1, -1, -1  );
					}
					else
					{
						trap_FX_PlayEffectID(weapon->altMuzzle6Effect, flashorigin, flashdir, -1, -1);
					}
					}
					}
					else if (cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_3  )
					{
					if(weapon->altMuzzle5Effect)
					{
					if (!thirdPerson)
					{
						trap_FX_PlayEntityEffectID(weapon->altMuzzle5Effect, flashorigin, flash.axis, -1, -1, -1, -1  );
					}
					else
					{
						trap_FX_PlayEffectID(weapon->altMuzzle5Effect, flashorigin, flashdir, -1, -1);
					}
					}
					}
					else if (cent->currentState.eFlags  & EF_WP_OPTION_4  )
					{
					if(weapon->altMuzzle4Effect)
					{
					if (!thirdPerson)
					{
						trap_FX_PlayEntityEffectID(weapon->altMuzzle4Effect, flashorigin, flash.axis, -1, -1, -1, -1  );
					}
					else
					{
						trap_FX_PlayEffectID(weapon->altMuzzle4Effect, flashorigin, flashdir, -1, -1);
					}
					}
					}
					else if (cent->currentState.eFlags  & EF_WP_OPTION_3  )
					{
					if(weapon->altMuzzle3Effect)
					{
					if (!thirdPerson)
					{
						trap_FX_PlayEntityEffectID(weapon->altMuzzle3Effect, flashorigin, flash.axis, -1, -1, -1, -1  );
					}
					else
					{
						trap_FX_PlayEffectID(weapon->altMuzzle3Effect, flashorigin, flashdir, -1, -1);
					}
					}
					}
					else if (cent->currentState.eFlags  & EF_WP_OPTION_2  )
					{
					if(weapon->altMuzzle2Effect)
					{
					if (!thirdPerson)
					{
						trap_FX_PlayEntityEffectID(weapon->altMuzzle2Effect, flashorigin, flash.axis, -1, -1, -1, -1  );
					}
					else
					{
						trap_FX_PlayEffectID(weapon->altMuzzle2Effect, flashorigin, flashdir, -1, -1);
					}
					}
					}
					else
					{
					if(weapon->altMuzzleEffect)
					{
					if (!thirdPerson)
					{
						trap_FX_PlayEntityEffectID(weapon->altMuzzleEffect, flashorigin, flash.axis, -1, -1, -1, -1  );
					}
					else
					{
						trap_FX_PlayEffectID(weapon->altMuzzleEffect, flashorigin, flashdir, -1, -1);
					}
					}
					}
				
			}
			else	
			{	// Regular firing

				if (cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_4 )
				{
				if(weapon->muzzle6Effect && (cg.predictedPlayerState.weapon != WP_FLECHETTE || cg.predictedPlayerState.weapon == WP_FLECHETTE && cg.predictedPlayerState.weaponTime == 100000))
				{
					if (!thirdPerson)
					{
						trap_FX_PlayEntityEffectID(weapon->muzzle6Effect, flashorigin, flash.axis, -1, -1, -1, -1  );
					}
					else
					{
						trap_FX_PlayEffectID(weapon->muzzle6Effect, flashorigin, flashdir, -1, -1);
					}					
				}
				}
				else if (cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_3)
				{
				if(weapon->muzzle5Effect && (cg.predictedPlayerState.weapon != WP_FLECHETTE || cg.predictedPlayerState.weapon == WP_FLECHETTE && cg.predictedPlayerState.weaponTime == 100000))
				{
					if (!thirdPerson)
					{
						trap_FX_PlayEntityEffectID(weapon->muzzle5Effect, flashorigin, flash.axis, -1, -1, -1, -1  );
					}
					else
					{
						trap_FX_PlayEffectID(weapon->muzzle5Effect, flashorigin, flashdir, -1, -1);
					}					
				}
				}
				else if (cent->currentState.eFlags  & EF_WP_OPTION_4 )
				{
				if(weapon->muzzle4Effect && (cg.predictedPlayerState.weapon != WP_FLECHETTE || cg.predictedPlayerState.weapon == WP_FLECHETTE && cg.predictedPlayerState.weaponTime == 100000))
				{
					if (!thirdPerson)
					{
						trap_FX_PlayEntityEffectID(weapon->muzzle4Effect, flashorigin, flash.axis, -1, -1, -1, -1  );
					}
					else
					{
						trap_FX_PlayEffectID(weapon->muzzle4Effect, flashorigin, flashdir, -1, -1);
					}					
				}
				}
				else if (cent->currentState.eFlags  & EF_WP_OPTION_3 )
				{
				if(weapon->muzzle3Effect && (cg.predictedPlayerState.weapon != WP_FLECHETTE || cg.predictedPlayerState.weapon == WP_FLECHETTE && cg.predictedPlayerState.weaponTime == 100000))
				{
					if (!thirdPerson)
					{
						trap_FX_PlayEntityEffectID(weapon->muzzle3Effect, flashorigin, flash.axis, -1, -1, -1, -1  );
					}
					else
					{
						trap_FX_PlayEffectID(weapon->muzzle3Effect, flashorigin, flashdir, -1, -1);
					}					
				}
				}
				else if (cent->currentState.eFlags  & EF_WP_OPTION_2 )
				{
				if(weapon->muzzle2Effect && (cg.predictedPlayerState.weapon != WP_FLECHETTE || cg.predictedPlayerState.weapon == WP_FLECHETTE && cg.predictedPlayerState.weaponTime == 100000))
				{
					if (!thirdPerson)
					{
						trap_FX_PlayEntityEffectID(weapon->muzzle2Effect, flashorigin, flash.axis, -1, -1, -1, -1  );
					}
					else
					{
						trap_FX_PlayEffectID(weapon->muzzle2Effect, flashorigin, flashdir, -1, -1);
					}					
				}
				}
				else
				{
				if(weapon->muzzleEffect && (cg.predictedPlayerState.weapon != WP_FLECHETTE || cg.predictedPlayerState.weapon == WP_FLECHETTE && cg.predictedPlayerState.weaponTime == 100000))
				{
					if (!thirdPerson)
					{
						trap_FX_PlayEntityEffectID(weapon->muzzleEffect, flashorigin, flash.axis, -1, -1, -1, -1  );
					}
					else
					{
						trap_FX_PlayEffectID(weapon->muzzleEffect, flashorigin, flashdir, -1, -1);
					}					
				}
				}
			}
		}

		// add lightning bolt
		CG_LightningBolt( nonPredictedCent, flashorigin );

		if ( weapon->flashDlightColor[0] || weapon->flashDlightColor[1] || weapon->flashDlightColor[2] ) {
			trap_R_AddLightToScene( flashorigin, 300 + (rand()&31), weapon->flashDlightColor[0],
				weapon->flashDlightColor[1], weapon->flashDlightColor[2] );
		}
	}
}

/*
==============
CG_AddViewWeapon

Add the weapon, and flash for the player's view
==============
*/
void CG_AddViewWeapon( playerState_t *ps ) {
	refEntity_t	hand;
	centity_t	*cent;
	clientInfo_t	*ci;
	float		fovOffset;
	vec3_t		angles;
	weaponInfo_t	*weapon;
	//[TrueView]
	float	cgFov;

	if(!cg.renderingThirdPerson && (cg_trueguns.integer || cg.predictedPlayerState.weapon == WP_SABER
	|| cg.predictedPlayerState.weapon == WP_MELEE) && cg_truefov.value 
		&& (cg.predictedPlayerState.pm_type != PM_SPECTATOR)
		&& (cg.predictedPlayerState.pm_type != PM_INTERMISSION))
	{
		cgFov = cg_truefov.value;
	}
	else
	{
		cgFov = cg_fov.value;
	}
	//float	cgFov = cg_fov.value;
	//[TrueView]
	

	if (cgFov < 1)
	{
		cgFov = 1;
	}

	//[TrueView]
	//Allow larger Fields of View
	if (cgFov > 180)
	{
		cgFov = 180;
	}
	/*
	if (cgFov > 97)
	{
		cgFov = 97;
	}
	*/
	//[TrueView]

	if ( ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		return;
	}

	if ( ps->pm_type == PM_INTERMISSION ) {
		return;
	}

	// no gun if in third person view or a camera is active
	//if ( cg.renderingThirdPerson || cg.cameraMode) {
	if ( cg.renderingThirdPerson ) {
		return;
	}

	// allow the gun to be completely removed
	//[TrueView]
	if ( !cg_drawGun.integer || cg.predictedPlayerState.zoomMode || cg_trueguns.integer
		|| cg.predictedPlayerState.weapon == WP_SABER || cg.predictedPlayerState.weapon == WP_MELEE) {
	//if ( !cg_drawGun.integer || cg.predictedPlayerState.zoomMode) {
	//[TrueView]
		vec3_t		origin;

		if ( cg.predictedPlayerState.eFlags & EF_FIRING ) {
			// special hack for lightning gun...
			VectorCopy( cg.refdef.vieworg, origin );
			VectorMA( origin, -8, cg.refdef.viewaxis[2], origin );
			CG_LightningBolt( &cg_entities[ps->clientNum], origin );
		}
		return;
	}

	// don't draw if testing a gun model
	if ( cg.testGun ) {
		return;
	}

	// drop gun lower at higher fov
	if ( cgFov > 90 ) {
		fovOffset = -0.2 * ( cgFov - 90 );
	} else {
		fovOffset = 0;
	}

	cent = &cg_entities[cg.predictedPlayerState.clientNum];
	CG_RegisterWeapon( ps->weapon );
	weapon = &cg_weapons[ ps->weapon ];

	memset (&hand, 0, sizeof(hand));

	// set up gun position
	CG_CalculateWeaponPosition( hand.origin, angles );

	VectorMA( hand.origin, cg_gun_x.value, cg.refdef.viewaxis[0], hand.origin );
	VectorMA( hand.origin, cg_gun_y.value, cg.refdef.viewaxis[1], hand.origin );
	VectorMA( hand.origin, (cg_gun_z.value+fovOffset), cg.refdef.viewaxis[2], hand.origin );

	AnglesToAxis( angles, hand.axis );

	// map torso animations to weapon animations
	if ( cg_gun_frame.integer ) {
		// development tool
		hand.frame = hand.oldframe = cg_gun_frame.integer;
		hand.backlerp = 0;
	} else {
		// get clientinfo for animation map
		if (cent->currentState.eType == ET_NPC)
		{
			if (!cent->npcClient)
			{
				return;
			}

			ci = cent->npcClient;
		}
		else
		{
			ci = &cgs.clientinfo[ cent->currentState.clientNum ];
		}
		hand.frame = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.frame, cent->currentState.torsoAnim );
		hand.oldframe = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.oldFrame, cent->currentState.torsoAnim );
		hand.backlerp = cent->pe.torso.backlerp;

		// Handle the fringe situation where oldframe is invalid
		if ( hand.frame == -1 )
		{
			hand.frame = 0;
			hand.oldframe = 0;
			hand.backlerp = 0;
		}
		else if ( hand.oldframe == -1 )
		{
			hand.oldframe = hand.frame;
			hand.backlerp = 0;
		}
	}

	hand.hModel = weapon->handsModel;
	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON;// | RF_MINLIGHT;

	// add everything onto the hand
	//[DualPistols]
	//CG_AddPlayerWeapon( &hand, ps, &cg_entities[cg.predictedPlayerState.clientNum], ps->persistant[PERS_TEAM], angles, qfalse );
	CG_AddPlayerWeapon( &hand, ps, &cg_entities[cg.predictedPlayerState.clientNum], ps->persistant[PERS_TEAM], angles, qfalse,qfalse );
	if((ps->eFlags & EF_DUAL_WEAPONS) && ps->weapon == WP_BRYAR_PISTOL)
	{
		memset (&hand, 0, sizeof(hand));
        
        // set up gun position
        CG_CalculateWeaponPosition( hand.origin, angles );
        
        VectorMA( hand.origin, cg_gun_x.value, cg.refdef.viewaxis[0], hand.origin );
        VectorMA( hand.origin, cg_gun_y.value + 10, cg.refdef.viewaxis[1], hand.origin );
        VectorMA( hand.origin, (cg_gun_z.value+fovOffset), cg.refdef.viewaxis[2], hand.origin );
        AnglesToAxis( angles, hand.axis );

		// map torso animations to weapon animations
        	if ( cg_gun_frame.integer ) {
        		// development tool
        		hand.frame = hand.oldframe = cg_gun_frame.integer;
        		hand.backlerp = 0;
        	} else {
        		// get clientinfo for animation map
        		if (cent->currentState.eType == ET_NPC)
        		{
        			if (!cent->npcClient)
        			{
        				return;
        			}
        
        			ci = cent->npcClient;
        		}
        		else
        		{
        			ci = &cgs.clientinfo[ cent->currentState.clientNum ];
        		}
        		hand.frame = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.frame, cent->currentState.torsoAnim );
        		hand.oldframe = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.oldFrame, cent->currentState.torsoAnim );
        		hand.backlerp = cent->pe.torso.backlerp;
        
        		// Handle the fringe situation where oldframe is invalid
        		if ( hand.frame == -1 )
        		{
        			hand.frame = 0;
        			hand.oldframe = 0;
        			hand.backlerp = 0;
        		}
        		else if ( hand.oldframe == -1 )
        		{
        			hand.oldframe = hand.frame;
        			hand.backlerp = 0;
        		}
        	}
			////adasadas
        	hand.hModel = weapon->handsModel;
        	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON;// | RF_MINLIGHT;
        	// add everything onto the hand
        	CG_AddPlayerWeapon( &hand, ps, &cg_entities[cg.predictedPlayerState.clientNum], ps->persistant[PERS_TEAM], angles, qfalse, qtrue );

	}
	else if((ps->eFlags & EF_DUAL_WEAPONS) && ps->weapon == WP_BRYAR_OLD)
	{
		memset (&hand, 0, sizeof(hand));
        
        // set up gun position
        CG_CalculateWeaponPosition( hand.origin, angles );
        
        VectorMA( hand.origin, cg_gun_x.value, cg.refdef.viewaxis[0], hand.origin );
        VectorMA( hand.origin, cg_gun_y.value + 10, cg.refdef.viewaxis[1], hand.origin );
        VectorMA( hand.origin, (cg_gun_z.value+fovOffset), cg.refdef.viewaxis[2], hand.origin );
        AnglesToAxis( angles, hand.axis );

		// map torso animations to weapon animations
        	if ( cg_gun_frame.integer ) {
        		// development tool
        		hand.frame = hand.oldframe = cg_gun_frame.integer;
        		hand.backlerp = 0;
        	} else {
        		// get clientinfo for animation map
        		if (cent->currentState.eType == ET_NPC)
        		{
        			if (!cent->npcClient)
        			{
        				return;
        			}
        
        			ci = cent->npcClient;
        		}
        		else
        		{
        			ci = &cgs.clientinfo[ cent->currentState.clientNum ];
        		}
        		hand.frame = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.frame, cent->currentState.torsoAnim );
        		hand.oldframe = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.oldFrame, cent->currentState.torsoAnim );
        		hand.backlerp = cent->pe.torso.backlerp;
        
        		// Handle the fringe situation where oldframe is invalid
        		if ( hand.frame == -1 )
        		{
        			hand.frame = 0;
        			hand.oldframe = 0;
        			hand.backlerp = 0;
        		}
        		else if ( hand.oldframe == -1 )
        		{
        			hand.oldframe = hand.frame;
        			hand.backlerp = 0;
        		}
        	}
			////adasadas
        	hand.hModel = weapon->handsModel;
        	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON;// | RF_MINLIGHT;
        	// add everything onto the hand
        	CG_AddPlayerWeapon( &hand, ps, &cg_entities[cg.predictedPlayerState.clientNum], ps->persistant[PERS_TEAM], angles, qfalse, qtrue );

  	}
	else if((ps->eFlags & EF_DUAL_WEAPONS) && ps->weapon == WP_STUN_BATON)
	{
		memset (&hand, 0, sizeof(hand));
        
        // set up gun position
        CG_CalculateWeaponPosition( hand.origin, angles );
        
        VectorMA( hand.origin, cg_gun_x.value, cg.refdef.viewaxis[0], hand.origin );
        VectorMA( hand.origin, cg_gun_y.value + 10, cg.refdef.viewaxis[1], hand.origin );
        VectorMA( hand.origin, (cg_gun_z.value+fovOffset), cg.refdef.viewaxis[2], hand.origin );
        AnglesToAxis( angles, hand.axis );

		// map torso animations to weapon animations
        	if ( cg_gun_frame.integer ) {
        		// development tool
        		hand.frame = hand.oldframe = cg_gun_frame.integer;
        		hand.backlerp = 0;
        	} else {
        		// get clientinfo for animation map
        		if (cent->currentState.eType == ET_NPC)
        		{
        			if (!cent->npcClient)
        			{
        				return;
        			}
        
        			ci = cent->npcClient;
        		}
        		else
        		{
        			ci = &cgs.clientinfo[ cent->currentState.clientNum ];
        		}
        		hand.frame = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.frame, cent->currentState.torsoAnim );
        		hand.oldframe = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.oldFrame, cent->currentState.torsoAnim );
        		hand.backlerp = cent->pe.torso.backlerp;
        
        		// Handle the fringe situation where oldframe is invalid
        		if ( hand.frame == -1 )
        		{
        			hand.frame = 0;
        			hand.oldframe = 0;
        			hand.backlerp = 0;
        		}
        		else if ( hand.oldframe == -1 )
        		{
        			hand.oldframe = hand.frame;
        			hand.backlerp = 0;
        		}
        	}
			////adasadas
        	hand.hModel = weapon->handsModel;
        	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON;// | RF_MINLIGHT;
        	// add everything onto the hand
        	CG_AddPlayerWeapon( &hand, ps, &cg_entities[cg.predictedPlayerState.clientNum], ps->persistant[PERS_TEAM], angles, qfalse, qtrue );

  	}
	//[/DualPistols]
}

/*
==============================================================================

WEAPON SELECTION

==============================================================================
*/
#define ICON_WEAPONS	0
#define ICON_FORCE		1
#define ICON_INVENTORY	2


void CG_DrawIconBackground(void)
{
	int				height,xAdd,x2,y2,t;
//	int				prongLeftX,prongRightX;
	float			inTime = cg.invenSelectTime+WEAPON_SELECT_TIME;
	float			wpTime = cg.weaponSelectTime+WEAPON_SELECT_TIME;
	float			fpTime = cg.forceSelectTime+WEAPON_SELECT_TIME;
//	int				drawType = cgs.media.weaponIconBackground;
//	int				yOffset = 0;

	// don't display if dead
	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 ) 
	{
		return;
	}

	if (cg_hudFiles.integer)
	{ //simple hud
		return;
	}

	x2 = 30;
	y2 = SCREEN_HEIGHT-70;

	//prongLeftX =x2+37; 
	//prongRightX =x2+544; 

	if (inTime > wpTime)
	{
//		drawType = cgs.media.inventoryIconBackground;
		cg.iconSelectTime = cg.invenSelectTime;

	}
	else
	{
//		drawType = cgs.media.weaponIconBackground;
		cg.iconSelectTime = cg.weaponSelectTime;
	}

	if (fpTime > inTime && fpTime > wpTime)
	{
//		drawType = cgs.media.forceIconBackground;
		cg.iconSelectTime = cg.forceSelectTime;
	}

	if ((cg.iconSelectTime+WEAPON_SELECT_TIME)<cg.time)	// Time is up for the HUD to display
	{
		if (cg.iconHUDActive)		// The time is up, but we still need to move the prongs back to their original position
		{
			t =  cg.time - (cg.iconSelectTime+WEAPON_SELECT_TIME);
			cg.iconHUDPercent = t/ 130.0f;
			cg.iconHUDPercent = 1 - cg.iconHUDPercent;

			if (cg.iconHUDPercent<0)
			{
				cg.iconHUDActive = qfalse;
				cg.iconHUDPercent=0;
			}

			xAdd = (int) 8*cg.iconHUDPercent;

			height = (int) (60.0f*cg.iconHUDPercent);
			//CG_DrawPic( x2+60, y2+30+yOffset, 460, -height, drawType);	// Top half
			//CG_DrawPic( x2+60, y2+30-2+yOffset, 460, height, drawType);	// Bottom half

		}
		else
		{
			xAdd = 0;
		}

		return;
	}
	//prongLeftX =x2+37; 
	//prongRightX =x2+544; 

	if (!cg.iconHUDActive)
	{
		t = cg.time - cg.iconSelectTime;
		cg.iconHUDPercent = t/ 130.0f;

		// Calc how far into opening sequence we are
		if (cg.iconHUDPercent>1)
		{
			cg.iconHUDActive = qtrue;
			cg.iconHUDPercent=1;
		}
		else if (cg.iconHUDPercent<0)
		{
			cg.iconHUDPercent=0;
		}
	}
	else
	{
		cg.iconHUDPercent=1;
	}

	//trap_R_SetColor( colorTable[CT_WHITE] );					
	//height = (int) (60.0f*cg.iconHUDPercent);
	//CG_DrawPic( x2+60, y2+30+yOffset, 460, -height, drawType);	// Top half
	//CG_DrawPic( x2+60, y2+30-2+yOffset, 460, height, drawType);	// Bottom half

	// And now for the prongs
/*	if ((cg.inventorySelectTime+WEAPON_SELECT_TIME)>cg.time)	
	{
		cgs.media.currentBackground = ICON_INVENTORY;
		background = &cgs.media.inventoryProngsOn;
	}
	else if ((cg.weaponSelectTime+WEAPON_SELECT_TIME)>cg.time)	
	{
		cgs.media.currentBackground = ICON_WEAPONS;
	}
	else 
	{
		cgs.media.currentBackground = ICON_FORCE;
		background = &cgs.media.forceProngsOn;
	}
*/
	// Side Prongs
//	trap_R_SetColor( colorTable[CT_WHITE]);					
//	xAdd = (int) 8*cg.iconHUDPercent;
//	CG_DrawPic( prongLeftX+xAdd, y2-10, 40, 80, background);
//	CG_DrawPic( prongRightX-xAdd, y2-10, -40, 80, background);

}

qboolean CG_WeaponCheck(int weap)
{
	//[Reload]
	/*
	if (cg.snap->ps.ammo[weaponData[weap].ammoIndex] < weaponData[weap].energyPerShot &&
		cg.snap->ps.ammo[weaponData[weap].ammoIndex] < weaponData[weap].altEnergyPerShot)
	{
		return qfalse;
	}
	*/
	if(cg.snap->ps.ammo[weaponData[weap].ammoIndex] == -10)
		return qfalse;

	if (weap == WP_DET_PACK && cg.predictedPlayerState.ammo[weaponData[weap].ammoIndex] < 1 &&
		!cg.predictedPlayerState.hasDetPackPlanted)
	{
		return qfalse;
	}

	if(weap == WP_TRIP_MINE && cg.predictedPlayerState.ammo[weaponData[weap].ammoIndex] < 1)
		return qfalse;

	if(weap == WP_THERMAL && cg.predictedPlayerState.ammo[weaponData[weap].ammoIndex] < 1)
		return qfalse;
	//[/Reload]

	return qtrue;
}

/*
===============
CG_WeaponSelectable
===============
*/
static qboolean CG_WeaponSelectable( int i ) {
	/*if ( !cg.snap->ps.ammo[weaponData[i].ammoIndex] ) {
		return qfalse;
	}*/
	if (!i)
	{
		return qfalse;
	}

	//[Reload]
	if (cg.predictedPlayerState.ammo[weaponData[i].ammoIndex] == -10)
	{
		return qfalse;
	}
	//[/Reload]
	

	if (i == WP_DET_PACK && cg.predictedPlayerState.ammo[weaponData[i].ammoIndex] < 1 &&
		!cg.predictedPlayerState.hasDetPackPlanted)
	{
		return qfalse;
	}

	//[Reload]
	if(i == WP_TRIP_MINE && cg.predictedPlayerState.ammo[weaponData[i].ammoIndex] < 1)
		return qfalse;

	if(i == WP_THERMAL && cg.predictedPlayerState.ammo[weaponData[i].ammoIndex] < 1)
		return qfalse;
	//[/Reload]

	if ( ! (cg.predictedPlayerState.stats[ STAT_WEAPONS ] & ( 1 << i ) ) ) {
		return qfalse;
	}

	return qtrue;
}

/*
===================
CG_DrawWeaponSelect
===================
*/
void CG_DrawWeaponSelect( void ) {
	int				i;
	int				bits;
	int				count;
	int				smallIconSize,bigIconSize;
	int				x,y,pad;
	int				sideLeftIconCnt,sideRightIconCnt;
	int				sideMax,holdCount;
	int				height;
	int		yOffset = 0;
	qboolean drewConc = qfalse;

	if (cg.predictedPlayerState.emplacedIndex)
	{ //can't cycle when on a weapon
		cg.weaponSelectTime = 0;
	}

	if ((cg.weaponSelectTime+WEAPON_SELECT_TIME)<cg.time)	// Time is up for the HUD to display
	{
		return;
	}

	// don't display if dead
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) 
	{
		return;
	}

	// showing weapon select clears pickup item display, but not the blend blob
	cg.itemPickupTime = 0;

	bits = cg.predictedPlayerState.stats[ STAT_WEAPONS ];

	// count the number of weapons owned
	count = 0;

	if ( !CG_WeaponSelectable(cg.weaponSelect) &&
		(cg.weaponSelect == WP_THERMAL || cg.weaponSelect == WP_TRIP_MINE) )
	{ //display this weapon that we don't actually "have" as unhighlighted until it's deselected
	  //since it's selected we must increase the count to display the proper number of valid selectable weapons
		count++;
	}

	for ( i = 1 ; i < WP_NUM_WEAPONS ; i++ ) 
	{
		if ( bits & ( 1 << i ) ) 
		{
			if ( CG_WeaponSelectable(i) ||
				(i != WP_THERMAL && i != WP_TRIP_MINE) )
			{
				count++;
			}
		}
	}

	if (count == 0)	// If no weapons, don't display
	{
		return;
	}

	sideMax = 3;	// Max number of icons on the side

	// Calculate how many icons will appear to either side of the center one
	holdCount = count - 1;	// -1 for the center icon
	if (holdCount == 0)			// No icons to either side
	{
		sideLeftIconCnt = 0;
		sideRightIconCnt = 0;
	}
	else if (count > (2*sideMax))	// Go to the max on each side
	{
		sideLeftIconCnt = sideMax;
		sideRightIconCnt = sideMax;
	}
	else							// Less than max, so do the calc
	{
		sideLeftIconCnt = holdCount/2;
		sideRightIconCnt = holdCount - sideLeftIconCnt;
	}

	if ( cg.weaponSelect == WP_CONCUSSION )
	{
		i = WP_FLECHETTE;
	}
	else
	{
		i = cg.weaponSelect - 1;
	}
	if (i<1)
	{
		i = LAST_USEABLE_WEAPON;
	}

	smallIconSize = 40;
	bigIconSize = 80;
	pad = 12;

	x = 320;
	y = 410;

	// Background
//	memcpy(calcColor, colorTable[CT_WHITE], sizeof(vec4_t));
//	calcColor[3] = .35f;
//	trap_R_SetColor( calcColor);	

				
/*
	// Left side ICONS
	trap_R_SetColor(colorTable[CT_WHITE]);
	// Work backwards from current icon
	holdX = x - ((bigIconSize/2) + pad + smallIconSize);
	height = smallIconSize * 1;//cg.iconHUDPercent;
	drewConc = qfalse;

	for (iconCnt=1;iconCnt<(sideLeftIconCnt+1);i--)
	{
		if ( i == WP_CONCUSSION )
		{
			i--;
		}
		else if ( i == WP_FLECHETTE && !drewConc && cg.weaponSelect != WP_CONCUSSION )
		{
			i = WP_CONCUSSION;
		}
		if (i<1)
		{
			//i = 13;
			//...don't ever do this.
			i = LAST_USEABLE_WEAPON;
		}

		if ( !(bits & ( 1 << i )))	// Does he have this weapon?
		{
			if ( i == WP_CONCUSSION )
			{
				drewConc = qtrue;
				i = WP_ROCKET_LAUNCHER;
			}
			continue;
		}

		if ( !CG_WeaponSelectable(i) &&
			(i == WP_THERMAL || i == WP_TRIP_MINE) )
		{ //Don't show thermal and tripmine when out of them
			continue;
		}

		++iconCnt;					// Good icon

		if (cgs.media.weaponIcons[i] || cgs.media.weaponIcons2[i] || cgs.media.weaponIcons3[i] || cgs.media.weaponIcons4[i] || cgs.media.weaponIcons5[i] || cgs.media.weaponIcons6[i])
		{
			weaponInfo_t	*weaponInfo;
			CG_RegisterWeapon( i );	
			weaponInfo = &cg_weapons[i];

			trap_R_SetColor(colorTable[CT_WHITE]);
			if (!CG_WeaponCheck(i))
			{

				
//				CG_DrawPic( holdX, y+10+yOffset, smallIconSize, smallIconSize, /*weaponInfo->weaponIconNoAmmocgs.media.weaponIcons_NA[i] );
				
			}
			else
			{

				
//				CG_DrawPic( holdX, y+10+yOffset, smallIconSize, smallIconSize, /*weaponInfo->weaponIconcgs.media.weaponIcons[i] );
				
				
			}

			holdX -= (smallIconSize+pad);
		}
		if ( i == WP_CONCUSSION )
		{
			drewConc = qtrue;
			i = WP_ROCKET_LAUNCHER;
		}
	}
*/
	// Current Center Icon
	height = bigIconSize * cg.iconHUDPercent;

		weaponInfo_t	*weaponInfo;
		CG_RegisterWeapon( cg.weaponSelect );	
		weaponInfo = &cg_weapons[cg.weaponSelect];

		trap_R_SetColor( colorTable[CT_WHITE]);
		if (!CG_WeaponCheck(cg.weaponSelect))
		{
				if(cg.snap->ps.eFlags & EF_WP_OPTION_2 && cg.snap->ps.eFlags & EF_WP_OPTION_4)
				{
				if(cgs.media.weaponIcons6[cg.weaponSelect])
				{
				CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10+yOffset, bigIconSize, bigIconSize, cgs.media.weaponIcons6_NA[cg.weaponSelect] );
				}
				}
				else if(cg.snap->ps.eFlags & EF_WP_OPTION_2 && cg.snap->ps.eFlags & EF_WP_OPTION_3 )
				{
				if(cgs.media.weaponIcons5[cg.weaponSelect])
				{
				CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10+yOffset, bigIconSize, bigIconSize, cgs.media.weaponIcons5_NA[cg.weaponSelect] );
				}
				}
				else if(cg.snap->ps.eFlags & EF_WP_OPTION_4 )
				{
				if(cgs.media.weaponIcons4[cg.weaponSelect])
				{
				CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10+yOffset, bigIconSize, bigIconSize, cgs.media.weaponIcons4_NA[cg.weaponSelect] );
				}
				}
				else if(cg.snap->ps.eFlags & EF_WP_OPTION_3 )
				{
				if(cgs.media.weaponIcons3[cg.weaponSelect])
				{
				CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10+yOffset, bigIconSize, bigIconSize, cgs.media.weaponIcons3_NA[cg.weaponSelect] );
				}
				}
				else if(cg.snap->ps.eFlags & EF_WP_OPTION_2 )
				{
				if(cgs.media.weaponIcons2[cg.weaponSelect])
				{
				CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10+yOffset, bigIconSize, bigIconSize, cgs.media.weaponIcons2_NA[cg.weaponSelect] );
				}
				}		
				else
				{
				if(cgs.media.weaponIcons[cg.weaponSelect])
				{
				CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10+yOffset, bigIconSize, bigIconSize, cgs.media.weaponIcons_NA[cg.weaponSelect] );
				}
				}
			
		}
		else
		{
				if(cg.snap->ps.eFlags & EF_WP_OPTION_2 && cg.snap->ps.eFlags & EF_WP_OPTION_4 )
				{
				if(cgs.media.weaponIcons6[cg.weaponSelect])
				{
				CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10+yOffset, bigIconSize, bigIconSize, cgs.media.weaponIcons6[cg.weaponSelect] );
				}
				}
				else if(cg.snap->ps.eFlags & EF_WP_OPTION_2 && cg.snap->ps.eFlags & EF_WP_OPTION_3 )
				{
				if(cgs.media.weaponIcons5[cg.weaponSelect])
				{
				CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10+yOffset, bigIconSize, bigIconSize, cgs.media.weaponIcons5[cg.weaponSelect] );
				}
				}
				else if(cg.snap->ps.eFlags & EF_WP_OPTION_4 )
				{
				if(cgs.media.weaponIcons4[cg.weaponSelect])
				{
				CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10+yOffset, bigIconSize, bigIconSize, cgs.media.weaponIcons4[cg.weaponSelect] );
				}
				}
				else if(cg.snap->ps.eFlags & EF_WP_OPTION_3)
				{
				if(cgs.media.weaponIcons3[cg.weaponSelect])
				{
				CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10+yOffset, bigIconSize, bigIconSize, cgs.media.weaponIcons3[cg.weaponSelect] );
				}
				}
				else if(cg.snap->ps.eFlags & EF_WP_OPTION_2 )
				{
				if(cgs.media.weaponIcons2[cg.weaponSelect])
				{
				CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10+yOffset, bigIconSize, bigIconSize, cgs.media.weaponIcons2[cg.weaponSelect] );
				}
				}			
				else
				{
				if(cgs.media.weaponIcons[cg.weaponSelect])
				{
				CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10+yOffset, bigIconSize, bigIconSize, cgs.media.weaponIcons[cg.weaponSelect] );
				}
				}
			
		}
	

	if ( cg.weaponSelect == WP_CONCUSSION )
	{
		i = WP_ROCKET_LAUNCHER;
	}
	else
	{
		i = cg.weaponSelect + 1;
	}
	if (i> LAST_USEABLE_WEAPON)
	{
		i = 1;
	}




/*
	// Right side ICONS
	// Work forwards from current icon

	holdX = x + (bigIconSize/2) + pad;
	height = smallIconSize * cg.iconHUDPercent;
	for (iconCnt=1;iconCnt<(sideRightIconCnt+1);i++)
	{
		if ( i == WP_CONCUSSION )
		{
			i++;
		}
		else if ( i == WP_ROCKET_LAUNCHER && !drewConc && cg.weaponSelect != WP_CONCUSSION )
		{
			i = WP_CONCUSSION;
		}
		if (i>LAST_USEABLE_WEAPON)
		{
			i = 1;
		}

		if ( !(bits & ( 1 << i )))	// Does he have this weapon?
		{
			if ( i == WP_CONCUSSION )
			{
				drewConc = qtrue;
				i = WP_FLECHETTE;
			}
			continue;
		}

		if ( !CG_WeaponSelectable(i) &&
			(i == WP_THERMAL || i == WP_TRIP_MINE) )
		{ //Don't show thermal and tripmine when out of them
			continue;
		}

		++iconCnt;					// Good icon

		if (/*weaponData[i].weaponIcon[0] cgs.media.weaponIcons[i] || cgs.media.weaponIcons2[i] || cgs.media.weaponIcons3[i] || cgs.media.weaponIcons4[i] || cgs.media.weaponIcons5[i] || cgs.media.weaponIcons6[i])
		{
			weaponInfo_t	*weaponInfo;
			CG_RegisterWeapon( i );	
			weaponInfo = &cg_weapons[i];
			// No ammo for this weapon?
			trap_R_SetColor( colorTable[CT_WHITE]);
			if (!CG_WeaponCheck(i))
			{

				
//				CG_DrawPic( holdX, y+10+yOffset, smallIconSize, smallIconSize, cgs.media.weaponIcons_NA[i] );
				
				
			}
			else
			{
				
//				CG_DrawPic( holdX, y+10+yOffset, smallIconSize, smallIconSize, cgs.media.weaponIcons[i] );
				
				
			}


			holdX += (smallIconSize+pad);
		}
		if ( i == WP_CONCUSSION )
		{
			drewConc = qtrue;
			i = WP_FLECHETTE;
		}
	}
*/
	// draw the selected name
	

	if(cg.snap->ps.eFlags & EF_WP_OPTION_2 && cg.snap->ps.eFlags & EF_WP_OPTION_4)
	{
	if(showWeaponsName6[cg.weaponSelect])
	{
		UI_DrawProportionalString(320, y+45+yOffset, CG_GetStringEdString("SP_INGAME", showWeaponsName6[cg.weaponSelect]), UI_CENTER | UI_SMALLFONT, colorTable[CT_ICON_BLUE]);
	}
	}
	else if(cg.snap->ps.eFlags & EF_WP_OPTION_2 && cg.snap->ps.eFlags & EF_WP_OPTION_3)
	{
	if(showWeaponsName5[cg.weaponSelect])
	{
		UI_DrawProportionalString(320, y+45+yOffset, CG_GetStringEdString("SP_INGAME", showWeaponsName5[cg.weaponSelect]), UI_CENTER | UI_SMALLFONT, colorTable[CT_ICON_BLUE]);
	}
	}	
	else if(cg.snap->ps.eFlags & EF_WP_OPTION_4)
	{
	if(showWeaponsName4[cg.weaponSelect])
	{
		UI_DrawProportionalString(320, y+45+yOffset, CG_GetStringEdString("SP_INGAME", showWeaponsName4[cg.weaponSelect]), UI_CENTER | UI_SMALLFONT, colorTable[CT_ICON_BLUE]);
	}
	}
	else if(cg.snap->ps.eFlags & EF_WP_OPTION_3)
	{
	if(showWeaponsName3[cg.weaponSelect])
	{
		UI_DrawProportionalString(320, y+45+yOffset, CG_GetStringEdString("SP_INGAME", showWeaponsName3[cg.weaponSelect]), UI_CENTER | UI_SMALLFONT, colorTable[CT_ICON_BLUE]);
	}
	}
	else if(cg.snap->ps.eFlags & EF_WP_OPTION_2 )
	{
	if(showWeaponsName2[cg.weaponSelect])
	{
		UI_DrawProportionalString(320, y+45+yOffset, CG_GetStringEdString("SP_INGAME", showWeaponsName2[cg.weaponSelect]), UI_CENTER | UI_SMALLFONT, colorTable[CT_ICON_BLUE]);
	}
	}
	else	
	{
	if(showWeaponsName[cg.weaponSelect])
	{
		UI_DrawProportionalString(320, y+45+yOffset, CG_GetStringEdString("SP_INGAME", showWeaponsName[cg.weaponSelect]), UI_CENTER | UI_SMALLFONT, colorTable[CT_ICON_BLUE]);
	}
	}	
	

	trap_R_SetColor( NULL );
}


/*
===============
CG_NextWeapon_f
===============
*/
void CG_NextWeapon_f( void ) {
	int		i;
	int		original;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.emplacedIndex)
	{
		return;
	}

	cg.weaponSelectTime = cg.time;
	cg.delaySelectTime = cg.time+WEAPON_SELECT_DELAY_TIME;
	original = cg.weaponSelect;

	for ( i = 0 ; i < WP_NUM_WEAPONS ; i++ ) {
		//*SIGH*... Hack to put concussion rifle before rocketlauncher
		if ( cg.weaponSelect == WP_FLECHETTE )
		{
			cg.weaponSelect = WP_CONCUSSION;
		}
		else if ( cg.weaponSelect == WP_CONCUSSION )
		{
			cg.weaponSelect = WP_ROCKET_LAUNCHER;
		}
		else if ( cg.weaponSelect == WP_DET_PACK )
		{
			cg.weaponSelect = WP_BRYAR_OLD;
		}
		else
		{
			cg.weaponSelect++;
		}
		if ( cg.weaponSelect == WP_NUM_WEAPONS ) {
			cg.weaponSelect = 0;
		}
	//	if ( cg.weaponSelect == WP_STUN_BATON ) {
	//		continue;		// never cycle to gauntlet
	//	}
		if ( CG_WeaponSelectable( cg.weaponSelect ) ) {
			break;
		}
	}
	if ( i == WP_NUM_WEAPONS ) {
		cg.weaponSelect = original;
	}
	else
	{
		trap_S_MuteSound(cg.snap->ps.clientNum, CHAN_WEAPON);
	}
}

/*
===============
CG_PrevWeapon_f
===============
*/
void CG_PrevWeapon_f( void ) {
	int		i;
	int		original;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.emplacedIndex)
	{
		return;
	}

	cg.weaponSelectTime = cg.time;
	cg.delaySelectTime = cg.time+WEAPON_SELECT_DELAY_TIME;
	original = cg.weaponSelect;

	for ( i = 0 ; i < WP_NUM_WEAPONS ; i++ ) {
		//*SIGH*... Hack to put concussion rifle before rocketlauncher
		if ( cg.weaponSelect == WP_ROCKET_LAUNCHER )
		{
			cg.weaponSelect = WP_CONCUSSION;
		}
		else if ( cg.weaponSelect == WP_CONCUSSION )
		{
			cg.weaponSelect = WP_FLECHETTE;
		}
		else if ( cg.weaponSelect == WP_BRYAR_OLD )
		{
			cg.weaponSelect = WP_DET_PACK;
		}
		else
		{
			cg.weaponSelect--;
		}
		if ( cg.weaponSelect == -1 ) {
			cg.weaponSelect = WP_NUM_WEAPONS-1;
		}
	//	if ( cg.weaponSelect == WP_STUN_BATON ) {
	//		continue;		// never cycle to gauntlet
	//	}
		if ( CG_WeaponSelectable( cg.weaponSelect ) ) {
			break;
		}
	}
	if ( i == WP_NUM_WEAPONS ) {
		cg.weaponSelect = original;
	}
	else
	{
		trap_S_MuteSound(cg.snap->ps.clientNum, CHAN_WEAPON);
	}
}

/*
===============
CG_Weapon_f
===============
*/
void CG_Weapon_f( void ) {
	int		num;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	if (cg.snap->ps.emplacedIndex)
	{
		return;
	}

	num = atoi( CG_Argv( 1 ) );

	if ( num < 1 || num > LAST_USEABLE_WEAPON ) {
		return;
	}

	if (num == 1 && cg.snap->ps.weapon == WP_SABER)
	{
		//[MELEE]
		//Switch to melee when blade is toggled if ojp_sabermelee is on
		if (cg.snap->ps.weaponTime < 1)
		{
			if(ojp_sabermelee.integer && !cg.snap->ps.saberHolstered && CG_WeaponSelectable(WP_MELEE))
			{
				num = WP_MELEE;

				cg.weaponSelectTime = cg.time;
				cg.delaySelectTime = cg.time+WEAPON_SELECT_DELAY_TIME;
				
				if (cg.weaponSelect != num)
				{
					trap_S_MuteSound(cg.snap->ps.clientNum, CHAN_WEAPON);
				}

				cg.weaponSelect = num;
			}
			else
			{
				trap_SendConsoleCommand("sv_saberswitch\n");
			}
		}
		
		/*
		if (cg.snap->ps.weaponTime < 1)
		{
			trap_SendConsoleCommand("sv_saberswitch\n");
		}
		*/
		//[/MELEE]
		return;
	}

	//rww - hack to make weapon numbers same as single player
	if (num > WP_STUN_BATON)
	{
		//num++;
		num += 2; //I suppose this is getting kind of crazy, what with the wp_melee in there too now.
	}
	else
	{
		if (cg.snap->ps.stats[STAT_WEAPONS] & (1 << WP_SABER))
		{
			num = WP_SABER;
		}
		else
		{
			num = WP_MELEE;
		}
	}

	if (num > LAST_USEABLE_WEAPON+1)
	{ //other weapons are off limits due to not actually being weapon weapons
		return;
	}

	if (num >= WP_THERMAL && num <= WP_DET_PACK)
	{
		int weap, i = 0;

		if (cg.snap->ps.weapon >= WP_THERMAL &&
			cg.snap->ps.weapon <= WP_DET_PACK)
		{
			// already in cycle range so start with next cycle item
			weap = cg.snap->ps.weapon + 1;
		}
		else
		{
			// not in cycle range, so start with thermal detonator
			weap = WP_THERMAL;
		}

		// prevent an endless loop
		while ( i <= 4 )
		{
			if (weap > WP_DET_PACK)
			{
				weap = WP_THERMAL;
			}

			if (CG_WeaponSelectable(weap))
			{
				num = weap;
				break;
			}

			weap++;
			i++;
		}
	}

	if (!CG_WeaponSelectable(num))
	{
		return;
	}

	cg.weaponSelectTime = cg.time;
	cg.delaySelectTime = cg.time+WEAPON_SELECT_DELAY_TIME;

	if ( ! ( cg.snap->ps.stats[STAT_WEAPONS] & ( 1 << num ) ) )
	{
		if (num == WP_SABER)
		{ //don't have saber, try melee on the same slot
			num = WP_MELEE;

			if ( ! ( cg.snap->ps.stats[STAT_WEAPONS] & ( 1 << num ) ) )
			{
				return;
			}
		}
		else
		{
			return;		// don't have the weapon
		}
	}

	if (cg.weaponSelect != num)
	{
		trap_S_MuteSound(cg.snap->ps.clientNum, CHAN_WEAPON);
	}

	cg.weaponSelect = num;
}


//Version of the above which doesn't add +2 to a weapon.  The above can't
//triger WP_MELEE or WP_STUN_BATON.  Derogatory comments go here.
void CG_WeaponClean_f( void ) {
	int		num;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	if (cg.snap->ps.emplacedIndex)
	{
		return;
	}

	num = atoi( CG_Argv( 1 ) );

	if ( num < 1 || num > LAST_USEABLE_WEAPON ) {
		return;
	}

	if (num == 1 && cg.snap->ps.weapon == WP_SABER)
	{
		if (cg.snap->ps.weaponTime < 1)
		{
			trap_SendConsoleCommand("sv_saberswitch\n");
		}
		return;
	}

	if(num == WP_STUN_BATON) {
		if (cg.snap->ps.stats[STAT_WEAPONS] & (1 << WP_SABER))
		{
			num = WP_SABER;
		}
		else
		{
			num = WP_MELEE;
		}
	}

	if (num > LAST_USEABLE_WEAPON+1)
	{ //other weapons are off limits due to not actually being weapon weapons
		return;
	}

	if (num >= WP_THERMAL && num <= WP_DET_PACK)
	{
		int weap, i = 0;

		if (cg.snap->ps.weapon >= WP_THERMAL &&
			cg.snap->ps.weapon <= WP_DET_PACK)
		{
			// already in cycle range so start with next cycle item
			weap = cg.snap->ps.weapon + 1;
		}
		else
		{
			// not in cycle range, so start with thermal detonator
			weap = WP_THERMAL;
		}

		// prevent an endless loop
		while ( i <= 4 )
		{
			if (weap > WP_DET_PACK)
			{
				weap = WP_THERMAL;
			}

			if (CG_WeaponSelectable(weap))
			{
				num = weap;
				break;
			}

			weap++;
			i++;
		}
	}

	if (!CG_WeaponSelectable(num))
	{
		return;
	}

	cg.weaponSelectTime = cg.time;
	cg.delaySelectTime = cg.time+WEAPON_SELECT_DELAY_TIME;

	if ( ! ( cg.snap->ps.stats[STAT_WEAPONS] & ( 1 << num ) ) )
	{
		if (num == WP_SABER)
		{ //don't have saber, try melee on the same slot
			num = WP_MELEE;

			if ( ! ( cg.snap->ps.stats[STAT_WEAPONS] & ( 1 << num ) ) )
			{
				return;
			}
		}
		else
		{
			return;		// don't have the weapon
		}
	}

	if (cg.weaponSelect != num)
	{
		trap_S_MuteSound(cg.snap->ps.clientNum, CHAN_WEAPON);
	}

	cg.weaponSelect = num;
}



/*
===================
CG_OutOfAmmoChange

The current weapon has just run out of ammo
===================
*/
void CG_OutOfAmmoChange( int oldWeapon )
{

	int		i;

	return;
	cg.weaponSelectTime = cg.time;
	cg.delaySelectTime = cg.time+WEAPON_SELECT_DELAY_TIME;

	for ( i = LAST_USEABLE_WEAPON ; i > 0 ; i-- )	//We don't want the emplaced or turret
	{
		if ( CG_WeaponSelectable( i ) )
		{
			
			if ( 1 == cg_autoswitch.integer && 
				( i == WP_TRIP_MINE || i == WP_DET_PACK || i == WP_THERMAL || i == WP_ROCKET_LAUNCHER) ) // safe weapon switch
			
			//rww - Don't we want to make sure i != one of these if autoswitch is 1 (safe)?
			if (cg_autoswitch.integer != 1 || (i != WP_TRIP_MINE && i != WP_DET_PACK && i != WP_THERMAL && i != WP_ROCKET_LAUNCHER))
			{
				if (i != oldWeapon)
				{ //don't even do anything if we're just selecting the weapon we already have/had
					cg.weaponSelect = i;
					break;
				}
			}
		}
	}

	trap_S_MuteSound(cg.snap->ps.clientNum, CHAN_WEAPON);

}



/*
===================================================================================================

WEAPON EVENTS

===================================================================================================
*/
void CG_GetClientWeaponMuzzleBoltPoint(int clIndex, vec3_t to, qboolean leftweap)//[DualPistols]
//void CG_GetClientWeaponMuzzleBoltPoint(int clIndex, vec3_t to)
{
	centity_t *cent;
	mdxaBone_t	boltMatrix;

	//[DualPistols]
	int	midx = (leftweap ? 2 : 1);

	if (clIndex < 0 || clIndex >= MAX_CLIENTS)
	{
		return;
	}

	cent = &cg_entities[clIndex];

	if (!cent || !cent->ghoul2 || !trap_G2_HaveWeGhoul2Models(cent->ghoul2) ||
		!trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), midx))
	{
		return;
	}

	trap_G2API_GetBoltMatrix(cent->ghoul2, midx, 0, &boltMatrix, cent->turAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, to);
}

/*
================
CG_FireWeapon

Caused by an EV_FIRE_WEAPON event
================
*/
void CG_FireWeapon( centity_t *cent, qboolean altFire ) {
	entityState_t *ent;
	int				c;
	weaponInfo_t	*weap;
	int mishap = cg.predictedPlayerState.MISHAP_VARIABLE;
	ent = &cent->currentState;
	if ( ent->weapon == WP_NONE ) {
		return;
	}

//	if(!altFire && ent->weapon == WP_FLECHETTE && cg.predictedPlayerState.weaponTime != 100000)
//		return;

	if ( ent->weapon >= WP_NUM_WEAPONS ) {
		CG_Error( "CG_FireWeapon: ent->weapon >= WP_NUM_WEAPONS" );
		return;
	}
	weap = &cg_weapons[ ent->weapon ];

	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model
	cent->muzzleFlashTime = cg.time;

	if (cg.predictedPlayerState.clientNum == cent->currentState.number)
	{/*
		if ((ent->weapon == WP_BRYAR_PISTOL && altFire) ||
			(ent->weapon == WP_BRYAR_OLD && altFire) ||
			(ent->weapon == WP_BOWCASTER && !altFire) ||	
			(ent->weapon == WP_DEMP2 && altFire))*/
		if ((ent->weapon == WP_BRYAR_PISTOL && altFire) ||
				(ent->weapon == WP_BRYAR_OLD && altFire) ||
				(ent->weapon == WP_BOWCASTER && !altFire) ||	
				(ent->weapon == WP_DEMP2 && altFire)
				|| mishap >= 5)
		{
			float val = 0.1;

			if (val > 1.0)
			{
				val = 1.0;
			}
			if (val < 0.1)
			{
				val = 0.1;
			}


			if(mishap >= 5 && mishap <= 8)
				val *= 1;
			else if(mishap > 8 && mishap <= 14)
				val *= 2;
			else if(mishap > 14)
				val *= 3;
			CGCam_Shake( val, 250 );
		}
		else if (ent->weapon == WP_ROCKET_LAUNCHER ||
			(ent->weapon == WP_REPEATER && altFire) ||
			ent->weapon == WP_FLECHETTE ||
			(ent->weapon == WP_CONCUSSION && !altFire))
		{
			float val = 0.1;
			if (ent->weapon == WP_CONCUSSION)
			{
				if (!cg.renderingThirdPerson )//gives an advantage to being in 3rd person, but would look silly otherwise
				{//kick the view back
					cg.kick_angles[PITCH] = flrand( -10, -15 );
					cg.kick_time = cg.time;
				}
			}
			else if (ent->weapon == WP_ROCKET_LAUNCHER)
			{
				val *= 5;
				CGCam_Shake(val, 250);
			}
			else if (ent->weapon == WP_REPEATER || ent->weapon == WP_FLECHETTE)
			{
				val *= 3;
				CGCam_Shake(val, 350);
			}
		}
	}
	// lightning gun only does this this on initial press
	if ( ent->weapon == WP_DEMP2 ) {
		if ( cent->pe.lightningFiring ) {
			return;
		}
	}

	// play quad sound if needed
	//if ( cent->currentState.powerups & ( 1 << PW_OVERLOADED ) ) {
	//	trap_S_StartSound (NULL, cent->currentState.number, CHAN_ITEM, trap_S_RegisterSound("sound/weapons/force/rage.wav") );
	//}

	if(altFire && ent->weapon == WP_BOWCASTER)
		return;
	if(altFire && cent->currentState.eFlags2& EF2_NOALTFIRE)
		return;		
	// play a sound
	if (altFire)
	{
		// play a sound
		for ( c = 0 ; c < 4 ; c++ ) {
			if ( !weap->altFlashSound[c] ) {
				break;
			}
		}
		if ( c > 0 ) {
			c = rand() % c;

				if(cent->currentState.eFlags & EF_WP_OPTION_2 && cent->currentState.eFlags & EF_WP_OPTION_4)
				{
				if(weap->altFlashSound6[c])
				{
				trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->altFlashSound6[c] );
				}
				}
				else if(cent->currentState.eFlags & EF_WP_OPTION_2 && cent->currentState.eFlags & EF_WP_OPTION_3 )
				{
				if(weap->altFlashSound5[c])
				{
				trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->altFlashSound5[c] );
				}
				}
				else if(cent->currentState.eFlags & EF_WP_OPTION_4 )
				{
				if(weap->altFlashSound4[c])
				{
				trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->altFlashSound4[c] );
				}
				}
				else if(cent->currentState.eFlags & EF_WP_OPTION_3 )
				{
				if(weap->altFlashSound3[c])
				{
				trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->altFlashSound3[c] );
				}
				}
				else if(cent->currentState.eFlags & EF_WP_OPTION_2 )
				{
				if(weap->altFlashSound2[c])
				{
				trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->altFlashSound2[c] );
				}
				}	
				else
				{
				if(weap->altFlashSound[c])
				{
				trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->altFlashSound[c] );
				}
				}

		}
//		if ( weap->altFlashSnd )
//		{
//			trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->altFlashSnd );
//		}
	}
	else
	{
		// play a sound
		for ( c = 0 ; c < 4 ; c++ ) {
			if ( !weap->flashSound[c] ) {
				break;
			}
		}
		if ( c > 0 ) {
			c = rand() % c;

				if(cent->currentState.eFlags & EF_WP_OPTION_2 && cent->currentState.eFlags & EF_WP_OPTION_4 )
				{
				if(weap->flashSound6[c])
				{
				trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->flashSound6[c] );
				}
				}
				else if(cent->currentState.eFlags & EF_WP_OPTION_2 && cent->currentState.eFlags & EF_WP_OPTION_3 )
				{
				if(weap->flashSound5[c])
				{
				trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->flashSound5[c] );
				}
				}
				else if(cent->currentState.eFlags & EF_WP_OPTION_4 )
				{
				if(weap->flashSound4[c])
				{
				trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->flashSound4[c] );
				}
				}
				else if(cent->currentState.eFlags & EF_WP_OPTION_3 )
				{
				if(weap->flashSound3[c])
				{
				trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->flashSound3[c] );
				}
				}
				else if(cent->currentState.eFlags & EF_WP_OPTION_2 )
				{
				if(weap->flashSound2[c])
				{
				trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->flashSound2[c] );
				}
				}
				else
				{
				if(weap->flashSound[c])
				{
				trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->flashSound[c] );
				}
				}
			
		}
	}
}

qboolean CG_VehicleWeaponImpact( centity_t *cent )
{//see if this is a missile entity that's owned by a vehicle and should do a special, overridden impact effect
	if ((cent->currentState.eFlags&EF_JETPACK_ACTIVE)//hack so we know we're a vehicle Weapon shot
		&& cent->currentState.otherEntityNum2
		&& g_vehWeaponInfo[cent->currentState.otherEntityNum2].iImpactFX)
	{//missile is from a special vehWeapon
		vec3_t normal;
		ByteToDir( cent->currentState.eventParm, normal );

		trap_FX_PlayEffectID( g_vehWeaponInfo[cent->currentState.otherEntityNum2].iImpactFX, cent->lerpOrigin, normal, -1, -1 );
		return qtrue;
	}
	return qfalse;
}

/*
=================
CG_MissileHitWall

Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing
=================
*/
void CG_MissileHitWall(int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType, qboolean altFire, int charge) 
{
	int parm;
	vec3_t up={0,0,1};

	switch( weapon )
	{
	case WP_STUN_BATON:
		FX_DestructionHitWall( origin, dir );
		break;
	case WP_BRYAR_PISTOL:
		if ( altFire )
		{
			parm = charge;
			FX_BryarAltHitWall( origin, dir, parm );
		}
		else
		{
			FX_BryarHitWall( origin, dir );
		}
		break;

	case WP_CONCUSSION:
		FX_ConcussionHitWall( origin, dir );
		break;

	case WP_BRYAR_OLD:
		if ( altFire )
		{
			parm = charge;
			FX_BryarOldAltHitWall( origin, dir, parm );
		}
		else
		{
			FX_BryarOldHitWall( origin, dir );
		}
		break;

	case WP_TURRET:
		FX_TurretHitWall( origin, dir );
		break;

	case WP_BLASTER:
		FX_BlasterWeaponHitWall( origin, dir );
		break;

	case WP_DISRUPTOR:
		FX_DisruptorAltMiss( origin, dir );
		break;

	case WP_BOWCASTER:
		FX_BowcasterHitWall( origin, dir );
		break;

	case WP_REPEATER:
		if ( altFire )
		{
			FX_RepeaterAltHitWall( origin, dir );
		}
		else
		{
			FX_RepeaterHitWall( origin, dir );
		}
		break;

	case WP_DEMP2:
		if (altFire)
		{
			trap_FX_PlayEffectID(cgs.effects.mAltDetonate, origin, dir, -1, -1);
		}
		else
		{
			FX_DEMP2_HitWall( origin, dir );
		}
		break;

	case WP_FLECHETTE:
		/*if (altFire)
		{
			CG_SurfaceExplosion(origin, dir, 20.0f, 12.0f, qtrue);
		}
		else
		*/
		if (!altFire)
		{
			FX_FlechetteWeaponHitWall( origin, dir );
		}
		break;

	case WP_ROCKET_LAUNCHER:
		trap_FX_PlayEffectID( cgs.effects.incineraryExplosionEffect, origin, dir, -1, -1 );

		break;

	case WP_THERMAL:
		trap_FX_PlayEffectID( cgs.effects.thermalExplosionEffect, origin, dir, -1, -1 );
		break;

	case WP_EMPLACED_GUN:
		//FIXME: Its own effect?
		FX_BlasterWeaponHitWall( origin, dir );
		break;



	}
}

void CG_MissileHitWall2(int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType, qboolean altFire, int charge) 
{
	int parm;
	vec3_t up={0,0,1};

	switch( weapon )
	{
	case WP_STUN_BATON:
		FX_DestructionHitWall( origin, dir );
		break;
	case WP_BRYAR_PISTOL:
		if ( altFire )
		{
			parm = charge;
			FX_BryarAltHitWall( origin, dir, parm );
		}
		else
		{
			FX_BryarHitWall( origin, dir );
		}
		break;

	case WP_CONCUSSION:
		FX_ConcussionHitWall2( origin, dir );	
		break;

	case WP_BRYAR_OLD:
		if ( altFire )
		{
			parm = charge;
			FX_BryarOldAltHitWall( origin, dir, parm );
		}
		else
		{
			FX_BryarOldHitWall( origin, dir );
		}
		break;

	case WP_TURRET:
		FX_TurretHitWall( origin, dir );
		break;

	case WP_BLASTER:
		FX_BlasterWeaponHitWall( origin, dir );
		break;

	case WP_DISRUPTOR:
		FX_DisruptorAltMiss( origin, dir );
		break;

	case WP_BOWCASTER:
		FX_BowcasterHitWall( origin, dir );
		break;

	case WP_REPEATER:
		if ( altFire )
		{
			FX_RepeaterAltHitWall2( origin, dir );
		}
		else
		{
			FX_RepeaterHitWall( origin, dir );
		}
		break;

	case WP_DEMP2:
			FX_DEMP2_HitWall2( origin, dir );
		break;

	case WP_FLECHETTE:
		/*if (altFire)
		{
			CG_SurfaceExplosion(origin, dir, 20.0f, 12.0f, qtrue);
		}
		else
		*/
		if (!altFire)
		{
			FX_FlechetteWeaponHitWall( origin, dir );
		}
		break;

	case WP_ROCKET_LAUNCHER:
		trap_FX_PlayEffectID( cgs.effects.thermalExplosionEffect, origin, dir, -1, -1 );
		break;

	case WP_THERMAL:
		trap_FX_PlayEffectID(cgs.effects.incineraryExplosionEffect, origin, dir, -1, -1);
		break;

	case WP_EMPLACED_GUN:
		//FIXME: Its own effect?
		FX_BlasterWeaponHitWall( origin, dir );
		break;



	}
}

void CG_MissileHitWall3(int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType, qboolean altFire, int charge) 
{
	int parm;
	vec3_t up={0,0,1};

	switch( weapon )
	{
	case WP_STUN_BATON:
		FX_DestructionHitWall( origin, dir );
		break;
	case WP_BRYAR_PISTOL:
		if ( altFire )
		{
			parm = charge;
			FX_BryarAltHitWall( origin, dir, parm );
		}
		else
		{
			FX_BryarHitWall( origin, dir );
		}
		break;

	case WP_CONCUSSION:
		FX_ConcussionHitWall3( origin, dir );

		break;

	case WP_BRYAR_OLD:
		if ( altFire )
		{
			parm = charge;
			FX_BryarOldAltHitWall( origin, dir, parm );
		}
		else
		{
			FX_BryarOldHitWall( origin, dir );
		}
		break;

	case WP_TURRET:
		FX_TurretHitWall( origin, dir );
		break;

	case WP_BLASTER:
		FX_BlasterWeaponHitWall( origin, dir );
		break;

	case WP_DISRUPTOR:
		FX_DisruptorAltMiss( origin, dir );
		break;

	case WP_BOWCASTER:
		FX_BowcasterHitWall( origin, dir );
		break;

	case WP_REPEATER:
		if ( altFire )
		{
			FX_RepeaterAltHitWall3( origin, dir );
		}
		else
		{
			FX_RepeaterHitWall( origin, dir );
		}
		break;

	case WP_DEMP2:
		FX_DEMP2_HitWall3( origin, dir );
		break;

	case WP_FLECHETTE:
		/*if (altFire)
		{
			CG_SurfaceExplosion(origin, dir, 20.0f, 12.0f, qtrue);
		}
		else
		*/
		if (!altFire)
		{
			FX_FlechetteWeaponHitWall( origin, dir );
		}
		break;

	case WP_ROCKET_LAUNCHER:
		trap_FX_PlayEffectID( cgs.effects.thermalExplosionEffect, origin, dir, -1, -1 );
		break;

	case WP_THERMAL:		
		trap_FX_PlayEffectID( cgs.effects.dioxisExplosionEffect, origin, dir, -1, -1 );
		break;

	case WP_EMPLACED_GUN:
		//FIXME: Its own effect?
		FX_BlasterWeaponHitWall( origin, dir );
		break;



	}
}

void CG_MissileHitWall4(int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType, qboolean altFire, int charge) 
{
	int parm;
	vec3_t up={0,0,1};

	switch( weapon )
	{
	case WP_STUN_BATON:
		FX_DestructionHitWall( origin, dir );
		break;
	case WP_BRYAR_PISTOL:
		if ( altFire )
		{
			parm = charge;
			FX_BryarAltHitWall( origin, dir, parm );
		}
		else
		{
			FX_BryarHitWall( origin, dir );
		}
		break;

	case WP_CONCUSSION:
		FX_ConcussionHitWall4( origin, dir );
		break;

	case WP_BRYAR_OLD:
		if ( altFire )
		{
			parm = charge;
			FX_BryarOldAltHitWall( origin, dir, parm );
		}
		else
		{
			FX_BryarOldHitWall( origin, dir );
		}
		break;

	case WP_TURRET:
		FX_TurretHitWall( origin, dir );
		break;

	case WP_BLASTER:
		FX_BlasterWeaponHitWall( origin, dir );
		break;

	case WP_DISRUPTOR:
		FX_DisruptorAltMiss( origin, dir );
		break;

	case WP_BOWCASTER:
		FX_BowcasterHitWall( origin, dir );
		break;

	case WP_REPEATER:
		if ( altFire )
		{
			FX_RepeaterAltHitWall4( origin, dir );
		}
		else
		{
			FX_RepeaterHitWall( origin, dir );
		}
		break;

	case WP_DEMP2:
		FX_DEMP2_HitWall4( origin, dir );
		break;

	case WP_FLECHETTE:
		/*if (altFire)
		{
			CG_SurfaceExplosion(origin, dir, 20.0f, 12.0f, qtrue);
		}
		else
		*/
		if (!altFire)
		{
			FX_FlechetteWeaponHitWall( origin, dir );
		}
		break;

	case WP_ROCKET_LAUNCHER:
		trap_FX_PlayEffectID( cgs.effects.incineraryExplosionEffect, origin, dir, -1, -1 );
		break;

	case WP_THERMAL:
		trap_FX_PlayEffectID( cgs.effects.ionExplosionEffect	, origin, dir, -1, -1 );
		break;

	case WP_EMPLACED_GUN:
		//FIXME: Its own effect?
		FX_BlasterWeaponHitWall( origin, dir );
		break;



	}
}

void CG_MissileHitWall5(int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType, qboolean altFire, int charge) 
{
	int parm;
	vec3_t up={0,0,1};

	switch( weapon )
	{
	case WP_STUN_BATON:
		FX_DestructionHitWall( origin, dir );
		break;
	case WP_BRYAR_PISTOL:
		if ( altFire )
		{
			parm = charge;
			FX_BryarAltHitWall( origin, dir, parm );
		}
		else
		{
			FX_BryarHitWall( origin, dir );
		}
		break;

	case WP_CONCUSSION:
		FX_ConcussionHitWall5( origin, dir );
		break;

	case WP_BRYAR_OLD:
		if ( altFire )
		{
			parm = charge;
			FX_BryarOldAltHitWall( origin, dir, parm );
		}
		else
		{
			FX_BryarOldHitWall( origin, dir );
		}
		break;

	case WP_TURRET:
		FX_TurretHitWall( origin, dir );
		break;

	case WP_BLASTER:
		FX_BlasterWeaponHitWall( origin, dir );
		break;

	case WP_DISRUPTOR:
		FX_DisruptorAltMiss( origin, dir );
		break;

	case WP_BOWCASTER:
		FX_BowcasterHitWall( origin, dir );
		break;

	case WP_REPEATER:
		if ( altFire )
		{
			FX_RepeaterAltHitWall5( origin, dir );
		}
		else
		{
			FX_RepeaterHitWall( origin, dir );
		}
		break;

	case WP_DEMP2:
		FX_DEMP2_HitWall5( origin, dir );
		break;

	case WP_FLECHETTE:
		/*if (altFire)
		{
			CG_SurfaceExplosion(origin, dir, 20.0f, 12.0f, qtrue);
		}
		else
		*/
		if (!altFire)
		{
			FX_FlechetteWeaponHitWall( origin, dir );
		}
		break;

	case WP_ROCKET_LAUNCHER:
		trap_FX_PlayEffectID( cgs.effects.thermalExplosionEffect, origin, dir, -1, -1 );
		break;

	case WP_THERMAL:
		trap_FX_PlayEffectID( cgs.effects.sonicExplosionEffect	, origin, dir, -1, -1 );
		break;

	case WP_EMPLACED_GUN:
		//FIXME: Its own effect?
		FX_BlasterWeaponHitWall( origin, dir );
		break;



	}
}

void CG_MissileHitWall6(int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType, qboolean altFire, int charge) 
{
	int parm;
	vec3_t up={0,0,1};

	switch( weapon )
	{
	case WP_STUN_BATON:
		FX_DestructionHitWall( origin, dir );
		break;
	case WP_BRYAR_PISTOL:
		if ( altFire )
		{
			parm = charge;
			FX_BryarAltHitWall( origin, dir, parm );
		}
		else
		{
			FX_BryarHitWall( origin, dir );
		}
		break;

	case WP_CONCUSSION:
		FX_ConcussionHitWall6( origin, dir );
		break;

	case WP_BRYAR_OLD:
		if ( altFire )
		{
			parm = charge;
			FX_BryarOldAltHitWall( origin, dir, parm );
		}
		else
		{
			FX_BryarOldHitWall( origin, dir );
		}
		break;

	case WP_TURRET:
		FX_TurretHitWall( origin, dir );
		break;

	case WP_BLASTER:
		FX_BlasterWeaponHitWall( origin, dir );
		break;

	case WP_DISRUPTOR:
		FX_DisruptorAltMiss( origin, dir );
		break;

	case WP_BOWCASTER:
		FX_BowcasterHitWall( origin, dir );
		break;

	case WP_REPEATER:
		if ( altFire )
		{
			FX_RepeaterAltHitWall6( origin, dir );
		}
		else
		{
			FX_RepeaterHitWall( origin, dir );
		}
		break;

	case WP_DEMP2:
		FX_DEMP2_HitWall6( origin, dir );
		break;

	case WP_FLECHETTE:
		/*if (altFire)
		{
			CG_SurfaceExplosion(origin, dir, 20.0f, 12.0f, qtrue);
		}
		else
		*/
		if (!altFire)
		{
			FX_FlechetteWeaponHitWall( origin, dir );
		}
		break;

	case WP_ROCKET_LAUNCHER:
		trap_FX_PlayEffectID( cgs.effects.incineraryExplosionEffect, origin, dir, -1, -1 );
		break;

	case WP_THERMAL:
		trap_FX_PlayEffectID( cgs.effects.flashExplosionEffect	, origin, dir, -1, -1 );
		break;

	case WP_EMPLACED_GUN:
		//FIXME: Its own effect?
		FX_BlasterWeaponHitWall( origin, dir );
		break;



	}
}
/*
=================
CG_MissileHitPlayer
=================
*/
void CG_MissileHitPlayer(int weapon, vec3_t origin, vec3_t dir, int entityNum, qboolean altFire) 
{
	qboolean	humanoid = qtrue;
	vec3_t up={0,0,1};

	/*
	// NOTENOTE Non-portable code from single player
	if ( cent->gent )
	{
		other = &g_entities[cent->gent->s.otherEntityNum];

		if ( other->client && other->client->playerTeam == TEAM_BOTS )
		{
			humanoid = qfalse;
		}
	}
	*/	

	// NOTENOTE No bleeding in this game
//	CG_Bleed( origin, entityNum );

	// some weapons will make an explosion with the blood, while
	// others will just make the blood
	switch ( weapon ) {
	case WP_STUN_BATON:
		FX_DestructionHitPlayer( origin, dir, humanoid );
		break;
	case WP_BRYAR_PISTOL:
		if ( altFire )
		{
			FX_BryarAltHitPlayer( origin, dir, humanoid );
		}
		else
		{
			FX_BryarHitPlayer( origin, dir, humanoid );
		}
		break;

	case WP_CONCUSSION:

		FX_ConcussionHitPlayer( origin, dir, humanoid );	
		break;

	case WP_BRYAR_OLD:
		if ( altFire )
		{
			FX_BryarOldAltHitPlayer( origin, dir, humanoid );
		}
		else
		{
			FX_BryarOldHitPlayer( origin, dir, humanoid );
		}
		break;

	case WP_TURRET:
		FX_TurretHitPlayer( origin, dir, humanoid );
		break;

	case WP_BLASTER:
		FX_BlasterWeaponHitPlayer( origin, dir, humanoid );
		break;

	case WP_DISRUPTOR:
		FX_DisruptorAltHit( origin, dir);
		break;

	case WP_BOWCASTER:
		FX_BowcasterHitPlayer( origin, dir, humanoid );
		break;

	case WP_REPEATER:
		if ( altFire )
		{
			FX_RepeaterAltHitPlayer( origin, dir, humanoid );
		}
		else
		{
			FX_RepeaterHitPlayer( origin, dir, humanoid );
		}
		break;

	case WP_DEMP2:
		// Do a full body effect here for some more feedback
		// NOTENOTE The chaining of the demp2 is not yet implemented.
		/*
		if ( other )
		{
			other->s.powerups |= ( 1 << PW_DISINT_1 );
			other->client->ps.powerups[PW_DISINT_1] = cg.time + 650;
		}
		*/
			FX_DEMP2_HitPlayer( origin, dir, humanoid );
		break;

	case WP_FLECHETTE:
		FX_FlechetteWeaponHitPlayer( origin, dir, humanoid );
		break;

	case WP_ROCKET_LAUNCHER:
		trap_FX_PlayEffectID( cgs.effects.incineraryExplosionEffect, origin, dir, -1, -1 );
		break;
	case WP_THERMAL:
		trap_FX_PlayEffectID( cgs.effects.thermalExplosionEffect, origin, dir, -1, -1 );
		break;
	case WP_EMPLACED_GUN:
		//FIXME: Its own effect?
		FX_BlasterWeaponHitPlayer( origin, dir, humanoid );
		break;

	default:
		break;
	}
}
void CG_MissileHitPlayer2(int weapon, vec3_t origin, vec3_t dir, int entityNum, qboolean altFire) 
{
	qboolean	humanoid = qtrue;
	vec3_t up={0,0,1};

	/*
	// NOTENOTE Non-portable code from single player
	if ( cent->gent )
	{
		other = &g_entities[cent->gent->s.otherEntityNum];

		if ( other->client && other->client->playerTeam == TEAM_BOTS )
		{
			humanoid = qfalse;
		}
	}
	*/	

	// NOTENOTE No bleeding in this game
//	CG_Bleed( origin, entityNum );

	// some weapons will make an explosion with the blood, while
	// others will just make the blood
	switch ( weapon ) {
	case WP_STUN_BATON:
		FX_DestructionHitPlayer( origin, dir, humanoid );
		break;
	case WP_BRYAR_PISTOL:
		if ( altFire )
		{
			FX_BryarAltHitPlayer( origin, dir, humanoid );
		}
		else
		{
			FX_BryarHitPlayer( origin, dir, humanoid );
		}
		break;

	case WP_CONCUSSION:
		FX_ConcussionHitPlayer2( origin, dir, humanoid );
		break;

	case WP_BRYAR_OLD:
		if ( altFire )
		{
			FX_BryarOldAltHitPlayer( origin, dir, humanoid );
		}
		else
		{
			FX_BryarOldHitPlayer( origin, dir, humanoid );
		}
		break;

	case WP_TURRET:
		FX_TurretHitPlayer( origin, dir, humanoid );
		break;

	case WP_BLASTER:
		FX_BlasterWeaponHitPlayer( origin, dir, humanoid );
		break;

	case WP_DISRUPTOR:
		FX_DisruptorAltHit( origin, dir);
		break;

	case WP_BOWCASTER:
		FX_BowcasterHitPlayer( origin, dir, humanoid );
		break;

	case WP_REPEATER:
		if ( altFire )
		{
			FX_RepeaterAltHitPlayer2( origin, dir, humanoid );
		}
		else
		{
			FX_RepeaterHitPlayer( origin, dir, humanoid );
		}
		break;

	case WP_DEMP2:
		// Do a full body effect here for some more feedback
		// NOTENOTE The chaining of the demp2 is not yet implemented.
		/*
		if ( other )
		{
			other->s.powerups |= ( 1 << PW_DISINT_1 );
			other->client->ps.powerups[PW_DISINT_1] = cg.time + 650;
		}
		*/
			FX_DEMP2_HitPlayer2( origin, dir, humanoid );		
		break;

	case WP_FLECHETTE:
		FX_FlechetteWeaponHitPlayer( origin, dir, humanoid );
		break;

	case WP_ROCKET_LAUNCHER:
		trap_FX_PlayEffectID( cgs.effects.thermalExplosionEffect, origin, dir, -1, -1 );

		break;
	case WP_THERMAL:
		trap_FX_PlayEffectID(cgs.effects.incineraryExplosionEffect, origin, dir, -1, -1);
		break;
	case WP_EMPLACED_GUN:
		//FIXME: Its own effect?
		FX_BlasterWeaponHitPlayer( origin, dir, humanoid );
		break;

	default:
		break;
	}
}
void CG_MissileHitPlayer3(int weapon, vec3_t origin, vec3_t dir, int entityNum, qboolean altFire) 
{
	qboolean	humanoid = qtrue;
	vec3_t up={0,0,1};

	/*
	// NOTENOTE Non-portable code from single player
	if ( cent->gent )
	{
		other = &g_entities[cent->gent->s.otherEntityNum];

		if ( other->client && other->client->playerTeam == TEAM_BOTS )
		{
			humanoid = qfalse;
		}
	}
	*/	

	// NOTENOTE No bleeding in this game
//	CG_Bleed( origin, entityNum );

	// some weapons will make an explosion with the blood, while
	// others will just make the blood
	switch ( weapon ) {
	case WP_STUN_BATON:
		FX_DestructionHitPlayer( origin, dir, humanoid );
		break;
	case WP_BRYAR_PISTOL:
		if ( altFire )
		{
			FX_BryarAltHitPlayer( origin, dir, humanoid );
		}
		else
		{
			FX_BryarHitPlayer( origin, dir, humanoid );
		}
		break;

	case WP_CONCUSSION:
		FX_ConcussionHitPlayer3( origin, dir, humanoid );
		break;

	case WP_BRYAR_OLD:
		if ( altFire )
		{
			FX_BryarOldAltHitPlayer( origin, dir, humanoid );
		}
		else
		{
			FX_BryarOldHitPlayer( origin, dir, humanoid );
		}
		break;

	case WP_TURRET:
		FX_TurretHitPlayer( origin, dir, humanoid );
		break;

	case WP_BLASTER:
		FX_BlasterWeaponHitPlayer( origin, dir, humanoid );
		break;

	case WP_DISRUPTOR:
		FX_DisruptorAltHit( origin, dir);
		break;

	case WP_BOWCASTER:
		FX_BowcasterHitPlayer( origin, dir, humanoid );
		break;

	case WP_REPEATER:
		if ( altFire )
		{
			FX_RepeaterAltHitPlayer3( origin, dir, humanoid );
		}
		else
		{
			FX_RepeaterHitPlayer( origin, dir, humanoid );
		}
		break;

	case WP_DEMP2:
		// Do a full body effect here for some more feedback
		// NOTENOTE The chaining of the demp2 is not yet implemented.
		/*
		if ( other )
		{
			other->s.powerups |= ( 1 << PW_DISINT_1 );
			other->client->ps.powerups[PW_DISINT_1] = cg.time + 650;
		}
		*/
			FX_DEMP2_HitPlayer3( origin, dir, humanoid );	
		break;

	case WP_FLECHETTE:
		FX_FlechetteWeaponHitPlayer( origin, dir, humanoid );
		break;

	case WP_ROCKET_LAUNCHER:
		trap_FX_PlayEffectID( cgs.effects.thermalExplosionEffect, origin, dir, -1, -1 );	
		break;
	case WP_THERMAL:
		trap_FX_PlayEffectID( cgs.effects.dioxisExplosionEffect, origin, dir, -1, -1 );
		break;
	case WP_EMPLACED_GUN:
		//FIXME: Its own effect?
		FX_BlasterWeaponHitPlayer( origin, dir, humanoid );
		break;

	default:
		break;
	}
}
void CG_MissileHitPlayer4(int weapon, vec3_t origin, vec3_t dir, int entityNum, qboolean altFire) 
{
	qboolean	humanoid = qtrue;
	vec3_t up={0,0,1};

	/*
	// NOTENOTE Non-portable code from single player
	if ( cent->gent )
	{
		other = &g_entities[cent->gent->s.otherEntityNum];

		if ( other->client && other->client->playerTeam == TEAM_BOTS )
		{
			humanoid = qfalse;
		}
	}
	*/	

	// NOTENOTE No bleeding in this game
//	CG_Bleed( origin, entityNum );

	// some weapons will make an explosion with the blood, while
	// others will just make the blood
	switch ( weapon ) {
	case WP_STUN_BATON:
		FX_DestructionHitPlayer( origin, dir, humanoid );
		break;
	case WP_BRYAR_PISTOL:
		if ( altFire )
		{
			FX_BryarAltHitPlayer( origin, dir, humanoid );
		}
		else
		{
			FX_BryarHitPlayer( origin, dir, humanoid );
		}
		break;

	case WP_CONCUSSION:

		FX_ConcussionHitPlayer4( origin, dir, humanoid );	
		break;

	case WP_BRYAR_OLD:
		if ( altFire )
		{
			FX_BryarOldAltHitPlayer( origin, dir, humanoid );
		}
		else
		{
			FX_BryarOldHitPlayer( origin, dir, humanoid );
		}
		break;

	case WP_TURRET:
		FX_TurretHitPlayer( origin, dir, humanoid );
		break;

	case WP_BLASTER:
		FX_BlasterWeaponHitPlayer( origin, dir, humanoid );
		break;

	case WP_DISRUPTOR:
		FX_DisruptorAltHit( origin, dir);
		break;

	case WP_BOWCASTER:
		FX_BowcasterHitPlayer( origin, dir, humanoid );
		break;

	case WP_REPEATER:
		if ( altFire )
		{
			FX_RepeaterAltHitPlayer4( origin, dir, humanoid );
		}
		else
		{
			FX_RepeaterHitPlayer( origin, dir, humanoid );
		}
		break;

	case WP_DEMP2:
		// Do a full body effect here for some more feedback
		// NOTENOTE The chaining of the demp2 is not yet implemented.
		/*
		if ( other )
		{
			other->s.powerups |= ( 1 << PW_DISINT_1 );
			other->client->ps.powerups[PW_DISINT_1] = cg.time + 650;
		}
		*/
			FX_DEMP2_HitPlayer4( origin, dir, humanoid );	
		break;

	case WP_FLECHETTE:
		FX_FlechetteWeaponHitPlayer( origin, dir, humanoid );
		break;

	case WP_ROCKET_LAUNCHER:
		trap_FX_PlayEffectID( cgs.effects.incineraryExplosionEffect, origin, dir, -1, -1 );
		break;
	case WP_THERMAL:
		trap_FX_PlayEffectID( cgs.effects.ionExplosionEffect	, origin, dir, -1, -1 );
		break;
	case WP_EMPLACED_GUN:
		//FIXME: Its own effect?
		FX_BlasterWeaponHitPlayer( origin, dir, humanoid );
		break;

	default:
		break;
	}
}

void CG_MissileHitPlayer5(int weapon, vec3_t origin, vec3_t dir, int entityNum, qboolean altFire) 
{
	qboolean	humanoid = qtrue;
	vec3_t up={0,0,1};

	/*
	// NOTENOTE Non-portable code from single player
	if ( cent->gent )
	{
		other = &g_entities[cent->gent->s.otherEntityNum];

		if ( other->client && other->client->playerTeam == TEAM_BOTS )
		{
			humanoid = qfalse;
		}
	}
	*/	

	// NOTENOTE No bleeding in this game
//	CG_Bleed( origin, entityNum );

	// some weapons will make an explosion with the blood, while
	// others will just make the blood
	switch ( weapon ) {
	case WP_STUN_BATON:
		FX_DestructionHitPlayer( origin, dir, humanoid );
		break;
	case WP_BRYAR_PISTOL:
		if ( altFire )
		{
			FX_BryarAltHitPlayer( origin, dir, humanoid );
		}
		else
		{
			FX_BryarHitPlayer( origin, dir, humanoid );
		}
		break;

	case WP_CONCUSSION:

		FX_ConcussionHitPlayer5( origin, dir, humanoid );	
		break;

	case WP_BRYAR_OLD:
		if ( altFire )
		{
			FX_BryarOldAltHitPlayer( origin, dir, humanoid );
		}
		else
		{
			FX_BryarOldHitPlayer( origin, dir, humanoid );
		}
		break;

	case WP_TURRET:
		FX_TurretHitPlayer( origin, dir, humanoid );
		break;

	case WP_BLASTER:
		FX_BlasterWeaponHitPlayer( origin, dir, humanoid );
		break;

	case WP_DISRUPTOR:
		FX_DisruptorAltHit( origin, dir);
		break;

	case WP_BOWCASTER:
		FX_BowcasterHitPlayer( origin, dir, humanoid );
		break;

	case WP_REPEATER:
		if ( altFire )
		{
			FX_RepeaterAltHitPlayer5( origin, dir, humanoid );
		}
		else
		{
			FX_RepeaterHitPlayer( origin, dir, humanoid );
		}
		break;

	case WP_DEMP2:
		// Do a full body effect here for some more feedback
		// NOTENOTE The chaining of the demp2 is not yet implemented.
		/*
		if ( other )
		{
			other->s.powerups |= ( 1 << PW_DISINT_1 );
			other->client->ps.powerups[PW_DISINT_1] = cg.time + 650;
		}
		*/
			FX_DEMP2_HitPlayer5( origin, dir, humanoid );	
		break;

	case WP_FLECHETTE:
		FX_FlechetteWeaponHitPlayer( origin, dir, humanoid );
		break;

	case WP_ROCKET_LAUNCHER:
		trap_FX_PlayEffectID( cgs.effects.thermalExplosionEffect, origin, dir, -1, -1 );
		break;
	case WP_THERMAL:
		trap_FX_PlayEffectID( cgs.effects.sonicExplosionEffect	, origin, dir, -1, -1 );
		break;
	case WP_EMPLACED_GUN:
		//FIXME: Its own effect?
		FX_BlasterWeaponHitPlayer( origin, dir, humanoid );
		break;

	default:
		break;
	}
}

void CG_MissileHitPlayer6(int weapon, vec3_t origin, vec3_t dir, int entityNum, qboolean altFire) 
{
	qboolean	humanoid = qtrue;
	vec3_t up={0,0,1};

	/*
	// NOTENOTE Non-portable code from single player
	if ( cent->gent )
	{
		other = &g_entities[cent->gent->s.otherEntityNum];

		if ( other->client && other->client->playerTeam == TEAM_BOTS )
		{
			humanoid = qfalse;
		}
	}
	*/	

	// NOTENOTE No bleeding in this game
//	CG_Bleed( origin, entityNum );

	// some weapons will make an explosion with the blood, while
	// others will just make the blood
	switch ( weapon ) {
	case WP_STUN_BATON:
		FX_DestructionHitPlayer( origin, dir, humanoid );
		break;
	case WP_BRYAR_PISTOL:
		if ( altFire )
		{
			FX_BryarAltHitPlayer( origin, dir, humanoid );
		}
		else
		{
			FX_BryarHitPlayer( origin, dir, humanoid );
		}
		break;

	case WP_CONCUSSION:

		FX_ConcussionHitPlayer6( origin, dir, humanoid );	
		break;

	case WP_BRYAR_OLD:
		if ( altFire )
		{
			FX_BryarOldAltHitPlayer( origin, dir, humanoid );
		}
		else
		{
			FX_BryarOldHitPlayer( origin, dir, humanoid );
		}
		break;

	case WP_TURRET:
		FX_TurretHitPlayer( origin, dir, humanoid );
		break;

	case WP_BLASTER:
		FX_BlasterWeaponHitPlayer( origin, dir, humanoid );
		break;

	case WP_DISRUPTOR:
		FX_DisruptorAltHit( origin, dir);
		break;

	case WP_BOWCASTER:
		FX_BowcasterHitPlayer( origin, dir, humanoid );
		break;

	case WP_REPEATER:
		if ( altFire )
		{
			FX_RepeaterAltHitPlayer6( origin, dir, humanoid );
		}
		else
		{
			FX_RepeaterHitPlayer( origin, dir, humanoid );
		}
		break;

	case WP_DEMP2:
		// Do a full body effect here for some more feedback
		// NOTENOTE The chaining of the demp2 is not yet implemented.
		/*
		if ( other )
		{
			other->s.powerups |= ( 1 << PW_DISINT_1 );
			other->client->ps.powerups[PW_DISINT_1] = cg.time + 650;
		}
		*/
			FX_DEMP2_HitPlayer6( origin, dir, humanoid );	
		break;

	case WP_FLECHETTE:
		FX_FlechetteWeaponHitPlayer( origin, dir, humanoid );
		break;

	case WP_ROCKET_LAUNCHER:
		trap_FX_PlayEffectID( cgs.effects.incineraryExplosionEffect, origin, dir, -1, -1 );
		break;
	case WP_THERMAL:
		trap_FX_PlayEffectID( cgs.effects.flashExplosionEffect	, origin, dir, -1, -1 );
		break;
	case WP_EMPLACED_GUN:
		//FIXME: Its own effect?
		FX_BlasterWeaponHitPlayer( origin, dir, humanoid );
		break;

	default:
		break;
	}
}
/*
============================================================================

BULLETS

============================================================================
*/


/*
======================
CG_CalcMuzzlePoint
======================
*/
qboolean CG_CalcMuzzlePoint( int entityNum, vec3_t muzzle ) {
	vec3_t		forward, right;
	vec3_t		gunpoint;
	centity_t	*cent;
	int			anim;

	if ( entityNum == cg.snap->ps.clientNum )
	{ //I'm not exactly sure why we'd be rendering someone else's crosshair, but hey.
		int weapontype = cg.snap->ps.weapon;
		vec3_t weaponMuzzle;
		centity_t *pEnt = &cg_entities[cg.predictedPlayerState.clientNum];

		VectorCopy(WP_MuzzlePoint[weapontype], weaponMuzzle);

		if (weapontype == WP_DISRUPTOR || weapontype == WP_STUN_BATON || weapontype == WP_MELEE || weapontype == WP_SABER)
		{
			VectorClear(weaponMuzzle);
		}

		if (cg.renderingThirdPerson)
		{
			VectorCopy( pEnt->lerpOrigin, gunpoint );
			AngleVectors( pEnt->lerpAngles, forward, right, NULL );
		}
		else
		{
			VectorCopy( cg.refdef.vieworg, gunpoint );
			AngleVectors( cg.refdef.viewangles, forward, right, NULL );
		}

		if (weapontype == WP_EMPLACED_GUN && cg.snap->ps.emplacedIndex)
		{
			centity_t *gunEnt = &cg_entities[cg.snap->ps.emplacedIndex];

			if (gunEnt)
			{
				vec3_t pitchConstraint;

				VectorCopy(gunEnt->lerpOrigin, gunpoint);
				gunpoint[2] += 46;

				if (cg.renderingThirdPerson)
				{
					VectorCopy(pEnt->lerpAngles, pitchConstraint);
				}
				else
				{
					VectorCopy(cg.refdef.viewangles, pitchConstraint);
				}

				if (pitchConstraint[PITCH] > 40)
				{
					pitchConstraint[PITCH] = 40;
				}
				AngleVectors( pitchConstraint, forward, right, NULL );
			}
		}

		VectorCopy(gunpoint, muzzle);

		VectorMA(muzzle, weaponMuzzle[0], forward, muzzle);
		VectorMA(muzzle, weaponMuzzle[1], right, muzzle);

		if (weapontype == WP_EMPLACED_GUN && cg.snap->ps.emplacedIndex)
		{
			//Do nothing
		}
		else if (cg.renderingThirdPerson)
		{
			muzzle[2] += cg.snap->ps.viewheight + weaponMuzzle[2];
		}
		else
		{
			muzzle[2] += weaponMuzzle[2];
		}

		return qtrue;
	}

	cent = &cg_entities[entityNum];
	if ( !cent->currentValid ) {
		return qfalse;
	}

	VectorCopy( cent->currentState.pos.trBase, muzzle );

	AngleVectors( cent->currentState.apos.trBase, forward, NULL, NULL );
	anim = cent->currentState.legsAnim;
	if ( anim == BOTH_CROUCH1WALK || anim == BOTH_CROUCH1IDLE ) {
		muzzle[2] += CROUCH_VIEWHEIGHT;
	} else {
		muzzle[2] += DEFAULT_VIEWHEIGHT;
	}

	VectorMA( muzzle, 14, forward, muzzle );

	return qtrue;

}



/*
Ghoul2 Insert Start
*/

// create one instance of all the weapons we are going to use so we can just copy this info into each clients gun ghoul2 object in fast way
static void *g2WeaponInstances[MAX_WEAPONS];
static void *g2WeaponInstances2[MAX_WEAPONS];//[DualPistols]
static void *g2WeaponInstances3[MAX_WEAPONS];//[Option1]
static void *g2WeaponInstances4[MAX_WEAPONS];//[Option1DualPistols]
static void *g2WeaponInstances5[MAX_WEAPONS];//[Option2]
static void *g2WeaponInstances6[MAX_WEAPONS];//[Option2DualPistols]
static void *g2WeaponInstances7[MAX_WEAPONS];//[Option3]
static void *g2WeaponInstances8[MAX_WEAPONS];//[Option3DualPistols]
static void *g2WeaponInstances9[MAX_WEAPONS];//[Option4]
static void *g2WeaponInstances10[MAX_WEAPONS];//[Option3DualPistols]
static void *g2WeaponInstances11[MAX_WEAPONS];//[Option5]
static void *g2WeaponInstances12[MAX_WEAPONS];//[Option3DualPistols]
//[VisualWeapons]
void *g2HolsterWeaponInstances[MAX_WEAPONS];
//[/VisualWeapons]
void CG_InitG2Weapons(void)
{
	int i = 0;
	gitem_t		*item;
	memset(g2WeaponInstances, 0, sizeof(g2WeaponInstances));
	memset(g2WeaponInstances2, 0, sizeof(g2WeaponInstances2));//[DualPistols]
	memset(g2WeaponInstances3, 0, sizeof(g2WeaponInstances3));//[Option1]
	memset(g2WeaponInstances4, 0, sizeof(g2WeaponInstances4));//[Option1DualPistols]
	memset(g2WeaponInstances5, 0, sizeof(g2WeaponInstances5));//[Option2]
	memset(g2WeaponInstances6, 0, sizeof(g2WeaponInstances6));//[Option2DualPistols]
	memset(g2WeaponInstances7, 0, sizeof(g2WeaponInstances7));//[Option2]
	memset(g2WeaponInstances8, 0, sizeof(g2WeaponInstances8));//[Option2DualPistols]
	memset(g2WeaponInstances9, 0, sizeof(g2WeaponInstances9));//[Option2DualPistols]
	memset(g2WeaponInstances10, 0, sizeof(g2WeaponInstances10));//[Option2DualPistols]
	memset(g2WeaponInstances11, 0, sizeof(g2WeaponInstances11));//[Option2DualPistols]
	memset(g2WeaponInstances12, 0, sizeof(g2WeaponInstances12));//[Option2DualPistols]
	for ( item = bg_itemlist + 1 ; item->classname ; item++ ) 
	{
		if ( item->giType == IT_WEAPON )
		{
			assert(item->giTag < MAX_WEAPONS);

			// initialise model
			trap_G2API_InitGhoul2Model(&g2WeaponInstances[/*i*/item->giTag], item->world_model[0], 0, 0, 0, 0, 0);
			//[VisualWeapons]
			//init holster models at the same time.
			trap_G2API_InitGhoul2Model(&g2HolsterWeaponInstances[item->giTag], item->world_model[0], 0, 0, 0, 0, 0);
			//[/VisualWeapons]
//			trap_G2API_InitGhoul2Model(&g2WeaponInstances[i], item->world_model[0],G_ModelIndex( item->world_model[0] ) , 0, 0, 0, 0);
			if (g2WeaponInstances[/*i*/item->giTag])
			{
				// indicate we will be bolted to model 0 (ie the player) on bolt 0 (always the right hand) when we get copied
				trap_G2API_SetBoltInfo(g2WeaponInstances[/*i*/item->giTag], 0, 0);
				// now set up the gun bolt on it
				if (item->giTag == WP_SABER)
				{
					trap_G2API_AddBolt(g2WeaponInstances[/*i*/item->giTag], 0, "*blade1");
				}
				else
				{
					trap_G2API_AddBolt(g2WeaponInstances[/*i*/item->giTag], 0, "*flash");
				}
				i++;
			}
			
			//[DualPistols]
			trap_G2API_InitGhoul2Model(&g2WeaponInstances2[/*i*/item->giTag], item->world_model[0], 0, 0, 0, 0, 0);
//			trap_G2API_InitGhoul2Model(&g2WeaponInstances2[i], item->world_model[0],G_ModelIndex( item->world_model[0] ) , 0, 0, 0, 0);
			if (g2WeaponInstances2[/*i*/item->giTag])
			{
				// indicate we will be bolted to model 0 (ie the player) on bolt 0 (always the right hand) when we get copied
				//WeaponMod FIXME ? : c bien 1?
				trap_G2API_SetBoltInfo(g2WeaponInstances2[/*i*/item->giTag], 0, 1);
				// now set up the gun bolt on it
				if (item->giTag == WP_SABER)
				{
					trap_G2API_AddBolt(g2WeaponInstances2[/*i*/item->giTag], 0, "*blade1");
				}
				else
				{
					trap_G2API_AddBolt(g2WeaponInstances2[/*i*/item->giTag], 0, "*flash");
				}
			}
			//[/DualPistols]

			#define PISTOL2_MODEL "models/weapons2/dh-17/dh-17_w.glm"
			#define BLASTER2_MODEL "models/weapons2/a280/a280_w.glm"
			#define DISRUPTOR2_MODEL "models/weapons2/DLT20a/dlt20a_w.glm"
			#define BOWCASTER2_MODEL "models/weapons2/ee-3/ee-3_w.glm"
			#define REPEATER2_MODEL "models/weapons2/dlt-18_repeater/dlt-18_repeater_w.glm"
			#define DEMP22_MODEL "models/weapons2/beamRIFLE/c_rifle_w.glm"
			#define FLECHETTE2_MODEL "models/weapons/Bryar_Rifle/model.glm"
			#define CONCUSSION2_MODEL "models/weapons/LJ-70_ConcRifle/model.glm"
			#define ROCKETS2_MODEL "models/weapons2/MiniMag_launcher/launcher_w.glm"
			#define THERMAL2_MODEL "models/weapons2/plasma/plasma_w.glm"
			#define OLD2_MODEL "models/weapons2/s5_heavy_pistol/s5_pistol_w.glm"
			//Option1
			// initialise model

			if (item->giTag == WP_BRYAR_PISTOL)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances3[/*i*/item->giTag], PISTOL2_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_BLASTER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances3[/*i*/item->giTag], BLASTER2_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_DISRUPTOR)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances3[/*i*/item->giTag], DISRUPTOR2_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_BOWCASTER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances3[/*i*/item->giTag], BOWCASTER2_MODEL, 0, 0, 0, 0, 0);
			}	
			else if (item->giTag == WP_REPEATER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances3[/*i*/item->giTag], REPEATER2_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_DEMP2)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances3[/*i*/item->giTag], DEMP22_MODEL, 0, 0, 0, 0, 0);
			}				
			else if (item->giTag == WP_FLECHETTE)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances3[/*i*/item->giTag], FLECHETTE2_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_CONCUSSION)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances3[/*i*/item->giTag], CONCUSSION2_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_ROCKET_LAUNCHER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances3[/*i*/item->giTag], ROCKETS2_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_THERMAL)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances3[/*i*/item->giTag], THERMAL2_MODEL, 0, 0, 0, 0, 0);
			}			
			else if (item->giTag == WP_BRYAR_OLD)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances3[/*i*/item->giTag], OLD2_MODEL, 0, 0, 0, 0, 0);
			}			
			else
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances3[/*i*/item->giTag], item->world_model[0], 0, 0, 0, 0, 0);
			}
			//[VisualWeapons]
			//[/VisualWeapons]
//			trap_G2API_InitGhoul2Model(&g2WeaponInstances[i], item->world_model[0],G_ModelIndex( item->world_model[0] ) , 0, 0, 0, 0);
			if (g2WeaponInstances3[/*i*/item->giTag])
			{
				// indicate we will be bolted to model 0 (ie the player) on bolt 0 (always the right hand) when we get copied
				trap_G2API_SetBoltInfo(g2WeaponInstances3[/*i*/item->giTag], 0, 0);
				// now set up the gun bolt on it
				if (item->giTag == WP_SABER)
				{
					trap_G2API_AddBolt(g2WeaponInstances3[/*i*/item->giTag], 0, "*blade1");
				}
				else
				{
					trap_G2API_AddBolt(g2WeaponInstances3[/*i*/item->giTag], 0, "*flash");
				}
				i++;
			}
			
			//[Option1DualPistols]

			if (item->giTag == WP_BRYAR_PISTOL)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances4[/*i*/item->giTag], PISTOL2_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_BLASTER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances4[/*i*/item->giTag], BLASTER2_MODEL, 0, 0, 0, 0, 0);
			}	
			else if (item->giTag == WP_DISRUPTOR)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances4[/*i*/item->giTag], DISRUPTOR2_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_BOWCASTER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances4[/*i*/item->giTag], BOWCASTER2_MODEL, 0, 0, 0, 0, 0);
			}	
			else if (item->giTag == WP_REPEATER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances4[/*i*/item->giTag], REPEATER2_MODEL, 0, 0, 0, 0, 0);
			}	
			else if (item->giTag == WP_DEMP2)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances4[/*i*/item->giTag], DEMP22_MODEL, 0, 0, 0, 0, 0);
			}	
			else if (item->giTag == WP_FLECHETTE)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances4[/*i*/item->giTag], FLECHETTE2_MODEL, 0, 0, 0, 0, 0);
			}	
			else if (item->giTag == WP_CONCUSSION)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances4[/*i*/item->giTag], CONCUSSION2_MODEL, 0, 0, 0, 0, 0);
			}		
			else if (item->giTag == WP_ROCKET_LAUNCHER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances4[/*i*/item->giTag], ROCKETS2_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_THERMAL)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances4[/*i*/item->giTag], THERMAL2_MODEL, 0, 0, 0, 0, 0);
			}			
			else if (item->giTag == WP_BRYAR_OLD)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances4[/*i*/item->giTag], OLD2_MODEL, 0, 0, 0, 0, 0);
			}				
			else
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances4[/*i*/item->giTag], item->world_model[0], 0, 0, 0, 0, 0);
			}
//			trap_G2API_InitGhoul2Model(&g2WeaponInstances2[i], item->world_model[0],G_ModelIndex( item->world_model[0] ) , 0, 0, 0, 0);
			if (g2WeaponInstances4[/*i*/item->giTag])
			{
				// indicate we will be bolted to model 0 (ie the player) on bolt 0 (always the right hand) when we get copied
				//WeaponMod FIXME ? : c bien 1?
				trap_G2API_SetBoltInfo(g2WeaponInstances4[/*i*/item->giTag], 0, 1);
				// now set up the gun bolt on it
				if (item->giTag == WP_SABER)
				{
					trap_G2API_AddBolt(g2WeaponInstances4[/*i*/item->giTag], 0, "*blade1");
				}
				else
				{
					trap_G2API_AddBolt(g2WeaponInstances4[/*i*/item->giTag], 0, "*flash");
				}
			}
			//[/Option1DualPistols]


			#define PISTOL3_MODEL "models/weapons2/dc-17/dc-17_w.glm"
			#define BLASTER3_MODEL "models/weapons2/dc-15s/blaster_w.glm"
			#define DISRUPTOR3_MODEL "models/weapons2/clone_disruptor/clone_disruptor_w.glm"
			#define BOWCASTER3_MODEL "models/weapons2/wbowcaster/wbowcaster1.glm"
			#define REPEATER3_MODEL "models/weapons2/dc-15a/dc-15a_w.glm"
			#define DEMP23_MODEL "models/weapons2/dc-17p/demp2_w.glm"
			#define FLECHETTE3_MODEL "models/weapons/DoubleBarrel_ArrayGun/model.glm"
			#define CONCUSSION3_MODEL "models/weapons/PulseCannon/model.glm"
			#define ROCKETS3_MODEL "models/weapons2/cw_launcher/cw_launcher_w.glm"
			#define THERMAL3_MODEL "models/weapons2/fraggrenade/thermal_w.glm"
			#define OLD3_MODEL "models/weapons2/sc-10_holdout/sc-10_holdout_w.glm"
			//Option2
			// initialise model

			if (item->giTag == WP_BRYAR_PISTOL)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances5[/*i*/item->giTag], PISTOL3_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_BLASTER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances5[/*i*/item->giTag], BLASTER3_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_DISRUPTOR)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances5[/*i*/item->giTag], DISRUPTOR3_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_BOWCASTER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances5[/*i*/item->giTag], BOWCASTER3_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_REPEATER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances5[/*i*/item->giTag], REPEATER3_MODEL, 0, 0, 0, 0, 0);
			}	
			else if (item->giTag == WP_DEMP2)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances5[/*i*/item->giTag], DEMP23_MODEL, 0, 0, 0, 0, 0);
			}	
			else if (item->giTag == WP_FLECHETTE)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances5[/*i*/item->giTag], FLECHETTE3_MODEL, 0, 0, 0, 0, 0);
			}	
			else if (item->giTag == WP_CONCUSSION)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances5[/*i*/item->giTag], CONCUSSION3_MODEL, 0, 0, 0, 0, 0);
			}		
			else if (item->giTag == WP_ROCKET_LAUNCHER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances5[/*i*/item->giTag], ROCKETS3_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_THERMAL)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances5[/*i*/item->giTag], THERMAL3_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_BRYAR_OLD)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances5[/*i*/item->giTag], OLD3_MODEL, 0, 0, 0, 0, 0);
			}			
			else
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances5[/*i*/item->giTag], item->world_model[0], 0, 0, 0, 0, 0);
			}
			//[VisualWeapons]
			//[/VisualWeapons]
//			trap_G2API_InitGhoul2Model(&g2WeaponInstances[i], item->world_model[0],G_ModelIndex( item->world_model[0] ) , 0, 0, 0, 0);
			if (g2WeaponInstances5[/*i*/item->giTag])
			{
				// indicate we will be bolted to model 0 (ie the player) on bolt 0 (always the right hand) when we get copied
				trap_G2API_SetBoltInfo(g2WeaponInstances5[/*i*/item->giTag], 0, 0);
				// now set up the gun bolt on it
				if (item->giTag == WP_SABER)
				{
					trap_G2API_AddBolt(g2WeaponInstances5[/*i*/item->giTag], 0, "*blade1");
				}
				else
				{
					trap_G2API_AddBolt(g2WeaponInstances5[/*i*/item->giTag], 0, "*flash");
				}
				i++;
			}
			
			//[Option2DualPistols]

			if (item->giTag == WP_BRYAR_PISTOL)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances6[/*i*/item->giTag], PISTOL3_MODEL, 0, 0, 0, 0, 0);
			}	
			else if (item->giTag == WP_BLASTER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances6[/*i*/item->giTag], BLASTER3_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_DISRUPTOR)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances6[/*i*/item->giTag], DISRUPTOR3_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_BOWCASTER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances6[/*i*/item->giTag], BOWCASTER3_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_REPEATER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances6[/*i*/item->giTag], REPEATER3_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_DEMP2)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances6[/*i*/item->giTag], DEMP23_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_FLECHETTE)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances6[/*i*/item->giTag], FLECHETTE3_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_CONCUSSION)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances6[/*i*/item->giTag], CONCUSSION3_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_ROCKET_LAUNCHER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances6[/*i*/item->giTag], ROCKETS3_MODEL, 0, 0, 0, 0, 0);
			}	
			else if (item->giTag == WP_THERMAL)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances6[/*i*/item->giTag], THERMAL3_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_BRYAR_OLD)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances6[/*i*/item->giTag], OLD3_MODEL, 0, 0, 0, 0, 0);
			}				
			else
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances6[/*i*/item->giTag], item->world_model[0], 0, 0, 0, 0, 0);
			}
//			trap_G2API_InitGhoul2Model(&g2WeaponInstances2[i], item->world_model[0],G_ModelIndex( item->world_model[0] ) , 0, 0, 0, 0);
			if (g2WeaponInstances6[/*i*/item->giTag])
			{
				// indicate we will be bolted to model 0 (ie the player) on bolt 0 (always the right hand) when we get copied
				//WeaponMod FIXME ? : c bien 1?
				trap_G2API_SetBoltInfo(g2WeaponInstances6[/*i*/item->giTag], 0, 1);
				// now set up the gun bolt on it
				if (item->giTag == WP_SABER)
				{
					trap_G2API_AddBolt(g2WeaponInstances6[/*i*/item->giTag], 0, "*blade1");
				}
				else
				{
					trap_G2API_AddBolt(g2WeaponInstances6[/*i*/item->giTag], 0, "*flash");
				}
			}
			//[/Option2DualPistols]





			#define PISTOL4_MODEL "models/weapons2/westar_if/blaster_pistol_w.glm"
			#define BLASTER4_MODEL "models/weapons2/sad_e5/blaster_w.glm"
			#define DISRUPTOR4_MODEL "models/weapons2/proj_rifle/proj_rifle_w.glm"
			#define BOWCASTER4_MODEL "models/weapons2/canderous_blaster/cand_blaster_w.glm"
			#define REPEATER4_MODEL "models/weapons2/t-21/t-21_w.glm"
			#define DEMP24_MODEL "models/weapons2/CR-24_flamerifle/rifle_w.glm"
			#define FLECHETTE4_MODEL "models/weapons/ACP_ArrayGun/model.glm"
			#define CONCUSSION4_MODEL "models/weapons2/geonosian/sonicrifle.glm"
			#define ROCKETS4_MODEL "models/weapons2/e60r_launcher/e60r_launcher_w.glm"
			#define THERMAL4_MODEL "models/weapons2/V-59_Concussion/V-59_conc_w.glm"
			#define OLD4_MODEL "models/weapons2/noweap/noweap.glm"
			//Option3
			// initialise model
			if (item->giTag == WP_BRYAR_PISTOL)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances7[/*i*/item->giTag], PISTOL4_MODEL, 0, 0, 0, 0, 0);
			}	
			else if (item->giTag == WP_BLASTER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances7[/*i*/item->giTag], BLASTER4_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_DISRUPTOR)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances7[/*i*/item->giTag], DISRUPTOR4_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_BOWCASTER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances7[/*i*/item->giTag], BOWCASTER4_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_REPEATER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances7[/*i*/item->giTag], REPEATER4_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_DEMP2)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances7[/*i*/item->giTag], DEMP24_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_FLECHETTE)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances7[/*i*/item->giTag], FLECHETTE4_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_CONCUSSION)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances7[/*i*/item->giTag], CONCUSSION4_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_ROCKET_LAUNCHER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances7[/*i*/item->giTag], ROCKETS4_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_THERMAL)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances7[/*i*/item->giTag], THERMAL4_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_BRYAR_OLD)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances7[/*i*/item->giTag], OLD4_MODEL, 0, 0, 0, 0, 0);
			}
			else
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances7[/*i*/item->giTag], item->world_model[0], 0, 0, 0, 0, 0);
			}
			//[VisualWeapons]
			//[/VisualWeapons]
//			trap_G2API_InitGhoul2Model(&g2WeaponInstances[i], item->world_model[0],G_ModelIndex( item->world_model[0] ) , 0, 0, 0, 0);
			if (g2WeaponInstances7[/*i*/item->giTag])
			{
				// indicate we will be bolted to model 0 (ie the player) on bolt 0 (always the right hand) when we get copied
				trap_G2API_SetBoltInfo(g2WeaponInstances7[/*i*/item->giTag], 0, 0);
				// now set up the gun bolt on it
				if (item->giTag == WP_SABER)
				{
					trap_G2API_AddBolt(g2WeaponInstances7[/*i*/item->giTag], 0, "*blade1");
				}
				else
				{
					trap_G2API_AddBolt(g2WeaponInstances7[/*i*/item->giTag], 0, "*flash");
				}
				i++;
			}
			
			//[Option3DualPistols]
			if (item->giTag == WP_BRYAR_PISTOL)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances8[/*i*/item->giTag], PISTOL4_MODEL, 0, 0, 0, 0, 0);
			}	
			else if (item->giTag == WP_BLASTER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances8[/*i*/item->giTag], BLASTER4_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_DISRUPTOR)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances8[/*i*/item->giTag], DISRUPTOR4_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_BOWCASTER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances8[/*i*/item->giTag], BOWCASTER4_MODEL, 0, 0, 0, 0, 0);
			}						
			else if (item->giTag == WP_REPEATER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances8[/*i*/item->giTag], REPEATER4_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_DEMP2)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances8[/*i*/item->giTag], DEMP24_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_FLECHETTE)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances8[/*i*/item->giTag], FLECHETTE4_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_CONCUSSION)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances8[/*i*/item->giTag], CONCUSSION4_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_ROCKET_LAUNCHER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances8[/*i*/item->giTag], ROCKETS4_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_THERMAL)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances8[/*i*/item->giTag], THERMAL4_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_BRYAR_OLD)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances8[/*i*/item->giTag], OLD4_MODEL, 0, 0, 0, 0, 0);
			}
			else
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances8[/*i*/item->giTag], item->world_model[0], 0, 0, 0, 0, 0);
			}
//			trap_G2API_InitGhoul2Model(&g2WeaponInstances2[i], item->world_model[0],G_ModelIndex( item->world_model[0] ) , 0, 0, 0, 0);
			if (g2WeaponInstances8[/*i*/item->giTag])
			{
				// indicate we will be bolted to model 0 (ie the player) on bolt 0 (always the right hand) when we get copied
				//WeaponMod FIXME ? : c bien 1?
				trap_G2API_SetBoltInfo(g2WeaponInstances8[/*i*/item->giTag], 0, 1);
				// now set up the gun bolt on it
				if (item->giTag == WP_SABER)
				{
					trap_G2API_AddBolt(g2WeaponInstances8[/*i*/item->giTag], 0, "*blade1");
				}
				else
				{
					trap_G2API_AddBolt(g2WeaponInstances8[/*i*/item->giTag], 0, "*flash");
				}
			}
			//[/Option3DualPistols]




			#define PISTOL5_MODEL "models/weapons2/Glie-44/blaster_pistol_w.glm"
			#define BLASTER5_MODEL "models/weapons2/EL-16/blaster_w.glm"
			#define DISRUPTOR5_MODEL "models/weapons2/tusken_rifle/tusken_rifle_w.glm"
			#define BOWCASTER5_MODEL "models/weapons/Bowcaster_heavy/model.glm"
			#define REPEATER5_MODEL "models/weapons2/dc-17m/dc-17m_w.glm"
			#define DEMP25_MODEL "models/weapons2/CR-25_icerifle/rifle_w.glm"
			#define FLECHETTE5_MODEL "models/weapons2/CR-1_cannon/rifle_w.glm"
			#define CONCUSSION5_MODEL "models/weapons2/z6_rotary/model.glm"
			#define ROCKETS5_MODEL "models/weapons/Packered_MortarGun/model.glm"
			#define THERMAL5_MODEL "models/weapons2/sonic_detonator/sonic_det_w.glm"
			#define OLD5_MODEL "models/weapons2/noweap/noweap.glm"
			//Option4
			// initialise model
			if (item->giTag == WP_BRYAR_PISTOL)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances9[/*i*/item->giTag], PISTOL5_MODEL, 0, 0, 0, 0, 0);
			}	
			else if (item->giTag == WP_BLASTER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances9[/*i*/item->giTag], BLASTER5_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_DISRUPTOR)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances9[/*i*/item->giTag], DISRUPTOR5_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_BOWCASTER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances9[/*i*/item->giTag], BOWCASTER5_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_REPEATER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances9[/*i*/item->giTag], REPEATER5_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_DEMP2)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances9[/*i*/item->giTag], DEMP25_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_FLECHETTE)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances9[/*i*/item->giTag], FLECHETTE5_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_CONCUSSION)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances9[/*i*/item->giTag], CONCUSSION5_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_ROCKET_LAUNCHER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances9[/*i*/item->giTag], ROCKETS5_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_THERMAL)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances9[/*i*/item->giTag], THERMAL5_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_BRYAR_OLD)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances9[/*i*/item->giTag], OLD5_MODEL, 0, 0, 0, 0, 0);
			}
			else
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances9[/*i*/item->giTag], item->world_model[0], 0, 0, 0, 0, 0);
			}
			//[VisualWeapons]
			//[/VisualWeapons]
//			trap_G2API_InitGhoul2Model(&g2WeaponInstances[i], item->world_model[0],G_ModelIndex( item->world_model[0] ) , 0, 0, 0, 0);
			if (g2WeaponInstances9[/*i*/item->giTag])
			{
				// indicate we will be bolted to model 0 (ie the player) on bolt 0 (always the right hand) when we get copied
				trap_G2API_SetBoltInfo(g2WeaponInstances9[/*i*/item->giTag], 0, 0);
				// now set up the gun bolt on it
				if (item->giTag == WP_SABER)
				{
					trap_G2API_AddBolt(g2WeaponInstances9[/*i*/item->giTag], 0, "*blade1");
				}
				else
				{
					trap_G2API_AddBolt(g2WeaponInstances9[/*i*/item->giTag], 0, "*flash");
				}
				i++;
			}
			
			//[Option4DualPistols]
			if (item->giTag == WP_BRYAR_PISTOL)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances10[/*i*/item->giTag], PISTOL5_MODEL, 0, 0, 0, 0, 0);
			}	
			else if (item->giTag == WP_BLASTER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances10[/*i*/item->giTag], BLASTER5_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_DISRUPTOR)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances10[/*i*/item->giTag], DISRUPTOR5_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_BOWCASTER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances10[/*i*/item->giTag], BOWCASTER5_MODEL, 0, 0, 0, 0, 0);
			}						
			else if (item->giTag == WP_REPEATER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances10[/*i*/item->giTag], REPEATER5_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_DEMP2)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances10[/*i*/item->giTag], DEMP25_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_FLECHETTE)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances10[/*i*/item->giTag], FLECHETTE5_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_CONCUSSION)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances10[/*i*/item->giTag], CONCUSSION5_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_ROCKET_LAUNCHER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances10[/*i*/item->giTag], ROCKETS5_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_THERMAL)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances10[/*i*/item->giTag], THERMAL5_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_BRYAR_OLD)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances10[/*i*/item->giTag], OLD5_MODEL, 0, 0, 0, 0, 0);
			}
			else
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances10[/*i*/item->giTag], item->world_model[0], 0, 0, 0, 0, 0);
			}
//			trap_G2API_InitGhoul2Model(&g2WeaponInstances2[i], item->world_model[0],G_ModelIndex( item->world_model[0] ) , 0, 0, 0, 0);
			if (g2WeaponInstances10[/*i*/item->giTag])
			{
				// indicate we will be bolted to model 0 (ie the player) on bolt 0 (always the right hand) when we get copied
				//WeaponMod FIXME ? : c bien 1?
				trap_G2API_SetBoltInfo(g2WeaponInstances10[/*i*/item->giTag], 0, 1);
				// now set up the gun bolt on it
				if (item->giTag == WP_SABER)
				{
					trap_G2API_AddBolt(g2WeaponInstances10[/*i*/item->giTag], 0, "*blade1");
				}
				else
				{
					trap_G2API_AddBolt(g2WeaponInstances10[/*i*/item->giTag], 0, "*flash");
				}
			}
			//[/Option4DualPistols]


			#define PISTOL6_MODEL "models/weapons2/SE-44C/blaster_pistol_w.glm"
			#define BLASTER6_MODEL "models/weapons2/f11d_blaster/model.glm"
			#define DISRUPTOR6_MODEL "models/weapons2/psg/disruptor_w.glm"
			#define BOWCASTER6_MODEL "models/weapons2/STCompRifle/bowcaster_w.glm"
			#define REPEATER6_MODEL "models/weapons2/FWMB-10/heavy_repeater_w.glm"
			#define DEMP26_MODEL "models/weapons/CarboniteRifle/model.glm"
			#define FLECHETTE6_MODEL "models/weapons2/junglerifle/c_rifle_w.glm"
			#define CONCUSSION6_MODEL "models/weapons2/minigun/minigun_w.glm"
			#define ROCKETS6_MODEL "models/weapons/Packered_MortarGun/model.glm"
			#define THERMAL6_MODEL "models/weapons2/Stormi_TD/stormiTD_w.glm"
			#define OLD6_MODEL "models/weapons2/concussion_new/c_rifle_w.glm"
			//Option5
			// initialise model
			if (item->giTag == WP_BRYAR_PISTOL)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances11[/*i*/item->giTag], PISTOL6_MODEL, 0, 0, 0, 0, 0);
			}	
			else if (item->giTag == WP_BLASTER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances11[/*i*/item->giTag], BLASTER6_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_DISRUPTOR)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances11[/*i*/item->giTag], DISRUPTOR6_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_BOWCASTER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances11[/*i*/item->giTag], BOWCASTER6_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_REPEATER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances11[/*i*/item->giTag], REPEATER6_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_DEMP2)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances11[/*i*/item->giTag], DEMP26_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_FLECHETTE)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances11[/*i*/item->giTag], FLECHETTE6_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_CONCUSSION)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances11[/*i*/item->giTag], CONCUSSION6_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_ROCKET_LAUNCHER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances11[/*i*/item->giTag], ROCKETS6_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_THERMAL)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances11[/*i*/item->giTag], THERMAL6_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_BRYAR_OLD)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances11[/*i*/item->giTag], OLD6_MODEL, 0, 0, 0, 0, 0);
			}
			else
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances11[/*i*/item->giTag], item->world_model[0], 0, 0, 0, 0, 0);
			}
			//[VisualWeapons]
			//[/VisualWeapons]
//			trap_G2API_InitGhoul2Model(&g2WeaponInstances[i], item->world_model[0],G_ModelIndex( item->world_model[0] ) , 0, 0, 0, 0);
			if (g2WeaponInstances11[/*i*/item->giTag])
			{
				// indicate we will be bolted to model 0 (ie the player) on bolt 0 (always the right hand) when we get copied
				trap_G2API_SetBoltInfo(g2WeaponInstances11[/*i*/item->giTag], 0, 0);
				// now set up the gun bolt on it
				if (item->giTag == WP_SABER)
				{
					trap_G2API_AddBolt(g2WeaponInstances11[/*i*/item->giTag], 0, "*blade1");
				}
				else
				{
					trap_G2API_AddBolt(g2WeaponInstances11[/*i*/item->giTag], 0, "*flash");
				}
				i++;
			}
			
			//[Option5DualPistols]
			if (item->giTag == WP_BRYAR_PISTOL)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances12[/*i*/item->giTag], PISTOL6_MODEL, 0, 0, 0, 0, 0);
			}	
			else if (item->giTag == WP_BLASTER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances12[/*i*/item->giTag], BLASTER6_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_DISRUPTOR)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances12[/*i*/item->giTag], DISRUPTOR6_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_BOWCASTER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances12[/*i*/item->giTag], BOWCASTER6_MODEL, 0, 0, 0, 0, 0);
			}						
			else if (item->giTag == WP_REPEATER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances12[/*i*/item->giTag], REPEATER6_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_DEMP2)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances12[/*i*/item->giTag], DEMP26_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_FLECHETTE)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances12[/*i*/item->giTag], FLECHETTE6_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_CONCUSSION)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances12[/*i*/item->giTag], CONCUSSION6_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_ROCKET_LAUNCHER)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances12[/*i*/item->giTag], ROCKETS6_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_THERMAL)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances12[/*i*/item->giTag], THERMAL6_MODEL, 0, 0, 0, 0, 0);
			}
			else if (item->giTag == WP_BRYAR_OLD)
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances12[/*i*/item->giTag], OLD6_MODEL, 0, 0, 0, 0, 0);
			}
			else
			{
			trap_G2API_InitGhoul2Model(&g2WeaponInstances12[/*i*/item->giTag], item->world_model[0], 0, 0, 0, 0, 0);
			}
//			trap_G2API_InitGhoul2Model(&g2WeaponInstances2[i], item->world_model[0],G_ModelIndex( item->world_model[0] ) , 0, 0, 0, 0);
			if (g2WeaponInstances12[/*i*/item->giTag])
			{
				// indicate we will be bolted to model 0 (ie the player) on bolt 0 (always the right hand) when we get copied
				//WeaponMod FIXME ? : c bien 1?
				trap_G2API_SetBoltInfo(g2WeaponInstances12[/*i*/item->giTag], 0, 1);
				// now set up the gun bolt on it
				if (item->giTag == WP_SABER)
				{
					trap_G2API_AddBolt(g2WeaponInstances12[/*i*/item->giTag], 0, "*blade1");
				}
				else
				{
					trap_G2API_AddBolt(g2WeaponInstances12[/*i*/item->giTag], 0, "*flash");
				}
			}
			//[/Option4DualPistols]







			i++;

			if (i == MAX_WEAPONS)
			{
				assert(0);	
				break;
			}
			
		}
	}
}

// clean out any g2 models we instanciated for copying purposes
void CG_ShutDownG2Weapons(void)
{
	int i;
	for (i=0; i<MAX_WEAPONS; i++)
	{
		trap_G2API_CleanGhoul2Models(&g2WeaponInstances[i]);
		//[VisualWeapons]
		trap_G2API_CleanGhoul2Models(&g2HolsterWeaponInstances[i]);
		//[/VisualWeapons]
		trap_G2API_CleanGhoul2Models(&g2WeaponInstances2[i]);//[DualPistols]
		trap_G2API_CleanGhoul2Models(&g2WeaponInstances3[i]);//[Option1]
		trap_G2API_CleanGhoul2Models(&g2WeaponInstances4[i]);//[Option1DualPistols]
		trap_G2API_CleanGhoul2Models(&g2WeaponInstances5[i]);//[Option2]
		trap_G2API_CleanGhoul2Models(&g2WeaponInstances6[i]);//[Option2DualPistols]
		trap_G2API_CleanGhoul2Models(&g2WeaponInstances7[i]);//[Option3]
		trap_G2API_CleanGhoul2Models(&g2WeaponInstances8[i]);//[Option3DualPistols]
		trap_G2API_CleanGhoul2Models(&g2WeaponInstances9[i]);//[Option3]
		trap_G2API_CleanGhoul2Models(&g2WeaponInstances10[i]);//[Option3DualPistols]
		trap_G2API_CleanGhoul2Models(&g2WeaponInstances11[i]);//[Option3]
		trap_G2API_CleanGhoul2Models(&g2WeaponInstances12[i]);//[Option3DualPistols]
	}
}

void *CG_G2WeaponInstance(centity_t *cent, int weapon)
{
	clientInfo_t *ci = NULL;

	if (weapon != WP_SABER)
	{
		if(cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_4)
		{
		return g2WeaponInstances11[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_3)
		{
		return g2WeaponInstances9[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_4)
		{
		return g2WeaponInstances7[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_3)
		{
		return g2WeaponInstances5[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_2)
		{
		return g2WeaponInstances3[weapon];
		}
		else
		{
		return g2WeaponInstances[weapon];
		}
	}

	if (cent->currentState.eType != ET_PLAYER &&
		cent->currentState.eType != ET_NPC)
	{
		if(cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_4)
		{
		return g2WeaponInstances11[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_3)
		{
		return g2WeaponInstances9[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_4)
		{
		return g2WeaponInstances7[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_3)
		{
		return g2WeaponInstances5[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_2)
		{
		return g2WeaponInstances3[weapon];
		}
		else
		{
		return g2WeaponInstances[weapon];
		}
		
	}

	if (cent->currentState.eType == ET_NPC)
	{
		ci = cent->npcClient;
	}
	else
	{
		ci = &cgs.clientinfo[cent->currentState.number];
	}

	if (!ci)
	{
		if(cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_4)
		{
		return g2WeaponInstances11[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_3)
		{
		return g2WeaponInstances9[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_4)
		{
		return g2WeaponInstances7[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_3)
		{
		return g2WeaponInstances5[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_2)
		{
		return g2WeaponInstances3[weapon];
		}
		else
		{
		return g2WeaponInstances[weapon];
		}
	}

	//Try to return the custom saber instance if we can.
	if (ci->saber[0].model[0] && ci->ghoul2Weapons[0])
	{
		return ci->ghoul2Weapons[0];
	}

	//If no custom then just use the default.
		if(cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_4)
		{
		return g2WeaponInstances11[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_3)
		{
		return g2WeaponInstances9[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_4)
		{
		return g2WeaponInstances7[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_3)
		{
		return g2WeaponInstances5[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_2)
		{
		return g2WeaponInstances3[weapon];
		}
		else
		{
		return g2WeaponInstances[weapon];
		}
}

//[DualPistols]
void *CG_G2WeaponInstance2(centity_t *cent, int weapon)
{
	clientInfo_t *ci = NULL;

	if (weapon != WP_SABER)
	{
		if(cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_4)
		{
		return g2WeaponInstances12[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_3)
		{
		return g2WeaponInstances10[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_4)
		{
		return g2WeaponInstances8[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_3)
		{
		return g2WeaponInstances6[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_2)
		{
		return g2WeaponInstances4[weapon];
		}
		else
		{
		return g2WeaponInstances2[weapon];
		}
	}

	if (cent->currentState.eType != ET_PLAYER &&
		cent->currentState.eType != ET_NPC)
	{
		if(cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_4)
		{
		return g2WeaponInstances12[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_3)
		{
		return g2WeaponInstances10[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_4)
		{
		return g2WeaponInstances8[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_3)
		{
		return g2WeaponInstances6[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_2)
		{
		return g2WeaponInstances4[weapon];
		}
		else
		{
		return g2WeaponInstances2[weapon];
		}
	}

	if (cent->currentState.eType == ET_NPC)
	{
		ci = cent->npcClient;
	}
	else
	{
		ci = &cgs.clientinfo[cent->currentState.number];
	}

	if (!ci)
	{
		if(cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_4)
		{
		return g2WeaponInstances12[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_3)
		{
		return g2WeaponInstances10[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_4)
		{
		return g2WeaponInstances8[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_3)
		{
		return g2WeaponInstances6[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_2)
		{
		return g2WeaponInstances4[weapon];
		}
		else
		{
		return g2WeaponInstances2[weapon];
		}
	}

	//Try to return the custom saber instance if we can.
	if (ci->saber[0].model[0] &&
		ci->ghoul2Weapons[0])
	{
		return ci->ghoul2Weapons[0];
	}

	//If no custom then just use the default.
		if(cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_4)
		{
		return g2WeaponInstances12[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_2 && cent->currentState.eFlags  & EF_WP_OPTION_3)
		{
		return g2WeaponInstances10[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_4)
		{
		return g2WeaponInstances8[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_3)
		{
		return g2WeaponInstances6[weapon];
		}
		else if(cent->currentState.eFlags  & EF_WP_OPTION_2)
		{
		return g2WeaponInstances4[weapon];
		}
		else
		{
		return g2WeaponInstances2[weapon];
		}
}
//[/DualPistols]



//[VisualWeapons]
void *CG_G2HolsterWeaponInstance(centity_t *cent, int weapon, qboolean secondSaber)
{
	clientInfo_t *ci = NULL;

	if (weapon != WP_SABER)
	{
		return g2HolsterWeaponInstances[weapon];
	}

	if (cent->currentState.eType != ET_PLAYER &&
		cent->currentState.eType != ET_NPC)
	{
		return g2HolsterWeaponInstances[weapon];
	}

	if (cent->currentState.eType == ET_NPC)
	{
		ci = cent->npcClient;
	}
	else
	{
		ci = &cgs.clientinfo[cent->currentState.number];
	}

	if (!ci)
	{
		return g2HolsterWeaponInstances[weapon];
	}

	//Try to return the custom saber instance if we can.
	if(secondSaber)
	{//return secondSaber instance
		if (ci->saber[1].model[0] &&
			ci->ghoul2HolsterWeapons[1])
		{
			return ci->ghoul2HolsterWeapons[1];
		}
	}
	else
	{//return first saber instance
		if (ci->saber[0].model[0] &&
			ci->ghoul2HolsterWeapons[0])
		{
			return ci->ghoul2HolsterWeapons[0];
		}
	}

	//If no custom then just use the default.
	return g2HolsterWeaponInstances[weapon];
}
//[/VisualWeapons]


// what ghoul2 model do we want to copy ?
void CG_CopyG2WeaponInstance(centity_t *cent, int weaponNum, void *toGhoul2)
{
	//rww - the -1 is because there is no "weapon" for WP_NONE
	assert(weaponNum < MAX_WEAPONS);
	if (CG_G2WeaponInstance(cent, weaponNum/*-1*/) )
	{
		if (weaponNum == WP_SABER)
		{
			clientInfo_t *ci = NULL;

			if (cent->currentState.eType == ET_NPC)
			{
				ci = cent->npcClient;
			}
			else
			{
				ci = &cgs.clientinfo[cent->currentState.number];
			}

			if (!ci)
			{
				trap_G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, weaponNum/*-1*/), 0, toGhoul2, 1); 
			}
			else
			{ //Try both the left hand saber and the right hand saber
				int i = 0;

				while (i < MAX_SABERS)
				{
					if (ci->saber[i].model[0] &&
						ci->ghoul2Weapons[i])
					{
						trap_G2API_CopySpecificGhoul2Model(ci->ghoul2Weapons[i], 0, toGhoul2, i+1); 
					}
					else if (ci->ghoul2Weapons[i])
					{ //if the second saber has been removed, then be sure to remove it and free the instance.
						qboolean g2HasSecondSaber = trap_G2API_HasGhoul2ModelOnIndex(&(toGhoul2), 2);

						if (g2HasSecondSaber)
						{ //remove it now since we're switching away from sabers
							trap_G2API_RemoveGhoul2Model(&(toGhoul2), 2);
						}
						trap_G2API_CleanGhoul2Models(&ci->ghoul2Weapons[i]);
					}

					i++;
				}
			}
		}
		else
		{
			qboolean g2HasSecondSaber = trap_G2API_HasGhoul2ModelOnIndex(&(toGhoul2), 2);

			if (g2HasSecondSaber)
			{ //remove it now since we're switching away from sabers
				trap_G2API_RemoveGhoul2Model(&(toGhoul2), 2);
			}

			if (weaponNum == WP_EMPLACED_GUN)
			{ //a bit of a hack to remove gun model when using an emplaced weap
				if (trap_G2API_HasGhoul2ModelOnIndex(&(toGhoul2), 1))
				{
					trap_G2API_RemoveGhoul2Model(&(toGhoul2), 1);
				}
			}
			else if (weaponNum == WP_MELEE)
			{ //don't want a weapon on the model for this one
				if (trap_G2API_HasGhoul2ModelOnIndex(&(toGhoul2), 1))
				{
					trap_G2API_RemoveGhoul2Model(&(toGhoul2), 1);
				}
			}
			else
			{

				trap_G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, weaponNum/*-1*/), 0, toGhoul2, 1); 
				//[DualPistols]
				if ( (cent->currentState.eFlags & EF_DUAL_WEAPONS) &&
					(cent->currentState.weapon == WP_BRYAR_PISTOL) )
				{
					trap_G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance2(cent, weaponNum/*-1*/), 0, toGhoul2, 2);
				}
				else if ( (cent->currentState.eFlags & EF_DUAL_WEAPONS) &&
					(cent->currentState.weapon == WP_BRYAR_OLD) )
				{
					trap_G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance2(cent, weaponNum/*-1*/), 0, toGhoul2, 2);
				}
				else if ( (cent->currentState.eFlags & EF_DUAL_WEAPONS) &&
					(cent->currentState.weapon == WP_STUN_BATON) )
				{
					trap_G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance2(cent, weaponNum/*-1*/), 0, toGhoul2, 2);
				}
				//[/DualPistols]					

			}
		}
	}

	//[CoOp]
	//only WP_NONE doesn't have a CG_G2WeaponInstance, in this case, jsut remove the weapon model then
	else
	{
		if (trap_G2API_HasGhoul2ModelOnIndex(&(toGhoul2), 1))
		{
			trap_G2API_RemoveGhoul2Model(&(toGhoul2), 1);
		}
	}
	//[/CoOp]
	
}

void CG_CheckPlayerG2Weapons(playerState_t *ps, centity_t *cent) 
{
	if (!ps)
	{
		assert(0);
		return;
	}

	if (ps->pm_flags & PMF_FOLLOW)
	{
		return;
	}

	if (cent->currentState.eType == ET_NPC)
	{
		assert(0);
		return;
	}

	//[SaberLockSys]
	//racc - this was preventing a non-saber weapon from rendering when the player's saber is dropped.
	/* basejka code
	// should we change the gun model on this player?
	if (cent->currentState.saberInFlight)
	{
		cent->ghoul2weapon = CG_G2WeaponInstance(cent, WP_SABER);
	}
	*/
	//[/SaberLockSys]

	if (cent->currentState.eFlags & EF_DEAD)
	{ //no updating weapons when dead
		cent->ghoul2weapon = NULL;
		return;
	}

	if (cent->torsoBolt)
	{ //got our limb cut off, no updating weapons until it's restored
		cent->ghoul2weapon = NULL;
		return;
	}

	if (cgs.clientinfo[ps->clientNum].team == TEAM_SPECTATOR ||
		ps->persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		cent->ghoul2weapon = cg_entities[ps->clientNum].ghoul2weapon = NULL;
		cent->weapon = cg_entities[ps->clientNum].weapon = 0;
		return;
	}




	if (cent->ghoul2 && ((cent->ghoul2weapon != CG_G2WeaponInstance(cent, ps->weapon)) || 
			((( cent->ghoul2weapon2 != CG_G2WeaponInstance2(cent, cent->currentState.weapon)) &&
			//WeaponMod FIXME?
			(cent->currentState.eFlags & EF_DUAL_WEAPONS )&& (cent->currentState.weapon != WP_SABER))) ||
		(cent->ghoul2weapon2 != NULL && ((!(cent->currentState.eFlags & EF_DUAL_WEAPONS)) || (cent->currentState.weapon == WP_SABER)))) &&
		ps->clientNum == cent->currentState.number) //don't want spectator mode forcing one client's weapon instance over another's
	//[/DualPistols]
	{
	
		CG_CopyG2WeaponInstance(cent, ps->weapon, cent->ghoul2);
		cent->ghoul2weapon = CG_G2WeaponInstance(cent, ps->weapon);
		if ((cent->currentState.eFlags & EF_DUAL_WEAPONS) &&
			(ps->weapon == WP_BRYAR_PISTOL))
		{
			cent->ghoul2weapon2 = CG_G2WeaponInstance2(cent, ps->weapon);
		}
		else if ((cent->currentState.eFlags & EF_DUAL_WEAPONS) &&
			(ps->weapon == WP_BRYAR_OLD))
		{
			cent->ghoul2weapon2 = CG_G2WeaponInstance2(cent, ps->weapon);
		}
		else if ((cent->currentState.eFlags & EF_DUAL_WEAPONS) &&
			(ps->weapon == WP_STUN_BATON))
		{
			cent->ghoul2weapon2 = CG_G2WeaponInstance2(cent, ps->weapon);
		}
		else
		{
			cent->ghoul2weapon2 = NULL;
		}					
		
		if (cent->weapon == WP_SABER && cent->weapon != ps->weapon && !ps->saberHolstered)
		{ //switching away from the saber
			//trap_S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, trap_S_RegisterSound( "sound/weapons/saber/saberoffquick.wav" ));
			if (cgs.clientinfo[ps->clientNum].saber[0].soundOff && !ps->saberHolstered)
			{
				trap_S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, cgs.clientinfo[ps->clientNum].saber[0].soundOff);
			}

			if (cgs.clientinfo[ps->clientNum].saber[1].soundOff &&
				cgs.clientinfo[ps->clientNum].saber[1].model[0] &&
				!ps->saberHolstered)
			{
				trap_S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, cgs.clientinfo[ps->clientNum].saber[1].soundOff);
			}
		}
		else if (ps->weapon == WP_SABER && cent->weapon != ps->weapon && !cent->saberWasInFlight)
		{ //switching to the saber
			//trap_S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, trap_S_RegisterSound( "sound/weapons/saber/saberon.wav" ));
			if (cgs.clientinfo[ps->clientNum].saber[0].soundOn)
			{
				trap_S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, cgs.clientinfo[ps->clientNum].saber[0].soundOn);
			}

			if (cgs.clientinfo[ps->clientNum].saber[1].soundOn)
			{
				trap_S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, cgs.clientinfo[ps->clientNum].saber[1].soundOn);
			}

			BG_SI_SetDesiredLength(&cgs.clientinfo[ps->clientNum].saber[0], 0, -1);
			BG_SI_SetDesiredLength(&cgs.clientinfo[ps->clientNum].saber[1], 0, -1);
		}
		cent->weapon = ps->weapon;
	}

}


/*
Ghoul2 Insert End
*/
