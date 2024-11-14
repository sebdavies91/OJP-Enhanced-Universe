// Any dedicated force oriented effects

#include "cg_local.h"

/*
-------------------------
FX_ForceDrained
-------------------------
*/
// This effect is not generic because of possible enhancements
void FX_ForceDrained(vec3_t origin, vec3_t dir)
{
	VectorScale(dir, -1.0, dir);
	trap_FX_PlayEffectID(cgs.effects.forceDrained, origin, dir, -1, -1);
}
/*
-------------------------
FX_ForceSevered
-------------------------
*/
// This effect is not generic because of possible enhancements
void FX_ForceSevered(vec3_t origin, vec3_t dir)
{
	VectorScale(dir, -1.0, dir);
	trap_FX_PlayEffectID(cgs.effects.forceSevered, origin, dir, -1, -1);
}	
/*
-------------------------
FX_ForceHealed
-------------------------
*/
// This effect is not generic because of possible enhancements
void FX_ForceHealed(vec3_t origin, vec3_t dir)
{
	VectorScale(dir, -1.0, dir);
	trap_FX_PlayEffectID(cgs.effects.forceHealed, origin, dir, -1, -1);
}
/*
-------------------------
FX_ForceMidichlorian
-------------------------
*/
// This effect is not generic because of possible enhancements
void FX_ForceMidichlorian(vec3_t origin, vec3_t dir)
{
	VectorScale(dir, -1.0, dir);
	trap_FX_PlayEffectID(cgs.effects.forceMidichlorian, origin, dir, -1, -1);
}
/*
-------------------------
FX_Burned
-------------------------
*/
// This effect is not generic because of possible enhancements
void FX_Burned(vec3_t origin, vec3_t dir)
{
	VectorScale(dir, -1.0, dir);
	trap_FX_PlayEffectID(cgs.effects.burned, origin, dir, -1, -1);
}	

/*
-------------------------
FX_Frozen
-------------------------
*/
// This effect is not generic because of possible enhancements
void FX_Frozen(vec3_t origin, vec3_t dir)
{
	VectorScale(dir, -1.0, dir);
	trap_FX_PlayEffectID(cgs.effects.frozen, origin, dir, -1, -1);
}	

/*
-------------------------
FX_Shocked
-------------------------
*/
// This effect is not generic because of possible enhancements
void FX_Shocked(vec3_t origin, vec3_t dir)
{
	VectorScale(dir, -1.0, dir);
	trap_FX_PlayEffectID(cgs.effects.frozen, origin, dir, -1, -1);
}