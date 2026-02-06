// Hazard Trooper AI glue (MP)
// SP has a much more elaborate "troop" system; MP already has group/squad logic for troopers.
// This file provides the missing entry points used by the experimental MP and routes the
// Hazard Trooper through the existing Stormtrooper AI, keeping behavior consistent and stable.

#include "b_local.h"

// Treat Hazard Troopers as troopers for group/squad logic.
qboolean NPC_IsTrooper( gentity_t *actor )
{
    if ( !actor || !actor->client )
    {
        return qfalse;
    }

    switch ( actor->client->NPC_class )
    {
    case CLASS_HAZARD_TROOPER:
    case CLASS_STORMTROOPER:
    case CLASS_SWAMPTROOPER:
        return qtrue;
    default:
        break;
    }

    return qfalse;
}



// SP AI_HazardTrooper.cpp uses a "troop" formation system. MP doesn’t have that system,
// but we can safely emulate one key SP behavior for Hazard Troopers: *they keep more spacing*
// inside their squad (SP sets a wider right/side spacing for CLASS_HAZARD_TROOPER).
//
// This is intentionally conservative:
// - Only runs for CLASS_HAZARD_TROOPER.
// - Never touches saber users.
// - Only nudges movement by setting existing Stormtrooper strafe timers.
//
static void HT_MaintainSpacing( int bState )
{
    if ( !NPC || !NPC->client || !NPC->NPC )
    {
        return;
    }

    if ( NPC->client->NPC_class != CLASS_HAZARD_TROOPER )
    {
        return;
    }

    // Safety: do not interfere with saber behavior.
    if ( NPC->client->ps.weapon == WP_SABER )
    {
        return;
    }

    // Only apply during the normal Stormtrooper combat-ish states.
    switch ( bState )
    {
    case BS_DEFAULT:
    case BS_HUNT_AND_KILL:
    case BS_STAND_AND_SHOOT:
        break;
    default:
        return;
    }

    if ( !NPC->NPC->group || NPC->NPC->group->numGroup < 2 )
    {
        return;
    }

    // Don't fight other strafing decisions.
    if ( !TIMER_Done( NPC, "strafeLeft" ) || !TIMER_Done( NPC, "strafeRight" ) )
    {
        return;
    }

    // Debounce so we don't jitter.
    if ( !TIMER_Done( NPC, "ht_spacing" ) )
    {
        return;
    }

    // SP troop system uses wider side spacing for Hazard Troopers (50 units vs 20).
    // We emulate that with a small strafe if we're too close to another trooper.
    const float desiredSpacing = 50.0f;
    const float desiredSpacingSq = desiredSpacing * desiredSpacing;

    // We only want to resolve *blocking/clumping*, not constantly "orbit" around
    // buddies behind us. SP troop code conceptually avoids guys *ahead* in the
    // formation. We emulate that conservatively by prioritizing troopers in front
    // of our movement direction, and only reacting to troopers behind us when
    // we're extremely close.
    float bestDistSq = 99999999.0f;
    gentity_t *closest = NULL;

    vec3_t forward, right;
    vec3_t flatAngles;
    VectorSet( flatAngles, 0.0f, NPC->client->ps.viewangles[YAW], 0.0f );
    AngleVectors( flatAngles, forward, right, NULL );

    for ( int i = 0; i < NPC->NPC->group->numGroup; i++ )
    {
        const int entNum = NPC->NPC->group->member[i].number;
        if ( entNum < 0 || entNum >= level.num_entities )
        {
            continue;
        }

        gentity_t *other = &g_entities[ entNum ];
        if ( !other || other == NPC || !other->inuse || !other->client )
        {
            continue;
        }

        // Only space from other troopers (matches SP troop behavior intent).
        if ( !NPC_IsTrooper( other ) )
        {
            continue;
        }

        const float dSq = DistanceSquared( NPC->r.currentOrigin, other->r.currentOrigin );

        // Prefer resolving spacing against troopers in front of us, like the SP
        // troop/formation intent. If they're behind us and not *right on top* of
        // us, ignore them to reduce unnecessary side-to-side jitter.
        vec3_t toBuddy;
        VectorSubtract( other->r.currentOrigin, NPC->r.currentOrigin, toBuddy );
        if ( DotProduct( toBuddy, forward ) <= 0.0f && dSq > (32.0f * 32.0f) )
        {
            continue;
        }

        if ( dSq < bestDistSq )
        {
            bestDistSq = dSq;
            closest = other;
        }
    }

    if ( !closest || bestDistSq >= desiredSpacingSq )
    {
        return;
    }

    // right/toBuddy computed from flat yaw angles above.
    vec3_t toBuddy;
    VectorSubtract( closest->r.currentOrigin, NPC->r.currentOrigin, toBuddy );

    const int dur = Q_irand( 250, 400 );

    // If buddy is on our right, strafe left (and vice-versa).
    if ( DotProduct( toBuddy, right ) > 0.0f )
    {
        TIMER_Set( NPC, "strafeLeft", dur );
    }
    else
    {
        TIMER_Set( NPC, "strafeRight", dur );
    }

    TIMER_Set( NPC, "ht_spacing", dur + Q_irand( 200, 350 ) );
}

// Called from NPC_BehaviorSet_Stormtrooper (in NPC.c) before running the main Stormtrooper logic.
void NPC_HazardTrooper_PreThink( int bState )
{
    HT_MaintainSpacing( bState );
}

extern void NPC_BehaviorSet_Stormtrooper( int bState );

void NPC_BehaviorSet_Trooper( int bState )
{
    // For now, route through Stormtrooper behavior.
    // This provides SP-like squad cohesion without introducing risky new formation code.
    NPC_BehaviorSet_Stormtrooper( bState );
}
