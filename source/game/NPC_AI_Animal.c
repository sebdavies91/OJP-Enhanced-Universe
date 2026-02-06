//[SPPortComplete]
// Animal AI (pack + wander) – inspired by SP's AI_Animal.cpp, implemented in MP C gamecode.
// Pure C (no STL/templates) so it compiles in JK2game.

#include "b_local.h"
// g_nav.h is C-safe and provides NF_CLEAR_PATH / WAYPOINT_NONE used by the trap_Nav_* API.
// IMPORTANT: Do not use NPC_MoveToGoalExt() in MP; it is declared but not implemented in this codebase.
#include "g_nav.h"

#define MAX_PACKS            10

#define LEAVE_PACK_DISTANCE  1000
#define JOIN_PACK_DISTANCE   800

// How often leaders pick a new wander destination.
#define WANDER_MIN_TIME      3000
#define WANDER_MAX_TIME      10000

// Radius used when moving toward a leader.
#define FOLLOW_RADIUS        96

static gentity_t *g_animalPackLeaders[MAX_PACKS];
static int g_animalPackLeaderCount = 0;

static int AnimalPack_FindIndex( gentity_t *ent )
{
	int i;
	for ( i = 0; i < g_animalPackLeaderCount; i++ )
	{
		if ( g_animalPackLeaders[i] == ent )
		{
			return i;
		}
	}
	return -1;
}

static void AnimalPack_RemoveIndex( int idx )
{
	if ( idx < 0 || idx >= g_animalPackLeaderCount )
	{
		return;
	}
	g_animalPackLeaderCount--;
	g_animalPackLeaders[idx] = g_animalPackLeaders[g_animalPackLeaderCount];
	g_animalPackLeaders[g_animalPackLeaderCount] = NULL;
}

static void AnimalPack_Add( gentity_t *ent )
{
	if ( !ent )
	{
		return;
	}
	if ( AnimalPack_FindIndex( ent ) >= 0 )
	{
		return;
	}
	if ( g_animalPackLeaderCount >= MAX_PACKS )
	{
		return;
	}
	g_animalPackLeaders[g_animalPackLeaderCount++] = ent;
}

/*
------------------------------------
NPC_AnimalUpdateLeader

Updates pack leader relationships:
- Removes invalid pack leaders.
- Joins nearby packs.
- Ensures we have a leader (possibly ourselves).

Returns our current leader (may be ourselves).
------------------------------------
*/
static gentity_t *NPC_AnimalUpdateLeader( void )
{
	// In MP, analysis tools can't assume the global NPC pointer is always valid.
	if ( !NPC || !NPC->client )
	{
		return NULL;
	}

	gentity_t	*closestLeader;
	float		closestDist;
	int			myLeaderIdx;
	int			i;

	closestLeader = NULL;
	closestDist = 0.0f;
	myLeaderIdx = -1;

	// Prune invalid leaders + find closest leader (not self)
	for ( i = 0; i < g_animalPackLeaderCount; i++ )
	{
		gentity_t *leader;
		float dist;

		leader = g_animalPackLeaders[i];
		if ( !leader || !leader->inuse || leader->health <= 0 )
		{
			if ( NPC->client->leader == leader )
			{
				NPC->client->leader = NULL;
			}
			AnimalPack_RemoveIndex( i );
			i--; // re-check swapped element
			continue;
		}

		if ( leader == NPC )
		{
			myLeaderIdx = i;
			continue;
		}

		dist = Distance( leader->r.currentOrigin, NPC->r.currentOrigin );
		if ( !closestLeader || dist < closestDist )
		{
			closestLeader = leader;
			closestDist = dist;
		}
	}

	// Join closest pack if in range.
	if ( closestLeader && closestDist < JOIN_PACK_DISTANCE )
	{
		// If we were a leader, remove ourselves from leader list.
		if ( NPC->client->leader == NPC && myLeaderIdx >= 0 )
		{
			AnimalPack_RemoveIndex( myLeaderIdx );
			myLeaderIdx = -1;
		}

		NPC->client->leader = closestLeader;
	}

	// Validate existing leader.
	if ( NPC->client->leader )
	{
		if ( NPC->client->leader != NPC )
		{
			// If leader is dead or removed, clear.
			if ( !NPC->client->leader->inuse || NPC->client->leader->health <= 0 )
			{
				NPC->client->leader = NULL;
			}
			// If my leader has a different leader, inherit their leader (pack flattening).
			else if ( NPC->client->leader->client && NPC->client->leader->client->leader
				&& NPC->client->leader->client->leader != NPC->client->leader )
			{
				NPC->client->leader = NPC->client->leader->client->leader;
			}
			// If leader got too far away, leave.
			else if ( Distance( NPC->client->leader->r.currentOrigin, NPC->r.currentOrigin ) > LEAVE_PACK_DISTANCE )
			{
				NPC->client->leader = NULL;
			}
		}
	}

	// If we still have no leader, become one.
	if ( !NPC->client->leader )
	{
		NPC->client->leader = NPC;
		AnimalPack_Add( NPC );
	}

	return NPC->client->leader;
}


/*
-------------------------
NPC_BSAnimal_Default

Used for animal vehicles when unpiloted (VH_ANIMAL).
-------------------------
*/
void NPC_BSAnimal_Default( void )
{
	gentity_t *leader;
	vec3_t threatLocation;
	qboolean evadeThreat;
	qboolean charmedDocile;
	qboolean charmedApproach;

	if ( !NPC || !NPC->client || !NPCInfo )
	{
		return;
	}

	// Only intended for animal vehicles while unpiloted.
	if ( !NPC->m_pVehicle || NPC->m_pVehicle->m_pPilot )
	{
		NPC_BSDefault();
		return;
	}

	// Update pack leader relationships.
	leader = NPC_AnimalUpdateLeader();

	// Basic threat awareness: closest live player (any team) + alert events.
	VectorClear( threatLocation );
	{
		gentity_t *closestPlayer;
		int alertEvent;

		closestPlayer = FindClosestPlayer( NPC->r.currentOrigin, -1 );
		if ( closestPlayer )
		{
			VectorCopy( closestPlayer->client->ps.origin, threatLocation );
		}

		alertEvent = NPC_CheckAlertEvents( qtrue, qtrue, -1, qfalse, AEL_MINOR, qfalse );
		if ( alertEvent >= 0 )
		{
			alertEvent_t *event;
			event = &level.alertEvents[alertEvent];
			if ( event->owner != NPC && Distance( event->position, NPC->r.currentOrigin ) < event->radius )
			{
				VectorCopy( event->position, threatLocation );
			}
		}
	}

	evadeThreat = ( level.time < NPCInfo->investigateSoundDebounceTime );
	charmedDocile = ( level.time < NPCInfo->confusionTime );
	charmedApproach = ( level.time < NPCInfo->charmedTime );

	// Charmed approach: move toward investigateGoal (set by scripts/force powers).
	if ( charmedApproach )
	{
		NPC_SetMoveGoal( NPC, NPCInfo->investigateGoal, 32, qfalse, -1, NULL );
		NPC_MoveToGoal( qtrue );
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

	// Charmed docile: stand still.
	if ( charmedDocile )
	{
		ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

	// Evade: flee from investigateGoal (threat position should have been copied there by event handlers).
	if ( evadeThreat )
	{
		vec3_t fleeFrom;
		VectorCopy( NPCInfo->investigateGoal, fleeFrom );
		if ( VectorCompare( fleeFrom, vec3_origin ) )
		{
			VectorCopy( threatLocation, fleeFrom );
		}

		NPC_StartFlee( NULL, fleeFrom, AEL_DANGER, 1000, 2000 );
		NPC_BSFlee();
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

	// Normal behavior:
	// - Followers move toward their leader.
	// - Leaders wander by selecting random connected nav nodes.
	if ( leader && leader != NPC )
	{
		NPC_SetMoveGoal( NPC, leader->r.currentOrigin, FOLLOW_RADIUS, qfalse, -1, leader );
		NPC_MoveToGoal( qtrue );
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

	// Leader wandering: periodically pick a new neighbor node.
	if ( NPCInfo->investigateDebounceTime < level.time
		|| !NPCInfo->goalEntity
		|| NAV_HitNavGoal( NPC->r.currentOrigin, NPC->r.mins, NPC->r.maxs,
			NPCInfo->goalEntity->r.currentOrigin, NPCInfo->goalRadius, qfalse ) )
	{
		int curNode;

		NPCInfo->investigateDebounceTime = level.time + Q_irand( WANDER_MIN_TIME, WANDER_MAX_TIME );

		curNode = trap_Nav_GetNearestNode( NPC, NPC->waypoint, NF_CLEAR_PATH, WAYPOINT_NONE );
		if ( curNode != WAYPOINT_NONE )
		{
			int numEdges;
			numEdges = trap_Nav_GetNodeNumEdges( curNode );
			if ( numEdges > 0 )
			{
				int edge;
				int nextNode;
				vec3_t nodePos;

				edge = Q_irand( 0, numEdges - 1 );
				nextNode = trap_Nav_GetNodeEdge( curNode, edge );
				if ( nextNode != WAYPOINT_NONE && trap_Nav_GetNodePosition( nextNode, nodePos ) )
				{
					NPC_SetMoveGoal( NPC, nodePos, 32, qtrue, -1, NULL );
				}
			}
		}
	}

	NPC_MoveToGoal( qtrue );
	NPC_UpdateAngles( qtrue, qtrue );
}
