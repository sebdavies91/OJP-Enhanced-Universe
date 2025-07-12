//
// fx_*.c
//

// NOTENOTE This is not the best, DO NOT CHANGE THESE!
#define	FX_ALPHA_LINEAR		0x00000001
#define	FX_SIZE_LINEAR		0x00000100



// Bryar
void FX_BryarProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarAltProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarProjectileThink2(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarAltProjectileThink2(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarProjectileThink3(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarAltProjectileThink3(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarProjectileThink4(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarAltProjectileThink4(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarProjectileThink5(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarAltProjectileThink5(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarProjectileThink6(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarAltProjectileThink6(  centity_t *cent, const struct weaponInfo_s *weapon );

void FX_BryarHitWall( vec3_t origin, vec3_t normal );
void FX_BryarAltHitWall( vec3_t origin, vec3_t normal, int power );
void FX_BryarHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );
void FX_BryarAltHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );

// Blaster
void FX_BlasterProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BlasterProjectileThink2( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BlasterProjectileThink3( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BlasterProjectileThink4( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BlasterProjectileThink5(centity_t* cent, const struct weaponInfo_s* weapon);
void FX_BlasterProjectileThink6(centity_t* cent, const struct weaponInfo_s* weapon);

void FX_BlasterAltFireThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BlasterWeaponHitWall( vec3_t origin, vec3_t normal );
void FX_BlasterWeaponHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );

// Disruptor
void FX_DisruptorMainShot( vec3_t start, vec3_t end );
void FX_DisruptorMainShot2( vec3_t start, vec3_t end );
void FX_DisruptorMainShot3( vec3_t start, vec3_t end );
void FX_DisruptorMainShot4( vec3_t start, vec3_t end );
void FX_DisruptorMainShot5( vec3_t start, vec3_t end );
void FX_DisruptorMainShot6( vec3_t start, vec3_t end );
void FX_DisruptorAltShot( vec3_t start, vec3_t end, qboolean fullCharge );
void FX_DisruptorAltShot2( vec3_t start, vec3_t end, qboolean fullCharge );
void FX_DisruptorAltShot3( vec3_t start, vec3_t end, qboolean fullCharge );
void FX_DisruptorAltShot4( vec3_t start, vec3_t end, qboolean fullCharge );
void FX_DisruptorAltShot5( vec3_t start, vec3_t end, qboolean fullCharge );
void FX_DisruptorAltShot6( vec3_t start, vec3_t end, qboolean fullCharge );
void FX_DisruptorAltMiss( vec3_t origin, vec3_t normal );
void FX_DisruptorAltHit( vec3_t origin, vec3_t normal );
void FX_DisruptorHitWall( vec3_t origin, vec3_t normal );
void FX_DisruptorHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );

// Bowcaster
void FX_BowcasterProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BowcasterAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BowcasterProjectileThink2( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BowcasterAltProjectileThink2( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BowcasterProjectileThink3( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BowcasterAltProjectileThink3( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BowcasterProjectileThink4( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BowcasterAltProjectileThink4( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BowcasterProjectileThink5( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BowcasterAltProjectileThink5( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BowcasterProjectileThink6( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BowcasterAltProjectileThink6( centity_t *cent, const struct weaponInfo_s *weapon );

void FX_BowcasterHitWall( vec3_t origin, vec3_t normal );
void FX_BowcasterHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );

// Heavy Repeater
void FX_RepeaterProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_RepeaterAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_RepeaterProjectileThink2( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_RepeaterAltProjectileThink2( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_RepeaterProjectileThink3( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_RepeaterAltProjectileThink3( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_RepeaterProjectileThink4( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_RepeaterAltProjectileThink4( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_RepeaterProjectileThink5( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_RepeaterAltProjectileThink5( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_RepeaterProjectileThink6( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_RepeaterAltProjectileThink6( centity_t *cent, const struct weaponInfo_s *weapon );

void FX_RepeaterHitWall( vec3_t origin, vec3_t normal );
void FX_RepeaterAltHitWall( vec3_t origin, vec3_t normal );
void FX_RepeaterAltHitWall2( vec3_t origin, vec3_t normal );
void FX_RepeaterAltHitWall3( vec3_t origin, vec3_t normal );
void FX_RepeaterAltHitWall4( vec3_t origin, vec3_t normal );
void FX_RepeaterAltHitWall5( vec3_t origin, vec3_t normal );
void FX_RepeaterAltHitWall6( vec3_t origin, vec3_t normal );
void FX_RepeaterHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );
void FX_RepeaterAltHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );
void FX_RepeaterAltHitPlayer2( vec3_t origin, vec3_t normal, qboolean humanoid );
void FX_RepeaterAltHitPlayer3( vec3_t origin, vec3_t normal, qboolean humanoid );
void FX_RepeaterAltHitPlayer4( vec3_t origin, vec3_t normal, qboolean humanoid );
void FX_RepeaterAltHitPlayer5( vec3_t origin, vec3_t normal, qboolean humanoid );
void FX_RepeaterAltHitPlayer6( vec3_t origin, vec3_t normal, qboolean humanoid );

// DEMP2
void FX_DEMP2_ProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_DEMP2_ProjectileThink2( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_DEMP2_ProjectileThink3( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_DEMP2_ProjectileThink4( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_DEMP2_ProjectileThink5( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_DEMP2_ProjectileThink6( centity_t *cent, const struct weaponInfo_s *weapon );

void FX_DEMP2_HitWall( vec3_t origin, vec3_t normal );
void FX_DEMP2_HitWall2( vec3_t origin, vec3_t normal );
void FX_DEMP2_HitWall3( vec3_t origin, vec3_t normal );
void FX_DEMP2_HitWall4( vec3_t origin, vec3_t normal );
void FX_DEMP2_HitWall5( vec3_t origin, vec3_t normal );
void FX_DEMP2_HitWall6( vec3_t origin, vec3_t normal );
void FX_DEMP2_HitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );
void FX_DEMP2_HitPlayer2( vec3_t origin, vec3_t normal, qboolean humanoid );
void FX_DEMP2_HitPlayer3( vec3_t origin, vec3_t normal, qboolean humanoid );
void FX_DEMP2_HitPlayer4( vec3_t origin, vec3_t normal, qboolean humanoid );
void FX_DEMP2_HitPlayer5( vec3_t origin, vec3_t normal, qboolean humanoid );
void FX_DEMP2_HitPlayer6( vec3_t origin, vec3_t normal, qboolean humanoid );
void FX_DEMP2_AltDetonate( vec3_t org, float size );

// Golan Arms Flechette
void FX_FlechetteProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_FlechetteProjectileThink2( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_FlechetteProjectileThink3( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_FlechetteProjectileThink4( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_FlechetteProjectileThink5( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_FlechetteProjectileThink6( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_FlechetteWeaponHitWall( vec3_t origin, vec3_t normal );
void FX_FlechetteWeaponHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );
void FX_FlechetteAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_FlechetteAltProjectileThink2( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_FlechetteAltProjectileThink3( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_FlechetteAltProjectileThink4( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_FlechetteAltProjectileThink5( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_FlechetteAltProjectileThink6( centity_t *cent, const struct weaponInfo_s *weapon );

// Personal Rocket Launcher
void FX_RocketProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_RocketAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_RocketHitWall( vec3_t origin, vec3_t normal );
void FX_RocketHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );

// Bryar Old
void FX_BryarOldProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarOldAltProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarOldProjectileThink2(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarOldAltProjectileThink2(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarOldProjectileThink3(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarOldAltProjectileThink3(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarOldProjectileThink4(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarOldAltProjectileThink4(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarOldProjectileThink5(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarOldAltProjectileThink5(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarOldProjectileThink6(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarOldAltProjectileThink6(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarOldHitWall( vec3_t origin, vec3_t normal );
void FX_BryarOldAltHitWall( vec3_t origin, vec3_t normal, int power );
void FX_BryarOldHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );
void FX_BryarOldAltHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );

// Emplaced
void FX_EmplacedProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_EmplacedAltFireThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_EmplacedWeaponHitWall( vec3_t origin, vec3_t normal );
void FX_EmplacedWeaponHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );
