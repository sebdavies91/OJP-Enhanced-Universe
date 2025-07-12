//
// cg_weaponinit.c -- events and effects dealing with weapons
#include "cg_local.h"
#include "fx_local.h"



/*
=================
CG_RegisterWeapon

The server says this item is used on this level
=================
*/
void CG_RegisterWeapon( int weaponNum) {
	weaponInfo_t	*weaponInfo;
	gitem_t			*item, *ammo;
	char			path[MAX_QPATH];
	vec3_t			mins, maxs;
	int				i;

	weaponInfo = &cg_weapons[weaponNum];

	if ( weaponNum == 0 ) {
		return;
	}

	if ( weaponInfo->registered ) {
		return;
	}

	memset( weaponInfo, 0, sizeof( *weaponInfo ) );
	weaponInfo->registered = qtrue;

	for ( item = bg_itemlist + 1 ; item->classname ; item++ ) {
		if ( item->giType == IT_WEAPON && item->giTag == weaponNum ) {
			weaponInfo->item = item;
			break;
		}
	}
	if ( !item->classname ) {
		CG_Error( "Couldn't find weapon %i", weaponNum );
	}
	CG_RegisterItemVisuals( item - bg_itemlist );

	// load cmodel before model so filecache works
	weaponInfo->weaponModel = trap_R_RegisterModel( item->world_model[0] );
	// load in-view model also
	weaponInfo->viewModel = trap_R_RegisterModel(item->view_model);

	// calc midpoint for rotation
	trap_R_ModelBounds( weaponInfo->weaponModel, mins, maxs );
	for ( i = 0 ; i < 3 ; i++ ) {
		weaponInfo->weaponMidpoint[i] = mins[i] + 0.5 * ( maxs[i] - mins[i] );
	}

				

				
				
				weaponInfo->weaponIcon = trap_R_RegisterShader( item->icon );
				weaponInfo->ammoIcon = trap_R_RegisterShader( item->icon );





	for ( ammo = bg_itemlist + 1 ; ammo->classname ; ammo++ ) {
		if ( ammo->giType == IT_AMMO && ammo->giTag == weaponNum ) {
			break;
		}
	}
	if ( ammo->classname && ammo->world_model[0] ) {
		weaponInfo->ammoModel = trap_R_RegisterModel( ammo->world_model[0] );
	}

//	strcpy( path, item->view_model );
//	COM_StripExtension( path, path );
//	strcat( path, "_flash.md3" );
	weaponInfo->flashModel = 0;//trap_R_RegisterModel( path );

	if (weaponNum == WP_DISRUPTOR ||
		weaponNum == WP_FLECHETTE ||
		weaponNum == WP_REPEATER ||
		weaponNum == WP_ROCKET_LAUNCHER||
		weaponNum == WP_CONCUSSION)
	{
		strcpy( path, item->view_model );
		COM_StripExtension( path, path );
		strcat( path, "_barrel.md3" );
		weaponInfo->barrelModel = trap_R_RegisterModel( path );
	}
	else if (weaponNum == WP_STUN_BATON)
	{ //only weapon with more than 1 barrel..
		trap_R_RegisterModel("models/weapons2/stun_baton/baton_barrel.md3");
		trap_R_RegisterModel("models/weapons2/stun_baton/baton_barrel2.md3");
		trap_R_RegisterModel("models/weapons2/stun_baton/baton_barrel3.md3");
	}
	else
	{
		weaponInfo->barrelModel = 0;
	}

	if (weaponNum != WP_SABER)
	{
		strcpy( path, item->view_model );
		COM_StripExtension( path, path );
		strcat( path, "_hand.md3" );
		weaponInfo->handsModel = trap_R_RegisterModel( path );
	}
	else
	{
		weaponInfo->handsModel = 0;
	}

//	if ( !weaponInfo->handsModel ) {
//		weaponInfo->handsModel = trap_R_RegisterModel( "models/weapons2/shotgun/shotgun_hand.md3" );
//	}

	switch ( weaponNum ) {
	case WP_STUN_BATON:
	case WP_MELEE:
/*		MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
		weaponInfo->firingSound = trap_S_RegisterSound( "sound/weapons/saber/saberhum.wav" );
//		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/melee/fstatck.wav" );
*/
		//trap_R_RegisterShader( "gfx/effects/stunPass" );
		trap_FX_RegisterEffect( "stunBaton/flesh_impact" );

		if (weaponNum == WP_MELEE)
		{//grapplehook
			MAKERGB( weaponInfo->flashDlightColor, 0.7f, 0.7f, 0.0f );
			weaponInfo->missileModel = trap_R_RegisterModel( "models/items/hook.md3" );
			weaponInfo->missileSound = trap_S_RegisterSound( "sound/weapons/grapple/hookfire.wav");

			MAKERGB( weaponInfo->missileDlightColor, 0.7f, 0.7f, 0.7f );
		
	
	 
																	}	
		if (weaponNum == WP_STUN_BATON)
		{
			trap_S_RegisterSound( "sound/weapons/baton/idle.wav" );

			
			
			
			
			
					weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/concussion/select.wav");

		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/baton/fire.mp3" );
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		//weaponInfo->muzzleEffect		= trap_FX_RegisterEffect( "rocket/muzzle_flash" );
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
		//weaponInfo->missileDlightColor= {0,0,0};
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= FX_DestructionProjectileThink;

		weaponInfo->altFlashSound[0] = trap_S_RegisterSound( "sound/weapons/baton/fire.mp3" ); 
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= trap_S_RegisterSound( "sound/weapons/bryar/altcharge.wav");
		//weaponInfo->altMuzzleEffect		= trap_FX_RegisterEffect( "rocket/altmuzzle_flash" );
		weaponInfo->altMissileModel		= NULL_HANDLE;
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
		//weaponInfo->altMissileDlightColor= {0,0,0};
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_DestructionProjectileThink;


		trap_R_RegisterShader("gfx/effects/blueLine");
		trap_R_RegisterShader("gfx/misc/whiteline2");
		}
		else
		{
			/*
			int j = 0;

			while (j < 4)
			{
				weaponInfo->flashSound[j] = trap_S_RegisterSound( va("sound/weapons/melee/swing%i", j+1) );
				weaponInfo->altFlashSound[j] = weaponInfo->flashSound[j];
				j++;
			}
			*/
			//No longer needed, animsound config plays them for us
		}
		break;
	case WP_SABER:
		MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
		weaponInfo->firingSound = trap_S_RegisterSound( "sound/weapons/saber/saberhum1.wav" );
		weaponInfo->missileModel		= trap_R_RegisterModel( DEFAULT_SABER_MODEL );
		break;


	case WP_BRYAR_PISTOL:
		weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/bryar/select.wav");

		weaponInfo->flashSound[0]		= trap_S_RegisterSound( "sound/weapons/bryar/fire.wav");
		weaponInfo->flashSound2[0]		= trap_S_RegisterSound( "sound/weapons/dh-17/fire.wav");
		weaponInfo->flashSound3[0]		= trap_S_RegisterSound( "sound/weapons/dc-17/fire.wav");
		weaponInfo->flashSound4[0]		= trap_S_RegisterSound( "sound/weapons/w-34/fire.wav");
		weaponInfo->flashSound5[0]		= trap_S_RegisterSound( "sound/weapons/Glie-44/fire.wav");
		weaponInfo->flashSound6[0]		= trap_S_RegisterSound( "sound/weapons/SE-44C/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= trap_FX_RegisterEffect( "bryar/muzzle_flash" );
		weaponInfo->muzzle2Effect		= trap_FX_RegisterEffect( "dh-17/muzzle_flash" );
		weaponInfo->muzzle3Effect		= trap_FX_RegisterEffect( "dc-17/muzzle_flash" );
		weaponInfo->muzzle4Effect		= trap_FX_RegisterEffect( "w-34/muzzle_flash" );
		weaponInfo->muzzle5Effect		= trap_FX_RegisterEffect( "Glie-44/muzzle_flash" );
		weaponInfo->muzzle6Effect		= trap_FX_RegisterEffect( "SE-44C/muzzle_flash" );
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
		//weaponInfo->missileDlightColor= {0,0,0};
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= FX_BryarProjectileThink;
		weaponInfo->missileTrailFunc2	= FX_BryarProjectileThink2;
		weaponInfo->missileTrailFunc3	= FX_BryarProjectileThink3;
		weaponInfo->missileTrailFunc4	= FX_BryarProjectileThink4;
		weaponInfo->missileTrailFunc5	= FX_BryarProjectileThink5;
		weaponInfo->missileTrailFunc6	= FX_BryarProjectileThink6;
		weaponInfo->altFlashSound[0]	= trap_S_RegisterSound( "sound/weapons/bryar/alt_fire.wav");
		weaponInfo->altFlashSound2[0]	= trap_S_RegisterSound( "sound/weapons/dh-17/alt_fire.wav");
		weaponInfo->altFlashSound3[0]	= trap_S_RegisterSound( "sound/weapons/dc-17/alt_fire.wav");
		weaponInfo->altFlashSound4[0]	= trap_S_RegisterSound( "sound/weapons/w-34/alt_fire.wav");
		weaponInfo->altFlashSound5[0]	= trap_S_RegisterSound( "sound/weapons/Glie-44/alt_fire.wav");
		weaponInfo->altFlashSound6[0]	= trap_S_RegisterSound( "sound/weapons/SE-44C/alt_fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= trap_S_RegisterSound( "sound/weapons/bryar/altcharge.wav");
		weaponInfo->altMuzzleEffect		= trap_FX_RegisterEffect( "bryar/muzzle_flash" );
		weaponInfo->altMuzzle2Effect		= trap_FX_RegisterEffect( "dh-17/muzzle_flash" );
		weaponInfo->altMuzzle3Effect		= trap_FX_RegisterEffect( "dc-17/muzzle_flash" );
		weaponInfo->altMuzzle4Effect		= trap_FX_RegisterEffect( "w-34/muzzle_flash" );
		weaponInfo->altMuzzle5Effect		= trap_FX_RegisterEffect( "Glie-44/muzzle_flash" );
		weaponInfo->altMuzzle6Effect		= trap_FX_RegisterEffect( "SE-44C/muzzle_flash" );
		weaponInfo->altMissileModel		= NULL_HANDLE;
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
		//weaponInfo->altMissileDlightColor= {0,0,0};
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_BryarAltProjectileThink;
		weaponInfo->altMissileTrailFunc2 = FX_BryarAltProjectileThink2;
		weaponInfo->altMissileTrailFunc3 = FX_BryarAltProjectileThink3;
		weaponInfo->altMissileTrailFunc4 = FX_BryarAltProjectileThink4;		
		weaponInfo->altMissileTrailFunc5 = FX_BryarAltProjectileThink5;	
		weaponInfo->altMissileTrailFunc6 = FX_BryarAltProjectileThink6;	


		
		
	

		// Note these are temp shared effects


		break;

	case WP_BLASTER:
	case WP_EMPLACED_GUN: //rww - just use the same as this for now..
		weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/blaster/select.wav");

		weaponInfo->flashSound[0]		= trap_S_RegisterSound( "sound/weapons/blaster/fire.wav");
		weaponInfo->flashSound2[0]		= trap_S_RegisterSound( "sound/weapons/a280/fire.wav");
		weaponInfo->flashSound3[0]		= trap_S_RegisterSound( "sound/weapons/dc-15s/fire.wav");
		weaponInfo->flashSound4[0]		= trap_S_RegisterSound( "sound/weapons/e-5b/fire.wav");
		weaponInfo->flashSound5[0]		= trap_S_RegisterSound( "sound/weapons/EL-16/fire.wav");
		weaponInfo->flashSound6[0]		= trap_S_RegisterSound( "sound/weapons/f11d_blaster/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= trap_FX_RegisterEffect( "blaster/muzzle_flash" );
		weaponInfo->muzzle2Effect		= trap_FX_RegisterEffect( "a280/muzzle_flash" );
		weaponInfo->muzzle3Effect		= trap_FX_RegisterEffect( "dc-15s/muzzle_flash" );
		weaponInfo->muzzle4Effect		= trap_FX_RegisterEffect( "e-5b/muzzle_flash" );
		weaponInfo->muzzle5Effect		= trap_FX_RegisterEffect( "EL-16/muzzle_flash" );
		weaponInfo->muzzle6Effect		= trap_FX_RegisterEffect( "f11d_blaster/muzzle_flash" );		
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
//		weaponInfo->missileDlightColor	= {0,0,0};
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= FX_BlasterProjectileThink;
		weaponInfo->missileTrailFunc2	= FX_BlasterProjectileThink2;
		weaponInfo->missileTrailFunc3	= FX_BlasterProjectileThink3;
		weaponInfo->missileTrailFunc4	= FX_BlasterProjectileThink4;
		weaponInfo->missileTrailFunc5	= FX_BlasterProjectileThink5;
		weaponInfo->missileTrailFunc6	= FX_BlasterProjectileThink6;
		weaponInfo->altFlashSound[0]	= trap_S_RegisterSound( "sound/weapons/blaster/alt_fire.wav");
		weaponInfo->altFlashSound2[0]	= trap_S_RegisterSound( "sound/weapons/a280/alt_fire.wav");
		weaponInfo->altFlashSound3[0]	= trap_S_RegisterSound( "sound/weapons/dc-15s/alt_fire.wav");
		weaponInfo->altFlashSound4[0]	= trap_S_RegisterSound( "sound/weapons/e-5b/alt_fire.wav");
		weaponInfo->altFlashSound5[0]	= trap_S_RegisterSound( "sound/weapons/EL-16/alt_fire.wav");
		weaponInfo->altFlashSound6[0]	= trap_S_RegisterSound( "sound/weapons/f11d_blaster/alt_fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= NULL_SOUND;
		weaponInfo->altMuzzleEffect		= trap_FX_RegisterEffect( "blaster/muzzle_flash" );
		weaponInfo->altMuzzle2Effect		= trap_FX_RegisterEffect(  "a280/muzzle_flash" );
		weaponInfo->altMuzzle3Effect		= trap_FX_RegisterEffect( "dc-15s/muzzle_flash" );
		weaponInfo->altMuzzle4Effect		= trap_FX_RegisterEffect( "e-5b/muzzle_flash" );
		weaponInfo->altMuzzle5Effect		= trap_FX_RegisterEffect( "EL-16/muzzle_flash" );
		weaponInfo->altMuzzle6Effect		= trap_FX_RegisterEffect( "f11d_blaster/muzzle_flash" );
		weaponInfo->altMissileModel		= NULL_HANDLE;
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
//		weaponInfo->altMissileDlightColor= {0,0,0};
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_BlasterProjectileThink;
		weaponInfo->altMissileTrailFunc2 = FX_BlasterProjectileThink2;
		weaponInfo->altMissileTrailFunc3 = FX_BlasterProjectileThink3;
		weaponInfo->altMissileTrailFunc4 = FX_BlasterProjectileThink4;
		weaponInfo->altMissileTrailFunc5 = FX_BlasterProjectileThink5;
		weaponInfo->altMissileTrailFunc6 = FX_BlasterProjectileThink6;
		trap_FX_RegisterEffect( "blaster/deflect" );

		


			
		break;

	case WP_DISRUPTOR:
		weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/disruptor/select.wav");

		weaponInfo->flashSound[0]		= trap_S_RegisterSound( "sound/weapons/disruptor/fire.wav");
		weaponInfo->flashSound2[0]		= trap_S_RegisterSound( "sound/weapons/dlt20a/fire.wav");
		weaponInfo->flashSound3[0]		= trap_S_RegisterSound( "sound/weapons/clone_disruptor/fire.wav");
		weaponInfo->flashSound4[0]		= trap_S_RegisterSound( "sound/weapons/projrifle/fire.wav");
		weaponInfo->flashSound5[0]		= trap_S_RegisterSound( "sound/weapons/tusken_rifle/fire.wav");
		weaponInfo->flashSound6[0]		= trap_S_RegisterSound( "sound/weapons/psg/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= trap_FX_RegisterEffect( "disruptor/muzzle_flash" );
		weaponInfo->muzzle2Effect		= trap_FX_RegisterEffect( "DLT20a/muzzle_flash" );
		weaponInfo->muzzle3Effect		= trap_FX_RegisterEffect( "clone_disruptor/muzzle_flash" );
		weaponInfo->muzzle4Effect		= trap_FX_RegisterEffect( "proj_rifle/muzzle_flash" );
		weaponInfo->muzzle5Effect		= trap_FX_RegisterEffect( "tusken_rifle/muzzle_flash" );
		weaponInfo->muzzle6Effect		= trap_FX_RegisterEffect( "psg/muzzle_flash" );
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
//		weaponInfo->missileDlightColor	= {0,0,0};
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= 0;
		weaponInfo->missileTrailFunc2	= 0;
		weaponInfo->missileTrailFunc3	= 0;
		weaponInfo->missileTrailFunc4	= 0;
		weaponInfo->missileTrailFunc5	= 0;
		weaponInfo->missileTrailFunc6	= 0;
		weaponInfo->altFlashSound[0]	= trap_S_RegisterSound( "sound/weapons/disruptor/alt_fire.wav");
		weaponInfo->altFlashSound2[0]	= trap_S_RegisterSound( "sound/weapons/dlt20a/alt_fire.wav");
		weaponInfo->altFlashSound3[0]	= trap_S_RegisterSound( "sound/weapons/clone_disruptor/alt_fire.wav");
		weaponInfo->altFlashSound4[0]	= trap_S_RegisterSound( "sound/weapons/projrifle/alt_fire.wav");
		weaponInfo->altFlashSound5[0]	= trap_S_RegisterSound( "sound/weapons/tusken_rifle/alt_fire.wav");
		weaponInfo->altFlashSound6[0]	= trap_S_RegisterSound( "sound/weapons/psg/alt_fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= trap_S_RegisterSound("sound/weapons/disruptor/altCharge.wav");
		weaponInfo->altMuzzleEffect		= trap_FX_RegisterEffect( "disruptor/muzzle_flash" );
		weaponInfo->altMuzzle2Effect		= trap_FX_RegisterEffect(  "DLT20a/muzzle_flash" );
		weaponInfo->altMuzzle3Effect		= trap_FX_RegisterEffect( "clone_disruptor/muzzle_flash" );
		weaponInfo->altMuzzle4Effect		= trap_FX_RegisterEffect( "proj_rifle/muzzle_flash" );
		weaponInfo->altMuzzle5Effect		= trap_FX_RegisterEffect( "tusken_rifle/muzzle_flash" );
		weaponInfo->altMuzzle6Effect		= trap_FX_RegisterEffect( "psg/muzzle_flash" );
		weaponInfo->altMissileModel		= NULL_HANDLE;
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
//		weaponInfo->altMissileDlightColor= {0,0,0};
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = 0;
		weaponInfo->altMissileTrailFunc2 = 0;
		weaponInfo->altMissileTrailFunc3 = 0;
		weaponInfo->altMissileTrailFunc4 = 0;
		weaponInfo->altMissileTrailFunc5 = 0;
		weaponInfo->altMissileTrailFunc6 = 0;


		trap_R_RegisterShader( "gfx/effects/redLine" );
		trap_R_RegisterShader( "gfx/misc/whiteline2" );
		trap_R_RegisterShader( "gfx/effects/smokeTrail" );

		trap_S_RegisterSound("sound/weapons/disruptor/zoomstart.wav");
		trap_S_RegisterSound("sound/weapons/disruptor/zoomend.wav");


		break;

	case WP_BOWCASTER:
		weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/bowcaster/select.wav");

		weaponInfo->altFlashSound[0]		= trap_S_RegisterSound( "sound/weapons/bowcaster/fire.wav");
		weaponInfo->altFlashSound2[0]	= trap_S_RegisterSound( "sound/weapons/ee-3/sniperfire.wav");
		weaponInfo->altFlashSound3[0]	= trap_S_RegisterSound( "sound/weapons/wbowcaster/alt_fire.wav");
		weaponInfo->altFlashSound4[0]	= trap_S_RegisterSound( "sound/weapons/canderous_blaster/fire.wav");
		weaponInfo->altFlashSound5[0]	= trap_S_RegisterSound( "sound/weapons/Bowcaster_heavy/alt_fire.wav");
		weaponInfo->altFlashSound6[0]	= trap_S_RegisterSound( "sound/weapons/STCompRifle/alt_fire.wav");
		weaponInfo->altFiringSound			= NULL_SOUND;
		weaponInfo->altChargeSound			= NULL_SOUND;
		weaponInfo->altMuzzleEffect		= trap_FX_RegisterEffect( "bowcaster/muzzle_flash" );
		weaponInfo->altMuzzle2Effect		= trap_FX_RegisterEffect(  "ee-3/muzzle_flash" );
		weaponInfo->altMuzzle3Effect		= trap_FX_RegisterEffect( "wbowcaster/muzzle_flash" );
		weaponInfo->altMuzzle4Effect		= trap_FX_RegisterEffect( "canderous_blaster/muzzle_flash" );
		weaponInfo->altMuzzle5Effect		= trap_FX_RegisterEffect( "Bowcaster_heavy/muzzle_flash" );
		weaponInfo->altMuzzle6Effect		= trap_FX_RegisterEffect( "STCompRifle/muzzle_flash" );
		weaponInfo->altMissileModel		= NULL_HANDLE;
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight		= 0;
//		weaponInfo->altMissileDlightColor	= {0,0,0};
		weaponInfo->altMissileHitSound		= NULL_SOUND;
		weaponInfo->altMissileTrailFunc	= FX_BowcasterProjectileThink;
		weaponInfo->altMissileTrailFunc2	= FX_BowcasterProjectileThink2;
		weaponInfo->altMissileTrailFunc3	= FX_BowcasterProjectileThink3;
		weaponInfo->altMissileTrailFunc4	= FX_BowcasterProjectileThink4;
		weaponInfo->altMissileTrailFunc5	= FX_BowcasterProjectileThink5;
		weaponInfo->altMissileTrailFunc6	= FX_BowcasterProjectileThink6;
		weaponInfo->flashSound[0]	= trap_S_RegisterSound( "sound/weapons/bowcaster/fire.wav");
		weaponInfo->flashSound2[0]		= trap_S_RegisterSound( "sound/weapons/ee-3/fire.wav");
		weaponInfo->flashSound3[0]		= trap_S_RegisterSound( "sound/weapons/wbowcaster/fire.wav");
		weaponInfo->flashSound4[0]		= trap_S_RegisterSound( "sound/weapons/canderous_blaster/fire.wav");
		weaponInfo->flashSound5[0]		= trap_S_RegisterSound( "sound/weapons/Bowcaster_heavy/fire.wav");
		weaponInfo->flashSound6[0]		= trap_S_RegisterSound( "sound/weapons/STCompRifle/fire.wav");
		weaponInfo->firingSound		= NULL_SOUND;
		weaponInfo->chargeSound		= trap_S_RegisterSound( "sound/weapons/bowcaster/altcharge.wav");
		weaponInfo->muzzleEffect		= trap_FX_RegisterEffect( "bowcaster/muzzle_flash" );
		weaponInfo->muzzle2Effect		= trap_FX_RegisterEffect( "ee-3/muzzle_flash" );
		weaponInfo->muzzle3Effect		= trap_FX_RegisterEffect( "wbowcaster/muzzle_flash" );
		weaponInfo->muzzle4Effect		= trap_FX_RegisterEffect( "canderous_blaster/muzzle_flash" );
		weaponInfo->muzzle5Effect		= trap_FX_RegisterEffect( "Bowcaster_heavy/muzzle_flash" );
		weaponInfo->muzzle6Effect		= trap_FX_RegisterEffect( "STCompRifle/muzzle_flash" );
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight	= 0;
//		weaponInfo->missileDlightColor= {0,0,0};
		weaponInfo->missileHitSound	= NULL_SOUND;
		weaponInfo->missileTrailFunc = FX_BowcasterAltProjectileThink;
		weaponInfo->missileTrailFunc2 = FX_BowcasterAltProjectileThink2;
		weaponInfo->missileTrailFunc3 = FX_BowcasterAltProjectileThink3;
		weaponInfo->missileTrailFunc4 = FX_BowcasterAltProjectileThink4;
		weaponInfo->missileTrailFunc5 = FX_BowcasterAltProjectileThink5;		
		weaponInfo->missileTrailFunc6 = FX_BowcasterAltProjectileThink6;				


		trap_FX_RegisterEffect( "bowcaster/deflect" );



		
		break;

	case WP_REPEATER:
		weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/repeater/select.wav");

		weaponInfo->flashSound[0]		= trap_S_RegisterSound( "sound/weapons/repeater/fire.wav");
		weaponInfo->flashSound2[0]		= trap_S_RegisterSound( "sound/weapons/dlt-18_repeater/fire.wav");
		weaponInfo->flashSound3[0]		= trap_S_RegisterSound( "sound/weapons/dc-15a/fire.wav");
		weaponInfo->flashSound4[0]		= trap_S_RegisterSound( "sound/weapons/t-21/fire.wav");
		weaponInfo->flashSound5[0]		= trap_S_RegisterSound( "sound/weapons/dc-17m/fire.wav");
		weaponInfo->flashSound6[0]		= trap_S_RegisterSound( "sound/weapons/FWMB-10/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= trap_FX_RegisterEffect( "repeater/muzzle_flash" );
		weaponInfo->muzzle2Effect		= trap_FX_RegisterEffect( "dlt-18_repeater/muzzle_flash" );
		weaponInfo->muzzle3Effect		= trap_FX_RegisterEffect( "dc-15a/muzzle_flash" );
		weaponInfo->muzzle4Effect		= trap_FX_RegisterEffect( "t-21/muzzle_flash" );
		weaponInfo->muzzle5Effect		= trap_FX_RegisterEffect( "dc-17m/muzzle_flash" );
		weaponInfo->muzzle6Effect		= trap_FX_RegisterEffect( "FWMB-10/muzzle_flash" );
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
//		weaponInfo->missileDlightColor	= {0,0,0};
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= FX_RepeaterProjectileThink;
		weaponInfo->missileTrailFunc2	= FX_RepeaterProjectileThink2;
		weaponInfo->missileTrailFunc3	= FX_RepeaterProjectileThink3;
		weaponInfo->missileTrailFunc4	= FX_RepeaterProjectileThink4;
		weaponInfo->missileTrailFunc5	= FX_RepeaterProjectileThink5;
		weaponInfo->missileTrailFunc6	= FX_RepeaterProjectileThink6;
		weaponInfo->altFlashSound[0]	= trap_S_RegisterSound( "sound/weapons/repeater/alt_fire.wav");
		weaponInfo->altFlashSound2[0]		= trap_S_RegisterSound( "sound/weapons/dlt-18_repeater/alt_fire.wav");
		weaponInfo->altFlashSound3[0]		= trap_S_RegisterSound( "sound/weapons/dc-15a/alt_fire.wav");
		weaponInfo->altFlashSound4[0]		= trap_S_RegisterSound( "sound/weapons/t-21/alt_fire.wav");
		weaponInfo->altFlashSound5[0]		= trap_S_RegisterSound( "sound/weapons/dc-17m/alt_fire.wav");
		weaponInfo->altFlashSound6[0]		= trap_S_RegisterSound( "sound/weapons/FWMB-10/alt_fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= NULL_SOUND;
		weaponInfo->altMuzzleEffect		= trap_FX_RegisterEffect( "repeater/muzzle_flash" );
		weaponInfo->altMuzzle2Effect		= trap_FX_RegisterEffect(  "dlt-18_repeater/muzzle_flash" );
		weaponInfo->altMuzzle3Effect		= trap_FX_RegisterEffect( "dc-15a/muzzle_flash" );
		weaponInfo->altMuzzle4Effect		= trap_FX_RegisterEffect( "t-21/muzzle_flash" );
		weaponInfo->altMuzzle5Effect		= trap_FX_RegisterEffect( "dc-17m/muzzle_flash" );
		weaponInfo->altMuzzle6Effect		= trap_FX_RegisterEffect( "FWMB-10/muzzle_flash" );
		weaponInfo->altMissileModel		= NULL_HANDLE;
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
//		weaponInfo->altMissileDlightColor= {0,0,0};
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_RepeaterAltProjectileThink;
		weaponInfo->altMissileTrailFunc2 = FX_RepeaterAltProjectileThink2;
		weaponInfo->altMissileTrailFunc3 = FX_RepeaterAltProjectileThink3;
		weaponInfo->altMissileTrailFunc4 = FX_RepeaterAltProjectileThink4;
		weaponInfo->altMissileTrailFunc5 = FX_RepeaterAltProjectileThink5;
		weaponInfo->altMissileTrailFunc6 = FX_RepeaterAltProjectileThink6;
	
		break;

	case WP_DEMP2:
		weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/demp2/select.wav");

		weaponInfo->flashSound[0]		= trap_S_RegisterSound("sound/weapons/demp2/fire.wav");
		weaponInfo->flashSound2[0]		= trap_S_RegisterSound( "sound/weapons/beamRIFLE/fire.wav");
		weaponInfo->flashSound3[0]		= trap_S_RegisterSound( "sound/weapons/dc-17p/fire.wav");
		weaponInfo->flashSound4[0]		= trap_S_RegisterSound( "sound/weapons/CR-24_flamerifle/flamethrower.wav");
		weaponInfo->flashSound5[0]		= trap_S_RegisterSound( "sound/weapons/CR-25_icerifle/dioxisthrower.wav");
		weaponInfo->flashSound6[0]		= trap_S_RegisterSound( "sound/weapons/CarboniteRifle/icethrower.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= trap_FX_RegisterEffect("demp2/muzzle_flash");
		weaponInfo->muzzle2Effect		= trap_FX_RegisterEffect( "beamRIFLE/muzzle_flash" );
		weaponInfo->muzzle3Effect		= trap_FX_RegisterEffect( "dc-17p/muzzle_flash" );
		weaponInfo->muzzle4Effect		= trap_FX_RegisterEffect( "CR-24_flamerifle/muzzle_flash" );
		weaponInfo->muzzle5Effect		= trap_FX_RegisterEffect( "CR-25_icerifle/muzzle_flash" );
		weaponInfo->muzzle6Effect		= trap_FX_RegisterEffect( "CarboniteRifle/muzzle_flash" );
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
//		weaponInfo->missileDlightColor	= {0,0,0};
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= FX_DEMP2_ProjectileThink;
		weaponInfo->missileTrailFunc2	= FX_DEMP2_ProjectileThink2;
		weaponInfo->missileTrailFunc3	= FX_DEMP2_ProjectileThink3;
		weaponInfo->missileTrailFunc4	= FX_DEMP2_ProjectileThink4;
		weaponInfo->missileTrailFunc5	= FX_DEMP2_ProjectileThink5;
		weaponInfo->missileTrailFunc6	= FX_DEMP2_ProjectileThink6;
		weaponInfo->altFlashSound[0]	= trap_S_RegisterSound("sound/weapons/demp2/altfire.wav");
		weaponInfo->altFlashSound2[0]		= trap_S_RegisterSound("sound/weapons/beamRIFLE/altfire.wav");
		weaponInfo->altFlashSound3[0]		= trap_S_RegisterSound( "sound/weapons/dc-17p/altfire.wav");
		weaponInfo->altFlashSound4[0]		= trap_S_RegisterSound( "sound/weapons/CR-24_flamerifle/flamethrower.wav");
		weaponInfo->altFlashSound5[0]		= trap_S_RegisterSound( "sound/weapons/CR-25_icerifle/dioxisthrower.wav");
		weaponInfo->altFlashSound6[0]		= trap_S_RegisterSound( "sound/weapons/CarboniteRifle/icethrower.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= trap_S_RegisterSound("sound/weapons/demp2/altCharge.wav");
		weaponInfo->altMuzzleEffect		= trap_FX_RegisterEffect("demp2/muzzle_flash");
		weaponInfo->altMuzzle2Effect		= trap_FX_RegisterEffect(  "beamRIFLE/muzzle_flash" );
		weaponInfo->altMuzzle3Effect		= trap_FX_RegisterEffect("dc-17p/muzzle_flash" );
		weaponInfo->altMuzzle4Effect		= trap_FX_RegisterEffect( "CR-24_flamerifle/muzzle_flash" );
		weaponInfo->altMuzzle5Effect		= trap_FX_RegisterEffect( "CR-25_icerifle/muzzle_flash" );
		weaponInfo->altMuzzle6Effect		= trap_FX_RegisterEffect( "CarboniteRifle/muzzle_flash" );
		weaponInfo->altMissileModel		= NULL_HANDLE;
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
//		weaponInfo->altMissileDlightColor= {0,0,0};
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_DEMP2_ProjectileThink;
		weaponInfo->altMissileTrailFunc2 = FX_DEMP2_ProjectileThink2;
		weaponInfo->altMissileTrailFunc3 = FX_DEMP2_ProjectileThink3;
		weaponInfo->altMissileTrailFunc4 = FX_DEMP2_ProjectileThink4;
		weaponInfo->altMissileTrailFunc5 = FX_DEMP2_ProjectileThink5;
		weaponInfo->altMissileTrailFunc6 = FX_DEMP2_ProjectileThink6;
	
	
	
		break;

	case WP_FLECHETTE:
		weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/flechette/select.wav");

		weaponInfo->flashSound[0]		= trap_S_RegisterSound( "sound/weapons/flechette/fire.wav");
		weaponInfo->flashSound2[0]		= trap_S_RegisterSound( "sound/weapons/Bryar_Rifle/fire.wav");
		weaponInfo->flashSound3[0]		= trap_S_RegisterSound( "sound/weapons/DoubleBarrel_ArrayGun/fire.wav");
		weaponInfo->flashSound4[0]		= trap_S_RegisterSound( "sound/weapons/ACP_ArrayGun/fire.wav");
		weaponInfo->flashSound5[0]		= trap_S_RegisterSound( "sound/weapons/cr-1/fire.wav");
		weaponInfo->flashSound6[0]		= trap_S_RegisterSound( "sound/weapons/junglerifle/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= trap_FX_RegisterEffect( "flechette/muzzle_flash" );
		weaponInfo->muzzle2Effect		= trap_FX_RegisterEffect(  "Bryar_Rifle/muzzle_flash" );
		weaponInfo->muzzle3Effect		= trap_FX_RegisterEffect( "DoubleBarrel_ArrayGun/muzzle_flash" );
		weaponInfo->muzzle4Effect		= trap_FX_RegisterEffect( "ACP_ArrayGun/muzzle_flash" );
		weaponInfo->muzzle5Effect		= trap_FX_RegisterEffect( "cr-1/muzzle_flash" );
		weaponInfo->muzzle6Effect		= trap_FX_RegisterEffect( "junglerifle/muzzle_flash" );
		weaponInfo->missileModel		= trap_R_RegisterModel("models/weapons2/golan_arms/projectileMain.md3");
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
//		weaponInfo->missileDlightColor	= {0,0,0};
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= FX_FlechetteProjectileThink;
		weaponInfo->missileTrailFunc2	= FX_FlechetteProjectileThink2;
		weaponInfo->missileTrailFunc3	= FX_FlechetteProjectileThink3;
		weaponInfo->missileTrailFunc4	= FX_FlechetteProjectileThink4;
		weaponInfo->missileTrailFunc5	= FX_FlechetteProjectileThink5;
		weaponInfo->missileTrailFunc6	= FX_FlechetteProjectileThink6;
		weaponInfo->altFlashSound[0]	= trap_S_RegisterSound( "sound/weapons/flechette/alt_fire.wav");
		weaponInfo->altFlashSound2[0]		= trap_S_RegisterSound( "sound/weapons/Bryar_Rifle/fire.wav");
		weaponInfo->altFlashSound3[0]		= trap_S_RegisterSound( "sound/weapons/DoubleBarrel_ArrayGun/alt_fire.wav");
		weaponInfo->altFlashSound4[0]		= trap_S_RegisterSound( "sound/weapons/ACP_ArrayGun/fire.wav");
		weaponInfo->altFlashSound5[0]		= trap_S_RegisterSound( "sound/weapons/cr-1/fire.wav");
		weaponInfo->altFlashSound6[0]		= trap_S_RegisterSound( "sound/weapons/junglerifle/fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= NULL_SOUND;
		weaponInfo->altMuzzleEffect		= trap_FX_RegisterEffect( "flechette/muzzle_flash" );
		weaponInfo->altMuzzle2Effect		= trap_FX_RegisterEffect(  "Bryar_Rifle/muzzle_flash" );
		weaponInfo->altMuzzle3Effect		= trap_FX_RegisterEffect("DoubleBarrel_ArrayGun/muzzle_flash" );
		weaponInfo->altMuzzle4Effect		= trap_FX_RegisterEffect( "ACP_ArrayGun/muzzle_flash" );
		weaponInfo->altMuzzle5Effect		= trap_FX_RegisterEffect( "cr-1/muzzle_flash" );
		weaponInfo->altMuzzle6Effect		= trap_FX_RegisterEffect( "junglerifle/muzzle_flash" );
		weaponInfo->altMissileModel		= trap_R_RegisterModel( "models/weapons2/golan_arms/projectile.md3" );
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
//		weaponInfo->altMissileDlightColor= {0,0,0};
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_FlechetteAltProjectileThink;
		weaponInfo->altMissileTrailFunc2 = FX_FlechetteAltProjectileThink2;
		weaponInfo->altMissileTrailFunc3 = FX_FlechetteAltProjectileThink3;
		weaponInfo->altMissileTrailFunc4 = FX_FlechetteAltProjectileThink4;
		weaponInfo->altMissileTrailFunc5 = FX_FlechetteAltProjectileThink5;
		weaponInfo->altMissileTrailFunc6 = FX_FlechetteAltProjectileThink6;

		break;

	case WP_ROCKET_LAUNCHER:
		weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/rocket/select.wav");

		weaponInfo->flashSound[0]		= trap_S_RegisterSound( "sound/weapons/rocket/fire.wav");
		weaponInfo->flashSound2[0]		= trap_S_RegisterSound( "sound/weapons/MiniMag_launcher/fire.wav");
		weaponInfo->flashSound3[0]		= trap_S_RegisterSound( "sound/weapons/cw_launcher/fire.wav");
		weaponInfo->flashSound4[0]		= trap_S_RegisterSound( "sound/weapons/e60r_launcher/fire.wav");
		weaponInfo->flashSound5[0]		= trap_S_RegisterSound( "sound/weapons/Packered_MortarGun/fire.wav");
		weaponInfo->flashSound6[0]		= trap_S_RegisterSound( "sound/weapons/Packered_MortarGun/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= trap_FX_RegisterEffect( "rocket/muzzle_flash" ); //trap_FX_RegisterEffect( "rocket/muzzle_flash2" );
		weaponInfo->muzzle2Effect		= trap_FX_RegisterEffect( "MiniMag_launcher/muzzle_flash" ); //trap_FX_RegisterEffect( "rocket/muzzle_flash2" );
		weaponInfo->muzzle3Effect		= trap_FX_RegisterEffect( "cw_launcher/muzzle_flash" ); //trap_FX_RegisterEffect( "rocket/muzzle_flash2" );
		weaponInfo->muzzle4Effect		= trap_FX_RegisterEffect( "e60r_launcher/muzzle_flash" ); //trap_FX_RegisterEffect( "rocket/muzzle_flash2" );
		weaponInfo->muzzle5Effect		= trap_FX_RegisterEffect( "Packered_MortarGun/muzzle_flash" ); //trap_FX_RegisterEffect( "rocket/muzzle_flash2" );
		weaponInfo->muzzle6Effect		= trap_FX_RegisterEffect( "Packered_MortarGun/muzzle_flash" ); //trap_FX_RegisterEffect( "rocket/muzzle_flash2" );
		//flash2 still looks crappy with the fx bolt stuff. Because the fx bolt stuff doesn't work entirely right.
		weaponInfo->missileModel		= trap_R_RegisterModel( "models/weapons2/merr_sonn/projectile.md3" );
		weaponInfo->missileSound		= trap_S_RegisterSound( "sound/weapons/rocket/missleloop.wav");
		weaponInfo->missileDlight		= 125;
		VectorSet(weaponInfo->missileDlightColor, 1.0, 1.0, 0.5);
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= FX_RocketProjectileThink;
		weaponInfo->missileTrailFunc2	= FX_RocketProjectileThink;
		weaponInfo->missileTrailFunc3	= FX_RocketProjectileThink;
		weaponInfo->missileTrailFunc4	= FX_RocketProjectileThink;
		weaponInfo->missileTrailFunc5	= FX_RocketProjectileThink;
		weaponInfo->missileTrailFunc6	= FX_RocketProjectileThink;
		weaponInfo->altFlashSound[0]	= trap_S_RegisterSound( "sound/weapons/rocket/alt_fire.wav");
		weaponInfo->altFlashSound2[0]		= trap_S_RegisterSound( "sound/weapons/MiniMag_launcher/fire.wav");
		weaponInfo->altFlashSound3[0]		= trap_S_RegisterSound( "sound/weapons/cw_launcher/fire.wav");
		weaponInfo->altFlashSound4[0]		= trap_S_RegisterSound( "sound/weapons/e60r_launcher/alt_fire.wav");
		weaponInfo->altFlashSound5[0]		= trap_S_RegisterSound( "sound/weapons/Packered_MortarGun/alt_fire.wav");
		weaponInfo->altFlashSound6[0]		= trap_S_RegisterSound( "sound/weapons/Packered_MortarGun/alt_fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= NULL_SOUND;
		weaponInfo->altMuzzleEffect		= trap_FX_RegisterEffect( "rocket/altmuzzle_flash" );
		weaponInfo->altMuzzle2Effect		= trap_FX_RegisterEffect( "MiniMag_launcher/altmuzzle_flash" );
		weaponInfo->altMuzzle3Effect		= trap_FX_RegisterEffect( "cw_launcher/altmuzzle_flash" );
		weaponInfo->altMuzzle4Effect		= trap_FX_RegisterEffect( "e60r_launcher/altmuzzle_flash" );
		weaponInfo->altMuzzle5Effect		= trap_FX_RegisterEffect( "Packered_MortarGun/altmuzzle_flash" );
		weaponInfo->altMuzzle6Effect		= trap_FX_RegisterEffect( "Packered_MortarGun/altmuzzle_flash" );
		weaponInfo->altMissileModel		= trap_R_RegisterModel( "models/weapons2/merr_sonn/projectile.md3" );
		weaponInfo->altMissileSound		= trap_S_RegisterSound( "sound/weapons/rocket/missleloop.wav");
		weaponInfo->altMissileDlight	= 125;
		VectorSet(weaponInfo->altMissileDlightColor, 1.0, 1.0, 0.5);
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_RocketAltProjectileThink;
		weaponInfo->altMissileTrailFunc2 = FX_RocketAltProjectileThink;
		weaponInfo->altMissileTrailFunc3 = FX_RocketAltProjectileThink;
		weaponInfo->altMissileTrailFunc4 = FX_RocketAltProjectileThink;
		weaponInfo->altMissileTrailFunc5 = FX_RocketAltProjectileThink;
		weaponInfo->altMissileTrailFunc6 = FX_RocketAltProjectileThink;
																	   
		
		trap_R_RegisterShaderNoMip( "gfx/2d/wedge" );
		trap_R_RegisterShaderNoMip( "gfx/2d/lock" );

		trap_S_RegisterSound( "sound/weapons/rocket/lock.wav" );
		trap_S_RegisterSound( "sound/weapons/rocket/tick.wav" );
		break;

	case WP_THERMAL:
		weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/thermal/select.wav");

		weaponInfo->flashSound[0]		= trap_S_RegisterSound( "sound/weapons/thermal/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= trap_S_RegisterSound( "sound/weapons/thermal/charge.wav");
		weaponInfo->muzzleEffect		= NULL_FX;
		weaponInfo->missileModel		= trap_R_RegisterModel( "models/weapons2/thermal/thermal_proj.md3" );
		weaponInfo->missileModel2		= trap_R_RegisterModel( "models/weapons2/plasma/plasma_proj.md3" );
		weaponInfo->missileModel3		= trap_R_RegisterModel( "models/weapons2/fraggrenade/thermal_proj.md3" );
		weaponInfo->missileModel4		= trap_R_RegisterModel( "models/weapons2/V-59_concussion/V-59_conc_proj.md3" );		
		weaponInfo->missileModel5		= trap_R_RegisterModel( "models/weapons2/sonic_detonator/sonic_det_proj.md3" );		
		weaponInfo->missileModel6		= trap_R_RegisterModel( "models/weapons2/Stormi_TD/stormiTD_proj.md3" );		
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
//		weaponInfo->missileDlightColor	= {0,0,0};
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= 0;
		weaponInfo->missileTrailFunc2	= 0;
		weaponInfo->missileTrailFunc3	= 0;
		weaponInfo->missileTrailFunc4	= 0;
		weaponInfo->missileTrailFunc5	= 0;
		weaponInfo->missileTrailFunc6	= 0;
		weaponInfo->altFlashSound[0]	= trap_S_RegisterSound( "sound/weapons/thermal/fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= trap_S_RegisterSound( "sound/weapons/thermal/charge.wav");
		weaponInfo->altMuzzleEffect		= NULL_FX;
		weaponInfo->altMissileModel		= trap_R_RegisterModel( "models/weapons2/thermal/thermal_proj.md3" );
		weaponInfo->altMissileModel2		= trap_R_RegisterModel( "models/weapons2/plasma/plasma_proj.md3" );
		weaponInfo->altMissileModel3		= trap_R_RegisterModel( "models/weapons2/fraggrenade/thermal_proj.md3" );
		weaponInfo->altMissileModel4		= trap_R_RegisterModel( "models/weapons2/V-59_concussion/V-59_conc_proj.md3" );		
		weaponInfo->altMissileModel5		= trap_R_RegisterModel( "models/weapons2/sonic_detonator/sonic_det_proj.md3" );	
		weaponInfo->altMissileModel6		= trap_R_RegisterModel( "models/weapons2/Stormi_TD/stormiTD_proj.md3" );	
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
//		weaponInfo->altMissileDlightColor= {0,0,0};
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = 0;
		weaponInfo->altMissileTrailFunc2 = 0;
		weaponInfo->altMissileTrailFunc3 = 0;
		weaponInfo->altMissileTrailFunc4 = 0;
		weaponInfo->altMissileTrailFunc5 = 0;
		weaponInfo->altMissileTrailFunc6 = 0;


		trap_S_RegisterSound( "sound/weapons/thermal/thermloop.wav" );
		trap_S_RegisterSound( "sound/weapons/thermal/warning.wav" );

		break;

	case WP_TRIP_MINE:
		weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/detpack/select.wav");

		weaponInfo->flashSound[0]		= trap_S_RegisterSound( "sound/weapons/laser_trap/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= NULL_FX;
		weaponInfo->missileModel		= 0;//trap_R_RegisterModel( "models/weapons2/laser_trap/laser_trap_w.md3" );
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
//		weaponInfo->missileDlightColor	= {0,0,0};
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= 0;
		weaponInfo->missileTrailFunc2	= 0;
		weaponInfo->missileTrailFunc3	= 0;
		weaponInfo->missileTrailFunc4	= 0;
		weaponInfo->missileTrailFunc5	= 0;
		weaponInfo->missileTrailFunc6	= 0;
		weaponInfo->altFlashSound[0]	= trap_S_RegisterSound( "sound/weapons/laser_trap/fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= NULL_SOUND;
		weaponInfo->altMuzzleEffect		= NULL_FX;
		weaponInfo->altMissileModel		= 0;//trap_R_RegisterModel( "models/weapons2/laser_trap/laser_trap_w.md3" );
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
//		weaponInfo->altMissileDlightColor= {0,0,0};
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = 0;
		weaponInfo->altMissileTrailFunc2 = 0;
		weaponInfo->altMissileTrailFunc3 = 0;
		weaponInfo->altMissileTrailFunc4 = 0;
		weaponInfo->altMissileTrailFunc5 = 0;
		weaponInfo->altMissileTrailFunc6 = 0;


		trap_FX_RegisterEffect( "tripMine/explosion" );
		// NOTENOTE temp stuff
		trap_S_RegisterSound( "sound/weapons/laser_trap/stick.wav" );
		trap_S_RegisterSound( "sound/weapons/laser_trap/warning.wav" );
		break;

	case WP_DET_PACK:
		weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/detpack/select.wav");

		weaponInfo->flashSound[0]		= trap_S_RegisterSound( "sound/weapons/detpack/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= NULL_FX;
		weaponInfo->missileModel		= trap_R_RegisterModel( "models/weapons2/detpack/det_pack.md3" );
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
//		weaponInfo->missileDlightColor	= {0,0,0};
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= 0;
		weaponInfo->missileTrailFunc2	= 0;
		weaponInfo->missileTrailFunc3	= 0;
		weaponInfo->missileTrailFunc4	= 0;
		weaponInfo->missileTrailFunc5	= 0;
		weaponInfo->missileTrailFunc6	= 0;
		weaponInfo->altFlashSound[0]	= trap_S_RegisterSound( "sound/weapons/detpack/fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= NULL_SOUND;
		weaponInfo->altMuzzleEffect		= NULL_FX;
		weaponInfo->altMissileModel		= trap_R_RegisterModel( "models/weapons2/detpack/det_pack.md3" );
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
//		weaponInfo->altMissileDlightColor= {0,0,0};
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = 0;
		weaponInfo->altMissileTrailFunc2 = 0;
		weaponInfo->altMissileTrailFunc3 = 0;
		weaponInfo->altMissileTrailFunc4 = 0;
		weaponInfo->altMissileTrailFunc5 = 0;
		weaponInfo->altMissileTrailFunc6 = 0;
		trap_R_RegisterModel( "models/weapons2/detpack/det_pack.md3" );
		trap_S_RegisterSound( "sound/weapons/detpack/stick.wav" );
		trap_S_RegisterSound( "sound/weapons/detpack/warning.wav" );
		trap_S_RegisterSound( "sound/weapons/explosions/explode5.wav" );
		break;
		
		case WP_CONCUSSION:
		weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/concussion/select.wav");

		weaponInfo->flashSound[0]		= trap_S_RegisterSound( "sound/weapons/concussion/fire.wav");
		weaponInfo->flashSound2[0]		= trap_S_RegisterSound( "sound/weapons/LJ-70_ConcRifle/fire.wav");
		weaponInfo->flashSound3[0]		= trap_S_RegisterSound( "sound/weapons/PulseCannon/fire.wav");
		weaponInfo->flashSound4[0]		= trap_S_RegisterSound( "sound/weapons/geonosian/fire.wav");
		weaponInfo->flashSound5[0]		= trap_S_RegisterSound( "sound/weapons/z6_rotary/fire.wav");
		weaponInfo->flashSound6[0]		= trap_S_RegisterSound( "sound/weapons/minigun/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= trap_FX_RegisterEffect( "concussion/muzzle_flash" );
		weaponInfo->muzzle2Effect		= trap_FX_RegisterEffect( "LJ-70_ConcRifle/muzzle_flash" );
		weaponInfo->muzzle3Effect		= trap_FX_RegisterEffect( "PulseCannon/muzzle_flash" );
		weaponInfo->muzzle4Effect		= trap_FX_RegisterEffect( "geonosian/muzzle_flash" );
		weaponInfo->muzzle5Effect		= trap_FX_RegisterEffect( "z6_rotary/muzzle_flash" );
		weaponInfo->muzzle6Effect		= trap_FX_RegisterEffect( "minigun/muzzle_flash" );
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
		//weaponInfo->missileDlightColor= {0,0,0};
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= FX_ConcussionProjectileThink;
		weaponInfo->missileTrailFunc2	= FX_ConcussionProjectileThink2;
		weaponInfo->missileTrailFunc3	= FX_ConcussionProjectileThink3;
		weaponInfo->missileTrailFunc4	= FX_ConcussionProjectileThink4;
		weaponInfo->missileTrailFunc5	= FX_ConcussionProjectileThink5;
		weaponInfo->missileTrailFunc6	= FX_ConcussionProjectileThink6;
		weaponInfo->altFlashSound[0]	= trap_S_RegisterSound( "sound/weapons/concussion/alt_fire.wav");
		weaponInfo->altFlashSound2[0]		= trap_S_RegisterSound( "sound/weapons/LJ-70_ConcRifle/alt_fire.wav");
		weaponInfo->altFlashSound3[0]		= trap_S_RegisterSound( "sound/weapons/PulseCannon/alt_fire.wav");
		weaponInfo->altFlashSound4[0]		= trap_S_RegisterSound("sound/weapons/geonosian/alt_fire.wav");
		weaponInfo->altFlashSound5[0]		= trap_S_RegisterSound("sound/weapons/z6_rotary/alt_fire.wav");
		weaponInfo->altFlashSound6[0]		= trap_S_RegisterSound("sound/weapons/minigun/alt_fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= trap_S_RegisterSound( "sound/weapons/bryar/altcharge.wav");
		weaponInfo->altMuzzleEffect		= trap_FX_RegisterEffect( "concussion/altmuzzle_flash" );
		weaponInfo->altMuzzle2Effect		= trap_FX_RegisterEffect(  "LJ-70_ConcRifle/muzzle_flash" );
		weaponInfo->altMuzzle3Effect		= trap_FX_RegisterEffect(  "PulseCannon/muzzle_flash"  );
		weaponInfo->altMuzzle4Effect		= trap_FX_RegisterEffect("geonosian/altmuzzle_flash" );
		weaponInfo->altMuzzle5Effect		= trap_FX_RegisterEffect("z6_rotary/altmuzzle_flash" );
		weaponInfo->altMuzzle6Effect		= trap_FX_RegisterEffect("minigun/altmuzzle_flash" );
		weaponInfo->altMissileModel		= NULL_HANDLE;
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
		//weaponInfo->altMissileDlightColor= {0,0,0};
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_ConcussionProjectileThink;
		weaponInfo->altMissileTrailFunc2 = FX_ConcussionProjectileThink2;
		weaponInfo->altMissileTrailFunc3 = FX_ConcussionProjectileThink3;
		weaponInfo->altMissileTrailFunc4 = FX_ConcussionProjectileThink4;
		weaponInfo->altMissileTrailFunc5 = FX_ConcussionProjectileThink5;
		weaponInfo->altMissileTrailFunc6 = FX_ConcussionProjectileThink6;
	
		trap_R_RegisterShader("gfx/effects/blueLine");
		trap_R_RegisterShader("gfx/misc/whiteline2");
		break;

					  
	case WP_BRYAR_OLD:
		weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/bryar/select.wav");

		weaponInfo->flashSound[0]		= trap_S_RegisterSound( "sound/weapons/bryar/fire.wav");
		weaponInfo->flashSound2[0]		= trap_S_RegisterSound( "sound/weapons/s5_heavy_pistol/fire.wav");
		weaponInfo->flashSound3[0]		= trap_S_RegisterSound( "sound/weapons/sc-10_holdout/fire.wav");
		weaponInfo->flashSound4[0]		= trap_S_RegisterSound( "sound/weapons/blaster_gauntlet/fire.wav");
		weaponInfo->flashSound5[0]		= trap_S_RegisterSound( "sound/weapons/mandalorian_gauntlet/fire.wav");
		weaponInfo->flashSound6[0]		= trap_S_RegisterSound( "sound/weapons/sbd_gauntlet/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= trap_FX_RegisterEffect( "bryar_old/muzzle_flash" );
		weaponInfo->muzzle2Effect		= trap_FX_RegisterEffect( "s5_heavy_pistol/muzzle_flash" );
		weaponInfo->muzzle3Effect		= trap_FX_RegisterEffect( "sc-10_holdout/muzzle_flash" );
		weaponInfo->muzzle4Effect		= trap_FX_RegisterEffect( "blaster_gauntlet/muzzle_flash" );
		weaponInfo->muzzle5Effect		= trap_FX_RegisterEffect( "gauntlet/muzzle_flash" );
		weaponInfo->muzzle6Effect		= trap_FX_RegisterEffect( "concussion_new/muzzle_flash" );
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
		//weaponInfo->missileDlightColor= {0,0,0};  
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= FX_BryarOldProjectileThink;
		weaponInfo->missileTrailFunc2	= FX_BryarOldProjectileThink2;
		weaponInfo->missileTrailFunc3	= FX_BryarOldProjectileThink3;
		weaponInfo->missileTrailFunc4	= FX_BryarOldProjectileThink4;
		weaponInfo->missileTrailFunc5	= FX_BryarOldProjectileThink5;
		weaponInfo->missileTrailFunc6	= FX_BryarOldProjectileThink6;
		weaponInfo->altFlashSound[0]	= trap_S_RegisterSound( "sound/weapons/bryar/alt_fire.wav");
		weaponInfo->altFlashSound2[0]		= trap_S_RegisterSound( "sound/weapons/s5_heavy_pistol/alt_fire.wav");
		weaponInfo->altFlashSound3[0]		= trap_S_RegisterSound( "sound/weapons/sc-10_holdout/alt_fire.wav");
		weaponInfo->altFlashSound4[0]		= trap_S_RegisterSound( "sound/weapons/blaster_gauntlet/alt_fire.wav");
		weaponInfo->altFlashSound5[0]		= trap_S_RegisterSound( "sound/weapons/mandalorian_gauntlet/alt_fire.wav");
		weaponInfo->altFlashSound6[0]		= trap_S_RegisterSound( "sound/weapons/sbd_gauntlet/alt_fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= trap_S_RegisterSound( "sound/weapons/bryar/altcharge.wav");
		weaponInfo->altMuzzleEffect		= trap_FX_RegisterEffect( "bryar_old/muzzle_flash" );
		weaponInfo->altMuzzle2Effect		= trap_FX_RegisterEffect( "s5_heavy_pistol/muzzle_flash" );
		weaponInfo->altMuzzle3Effect		= trap_FX_RegisterEffect( "sc-10_holdout/altmuzzle_flash" );
		weaponInfo->altMuzzle4Effect		= trap_FX_RegisterEffect( "blaster_gauntlet/altmuzzle_flash" );	
		weaponInfo->altMuzzle5Effect		= trap_FX_RegisterEffect( "gauntlet/altmuzzle_flash" );	
		weaponInfo->altMuzzle6Effect		= trap_FX_RegisterEffect( "concussion_new/altmuzzle_flash" );			
		weaponInfo->altMissileModel		= NULL_HANDLE;
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
		//weaponInfo->altMissileDlightColor= {0,0,0};
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_BryarOldAltProjectileThink;
		weaponInfo->altMissileTrailFunc2 = FX_BryarOldAltProjectileThink2;
		weaponInfo->altMissileTrailFunc3 = FX_BryarOldAltProjectileThink3;
		weaponInfo->altMissileTrailFunc4 = FX_BryarOldAltProjectileThink4;	
		weaponInfo->altMissileTrailFunc5 = FX_BryarOldAltProjectileThink5;	
		weaponInfo->altMissileTrailFunc6 = FX_BryarOldAltProjectileThink6;			

		
		


		// Note these are temp shared effects

		break;
	case WP_TURRET:
		weaponInfo->flashSound[0]		= NULL_SOUND;
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= NULL_HANDLE;
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= FX_TurretProjectileThink;

		trap_FX_RegisterEffect("effects/blaster/wall_impact.efx");
		trap_FX_RegisterEffect("effects/blaster/flesh_impact.efx");

		break;

	 default:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 1 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/rocket/rocklf1a.wav" );
		break;
	}
}
