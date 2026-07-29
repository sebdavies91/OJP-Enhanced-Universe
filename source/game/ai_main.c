// Copyright (C) 1999-2000 Id Software, Inc.
//

/*****************************************************************************
 * name:		ai_main.c
 *
 * desc:		Quake3 bot AI
 *****************************************************************************/


#include "g_local.h"
#include "q_shared.h"
#include "botlib.h"		//bot lib interface
#include "be_aas.h"
#include "bg_public.h"
//[SaberSys]
//I think this isn't defined correctly.
#include "../game/be_ea.h"
//#include "be_ea.h"
//[/SaberSys]
#include "be_ai_char.h"
#include "be_ai_chat.h"
#include "be_ai_gen.h"
#include "be_ai_goal.h"
#include "be_ai_move.h"
#include "be_ai_weap.h"
//
#include "ai_main.h"
#include "w_saber.h"
//
#include "chars.h"
#include "inv.h"
#include "syn.h"

/*
#define BOT_CTF_DEBUG	1
*/


//[BotTweaks]
//Make the bot ai run at a seperate fps than the world updates.  This is allow players to
//up the sv_fps without unintentionally upping this as well.
//[TABBot]
//moved to ai_main.h so we could seperate out the tab code into ai_tab.c
//#define BOT_THINK_TIME	1000/bot_fps.integer
//#define BOT_THINK_TIME	0
//[/BotTweaks]

//bot states
bot_state_t	*botstates[MAX_CLIENTS];
//number of bots
int numbots;
//floating point time
float floattime;
//time to do a regular update
float regularupdate_time;
//
static vec3_t botLastPos[MAX_CLIENTS];
static int    botLastSampleTime[MAX_CLIENTS];
static int    botStuckTime[MAX_CLIENTS];
static int    botCTFRoleUpdateTime[TEAM_NUM_TEAMS];
static int    botTeamRoleUpdateTime[TEAM_NUM_TEAMS];
static int    botLastEnemyTarget[MAX_CLIENTS];
static int    botLastEnemyTargetTime[MAX_CLIENTS];

static int    botMoveCommitUntil[MAX_CLIENTS];
static int    botMoveCommitFlags[MAX_CLIENTS];


extern qboolean G_ValidSaberStyle(gentity_t *ent, int saberStyle);

static int Bot_ValidateSingleSaberStyle(gentity_t *ent, int preferredStyle)
{
	int style;
	int count;

	if (!ent || !ent->client)
	{
		return SS_MEDIUM;
	}

	if (preferredStyle < SS_FAST || preferredStyle > SS_TAVION)
	{
		preferredStyle = SS_MEDIUM;
	}

	if (G_ValidSaberStyle(ent, preferredStyle))
	{
		return preferredStyle;
	}

	style = preferredStyle;
	for (count = SS_FAST; count <= SS_TAVION; count++)
	{
		style++;
		if (style > SS_TAVION)
		{
			style = SS_FAST;
		}

		if (G_ValidSaberStyle(ent, style))
		{
			return style;
		}
	}

	return SS_MEDIUM;
}

static int Bot_SelectSingleSaberCombatStyle(bot_state_t *bs)
{
	gentity_t *self;
	gentity_t *enemy;
	int targetStyle = SS_MEDIUM;
	qboolean enemyUsingSaber = qfalse;
	qboolean enemyInBigMove = qfalse;

	if (!bs)
	{
		return SS_MEDIUM;
	}

	self = &g_entities[bs->client];
	enemy = bs->currentEnemy;

	if (bs->saberStyleBiasTime < level.time)
	{
		bs->saberStyleBias = Q_irand(SS_FAST, SS_TAVION);
		bs->saberStyleBiasTime = level.time + Q_irand(7000, 16000);
	}

	if (enemy && enemy->client)
	{
		enemyUsingSaber = (enemy->client->ps.weapon == WP_SABER);
		enemyInBigMove = (BG_SaberInKata(enemy->client->ps.saberMove)
			|| BG_SaberInSpecial(enemy->client->ps.saberMove)
			|| enemy->client->ps.saberMove == LS_SPINATTACK
			|| enemy->client->ps.saberMove == LS_SPINATTACK_DUAL);
	}

	if (enemy && enemyInBigMove && bs->frame_Enemy_Len < 128)
	{
		// TAVION/purple is MAKASHI: FAST/SORESU + MEDIUM/SHII-CHO hybrid, keep it in the close pressure bucket.
		targetStyle = (bs->saberStyleBias == SS_TAVION) ? SS_TAVION : SS_FAST;
	}
	else if (enemy && enemy->health > 175 && bs->frame_Enemy_Len > 96)
	{
		// DESANN/green is JUYO: MEDIUM/SHII-CHO + STRONG/DJEM SO hybrid, prefer it for stronger ranged/punish choices.
		targetStyle = (bs->saberStyleBias == SS_DESANN) ? SS_DESANN : SS_STRONG;
	}
	else if (enemy && enemy->health < 55)
	{
		targetStyle = (bs->saberStyleBias == SS_TAVION) ? SS_TAVION : SS_FAST;
	}
	else if (bs->frame_Enemy_Len < 70)
	{
		targetStyle = (bs->saberStyleBias == SS_TAVION) ? SS_TAVION : SS_FAST;
	}
	else if (enemyUsingSaber && bs->cur_ps.stats[STAT_DODGE] < 45)
	{
		targetStyle = (bs->saberStyleBias == SS_DESANN || bs->saberStyleBias == SS_TAVION) ? bs->saberStyleBias : SS_MEDIUM;
	}
	else if (bs->frame_Enemy_Len > 140)
	{
		targetStyle = (bs->saberStyleBias == SS_DESANN) ? SS_DESANN : SS_STRONG;
	}
	else
	{
		targetStyle = bs->saberStyleBias;
	}

	return Bot_ValidateSingleSaberStyle(self, targetStyle);
}

static qboolean BotForceIsOffensive(int forcePower)
{
    switch (forcePower)
    {
        case FP_GRIP:
        case FP_LIGHTNING:
        case FP_DRAIN:
        case FP_RAGE:
        case FP_PUSH:
        case FP_PULL:
        case FP_TELEPATHY:
        case FP_SABERTHROW:
            return qtrue;
        default:
            return qfalse;
    }
}

/*
 * Legacy entry points: TAB/AOTC/HYBRID used to live in separate .c files.
 * We keep the symbols so the rest of the code (and project files) don't need
 * to change. The behavioural differences live inside StandardBotAI via
 * botType checks and small "style patches".
 */
void TAB_StandardBotAI(bot_state_t *bs, float thinktime);
void AOTC_StandardBotAI(bot_state_t *bs, float thinktime);
void HYBRID_StandardBotAI(bot_state_t *bs, float thinktime);
void StandardBotAI(bot_state_t *bs, float thinktime);

int BotGetWeaponRange(bot_state_t *bs);

static int    botCliffBrakeUntil[MAX_CLIENTS];
static int    botCliffBrakeFlags[MAX_CLIENTS];
static int    botLedgeHangStart[MAX_CLIENTS];
static int    botLedgeLastZ[MAX_CLIENTS];
static int    botLedgeForceDropUntil[MAX_CLIENTS];
static int    botWPCommitUntil[MAX_CLIENTS];
static int    botWPCommitIndex[MAX_CLIENTS];
// Helps prevent "ping-pong" between two adjacent waypoints when switching branches.
static int    botPrevWPCommitIndex[MAX_CLIENTS];
// --- Combat cover/peek state (kept external to bot_state_t to avoid ABI churn) ---
static int    botCoverUntil[MAX_CLIENTS];
static int    botCoverWpIndex[MAX_CLIENTS];
static int    botPeekUntil[MAX_CLIENTS];
static int    botPeekState[MAX_CLIENTS]; // 0=in cover, 1=peek/out

static int    botWPLastProgressTime[MAX_CLIENTS];

// Waypoint ping-pong / loop detection (helps maps where bots oscillate at corners/doors)
static int    botLastWPVisited[MAX_CLIENTS];
static int    botPrevWPVisited[MAX_CLIENTS];
static int    botLastWPSwitchTime[MAX_CLIENTS];
static int    botWPPingPongCount[MAX_CLIENTS];

static float  botWPLastProgressDist[MAX_CLIENTS];
static int    botLastThermalTime[MAX_CLIENTS];

// Force/item "cover" throttles (do not change loadouts; only decide when to use what the bot already has).
static int    botForceCoverUntil[MAX_CLIENTS];
static int    botItemCoverUntil[MAX_CLIENTS];


// Burst-fire gating for trooper-like bots (AOTC/HYBRID).
// Helps them feel less like perfect aimbots and more like Battlefront-style suppression.
static int    botBurstFireUntil[MAX_CLIENTS];
static int    botBurstFireCooldownUntil[MAX_CLIENTS];

// Timestamp of last time this bot took damage (separate from bs->lastHurt pointer).
static int    botLastHurtTime[MAX_CLIENTS];


// -----------------------------------------------------------------
// Forward declarations for style patching helpers (avoid implicit externs)
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// Style variance (does NOT touch loadouts/weights)
// -----------------------------------------------------------------
static qboolean botStylePersonalityInit[MAX_CLIENTS];
static float    botStylePersonalityScalar[MAX_CLIENTS];
static int      botStyleCircleDir[MAX_CLIENTS]; // 0 = left, 1 = right

static float BotStyle_GetPersonalityScalar(bot_state_t *bs)
{
    int clientNum;
    float scalar;
    int jitterPct;

    if (!bs)
    {
        return 1.0f;
    }

    clientNum = bs->client;
    if (clientNum < 0 || clientNum >= MAX_CLIENTS)
    {
        return 1.0f;
    }

    if (botStylePersonalityInit[clientNum])
    {
        return botStylePersonalityScalar[clientNum];
    }

    // Default bots: keep fully deterministic/stock.
    if ((bs && bs->settings.botType == BOT_DEFAULT))
    {
        botStylePersonalityInit[clientNum] = qtrue;
        botStylePersonalityScalar[clientNum] = 1.0f;
        botStyleCircleDir[clientNum] = 0;
        return 1.0f;
    }

    // TAB: intentionally tighter variance ("samey" within TAB is OK).
    // HYBRID/AOTC: more variance so they don't feel copy-pasted.
    jitterPct = 0;
    if ((bs && bs->settings.botType == BOT_TAB))
    {
        jitterPct = Q_irand(-5, 5);
    }
    else if ((bs && bs->settings.botType == BOT_HYBRID))
    {
        jitterPct = Q_irand(-12, 12);
    }
    else if ((bs && bs->settings.botType == BOT_AOTC))
    {
        jitterPct = Q_irand(-18, 18);
    }

    scalar = 1.0f + ((float)jitterPct * 0.01f);

    // Safety clamp
    if (scalar < 0.80f)
        scalar = 0.80f;
    if (scalar > 1.25f)
        scalar = 1.25f;

    botStylePersonalityInit[clientNum] = qtrue;
    botStylePersonalityScalar[clientNum] = scalar;
    botStyleCircleDir[clientNum] = Q_irand(0, 1);
    return scalar;
}

static int BotStyle_CountNearbyClients(const bot_state_t *bs, float radius, qboolean countFriendlies)
{
    int i;
    int count = 0;
    float r2;
    team_t myTeam;
    vec3_t d;
    const gentity_t *me;

    if (!bs)
        return 0;

    me = &g_entities[bs->client];
    myTeam = me->client ? me->client->sess.sessionTeam : TEAM_FREE;

    r2 = radius * radius;

    for (i = 0; i < level.maxclients; i++)
    {
        const gentity_t *ent = &g_entities[i];

        if (i == bs->client)
            continue;
        if (!ent->inuse || !ent->client)
            continue;
        if (ent->health <= 0)
            continue;
        if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
            continue;

        if (myTeam != TEAM_FREE)
        {
            qboolean sameTeam = (ent->client->sess.sessionTeam == myTeam);
            if (countFriendlies && !sameTeam)
                continue;
            if (!countFriendlies && sameTeam)
                continue;
        }
        // TEAM_FREE: treat everyone else as enemy for "enemy count", none as friendly.
        else
        {
            if (countFriendlies)
                continue;
        }

        VectorSubtract(ent->client->ps.origin, bs->origin, d);
        if (VectorLengthSquared(d) <= r2)
            count++;
    }

    return count;
}


// Find a nearby waypoint that acts like 'cover' by breaking LOS to the enemy.
// We use existing snipe/camp waypoints as proxy cover points (common in MP waypoint packs).
static int BotFindNearbyCoverWaypoint(bot_state_t *bs, const vec3_t enemyOrg)
{
    int i;
    int best = -1;
    float bestScore = -999999.0f;
    trace_t tr;
    vec3_t start, end, toWp;

    if (!bs || gWPNum <= 0)
        return -1;

    for (i = 0; i < gWPNum; i++)
    {
        wpobject_t *wp = gWPArray[i];
        float dMe, dEnemy, score;

        if (!wp)
            continue;
        if (!wp->inuse)
            continue;

        // Prefer existing camp/snipe points as "cover" proxies.
        if (!(wp->flags & WPFLAG_SNIPEORCAMP) && !(wp->flags & WPFLAG_SNIPEORCAMPSTAND))
            continue;

        VectorSubtract(wp->origin, bs->origin, toWp);
        dMe = VectorLength(toWp);
        if (dMe > 900.0f)
            continue;

        // Avoid running to a cover point that is basically on top of the enemy.
        VectorSubtract(enemyOrg, wp->origin, toWp);
        dEnemy = VectorLength(toWp);
        if (dEnemy < 220.0f)
            continue;

        // Cover check: LOS from waypoint to enemy must be blocked.
        VectorCopy(wp->origin, start);
        VectorCopy(enemyOrg, end);
        trap_Trace(&tr, start, NULL, NULL, end, bs->client, MASK_PLAYERSOLID);
        if (tr.fraction > 0.85f)
            continue;

        // Score: closer to us is better, moderate distance to enemy is better.
        score = 0.0f;
        score += (900.0f - dMe);
        score += (dEnemy > 400.0f ? 50.0f : 0.0f);

        if (score > bestScore)
        {
            bestScore = score;
            best = i;
        }
    }

    return best;
}

static void BotStyle_ApplyCombatPositioning(bot_state_t *bs, float personality)
{
    vec3_t eorg, toEnemy, side, up;
    float dist;
    float desired;
    float retreatBand;
    float chaseBand;
    float orbit;
    int friendsNear, enemiesNear, odds;
    int myHP;

    if (!bs || !bs->currentEnemy)
        return;

    // Keep BOT_DEFAULT behaviour intact.
    if ((bs && bs->settings.botType == BOT_DEFAULT))
        return;

    // Only when we can actually see the target; otherwise normal navigation should prevail.
    if (!bs->frame_Enemy_Vis)
        return;

    // Don't fight the camping system or explicit "be still" states.
    if (bs->wpCamping || bs->beStill > level.time)
        return;

    myHP = g_entities[bs->client].health;

    // Establish enemy origin (prefer playerstate if available).
    if (bs->currentEnemy->client)
        VectorCopy(bs->currentEnemy->client->ps.origin, eorg);
    else
        VectorCopy(bs->currentEnemy->r.currentOrigin, eorg);

    VectorSubtract(eorg, bs->origin, toEnemy);
    dist = VectorLength(toEnemy);
    if (dist < 1.0f)
        return;
    VectorScale(toEnemy, 1.0f / dist, toEnemy);

    // Local team pressure: simple, cheap heuristic (no weights/loadouts affected).
    friendsNear = BotStyle_CountNearbyClients(bs, 700.0f, qtrue);
    enemiesNear = BotStyle_CountNearbyClients(bs, 700.0f, qfalse);
    odds = enemiesNear - friendsNear;


    // -----------------------------------------------------------------
    // AOTC/HYBRID: basic cover-seek + peek rhythm (trooper flavour)
    // Uses existing snipe/camp waypoints as cover proxies.
    // Carefully scoped: no BOT_DEFAULT, no saber behaviour.
    // -----------------------------------------------------------------
    if ((bs->settings.botType == BOT_AOTC || bs->settings.botType == BOT_HYBRID) && bs->cur_ps.weapon != WP_SABER)
    {
        int clientNum = bs->client;
        int pressure = 0;

        if (odds >= 1)
            pressure++;
        if (myHP > 0 && myHP < 40)
            pressure++;
        if (botLastHurtTime[clientNum] > level.time - 1200)
            pressure++;

        // Expire cover state if we lost visibility or enemy changed.
        if (!bs->frame_Enemy_Vis)
        {
            botCoverUntil[clientNum] = 0;
            botPeekUntil[clientNum] = 0;
            botPeekState[clientNum] = 0;
            botCoverWpIndex[clientNum] = -1;
        }

        // If we are currently in cover mode, alternate between hiding and peeking.
        if (botCoverUntil[clientNum] > level.time && botCoverWpIndex[clientNum] >= 0)
        {
            wpobject_t *cwp = gWPArray[botCoverWpIndex[clientNum]];
            if (cwp && cwp->inuse)
            {
                // Toggle peek state on timer.
                if (botPeekUntil[clientNum] <= level.time)
                {
                    if (botPeekState[clientNum])
                    {
                        botPeekState[clientNum] = 0;
                        botPeekUntil[clientNum] = level.time + 450 + (int)(200.0f * personality);
                    }
                    else
                    {
                        botPeekState[clientNum] = 1;
                        botPeekUntil[clientNum] = level.time + 280 + (int)(140.0f * personality);
                    }
                }

                if (!botPeekState[clientNum])
                {
                    // Stay tucked in cover.
                    VectorCopy(cwp->origin, bs->goalPosition);
                }
                else
                {
                    // Peek: step a little out toward the enemy from cover point.
                    vec3_t peekGoal;
                    VectorCopy(cwp->origin, peekGoal);
                    peekGoal[0] += toEnemy[0] * 120.0f;
                    peekGoal[1] += toEnemy[1] * 120.0f;
                    VectorCopy(peekGoal, bs->goalPosition);
                }
                return; // while in cover mode, do not apply other positioning.
            }
            else
            {
                botCoverUntil[clientNum] = 0;
                botCoverWpIndex[clientNum] = -1;
            }
        }

        // Consider taking cover when pressured and not already extremely close.
        if (pressure >= 2 && dist > 360.0f && dist < 1400.0f)
        {
            // Throttle cover searches.
            if (botCoverUntil[clientNum] <= level.time)
            {
                int cwp = BotFindNearbyCoverWaypoint(bs, eorg);
                if (cwp >= 0)
                {
                    botCoverWpIndex[clientNum] = cwp;
                    botCoverUntil[clientNum] = level.time + 2200 + (int)(600.0f * personality);
                    botPeekUntil[clientNum] = level.time + 350;
                    botPeekState[clientNum] = 0;

                    // Drive toward cover immediately.
                    VectorCopy(gWPArray[cwp]->origin, bs->goalPosition);
                    return;
                }
            }
        }
    }

    VectorSet(up, 0, 0, 1);
    CrossProduct(toEnemy, up, side);
    if (VectorNormalize(side) == 0.0f)
        return;
    if (!botStyleCircleDir[bs->client])
        VectorScale(side, -1.0f, side);

    // -----------------------------------------------------------------
    // TAB (Force Unleashed flavour): relentless pressure + orbiting
    // -----------------------------------------------------------------
    if ((bs && bs->settings.botType == BOT_TAB))
    {
        // Prefer close range even with non-saber weapons (we do NOT change weapons).
        desired = (bs->cur_ps.weapon == WP_SABER) ? 160.0f : 260.0f;
        desired *= personality;

        // TAB almost never "kites"; even outnumbered it presses.
        retreatBand = desired * 0.55f;
        chaseBand   = desired * 1.60f;

        orbit = (bs->cur_ps.weapon == WP_SABER) ? (210.0f * personality) : (140.0f * personality);

        // Hard pressure when outside band.
        if (dist > chaseBand)
        {
            VectorCopy(eorg, bs->goalPosition);
            return;
        }

        // Orbit aggressively inside band (keeps engagement dynamic).
        VectorCopy(eorg, bs->goalPosition);
        bs->goalPosition[0] += side[0] * orbit;
        bs->goalPosition[1] += side[1] * orbit;

        // If extremely close, pull back a touch to avoid sticking on the target.
        if (dist < retreatBand)
        {
            bs->goalPosition[0] -= toEnemy[0] * 80.0f;
            bs->goalPosition[1] -= toEnemy[1] * 80.0f;
        }
        // TAB wall avoidance: if our orbit goal is blocked, flip orbit direction and fall back to a simple pressure point.
        {
            trace_t tr;
            trap_Trace(&tr, bs->origin, g_entities[bs->client].r.mins, g_entities[bs->client].r.maxs, bs->goalPosition, bs->client, MASK_PLAYERSOLID);
            if (tr.fraction < 0.90f)
            {
                botStyleCircleDir[bs->client] = botStyleCircleDir[bs->client] ? 0 : 1;
                VectorCopy(eorg, bs->goalPosition);
                bs->goalPosition[0] += side[0] * (orbit * 0.60f);
                bs->goalPosition[1] += side[1] * (orbit * 0.60f);
            }
        }
        return;
    }

    // -----------------------------------------------------------------
    // AOTC (Battlefront trooper flavour): range discipline + reposition
    // -----------------------------------------------------------------
    if ((bs && bs->settings.botType == BOT_AOTC))
    {
        // Strong preference for mid/long engagement distances.
        desired = 620.0f * personality;

        // If we're outnumbered or weak, increase space + disengage more.
        if (odds >= 1)
            desired *= 1.12f;
        if (myHP > 0 && myHP < 40)
            desired *= 1.15f;

        retreatBand = desired * 0.92f;  // retreat earlier
        chaseBand   = desired * 1.38f;  // chase reluctantly
        orbit       = 110.0f * personality; // small lateral "lane shift" rather than circling

        if (dist < retreatBand)
        {
            // Back off hard + drift laterally (Battlefront "kite + strafe").
            float backStep = 170.0f;
            float sideStep = orbit;

            if (odds >= 1)
                backStep += 40.0f;
            if (myHP > 0 && myHP < 40)
                backStep += 40.0f;

            VectorCopy(bs->origin, bs->goalPosition);
            bs->goalPosition[0] -= toEnemy[0] * backStep;
            bs->goalPosition[1] -= toEnemy[1] * backStep;
            bs->goalPosition[0] += side[0] * sideStep;
            bs->goalPosition[1] += side[1] * sideStep;

            // AOTC wall avoidance: if the suggested retreat+lane goal is blocked, try the opposite lane,
            // then fall back to a straight retreat. This prevents AOTC bots from getting stuck on walls.
            {
                trace_t tr;
                trap_Trace(&tr, bs->origin, g_entities[bs->client].r.mins, g_entities[bs->client].r.maxs, bs->goalPosition, bs->client, MASK_PLAYERSOLID);
                if (tr.fraction < 0.90f)
                {
                    vec3_t alt;
                    VectorCopy(bs->goalPosition, alt);
                    alt[0] -= side[0] * (sideStep * 2.0f);
                    alt[1] -= side[1] * (sideStep * 2.0f);
                    trap_Trace(&tr, bs->origin, g_entities[bs->client].r.mins, g_entities[bs->client].r.maxs, alt, bs->client, MASK_PLAYERSOLID);
                    if (tr.fraction >= 0.90f)
                    {
                        VectorCopy(alt, bs->goalPosition);
                    }
                    else
                    {
                        // Still blocked: remove the lane shift, keep only the backstep.
                        bs->goalPosition[0] -= side[0] * sideStep;
                        bs->goalPosition[1] -= side[1] * sideStep;

                        // If even the straight-back retreat point is blocked (we're pinned to geometry),
                        // slide along the wall instead of continuously pushing into it.
                        {
                            trace_t tr2;
                            vec3_t backOnly;
                            VectorCopy(bs->origin, backOnly);
                            backOnly[0] -= toEnemy[0] * backStep;
                            backOnly[1] -= toEnemy[1] * backStep;
                            trap_Trace(&tr2, bs->origin, g_entities[bs->client].r.mins, g_entities[bs->client].r.maxs, backOnly, bs->client, MASK_PLAYERSOLID);
                            if (tr2.fraction < 0.40f)
                            {
                                // Try sliding along side instead.
                                VectorCopy(bs->origin, bs->goalPosition);
                                bs->goalPosition[0] += side[0] * 140.0f;
                                bs->goalPosition[1] += side[1] * 140.0f;
                                botStyleCircleDir[bs->client] = botStyleCircleDir[bs->client] ? 0 : 1;
                            }
                        }
                    }
                }
            }
            return;
        }

        if (dist > chaseBand)
        {
            // Close slightly to get back into range, but don't hard-charge.
            float fwdStep = 88.0f;
            if (odds >= 1)
                fwdStep = 64.0f;

            VectorCopy(bs->origin, bs->goalPosition);
            bs->goalPosition[0] += toEnemy[0] * fwdStep;
            bs->goalPosition[1] += toEnemy[1] * fwdStep;
            // add slight lateral so it doesn't feel like a straight line march
            bs->goalPosition[0] += side[0] * (orbit * 0.35f);
            bs->goalPosition[1] += side[1] * (orbit * 0.35f);

            // AOTC wall avoidance: if closing-in goal is blocked, drop the lateral offset first, then give up.
            {
                trace_t tr;
                trap_Trace(&tr, bs->origin, g_entities[bs->client].r.mins, g_entities[bs->client].r.maxs, bs->goalPosition, bs->client, MASK_PLAYERSOLID);
                if (tr.fraction < 0.90f)
                {
                    bs->goalPosition[0] -= side[0] * (orbit * 0.35f);
                    bs->goalPosition[1] -= side[1] * (orbit * 0.35f);
                }
            }
            return;
        }

        // In-band: small lane shift/strafe to feel like a trooper "working angles".
        if (Q_irand(0, 100) < 55)
        {
            VectorCopy(bs->origin, bs->goalPosition);
            bs->goalPosition[0] += side[0] * (orbit * 0.75f);
            bs->goalPosition[1] += side[1] * (orbit * 0.75f);

            // AOTC wall avoidance for in-band lane shifting: flip lane direction if blocked.
            {
                trace_t tr;
                trap_Trace(&tr, bs->origin, g_entities[bs->client].r.mins, g_entities[bs->client].r.maxs, bs->goalPosition, bs->client, MASK_PLAYERSOLID);
                if (tr.fraction < 0.90f)
                {
                    bs->goalPosition[0] -= side[0] * (orbit * 1.50f);
                    bs->goalPosition[1] -= side[1] * (orbit * 1.50f);
                    // also flip persistence so the next frames keep trying the free side
                    botStyleCircleDir[bs->client] = botStyleCircleDir[bs->client] ? 0 : 1;
                }
            }
            return;
        }

        return;
    }

    // -----------------------------------------------------------------
    // HYBRID: between TAB and AOTC (adaptive)
    // -----------------------------------------------------------------
    if ((bs && bs->settings.botType == BOT_HYBRID))
    {
        int wr = BotGetWeaponRange(bs);

        // If they're in saber/melee posture, behave more like TAB; otherwise more like AOTC.
        if (bs->cur_ps.weapon == WP_SABER || wr == BWEAPONRANGE_MELEE || wr == BWEAPONRANGE_SABER)
            desired = 260.0f;
        else
            desired = 470.0f;

        desired *= personality;

        // Hybrid respects odds: pushes when advantaged, repositions when not.
        if (odds >= 2)
            desired *= 1.10f;
        else if (odds <= -1)
            desired *= 0.92f;

        retreatBand = desired * 0.78f;
        chaseBand   = desired * 1.45f;
        orbit       = 150.0f * personality;

        if (dist < retreatBand)
        {
            float backStep = 120.0f;
            float sideStep = orbit;

            if (odds >= 1)
                backStep += 30.0f;

            VectorCopy(bs->origin, bs->goalPosition);
            bs->goalPosition[0] -= toEnemy[0] * backStep;
            bs->goalPosition[1] -= toEnemy[1] * backStep;
            bs->goalPosition[0] += side[0] * sideStep;
            bs->goalPosition[1] += side[1] * sideStep;

            // HYBRID wall avoidance: flip lane if blocked.
            {
                trace_t tr;
                trap_Trace(&tr, bs->origin, g_entities[bs->client].r.mins, g_entities[bs->client].r.maxs, bs->goalPosition, bs->client, MASK_PLAYERSOLID);
                if (tr.fraction < 0.90f)
                {
                    bs->goalPosition[0] -= side[0] * (sideStep * 2.0f);
                    bs->goalPosition[1] -= side[1] * (sideStep * 2.0f);
                    botStyleCircleDir[bs->client] = botStyleCircleDir[bs->client] ? 0 : 1;
                }
            }
            return;
        }

        if (dist > chaseBand)
        {
            float fwdStep = 110.0f;
            if (odds >= 1)
                fwdStep = 90.0f;

            VectorCopy(bs->origin, bs->goalPosition);
            bs->goalPosition[0] += toEnemy[0] * fwdStep;
            bs->goalPosition[1] += toEnemy[1] * fwdStep;
            return;
        }

        // In-band: occasional orbit to feel "heroic" but not constant.
        if (Q_irand(0, 100) < 35)
        {
            VectorCopy(eorg, bs->goalPosition);
            bs->goalPosition[0] += side[0] * orbit;
            bs->goalPosition[1] += side[1] * orbit;

            // If orbit goal blocked, flip direction.
            {
                trace_t tr;
                trap_Trace(&tr, bs->origin, g_entities[bs->client].r.mins, g_entities[bs->client].r.maxs, bs->goalPosition, bs->client, MASK_PLAYERSOLID);
                if (tr.fraction < 0.90f)
                {
                    botStyleCircleDir[bs->client] = botStyleCircleDir[bs->client] ? 0 : 1;
                    bs->goalPosition[0] -= side[0] * (orbit * 1.50f);
                    bs->goalPosition[1] -= side[1] * (orbit * 1.50f);
                }
            }
            return;
        }

        return;
    }

    // Any other bot types: no special handling.
}

//
// ---- Bot movement & mobility tunables ------------------------------------
//

// Cliff / drop checks
#define BOT_DROP_CHECK_DIST   72.0f   // how far ahead we probe for drops
#define BOT_MAX_SAFE_DROP             64.0f   // anything deeper is considered dangerous
#define BOT_DROP_TRACE_DEPTH  1024.0f   // how far down we trace for ground
#define BOT_FATAL_DROP        320.0f   // treat as fatal unless mobility
#define BOT_CLIFF_BRAKE_TIME         900      // ms to bias away from drop once detected
#define BOT_LEDGE_MAX_HANG_MS        2200   // ms before forcing escape from ledge hang
#define BOT_LEDGE_PROGRESS_EPS_Z        6      // minimal Z progress to count as climbing
#define BOT_LEDGE_FORCE_DROP_MS         600    // ms to hold drop input

// Cluster / spacing
#define BOT_CLUSTER_RADIUS            96.0f   // radius for "cluster" around this bot
#define BOT_CLUSTER_MIN_TEAMMATES      2      // minimum teammates nearby to count as cluster
#define BOT_WP_COMMIT_MS              2800    // ms to commit to a waypoint direction before switching
#define BOT_WP_PROGRESS_CHECK_MS       900    // ms between progress checks
#define BOT_WP_PROGRESS_EPS           64.0f   // distance improvement threshold to count as progress
#define BOT_WP_REROUTE_PENALTY_MS     1400    // ignore current dest briefly after reroute

// ---------------------------------------------------------------------------
// Navigation profiles per botType (movement/pathing only).
// Keep BOT_DEFAULT identical to stock JKA behaviour unless explicitly stated.
// ---------------------------------------------------------------------------
typedef struct bot_nav_profile_s {
    int   wpCommitMS;
    int   wpProgressCheckMS;
    float wpProgressEps;
    int   routeOptimizeIntervalMS;
    float coverPreference;     // >0 prefers cover-ish WPs (AOTC), <0 avoids them (TAB)
    float jumpAversion;        // >0 avoids WPFLAG_JUMP/forceJumpTo, <0 prefers vertical
    float backtrackPenalty;    // added when selecting the prev committed WP
} bot_nav_profile_t;

static bot_nav_profile_t BotNavProfile(const bot_state_t *bs)
{
    bot_nav_profile_t p;

    // Baseline: tuned to current behaviour (BOT_DEFAULT / stock-ish).
    p.wpCommitMS              = BOT_WP_COMMIT_MS;
    p.wpProgressCheckMS       = BOT_WP_PROGRESS_CHECK_MS;
    p.wpProgressEps           = BOT_WP_PROGRESS_EPS;
    p.routeOptimizeIntervalMS = 1500;
    p.coverPreference         = 0.0f;
    p.jumpAversion            = 0.0f;
    p.backtrackPenalty        = 256.0f;

    if (!bs)
        return p;

    switch (bs->settings.botType)
    {
    default:
    case BOT_DEFAULT:
        // Do not touch baseline.
        break;

    case BOT_TAB:      // Force Unleashed-ish: decisive, aggressive, vertical
        p.wpCommitMS              = 3600;
        p.wpProgressCheckMS       = 700;
        p.wpProgressEps           = 56.0f;
        p.routeOptimizeIntervalMS = 900;
        p.coverPreference         = -0.20f;
        p.jumpAversion            = -0.65f;
        p.backtrackPenalty        = 192.0f;
        break;

    case BOT_AOTC:     // Battlefront-ish: cover seeking, lane-ish, avoids parkour
        p.wpCommitMS              = 2400;
        p.wpProgressCheckMS       = 1100;
        p.wpProgressEps           = 72.0f;
        p.routeOptimizeIntervalMS = 1900;
        p.coverPreference         = 0.55f;
        p.jumpAversion            = 0.80f;
        p.backtrackPenalty        = 340.0f;
        break;

    case BOT_HYBRID:   // Mix: a bit more decisive + some cover bias
        p.wpCommitMS              = 3000;
        p.wpProgressCheckMS       = 900;
        p.wpProgressEps           = 64.0f;
        p.routeOptimizeIntervalMS = 1400;
        p.coverPreference         = 0.20f;
        p.jumpAversion            = 0.20f;
        p.backtrackPenalty        = 280.0f;
        break;
    }

    return p;
}

// Personal space
#define BOT_PERSONAL_RADIUS           64.0f   // if teammates are closer than this, separate
#define BOT_SEPARATION_SLOWSPEED     140.0f   // only separate when slower than this

// Door / jam handling
#define BOT_DOOR_CLUSTER_RADIUS       96.0f   // how far around a door we look for teammates
#define BOT_DOOR_YIELD_TOLERANCE       8.0f   // distance tolerance when deciding who is "closest"

// Jetpack tunables
#define BOT_JP_VERTICAL_DELTA_ENEMY   64.0f   // enemy/route height diff to want vertical help
#define BOT_JP_GAP_MIN_DROP           64.0f   // drop to consider a "gap" that needs help
#define BOT_JP_HELP_INTERVAL         50      // ms between extra jump pulses for gaps
#define BOT_JP_MIN_HEADROOM           48.0f   // minimum free vertical space to start climbs safely
#define BOT_JP_UP_TRACE              192.0f   // how far up we trace to measure headroom
#define BOT_JP_TRAVEL_MIN_LEN         360.0f   // waypoint distance to prefer jetpack travel
#define BOT_JP_TRAVEL_MIN_DZ          64.0f   // vertical delta to prefer jetpack
#define BOT_JP_RETRY_COOLDOWN         350      // ms between jetpack start attempts

// Grapple tunables
#define BOT_GRAPPLE_MAX_HOLD_MS      5000      // hard cap on how long we keep the button down
#define BOT_GRAPPLE_DECIDE_INTERVAL  200      // how often we re-check (logic tick)
#define BOT_GRAPPLE_USE_COOLDOWN     3800      // ms between actual grapple uses
#define BOT_GRAPPLE_MIN_DIST        512.0f    // min distance to enemy/path to consider grapple
#define BOT_GRAPPLE_MIN_DZ          0.0f    // min vertical difference to enemy
#define BOT_GRAPPLE_MAX_DIST       2000.0f    // beyond this, don't bother
#define BOT_GRAPPLE_TRACE_RANGE    2200.0f    // farther hook search for travel    // how far to look for a hook surface

// When distance^2 to hook <= this, we consider "distance == 0" (reached hook)
#define BOT_GRAPPLE_REACH_EPSILON_SQ   9.0f   // ~3 units

// State-sync tolerances.  These keep the beam tied to real grapple use:
// give the hook a short time to spawn/attach, but release stale/passive tethers.
#define BOT_GRAPPLE_FIRE_GRACE_MS      650      // max time to wait for a hook entity after pressing grapple
#define BOT_GRAPPLE_ATTACH_GRACE_MS    1200     // max time to show a flying/unattached hook before retrying
#define BOT_GRAPPLE_PASSIVE_GRACE_MS   1400     // max time to stay ceiling-hooked while grounded/not lifting
#define BOT_GRAPPLE_MIN_UPVEL          80.0f    // must exceed this to count as being lifted

// Minimum vertical difference for a valid hook point (prefer climbing)
#define BOT_GRAPPLE_MIN_HOOK_DZ       256.0f   // hook must be at least this much above eyes
#define BOT_GRAPPLE_GAP_HOOK_DZ       420.0f   // fatal-gap grapple must hook high enough to pull the bot upward


// Thermal grenade tunables
#define BOT_THERMAL_DECIDE_INTERVAL   50      // logic tick for throw decisions
#define BOT_THERMAL_USE_COOLDOWN     5000      // ms between actual throws
#define BOT_THERMAL_CHARGE_MS         1500      // how long we "hold" before releasing
#define BOT_THERMAL_MIN_DIST  650.0f    // min range for throwing
#define BOT_THERMAL_MAX_DIST        1200.0f    // max range for throwing
#define BOT_THERMAL_RANDOM_CHANCE     150      // 6% per think when in perfect conditions

// Backpack rocket tunables
#define BOT_BACKPACKROCKET_DECIDE_INTERVAL   120      // logic tick for rocket decisions
#define BOT_BACKPACKROCKET_USE_COOLDOWN     6500      // extra AI cooldown; w_force also enforces backpackrocketTime
#define BOT_BACKPACKROCKET_MIN_DIST         700.0f    // avoid suicide/splash at close range
#define BOT_BACKPACKROCKET_MAX_DIST        2200.0f    // matches the long-range nature of the backpack rocket
#define BOT_BACKPACKROCKET_RANDOM_CHANCE     90      // small chance per decision so it is not spammed


//for siege:
extern int rebel_attackers;
extern int imperial_attackers;
extern qboolean G_SiegeGetCompletionStatus(int team, int objective);

//[AotCAI]
extern vmCvar_t bot_cpu_usage;
//[/AotCAI]

//[SaberSys]
//RAFIXME - Part of hack to prevent bots from being stupid and doing fakes all the time
extern qboolean PM_SaberInStart( int move );
extern qboolean PM_SaberInTransition( int move );
//[/SaberSys]

boteventtracker_t gBotEventTracker[MAX_CLIENTS];

//rww - new bot cvars..
vmCvar_t bot_forcepowers;
vmCvar_t bot_forgimmick;
vmCvar_t bot_honorableduelacceptance;
vmCvar_t bot_pvstype;
vmCvar_t bot_normgpath;
#ifndef FINAL_BUILD
vmCvar_t bot_getinthecarrr;
#endif

#ifdef _DEBUG
vmCvar_t bot_nogoals;
vmCvar_t bot_debugmessages;
#endif

vmCvar_t bot_attachments;
vmCvar_t bot_camp;

vmCvar_t bot_wp_info;
vmCvar_t bot_wp_edit;
vmCvar_t bot_wp_clearweight;
vmCvar_t bot_wp_distconnect;
vmCvar_t bot_wp_visconnect;

//[BotTweaks]
vmCvar_t bot_fps;

vmCvar_t bot_wp_editornumber;
//[/BotTweaks]
//end rww

wpobject_t *flagRed;
wpobject_t *oFlagRed;
wpobject_t *flagBlue;
wpobject_t *oFlagBlue;

gentity_t *eFlagRed;
gentity_t *droppedRedFlag;
gentity_t *eFlagBlue;
gentity_t *droppedBlueFlag;

char *ctfStateNames[] = {
	"CTFSTATE_NONE",
	"CTFSTATE_ATTACKER",
	"CTFSTATE_DEFENDER",
	"CTFSTATE_RETRIEVAL",
	"CTFSTATE_GUARDCARRIER",
	"CTFSTATE_GETFLAGHOME",
	"CTFSTATE_MAXCTFSTATES"
};

char *ctfStateDescriptions[] = {
	"I'm not occupied",
	"I'm attacking the enemy's base",
	"I'm defending our base",
	"I'm getting our flag back",
	"I'm escorting our flag carrier",
	"I've got the enemy's flag"
};

char *siegeStateDescriptions[] = {
	"I'm not occupied",
	"I'm attemtping to complete the current objective",
	"I'm preventing the enemy from completing their objective"
};

char *teamplayStateDescriptions[] = {
	"I'm not occupied",
	"I'm following my squad commander",
	"I'm assisting my commanding",
	"I'm attempting to regroup and form a new squad"
};

void BotStraightTPOrderCheck(gentity_t *ent, int ordernum, bot_state_t *bs)
{
	switch (ordernum)
	{
	case 0:
		if (bs->squadLeader == ent)
		{
			bs->teamplayState = 0;
			bs->squadLeader = NULL;
		}
		break;
	case TEAMPLAYSTATE_FOLLOWING:
		bs->teamplayState = ordernum;
		bs->isSquadLeader = 0;
		bs->squadLeader = ent;
		bs->wpDestSwitchTime = 0;
		break;
	case TEAMPLAYSTATE_ASSISTING:
		bs->teamplayState = ordernum;
		bs->isSquadLeader = 0;
		bs->squadLeader = ent;
		bs->wpDestSwitchTime = 0;
		break;
	default:
		bs->teamplayState = ordernum;
		break;
	}
}

void BotSelectWeapon(int client, int weapon)
{
	if (weapon <= WP_NONE)
	{
//		assert(0);
		return;
	}
	trap_EA_SelectWeapon(client, weapon);
}

void BotReportStatus(bot_state_t *bs)
{
	if (g_gametype.integer == GT_TEAM)
	{
		trap_EA_SayTeam(bs->client, teamplayStateDescriptions[bs->teamplayState]);
	}
	else if (g_gametype.integer == GT_SIEGE)
	{
		trap_EA_SayTeam(bs->client, siegeStateDescriptions[bs->siegeState]);
	}
	//[NewGameTypes][EnhancedImpliment]
	//else if (g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTY || g_gametype.integer == GT_ITG)
	else if (g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTY)
	//[NewGameTypes]
	{
		trap_EA_SayTeam(bs->client, ctfStateDescriptions[bs->ctfState]);
	}
}

//accept a team order from a player
void BotOrder(gentity_t *ent, int clientnum, int ordernum)
{
	int stateMin = 0;
	int stateMax = 0;
	int i = 0;

	if (!ent || !ent->client || !ent->client->sess.teamLeader)
	{
		return;
	}

	if (clientnum != -1 && !botstates[clientnum])
	{
		return;
	}

	if (clientnum != -1 && !OnSameTeam(ent, &g_entities[clientnum]))
	{
		return;
	}

	//[NewGameTypes][EnhancedImpliment]	
//	if (g_gametype.integer != GT_CTF && g_gametype.integer != GT_CTY && g_gametype.integer != GT_ITG 
//			&& g_gametype.integer != GT_SIEGE && g_gametype.integer != GT_TEAM)

	if (g_gametype.integer != GT_CTF && g_gametype.integer != GT_CTY && g_gametype.integer != GT_SIEGE &&
		g_gametype.integer != GT_TEAM )
	//[/NewGameTypes][/EnhancedImpliment]
	{
		return;
	}

	//[NewGameTypes][EnhancedImpliment]
//	if (g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTY || g_gametype.integer == GT_ITG)
	if (g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTY)
	//[/NewGameTypes][EnhancedImpliment]
	{
		stateMin = CTFSTATE_NONE;
		stateMax = CTFSTATE_MAXCTFSTATES;
	}
	else if (g_gametype.integer == GT_SIEGE)
	{
		stateMin = SIEGESTATE_NONE;
		stateMax = SIEGESTATE_MAXSIEGESTATES;
	}
	else if (g_gametype.integer == GT_TEAM)
	{
		stateMin = TEAMPLAYSTATE_NONE;
		stateMax = TEAMPLAYSTATE_MAXTPSTATES;
	}

	if ((ordernum < stateMin && ordernum != -1) || ordernum >= stateMax)
	{
		return;
	}

	if (clientnum != -1)
	{
		if (ordernum == -1)
		{
			BotReportStatus(botstates[clientnum]);
		}
		else
		{
			BotStraightTPOrderCheck(ent, ordernum, botstates[clientnum]);
			botstates[clientnum]->state_Forced = ordernum;
			botstates[clientnum]->chatObject = ent;
			botstates[clientnum]->chatAltObject = NULL;
			if (BotDoChat(botstates[clientnum], "OrderAccepted", 1))
			{
				botstates[clientnum]->chatTeam = 1;
			}
		}
	}
	else
	{
		while (i < MAX_CLIENTS)
		{
			if (botstates[i] && OnSameTeam(ent, &g_entities[i]))
			{
				if (ordernum == -1)
				{
					BotReportStatus(botstates[i]);
				}
				else
				{
					BotStraightTPOrderCheck(ent, ordernum, botstates[i]);
					botstates[i]->state_Forced = ordernum;
					botstates[i]->chatObject = ent;
					botstates[i]->chatAltObject = NULL;
					if (BotDoChat(botstates[i], "OrderAccepted", 0))
					{
						botstates[i]->chatTeam = 1;
					}
				}
			}

			i++;
		}
	}
}

//See if bot is mindtricked by the client in question
int BotMindTricked(int botClient, int enemyClient)
{
	forcedata_t *fd;

	if (!g_entities[enemyClient].client)
	{
		return 0;
	}
	
	fd = &g_entities[enemyClient].client->ps.fd;

	if (!fd)
	{
		return 0;
	}

	if (botClient > 47)
	{
		if (fd->forceMindtrickTargetIndex4 & (1 << (botClient-48)))
		{
			return 1;
		}
	}
	else if (botClient > 31)
	{
		if (fd->forceMindtrickTargetIndex3 & (1 << (botClient-32)))
		{
			return 1;
		}
	}
	else if (botClient > 15)
	{
		if (fd->forceMindtrickTargetIndex2 & (1 << (botClient-16)))
		{
			return 1;
		}
	}
	else
	{
		if (fd->forceMindtrickTargetIndex & (1 << botClient))
		{
			return 1;
		}
	}

	return 0;
}
int PassLovedOneCheck(bot_state_t *bs, gentity_t *ent);

void ExitLevel( void );

void QDECL BotAI_Print(int type, char *fmt, ...) { return; }

qboolean WP_ForcePowerUsable( gentity_t *self, forcePowers_t forcePower );

int IsTeamplay(void)
{
	//[NewGameTypes][EnhancedImpliment]
	//if ( g_gametype.integer < GT_TEAM || g_gametype.integer == GT_RPG)
	if ( g_gametype.integer < GT_TEAM )
	//[/NewGameTypes][EnhancedImpliment]
	{
		return 0;
	}

	return 1;
}

/*
==================
BotAI_GetClientState
==================
*/
int BotAI_GetClientState( int clientNum, playerState_t *state ) {
	gentity_t	*ent;

	ent = &g_entities[clientNum];
	if ( !ent->inuse ) {
		return qfalse;
	}
	if ( !ent->client ) {
		return qfalse;
	}

	memcpy( state, &ent->client->ps, sizeof(playerState_t) );
	return qtrue;
}

/*
==================
BotAI_GetEntityState
==================
*/
int BotAI_GetEntityState( int entityNum, entityState_t *state ) {
	gentity_t	*ent;

	ent = &g_entities[entityNum];
	memset( state, 0, sizeof(entityState_t) );
	if (!ent->inuse) return qfalse;
	if (!ent->r.linked) return qfalse;
	if (ent->r.svFlags & SVF_NOCLIENT) return qfalse;
	memcpy( state, &ent->s, sizeof(entityState_t) );
	return qtrue;
}

/*
==================
BotAI_GetSnapshotEntity
==================
*/
int BotAI_GetSnapshotEntity( int clientNum, int sequence, entityState_t *state ) {
	int		entNum;

	entNum = trap_BotGetSnapshotEntity( clientNum, sequence );
	if ( entNum == -1 ) {
		memset(state, 0, sizeof(entityState_t));
		return -1;
	}

	BotAI_GetEntityState( entNum, state );

	return sequence + 1;
}

/*
==============
BotEntityInfo
==============
*/
void BotEntityInfo(int entnum, aas_entityinfo_t *info) {
	trap_AAS_EntityInfo(entnum, info);
}

/*
==============
NumBots
==============
*/
int NumBots(void) {
	return numbots;
}

/*
==============
AngleDifference
==============
*/
float AngleDifference(float ang1, float ang2) {
	float diff;

	diff = ang1 - ang2;
	if (ang1 > ang2) {
		if (diff > 180.0) diff -= 360.0;
	}
	else {
		if (diff < -180.0) diff += 360.0;
	}
	return diff;
}

/*
==============
BotChangeViewAngle
==============
*/
float BotChangeViewAngle(float angle, float ideal_angle, float speed) {
	float move;

	angle = AngleMod(angle);
	ideal_angle = AngleMod(ideal_angle);
	if (angle == ideal_angle) return angle;
	move = ideal_angle - angle;
	if (ideal_angle > angle) {
		if (move > 180.0) move -= 360.0;
	}
	else {
		if (move < -180.0) move += 360.0;
	}
	if (move > 0) {
		if (move > speed) move = speed;
	}
	else {
		if (move < -speed) move = -speed;
	}
	return AngleMod(angle + move);
}

/*
==============
BotChangeViewAngles
==============
*/
void BotChangeViewAngles(bot_state_t *bs, float thinktime) {
	float diff, factor, maxchange, anglespeed, disired_speed;
	int i;

	if (bs->ideal_viewangles[PITCH] > 180) bs->ideal_viewangles[PITCH] -= 360;
	
	if (bs->currentEnemy && bs->frame_Enemy_Vis)
	{
		if (bs->settings.skill <= 1)
		{
			factor = (bs->skills.turnspeed_combat*0.4f)*bs->settings.skill;
		}
		else if (bs->settings.skill <= 2)
		{
			factor = (bs->skills.turnspeed_combat*0.6f)*bs->settings.skill;
		}
		else if (bs->settings.skill <= 3)
		{
			factor = (bs->skills.turnspeed_combat*0.8f)*bs->settings.skill;
		}
		else
		{
			factor = bs->skills.turnspeed_combat*bs->settings.skill;
		}
	}
	else
	{
		factor = bs->skills.turnspeed;
	}

	if (factor > 1)
	{
		factor = 1;
	}
	if (factor < 0.001)
	{
		factor = 0.001f;
	}

	maxchange = bs->skills.maxturn;

	//if (maxchange < 240) maxchange = 240;
	maxchange *= thinktime;
	for (i = 0; i < 2; i++) {
		bs->viewangles[i] = AngleMod(bs->viewangles[i]);
		bs->ideal_viewangles[i] = AngleMod(bs->ideal_viewangles[i]);
		diff = AngleDifference(bs->viewangles[i], bs->ideal_viewangles[i]);
		disired_speed = diff * factor;
		bs->viewanglespeed[i] += (bs->viewanglespeed[i] - disired_speed);
		if (bs->viewanglespeed[i] > 180) bs->viewanglespeed[i] = maxchange;
		if (bs->viewanglespeed[i] < -180) bs->viewanglespeed[i] = -maxchange;
		anglespeed = bs->viewanglespeed[i];
		if (anglespeed > maxchange) anglespeed = maxchange;
		if (anglespeed < -maxchange) anglespeed = -maxchange;
		bs->viewangles[i] += anglespeed;
		bs->viewangles[i] = AngleMod(bs->viewangles[i]);
		bs->viewanglespeed[i] *= 0.45 * (1 - factor);
	}
	if (bs->viewangles[PITCH] > 180) bs->viewangles[PITCH] -= 360;
	trap_EA_View(bs->client, bs->viewangles);
}

/*
==============
BotInputToUserCommand
==============
*/
void BotInputToUserCommand(bot_input_t *bi, usercmd_t *ucmd, int delta_angles[3], int time, int useTime) {
	vec3_t angles, forward, right;
	short temp;
	int j;
	float f, r, u, m;
	//clear the whole structure
	memset(ucmd, 0, sizeof(usercmd_t));
	//
	//Com_Printf("dir = %f %f %f speed = %f\n", bi->dir[0], bi->dir[1], bi->dir[2], bi->speed);
	//the duration for the user command in milli seconds
	ucmd->serverTime = time;
	//
	if (bi->actionflags & ACTION_DELAYEDJUMP) {
		bi->actionflags |= ACTION_JUMP;
		bi->actionflags &= ~ACTION_DELAYEDJUMP;
	}
	//set the buttons
	if (bi->actionflags & ACTION_RESPAWN) ucmd->buttons = BUTTON_ATTACK;
	if (bi->actionflags & ACTION_ATTACK) ucmd->buttons |= BUTTON_ATTACK;
	if (bi->actionflags & ACTION_ALT_ATTACK) ucmd->buttons |= BUTTON_ALT_ATTACK;

	//[SaberSys]
	// Only send the saber-throw button while actually using a saber.
	// Dual-gun mode is no longer toggled from BUTTON_SABERTHROW, but keeping
	// this guard prevents stale bot saberthrow intent from affecting guns.
	if ((bi->actionflags & ACTION_SABERTHROW) && bi->weapon == WP_SABER)
	{
		ucmd->buttons |= BUTTON_SABERTHROW;
	}
	//[/SaberSys]
	
    if (bi->actionflags & ACTION_THERMALTHROW) ucmd->buttons |= BUTTON_THERMALTHROW;
    if (bi->actionflags & ACTION_BACKPACKROCKET) ucmd->buttons |= BUTTON_USE;
	
//	if (bi->actionflags & ACTION_TALK) ucmd->buttons |= BUTTON_TALK;
	if (bi->actionflags & ACTION_GESTURE) ucmd->buttons |= BUTTON_GESTURE;
	if (bi->actionflags & ACTION_USE) ucmd->buttons |= BUTTON_USE_HOLDABLE;
	if (bi->actionflags & ACTION_WALK) ucmd->buttons |= BUTTON_WALKING;

	if(bi->weapon == WP_SABER)
	{
		if (bi->actionflags & ACTION_FORCEPOWER) ucmd->buttons |= BUTTON_ATTACK;
	}
	else
	{
		if (bi->actionflags & ACTION_FORCEPOWER) ucmd->buttons |= BUTTON_FORCEPOWER;
	}
		

	//[TABBot]
	//RAFIXME:  This is a hack to fix the hack that Raven did
	if (bi->actionflags & ACTION_USE) ucmd->buttons |= BUTTON_USE;
	//[/TABBot]


	if (bi->actionflags & ACTION_GRAPPLE)
	{
		ucmd->buttons |= BUTTON_GRAPPLE;
	}

	if (useTime < level.time && Q_irand(1, 10) < 5)
	{ //for now just hit use randomly in case there's something useable around
		ucmd->buttons |= BUTTON_USE;
	}
#if 0
// Here's an interesting bit.  The bots in TA used buttons to do additional gestures.
// I ripped them out because I didn't want too many buttons given the fact that I was already adding some for JK2.
// We can always add some back in if we want though.
	if (bi->actionflags & ACTION_AFFIRMATIVE) ucmd->buttons |= BUTTON_AFFIRMATIVE;
	if (bi->actionflags & ACTION_NEGATIVE) ucmd->buttons |= BUTTON_NEGATIVE;
	if (bi->actionflags & ACTION_GETFLAG) ucmd->buttons |= BUTTON_GETFLAG;
	if (bi->actionflags & ACTION_GUARDBASE) ucmd->buttons |= BUTTON_GUARDBASE;
	if (bi->actionflags & ACTION_PATROL) ucmd->buttons |= BUTTON_PATROL;
	if (bi->actionflags & ACTION_FOLLOWME) ucmd->buttons |= BUTTON_FOLLOWME;
#endif //0

	if (bi->weapon == WP_NONE)
	{
#ifdef _DEBUG
//		Com_Printf("WARNING: Bot tried to use WP_NONE!\n");
#endif
		bi->weapon = WP_BRYAR_PISTOL;
	}

	//
	ucmd->weapon = bi->weapon;
	//set the view angles
	//NOTE: the ucmd->angles are the angles WITHOUT the delta angles
	ucmd->angles[PITCH] = ANGLE2SHORT(bi->viewangles[PITCH]);
	ucmd->angles[YAW] = ANGLE2SHORT(bi->viewangles[YAW]);
	ucmd->angles[ROLL] = ANGLE2SHORT(bi->viewangles[ROLL]);
	//subtract the delta angles
	for (j = 0; j < 3; j++) {
		temp = ucmd->angles[j] - delta_angles[j];
		ucmd->angles[j] = temp;
	}
	//NOTE: movement is relative to the REAL view angles
	//get the horizontal forward and right vector
	//get the pitch in the range [-180, 180]
	if (bi->dir[2]) angles[PITCH] = bi->viewangles[PITCH];
	else angles[PITCH] = 0;
	angles[YAW] = bi->viewangles[YAW];
	angles[ROLL] = 0;
	AngleVectors(angles, forward, right, NULL);
	//bot input speed is in the range [0, 400]
	bi->speed = bi->speed * 127 / 400;
	//set the view independent movement
		//set the view independent movement
	f = DotProduct(forward, bi->dir);
	r = DotProduct(right, bi->dir);
	u = fabs(forward[2]) * bi->dir[2];
	m = fabs(f);

	if (fabs(r) > m) {
		m = fabs(r);
	}

	if (fabs(u) > m) {
		m = fabs(u);
	}

	if (m > 0) {
		f *= bi->speed / m;
		r *= bi->speed / m;
		u *= bi->speed / m;
	}

	ucmd->forwardmove = f;
	ucmd->rightmove = r;
	ucmd->upmove = u;
	//normal keyboard movement
	if (bi->actionflags & ACTION_MOVEFORWARD) ucmd->forwardmove += 127;
	if (bi->actionflags & ACTION_MOVEBACK) ucmd->forwardmove -= 127;
	if (bi->actionflags & ACTION_MOVELEFT) ucmd->rightmove -= 127;
	if (bi->actionflags & ACTION_MOVERIGHT) ucmd->rightmove += 127;
	//jump/moveup
	if (bi->actionflags & ACTION_JUMP) ucmd->upmove += 127;
	//crouch/movedown
	if (bi->actionflags & ACTION_CROUCH) ucmd->upmove -= 127;

	//[TABBot]
	//Make walking work for bots.
	if (bi->actionflags & ACTION_WALK)
	{
		if (ucmd->forwardmove > 46)
		{
			ucmd->forwardmove = 46;	
		}
		else if (ucmd->forwardmove < -46)
		{
			ucmd->forwardmove = -46;
		}
		
		if (ucmd->rightmove > 46)
		{
			ucmd->rightmove = 46;
		}
		else if ( ucmd->rightmove < -46)
		{
			ucmd->rightmove = -46;
		}
	}
	//[/TABBot]

	//[BugFix20]
	//fixed problem with a bot's force power select not being treating properly by the pm code.
	ucmd->forcesel = bi->forcesel;
	//[/BugFix20]

	//
	//Com_Printf("forward = %d right = %d up = %d\n", ucmd.forwardmove, ucmd.rightmove, ucmd.upmove);
	//Com_Printf("ucmd->serverTime = %d\n", ucmd->serverTime);
}


// ---------------------------------------------------------------------------
// Cluster-level spacing: move away from the center of a blob of teammates.
// Plays nicely with normal movement and BotAvoidTeammates.
// ---------------------------------------------------------------------------
qboolean BotAvoidTeamCluster(bot_state_t* bs, bot_input_t* bi,
	const vec3_t fwd, const vec3_t right)
{
	int        clientNum;
	gentity_t* self;
	int        i;
	int        count = 0;
	vec3_t     center;
	vec3_t     horizVel;
	float      speed;

	if (!bs || !bi)
		return qfalse;

	// Avoidance must not strafe while hanging/climbing a ledge.
	if ((bs->cur_ps.pm_flags & PMF_STUCK_TO_WALL) && BG_InLedgeMove(bs->cur_ps.legsAnim))
		return qfalse;

	clientNum = bs->client;
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
		return qfalse;

	self = &g_entities[clientNum];
	if (!self || !self->client)
		return qfalse;

	// Only bother when moving slowly → more likely stuck / clumped.
	VectorCopy(bs->cur_ps.velocity, horizVel);
	horizVel[2] = 0.0f;
	speed = VectorLength(horizVel);
	if (speed > BOT_SEPARATION_SLOWSPEED)
		return qfalse;

	// Light throttling: not every frame
	if (((level.time / 120) + clientNum) & 1)
		return qfalse;

	if ((bs->cur_ps.pm_flags & PMF_STUCK_TO_WALL) &&
		BG_InLedgeMove(bs->cur_ps.legsAnim))
	{
		return qfalse;
	}


	VectorClear(center);

	for (i = 0; i < level.maxclients; i++)
	{
		gentity_t* other = &g_entities[i];
		vec3_t     d;
		float      distSq;

		if (i == clientNum)
			continue;
		if (!other->inuse || !other->client)
			continue;
		if (!OnSameTeam(self, other))
			continue;

		VectorSubtract(other->r.currentOrigin, bs->origin, d);
		d[2] = 0.0f;
		distSq = d[0] * d[0] + d[1] * d[1];

		if (distSq > (BOT_CLUSTER_RADIUS * BOT_CLUSTER_RADIUS))
			continue;

		VectorAdd(center, other->r.currentOrigin, center);
		count++;
	}

	if (count < BOT_CLUSTER_MIN_TEAMMATES)
		return qfalse; // not a real cluster

	// Compute cluster center (excluding self)
	VectorScale(center, 1.0f / (float)count, center);

	// Vector from cluster center -> us
	{
		vec3_t away;
		float  dotF, dotR;

		VectorSubtract(bs->origin, center, away);
		away[2] = 0.0f;
		if (VectorNormalize(away) < 0.1f)
			return qfalse;

		// Project "away" onto our local axes
		dotF = DotProduct(away, fwd);
		dotR = DotProduct(away, right);

		// Forward/back bias
		if (dotF < -0.2f)
		{
			// Cluster center is in front of us -> back up
			bi->actionflags |= ACTION_MOVEBACK;
			bi->actionflags &= ~ACTION_MOVEFORWARD;
		}
		else if (dotF > 0.3f)
		{
			// Cluster center is behind us -> encourage moving forward out of it
			if (!(bi->actionflags & ACTION_MOVEBACK))
				bi->actionflags |= ACTION_MOVEFORWARD;
		}

		// Sideways bias
		if (dotR > 0.2f)
		{
			// Cluster center to our right -> move left
			bi->actionflags |= ACTION_MOVELEFT;
			bi->actionflags &= ~ACTION_MOVERIGHT;
		}
		else if (dotR < -0.2f)
		{
			// Cluster center to our left -> move right
			bi->actionflags |= ACTION_MOVERIGHT;
			bi->actionflags &= ~ACTION_MOVELEFT;
		}
	}

	return qtrue;
}



// ---------------------------------------------------------------------------
// Simple personal-space separation between teammates.
// Sideways-only bias away from the closest teammate.
// ---------------------------------------------------------------------------
qboolean BotAvoidTeammates(bot_state_t* bs, bot_input_t* bi,
	const vec3_t fwd, const vec3_t right)
{
	int        clientNum;
	gentity_t* self;
	int        i;
	gentity_t* closest = NULL;
	float      closestDistSq = 999999.0f;
	vec3_t     horizVel;
	float      speed;

	if (!bs || !bi)
		return qfalse;

	// Avoidance must not strafe while hanging/climbing a ledge.
	if ((bs->cur_ps.pm_flags & PMF_STUCK_TO_WALL) && BG_InLedgeMove(bs->cur_ps.legsAnim))
		return qfalse;

	clientNum = bs->client;
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
		return qfalse;

	self = &g_entities[clientNum];
	if (!self || !self->client)
		return qfalse;

	// Only bother if we're moving relatively slowly (more likely to be in a clump)
	VectorCopy(bs->cur_ps.velocity, horizVel);
	horizVel[2] = 0.0f;
	speed = VectorLength(horizVel);
	if (speed > BOT_SEPARATION_SLOWSPEED)
		return qfalse;

	// Light throttling so not every frame per bot (reduces jitter)
	if (((level.time / 100) + clientNum) & 1)
		return qfalse;
	
	
	if ((bs->cur_ps.pm_flags & PMF_STUCK_TO_WALL) &&
		BG_InLedgeMove(bs->cur_ps.legsAnim))
	{
		return qfalse;
	}


	// Find the closest same-team bot
	for (i = 0; i < level.maxclients; i++)
	{
		gentity_t* other = &g_entities[i];
		vec3_t     d;
		float      distSq;

		if (i == clientNum)
			continue;
		if (!other->inuse || !other->client)
			continue;
		if (!OnSameTeam(self, other))
			continue;

		VectorSubtract(other->r.currentOrigin, bs->origin, d);
		d[2] = 0.0f;
		distSq = d[0] * d[0] + d[1] * d[1];

		if (distSq < closestDistSq)
		{
			closestDistSq = distSq;
			closest = other;
		}
	}

	if (!closest)
		return qfalse;

	// Too far away to care
	if (closestDistSq > (BOT_PERSONAL_RADIUS * BOT_PERSONAL_RADIUS))
		return qfalse;

	// Strafe away from the closest teammate
	{
		vec3_t away;
		float  dotR;

		VectorSubtract(bs->origin, closest->r.currentOrigin, away); // from teammate -> us
		away[2] = 0.0f;

		if (VectorNormalize(away) < 0.1f)
			return qfalse;

		dotR = DotProduct(away, right);

		// Strafe away from them; no forward/back changes
		if (dotR > 0.2f)
		{
			bi->actionflags |= ACTION_MOVERIGHT;
			bi->actionflags &= ~ACTION_MOVELEFT;
		}
		else if (dotR < -0.2f)
		{
			bi->actionflags |= ACTION_MOVELEFT;
			bi->actionflags &= ~ACTION_MOVERIGHT;
		}
	}

	return qtrue;
}


#define BOT_STUCK_SAMPLE_MS     350
#define BOT_STUCK_MINMOVE       10.0f
#define BOT_STUCK_TRIGGER_MS   1200
#define BOT_STUCK_HARD_MS      2500

static void BotDetectAndRecoverStuck(bot_state_t *bs, bot_input_t *bi, const vec3_t fwd, const vec3_t right)
{
    const int clientNum = bs->client;
    const int now = level.time;

	// Don't fight ledge-hang logic here; it will be handled later by ledge-specific code.
	if ((bs->cur_ps.pm_flags & PMF_STUCK_TO_WALL) && BG_InLedgeMove(bs->cur_ps.legsAnim))
		return;

	// Elevator / mover anti-block:
	// If we're standing on a mover (elevator) and it's not moving for a while, step off to clear it.
	if (bs->cur_ps.groundEntityNum != ENTITYNUM_NONE && bs->cur_ps.groundEntityNum < ENTITYNUM_MAX_NORMAL)
	{
		gentity_t *gnd = &g_entities[bs->cur_ps.groundEntityNum];
		if (gnd && gnd->s.eType == ET_MOVER)
		{
			vec3_t hv;
			VectorCopy(bs->cur_ps.velocity, hv);
			hv[2] = 0.0f;
			if (VectorLength(hv) < 10.0f && botStuckTime[clientNum] > 1200)
			{
				bi->actionflags &= ~(ACTION_MOVEFORWARD|ACTION_MOVELEFT|ACTION_MOVERIGHT);
				bi->actionflags |= ACTION_MOVEBACK;
				if (((now / 300) + clientNum) & 1) bi->actionflags |= ACTION_MOVELEFT;
				else bi->actionflags |= ACTION_MOVERIGHT;
				bs->wpDestIgnoreTime = now + 800;
				bs->wpSeenTime = 0;
				return;
			}
		}
	}


    // Only bother when we are trying to go somewhere OR we have movement intent in combat.
// Some maps (tight corners/door frames) can trap bots even without an active wpDestination.
{
	const int moveMask = (ACTION_MOVEFORWARD|ACTION_MOVEBACK|ACTION_MOVELEFT|ACTION_MOVERIGHT);
	qboolean wantsMove = (bi->actionflags & moveMask) ? qtrue : qfalse;

	if ((!bs->wpCurrent || !bs->wpDestination) && !wantsMove)
		return;
}

    // don't fight ledge logic / knockback / being dead
    if (bs->cur_ps.pm_type == PM_DEAD)
        return;
    if ((bs->cur_ps.pm_flags & PMF_STUCK_TO_WALL) && BG_InLedgeMove(bs->cur_ps.legsAnim))
        return;

    // sample movement every BOT_STUCK_SAMPLE_MS
    if (!botLastSampleTime[clientNum])
    {
        VectorCopy(bs->origin, botLastPos[clientNum]);
        botLastSampleTime[clientNum] = now;
        botStuckTime[clientNum] = 0;
        return;
    }

    if (now - botLastSampleTime[clientNum] < BOT_STUCK_SAMPLE_MS)
        return;

    {
        vec3_t d;
        float moved;

        VectorSubtract(bs->origin, botLastPos[clientNum], d);
        d[2] = 0.0f;
        moved = VectorLength(d);

        VectorCopy(bs->origin, botLastPos[clientNum]);
        botLastSampleTime[clientNum] = now;

        // if we aren't moving much, accumulate "stuck time"
        if (moved < BOT_STUCK_MINMOVE)
            botStuckTime[clientNum] += BOT_STUCK_SAMPLE_MS;
        else
            botStuckTime[clientNum] = 0;
    }

    if (botStuckTime[clientNum] < BOT_STUCK_TRIGGER_MS)
        return;

    // --- Recovery tier 1: small de-jam (shuffle / hop / slight turn) ---
    if (botStuckTime[clientNum] < BOT_STUCK_HARD_MS)
    {
        // strafe away from obstruction (alternate L/R by time & client)
        if (((now / 300) + clientNum) & 1)
            bi->actionflags |= ACTION_MOVELEFT;
        else
            bi->actionflags |= ACTION_MOVERIGHT;

        // short hop helps door lips / ramps
        bi->actionflags |= ACTION_JUMP;

		// If we're bumping into something interactive (doors/lifts/buttons), try USE before rerouting.
		{
			trace_t tr;
			vec3_t end;
			gentity_t *hit;

			VectorMA(bs->origin, 48.0f, fwd, end);
			trap_Trace(&tr, bs->origin,
			           g_entities[clientNum].r.mins, g_entities[clientNum].r.maxs,
			           end, clientNum, MASK_SOLID);

			if (tr.fraction < 1.0f && tr.entityNum >= 0 && tr.entityNum < ENTITYNUM_MAX_NORMAL)
			{
				hit = &g_entities[tr.entityNum];
				if (hit && hit->classname && !Q_strncmp(hit->classname, "func_", 5))
				{
					// If we are blocked by a mover (elevator/lift), prefer to USE and wait rather than hop/push.
					// This avoids bots jumping off lifts or repeatedly rerouting.
					if (!Q_strncmp(hit->classname, "func_plat", 9) || !Q_strncmp(hit->classname, "func_train", 10))
					{
						bi->actionflags |= ACTION_USE;
						bi->actionflags &= ~(ACTION_JUMP|ACTION_MOVEFORWARD|ACTION_MOVEBACK);
						bi->actionflags &= ~(ACTION_MOVELEFT|ACTION_MOVERIGHT);
						return;
					}

					bi->actionflags |= ACTION_USE;
				}
				// If another client is blocking us (corridor congestion), sidestep harder.
				else if (tr.entityNum < MAX_CLIENTS && tr.entityNum != clientNum)
				{
					bi->actionflags &= ~(ACTION_MOVEFORWARD|ACTION_MOVEBACK);
					bi->actionflags |= (((now / 200) + clientNum) & 1) ? ACTION_MOVELEFT : ACTION_MOVERIGHT;
					return;
				}
			}
		}

        // force forward intent
        bi->actionflags |= ACTION_MOVEFORWARD;
        bi->actionflags &= ~ACTION_MOVEBACK;

        // try to switch WP branch without losing destination
        TrySwitchWPBranch(bs);
        return;
    }

    // --- Recovery tier 2: hard reset route ---
    // If we don't currently have a route, do a pure movement unstick:
    // back up, strafe, and slightly turn to break out of corners/door frames.
    if (!bs->wpDestination || !bs->wpCurrent)
    {
        bi->actionflags &= ~(ACTION_MOVEFORWARD|ACTION_MOVELEFT|ACTION_MOVERIGHT);
        bi->actionflags |= ACTION_MOVEBACK;
        bi->actionflags |= (((now / 250) + clientNum) & 1) ? ACTION_MOVELEFT : ACTION_MOVERIGHT;
        bi->actionflags |= ACTION_JUMP;

        // Nudge view a bit so forward vectors change (helps when wedged).
        bs->goalPosition[0] = bs->origin[0] + right[0] * ((((now / 250) + clientNum) & 1) ? 64.0f : -64.0f);
        bs->goalPosition[1] = bs->origin[1] + right[1] * ((((now / 250) + clientNum) & 1) ? 64.0f : -64.0f);
        bs->goalPosition[2] = bs->origin[2];

        botStuckTime[clientNum] = 0;
        return;
    }

    // mark this destination "ignore for a bit" so selection can change
    bs->wpDestIgnoreTime = now + 1500;
    bs->wpSeenTime = 0;

    // if we can branch, do it; else drop current route entirely
    if (!TrySwitchWPBranch(bs))
    {
        bs->wpDestination = NULL;
        bs->wpCurrent = NULL;
    }

    botStuckTime[clientNum] = 0;
}



// ---------------------------------------------------------------------------
// Jetpack handling
// ---------------------------------------------------------------------------

// Per-bot cooldown for jetpack "extra jump help" (ms)
static int botNextJetpackJump[MAX_CLIENTS];
// Per-bot "I am flying with jetpack" timer (ms timestamp)
static int botJetpackFlyUntil[MAX_CLIENTS];
static int    botJetpackRetryUntil[MAX_CLIENTS];


// ONE function: biases JUMP/CROUCH if we have a jetpack.
// - "Flight mode" = long sustained thrust (several seconds).
// - While flying:
//      * JUMP   = climb / maintain flight.
//      * CROUCH = avoid low ceilings / descend.
// - Jetpack is used to move along the path as well (not just vertical taps).
// - Gap safety still uses short JUMP pulses.

// ---------------------------------------------------------------------------
// Mobility helpers
// ---------------------------------------------------------------------------
static qboolean BotHasJetpack(const bot_state_t *bs)
{
	return (bs && (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))) ? qtrue : qfalse;
}
static qboolean BotHasGrapple(const bot_state_t *bs)
{
	return (bs && (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_GRAPPLE))) ? qtrue : qfalse;
}
static qboolean BotHasForceJump(const bot_state_t *bs)
{
	return (bs &&
		(bs->cur_ps.fd.forcePowersKnown & (1 << FP_LEVITATION)) &&
		bs->cur_ps.fd.forcePowerLevel[FP_LEVITATION] > 0) ? qtrue : qfalse;
}

static qboolean BotJetpackHasHorizontalIntent(const bot_input_t *bi)
{
	vec3_t flatDir;

	if (!bi)
		return qfalse;

	if (bi->actionflags & (ACTION_MOVEFORWARD|ACTION_MOVEBACK|ACTION_MOVELEFT|ACTION_MOVERIGHT))
		return qtrue;

	VectorCopy(bi->dir, flatDir);
	flatDir[2] = 0.0f;
	if (VectorLength(flatDir) > 0.1f && bi->speed > 40.0f)
		return qtrue;

	return qfalse;
}

static qboolean BotJetpackRouteDir(bot_state_t *bs, vec3_t outDir)
{
	vec3_t target;

	if (!bs || !outDir)
		return qfalse;

	VectorClear(outDir);

	if (bs->wpCurrent)
	{
		VectorCopy(bs->wpCurrent->origin, target);
	}
	else if (bs->wpDestination)
	{
		VectorCopy(bs->wpDestination->origin, target);
	}
	else
	{
		VectorCopy(bs->goalPosition, target);
	}

	VectorSubtract(target, bs->origin, outDir);
	outDir[2] = 0.0f;
	if (VectorNormalize(outDir) > 0.1f)
		return qtrue;

	/* Last resort: if no waypoint/goal route is available during flight, keep
	 * moving toward a visible enemy rather than hovering in place. Normal route
	 * movement always wins above. */
	if (bs->currentEnemy && bs->currentEnemy->client && bs->currentEnemy->health > 0 && bs->frame_Enemy_Vis)
	{
		VectorSubtract(bs->currentEnemy->client->ps.origin, bs->origin, outDir);
		outDir[2] = 0.0f;
		if (VectorNormalize(outDir) > 0.1f)
			return qtrue;
	}

	return qfalse;
}

static void BotJetpackEnsureRouteMovement(bot_state_t *bs, bot_input_t *bi)
{
	vec3_t routeDir;

	if (!bs || !bi)
		return;

	/* Do not override normal route/combat movement. This only prevents the
	 * sustained jetpack window from becoming vertical-only when another system
	 * failed to provide horizontal movement this frame. */
	if (BotJetpackHasHorizontalIntent(bi))
		return;

	if (!BotJetpackRouteDir(bs, routeDir))
		return;

	VectorCopy(routeDir, bi->dir);
	bi->dir[2] = 0.0f;
	if (bi->speed < 280.0f)
		bi->speed = 400.0f;
}

static void BotEnsureJetpackOn(bot_state_t *bs, gentity_t *self)
{
	int itemIndex;

	if (!bs || !self || !self->client)
		return;

	/* Never blindly press USE while the pack is already on.  ItemUse_Jetpack()
	 * is a toggle, so repeated bot USE pulses could switch the pack off exactly
	 * when the bot is trying to start or sustain flight. */
	if (self->client->jetPackOn)
		return;

	if (self->client->ps.jetpackFuel < 5)
		return;

	itemIndex = BG_GetItemIndexByTag(HI_JETPACK, IT_HOLDABLE);
	if (itemIndex <= 0)
		return;

	self->client->ps.stats[STAT_HOLDABLE_ITEM] = itemIndex;
	trap_EA_Use(bs->client);
}

static float BotMeasureHeadroom(bot_state_t *bs, gentity_t *self)
{
	vec3_t start, end;
	trace_t trUp;
	int clientNum;

	if (!bs || !self || !self->client)
		return 0.0f;

	clientNum = bs->client;
	VectorCopy(self->client->ps.origin, start);
	start[2] += self->client->ps.viewheight;

	VectorCopy(start, end);
	end[2] += BOT_JP_UP_TRACE;

	trap_Trace(&trUp, start, self->r.mins, self->r.maxs, end, clientNum, MASK_PLAYERSOLID);

	if (trUp.fraction < 1.0f)
		return trUp.endpos[2] - start[2];

	return BOT_JP_UP_TRACE;
}

static float BotMeasureGroundDist(bot_state_t *bs, gentity_t *self)
{
	vec3_t startG, endG;
	trace_t trDown;
	int clientNum;

	if (!bs || !self || !self->client)
		return BOT_DROP_TRACE_DEPTH;

	clientNum = bs->client;
	VectorCopy(self->client->ps.origin, startG);
	VectorCopy(startG, endG);
	endG[2] -= BOT_DROP_TRACE_DEPTH;

	trap_Trace(&trDown, startG, self->r.mins, self->r.maxs, endG, clientNum, MASK_PLAYERSOLID);

	if (trDown.fraction < 1.0f)
		return startG[2] - trDown.endpos[2];

	return BOT_DROP_TRACE_DEPTH;
}


// ---------------------------------------------------------------------------
// Waypoint decisiveness helpers
// ---------------------------------------------------------------------------
static void BotCommitToWaypoint(bot_state_t *bs, int wpIndex)
{
	int clientNum, now;

	if (!bs) return;
	clientNum = bs->client;
	if (clientNum < 0 || clientNum >= MAX_CLIENTS) return;

	now = level.time;

	// If we're already committed and still within the window, don't change.
	if (botWPCommitUntil[clientNum] > now && botWPCommitIndex[clientNum] == wpIndex)
		return;

	// Remember previous commit to help avoid ping-pong switching.
	botPrevWPCommitIndex[clientNum] = botWPCommitIndex[clientNum];
	botWPCommitIndex[clientNum] = wpIndex;
	{
		const bot_nav_profile_t navp = BotNavProfile(bs);
		botWPCommitUntil[clientNum] = now + navp.wpCommitMS;
	}

	// Track wp oscillation (A<->B ping-pong) so we can force reroutes on bad spots.
	botPrevWPVisited[clientNum] = botLastWPVisited[clientNum];
	botLastWPVisited[clientNum] = wpIndex;

	if (botPrevWPVisited[clientNum] == wpIndex &&
	    (now - botLastWPSwitchTime[clientNum]) < 1500)
	{
		botWPPingPongCount[clientNum]++;
	}
	else
	{
		botWPPingPongCount[clientNum] = 0;
	}
	botLastWPSwitchTime[clientNum] = now;

	// Reset progress tracking
	botWPLastProgressTime[clientNum] = now;
	botWPLastProgressDist[clientNum] = bs->frame_Waypoint_Len;
}

static qboolean BotIsCommittedToCurrentWP(bot_state_t *bs)
{
	int clientNum;
	if (!bs) return qfalse;
	clientNum = bs->client;
	if (clientNum < 0 || clientNum >= MAX_CLIENTS) return qfalse;

	if (!bs->wpCurrent) return qfalse;
	return (botWPCommitUntil[clientNum] > level.time && botWPCommitIndex[clientNum] == bs->wpCurrent->index) ? qtrue : qfalse;
}


// Returns true if the bot is standing on (or immediately over) a mover such as an elevator/lift.
// Keep this cheap and conservative: prefer groundEntityNum, fall back to a short downward trace.
static qboolean BotOnMover(bot_state_t *bs)
{
    if (!bs) return qfalse;

    if (bs->cur_ps.groundEntityNum != ENTITYNUM_NONE &&
        bs->cur_ps.groundEntityNum < ENTITYNUM_MAX_NORMAL)
    {
        gentity_t *gnd = &g_entities[bs->cur_ps.groundEntityNum];
        if (gnd && gnd->s.eType == ET_MOVER)
            return qtrue;
    }

    {
        trace_t tr;
        vec3_t start, end;

        VectorCopy(bs->origin, start);
        VectorCopy(bs->origin, end);
        end[2] -= 24.0f;

        trap_Trace(&tr, start,
                   g_entities[bs->client].r.mins,
                   g_entities[bs->client].r.maxs,
                   end, bs->client, MASK_SOLID);

        if (tr.fraction < 1.0f &&
            tr.entityNum >= 0 &&
            tr.entityNum < ENTITYNUM_MAX_NORMAL)
        {
            gentity_t *ent = &g_entities[tr.entityNum];
            if (ent && ent->s.eType == ET_MOVER)
                return qtrue;

            if (ent && ent->classname && !Q_strncmp(ent->classname, "func_", 5))
                return qtrue;
        }
    }

    return qfalse;
}

// If we are not making progress toward the current WP for a while, allow reroute.
static qboolean BotWaypointProgressStalled(bot_state_t *bs)
{
	int clientNum, now;
	float dist;
	bot_nav_profile_t navp;

	if (!bs) return qfalse;
	clientNum = bs->client;
	if (clientNum < 0 || clientNum >= MAX_CLIENTS) return qfalse;
	navp = BotNavProfile(bs);

	now = level.time;
	if (!bs->wpCurrent) return qfalse;

	// Only check periodically
	if (now - botWPLastProgressTime[clientNum] < navp.wpProgressCheckMS)
		return qfalse;

	dist = bs->frame_Waypoint_Len;

	// If we're ping-ponging between waypoints, treat it as a stall quickly.
	if (botWPPingPongCount[clientNum] >= 3)
	{
		botWPLastProgressTime[clientNum] = now;
		botWPLastProgressDist[clientNum] = dist;
		return qtrue;
	}



    // Riding a lift/mover often looks like "no progress" in pure distance terms.
    // Do not treat this as a path stall; keep the commitment and wait it out.
    if (BotOnMover(bs))
    {
        botWPLastProgressTime[clientNum] = now;
        botWPLastProgressDist[clientNum] = dist;
        return qfalse;
    }
	// Vertical failure helper:
	// If the current waypoint is significantly above us, and we're not gaining height,
	// treat it as stalled sooner so we can branch/reroute (helps with jump/ledge failures).
	{
		const float wpZ = bs->wpCurrent->origin[2];
		const float dz = fabs(bs->origin[2] - wpZ);

		if (!BotOnMover(bs) && dz > 48.0f && bs->origin[2] < wpZ)
		{
			// If we're not moving upward at all, we likely failed the jump/ledge.
			if (fabs(bs->cur_ps.velocity[2]) < 10.0f)
			{
				botWPLastProgressTime[clientNum] = now;
				botWPLastProgressDist[clientNum] = dist;
				return qtrue;
			}
		}
	}

	// If distance hasn't improved meaningfully, consider it stalled.
	if (dist > (botWPLastProgressDist[clientNum] - navp.wpProgressEps))
	{
		botWPLastProgressTime[clientNum] = now;
		botWPLastProgressDist[clientNum] = dist;
		return qtrue;
	}

	botWPLastProgressTime[clientNum] = now;
	botWPLastProgressDist[clientNum] = dist;
	return qfalse;
}

void BotHandleJetpack(bot_state_t* bs, bot_input_t* bi, const vec3_t fwd)
{
	int        clientNum;
	gentity_t* self;
	int        now;

	if (!bs || !bi)
		return;

	clientNum = bs->client;
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
		return;

	self = &g_entities[clientNum];
	if (!self || !self->client)
		return;

	// Dead? no flying.
	if (bs->cur_ps.pm_type == PM_DEAD)
		return;

	// Must own jetpack item
	if (!(bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK)))
		return;





	now = level.time;


	// Throttle start attempts a bit so we don't flicker.
	if (botJetpackRetryUntil[clientNum] > now)
	{
		// still allow existing flight window to control vertical
	}
	// --------------------------------------------------
	// 0) Measure headroom / ground distance.
	//    Shared with cliff safety so indoor jetpack logic stays conservative.
	// --------------------------------------------------
	float headroom = BotMeasureHeadroom(bs, self);
	float groundDist = BotMeasureGroundDist(bs, self);

	// --------------------------------------------------
	// 1) Already in flight mode? Sustain flight:
	//    - default: hold JUMP
	//    - if roof close: use CROUCH to avoid ceiling / descend
	// --------------------------------------------------
// 1) Already in flight mode? Sustain flight:
//    Use ground + ceiling distance to stay in a nice corridor:
//    - If very close to ceiling  → crouch (go down).
//    - Else if very close to floor → jump (go up).
//    - Else if mid-range         → usually no vertical input (cruise),
//                                  but allow gentle climb if we’re low.
// 1) Already in flight mode? Sustain flight:
//    While in flight, we basically HOLD JUMP unless the ceiling is too close.
//    This keeps bots airborne instead of bouncing on the ground.
if (botJetpackFlyUntil[clientNum] > now)
{
    qboolean wantClimb = qfalse;
    float targetDz = 0.0f;

    // Only control vertical thrust here. Do not clear or replace
    // horizontal route/combat movement: sustained jetpack flight should
    // move along the normal bot path, not hover in place.
    bi->actionflags &= ~ACTION_JUMP;
    bi->actionflags &= ~ACTION_CROUCH;

    BotEnsureJetpackOn(bs, self);

    if (bs->wpCurrent)
    {
        targetDz = bs->wpCurrent->origin[2] - bs->origin[2];
    }
    else if (bs->wpDestination)
    {
        targetDz = bs->wpDestination->origin[2] - bs->origin[2];
    }
    else if (bs->currentEnemy && bs->currentEnemy->client && bs->currentEnemy->health > 0)
    {
        targetDz = bs->currentEnemy->r.currentOrigin[2] - bs->origin[2];
    }

    if (targetDz > BOT_JP_VERTICAL_DELTA_ENEMY)
    {
        wantClimb = qtrue;
    }

    if (headroom < 36.0f)
    {
        // Too close to the roof: stop climbing and descend slightly.
        bi->actionflags |= ACTION_CROUCH;
    }
    else if (headroom > BOT_JP_MIN_HEADROOM &&
        (groundDist < 192.0f || bs->cur_ps.velocity[2] < 40.0f || wantClimb))
    {
        /* Maintain flight, not just a single hop.  A small positive vertical
         * command keeps PM_JETPACK from settling back to the floor, but the
         * headroom gate prevents indoor ceiling grinding. */
        bi->actionflags |= ACTION_JUMP;
    }

    BotJetpackEnsureRouteMovement(bs, bi);
    return;
}





	// --------------------------------------------------
	// Not in flight mode → maybe start one, or do gap-safety hop.
	// --------------------------------------------------

	// Direction we’re moving in (horizontal only)
	vec3_t horizVel;
	float  speed;
	VectorCopy(bs->cur_ps.velocity, horizVel);
	horizVel[2] = 0.0f;
	speed = VectorLength(horizVel);

	// --------------------------------------------------
	// 2) Enemy logic: use jetpack to chase enemy along path
	//    and to reach higher ground if needed.
	// --------------------------------------------------
	if (bs->currentEnemy && bs->currentEnemy->client && bs->currentEnemy->health > 0)
	{
		float dz = bs->currentEnemy->r.currentOrigin[2] - bs->origin[2];
		float dist = Distance(bs->origin, bs->currentEnemy->r.currentOrigin);

		// Conditions to start sustained flight:
		//  - Enemy significantly above us, OR
		//  - Enemy fairly far away (use jetpack to close distance along path),
		//  and we are not under an extremely tiny ceiling.
		if (headroom > 24.0f &&
			((dz > BOT_JP_VERTICAL_DELTA_ENEMY) ||
				(dist > 800.0f && dist < 2500.0f)))
		{
			// Combat flight: long burst (do NOT use skill level).
			{
				int duration = 8500;
				if (bs->settings.botType == BOT_TAB) { duration = 10500; }
				else if (bs->settings.botType == BOT_HYBRID) { duration = 9500; }
				else if (bs->settings.botType == BOT_AOTC) { duration = 8000; }

				botJetpackFlyUntil[clientNum] = now + duration;
				// Ensure jetpack is actually on without accidentally toggling it off.
				BotEnsureJetpackOn(bs, self);
			}

			// First frame: if we have some headroom, start climbing.
			if (headroom > 32.0f)
			{
				bi->actionflags |= ACTION_JUMP;
			}
			BotJetpackEnsureRouteMovement(bs, bi);
			return;
		}
	}

	// --------------------------------------------------

	// --------------------------------------------------
	// 2b) Waypoint stall recovery: if our current waypoint is significantly above us
	//     and we are not making progress, use jetpack as an "unstick" tool.
	// --------------------------------------------------
	if (bs->wpCurrent && headroom > 24.0f && now > botJetpackRetryUntil[clientNum])
	{
		float dzWP = bs->wpCurrent->origin[2] - bs->origin[2];
		if (dzWP > 96.0f && BotWaypointProgressStalled(bs))
		{
			int duration = 5200;
			if (bs->settings.botType == BOT_TAB) { duration = 7200; }
			else if (bs->settings.botType == BOT_HYBRID) { duration = 6200; }
			else if (bs->settings.botType == BOT_AOTC) { duration = 5600; }

			botJetpackFlyUntil[clientNum] = now + duration;
			botJetpackRetryUntil[clientNum] = now + 1200;
			BotEnsureJetpackOn(bs, self);
			bi->actionflags |= ACTION_JUMP;
			BotJetpackEnsureRouteMovement(bs, bi);
			return;
		}
	}


	// --------------------------------------------------
	// 2c) Route travel: use the jetpack proactively for long or upward
	//     waypoint movement.  This preserves the normal route direction; it
	//     only adds sustained vertical thrust when there is enough headroom.
	// --------------------------------------------------
	if (headroom > BOT_JP_MIN_HEADROOM && now > botJetpackRetryUntil[clientNum] &&
		(bs->wpCurrent || bs->wpDestination))
	{
		vec3_t target, delta;
		float routeLen, routeDz;

		if (bs->wpCurrent)
			VectorCopy(bs->wpCurrent->origin, target);
		else
			VectorCopy(bs->wpDestination->origin, target);

		VectorSubtract(target, bs->origin, delta);
		routeDz = delta[2];
		delta[2] = 0.0f;
		routeLen = VectorLength(delta);

		if (routeLen > BOT_JP_TRAVEL_MIN_LEN || routeDz > BOT_JP_TRAVEL_MIN_DZ ||
			(bs->cur_ps.groundEntityNum == ENTITYNUM_NONE && bs->cur_ps.velocity[2] < -80.0f))
		{
			int duration = 6500;
			if (bs->settings.botType == BOT_TAB) { duration = 9000; }
			else if (bs->settings.botType == BOT_HYBRID) { duration = 7800; }
			else if (bs->settings.botType == BOT_AOTC) { duration = 6200; }

			botJetpackFlyUntil[clientNum] = now + duration;
			botJetpackRetryUntil[clientNum] = now + BOT_JP_RETRY_COOLDOWN;
			BotEnsureJetpackOn(bs, self);

			if (routeDz > 24.0f || groundDist < 192.0f || bs->cur_ps.velocity[2] < 40.0f)
				bi->actionflags |= ACTION_JUMP;

			BotJetpackEnsureRouteMovement(bs, bi);
			return;
		}
	}


	// 3) Big drop / gap ahead while moving → short hop
	//     (safety logic; not a full flight mode)
	// --------------------------------------------------
	if (speed > 80.0f && DotProduct(horizVel, fwd) > 0.0f)
	{
		vec3_t  checkOrigin, start, endDown;
		trace_t trDrop;
		float   dropHeight;

		VectorMA(bs->origin, BOT_DROP_CHECK_DIST, fwd, checkOrigin);
		VectorCopy(checkOrigin, start);
		start[2] += 16.0f;
		VectorCopy(start, endDown);
		endDown[2] -= BOT_DROP_TRACE_DEPTH;

		trap_Trace(&trDrop, start, self->r.mins, self->r.maxs,
			endDown, clientNum, MASK_PLAYERSOLID);

		if (trDrop.fraction == 1.0f)
		{
			dropHeight = BOT_JP_GAP_MIN_DROP + 1.0f; // no ground
		}
		else
		{
			dropHeight = start[2] - trDrop.endpos[2];
		}

		if (dropHeight > BOT_JP_GAP_MIN_DROP)
		{
			// Single pulse: hop over, let pmove + jetpack physics handle it.
			if (bs->cur_ps.groundEntityNum == ENTITYNUM_NONE || botNextJetpackJump[clientNum] <= now)
			{
				bi->actionflags |= ACTION_JUMP;
				botNextJetpackJump[clientNum] = now + BOT_JP_HELP_INTERVAL;
			}
			return;
		}
	}

	// --------------------------------------------------
	// 4) Idle / general locomotion: sometimes fly along path
	//    when there is decent headroom.
	// --------------------------------------------------
	if (!bs->currentEnemy)
	{
		// Frequent travel/idle flight while moving, as long as we have headroom.
		if (headroom > BOT_JP_MIN_HEADROOM &&
			(speed > 60.0f || bs->wpCurrent || bs->wpDestination) &&
			Q_irand(0, 1000) < 180) // ~18% chance per think
		{
			// Idle / travel flight: ~2–4 seconds.
			int duration = 12000 + Q_irand(0, 12000);

			botJetpackFlyUntil[clientNum] = now + duration;
			botJetpackRetryUntil[clientNum] = now + BOT_JP_RETRY_COOLDOWN;
			BotEnsureJetpackOn(bs, self);

			// Start by climbing a bit if we can.
			if (headroom > BOT_JP_MIN_HEADROOM)
			{
				bi->actionflags |= ACTION_JUMP;
			}
			BotJetpackEnsureRouteMovement(bs, bi);
			return;
		}
	}

	// Otherwise, let normal movement/AI decide about jump/crouch.
}





// ---------------------------------------------------------------------------
// Grapple handling
// ---------------------------------------------------------------------------

// Per-bot cooldown for grapple decisions (ms)
static int botNextGrappleDecision[MAX_CLIENTS];
// Per-bot grapple "start time" for current hook
static int botGrappleStartTime[MAX_CLIENTS];
// Per-bot cooldown between actual grapple uses
static int botGrappleUseCooldownUntil[MAX_CLIENTS];
static int    botGrappleRetryUntil[MAX_CLIENTS];

// Grapple state
static qboolean botGrappleActive[MAX_CLIENTS];   // currently being pulled to a hook
static vec3_t   botGrapplePoint[MAX_CLIENTS];    // stored hook point
static int      botGrapplePassiveSince[MAX_CLIENTS]; // grounded/tight hook without real lift


// ONE function: decides when to press (and briefly hold) the grapple button.
// Behaviour:
//  - When we start a grapple, we record the hook point and start time.
//  - While we are far from that point and under the time cap, we keep holding BUTTON_GRAPPLE.
//  - Once we are close enough (distance^2 <= BOT_GRAPPLE_REACH_EPSILON_SQ) OR we exceed
//    BOT_GRAPPLE_MAX_HOLD_MS, we stop pressing and clear active state.

// ---------------------------------------------------------------------------
// BotSelectGrappleTarget
// Picks a grapple point by sampling several "high" aim pitches and choosing a
// useful hook (favor height + distance). Enforces >=60 degrees upward.
// Returns qtrue if a valid hook is found, and fills outPoint.
// ---------------------------------------------------------------------------
static qboolean BotSelectGrappleTarget(bot_state_t *bs, gentity_t *self, const bot_input_t *bi, float minDist, float minDz, vec3_t outPoint)
{
	int clientNum;
	vec3_t startHook;
	vec3_t routeDir;
	vec3_t baseAng;
	qboolean hasRouteDir = qfalse;
	int i;

	if (!bs || !self || !self->client)
		return qfalse;

	clientNum = bs->client;

	VectorCopy(self->client->ps.origin, startHook);
	startHook[2] += self->client->ps.viewheight;

	/* Use the bot's route as the horizontal direction so grappling still advances
	 * through the map.  The older rescue code was aiming almost straight up, which
	 * frequently attached to nearby ceilings and left bots stuck under them. */
	hasRouteDir = BotJetpackRouteDir(bs, routeDir);
	if (hasRouteDir)
	{
		vectoangles(routeDir, baseAng);
	}
	else
	{
		VectorCopy(bs->viewangles, baseAng);
	}

	{
		/* Moderate upward pitches.  Keep -60 as the steepest normal sample, but do
		 * not use -70/-80/-86 unless the selector is explicitly asked for a fatal
		 * fall rescue, and even then reject near-vertical hits below. */
		const float pitches[] = { -22.0f, -32.0f, -42.0f, -52.0f, -60.0f };
		float bestScore = -1.0f;
		float minHoriz = minDist * 0.55f;
		float maxRiseRun = (minDz >= BOT_GRAPPLE_GAP_HOOK_DZ) ? 1.95f : 1.35f;
		vec3_t best;

		if (minHoriz < 220.0f)
			minHoriz = 220.0f;
		if (minDz >= BOT_GRAPPLE_GAP_HOOK_DZ && minHoriz < 180.0f)
			minHoriz = 180.0f;

		VectorClear(best);

		for (i = 0; i < (int)(sizeof(pitches)/sizeof(pitches[0])); i++)
		{
			vec3_t ang, fwd, right, up, dir, endHook;
			trace_t tr;

			VectorCopy(baseAng, ang);
			ang[PITCH] = pitches[i];
			if (ang[PITCH] < -64.0f) ang[PITCH] = -64.0f;

			AngleVectors(ang, fwd, right, up);
			VectorCopy(fwd, dir);

			/* Only a light lift bias: enough to find high walls/forward ceilings, but
			 * not enough to turn every trace into a straight-up ceiling hook. */
			dir[2] += (minDz >= BOT_GRAPPLE_GAP_HOOK_DZ) ? 0.28f : 0.18f;

			if (VectorNormalize(dir) == 0.0f)
				continue;

			VectorMA(startHook, BOT_GRAPPLE_TRACE_RANGE, dir, endHook);
			trap_Trace(&tr, startHook, NULL, NULL, endHook, clientNum, MASK_SOLID);

			if (tr.fraction < 1.0f)
			{
				vec3_t toHit;
				float distSq;
				float horiz;
				float dz;
				float riseRun;

				VectorSubtract(tr.endpos, startHook, toHit);
				distSq = toHit[0]*toHit[0] + toHit[1]*toHit[1] + toHit[2]*toHit[2];
				horiz = sqrtf(toHit[0]*toHit[0] + toHit[1]*toHit[1]);
				dz = toHit[2];

				if (distSq < (minDist*minDist) || dz < minDz || horiz < minHoriz)
					continue;

				riseRun = dz / (horiz > 1.0f ? horiz : 1.0f);
				if (riseRun > maxRiseRun)
					continue;

				/* Avoid shooting into a very nearby low ceiling.  A ceiling hit is still
				 * valid when it is far enough forward to pull the bot through the route. */
				if (tr.plane.normal[2] < -0.55f && horiz < (minHoriz + 128.0f))
					continue;

				{
					float dist = sqrtf(distSq);
					float steepPenalty = riseRun * riseRun * 160.0f;
					float wallBonus = (tr.plane.normal[2] > -0.35f) ? 140.0f : 0.0f;
					float score = (0.60f * horiz) + (0.45f * dz) + (0.20f * dist) + wallBonus - steepPenalty;

					if (score > bestScore)
					{
						bestScore = score;
						VectorCopy(tr.endpos, best);
					}
				}
			}
		}

		if (bestScore > 0.0f)
		{
			VectorCopy(best, outPoint);
			return qtrue;
		}
	}

	return qfalse;
}


static qboolean BotSelectGrappleTargetTowardPoint(bot_state_t *bs, gentity_t *self, const vec3_t point, float minDist, float minDz, vec3_t outPoint)
{
	int clientNum;
	vec3_t startHook;
	vec3_t toPoint;
	vec3_t baseAng;
	int y, p;
	const float yawOffsets[] = { 0.0f, -32.0f, 32.0f, -64.0f, 64.0f, -96.0f, 96.0f };
	const float pitches[] = { -18.0f, -28.0f, -38.0f, -48.0f, -58.0f };
	float bestScore = -1.0f;
	float minHoriz = minDist * 0.55f;
	float maxRiseRun = (minDz >= BOT_GRAPPLE_GAP_HOOK_DZ) ? 1.95f : 1.35f;
	vec3_t best;

	if (!bs || !self || !self->client)
		return qfalse;

	clientNum = bs->client;

	VectorCopy(self->client->ps.origin, startHook);
	startHook[2] += self->client->ps.viewheight;
	VectorSubtract(point, startHook, toPoint);
	toPoint[2] = 0.0f;
	if (VectorNormalize(toPoint) == 0.0f)
		return qfalse;
	vectoangles(toPoint, baseAng);

	if (minHoriz < 220.0f)
		minHoriz = 220.0f;
	if (minDz >= BOT_GRAPPLE_GAP_HOOK_DZ && minHoriz < 180.0f)
		minHoriz = 180.0f;

	VectorClear(best);

	/* Enemy grapples must not assume the enemy is in the route/front direction.
	 * Scan a controlled horizontal fan around the enemy direction, but keep the
	 * same anti-vertical filters so bots do not hook straight into ceilings. */
	for (y = 0; y < (int)(sizeof(yawOffsets)/sizeof(yawOffsets[0])); y++)
	{
		for (p = 0; p < (int)(sizeof(pitches)/sizeof(pitches[0])); p++)
		{
			vec3_t ang, fwd, right, up, dir, endHook;
			trace_t tr;

			VectorCopy(baseAng, ang);
			ang[YAW] += yawOffsets[y];
			ang[PITCH] = pitches[p];
			if (ang[PITCH] < -64.0f) ang[PITCH] = -64.0f;

			AngleVectors(ang, fwd, right, up);
			VectorCopy(fwd, dir);
			dir[2] += (minDz >= BOT_GRAPPLE_GAP_HOOK_DZ) ? 0.25f : 0.15f;

			if (VectorNormalize(dir) == 0.0f)
				continue;

			VectorMA(startHook, BOT_GRAPPLE_TRACE_RANGE, dir, endHook);
			trap_Trace(&tr, startHook, NULL, NULL, endHook, clientNum, MASK_SOLID);

			if (tr.fraction < 1.0f)
			{
				vec3_t toHit;
				float distSq;
				float horiz;
				float dz;
				float riseRun;

				VectorSubtract(tr.endpos, startHook, toHit);
				distSq = toHit[0]*toHit[0] + toHit[1]*toHit[1] + toHit[2]*toHit[2];
				horiz = sqrtf(toHit[0]*toHit[0] + toHit[1]*toHit[1]);
				dz = toHit[2];

				if (distSq < (minDist*minDist) || dz < minDz || horiz < minHoriz)
					continue;

				riseRun = dz / (horiz > 1.0f ? horiz : 1.0f);
				if (riseRun > maxRiseRun)
					continue;

				if (tr.plane.normal[2] < -0.55f && horiz < (minHoriz + 128.0f))
					continue;

				{
					float dist = sqrtf(distSq);
					float yawPenalty = fabs(yawOffsets[y]) * 2.0f;
					float steepPenalty = riseRun * riseRun * 160.0f;
					float wallBonus = (tr.plane.normal[2] > -0.35f) ? 140.0f : 0.0f;
					float score = (0.60f * horiz) + (0.45f * dz) + (0.20f * dist) + wallBonus - steepPenalty - yawPenalty;

					if (score > bestScore)
					{
						bestScore = score;
						VectorCopy(tr.endpos, best);
					}
				}
			}
		}
	}

	if (bestScore > 0.0f)
	{
		VectorCopy(best, outPoint);
		return qtrue;
	}

	return qfalse;
}


static void BotAimAtPoint(bot_state_t *bs, bot_input_t *bi, gentity_t *self, const vec3_t point)
{
	vec3_t start, dir, ang;

	if (!bs || !bi || !self || !self->client)
		return;

	VectorCopy(self->client->ps.origin, start);
	start[2] += self->client->ps.viewheight;
	VectorSubtract(point, start, dir);
	if (VectorNormalize(dir) == 0.0f)
		return;

	vectoangles(dir, ang);
	if (ang[PITCH] > 180.0f)
		ang[PITCH] -= 360.0f;
	ang[ROLL] = 0.0f;

	VectorCopy(ang, bs->ideal_viewangles);
	VectorCopy(ang, bs->viewangles);
	VectorCopy(ang, bi->viewangles);
}

int PassStandardEnemyChecks(bot_state_t *bs, gentity_t *en);

static qboolean BotGrappleTraceShootTarget(gentity_t *self, gentity_t *enemy, vec3_t target)
{
	vec3_t start;
	trace_t tr;

	if (!self || !self->client || !enemy || !enemy->inuse || !enemy->client || enemy->health <= 0)
		return qfalse;

	VectorCopy(self->client->ps.origin, start);
	start[2] += self->client->ps.viewheight;

	VectorCopy(enemy->client->ps.origin, target);
	target[2] += enemy->client->ps.viewheight;

	trap_Trace(&tr, start, NULL, NULL, target, self->s.number, MASK_SHOT);
	if (tr.fraction < 1.0f && tr.entityNum != enemy->s.number)
		return qfalse;

	return qtrue;
}

static gentity_t *BotFindGrappleCombatTarget(bot_state_t *bs, gentity_t *self, vec3_t target)
{
	gentity_t *best = NULL;
	float bestScore = -999999.0f;
	int i;

	if (!bs || !self || !self->client)
		return NULL;

	/* While grappled the bot may be facing the hook, so bs->frame_Enemy_Vis/FOV
	 * can be false even when a valid enemy is plainly visible beside or behind it.
	 * Keep the current enemy if it is shootable from any direction, otherwise do a
	 * small 360-degree LOS scan and let combat aim turn to that target. */
	if (bs->currentEnemy &&
		PassStandardEnemyChecks(bs, bs->currentEnemy) &&
		BotGrappleTraceShootTarget(self, bs->currentEnemy, target))
	{
		return bs->currentEnemy;
	}

	for (i = 0; i < level.num_entities; i++)
	{
		gentity_t *ent;
		vec3_t tmpTarget;
		float distSq;
		float score;

		if (i == self->s.number)
			continue;

		ent = &g_entities[i];
		if (!ent->inuse || !ent->client || ent->health <= 0)
			continue;

		if (!PassStandardEnemyChecks(bs, ent))
			continue;

		/* PassLovedOneCheck assumes player/client style targets.  Use it for real
		 * clients, but keep NPC/vehicle targets available to the 360 scan. */
		if (i < MAX_CLIENTS && !PassLovedOneCheck(bs, ent))
			continue;

		if (!BotGrappleTraceShootTarget(self, ent, tmpTarget))
			continue;

		distSq = DistanceSquared(self->client->ps.origin, ent->client->ps.origin);
		score = 1000000.0f - distSq;
		if (ent->client->ps.eFlags & (EF_FIRING | EF_ALT_FIRING))
			score += 200000.0f;
		if (ent->client->ps.powerups[PW_REDFLAG] || ent->client->ps.powerups[PW_BLUEFLAG])
			score += 250000.0f;
		if (bs->lastHurt && bs->lastHurt->s.number == i)
			score += 150000.0f;
		if (bs->revengeEnemy && bs->revengeEnemy->s.number == i)
			score += 125000.0f;

		if (score > bestScore)
		{
			bestScore = score;
			best = ent;
			VectorCopy(tmpTarget, target);
		}
	}

	return best;
}

static qboolean BotGrappleAimAndFightEnemy(bot_state_t *bs, bot_input_t *bi, gentity_t *self)
{
	gentity_t *enemy;
	vec3_t start, target, dir, ang;

	if (!bs || !bi || !self || !self->client)
		return qfalse;

	enemy = BotFindGrappleCombatTarget(bs, self, target);
	if (!enemy)
		return qfalse;

	VectorCopy(self->client->ps.origin, start);
	start[2] += self->client->ps.viewheight;

	VectorSubtract(target, start, dir);
	if (VectorNormalize(dir) == 0.0f)
		return qfalse;

	vectoangles(dir, ang);
	if (ang[PITCH] > 180.0f)
		ang[PITCH] -= 360.0f;
	ang[ROLL] = 0.0f;

	VectorCopy(ang, bs->ideal_viewangles);
	VectorCopy(ang, bs->viewangles);
	VectorCopy(ang, bi->viewangles);

	if (bs->cur_ps.weapon != WP_NONE && bs->cur_ps.weapon != WP_SABER && bs->cur_ps.weapon != WP_MELEE)
	{
		bi->actionflags |= ACTION_ATTACK;
	}

	return qtrue;
}

static void BotStopGrappleAI(int clientNum)
{
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
		return;

	botGrappleActive[clientNum] = qfalse;
	botGrapplePassiveSince[clientNum] = 0;
}

static void BotCommitToActiveGrapple(bot_state_t *bs, bot_input_t *bi, gentity_t *self, const vec3_t toHook)
{
	if (!bs || !bi || !self || !self->client)
		return;

	/* A visible grapple should not leave normal route-walking active underneath it,
	 * but do not turn every attached hook into constant vertical jumping.  Move
	 * into the pull and jump only when the hook is clearly forward-and-above. */
	bi->actionflags &= ~(ACTION_MOVEBACK | ACTION_MOVELEFT | ACTION_MOVERIGHT | ACTION_WALK);
	bi->actionflags |= ACTION_GRAPPLE | ACTION_MOVEFORWARD;

	{
		float horiz = sqrtf(toHook[0]*toHook[0] + toHook[1]*toHook[1]);
		if (self->client->ps.groundEntityNum != ENTITYNUM_NONE && toHook[2] > 96.0f && horiz > 160.0f)
		{
			bi->actionflags |= ACTION_JUMP;
		}
	}
}

static void BotStartGrapple(bot_state_t *bs, gentity_t *self, bot_input_t *bi,
	const vec3_t hook, int decisionDelay, int useCooldown, int retryDelay)
{
	int clientNum;

	if (!bs || !self || !self->client || !bi)
		return;

	clientNum = bs->client;
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
		return;

	VectorCopy(hook, botGrapplePoint[clientNum]);
	botGrappleActive[clientNum] = qtrue;
	botGrapplePassiveSince[clientNum] = 0;
	botGrappleStartTime[clientNum] = level.time;
	botNextGrappleDecision[clientNum] = level.time + decisionDelay;
	botGrappleUseCooldownUntil[clientNum] = level.time + useCooldown;
	if (retryDelay > 0)
		botGrappleRetryUntil[clientNum] = level.time + retryDelay;

	/* The server fires the hook along ps.viewangles.  Picked trace targets are
	 * useless unless the bot is forced to look at the chosen wall/ceiling point
	 * on the same command that presses BUTTON_GRAPPLE. */
	BotAimAtPoint(bs, bi, self, hook);
	bi->actionflags |= ACTION_GRAPPLE;
}


void BotHandleGrapple(bot_state_t* bs, bot_input_t* bi, const vec3_t fwd)
{
	int        clientNum;
	gentity_t* self;
	int        now;
	qboolean   wantGrapple = qfalse;
	qboolean   haveHookPoint = qfalse;
	trace_t    trHook;  // last hook trace we want to use

	if (!bs || !bi)
		return;

	clientNum = bs->client;
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
		return;

	self = &g_entities[clientNum];
	if (!self || !self->client)
		return;

	if (bs->cur_ps.pm_type == PM_DEAD)
		return;

	// Must own grapple item
	if (!(bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_GRAPPLE)))
	{
		BotStopGrappleAI(clientNum);
		return;
	}

	now = level.time;

	const qboolean airborne = (bs->cur_ps.groundEntityNum == ENTITYNUM_NONE) ? qtrue : qfalse;

	// -------------------------------------------------
	// Expanded grapple usage cases:
	//  - Long waypoint travel (prefer far/high hooks).
	//  - Enemy far above (contest high ground).
	//  - Over void/high drop (avoid falling by grabbing a hook).
	// -------------------------------------------------
	if (!botGrappleActive[clientNum] && BotHasGrapple(bs) && now > botGrappleUseCooldownUntil[clientNum] && now > botGrappleRetryUntil[clientNum])
	{
		vec3_t hook;


		// Waypoint stall recovery: if our goal waypoint is above us and we're not progressing,
		// try to grapple up even if overall path isn't long.
		if (bs->wpCurrent && BotWaypointProgressStalled(bs))
		{
			float dzWP = bs->wpCurrent->origin[2] - bs->origin[2];
			if (dzWP > 96.0f && bs->frame_Waypoint_Len < 900.0f)
			{
				if (BotSelectGrappleTarget(bs, self, bi, 420.0f, 220.0f, hook))
				{
					BotStartGrapple(bs, self, bi, hook, BOT_GRAPPLE_DECIDE_INTERVAL, BOT_GRAPPLE_USE_COOLDOWN, 250);
					return;
				}
			}
		}

		if (bs->frame_Waypoint_Len > 360.0f)
		{
			if (BotSelectGrappleTarget(bs, self, bi, 420.0f, BOT_GRAPPLE_MIN_HOOK_DZ, hook))
			{
				BotStartGrapple(bs, self, bi, hook, BOT_GRAPPLE_DECIDE_INTERVAL, BOT_GRAPPLE_USE_COOLDOWN, 0);
				return;
			}
		}

		if (bs->currentEnemy && bs->currentEnemy->client && bs->currentEnemy->health > 0)
		{
			float dzGoal = bs->currentEnemy->r.currentOrigin[2] - bs->origin[2];
			if (dzGoal > 220.0f)
			{
				/* Enemy can be anywhere around the bot.  For enemy-driven grapple
				 * starts, search around the enemy direction first; fall back to the
				 * route-aware selector if no useful side wall/ceiling is found. */
				if (BotSelectGrappleTargetTowardPoint(bs, self, bs->currentEnemy->r.currentOrigin, 420.0f, 220.0f, hook) ||
					BotSelectGrappleTarget(bs, self, bi, 420.0f, 220.0f, hook))
				{
					BotStartGrapple(bs, self, bi, hook, BOT_GRAPPLE_DECIDE_INTERVAL, BOT_GRAPPLE_USE_COOLDOWN, 0);
					return;
				}
			}
		}

		if (bs->cur_ps.groundEntityNum == ENTITYNUM_NONE && bs->cur_ps.velocity[2] < -180.0f)
		{
			if (BotSelectGrappleTarget(bs, self, bi, 420.0f, 220.0f, hook))
			{
				BotStartGrapple(bs, self, bi, hook, 100, 1200, 250);
				return;
			}
		}
	}


	// Do not start a new grapple too frequently (cooldown between uses).
	if (!airborne && !botGrappleActive[clientNum] && botGrappleUseCooldownUntil[clientNum] > now)
	{
		return;
	}

	// ---------------------------------------------------------
	// 0) Already mid-grapple? Hold until we reach the hook OR
	//    we've been holding long enough, then release.
	// ---------------------------------------------------------
	if (botGrappleActive[clientNum])
	{
		int   elapsed = now - botGrappleStartTime[clientNum];
		vec3_t toHook;
		float  distSq;


		// If we're very high above ground, try to rehook instead of releasing into a lethal fall.
		{
			trace_t trDown;
			vec3_t startG, endG;
			float groundDist = BOT_DROP_TRACE_DEPTH;

			VectorCopy(self->client->ps.origin, startG);
			VectorCopy(startG, endG);
			endG[2] -= BOT_DROP_TRACE_DEPTH;
			trap_Trace(&trDown, startG, self->r.mins, self->r.maxs, endG, clientNum, MASK_PLAYERSOLID);
			if (trDown.fraction < 1.0f)
				groundDist = startG[2] - trDown.endpos[2];

			if (groundDist > (BOT_FATAL_DROP + 160.0f) && (airborne || now > botGrappleUseCooldownUntil[clientNum]))
			{
				vec3_t hook;

				/* While crossing a lethal gap, do not rehook to a flat/low point.
				 * Use the bot route for horizontal direction, but require a high hook
				 * so the pull has enough upward component to prevent a fatal fall. */
				if (BotSelectGrappleTarget(bs, self, bi, 420.0f, BOT_GRAPPLE_GAP_HOOK_DZ, hook))
				{
					BotStartGrapple(bs, self, bi, hook, 50, 900, 250);
					return;
				}
			}
		}

		// Hard safety cap: never hold longer than BOT_GRAPPLE_MAX_HOLD_MS
		if (elapsed > BOT_GRAPPLE_MAX_HOLD_MS)
		{
			BotStopGrappleAI(clientNum);
			return; // stop pressing; g_active will free the hook
		}

		VectorSubtract(botGrapplePoint[clientNum], bs->origin, toHook);
		distSq = toHook[0] * toHook[0] + toHook[1] * toHook[1] + toHook[2] * toHook[2];

		/* Keep the AI grapple state synced to the real server hook.  A bot may
		 * decide to grapple while weaponTime/debounce prevents the shot, or the
		 * hook missile may miss/never attach.  In both cases, do not leave a
		 * visible/passive tether around for the full hold time. */
		if (!self->client->hook)
		{
			if (elapsed > BOT_GRAPPLE_FIRE_GRACE_MS)
			{
				BotStopGrappleAI(clientNum);
				return;
			}

			BotAimAtPoint(bs, bi, self, botGrapplePoint[clientNum]);
			bi->actionflags |= ACTION_GRAPPLE;
			return;
		}

		if (!(self->client->ps.pm_flags & PMF_GRAPPLE_PULL))
		{
			if (elapsed > BOT_GRAPPLE_ATTACH_GRACE_MS)
			{
				BotStopGrappleAI(clientNum);
				return;
			}

			BotAimAtPoint(bs, bi, self, botGrapplePoint[clientNum]);
			bi->actionflags |= ACTION_GRAPPLE;
			return;
		}

		/* Real hook is attached.  Keep the grapple button and pull movement committed,
		 * but do not force the bot to stare at the hook if it has a visible enemy.
		 * Once attached, the server pull does not require viewangles to remain on the
		 * hook point, so combat aim/fire can work like before. */
		BotCommitToActiveGrapple(bs, bi, self, toHook);
		if (!BotGrappleAimAndFightEnemy(bs, bi, self))
		{
			BotAimAtPoint(bs, bi, self, botGrapplePoint[clientNum]);
		}

		/* Allow the hook to remain for a short window, but if a high/ceiling hook
		 * is not lifting the bot at all, release it and let the normal cooldown
		 * advance the route instead of dragging a useless visible line around. */
		if (toHook[2] > 96.0f && self->client->ps.groundEntityNum != ENTITYNUM_NONE &&
			self->client->ps.velocity[2] < BOT_GRAPPLE_MIN_UPVEL)
		{
			if (!botGrapplePassiveSince[clientNum])
			{
				botGrapplePassiveSince[clientNum] = now;
			}
			else if (now - botGrapplePassiveSince[clientNum] > BOT_GRAPPLE_PASSIVE_GRACE_MS)
			{
				BotStopGrappleAI(clientNum);
				return;
			}
		}
		else
		{
			botGrapplePassiveSince[clientNum] = 0;
		}

		// Movement says "reached hook when distance == 0".
		// We treat anything <= eps^2 as 0.
		if (distSq > BOT_GRAPPLE_REACH_EPSILON_SQ)
		{
			return;
		}

		// Reached the hook (distance ~ 0) -> stop forcing grapple immediately
		BotStopGrappleAI(clientNum);
		return;
	}

	// Only decide to start a new grapple every so often
	if (botNextGrappleDecision[clientNum] > now)
		return;

	// -------------------------------------------------
	// Fall rescue grapple:
	// If we're airborne and falling fast, try to grab a high surface ahead/up to save ourselves.
	// (Works well for elevator shafts / holes, and gives "Spider-Man" recovery.)
	// -------------------------------------------------
	if (BotHasGrapple(bs) &&
		bs->cur_ps.groundEntityNum == ENTITYNUM_NONE &&
		bs->cur_ps.velocity[2] < -280.0f &&
		now > botGrappleRetryUntil[clientNum] &&
		now > botGrappleUseCooldownUntil[clientNum])
	{
		vec3_t hook;

		/* Emergency falling rescue: follow the route horizontally if possible,
		 * but require a high hook so the grapple actually lifts the bot instead
		 * of pulling it flat across a fatal gap. */
		if (BotSelectGrappleTarget(bs, self, bi, 420.0f, BOT_GRAPPLE_GAP_HOOK_DZ, hook))
		{
			BotStartGrapple(bs, self, bi, hook, 100, 1200, 250);
			return;
		}

		botGrappleRetryUntil[clientNum] = now + 250;
	}


	// ---------------------------------------
	// A) Dangerous drop / shaft ahead
	// ---------------------------------------
	{
		vec3_t  checkOrigin, start, endDown;
		trace_t trDrop;
		float   dropHeight;

		VectorMA(bs->origin, BOT_DROP_CHECK_DIST, fwd, checkOrigin);
		VectorCopy(checkOrigin, start);
		start[2] += 16.0f;
		VectorCopy(start, endDown);
		endDown[2] -= BOT_DROP_TRACE_DEPTH;

		trap_Trace(&trDrop, start, self->r.mins, self->r.maxs, endDown, clientNum, MASK_PLAYERSOLID);

		if (trDrop.fraction == 1.0f)
		{
			dropHeight = BOT_MAX_SAFE_DROP + 999.0f; // open shaft
		}
		else
		{
			dropHeight = start[2] - trDrop.endpos[2];
		}

if (dropHeight > BOT_MAX_SAFE_DROP + 16.0f)
{
    vec3_t hook;
    float requiredDz = (dropHeight > BOT_FATAL_DROP) ? BOT_GRAPPLE_GAP_HOOK_DZ : BOT_GRAPPLE_MIN_HOOK_DZ;

    /* When the route crosses a deep/fatal gap, a low grapple point is a trap:
     * it pulls mostly sideways and the bot still drops.  Reuse the route-aware
     * selector, but require a much higher hook for fatal gaps. */
    if (BotSelectGrappleTarget(bs, self, bi, 360.0f, requiredDz, hook))
    {
        VectorCopy(hook, trHook.endpos);
        wantGrapple   = qtrue;
        haveHookPoint = qtrue;
    }
}


	}

	// ---------------------------------------
	// B) High & far enemy: climb/swing to them
	// ---------------------------------------
	if (!wantGrapple &&
		bs->currentEnemy && bs->currentEnemy->client && bs->currentEnemy->health > 0)
	{
		float dist = Distance(bs->origin, bs->currentEnemy->r.currentOrigin);
		float dz   = bs->currentEnemy->r.currentOrigin[2] - bs->origin[2];

		if (dist >= BOT_GRAPPLE_MIN_DIST && dist <= BOT_GRAPPLE_MAX_DIST && dz >= BOT_GRAPPLE_MIN_DZ)
		{
			vec3_t hook;
			if (BotSelectGrappleTargetTowardPoint(bs, self, bs->currentEnemy->r.currentOrigin, 420.0f, BOT_GRAPPLE_MIN_HOOK_DZ, hook) ||
				BotSelectGrappleTarget(bs, self, bi, 420.0f, BOT_GRAPPLE_MIN_HOOK_DZ, hook))
			{
				VectorCopy(hook, trHook.endpos);
				wantGrapple   = qtrue;
				haveHookPoint = qtrue;
			}
		}
	}

	// ---------------------------------------
	// C) General climbing / movement in open space
	// ---------------------------------------
	if (!wantGrapple)
	{
		vec3_t horizVel;
		float  speed;

		VectorCopy(bs->cur_ps.velocity, horizVel);
		horizVel[2] = 0.0f;
		speed = VectorLength(horizVel);

		if (speed > 120.0f && Q_irand(0, 1000) < 25) // opportunistic travel grapples
		{
			vec3_t hook;
			if (BotSelectGrappleTarget(bs, self, bi, 420.0f, BOT_GRAPPLE_MIN_HOOK_DZ, hook))
			{
				VectorCopy(hook, trHook.endpos);
				wantGrapple   = qtrue;
				haveHookPoint = qtrue;
			}
		}
	}


	if (!wantGrapple || !haveHookPoint)
		return;

	// -------------------------------------------------
	// Start a new grapple:
	//  - record hook point & start time
	//  - mark active
	//  - set decision + use cooldown
	//  - press grapple this frame
	// -------------------------------------------------
	BotStartGrapple(bs, self, bi, trHook.endpos, BOT_GRAPPLE_DECIDE_INTERVAL, BOT_GRAPPLE_USE_COOLDOWN, 0);
}


// Thermal handling
static int botThermalChargeUntil[MAX_CLIENTS];        // while > now, we keep holding the throw button
static int botThermalUseCooldownUntil[MAX_CLIENTS];   // global cooldown between throws
static int botNextThermalDecision[MAX_CLIENTS];       // throttling the decision logic
static int botBackpackRocketUseCooldownUntil[MAX_CLIENTS];
static int botNextBackpackRocketDecision[MAX_CLIENTS];
// ---------------------------------------------------------------------------
// Thermal grenade handling
// ---------------------------------------------------------------------------
//
// Behaviour:
//  - We "charge" throws by holding ACTION_THERMALTHROW for BOT_THERMAL_CHARGE_MS.
//  - We only start a new charge if:
//       * we have an enemy in a good distance window,
//       * we can see them,
//       * our cooldown has expired,
//       * and a small random chance hits (so they don't spam).
//  - Once charging has started, we *always* hold the button until the charge
//    duration is over, then release and set cooldown.
//

// ---------------------------------------------------------------------------
// BotHandleCliffSafety
// Prevent bots from stepping into fatal drops/shafts. Uses a small "fan" of
// traces to catch cases where only a corner is over the void.
// Run this late in BotUpdateInput so it overrides other injected movement.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// BotApplyMoveCommit
// Reduce forward/back/side thrashing by committing to a chosen move for a
// short window when not in combat and not in special states.
// ---------------------------------------------------------------------------
static void BotApplyMoveCommit(bot_state_t *bs, bot_input_t *bi)
{
	int clientNum, now;
	int mvFlags;

	if (!bs || !bi)
		return;

	clientNum = bs->client;
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
		return;

	now = level.time;

	// Don't interfere with combat or ledge/wall states.
	if (bs->currentEnemy)
		return;
	if (bs->cur_ps.pm_flags & PMF_STUCK_TO_WALL)
		return;

	mvFlags = bi->actionflags & (ACTION_MOVEFORWARD | ACTION_MOVEBACK | ACTION_MOVELEFT | ACTION_MOVERIGHT);

	if (botMoveCommitUntil[clientNum] > now && botMoveCommitFlags[clientNum])
	{
		bi->actionflags &= ~(ACTION_MOVEFORWARD | ACTION_MOVEBACK | ACTION_MOVELEFT | ACTION_MOVERIGHT);
		bi->actionflags |= botMoveCommitFlags[clientNum];
		return;
	}

	if (mvFlags)
	{
		botMoveCommitFlags[clientNum] = mvFlags;
		botMoveCommitUntil[clientNum] = now + 220;
	}
	else
	{
		botMoveCommitFlags[clientNum] = 0;
	}
}

// ---------------------------------------------------------------------------
// BotUnderLiftTrace
// More reliable "under mover" detection using an upward trace for ET_MOVER.
// ---------------------------------------------------------------------------
static qboolean BotUnderLiftTrace(bot_state_t *bs)
{
	int clientNum;
	gentity_t *self;
	trace_t tr;
	vec3_t start, end;

	if (!bs)
		return qfalse;

	clientNum = bs->client;
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
		return qfalse;

	self = &g_entities[clientNum];
	if (!self || !self->client)
		return qfalse;

	VectorCopy(bs->origin, start);
	start[2] += 8.0f;
	VectorCopy(start, end);
	end[2] += 160.0f;

	trap_Trace(&tr, start, self->r.mins, self->r.maxs, end, clientNum, MASK_PLAYERSOLID);

	if (tr.fraction < 1.0f && tr.entityNum >= 0 && tr.entityNum < ENTITYNUM_MAX_NORMAL)
	{
		gentity_t *hit = &g_entities[tr.entityNum];
		if (hit && hit->s.eType == ET_MOVER)
		{
			// If our head-space is close to the mover's underside, assume we're under it.
			if (start[2] < hit->r.absmin[2] + 32.0f)
				return qtrue;
		}
	}

	return qfalse;
}

static int BotMoveFlagsAwayFromDir(const vec3_t avoidDir, const vec3_t fwd, const vec3_t right)
{
	float fwdDot, rightDot;
	int flags = 0;

	fwdDot = DotProduct(avoidDir, fwd);
	rightDot = DotProduct(avoidDir, right);

	if (fwdDot > 0.25f)
		flags |= ACTION_MOVEFORWARD;
	else if (fwdDot < -0.25f)
		flags |= ACTION_MOVEBACK;

	if (rightDot > 0.25f)
		flags |= ACTION_MOVERIGHT;
	else if (rightDot < -0.25f)
		flags |= ACTION_MOVELEFT;

	if (!flags)
	{
		if (fwdDot >= 0.0f)
			flags |= ACTION_MOVEFORWARD;
		else
			flags |= ACTION_MOVEBACK;
	}

	return flags;
}

static void BotHandleCliffSafety(bot_state_t *bs, bot_input_t *bi, const vec3_t fwd, const vec3_t right)
{
	gentity_t *self;
	int clientNum, now;
	vec3_t dirs[5];
	vec3_t mv;
	int i;

	if (!bs || !bi)
		return;

	clientNum = bs->client;
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
		return;

	self = &g_entities[clientNum];
	if (!self || !self->client)
		return;

	now = level.time;


	// Don't fight ledge-hang pullups; let ledge logic drive MOVEFORWARD.
	if (bs->cur_ps.pm_flags & PMF_STUCK_TO_WALL)
		return;

	// Do not exempt jetpack/grapple/Force Jump users from cliff checks.
	// They still need the guard because route/combat steering can push them
	// into voids before their mobility logic has actually recovered them.

	// If braking window active, keep backing away.
	if (botCliffBrakeUntil[clientNum] > now)
	{
		int brakeFlags = botCliffBrakeFlags[clientNum];

		bi->actionflags &= ~(ACTION_MOVEFORWARD | ACTION_MOVEBACK | ACTION_MOVELEFT | ACTION_MOVERIGHT);
		if (!BotHasJetpack(bs) && !BotHasGrapple(bs) && !BotHasForceJump(bs))
			bi->actionflags &= ~ACTION_JUMP;

		if (BotHasJetpack(bs))
		{
			float headroom = BotMeasureHeadroom(bs, self);
			if (headroom > BOT_JP_MIN_HEADROOM)
			{
				botJetpackFlyUntil[clientNum] = now + 2500;
				BotEnsureJetpackOn(bs, self);
				bi->actionflags |= ACTION_JUMP;
			}
		}

		if (!brakeFlags)
			brakeFlags = ACTION_MOVEBACK;

		bi->actionflags |= brakeFlags;
		return;
	}
	if (botCliffBrakeUntil[clientNum] <= now)
		botCliffBrakeFlags[clientNum] = 0;

	// Only care if we're trying to move.
	if (!(bi->actionflags & (ACTION_MOVEFORWARD|ACTION_MOVEBACK|ACTION_MOVELEFT|ACTION_MOVERIGHT)))
		return;

	// Build intended horizontal move vector
	VectorClear(mv);
	if (bi->actionflags & ACTION_MOVEFORWARD) VectorAdd(mv, fwd, mv);
	if (bi->actionflags & ACTION_MOVEBACK)   VectorSubtract(mv, fwd, mv);
	if (bi->actionflags & ACTION_MOVERIGHT)  VectorAdd(mv, right, mv);
	if (bi->actionflags & ACTION_MOVELEFT)   VectorSubtract(mv, right, mv);
	mv[2] = 0.0f;
	if (VectorNormalize(mv) == 0.0f)
		return;

	// Fan directions: center, slight left, slight right
	VectorCopy(mv, dirs[0]);
	VectorMA(mv, 0.45f, right, dirs[1]);
	VectorMA(mv, -0.45f, right, dirs[2]);
	// Diagonals help catch thin lips around shafts/cliffs
	VectorMA(dirs[1], 0.35f, fwd, dirs[3]);
	VectorMA(dirs[2], 0.35f, fwd, dirs[4]);
for (i = 0; i < 5; i++)
	{
		vec3_t checkOrigin, start, endDown;
		trace_t trDrop;
		float dropHeight;

		dirs[i][2] = 0.0f;
		VectorNormalize(dirs[i]);

		VectorMA(bs->origin, BOT_DROP_CHECK_DIST, dirs[i], checkOrigin);
		VectorCopy(checkOrigin, start);
		start[2] += 16.0f;
		VectorCopy(start, endDown);
		endDown[2] -= BOT_DROP_TRACE_DEPTH;

		trap_Trace(&trDrop, start, self->r.mins, self->r.maxs, endDown, clientNum, MASK_PLAYERSOLID);

		if (trDrop.fraction == 1.0f)
			dropHeight = BOT_DROP_TRACE_DEPTH;
		else
			dropHeight = start[2] - trDrop.endpos[2];

		if (dropHeight >= BOT_FATAL_DROP)
		{
			vec3_t avoidDir;
			float headroom;

			/* Mobility is no longer exempt.  First try an actual recovery input
			 * that preserves route movement; if that is not available/safe, brake
			 * and back away just like an ordinary bot. */
			headroom = BotMeasureHeadroom(bs, self);
			if (BotHasJetpack(bs) && headroom > BOT_JP_MIN_HEADROOM)
			{
				botJetpackFlyUntil[clientNum] = now + 3500;
				botJetpackRetryUntil[clientNum] = now + BOT_JP_RETRY_COOLDOWN;
				BotEnsureJetpackOn(bs, self);
				bi->actionflags |= ACTION_JUMP;
				BotJetpackEnsureRouteMovement(bs, bi);
				return;
			}

			if (BotHasGrapple(bs) && !(bi->actionflags & ACTION_GRAPPLE))
			{
				vec3_t hook;
				if (BotSelectGrappleTarget(bs, self, bi, 360.0f, BOT_GRAPPLE_GAP_HOOK_DZ, hook))
				{
					BotStartGrapple(bs, self, bi, hook, 100, 1200, 250);
					return;
				}
			}

			VectorScale(dirs[i], -1.0f, avoidDir);
			botCliffBrakeUntil[clientNum] = now + BOT_CLIFF_BRAKE_TIME;
			botCliffBrakeFlags[clientNum] = BotMoveFlagsAwayFromDir(avoidDir, fwd, right);

			bi->actionflags &= ~(ACTION_MOVEFORWARD|ACTION_MOVEBACK|ACTION_MOVELEFT|ACTION_MOVERIGHT);
			if (!BotHasJetpack(bs) && !BotHasGrapple(bs) && !BotHasForceJump(bs))
				bi->actionflags &= ~ACTION_JUMP;
			bi->actionflags |= botCliffBrakeFlags[clientNum];

			// Nudge WP logic to re-evaluate
			bs->wpDestIgnoreTime = now + 1200;
			bs->wpSeenTime = 0;
			return;
		}
	}
}



static qboolean BotDirectMoveWouldFatalDrop(bot_state_t *bs, const vec3_t dir)
{
	gentity_t *self;
	int clientNum;
	vec3_t moveDir, side, start, endDown, checkOrigin;
	trace_t trDrop;
	float dropHeight;
	int i;
	float sideOffsets[3] = { 0.0f, 16.0f, -16.0f };

	if (!bs)
		return qfalse;

	/* Final safety veto: do not exempt jetpack/grapple/Force Jump users here.
	 * Direct EA movement helpers cannot guarantee that recovery actually starts,
	 * so any direct move into a fatal drop is rejected for every bot. */

	/* Do not fight ledge/wall pull-up movement. */
	if (bs->cur_ps.pm_flags & PMF_STUCK_TO_WALL)
		return qfalse;

	clientNum = bs->client;
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
		return qfalse;

	self = &g_entities[clientNum];
	if (!self || !self->client)
		return qfalse;

	VectorCopy(dir, moveDir);
	moveDir[2] = 0.0f;
	if (VectorNormalize(moveDir) == 0.0f)
		return qfalse;

	/* If the bot is already airborne, do not let direct movement keep pushing
	 * it farther into a void. */
	if (bs->cur_ps.groundEntityNum == ENTITYNUM_NONE && bs->cur_ps.velocity[2] < -80.0f)
		return qtrue;

	VectorSet(side, -moveDir[1], moveDir[0], 0.0f);
	VectorNormalize(side);

	for (i = 0; i < 3; i++)
	{
		VectorMA(bs->origin, BOT_DROP_CHECK_DIST, moveDir, checkOrigin);
		VectorMA(checkOrigin, sideOffsets[i], side, checkOrigin);

		VectorCopy(checkOrigin, start);
		start[2] += 16.0f;
		VectorCopy(start, endDown);
		endDown[2] -= BOT_DROP_TRACE_DEPTH;

		trap_Trace(&trDrop, start, self->r.mins, self->r.maxs, endDown, clientNum, MASK_PLAYERSOLID);

		if (trDrop.fraction == 1.0f)
			dropHeight = BOT_DROP_TRACE_DEPTH;
		else
			dropHeight = start[2] - trDrop.endpos[2];

		if (dropHeight >= BOT_FATAL_DROP)
			return qtrue;
	}

	return qfalse;
}

static qboolean BotDirectSafeMove(bot_state_t *bs, const vec3_t dir, float speed)
{
	vec3_t moveDir;

	if (!bs)
		return qfalse;
	if (BotDirectMoveWouldFatalDrop(bs, dir))
		return qfalse;

	// trap_EA_Move takes a non-const vec3_t; copy the read-only direction
	// locally to avoid MSVC C4090 without changing behavior.
	VectorCopy(dir, moveDir);
	trap_EA_Move(bs->client, moveDir, speed);
	return qtrue;
}

static qboolean BotDirectSafeButtonMove(bot_state_t *bs, int moveFlag)
{
	vec3_t fwd, right, dir;
	vec3_t angles;

	if (!bs)
		return qfalse;

	VectorCopy(bs->viewangles, angles);
	angles[PITCH] = 0.0f;
	angles[ROLL] = 0.0f;
	AngleVectors(angles, fwd, right, NULL);
	VectorClear(dir);

	switch (moveFlag)
	{
	case ACTION_MOVEFORWARD:
		VectorCopy(fwd, dir);
		break;
	case ACTION_MOVEBACK:
		VectorScale(fwd, -1.0f, dir);
		break;
	case ACTION_MOVERIGHT:
		VectorCopy(right, dir);
		break;
	case ACTION_MOVELEFT:
		VectorScale(right, -1.0f, dir);
		break;
	default:
		return qfalse;
	}

	if (BotDirectMoveWouldFatalDrop(bs, dir))
		return qfalse;

	switch (moveFlag)
	{
	case ACTION_MOVEFORWARD:
		trap_EA_MoveForward(bs->client);
		break;
	case ACTION_MOVEBACK:
		trap_EA_MoveBack(bs->client);
		break;
	case ACTION_MOVERIGHT:
		trap_EA_MoveRight(bs->client);
		break;
	case ACTION_MOVELEFT:
		trap_EA_MoveLeft(bs->client);
		break;
	}

	return qtrue;
}

static void BotHandleLedgeHold(bot_state_t *bs, bot_input_t *bi)
{
	gentity_t *self;
	int clientNum, now;

	if (!bs || !bi)
		return;

	clientNum = bs->client;
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
		return;

	self = &g_entities[clientNum];
	if (!self || !self->client)
		return;

	now = level.time;

	// If we recently decided to drop, keep holding the drop inputs briefly.
	if (botLedgeForceDropUntil[clientNum] > now)
	{
		bi->actionflags &= ~(ACTION_MOVEFORWARD | ACTION_MOVELEFT | ACTION_MOVERIGHT);
		bi->actionflags |= ACTION_MOVEBACK;
		bi->actionflags |= ACTION_CROUCH; // crouch/back tends to release ledge hangs
		return;
	}

	if (bs->cur_ps.pm_flags & PMF_STUCK_TO_WALL)
	{
		qboolean inLedgeMove = BG_InLedgeMove(bs->cur_ps.legsAnim);

		// Track hang start / vertical progress while stuck-to-wall (ledge or wall cling)
		if (botLedgeHangStart[clientNum] <= 0 || botLedgeHangStart[clientNum] > now)
		{
			botLedgeHangStart[clientNum] = now;
			botLedgeLastZ[clientNum] = (int)bs->origin[2];
		}
		else
		{
			int zNow = (int)bs->origin[2];
			if (zNow > botLedgeLastZ[clientNum] + BOT_LEDGE_PROGRESS_EPS_Z)
			{
				// Making upward progress; refresh timer
				botLedgeHangStart[clientNum] = now;
				botLedgeLastZ[clientNum] = zNow;
			}
		}

		// Avoid sideways dithering while stuck to walls; it looks stupid and often causes loops.
		bi->actionflags &= ~(ACTION_MOVELEFT | ACTION_MOVERIGHT);
		bi->actionflags &= ~ACTION_CROUCH;

			// Default: try to climb/advance rather than oscillate.
		bi->actionflags |= ACTION_MOVEFORWARD;
		bi->actionflags &= ~ACTION_MOVEBACK;

		// If we've been stuck to a wall for too long without making progress, escape:
		// Prefer jetpack/grapple recovery; otherwise drop and reroute.
		{
			const int maxHang = inLedgeMove ? BOT_LEDGE_MAX_HANG_MS : 1200;
			if (now - botLedgeHangStart[clientNum] > maxHang)
			{
				// Try jetpack to pop up
				if (BotHasJetpack(bs))
				{
					bi->actionflags |= ACTION_JUMP;
					botLedgeHangStart[clientNum] = now; // give it another window
					return;
				}

				// Try grapple to save/transition
				if (BotHasGrapple(bs))
				{
					bi->actionflags |= ACTION_GRAPPLE;
					botLedgeHangStart[clientNum] = now;
					return;
				}

				// Otherwise drop off the wall/ledge so we don't hang forever, and force reroute.
				botLedgeForceDropUntil[clientNum] = now + BOT_LEDGE_FORCE_DROP_MS;
				bs->wpDestIgnoreTime = now + 1200;
				bs->wpSeenTime = 0;
				return;
			}
		}

		return;
	}

	// Not ledge-hanging: clear timers
	botLedgeHangStart[clientNum] = 0;
	botLedgeLastZ[clientNum] = 0;
}

void BotHandleThermal(bot_state_t* bs, bot_input_t* bi)
{
    int        clientNum;
    gentity_t* self;
    int        now;

    if (!bs || !bi)
        return;

    clientNum = bs->client;
    if (clientNum < 0 || clientNum >= MAX_CLIENTS)
        return;

    self = &g_entities[clientNum];
    if (!self || !self->client)
        return;

    now = level.time;

    // ----------------------------------------------------
    // 0) Already charging a throw? Keep holding the button
    //    until the charge time is over.
    // ----------------------------------------------------
    if (botThermalChargeUntil[clientNum] > now)
    {
        // While charging, continuously re-check for suicide conditions.
        // If the enemy closes in, or our throw arc would likely bounce, cancel the charge.
        if (!bs->currentEnemy || !bs->currentEnemy->client || bs->currentEnemy->health <= 0)
        {
            botThermalChargeUntil[clientNum] = 0;
            return;
        }

        {
            vec3_t toEnemy;
            float  dist;

            VectorSubtract(bs->currentEnemy->r.currentOrigin, bs->origin, toEnemy);
            dist = VectorLength(toEnemy);

            // Enemy rushed us while charging: cancel to avoid self-detonation.
            if (dist < 520.0f)
            {
                botThermalChargeUntil[clientNum] = 0;
                return;
            }

            // If there is solid geometry near the throw direction, cancel to avoid bounce-back.
            {
                vec3_t startW, endW, fwdW;
                trace_t trW;

                VectorCopy(bs->origin, startW);
                startW[2] += bs->cur_ps.viewheight;

                AngleVectors(bs->viewangles, fwdW, NULL, NULL);
                VectorMA(startW, 96.0f, fwdW, endW);
                trap_Trace(&trW, startW, NULL, NULL, endW, clientNum, MASK_SOLID);

                if (trW.fraction < 1.0f)
                {
                    botThermalChargeUntil[clientNum] = 0;
                    return;
                }
            }
        }

        // Keep the "throw" button pressed
        bi->actionflags |= ACTION_THERMALTHROW;
        return;
    }

    // If we were charging but the time is now over, we simply
    // stop setting ACTION_THERMALTHROW, and the weapon code
    // will interpret that as "release & throw".
    // No explicit extra code needed here.

    // ----------------------------------------------------
    // 1) Check basic preconditions to consider starting a new throw
    // ----------------------------------------------------
    if (!bs->currentEnemy || !bs->currentEnemy->client || bs->currentEnemy->health <= 0)
        return;

    // Must have thermals and ammo
    if (!(bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_THERMAL)))
        return;
    if (bs->cur_ps.ammo[AMMO_THERMAL] <= 0)
        return;

    // Global cooldown between throws
    if (botThermalUseCooldownUntil[clientNum] > now)
        return;

    // Throttle decision logic
    if (botNextThermalDecision[clientNum] > now)
        return;

    botNextThermalDecision[clientNum] = now + BOT_THERMAL_DECIDE_INTERVAL;

    // ----------------------------------------------------
    // 2) Range / visibility checks
    // ----------------------------------------------------
    {
        vec3_t toEnemy;
        float  dist;

        VectorSubtract(bs->currentEnemy->r.currentOrigin, bs->origin, toEnemy);
        dist = VectorLength(toEnemy);

		// Prevent thermal suicide: never snap-throw at close range, or into nearby geometry.
		if (dist < BOT_THERMAL_MIN_DIST)
			return;

		// If there is solid geometry directly in front of us, a snap throw can bounce back.
		{
			vec3_t startW, endW, fwdW;
			trace_t trW;

			VectorCopy(bs->origin, startW);
			startW[2] += bs->cur_ps.viewheight;

			AngleVectors(bs->viewangles, fwdW, NULL, NULL);
			VectorMA(startW, 96.0f, fwdW, endW);
			trap_Trace(&trW, startW, NULL, NULL, endW, clientNum, MASK_SOLID);

			if (trW.fraction < 1.0f)
				return;
		}

		// If we are sprinting into danger at mid-close range, don't throw.
		if (dist < 800.0f && VectorLengthSquared(bs->cur_ps.velocity) > (220.0f * 220.0f))
			return;


        if (dist < BOT_THERMAL_MIN_DIST || dist > BOT_THERMAL_MAX_DIST)
            return;

        // Use your existing visibility helper
        if (!bs->frame_Enemy_Len && !bs->currentEnemy)
            return;

        // Small random chance so they don't carpet-bomb (more aggressive when closer)
        {
            int bonus = 0;
            if (dist < 600.0f) bonus = 80; // +8% when closer
            if (Q_irand(0, 1000) >= (BOT_THERMAL_RANDOM_CHANCE + bonus))
                return;
        }
    }

    // ----------------------------------------------------
    // 3) Start charging a throw
    // ----------------------------------------------------
    botThermalChargeUntil[clientNum]      = now + BOT_THERMAL_CHARGE_MS;
    botThermalUseCooldownUntil[clientNum] = now + BOT_THERMAL_USE_COOLDOWN;

    // First frame of charge: press the throw button
    bi->actionflags |= ACTION_THERMALTHROW;
}









void BotHandleBackpackRocket(bot_state_t* bs, bot_input_t* bi)
{
    int        clientNum;
    gentity_t* self;
    int        now;

    if (!bs || !bi)
        return;

    clientNum = bs->client;
    if (clientNum < 0 || clientNum >= MAX_CLIENTS)
        return;

    self = &g_entities[clientNum];
    if (!self || !self->client)
        return;

    now = level.time;

    if (self->client->skillLevel[SK_BACKPACKROCKET] < FORCE_LEVEL_1)
        return;

    if (self->client->backpackrocketTime > now)
        return;

    if (!bs->currentEnemy || !bs->currentEnemy->client || bs->currentEnemy->health <= 0)
        return;

    if (botBackpackRocketUseCooldownUntil[clientNum] > now)
        return;

    if (botNextBackpackRocketDecision[clientNum] > now)
        return;

    botNextBackpackRocketDecision[clientNum] = now + BOT_BACKPACKROCKET_DECIDE_INTERVAL;

    {
        vec3_t toEnemy;
        float  dist;

        VectorSubtract(bs->currentEnemy->r.currentOrigin, bs->origin, toEnemy);
        dist = VectorLength(toEnemy);

        if (dist < BOT_BACKPACKROCKET_MIN_DIST || dist > BOT_BACKPACKROCKET_MAX_DIST)
            return;

        // Do not launch a backpack rocket into a wall right in front of us.
        {
            vec3_t startW, endW, fwdW;
            trace_t trW;

            VectorCopy(bs->origin, startW);
            startW[2] += bs->cur_ps.viewheight;

            AngleVectors(bs->viewangles, fwdW, NULL, NULL);
            VectorMA(startW, 96.0f, fwdW, endW);
            trap_Trace(&trW, startW, NULL, NULL, endW, clientNum, MASK_SOLID);

            if (trW.fraction < 1.0f)
                return;
        }

        if (!bs->frame_Enemy_Len && !bs->currentEnemy)
            return;

        {
            int bonus = 0;
            if (dist > 1200.0f) bonus = 35;
            if (self->client->skillLevel[SK_BACKPACKROCKET] >= FORCE_LEVEL_2) bonus += 25;
            if (Q_irand(0, 1000) >= (BOT_BACKPACKROCKET_RANDOM_CHANCE + bonus))
                return;
        }
    }

    botBackpackRocketUseCooldownUntil[clientNum] = now + BOT_BACKPACKROCKET_USE_COOLDOWN;

    // Backpack rocket is player-triggered by crouch + normal use.
    // Use a dedicated bot action so we do not accidentally press BUTTON_USE_HOLDABLE.
    bi->actionflags |= ACTION_BACKPACKROCKET;
    bi->actionflags |= ACTION_CROUCH;
}

/*
==============
BotUpdateInput
==============
*/
//[AotCAI]
//[VoiceEvents][EnhancedImpliment]
//int		talk_time[MAX_CLIENTS];	//debouncer for taunting enemies
//[/VoiceEvents][EnhancedImpliment]
int		walktime[MAX_CLIENTS];	//timer for walking
qboolean visible (gentity_t *self, gentity_t *other)
{ // From TAB/OJP AOTC bot code: line-of-sight check for bot combat decisions.
    vec3_t spot1, spot2;
    trace_t tr;
    gentity_t *traceEnt;

    if (!self || !other)
        return qfalse;

    if (!other->client || !self->client)
        return qfalse;

    VectorCopy(self->r.currentOrigin, spot1);
    spot1[2] += 48;

    VectorCopy(other->r.currentOrigin, spot2);
    spot2[2] += 48;

    trap_Trace(&tr, spot1, NULL, NULL, other->r.currentOrigin, self->s.number, MASK_SHOT);

    traceEnt = &g_entities[tr.entityNum];
    if (traceEnt == other)
        return qtrue;

    return qfalse;
}

//[/AotCAI]

// -----------------------------------------------------------------
// Bot type helpers
// -----------------------------------------------------------------
static qboolean BotStyle_IsBurstWeapon(int weap)
{
    switch (weap)
    {
        case WP_BRYAR_PISTOL:
        case WP_BRYAR_OLD:
        case WP_BLASTER:
        case WP_DISRUPTOR:
        case WP_BOWCASTER:
        case WP_REPEATER:
        case WP_DEMP2:
        case WP_FLECHETTE:
        case WP_CONCUSSION:
            return qtrue;
        default:
            break;
    }
    return qfalse;
}

void BotUpdateInput(bot_state_t *bs, int time, int elapsed_time) {
	bot_input_t bi;
	int j;
	vec3_t fwd, right, up;

	memset(&bi, 0, sizeof(bi));

	//add the delta angles to the bot's current view angles
	for (j = 0; j < 3; j++) {
		bs->viewangles[j] = AngleMod(bs->viewangles[j] + SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
	//change the bot view angles
	//[TABBot]
	//the current turn speed blows.
	if(bs->settings.botType == BOT_TAB || bs->settings.botType == BOT_HYBRID)
	{
		BotChangeViewAngles(bs, (float) 2 * elapsed_time / 1000);
	}
	else
	{
		BotChangeViewAngles(bs, (float) elapsed_time / 1000);
	}
	//BotChangeViewAngles(bs, (float) elapsed_time / 1000);
	//[/TABBot]

	//retrieve the bot input
	trap_EA_GetInput(bs->client, (float) time / 1000, &bi);

    // Never jump while standing on a mover/lift; it causes sliding/falling and looks dumb.
    if (BotOnMover(bs))
    {
        bi.actionflags &= ~(ACTION_JUMP|ACTION_DELAYEDJUMP);
    }
	//respawn hack
	if (bi.actionflags & ACTION_RESPAWN) {
		if (bs->lastucmd.buttons & BUTTON_ATTACK) bi.actionflags &= ~(ACTION_RESPAWN|ACTION_ATTACK);
	}

	//[AotCAI]
	if(bs->settings.botType == BOT_AOTC || bs->settings.botType == BOT_HYBRID)
	{
		if ( bs->currentEnemy 
			&& bs->currentEnemy->client
			&& bs->currentEnemy->health > 0
			&& bs->jumpTime <= level.time // Don't walk during jumping...
			&& (DistanceSquared(g_entities[bs->cur_ps.clientNum].r.currentOrigin, bs->currentEnemy->r.currentOrigin) < (300.0f * 300.0f) || walktime[bs->cur_ps.clientNum] > level.time) )
		{
			if ( bs->frame_Enemy_Vis || walktime[bs->cur_ps.clientNum] > level.time )
			{// Unique1 Added. Make bots with an enemy walk...
				//[VoiceEvents][EnhancedImpliment]
				/*
				if (walktime[bs->cur_ps.clientNum] < level.time && talk_time[bs->cur_ps.clientNum] < level.time)
				{// A new enemy.. Taunt them.
					G_BotVoiceEvent( &g_entities[bs->cur_ps.clientNum] );
					talk_time[bs->cur_ps.clientNum] = level.time + Q_irand(30000, 60000);
				}
				*/
				//[/VoiceEvents][EnhancedImpliment]

				bi.actionflags |= ACTION_WALK;
				walktime[bs->cur_ps.clientNum] = level.time + 2000;
			}
			else
			{// Reset.
				walktime[bs->cur_ps.clientNum] = 0;
			}
		}
		else
		{// Reset.
			walktime[bs->cur_ps.clientNum] = 0;
		}
	}
	//[/AotCAI]

	//[SaberSys]
	if (bs->doSaberThrow)
	{
		bi.actionflags |= ACTION_SABERTHROW;
	}
	
	//RAFIXME - Hack to prevent the bots from using fakes all the time
	if(1)
	{
		int curmove = g_entities[bs->client].client->ps.saberMove;
		if( PM_SaberInStart( curmove ) || PM_SaberInTransition( curmove ) )
		{
			bi.actionflags |= ACTION_ATTACK;
		}
	}
	//[/SaberSys]


	//[TABBot]
	if(bs->doWalk)
	{
		bi.actionflags |= ACTION_WALK;
	}
	//[/TABBot]


	// ----------------------------------------------------
	// Ledge hang handling:
	// If we are stuck to a wall in a ledge move (grab/hold),
	// push MOVEFORWARD so PM_AdjustAngleForWallGrap will
	// trigger the pull-up (BOTH_LEDGE_MERCPULL) instead of
	// hanging forever.
	// ----------------------------------------------------
	if ((bs->cur_ps.pm_flags & PMF_STUCK_TO_WALL) &&
		BG_InLedgeMove(bs->cur_ps.legsAnim))
	{
		// Force climb intent: move forward, no back/side noise
		bi.actionflags |= ACTION_MOVEFORWARD;
		bi.actionflags &= ~(ACTION_MOVEBACK | ACTION_MOVELEFT | ACTION_MOVERIGHT);
	}
    // ----------------------------------------------------
    // Bot trooper burst-fire gating (AOTC/HYBRID)
    // Battlefront-style bots should suppress in bursts instead of holding fire forever.
    // Keep this lightweight and never touch BOT_DEFAULT.
    // ----------------------------------------------------
    if ((bs->settings.botType == BOT_AOTC || bs->settings.botType == BOT_HYBRID) &&
        (bi.actionflags & ACTION_ATTACK) && !(bi.actionflags & ACTION_ALT_ATTACK) &&
        bs->cur_ps.weapon != WP_SABER && BotStyle_IsBurstWeapon(bs->cur_ps.weapon))
    {
        int now = level.time;

        // If we are in a firing window, keep shooting.
        if (now < botBurstFireUntil[bs->client])
        {
            // do nothing
        }
        else
        {
            // If we're cooling down, stop firing for a beat.
            if (now < botBurstFireCooldownUntil[bs->client])
            {
                bi.actionflags &= ~ACTION_ATTACK;
            }
            else
            {
                int burstMin, burstMax;
                int coolMin, coolMax;

                // AOTC: shorter bursts, more lane-shift feel.
                if (bs->settings.botType == BOT_AOTC)
                {
                    burstMin = 220;
                    burstMax = 420;
                    coolMin  = 160;
                    coolMax  = 300;
                }
                else
                {
                    // HYBRID: a bit more "Jedi Academy" continuous pressure.
                    burstMin = 260;
                    burstMax = 520;
                    coolMin  = 130;
                    coolMax  = 260;
                }

                botBurstFireUntil[bs->client] = now + Q_irand(burstMin, burstMax);
                botBurstFireCooldownUntil[bs->client] = botBurstFireUntil[bs->client] + Q_irand(coolMin, coolMax);
            }
        }
    }



AngleVectors(bs->viewangles, fwd, right, up);

BotHandleJetpack(bs, &bi, fwd);
BotHandleGrapple(bs, &bi, fwd);
BotHandleThermal(bs, &bi);
BotHandleBackpackRocket(bs, &bi);

BotAvoidTeamCluster(bs, &bi, fwd, right);
BotAvoidTeammates(bs, &bi, fwd, right);

BotDetectAndRecoverStuck(bs, &bi, fwd, right);




BotApplyMoveCommit(bs, &bi);
BotHandleCliffSafety(bs, &bi, fwd, right);
	//convert the bot input to a usercmd
	BotInputToUserCommand(&bi, &bs->lastucmd, bs->cur_ps.delta_angles, time, bs->noUseTime);
	//subtract the delta angles
	for (j = 0; j < 3; j++) {
		bs->viewangles[j] = AngleMod(bs->viewangles[j] - SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
	

	
}

/*
==============
BotAIRegularUpdate
==============
*/
void BotAIRegularUpdate(void) {
	if (regularupdate_time < FloatTime()) {
		trap_BotUpdateEntityItems();
		regularupdate_time = FloatTime() + 0.3;
	}
}

/*
==============
RemoveColorEscapeSequences
==============
*/
void RemoveColorEscapeSequences( char *text ) {
	int i, l;

	l = 0;
	for ( i = 0; text[i]; i++ ) {
		int colorLength = Q_ColorStringLength( &text[i] );
		if ( colorLength ) {
			i += colorLength - 1;
			continue;
		}
		if (text[i] > 0x7E)
			continue;
		text[l++] = text[i];
	}
	text[l] = '\0';
}


/*
==============
BotAI
==============
*/
int BotAI(int client, float thinktime) {
	bot_state_t *bs;
	char buf[1024], *args;
	int j;
#ifdef _DEBUG
	int start = 0;
	int end = 0;
#endif

	trap_EA_ResetInput(client);

	// Get bot state
	bs = botstates[client];
	if (!bs || !bs->inuse) {
		BotAI_Print(PRT_FATAL, "BotAI: client %d is not setup\n", client);
		return qfalse;
	}

	// Retrieve the current client state into bs->cur_ps
	BotAI_GetClientState(client, &bs->cur_ps);

	// Process any pending server commands (centerprints, scores, etc.)
	while (trap_BotGetServerCommand(client, buf, sizeof(buf))) {
		// buf = "cmd args"
		args = strchr(buf, ' ');
		if (!args) {
			continue;
		}
		*args++ = '\0';

		// Strip color codes from args
		RemoveColorEscapeSequences(args);

		if (!Q_stricmp(buf, "cp "))
		{
			/* CenterPrintf – ignored by bots */
		}
		else if (!Q_stricmp(buf, "cs"))
		{
			/* ConfigStringModified – ignored for now */
		}
		else if (!Q_stricmp(buf, "scores"))
		{
			/* Could parse scores here if needed */
		}
		else if (!Q_stricmp(buf, "clientLevelShot"))
		{
			/* ignore */
		}
	}

	// Add the delta angles to the bot's current view angles
	for (j = 0; j < 3; j++) {
		bs->viewangles[j] = AngleMod(bs->viewangles[j] + SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}

	// Increase the local time of the bot
	bs->ltime += thinktime;
	bs->thinktime = thinktime;

	// Origin of the bot
	VectorCopy(bs->cur_ps.origin, bs->origin);

	// Eye coordinates of the bot
	VectorCopy(bs->cur_ps.origin, bs->eye);
	bs->eye[2] += bs->cur_ps.viewheight;

#ifdef _DEBUG
	start = trap_Milliseconds();
#endif

	/*
	 * Bot type dispatch:
	 * ------------------
	 * All variants ultimately run through StandardBotAI.
	 * TAB / HYBRID / AOTC wrappers are allowed to tweak
	 * behaviour flags, movement, etc., but they MUST NOT
	 * change weapon/item/force weights or skill assignment.
	 */
	switch (bs->settings.botType)
	{
	case BOT_TAB:
		TAB_StandardBotAI(bs, thinktime);
		break;

	case BOT_HYBRID:
		HYBRID_StandardBotAI(bs, thinktime);
		break;

	case BOT_AOTC:
		AOTC_StandardBotAI(bs, thinktime);
		break;

	default: // BOT_STANDARD, BOT_FAST or unknown
		StandardBotAI(bs, thinktime);
		break;
	}

#ifdef _DEBUG
	end = trap_Milliseconds();

	trap_Cvar_Update(&bot_debugmessages);

	if (bot_debugmessages.integer)
	{
		Com_Printf("Single AI frametime: %i\n", (end - start));
	}
#endif

	// Subtract the delta angles again
	for (j = 0; j < 3; j++) {
		bs->viewangles[j] = AngleMod(bs->viewangles[j] - SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}

	// Everything was ok
	return qtrue;
}


/*
==================
BotScheduleBotThink
==================
*/
void BotScheduleBotThink(void) {
	int i, botnum;

	botnum = 0;

	for( i = 0; i < MAX_CLIENTS; i++ ) {
		if( !botstates[i] || !botstates[i]->inuse ) {
			continue;
		}
		//initialize the bot think residual time
		botstates[i]->botthink_residual = BOT_THINK_TIME * botnum / numbots;
		botnum++;
	}
}

int PlayersInGame(void)
{
	int i = 0;
	gentity_t *ent;
	int pl = 0;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->client && ent->client->pers.connected == CON_CONNECTED)
		{
			pl++;
		}

		i++;
	}

	return pl;
}

/*
==============
BotAISetupClient
==============
*/
int BotAISetupClient(int client, struct bot_settings_s *settings, qboolean restart) {
	bot_state_t *bs;

	// Guard against invalid indices before touching botstates[].
	/*
	 * Static analysis (C6385): make the bounds check obvious for the analyzer
	 * before any botstates[c] access.
	 */
	if ( client < 0 || client >= MAX_CLIENTS ) {
		BotAI_Print(PRT_FATAL, "BotAISetupClient: invalid client index %d\n", client);
		return qfalse;
	}
#ifdef _MSC_VER
	__analysis_assume( client >= 0 && client < MAX_CLIENTS );
#endif
	const int c = client;

	if (!botstates[c]) {
		botstates[c] = (bot_state_t*)B_Alloc(sizeof(bot_state_t));
		if (!botstates[c]) {
			BotAI_Print(PRT_FATAL, "BotAISetupClient: Failed to allocate bot state for client %d\n", client);
			return qfalse;  // Return failure
		}
	}

	memset(botstates[c], 0, sizeof(bot_state_t));

	// Reset per-client style throttles (global arrays).
	if (client >= 0 && client < MAX_CLIENTS)
	{
		botForceCoverUntil[c] = 0;
		botItemCoverUntil[c] = 0;
	}


		/*
		 * MSVC static analysis (C6385) can still complain here even though we've
		 * proven 'c' is within [0, MAX_CLIENTS) above. Suppress the false positive
		 * for this specific, validated access.
		 */
	#ifdef _MSC_VER
	#pragma warning(suppress:6385)
	#endif
		bs = botstates[c];

	// Ensure settings are valid before copying
	if (settings == NULL) {
		BotAI_Print(PRT_FATAL, "BotAISetupClient: Invalid bot settings for client %d\n", client);
		return qfalse;
	}

	memcpy(&bs->settings, settings, sizeof(bot_settings_t));

	bs->client = client; //need to know the client number before doing personality stuff

	//initialize bot-file weight defaults. Missing bot-file entries should remain unavailable.
	bs->botWeaponWeights[WP_NONE] = 0;
	bs->botWeaponWeights[WP_MELEE] = 0;									
	bs->botWeaponWeights[WP_STUN_BATON] = 0;
	bs->botWeaponWeights[WP_SABER] = 0;
	bs->botWeaponWeights[WP_BRYAR_PISTOL] = 0;
	bs->botWeaponWeights[WP_BLASTER] = 0;
	bs->botWeaponWeights[WP_DISRUPTOR] = 0;
	bs->botWeaponWeights[WP_BOWCASTER] = 0;
	bs->botWeaponWeights[WP_REPEATER] = 0;
	bs->botWeaponWeights[WP_DEMP2] = 0;
	bs->botWeaponWeights[WP_FLECHETTE] = 0;
	bs->botWeaponWeights[WP_ROCKET_LAUNCHER] = 0;
	bs->botWeaponWeights[WP_CONCUSSION] = 0;
	bs->botWeaponWeights[WP_THERMAL] = 0;
	bs->botWeaponWeights[WP_TRIP_MINE] = 0;
	bs->botWeaponWeights[WP_DET_PACK] = 0;
	bs->botWeaponWeights[WP_BRYAR_OLD] = 0;
	
	bs->botItemWeights[HI_NONE] = 0;
	bs->botItemWeights[HI_SEEKER] = 0;
	bs->botItemWeights[HI_SHIELD] = 0;
	bs->botItemWeights[HI_MEDPAC] = 0;
	bs->botItemWeights[HI_SHIELDBOOSTER] = 0;
	bs->botItemWeights[HI_BINOCULARS] = 0;
	bs->botItemWeights[HI_SENTRY_GUN] = 0;
	bs->botItemWeights[HI_JETPACK] = 0;	
	bs->botItemWeights[HI_SQUADTEAM] = 0;	
	bs->botItemWeights[HI_VEHICLEMOUNT] = 0;		
	bs->botItemWeights[HI_EWEB] = 0;			
	bs->botItemWeights[HI_CLOAK] = 0;
	bs->botItemWeights[HI_FLAMETHROWER] = 0;
	bs->botItemWeights[HI_ELECTROSHOCKER] = 0;	
	bs->botItemWeights[HI_SPHERESHIELD] = 0;
	bs->botItemWeights[HI_OVERLOAD] = 0;
	bs->botItemWeights[HI_GRAPPLE] = 0;

	bs->botForceTypeWeights[FT_NONE] = 0;
	bs->botForceTypeWeights[FT_PUSHA] = 0;
	bs->botForceTypeWeights[FT_PULLA] = 0;	
	bs->botForceTypeWeights[FT_HEALA] = 0;	
	bs->botForceTypeWeights[FT_PROTECTA] = 0;	
	bs->botForceTypeWeights[FT_ABSORBA] = 0;
	bs->botForceTypeWeights[FT_TELEPATHYA] = 0;
	bs->botForceTypeWeights[FT_STASISA] = 0;
	bs->botForceTypeWeights[FT_GRIPA] = 0;
	bs->botForceTypeWeights[FT_LIGHTNINGA] = 0;
	bs->botForceTypeWeights[FT_DRAINA] = 0;
	bs->botForceTypeWeights[FT_RAGEA] = 0;
	bs->botForceTypeWeights[FT_DESTRUCTIONA] = 0;
	
	bs->botWeaponTypeWeights[WT_NONE] = 0;
	bs->botWeaponTypeWeights[WT_WRISTA] = 0;
	bs->botWeaponTypeWeights[WT_WRISTB] = 0;
	bs->botWeaponTypeWeights[WT_PISTOLA] = 0;
	bs->botWeaponTypeWeights[WT_PISTOLB] = 0;
	bs->botWeaponTypeWeights[WT_BLASTERA] = 0;
	bs->botWeaponTypeWeights[WT_BLASTERB] = 0;	
	bs->botWeaponTypeWeights[WT_DISRUPTORA] = 0;
	bs->botWeaponTypeWeights[WT_DISRUPTORB] = 0;		
	bs->botWeaponTypeWeights[WT_BOWCASTERA] = 0;
	bs->botWeaponTypeWeights[WT_BOWCASTERB] = 0;	
	bs->botWeaponTypeWeights[WT_REPEATERA] = 0;
	bs->botWeaponTypeWeights[WT_REPEATERB] = 0;		
	bs->botWeaponTypeWeights[WT_DEMP2A] = 0;
	bs->botWeaponTypeWeights[WT_DEMP2B] = 0;		
	bs->botWeaponTypeWeights[WT_FLECHETTEA] = 0;
	bs->botWeaponTypeWeights[WT_FLECHETTEB] = 0;	
	bs->botWeaponTypeWeights[WT_CONCUSSIONA] = 0;
	bs->botWeaponTypeWeights[WT_CONCUSSIONB] = 0;	
	bs->botWeaponTypeWeights[WT_ROCKETA] = 0;
	bs->botWeaponTypeWeights[WT_ROCKETB] = 0;	
	bs->botWeaponTypeWeights[WT_THERMALA] = 0;
	bs->botWeaponTypeWeights[WT_THERMALB] = 0;
	bs->botWeaponTypeWeights[WT_TRIPMINEA] = 0;
	bs->botWeaponTypeWeights[WT_TRIPMINEB] = 0;
	bs->botWeaponTypeWeights[WT_DETPACKA] = 0;
	bs->botWeaponTypeWeights[WT_DETPACKB] = 0;
	bs->botWeaponTypeWeights[WT_OLDA] = 0;
	bs->botWeaponTypeWeights[WT_OLDB] = 0;
	
	
	

	bs->botItemTypeWeights[HT_NONE] = 0;
	bs->botItemTypeWeights[HT_FLAMETHROWERA] = 0;
	bs->botItemTypeWeights[HT_ELECTROSHOCKERA] = 0;
	bs->botItemTypeWeights[HT_JETPACKA] = 0;
	bs->botItemTypeWeights[HT_JETPACKB] = 0;
	bs->botItemTypeWeights[HT_JETPACKC] = 0;
	bs->botItemTypeWeights[HT_SQUADTEAMA] = 0;
	bs->botItemTypeWeights[HT_SQUADTEAMB] = 0;
	bs->botItemTypeWeights[HT_SQUADTEAMC] = 0;
	//[BotTweaks] UNIQUEFIXME - wha?
//	bs->botWeaponWeights[WP_TRIP_MINE] = 3;
//	bs->botWeaponWeights[WP_TRIP_MINE] = 10;
	//[/BotTweaks]UNIQUEFIXME - wha?
//	bs->botWeaponWeights[WP_DET_PACK] = 0;
//	bs->botWeaponWeights[WP_MELEE] = 1;
//	bs->botWeaponWeights[WP_BRYAR_OLD] = 10; 
	//[NewWeapons][EnhancedImpliment]
	/*
	bs->botWeaponWeights[WP_BRYAR_OLD] = 1;
	bs->botWeaponWeights[WP_ZAM_BLASTER] = 2;
	bs->botWeaponWeights[WP_NABOO_BLASTER] = 2;
	bs->botWeaponWeights[WP_NONE2] = 0;
	bs->botWeaponWeights[WP_FLAMETHROWER] = 17;
	bs->botWeaponWeights[WP_FREE2] = 0;
	bs->botWeaponWeights[WP_FREE3] = 0;
	bs->botWeaponWeights[WP_FREE4] = 0;
	bs->botWeaponWeights[WP_FREE5] = 0;
	bs->botWeaponWeights[WP_FREE6] = 0;
	bs->botWeaponWeights[WP_FREE7] = 0;
	bs->botWeaponWeights[WP_FREE8] = 0;
	bs->botWeaponWeights[WP_DKA_ARM] = 0;
	bs->botWeaponWeights[WP_SBD_ARM] = 20;
	bs->botWeaponWeights[WP_ADV_DISRUPTOR] = 14;
	bs->botWeaponWeights[WP_ADV_ROCKET] = 19;
	bs->botWeaponWeights[WP_GRAPPLE_HOOK] = 0;
	bs->botWeaponWeights[WP_TRIP_MINE_2] = 5;
	bs->botWeaponWeights[WP_NONE3] = 0;
	bs->botWeaponWeights[WP_WESTAR] = 3;
	bs->botWeaponWeights[WP_FETT_BLASTER] = 4;
	bs->botWeaponWeights[WP_ZAM_RIFLE] = 7;
	bs->botWeaponWeights[WP_TUSKEN_RIFLE] = 7;
	bs->botWeaponWeights[WP_CLONE_LIGHT_RIFLE] = 7;
	bs->botWeaponWeights[WP_SEP_ROCKET] = 19;
	bs->botWeaponWeights[WP_MP_BLASTER] = 8;
	bs->botWeaponWeights[WP_OFFICER_PISTOL] = 9;
	bs->botWeaponWeights[WP_NABOO_S5] = 8;
	bs->botWeaponWeights[WP_CLONE_BLASTER] = 8;
	bs->botWeaponWeights[WP_DROID_BLASTER] = 8;
	bs->botWeaponWeights[WP_TUSKEN_RIFLE2] = 8;
	bs->botWeaponWeights[WP_NABOO_SEC_PISTOL] = 7;
	bs->botWeaponWeights[WP_SITH_SCEPTER] = 20;
	bs->botWeaponWeights[WP_NONE4] = 0;
	bs->botWeaponWeights[WP_VIBROBLADE2] = 13; // For enhancing lightsaber... But if we need more weapons, these can be used! - uq1
	bs->botWeaponWeights[WP_FORCE_PIKE] = 11; // For enhancing lightsaber...
	bs->botWeaponWeights[WP_AMPHI_STAFF] = 12; // For enhancing lightsaber...
	bs->botWeaponWeights[WP_VIBROAXE2] = 11; // For enhancing lightsaber...
	bs->botWeaponWeights[WP_SABER6] = 0; // For enhancing lightsaber...
	bs->botWeaponWeights[WP_VIBROAXE] = 10;
	bs->botWeaponWeights[WP_VIBRODAGGER] = 9;
	bs->botWeaponWeights[WP_VIBROBLADE] = 10;
	bs->botWeaponWeights[WP_SITH_SWORD] = 14;
	bs->botWeaponWeights[WP_STAFF1] = 15;
	bs->botWeaponWeights[WP_TUSKEN_STAFF] = 11;
	bs->botWeaponWeights[WP_NOGHRI_STICK] = 10;

//	WP_EMPLACED_GUN, // Always at the end...
//	WP_TURRET, // Always at the end...
	*/
	//[/NewWeapons]

	BotUtilizePersonality(bs);

	if (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL)
	{
		bs->botWeaponWeights[WP_SABER] = 13;
	}

	//allocate a goal state
	bs->gs = trap_BotAllocGoalState(client);

	//allocate a weapon state
	bs->ws = trap_BotAllocWeaponState();

	bs->inuse = qtrue;
	bs->entitynum = client;
	bs->setupcount = 4;
	bs->entergame_time = FloatTime();
	bs->ms = trap_BotAllocMoveState();
	numbots++;

	//NOTE: reschedule the bot thinking
	BotScheduleBotThink();

	if (PlayersInGame())
	{ //don't talk to yourself
		BotDoChat(bs, "GeneralGreetings", 0);
	}

	return qtrue;
}

/*
==============
BotAIShutdownClient
==============
*/
int BotAIShutdownClient(int client, qboolean restart) {
	// Guard against invalid indices before touching botstates[].
	if (client < 0 || client >= MAX_CLIENTS) {
		return qfalse;
	}

	// Reset per-client style throttles (global arrays).
	{
		botForceCoverUntil[client] = 0;
		botItemCoverUntil[client] = 0;
	}

	bot_state_t *bs;

	bs = botstates[client];
	if (!bs || !bs->inuse) {
		//BotAI_Print(PRT_ERROR, "BotAIShutdownClient: client %d already shutdown\n", client);
		return qfalse;
	}

	trap_BotFreeMoveState(bs->ms);
	//free the goal state`			
	trap_BotFreeGoalState(bs->gs);
	//free the weapon weights
	trap_BotFreeWeaponState(bs->ws);
	//
	//clear the bot state
	memset(bs, 0, sizeof(bot_state_t));
	//set the inuse flag to qfalse
	bs->inuse = qfalse;
	//there's one bot less
	numbots--;
	//everything went ok
	return qtrue;
}

/*
==============
BotResetState

called when a bot enters the intermission or observer mode and
when the level is changed
==============
*/
void BotResetState(bot_state_t *bs) {
	int client, entitynum, inuse;
	int movestate, goalstate, weaponstate;
	bot_settings_t settings;
	playerState_t ps;							//current player state
	float entergame_time;

	//save some things that should not be reset here
	memcpy(&settings, &bs->settings, sizeof(bot_settings_t));
	memcpy(&ps, &bs->cur_ps, sizeof(playerState_t));
	inuse = bs->inuse;
	client = bs->client;
	entitynum = bs->entitynum;
	movestate = bs->ms;
	goalstate = bs->gs;
	weaponstate = bs->ws;
	entergame_time = bs->entergame_time;
	//reset the whole state
	memset(bs, 0, sizeof(bot_state_t));
	//copy back some state stuff that should not be reset
	bs->ms = movestate;
	bs->gs = goalstate;
	bs->ws = weaponstate;
	memcpy(&bs->cur_ps, &ps, sizeof(playerState_t));
	memcpy(&bs->settings, &settings, sizeof(bot_settings_t));
	bs->inuse = inuse;
	bs->client = client;
	bs->entitynum = entitynum;
	bs->entergame_time = entergame_time;
	//reset several states
	if (bs->ms) trap_BotResetMoveState(bs->ms);
	if (bs->gs) trap_BotResetGoalState(bs->gs);
	if (bs->ws) trap_BotResetWeaponState(bs->ws);
	if (bs->gs) trap_BotResetAvoidGoals(bs->gs);
	if (bs->ms) trap_BotResetAvoidReach(bs->ms);

}

/*
==============
BotAILoadMap
==============
*/
int BotAILoadMap( int restart ) {
	int			i;

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (botstates[i] && botstates[i]->inuse) {
			BotResetState( botstates[i] );
			botstates[i]->setupcount = 4;
		}
	}

	return qtrue;
}

//rww - bot ai

//standard visibility check
int OrgVisible(vec3_t org1, vec3_t org2, int ignore)
{
	trace_t tr;

	trap_Trace(&tr, org1, NULL, NULL, org2, ignore, MASK_SOLID);

	if (tr.fraction == 1)
	{
		return 1;
	}

	return 0;
}

//special waypoint visibility check
//RACC
//0 = wall in way
//1 = player or no obstruction
//2 = force field in the way.
int WPOrgVisible(gentity_t *bot, vec3_t org1, vec3_t org2, int ignore)
{
	trace_t tr;
	gentity_t *ownent;

	trap_Trace(&tr, org1, NULL, NULL, org2, ignore, MASK_SOLID);

	if (tr.fraction == 1)
	{
		trap_Trace(&tr, org1, NULL, NULL, org2, ignore, MASK_PLAYERSOLID);

		if (tr.fraction != 1 && tr.entityNum != ENTITYNUM_NONE && g_entities[tr.entityNum].s.eType == ET_SPECIAL)
		{
			if (g_entities[tr.entityNum].parent && g_entities[tr.entityNum].parent->client)
			{
				ownent = g_entities[tr.entityNum].parent;

				if (OnSameTeam(bot, ownent) || bot->s.number == ownent->s.number)
				{
					return 1;
				}
			}
			return 2;
		}

		return 1;
	}

	return 0;
}

//visibility check with hull trace
int OrgVisibleBox(vec3_t org1, vec3_t mins, vec3_t maxs, vec3_t org2, int ignore)
{
	trace_t tr;

	if (g_RMG.integer)
	{
		trap_Trace(&tr, org1, NULL, NULL, org2, ignore, MASK_SOLID);
	}
	else
	{
		trap_Trace(&tr, org1, mins, maxs, org2, ignore, MASK_SOLID);
	}

	if (tr.fraction == 1 && !tr.startsolid && !tr.allsolid)
	{
		return 1;
	}

	return 0;
}

//see if there's a func_* ent under the given pos.
//kind of badly done, but this shouldn't happen
//often.

// --- Lift/door WAITFORFUNC anti-cluster helpers ---
static int bot_waitfunc_since[MAX_CLIENTS];
static int bot_waitfunc_backoff_until[MAX_CLIENTS];
static int bot_waitfunc_last_wp[MAX_CLIENTS];

static void BotWaitFunc_Reset(int clientNum)
{
	bot_waitfunc_since[clientNum] = 0;
	bot_waitfunc_backoff_until[clientNum] = 0;
	bot_waitfunc_last_wp[clientNum] = 0;
}

// Returns qtrue if we should actively back away from the wait point to avoid clogging
static qboolean BotWaitFunc_ShouldBackoff(bot_state_t *bs, const vec3_t wpOrg)
{
	const int cn = bs->client;
	vec3_t d;
	float dist2;

	VectorSubtract(bs->origin, wpOrg, d);
	dist2 = d[0]*d[0] + d[1]*d[1];

	// Too close to the wait origin (often inside lift shaft mouth / door trigger volume)
	if (dist2 < (64.0f*64.0f))
	{
		return qtrue;
	}
	// If we're below the wait origin while platform isn't present, we may be under an elevator.
	if (bs->origin[2] < wpOrg[2] - 24.0f)
	{
		return qtrue;
	}
	return qfalse;
}

// Apply movement keys to move away from the WP origin (with slight side bias for dispersal).
static void BotWaitFunc_DoBackoff(bot_state_t *bs, const vec3_t wpOrg)
{
	vec3_t away, fwd, right;
	float fDot, rDot;
	int cn = bs->client;

	VectorSubtract(bs->origin, wpOrg, away);
	away[2] = 0;

	// If we're exactly at the origin, just use facing.
	if (VectorNormalize(away) < 0.001f)
	{
		AngleVectors(bs->viewangles, fwd, right, NULL);
		VectorScale(fwd, -1.0f, away);
	}

	AngleVectors(bs->viewangles, fwd, right, NULL);
	fDot = DotProduct(fwd, away);
	rDot = DotProduct(right, away);

	// Prefer moving "away" relative to facing.
	if (fDot > 0.25f)
	{
		trap_EA_MoveForward(cn);
	}
	else if (fDot < -0.25f)
	{
		trap_EA_MoveBack(cn);
	}
	else
	{
		// If roughly perpendicular, back up to clear volume.
		trap_EA_MoveBack(cn);
	}

	// Side bias to reduce clumping (deterministic per bot)
	if ((cn & 1) == 0)
	{
		if (rDot > 0.0f) trap_EA_MoveRight(cn);
		else trap_EA_MoveLeft(cn);
	}
	else
	{
		if (rDot > 0.0f) trap_EA_MoveLeft(cn);
		else trap_EA_MoveRight(cn);
	}
}
int CheckForFunc(vec3_t org, int ignore)
{
	gentity_t *fent;
	vec3_t under;
	trace_t tr;

	VectorCopy(org, under);

	under[2] -= 64;

	trap_Trace(&tr, org, vec3_origin, vec3_origin, under, ignore, MASK_SOLID);

	if (tr.fraction == 1)
	{
		return 0;
	}

	fent = &g_entities[tr.entityNum];

	if (!fent)
	{
		return 0;
	}

	if (strstr(fent->classname, "func_"))
	{
		return 1; //there's a func brush here
	}

	return 0;
}

//perform pvs check based on rmg or not
qboolean BotPVSCheck( const vec3_t p1, const vec3_t p2 )
{
	if (g_RMG.integer && bot_pvstype.integer)
	{
		vec3_t subPoint;
		VectorSubtract(p1, p2, subPoint);

		if (VectorLength(subPoint) > 5000)
		{
			return qfalse;
		}
		return qtrue;
	}

	return trap_InPVS(p1, p2);
}


//get the index to the nearest visible waypoint in the global trail
int GetNearestVisibleWP(vec3_t org, int ignore)
{
	int i;
	float bestdist;
	float flLen;
	int bestindex;
	vec3_t a, mins, maxs;

	i = 0;
	if (g_RMG.integer)
	{
		bestdist = 300;
	}
	else
	{
		bestdist = 800;//99999;
				   //don't trace over 800 units away to avoid GIANT HORRIBLE SPEED HITS ^_^
	}
	bestindex = -1;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -1;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 1;

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse)
		{
			VectorSubtract(org, gWPArray[i]->origin, a);
			flLen = VectorLength(a);

			if (flLen < bestdist && (g_RMG.integer || BotPVSCheck(org, gWPArray[i]->origin)) && OrgVisibleBox(org, mins, maxs, gWPArray[i]->origin, ignore))
			{
				bestdist = flLen;
				bestindex = i;
			}
		}

		i++;
	}

	return bestindex;
}

//wpDirection
//0 == FORWARD
//1 == BACKWARD

//see if this is a valid waypoint to pick up in our
//current state (whatever that may be)
int PassWayCheck(bot_state_t *bs, int windex)
{
	if (!gWPArray[windex] || !gWPArray[windex]->inuse)
	{ //bad point index
		return 0;
	}

	if (g_RMG.integer)
	{
		if ((gWPArray[windex]->flags & WPFLAG_RED_FLAG) ||
			(gWPArray[windex]->flags & WPFLAG_BLUE_FLAG))
		{ //red or blue flag, we'd like to get here
			return 1;
		}
	}

	//[NewGametypes][EnhanceImpliment]
	//if (g_gametype.integer == GT_SCENARIO)
	//	return 1;
	//[/NewGametypes]

	if (bs->wpDirection && (gWPArray[windex]->flags & WPFLAG_ONEWAY_FWD))
	{ //we're not travelling in a direction on the trail that will allow us to pass this point
		return 0;
	}
	else if (!bs->wpDirection && (gWPArray[windex]->flags & WPFLAG_ONEWAY_BACK))
	{ //we're not travelling in a direction on the trail that will allow us to pass this point
		return 0;
	}

	if (bs->wpCurrent && gWPArray[windex]->forceJumpTo &&
		gWPArray[windex]->origin[2] > (bs->wpCurrent->origin[2]+64) &&
		bs->cur_ps.fd.forcePowerLevel[FP_LEVITATION] < gWPArray[windex]->forceJumpTo)
	{ //waypoint requires force jump level greater than our current one to pass
		return 0;
	}

	return 1;
}


//tally up the distance between two waypoints
float TotalTrailDistance(int start, int end, bot_state_t *bs)
{
	int beginat;
	int endat;
	float distancetotal;

	distancetotal = 0;

	if (start > end)
	{
		beginat = end;
		endat = start;
	}
	else
	{
		beginat = start;
		endat = end;
	}

	while (beginat < endat)
	{
		if (beginat >= gWPNum || !gWPArray[beginat] || !gWPArray[beginat]->inuse)
		{ //invalid waypoint index
			return -1;
		}

		if (!g_RMG.integer)
		{
			if ((end > start && gWPArray[beginat]->flags & WPFLAG_ONEWAY_BACK) ||
				(start > end && gWPArray[beginat]->flags & WPFLAG_ONEWAY_FWD))
			{ //a one-way point, this means this path cannot be travelled to the final point
				return -1;
			}
		}
	#if 0 //disabled force jump checks for now
		if (gWPArray[beginat]->forceJumpTo)
		{
			if (gWPArray[beginat-1] && gWPArray[beginat-1]->origin[2]+64 < gWPArray[beginat]->origin[2])
			{
				gdif = gWPArray[beginat]->origin[2] - gWPArray[beginat-1]->origin[2];
			}

			if (gdif)
			{
				if (bs && bs->cur_ps.fd.forcePowerLevel[FP_LEVITATION] < gWPArray[beginat]->forceJumpTo)
				{
					return -1;
				}
			}
		}
		
		if (bs->wpCurrent && gWPArray[windex]->forceJumpTo &&
			gWPArray[windex]->origin[2] > (bs->wpCurrent->origin[2]+64) &&
			bs->cur_ps.fd.forcePowerLevel[FP_LEVITATION] < gWPArray[windex]->forceJumpTo)
		{
			return -1;
		}
#endif



		distancetotal += gWPArray[beginat]->disttonext;

		beginat++;
	}

	return distancetotal;
}
// Route optimization throttling: don't re-pick "shorter" routes too often
static int botNextRouteOptimizeTime[MAX_CLIENTS];

//see if there's a route shorter than our current one to get
//to the final destination we currently desire
void CheckForShorterRoutes(bot_state_t *bs, int newwpindex)
{
    int i = 0;
    int bestindex;
    int checklen;
    int bestlen;
    int fj = 0;
    int clientNum;
    gclient_t *client;
    const wpobject_t *curWP;
    int neighborCount;

    if (!bs || !bs->wpCurrent || !bs->wpDestination)
    {
        return;
    }

    clientNum = bs->cur_ps.clientNum;

    {
        const bot_nav_profile_t navp = BotNavProfile(bs);
        (void)navp; // used below
    }

    if (clientNum < 0 || clientNum >= MAX_CLIENTS)
    {
        return;
    }

    client = g_entities[clientNum].client;

    // --- NEW: throttle how often we try to "optimize" the route ---
    if (level.time < botNextRouteOptimizeTime[clientNum])
    {
        return;
    }
    {
        const bot_nav_profile_t navp = BotNavProfile(bs);
        botNextRouteOptimizeTime[clientNum] = level.time + navp.routeOptimizeIntervalMS;
    }
    // ---------------------------------------------------------------
    // Validate waypoint index/pointer before dereferencing gWPArray[newwpindex]
    if (newwpindex < 0 || newwpindex >= gWPNum)
    {
        return;
    }
    curWP = gWPArray[newwpindex];
    if (!curWP)
    {
        return;
    }

    //must have neighbors to check off of
    neighborCount = curWP->neighbornum;
    if (neighborCount <= 0)
    {
        return;
    }
    if (neighborCount > MAX_NEIGHBOR_SIZE)
    {
        neighborCount = MAX_NEIGHBOR_SIZE;
    }

    //get the trail distance for our wp
    bestindex = newwpindex;
    bestlen = TotalTrailDistance(newwpindex, bs->wpDestination->index, bs);

    // Improvement required before switching routes (avoid jitter).
    // Tuned per botType via BotNavProfile().
    const int improveThreshold = (bs->settings.botType == BOT_TAB) ? 64 :
                                (bs->settings.botType == BOT_HYBRID) ? 96 :
                                (bs->settings.botType == BOT_AOTC) ? 160 :
                                128;


    while (i < neighborCount)
    {   //now go through the neighbors and check the distance to the desired point from each neighbor
        const wpneighbor_t *neighbor = &curWP->neighbors[i];

        checklen = TotalTrailDistance(neighbor->num,
                                      bs->wpDestination->index, bs);

        // Behavioural bias: AOTC prefers cover-ish nodes and avoids jump/parkour unless needed.
        // TAB prefers decisive vertical routes (reduced penalty for jump nodes).
        if (checklen > 0)
        {
            const int n = neighbor->num;
            const wpobject_t *wpN = (n >= 0 && n < gWPNum) ? gWPArray[n] : NULL;
            if (wpN)
            {
                const int isCover = (wpN->flags & (WPFLAG_SNIPEORCAMP|WPFLAG_SNIPEORCAMPSTAND|WPFLAG_NOVIS)) ? 1 : 0;
                const int isJumpy = (wpN->flags & WPFLAG_JUMP) || wpN->forceJumpTo;

                if (bs->settings.botType == BOT_AOTC)
                {
                    if (isCover) checklen -= 96;
                    if (isJumpy) checklen += 220;
                }
                else if (bs->settings.botType == BOT_TAB)
                {
                    if (isCover) checklen += 48;
                    if (isJumpy) checklen += 40;
                }
                else if (bs->settings.botType == BOT_HYBRID)
                {
                    if (isCover) checklen -= 40;
                    if (isJumpy) checklen += 120;
                }
            }
        }

        // --- TWEAK: only accept routes that are *significantly* shorter ---
        // was: if (checklen < bestlen-64 || bestlen == -1)
        if (bestlen == -1 || checklen + improveThreshold < bestlen)
        {
            bestlen = checklen;
            bestindex = neighbor->num;

            if (neighbor->forceJumpTo)
            {
                fj = neighbor->forceJumpTo;
            }
            else
            {
                fj = 0;
            }
        }

        i++;
    }

    if (bestindex != newwpindex && bestindex != -1)
    {   //we found a path we want to switch to, let's do it
        if (bestindex < 0 || bestindex >= gWPNum || !gWPArray[bestindex])
        {
            return;
        }

        bs->wpCurrent = gWPArray[bestindex];
        BotCommitToWaypoint(bs, bestindex);
        bs->wpSwitchTime = level.time + 3000;

        if (fj)
        {
#ifndef FORCEJUMP_INSTANTMETHOD
            if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
            {
                client->jetPackOn = qtrue;
				client->ps.pm_type = PM_JETPACK;
				bs->cur_ps.eFlags |= EF_JETPACK;
                bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
                bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
                bs->jumpHoldTime = ((bs->forceJumpChargeTime - level.time)/2) + level.time;
            }
            bs->forceJumpChargeTime = level.time + 1000;
            bs->beStill = level.time + 1000;
            bs->forceJumping = bs->forceJumpChargeTime;
#else
            bs->beStill = level.time + 500;
            bs->jumpTime = level.time + fj*1200;
            bs->jDelay = level.time + 200;
            bs->forceJumping = bs->jumpTime;
#endif
        }
    }
}


//check for flags on the waypoint we're currently travelling to
//and perform the desired behavior based on the flag
void WPConstantRoutine(bot_state_t *bs)
{
	gclient_t *client = g_entities[bs->cur_ps.clientNum].client;
	if (!bs->wpCurrent)
	{
		return;
	}

	if (bs->wpCurrent->flags & WPFLAG_DUCK)
	{ //duck while travelling to this point
		bs->duckTime = level.time + 100;
	}

#ifndef FORCEJUMP_INSTANTMETHOD
	if (bs->wpCurrent->flags & WPFLAG_JUMP)
	{ //jump while travelling to this point
		float heightDif = (bs->wpCurrent->origin[2] - bs->origin[2]+16);

		if (bs->origin[2]+16 >= bs->wpCurrent->origin[2])
		{ //don't need to jump, we're already higher than this point
			heightDif = 0;
		}
		if (heightDif > 128 && bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
		{// Jetpacker.. Jetpack ON!
			client->jetPackOn = qtrue;
				client->ps.pm_type = PM_JETPACK;
				bs->cur_ps.eFlags |= EF_JETPACK;
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
			bs->jumpHoldTime = ((bs->forceJumpChargeTime - level.time)/2) + level.time;
		}

		if (heightDif > 40 && (bs->cur_ps.fd.forcePowersKnown & (1 << FP_LEVITATION)) && (bs->cur_ps.fd.forceJumpCharge < (forceJumpStrength[bs->cur_ps.fd.forcePowerLevel[FP_LEVITATION]]-100) || bs->cur_ps.groundEntityNum == ENTITYNUM_NONE))
		{ //alright, let's jump
			bs->forceJumpChargeTime = level.time + 1000;
			if (bs->cur_ps.groundEntityNum != ENTITYNUM_NONE && bs->jumpPrep < (level.time-300))
			{
				bs->jumpPrep = level.time + 700;
			}
			bs->beStill = level.time + 300;
			bs->jumpTime = 0;

			if (bs->wpSeenTime < (level.time + 600))
			{
				bs->wpSeenTime = level.time + 600;
			}
		}
		else if (heightDif > 64 &&
			!(bs->cur_ps.fd.forcePowersKnown & (1 << FP_LEVITATION)))
		{
			int otherIndex;

			// candidate wp in opposite direction
			if (bs->wpDirection)
				otherIndex = bs->wpCurrent->index - 1;
			else
				otherIndex = bs->wpCurrent->index + 1;

			if (otherIndex >= 0 && otherIndex < gWPNum &&
				gWPArray[otherIndex] && gWPArray[otherIndex]->inuse &&
				PassWayCheck(bs, otherIndex))
			{
				// OK, we can safely turn around
				bs->wpCurrent = gWPArray[otherIndex];
				BotCommitToWaypoint(bs, otherIndex);
				bs->wpDirection ^= 1;
				return;
			}

			// Otherwise: DON'T nuke wpCurrent, just give up the jump flag
			// so bots can still walk the trail
			bs->wpCurrent->flags &= ~WPFLAG_JUMP;
		}
	}
#endif

	if (bs->wpCurrent->forceJumpTo)
	{
#ifdef FORCEJUMP_INSTANTMETHOD
		if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
		{
			client->jetPackOn = qtrue;
				client->ps.pm_type = PM_JETPACK;
				bs->cur_ps.eFlags |= EF_JETPACK;
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
			bs->jumpHoldTime = ((bs->forceJumpChargeTime - level.time)/2) + level.time;
		}
		if (bs->origin[2]+16 < bs->wpCurrent->origin[2])
		{
			bs->jumpTime = level.time + 100;
		}
#else
		float heightDif = (bs->wpCurrent->origin[2] - bs->origin[2]+16);

		if (bs->origin[2]+16 >= bs->wpCurrent->origin[2])
		{ //then why exactly would we be force jumping?
			heightDif = 0;
		}

		if (bs->cur_ps.fd.forceJumpCharge < (forceJumpStrength[bs->cur_ps.fd.forcePowerLevel[FP_LEVITATION]]-100))
		{
			bs->forceJumpChargeTime = level.time + 200;
		}
#endif
	}
}

//check if our ctf state is to guard the base
qboolean BotCTFGuardDuty(bot_state_t *bs)
{
	if (g_gametype.integer != GT_CTF &&
		g_gametype.integer != GT_CTY)
	{
		return qfalse;
	}

	if (bs->ctfState == CTFSTATE_DEFENDER)
	{
		return qtrue;
	}

	return qfalse;
}

//when we reach the waypoint we are travelling to,
//this function will be called. We will perform any
//checks for flags on the current wp and activate
//any "touch" events based on that.
void WPTouchRoutine(bot_state_t *bs)
{
	
	if ( !bs ) {
		return;
	}
int lastNum;
	gclient_t *client;

	if ( !bs )
	{
		return;
	}

	client = g_entities[bs->cur_ps.clientNum].client;

	if (!bs->wpCurrent)
	{
		return;
	}

	bs->wpTravelTime = level.time + 10000;

	if (bs->wpCurrent->flags & WPFLAG_NOMOVEFUNC)
	{ //don't try to use any nearby map objects for a little while
		bs->noUseTime = level.time + 4000;
	}

#ifdef FORCEJUMP_INSTANTMETHOD
	if ((bs->wpCurrent->flags & WPFLAG_JUMP) && bs->wpCurrent->forceJumpTo)
	{ //jump if we're flagged to but not if this indicates a force jump point. Force jumping is
	  //handled elsewhere.
		if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
		{
			client->jetPackOn = qtrue;
				client->ps.pm_type = PM_JETPACK;
				bs->cur_ps.eFlags |= EF_JETPACK;
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
			bs->jumpHoldTime = ((bs->forceJumpChargeTime - level.time)/2) + level.time;
		}
		bs->jumpTime = level.time + 100;
	}
#else
	if ((bs->wpCurrent->flags & WPFLAG_JUMP) && !bs->wpCurrent->forceJumpTo)
	{ //jump if we're flagged to but not if this indicates a force jump point. Force jumping is
	  //handled elsewhere.
		if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
		{
			client->jetPackOn = qtrue;
				client->ps.pm_type = PM_JETPACK;
				bs->cur_ps.eFlags |= EF_JETPACK;
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
			bs->jumpHoldTime = ((bs->forceJumpChargeTime - level.time)/2) + level.time;
		}
		bs->jumpTime = level.time + 100;
	}
#endif

	if (bs->isCamper && bot_camp.integer && !(bs && bs->settings.botType == BOT_TAB) && (BotIsAChickenWuss(bs) || BotCTFGuardDuty(bs) || bs->isCamper == 2) && ((bs->wpCurrent->flags & WPFLAG_SNIPEORCAMP) || (bs->wpCurrent->flags & WPFLAG_SNIPEORCAMPSTAND)) &&
		bs->cur_ps.weapon != WP_SABER && bs->cur_ps.weapon != WP_MELEE && bs->cur_ps.weapon != WP_STUN_BATON)
	{ //if we're a camper and a chicken then camp
		if (bs->wpDirection)
		{
			lastNum = bs->wpCurrent->index+1;
		}
		else
		{
			lastNum = bs->wpCurrent->index-1;
		}

		if (gWPArray[lastNum] && gWPArray[lastNum]->inuse && gWPArray[lastNum]->index && bs->isCamping < level.time)
		{
			bs->isCamping = level.time + rand()%15000 + 30000;
			bs->wpCamping = bs->wpCurrent;
			bs->wpCampingTo = gWPArray[lastNum];

			if (bs->wpCurrent->flags & WPFLAG_SNIPEORCAMPSTAND)
			{
				bs->campStanding = qtrue;
			}
			else
			{
				bs->campStanding = qfalse;
			}
		}

	}
	else if ((bs->cur_ps.weapon == WP_SABER  || bs->cur_ps.weapon == WP_STUN_BATON || bs->cur_ps.weapon == WP_MELEE) &&
		bs->isCamping > level.time)
	{ //don't snipe/camp with a melee weapon, that would be silly
		bs->isCamping = 0;
		bs->wpCampingTo = NULL;
		bs->wpCamping = NULL;
	}

	if (bs->wpDestination)
	{
		if (bs->wpCurrent->index == bs->wpDestination->index)
		{
			bs->wpDestination = NULL;

			if (bs->runningLikeASissy)
			{ //this obviously means we're scared and running, so we'll want to keep our navigational priorities less delayed
				bs->destinationGrabTime = level.time + 500;
			}
			else
			{
				bs->destinationGrabTime = level.time + 3500;
			}
		}
		//[NewGametypes][EnhanceImpliment]
		//else if (g_gametype.integer != GT_SCENARIO)
		else
		//[/NewGametypes]
		{
			CheckForShorterRoutes(bs, bs->wpCurrent->index);
		}
	}
}

//could also slowly lerp toward, but for now
//just copying straight over.
void MoveTowardIdealAngles(bot_state_t *bs)
{
	//[PlayerClasses][EnhanceImpliment]
	/*
	if (mod_classes.integer == 2 
		&& classnumber[bs->entitynum] != GCLASS_FORCEMASTER
		&& classnumber[bs->entitynum] != GCLASS_JEDI 
		&& !bs->currentEnemy 
		&& bs->wpCurrent )
	{// Non-Jedi should never combat move unless attacking in scenario...
		vec3_t a, ang;

		VectorSubtract(gWPArray[bs->wpCurrent->index]->origin, g_entities[bs->entitynum].r.currentOrigin, a);
		vectoangles(a, ang);

		VectorCopy(ang, bs->goalAngles);
	}
	*/
	//[/PlayerClasses]

	VectorCopy(bs->goalAngles, bs->ideal_viewangles);
}

#define BOT_STRAFE_AVOIDANCE

#ifdef BOT_STRAFE_AVOIDANCE
#define STRAFEAROUND_RIGHT			1
#define STRAFEAROUND_LEFT			2

//do some trace checks for strafing to get an idea of where we
//are and if we should move to avoid obstacles.
int BotTrace_Strafe(bot_state_t *bs, vec3_t traceto)
{
	vec3_t playerMins = {-15, -15, /*DEFAULT_MINS_2*/-8};
	vec3_t playerMaxs = {15, 15, DEFAULT_MAXS_2};
	vec3_t from, to;
	vec3_t dirAng, dirDif;
	vec3_t forward, right;
	trace_t tr;

	if (bs->cur_ps.groundEntityNum == ENTITYNUM_NONE)
	{ //don't do this in the air, it can be.. dangerous.
		return 0;
	}

	VectorSubtract(traceto, bs->origin, dirAng);
	VectorNormalize(dirAng);
	vectoangles(dirAng, dirAng);

	if (AngleDifference(bs->viewangles[YAW], dirAng[YAW]) > 60 ||
		AngleDifference(bs->viewangles[YAW], dirAng[YAW]) < -60)
	{ //If we aren't facing the direction we're going here, then we've got enough excuse to be too stupid to strafe around anyway
		return 0;
	}

	VectorCopy(bs->origin, from);
	VectorCopy(traceto, to);

	VectorSubtract(to, from, dirDif);
	VectorNormalize(dirDif);
	vectoangles(dirDif, dirDif);

	AngleVectors(dirDif, forward, 0, 0);

	to[0] = from[0] + forward[0]*32;
	to[1] = from[1] + forward[1]*32;
	to[2] = from[2] + forward[2]*32;

	trap_Trace(&tr, from, playerMins, playerMaxs, to, bs->client, MASK_PLAYERSOLID);

	if (tr.fraction == 1)
	{
		return 0;
	}

	AngleVectors(dirAng, 0, right, 0);

	from[0] += right[0]*32;
	from[1] += right[1]*32;
	from[2] += right[2]*16;

	to[0] += right[0]*32;
	to[1] += right[1]*32;
	to[2] += right[2]*32;

	trap_Trace(&tr, from, playerMins, playerMaxs, to, bs->client, MASK_PLAYERSOLID);

	if (tr.fraction == 1)
	{
		return STRAFEAROUND_RIGHT;
	}

	from[0] -= right[0]*64;
	from[1] -= right[1]*64;
	from[2] -= right[2]*64;

	to[0] -= right[0]*64;
	to[1] -= right[1]*64;
	to[2] -= right[2]*64;

	trap_Trace(&tr, from, playerMins, playerMaxs, to, bs->client, MASK_PLAYERSOLID);

	if (tr.fraction == 1)
	{
		return STRAFEAROUND_LEFT;
	}

	return 0;
}
#endif

//Similar to the trace check, but we want to trace to see
//if there's anything we can jump over.
int BotTrace_Jump(bot_state_t *bs, vec3_t traceto)
{
	vec3_t mins, maxs, a, fwd, traceto_mod, tracefrom_mod;
	gclient_t *client = g_entities[bs->cur_ps.clientNum].client;
	trace_t tr;
	int orTr;

	VectorSubtract(traceto, bs->origin, a);
	vectoangles(a, a);

	AngleVectors(a, fwd, NULL, NULL);

	traceto_mod[0] = bs->origin[0] + fwd[0]*4;
	traceto_mod[1] = bs->origin[1] + fwd[1]*4;
	traceto_mod[2] = bs->origin[2] + fwd[2]*4;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -18;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 32;

	trap_Trace(&tr, bs->origin, mins, maxs, traceto_mod, bs->client, MASK_PLAYERSOLID);

	if (tr.fraction == 1)
	{
		return 0;
	}

	orTr = tr.entityNum;

	VectorCopy(bs->origin, tracefrom_mod);

	tracefrom_mod[2] += 41;
	traceto_mod[2] += 41;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = 0;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 8;

	trap_Trace(&tr, tracefrom_mod, mins, maxs, traceto_mod, bs->client, MASK_PLAYERSOLID);

	if (tr.fraction == 1)
	{
				if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
		{
			client->jetPackOn = qtrue;
				client->ps.pm_type = PM_JETPACK;
				bs->cur_ps.eFlags |= EF_JETPACK;
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
			bs->jumpHoldTime = ((bs->forceJumpChargeTime - level.time)/2) + level.time;
		}
		if (orTr >= 0 && orTr < MAX_CLIENTS && botstates[orTr] && botstates[orTr]->jumpTime > level.time)
		{
			return 0; //so bots don't try to jump over each other at the same time
		}

		if (bs->currentEnemy && bs->currentEnemy->s.number == orTr && (BotGetWeaponRange(bs) == BWEAPONRANGE_SABER || BotGetWeaponRange(bs) == BWEAPONRANGE_MELEE))
		{
			return 0;
		}

		return 1;
	}

	return 0;
}

//And yet another check to duck under any obstacles.
int BotTrace_Duck(bot_state_t *bs, vec3_t traceto)
{
	vec3_t mins, maxs, a, fwd, traceto_mod, tracefrom_mod;
	trace_t tr;

	VectorSubtract(traceto, bs->origin, a);
	vectoangles(a, a);

	AngleVectors(a, fwd, NULL, NULL);

	traceto_mod[0] = bs->origin[0] + fwd[0]*4;
	traceto_mod[1] = bs->origin[1] + fwd[1]*4;
	traceto_mod[2] = bs->origin[2] + fwd[2]*4;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -23;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 8;

	trap_Trace(&tr, bs->origin, mins, maxs, traceto_mod, bs->client, MASK_PLAYERSOLID);

	if (tr.fraction != 1)
	{
		return 0;
	}

	VectorCopy(bs->origin, tracefrom_mod);

	tracefrom_mod[2] += 31;//33;
	traceto_mod[2] += 31;//33;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = 0;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 32;

	trap_Trace(&tr, tracefrom_mod, mins, maxs, traceto_mod, bs->client, MASK_PLAYERSOLID);

	if (tr.fraction != 1)
	{
		return 1;
	}

	return 0;
}


//[BotTweaks]
qboolean G_ThereIsAMaster(void);
//[/BotTweaks]

//[JediMasterBotAI]
static qboolean Bot_JediMasterAIEnabled(void)
{
	return (g_gametype.integer == GT_JEDIMASTER && g_jediMasterBotAI.integer);
}

static qboolean Bot_JediMasterShouldIgnoreNonMaster(bot_state_t *bs, gentity_t *en, float dist)
{
	if (!Bot_JediMasterAIEnabled() || !bs || !en || !en->client)
	{
		return qfalse;
	}

	if (bs->cur_ps.isJediMaster || en->client->ps.isJediMaster || !G_ThereIsAMaster())
	{
		return qfalse;
	}

	// Non-master bots should keep pressure on the Master.  Still allow retaliation
	// or very close self-defense so they do not ignore immediate threats.
	if ((bs->lastHurt && bs->lastHurt == en) || (bs->revengeEnemy && bs->revengeEnemy == en))
	{
		return qfalse;
	}

	return (dist > 256.0f);
}

static qboolean Bot_JediMasterShouldAvoidChase(bot_state_t *bs, gentity_t *en, float dist)
{
	if (!Bot_JediMasterAIEnabled() || !bs || !en || !en->client || !bs->cur_ps.isJediMaster)
	{
		return qfalse;
	}

	// The Master should survive and force enemies to come to him instead of
	// chasing distant, unrelated targets across the map.
	if ((bs->lastHurt && bs->lastHurt == en) || (bs->revengeEnemy && bs->revengeEnemy == en))
	{
		return qfalse;
	}

	if (g_entities[bs->client].health > 0 && g_entities[bs->client].health < 60)
	{
		return (dist > 384.0f);
	}

	return (dist > 768.0f);
}
//[/JediMasterBotAI]

//[BotTargetDiversity]
static qboolean Bot_TargetDiversityApplies(void)
{
	if (!g_botTargetDiversity.integer)
	{
		return qfalse;
	}

	// Duel and Power Duel have tightly controlled opponent selection. Do not let
	// general target-diversity logic redirect bots away from their assigned duel enemy.
	if (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL)
	{
		return qfalse;
	}

	return qtrue;
}

static int Bot_NumBotsAttackingClient(int clientNum, int ignoreBot)
{
	int i;
	int count = 0;

	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		return 0;
	}

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (i == ignoreBot || !botstates[i] || !botstates[i]->inuse)
		{
			continue;
		}

		if (botstates[i]->currentEnemy && botstates[i]->currentEnemy->s.number == clientNum)
		{
			count++;
		}
		else if (botLastEnemyTarget[i] == clientNum && botLastEnemyTargetTime[i] > 0 && botLastEnemyTargetTime[i] > level.time - 3000)
		{
			count++;
		}
	}

	return count;
}

static qboolean Bot_IsHighPriorityObjectiveTarget(bot_state_t *bs, gentity_t *en)
{
	if (!bs || !en || !en->client)
	{
		return qfalse;
	}

	if (en->client->ps.isJediMaster)
	{
		return qtrue;
	}

	if (en->client->ps.powerups[PW_REDFLAG] || en->client->ps.powerups[PW_BLUEFLAG])
	{
		return qtrue;
	}

	if (bs->lastHurt && bs->lastHurt == en)
	{
		return qtrue;
	}

	return qfalse;
}

static float Bot_TargetDiversityDistanceAdjustment(bot_state_t *bs, gentity_t *en, float dist)
{
	float adjust = 0.0f;
	int attackers;
	int rounds;

	if (!Bot_TargetDiversityApplies() || !bs || !en || !en->client || en->s.number < 0 || en->s.number >= MAX_CLIENTS)
	{
		return 0.0f;
	}

	// Objective/threat targets must remain high priority. Diversity should polish
	// ordinary combat selection, not break CTF/CTY/JediMaster/Siege-like objectives.
	if (Bot_IsHighPriorityObjectiveTarget(bs, en))
	{
		return 0.0f;
	}

	attackers = Bot_NumBotsAttackingClient(en->s.number, bs->client);
	if (attackers > 0)
	{
		adjust += (float)attackers * 220.0f;
	}

	// Avoid repeatedly tunneling the same target when other valid enemies exist.
	if (bs->client >= 0 && bs->client < MAX_CLIENTS && botLastEnemyTarget[bs->client] == en->s.number &&
		botLastEnemyTargetTime[bs->client] > 0 &&
		botLastEnemyTargetTime[bs->client] > level.time - 8000)
	{
		adjust += 180.0f;
	}

	// Do not waste attacks on newly spawn-protected players if alternatives exist.
	if (G_HasSpawnProtection(en))
	{
		adjust += 1200.0f;
	}

	// Mildly pressure the score leader in non-objective free/team fights.
	if (en->client->ps.persistant[PERS_SCORE] > 0)
	{
		rounds = en->client->ps.persistant[PERS_SCORE];
		if (rounds > 30)
		{
			rounds = 30;
		}
		adjust -= (float)rounds * 8.0f;
	}

	// Do not let a very distant target win only from score bias.
	if (dist > 1600.0f && adjust < 0.0f)
	{
		adjust *= 0.25f;
	}

	return adjust;
}

static float Bot_TargetDiversityThreatAdjustment(bot_state_t *bs, gentity_t *en, float dist)
{
	return -Bot_TargetDiversityDistanceAdjustment(bs, en, dist);
}
//[/BotTargetDiversity]

//check of the potential enemy is a valid one
int PassStandardEnemyChecks(bot_state_t *bs, gentity_t *en)
{
	if (!bs || !en)
	{ //shouldn't happen
		return 0;
	}

	if (!en->client )
	{ //not a client, don't care about him
		return 0;
	}
	
	if (en->client && en->client->ps.powerups[PW_CLOAKED])
	{ //not a client, don't care about him
			if(!(en->client->ps.eFlags & EF_FIRING) && !(en->client->ps.eFlags & EF_ALT_FIRING))
			{
				return 0;
			}		
	}
	
	if (en->client && (level.clients[bs->client].blindingTime > level.time || level.clients[bs->client].flashTime > level.time))
	{ //not a client, don't care about him
			if(!(en->client->ps.eFlags & EF_FIRING) && !(en->client->ps.eFlags & EF_ALT_FIRING))
			{
				return 0;
			}		
	}
	
	if (en->health < 1)
	{ //he's already dead
		return 0;
	}

	if (!en->takedamage)
	{ //a client that can't take damage?
		return 0;
	}

	//[AotCAI]
	//UNIQUEFIXME - What's with the weird saber checking?
	if ( (bs->settings.botType == BOT_AOTC || bs->settings.botType == BOT_HYBRID)
		&& en->s.weapon == WP_SABER
		//[NewGameTypes][EnhancedImpliment]
		/*&& g_gametype.integer != GT_SCENARIO &&*/
		//[/NewGameTypes]
		&& ((en->client->ps.saberHolstered >= 2 && en->client->saber[0].numBlades >= 2)
		|| (en->client->ps.saberHolstered >= 2 && en->client->saber[1].type)
		|| (en->client->ps.saberHolstered && !en->client->saber[1].type 
		&& en->client->saber[0].numBlades <= 1)) )
	{// Ignore players with a holsterred saber! AIMod.
		return 0;
	}
	//[/AotCAI]

	if (bs->doingFallback &&
		(gLevelFlags & LEVELFLAG_IGNOREINFALLBACK))
	{ //we screwed up in our nav routines somewhere and we've reverted to a fallback state to
		//try to get back on the trail. If the level specifies to ignore enemies in this state,
		//then ignore them.
		return 0;
	}

	if (en->client->ps.pm_type == PM_INTERMISSION ||
		en->client->ps.pm_type == PM_SPECTATOR ||
		en->client->sess.sessionTeam == TEAM_SPECTATOR)
	{ //don't attack spectators
		return 0;
	}

	
	//[BotTweaks] UNIQUEFIXME - wha?
	//if (!en->client->pers.connected && !en->NPC)
	if (!en->client->pers.connected && en->s.eType != ET_NPC)
	//[/BotTweaks] UNIQUEFIXME - wha?
	{ //a "zombie" client?
		return 0;
	}

	if (!en->s.solid)
	{ //shouldn't happen
		return 0;
	}

	if (bs->client == en->s.number)
	{ //don't attack yourself
		return 0;
	}

	if (OnSameTeam(&g_entities[bs->client], en))
	{ //don't attack teammates
		return 0;
	}
	
	
		//[BotTweaks] UNIQUEFIXME - wha?
	//if (en->NPC)
	//	return 1;
	//[/BotTweaks] UNIQUEFIXME - wha?

	if (en->s.number < MAX_CLIENTS && BotMindTricked(bs->client, en->s.number))
	{
		if (bs->currentEnemy && bs->currentEnemy->s.number == en->s.number)
		{ //if mindtricked by this enemy, then be less "aware" of them, even though
			//we know they're there.
			vec3_t vs;
			float vLen = 0;

			VectorSubtract(bs->origin, en->client->ps.origin, vs);
			vLen = VectorLength(vs);

			if (vLen > 64 /*&& (level.time - en->client->dangerTime) > 150*/)
			{
				return 0;
			}
		}
	}

	if (en->client->ps.duelInProgress && en->client->ps.duelIndex != bs->client)
	{ //don't attack duelists unless you're dueling them
		return 0;
	}

	if (bs->cur_ps.duelInProgress && en->s.number != bs->cur_ps.duelIndex)
	{ //ditto, the other way around
		return 0;
	}

	//[BotTweaks]
	//bots should attack other players if the JM saber has been dropped in the jedimaster gametype.
	if (g_gametype.integer == GT_JEDIMASTER && !en->client->ps.isJediMaster && !bs->cur_ps.isJediMaster && G_ThereIsAMaster())
	//if (g_gametype.integer == GT_JEDIMASTER && !en->client->ps.isJediMaster && !bs->cur_ps.isJediMaster)
	//[/BotTweaks]
	{ //rules for attacking non-JM in JM mode
		vec3_t vs;
		float vLen = 0;

		if (!g_friendlyFire.integer)
		{ //can't harm non-JM in JM mode if FF is off
			return 0;
		}

		VectorSubtract(bs->origin, en->client->ps.origin, vs);
		vLen = VectorLength(vs);

		if (vLen > 350)
		{
			return 0;
		}
	}

	return 1;
}

//Notifies the bot that he has taken damage from "attacker".
void BotDamageNotification(gclient_t *bot, gentity_t *attacker)
{
	bot_state_t *bs;
	bot_state_t *bs_a;
	int i;

	if (!bot || !attacker || !attacker->client)
	{
		return;
	}

	if (bot->ps.clientNum >= MAX_CLIENTS)
	{ //an NPC.. do nothing for them.
		return;
	}
	if (OnSameTeam(&g_entities[bot->ps.clientNum], attacker))
	{
		return;
	}

	

	bs_a = (attacker->s.number < MAX_CLIENTS) ? botstates[attacker->s.number] : NULL;

	if (bs_a)
	{ //if the client attacking us is a bot as well
		bs_a->lastAttacked = &g_entities[bot->ps.clientNum];
		i = 0;

		while (i < MAX_CLIENTS)
		{
			if (botstates[i] &&
				i != bs_a->client &&
				botstates[i]->lastAttacked == &g_entities[bot->ps.clientNum])
			{
				botstates[i]->lastAttacked = NULL;
			}

			i++;
		}
	}
	else //got attacked by a real client, so no one gets rights to lastAttacked
	{
		i = 0;

		while (i < MAX_CLIENTS)
		{
			if (botstates[i] &&
				botstates[i]->lastAttacked == &g_entities[bot->ps.clientNum])
			{
				botstates[i]->lastAttacked = NULL;
			}

			i++;
		}
	}

	bs = botstates[bot->ps.clientNum];

	if (!bs)
	{
		return;
	}

	bs->lastHurt = attacker;
	// Record time separately; lastHurt is a pointer.
	if (bs->client >= 0 && bs->client < MAX_CLIENTS)
	{
		botLastHurtTime[bs->client] = level.time;
	}

	if (bs->currentEnemy)
	{ //we don't care about the guy attacking us if we have an enemy already
		return;
	}

	if (!PassStandardEnemyChecks(bs, attacker))
	{ //the person that hurt us is not a valid enemy
		return;
	}

	if (PassLovedOneCheck(bs, attacker))
	{ //the person that hurt us is the one we love!
		bs->currentEnemy = attacker;
		bs->enemySeenTime = level.time + ENEMY_FORGET_MS;
	}
}

//perform cheap "hearing" checks based on the event catching
//system
int BotCanHear(bot_state_t *bs, gentity_t *en, float endist)
{
	float minlen;

	if (!en || !en->client)
	{
		return 0;
	}

	// gBotEventTracker is client-indexed; NPC entity numbers are out of range
	if (en->s.number >= MAX_CLIENTS)
	{
		return 0;
	}

	if (en && en->client && en->client->ps.otherSoundTime > level.time)
	{ //they made a noise in recent time
		minlen = en->client->ps.otherSoundLen;
		goto checkStep;
	}

	if (en && en->client && en->client->ps.footstepTime > level.time)
	{ //they made a footstep
		minlen = 256;
		goto checkStep;
	}

	if (gBotEventTracker[en->s.number].eventTime < level.time)
	{ //no recent events to check
		return 0;
	}

	switch(gBotEventTracker[en->s.number].events[gBotEventTracker[en->s.number].eventSequence & (MAX_PS_EVENTS-1)])
	{ //did the last event contain a sound?
	case EV_GLOBAL_SOUND:
		minlen = 256;
		break;
	case EV_FIRE_WEAPON:
	//[NewWeapons][EnhancedImpliment]
	//case EV_FIRE_WEAPON2:
	//[/NewWeapons]
	case EV_ALT_FIRE:
	case EV_SABER_ATTACK:
		minlen = 512;
		break;
	case EV_STEP_4:
	case EV_STEP_8:
	case EV_STEP_12:
	case EV_STEP_16:
	case EV_FOOTSTEP:
	case EV_FOOTSTEP_METAL:
	case EV_FOOTWADE:
		minlen = 256;
		break;
	case EV_JUMP:
	case EV_ROLL:
		minlen = 256;
		break;
	default:
		minlen = 999999;
		break;
	}
checkStep:
	if (en->s.number < MAX_CLIENTS && BotMindTricked(bs->client, en->s.number))
	{ //if mindtricked by this person, cut down on the minlen so they can't "hear" as well
		minlen /= 4;
	}

	if (endist <= minlen)
	{ //we heard it
		return 1;
	}

	return 0;
}

//check for new events
void UpdateEventTracker(void)
{
	int i;

	i = 0;

	while (i < MAX_CLIENTS)
	{
		if (gBotEventTracker[i].eventSequence != level.clients[i].ps.eventSequence)
		{ //updated event
			gBotEventTracker[i].eventSequence = level.clients[i].ps.eventSequence;
			gBotEventTracker[i].events[0] = level.clients[i].ps.events[0];
			gBotEventTracker[i].events[1] = level.clients[i].ps.events[1];
			gBotEventTracker[i].eventTime = level.time + 0.5;
		}

		i++;
	}
}

//check if said angles are within our fov
int InFieldOfVision(vec3_t viewangles, float fov, vec3_t angles)
{
	int i;
	float diff, angle;

	for (i = 0; i < 2; i++)
	{
		angle = AngleMod(viewangles[i]);
		angles[i] = AngleMod(angles[i]);
		diff = angles[i] - angle;
		if (angles[i] > angle)
		{
			if (diff > 180.0)
			{
				diff -= 360.0;
			}
		}
		else
		{
			if (diff < -180.0)
			{
				diff += 360.0;
			}
		}
		if (diff > 0)
		{
			if (diff > fov * 0.5)
			{
				return 0;
			}
		}
		else
		{
			if (diff < -fov * 0.5)
			{
				return 0;
			}
		}
	}
	return 1;
}

//We cannot hurt the ones we love. Unless of course this
//function says we can.
int PassLovedOneCheck(bot_state_t *bs, gentity_t *ent)
{
	int i;
	bot_state_t *loved;

	// Safety: This function only applies to bot-vs-bot relationships.
	// It is sometimes called with NPC entities (e.g. when bots are attacked by NPCs),
	// and botstates[] is only sized for MAX_CLIENTS.
	if (!ent || ent->s.number < 0 || ent->s.number >= MAX_CLIENTS)
	{
		return 1;
	}

	if (!bs->lovednum)
	{
		return 1;
	}

	if (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL|| g_gametype.integer == GT_FFA)
	{ //There is no love in 1-on-1
		return 1;
	}

	i = 0;

	if (!botstates[ent->s.number])
	{ //not a bot
		return 1;
	}

	if (!bot_attachments.integer)
	{
		return 1;
	}

	loved = botstates[ent->s.number];

	while (i < bs->lovednum)
	{
		if (strcmp(level.clients[loved->client].pers.netname, bs->loved[i].name) == 0)
		{
			if (!IsTeamplay() && bs->loved[i].level < 2)
			{ //if FFA and level of love is not greater than 1, just don't care
				return 1;
			}
			else if (IsTeamplay() && !OnSameTeam(&g_entities[bs->client], &g_entities[loved->client]) && bs->loved[i].level < 2)
			{ //is teamplay, but not on same team and level < 2
				return 1;
			}
			else
			{
				return 0;
			}
		}

		i++;
	}

	return 1;
}

//[BotTweaks]
//moved this up so we can use it earlier in PassStandardEnemyChecks to make the bots attack other players while there isn't
//a jedimaster in the JM gametype.
//qboolean G_ThereIsAMaster(void);
//[/BotTweaks]

//standard check to find a new enemy.
int ScanForEnemies(bot_state_t *bs)
{
    vec3_t a;
    float distcheck;
    float closest;
    int bestindex;
    int i;
    float hasEnemyDist = 0.0f;
    float adjustedDist = 0.0f;
    qboolean noAttackNonJM = qfalse;
    qboolean ctfCarrierAvoidChase = qfalse;
    int ctfCarrierOwnFlag = 0;
    float ctfCarrierThreatRadius = 512.0f;

    // Style scoring (only used for non-default bot types)
    float bestScore = -999999.0f;

    closest = 999999.0f;
    bestindex = -1;
    i = 0;

    if (bs->currentEnemy)
    { // only switch to a new enemy if he's significantly closer (BOT_DEFAULT)
        hasEnemyDist = bs->frame_Enemy_Len;
    }

    if (bs->currentEnemy && bs->currentEnemy->client && bs->currentEnemy->client->ps.isJediMaster)
    { // The Jedi Master must die.
        return -1;
    }

    if (g_gametype.integer == GT_JEDIMASTER)
    {
        if (G_ThereIsAMaster() && !bs->cur_ps.isJediMaster)
        {
            if (!g_friendlyFire.integer)
            {
                noAttackNonJM = qtrue;
            }
            else
            {
                closest = 128.0f; // only get mad if people get close enough or hurt you
            }
        }
    }

    if (((g_ctfBotCarrierAI.integer && g_gametype.integer == GT_CTF) ||
         (g_ctyBotCarrierAI.integer && g_gametype.integer == GT_CTY)) &&
        bs->client >= 0 && bs->client < MAX_CLIENTS && g_entities[bs->client].client)
    {
        if (g_entities[bs->client].client->sess.sessionTeam == TEAM_RED &&
            bs->cur_ps.powerups[PW_BLUEFLAG])
        {
            ctfCarrierAvoidChase = qtrue;
            ctfCarrierOwnFlag = PW_REDFLAG;
        }
        else if (g_entities[bs->client].client->sess.sessionTeam == TEAM_BLUE &&
            bs->cur_ps.powerups[PW_REDFLAG])
        {
            ctfCarrierAvoidChase = qtrue;
            ctfCarrierOwnFlag = PW_BLUEFLAG;
        }

        if (ctfCarrierAvoidChase && g_entities[bs->client].health > 0 &&
            g_entities[bs->client].health < 50)
        {
            ctfCarrierThreatRadius = 384.0f;
        }
    }

    while (i < MAX_CLIENTS)
    {
        if (i != bs->client && g_entities[i].client &&
            !OnSameTeam(&g_entities[bs->client], &g_entities[i]) &&
            PassStandardEnemyChecks(bs, &g_entities[i]) &&
            BotPVSCheck(g_entities[i].client->ps.origin, bs->eye) &&
            PassLovedOneCheck(bs, &g_entities[i]))
        {
            VectorSubtract(g_entities[i].client->ps.origin, bs->eye, a);
            distcheck = VectorLength(a);

            if (Bot_JediMasterShouldIgnoreNonMaster(bs, &g_entities[i], distcheck) ||
                Bot_JediMasterShouldAvoidChase(bs, &g_entities[i], distcheck))
            {
                i++;
                continue;
            }

            // CTF/CTY carriers should get home instead of chasing every visible enemy.
            // Still allow close threats and the enemy carrying our objective to be targeted.
            if (ctfCarrierAvoidChase && distcheck > ctfCarrierThreatRadius &&
                (!ctfCarrierOwnFlag || !g_entities[i].client->ps.powerups[ctfCarrierOwnFlag]))
            {
                i++;
                continue;
            }

            vectoangles(a, a);

            if (g_entities[i].client->ps.isJediMaster)
            {
                // make us think the Jedi Master is close so we'll attack him above all
                distcheck = 1.0f;
            }

            // Must be in FOV/hearing and visible
            if (((InFieldOfVision(bs->viewangles, 90, a) && !BotMindTricked(bs->client, i)) ||
                 BotCanHear(bs, &g_entities[i], distcheck)) &&
                OrgVisible(bs->eye, g_entities[i].client->ps.origin, -1))
            {
                // Preserve classic behaviour for BOT_DEFAULT
                if (bs->settings.botType == BOT_DEFAULT)
                {
                    adjustedDist = distcheck + Bot_TargetDiversityDistanceAdjustment(bs, &g_entities[i], distcheck);
                    if (adjustedDist < closest)
                    {
                        if (BotMindTricked(bs->client, i))
                        {
                            if (distcheck < 256.0f || (level.time - g_entities[i].client->dangerTime) < 100)
                            {
                                if (!hasEnemyDist || distcheck < (hasEnemyDist - 128.0f))
                                {
                                    if (!noAttackNonJM || g_entities[i].client->ps.isJediMaster)
                                    {
                                        closest = adjustedDist;
                                        bestindex = i;
                                    }
                                }
                            }
                        }
                        else
                        {
                            if (!hasEnemyDist || distcheck < (hasEnemyDist - 128.0f))
                            {
                                if (!noAttackNonJM || g_entities[i].client->ps.isJediMaster)
                                {
                                    closest = adjustedDist;
                                    bestindex = i;
                                }
                            }
                        }
                    }
                }
                else
                {
                    // Non-default: threat scoring
                    float threat = 0.0f;
                    int curEnemyNum = (bs->currentEnemy ? bs->currentEnemy->s.number : -1);

                    // Base: closer is better, but not the only factor
                    threat += (1400.0f - distcheck);

                    // Don't ignore the Jedi Master
                    if (g_entities[i].client->ps.isJediMaster)
                        threat += 3000.0f;

                    // Objective pressure: flag carrier is high priority
                    if (g_entities[i].client->ps.powerups[PW_REDFLAG] || g_entities[i].client->ps.powerups[PW_BLUEFLAG])
                        threat += 1400.0f;

                    // Retaliation / threat memory
                    if (bs->lastHurt && bs->lastHurt->s.number == i)
                        threat += 650.0f;
                    if (bs->revengeEnemy && bs->revengeEnemy->s.number == i)
                        threat += 550.0f;

                    // Finish off weak enemies a bit more often
                    if (g_entities[i].health > 0)
                        threat += (100.0f - (float)g_entities[i].health) * 2.0f;

                    // Mild bias to keep current target (reduces flip-flop)
                    if (i == curEnemyNum)
                        threat += 200.0f;

                    // Jedi Master rules: if we are forbidden from attacking non-JM, keep that
                    if (noAttackNonJM && !g_entities[i].client->ps.isJediMaster)
                        threat -= 999999.0f;

                    if (Bot_JediMasterAIEnabled() && bs->cur_ps.isJediMaster &&
                        distcheck > 512.0f &&
                        !(bs->lastHurt && bs->lastHurt->s.number == i) &&
                        !(bs->revengeEnemy && bs->revengeEnemy->s.number == i))
                    {
                        threat -= 900.0f;
                    }

                    threat += Bot_TargetDiversityThreatAdjustment(bs, &g_entities[i], distcheck);

                    // Small hysteresis to avoid rapid switching
                    if (threat > bestScore + 60.0f)
                    {
                        bestScore = threat;
                        bestindex = i;
                        closest = distcheck;
                    }
                }
            }
        }
        i++;
    }

    // --- Extended scan: allow bots to consider NPC entities (including vehicles) as enemies ---
    i = MAX_CLIENTS;
    while (i < level.num_entities)
    {
        gentity_t *nent = &g_entities[i];

        if (!nent->inuse || nent->s.eType != ET_NPC || !nent->client)
        {
            i++;
            continue;
        }

        // don't consider self
        if (i == bs->client)
        {
            i++;
            continue;
        }

        // team / validity checks (OnSameTeam handles vehicles via pilot)
        if (OnSameTeam(&g_entities[bs->client], nent) ||
            !PassStandardEnemyChecks(bs, nent) ||
            !BotPVSCheck(nent->client->ps.origin, bs->eye))
        {
            i++;
            continue;
        }

        // NOTE: don't call PassLovedOneCheck/BotCanHear here (they assume client indices).
        VectorSubtract(nent->client->ps.origin, bs->eye, a);
        distcheck = VectorLength(a);
        vectoangles(a, a);

        // Must be in FOV and visible
        if (InFieldOfVision(bs->viewangles, 90, a) &&
            OrgVisible(bs->eye, nent->client->ps.origin, -1))
        {
            if (bs->settings.botType == BOT_DEFAULT)
            {
                if (distcheck < closest)
                {
                    closest = distcheck;
                    bestindex = i;
                }
            }
            else
            {
                // Keep the same scoring behaviour as the client scan for non-default bot types:
                float score = 0.0f;

                // Prefer closer targets
                score += (99999.0f - distcheck);

                // Prefer actively fighting targets
                if (nent->client->ps.weaponTime > 0 ||
                    (nent->client->ps.eFlags & (EF_FIRING | EF_ALT_FIRING)))
                {
                    score += 500.0f;
                }

                if (score > bestScore)
                {
                    bestScore = score;
                    bestindex = i;
                }
            }
        }

        i++;
    }

    if (bs->client >= 0 && bs->client < MAX_CLIENTS && bestindex >= 0 && bestindex < MAX_CLIENTS)
    {
        botLastEnemyTarget[bs->client] = bestindex;
        botLastEnemyTargetTime[bs->client] = level.time;
    }

    return bestindex;
}

int WaitingForNow(bot_state_t *bs, vec3_t goalpos)
{ //checks if the bot is doing something along the lines of waiting for an elevator to raise up
	vec3_t xybot, xywp, a;

	if (!bs->wpCurrent)
	{
		return 0;
	}

	if ((int)goalpos[0] != (int)bs->wpCurrent->origin[0] ||
		(int)goalpos[1] != (int)bs->wpCurrent->origin[1] ||
		(int)goalpos[2] != (int)bs->wpCurrent->origin[2])
	{
		return 0;
	}

	VectorCopy(bs->origin, xybot);
	VectorCopy(bs->wpCurrent->origin, xywp);

	xybot[2] = 0;
	xywp[2] = 0;

	VectorSubtract(xybot, xywp, a);

	if (VectorLength(a) < 16 && bs->frame_Waypoint_Len > 100)
	{
		if (CheckForFunc(bs->origin, bs->client))
		{
			return 1; //we're probably standing on an elevator and riding up/down. Or at least we hope so.
		}
	}
	else if (VectorLength(a) < 64 && bs->frame_Waypoint_Len > 64 &&
		CheckForFunc(bs->origin, bs->client))
	{
		bs->noUseTime = level.time + 2000;
	}

	return 0;
}

//get an ideal distance for us to be at in relation to our opponent
//based on our weapon.
int BotGetWeaponRange(bot_state_t *bs)
{
	switch (bs->cur_ps.weapon)
	{
	case WP_STUN_BATON:
		return BWEAPONRANGE_MELEE;			   
	case WP_MELEE:
		return BWEAPONRANGE_MELEE;
	case WP_SABER:
	//[NewWeapons][EnhancedImpliment]
	/*
	case WP_SITH_SCEPTER:
	case WP_TUSKEN_STAFF:
	case WP_NOGHRI_STICK:
	case WP_VIBROBLADE2:
	case WP_FORCE_PIKE:
	case WP_AMPHI_STAFF:
	case WP_VIBROAXE2:
	case WP_SABER6:
	case WP_VIBROAXE:
	case WP_VIBRODAGGER:
	case WP_VIBROBLADE:
	case WP_SITH_SWORD:
	case WP_STAFF1:
	*/
	//[/NewWeapons][/EnhancedImpliment]
		return BWEAPONRANGE_SABER;
	case WP_BRYAR_PISTOL:
		return BWEAPONRANGE_MID;
	case WP_BLASTER:
		return BWEAPONRANGE_MID;
	case WP_DISRUPTOR:
	//[NewWeapons][EnhancedImpliment]
	//case WP_ADV_DISRUPTOR:
		return BWEAPONRANGE_LONG;
	case WP_BOWCASTER:
		   
																		   
  
						  
  
	 
  
		return BWEAPONRANGE_LONG;
  
							
			
	case WP_REPEATER:
		return BWEAPONRANGE_LONG;
	case WP_DEMP2:
		   
																		   
  
		return BWEAPONRANGE_MID;
  
	 
  
						   
  
							
			
	case WP_FLECHETTE:
	   
																		   
  
						  
  
	 
  
		return BWEAPONRANGE_LONG;
  
	case WP_CONCUSSION:
		return BWEAPONRANGE_LONG;							   
	case WP_ROCKET_LAUNCHER:
		return BWEAPONRANGE_LONG;
	case WP_THERMAL:
	//[AotCAI]
																		   
  
		return BWEAPONRANGE_MID;
  
	 
  
						   
  
							
			
	case WP_TRIP_MINE:
																	   
  
						   
  
	 
  
		return BWEAPONRANGE_MID;
  
						   
			

								  
					   
							
									
	case WP_DET_PACK:
		   
																		   
  
		return BWEAPONRANGE_MELEE;
	case WP_BRYAR_OLD:
	 
  
						   
  
		return BWEAPONRANGE_MID;
			
	default:
		return BWEAPONRANGE_MID;
	}
}

//see if we want to run away from the opponent for whatever reason
int BotIsAChickenWuss(bot_state_t *bs)
{
	int bWRange;

	if (gLevelFlags & LEVELFLAG_IMUSTNTRUNAWAY)
	{ //The level says we mustn't run away!
		return 0;
	}

	if (g_gametype.integer == GT_SINGLE_PLAYER)
	{ //"coop" (not really)
		return 0;
	}

	if (Bot_JediMasterAIEnabled() && bs->cur_ps.isJediMaster &&
		g_entities[bs->client].health > 0 && g_entities[bs->client].health < 60)
	{ //A low-health bot Master should value survival instead of overcommitting.
		return 1;
	}

	if (g_gametype.integer == GT_JEDIMASTER && !bs->cur_ps.isJediMaster)
	{ //Then you may know no fear.
		//Well, unless he's strong.
		if (bs->currentEnemy && bs->currentEnemy->client &&
			bs->currentEnemy->client->ps.isJediMaster &&
			bs->currentEnemy->health > 40 &&
			bs->cur_ps.weapon < WP_ROCKET_LAUNCHER)
		{ //explosive weapons are most effective against the Jedi Master
			goto jmPass;
		}
		return 0;
	}

	if (g_gametype.integer == GT_CTF && bs->currentEnemy && bs->currentEnemy->client)
	{
		if (bs->currentEnemy->client->ps.powerups[PW_REDFLAG] ||
			bs->currentEnemy->client->ps.powerups[PW_BLUEFLAG])
		{ //don't be afraid of flag carriers, they must die!
			return 0;
		}
	}

jmPass:
	if (bs->chickenWussCalculationTime > level.time)
	{
		return 2; //don't want to keep going between two points...
	}

	if (bs->cur_ps.fd.forcePowersActive & (1 << FP_RAGE))
	{ //don't run while raging
		return 0;
	}

	if (g_gametype.integer == GT_JEDIMASTER && !bs->cur_ps.isJediMaster)
	{ //be frightened of the jedi master? I guess in this case.
		return 1;
	}

	bs->chickenWussCalculationTime = level.time + MAX_CHICKENWUSS_TIME;

	if (g_entities[bs->client].health < BOT_RUN_HEALTH)
	{ //we're low on health, let's get away
		return 1;
	}

	bWRange = BotGetWeaponRange(bs);

	if (bWRange == BWEAPONRANGE_MELEE || bWRange == BWEAPONRANGE_SABER)
	{
		if (bWRange != BWEAPONRANGE_SABER || !bs->saberSpecialist)
		{ //run away if we're using melee, or if we're using a saber and not a "saber specialist"
			return 1;
		}
	}

//	if (bs->cur_ps.weapon == WP_BRYAR_PISTOL)
//	{ //the bryar is a weak weapon, so just try to find a new one if it's what you're having to use
//		return 1;
//	}

	if (bs->currentEnemy && bs->currentEnemy->client &&
		bs->currentEnemy->client->ps.weapon == WP_SABER &&
		bs->frame_Enemy_Len < 512 && bs->cur_ps.weapon != WP_SABER)
	{ //if close to an enemy with a saber and not using a saber, then try to back off
		return 1;
	}

	if ((level.time-bs->cur_ps.electrifyTime) < 16000)
	{ //lightning is dangerous.
		return 1;
	}

	//didn't run, reset the timer
	bs->chickenWussCalculationTime = 0;

	return 0;
}

//look for "bad things". bad things include detpacks, thermal detonators,
//and other dangerous explodey items.
gentity_t *GetNearestBadThing(bot_state_t *bs)
{
	int i = 0;
	float glen;
	vec3_t hold;
	int bestindex = 0;
	float bestdist = 800; //if not within a radius of 800, it's no threat anyway
	int foundindex = 0;
	float factor = 0;
	gentity_t *ent;
	trace_t tr;

	while (i < level.num_entities)
	{
		ent = &g_entities[i];

		if ( (ent &&
			!ent->client &&
			ent->inuse &&
			ent->damage &&
			/*(ent->s.weapon == WP_THERMAL || ent->s.weapon == WP_FLECHETTE)*/
			ent->s.weapon &&
			ent->splashDamage) ||
			(ent &&
			ent->genericValue5 == 1000 &&
			ent->inuse &&
			ent->health > 0 &&
			ent->genericValue3 != bs->client &&
			g_entities[ent->genericValue3].client && !OnSameTeam(&g_entities[bs->client], &g_entities[ent->genericValue3])) )
		{ //try to escape from anything with a non-0 s.weapon and non-0 damage. This hopefully only means dangerous projectiles.
		  //Or a sentry gun if bolt_Head == 1000. This is a terrible hack, yes.
			VectorSubtract(bs->origin, ent->r.currentOrigin, hold);
			glen = VectorLength(hold);

			if (ent->s.weapon != WP_THERMAL && ent->s.weapon != WP_FLECHETTE &&
				//[NewWeapons][EnhancedImpliment]
				//ent->s.weapon != WP_DET_PACK && ent->s.weapon != WP_TRIP_MINE && ent->s.weapon != WP_TRIP_MINE_2)
				ent->s.weapon != WP_DET_PACK && ent->s.weapon != WP_TRIP_MINE)
				//[/NewWeapons][EnhancedImpliment]
			{
				factor = 0.5;

				if (ent->s.weapon && glen <= 256 && bs->settings.skill > 2)
				{ //it's a projectile so push it away
					bs->doForcePush = level.time + 700;
					//G_Printf("PUSH PROJECTILE\n");
				}
			}
			else
			{
				factor = 1;
			}

			if (ent->s.weapon == WP_ROCKET_LAUNCHER &&
				(ent->r.ownerNum == bs->client ||
				(ent->r.ownerNum > 0 && ent->r.ownerNum < MAX_CLIENTS &&
				g_entities[ent->r.ownerNum].client && OnSameTeam(&g_entities[bs->client], &g_entities[ent->r.ownerNum]))) )
			{ //don't be afraid of your own rockets or your teammates' rockets
				factor = 0;
			}

			if (ent->s.weapon == WP_DET_PACK &&
				(ent->r.ownerNum == bs->client ||
				(ent->r.ownerNum > 0 && ent->r.ownerNum < MAX_CLIENTS &&
				g_entities[ent->r.ownerNum].client && OnSameTeam(&g_entities[bs->client], &g_entities[ent->r.ownerNum]))) )
			{ //don't be afraid of your own detpacks or your teammates' detpacks
				factor = 0;
			}

			if (ent->s.weapon == WP_TRIP_MINE &&
				(ent->r.ownerNum == bs->client ||
				(ent->r.ownerNum > 0 && ent->r.ownerNum < MAX_CLIENTS &&
				g_entities[ent->r.ownerNum].client && OnSameTeam(&g_entities[bs->client], &g_entities[ent->r.ownerNum]))) )
			{ //don't be afraid of your own trip mines or your teammates' trip mines
				factor = 0;
			}

			if (ent->s.weapon == WP_THERMAL &&
				(ent->r.ownerNum == bs->client ||
				(ent->r.ownerNum > 0 && ent->r.ownerNum < MAX_CLIENTS &&
				g_entities[ent->r.ownerNum].client && OnSameTeam(&g_entities[bs->client], &g_entities[ent->r.ownerNum]))) )
			{ //don't be afraid of your own thermals or your teammates' thermals
				factor = 0;
			}
			if (glen < bestdist*factor && BotPVSCheck(bs->origin, ent->s.pos.trBase))
			{
				trap_Trace(&tr, bs->origin, NULL, NULL, ent->s.pos.trBase, bs->client, MASK_SOLID);

				if (tr.fraction == 1 || tr.entityNum == ent->s.number)
				{
					bestindex = i;
					bestdist = glen;
					foundindex = 1;
				}
			}
		}

		if (ent && !ent->client && ent->inuse && ent->damage && ent->s.weapon && ent->r.ownerNum < MAX_CLIENTS && ent->r.ownerNum >= 0)
		{ //if we're in danger of a projectile belonging to someone and don't have an enemy, set the enemy to them
			gentity_t *projOwner = &g_entities[ent->r.ownerNum];

			if (projOwner && projOwner->inuse && projOwner->client)
			{
				if (!bs->currentEnemy)
				{
					if (PassStandardEnemyChecks(bs, projOwner))
					{
						if (PassLovedOneCheck(bs, projOwner))
						{
							VectorSubtract(bs->origin, ent->r.currentOrigin, hold);
							glen = VectorLength(hold);

							if (glen < 512)
							{
								bs->currentEnemy = projOwner;
								bs->enemySeenTime = level.time + ENEMY_FORGET_MS;
							}
						}
					}
				}
			}
		}

		i++;
	}

	if (foundindex)
	{
		bs->dontGoBack = level.time + 1500;
		return &g_entities[bestindex];
	}
	else
	{
		return NULL;
	}
}

//Keep our CTF priorities on defending our team's flag
int BotDefendFlag(bot_state_t *bs)
{
	wpobject_t *flagPoint;
	vec3_t a;

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		flagPoint = flagRed;
	}
	else if (level.clients[bs->client].sess.sessionTeam == TEAM_BLUE)
	{
		flagPoint = flagBlue;
	}
	else
	{
		return 0;
	}

	if (!flagPoint)
	{
		return 0;
	}

	VectorSubtract(bs->origin, flagPoint->origin, a);

	if (VectorLength(a) > BASE_GUARD_DISTANCE)
	{
		bs->wpDestination = flagPoint;
	}

	return 1;
}

//Keep our CTF priorities on getting the other team's flag
int BotGetEnemyFlag(bot_state_t *bs)
{
	wpobject_t *flagPoint;
	vec3_t a;

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		flagPoint = flagBlue;
	}
	else if (level.clients[bs->client].sess.sessionTeam == TEAM_BLUE)
	{
		flagPoint = flagRed;
	}
	else
	{
		return 0;
	}

	if (!flagPoint)
	{
		return 0;
	}

	VectorSubtract(bs->origin, flagPoint->origin, a);

	if (VectorLength(a) > BASE_GETENEMYFLAG_DISTANCE)
	{
		bs->wpDestination = flagPoint;
	}

	return 1;
}

//Our team's flag is gone, so try to get it back
int BotGetFlagBack(bot_state_t *bs)
{
	int i = 0;
	int myFlag = 0;
	int foundCarrier = 0;
	int tempInt = 0;
	gentity_t *ent = NULL;
	gentity_t *ownDroppedFlag = NULL;
	vec3_t usethisvec;

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		myFlag = PW_REDFLAG;
		ownDroppedFlag = droppedRedFlag;
	}
	else
	{
		myFlag = PW_BLUEFLAG;
		ownDroppedFlag = droppedBlueFlag;
	}

	// Stronger CTF return priority: if our flag is dropped, return it instead
	// of falling back just because no enemy carrier currently has it.
	if (g_ctfBotReturnFlagPriority.integer && ownDroppedFlag &&
		(ownDroppedFlag->flags & FL_DROPPED_ITEM) &&
		ownDroppedFlag->classname && strcmp(ownDroppedFlag->classname, "freed") != 0)
	{
		if (bs->wpDestSwitchTime < level.time)
		{
			VectorCopy(ownDroppedFlag->s.pos.trBase, usethisvec);
			tempInt = GetNearestVisibleWP(usethisvec, 0);

			if (tempInt != -1 && TotalTrailDistance(bs->wpCurrent->index, tempInt, bs) != -1)
			{
				bs->wpDestination = gWPArray[tempInt];
				bs->wpDestSwitchTime = level.time + Q_irand(750, 2000);
			}
		}

		return 1;
	}

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->client && ent->client->ps.powerups[myFlag] && !OnSameTeam(&g_entities[bs->client], ent))
		{
			foundCarrier = 1;
			break;
		}

		i++;
	}

	if (!foundCarrier)
	{
		return 0;
	}

	if (!ent)
	{
		return 0;
	}

	if (bs->wpDestSwitchTime < level.time)
	{
		if (ent->client)
		{
			VectorCopy(ent->client->ps.origin, usethisvec);
		}
		else
		{
			VectorCopy(ent->s.origin, usethisvec);
		}

		tempInt = GetNearestVisibleWP(usethisvec, 0);

		if (tempInt != -1 && TotalTrailDistance(bs->wpCurrent->index, tempInt, bs) != -1)
		{
			bs->wpDestination = gWPArray[tempInt];
			bs->wpDestSwitchTime = level.time + Q_irand(1000, 5000);
		}
	}

	return 1;
}

//Someone else on our team has the enemy flag, so try to get
//to their assistance
int BotGuardFlagCarrier(bot_state_t *bs)
{
	int i = 0;
	int enemyFlag = 0;
	int foundCarrier = 0;
	int tempInt = 0;
	float escortDistSq;
	gentity_t *ent = NULL;
	gentity_t *carrier = NULL;
	gentity_t *bestThreat = NULL;
	float bestThreatDist = 999999999.0f;
	vec3_t usethisvec;

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		enemyFlag = PW_BLUEFLAG;
	}
	else
	{
		enemyFlag = PW_REDFLAG;
	}

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->client && ent->client->ps.powerups[enemyFlag] && OnSameTeam(&g_entities[bs->client], ent))
		{
			foundCarrier = 1;
			carrier = ent;
			break;
		}

		i++;
	}

	if (!foundCarrier || !carrier || !carrier->client)
	{
		return 0;
	}

	// Escort AI: protect the carrier instead of wandering to unrelated fights.
	// Stay near the carrier, but do not stand directly on top of them.
	if (((g_ctfBotEscortAI.integer && g_gametype.integer == GT_CTF) ||
		(g_ctyBotEscortAI.integer && g_gametype.integer == GT_CTY)))
	{
		for (i = 0; i < MAX_CLIENTS; i++)
		{
			float threatDistSq;
			ent = &g_entities[i];

			if (!ent || !ent->client || ent == carrier || ent == &g_entities[bs->client])
			{
				continue;
			}

			if (OnSameTeam(carrier, ent) || !PassStandardEnemyChecks(bs, ent))
			{
				continue;
			}

			// Only break from escort duty for enemies who are threatening the carrier
			// or are close enough to the escort to matter.
			threatDistSq = DistanceSquared(ent->client->ps.origin, carrier->client->ps.origin);
			if (threatDistSq > (768.0f * 768.0f) &&
				DistanceSquared(ent->client->ps.origin, bs->origin) > (512.0f * 512.0f))
			{
				continue;
			}

			if (!BotPVSCheck(ent->client->ps.origin, bs->eye) ||
				!OrgVisible(bs->eye, ent->client->ps.origin, -1))
			{
				continue;
			}

			if (threatDistSq < bestThreatDist)
			{
				bestThreatDist = threatDistSq;
				bestThreat = ent;
			}
		}

		if (bestThreat)
		{
			bs->currentEnemy = bestThreat;
			bs->enemySeenTime = level.time + ENEMY_FORGET_MS;
		}
	}

	escortDistSq = DistanceSquared(bs->origin, carrier->client->ps.origin);
	if (bs->wpDestSwitchTime < level.time && escortDistSq > (384.0f * 384.0f))
	{
		VectorCopy(carrier->client->ps.origin, usethisvec);

		tempInt = GetNearestVisibleWP(usethisvec, 0);

		if (tempInt != -1 && bs->wpCurrent && TotalTrailDistance(bs->wpCurrent->index, tempInt, bs) != -1)
		{
			bs->wpDestination = gWPArray[tempInt];
			bs->wpDestSwitchTime = level.time + Q_irand(750, 2000);
		}
	}

	return 1;
}

//We have the flag, let's get it home.
int BotGetFlagHome(bot_state_t *bs)
{
	wpobject_t *flagPoint;
	vec3_t a;

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		flagPoint = flagRed;
	}
	else if (level.clients[bs->client].sess.sessionTeam == TEAM_BLUE)
	{
		flagPoint = flagBlue;
	}
	else
	{
		return 0;
	}

	if (!flagPoint)
	{
		return 0;
	}

	VectorSubtract(bs->origin, flagPoint->origin, a);

	if (VectorLength(a) > BASE_FLAGWAIT_DISTANCE)
	{
		bs->wpDestination = flagPoint;
	}

	return 1;
}

void GetNewFlagPoint(wpobject_t *wp, gentity_t *flagEnt, int team)
{ //get the nearest possible waypoint to the flag since it's not in its original position
	int i = 0;
	vec3_t a, mins, maxs;
	float bestdist;
	float testdist;
	int bestindex = 0;
	int foundindex = 0;
	trace_t tr;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -5;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 5;

	VectorSubtract(wp->origin, flagEnt->s.pos.trBase, a);

	bestdist = VectorLength(a);

	if (bestdist <= WP_KEEP_FLAG_DIST)
	{
		trap_Trace(&tr, wp->origin, mins, maxs, flagEnt->s.pos.trBase, flagEnt->s.number, MASK_SOLID);

		if (tr.fraction == 1)
		{ //this point is good
			return;
		}
	}

	while (i < gWPNum)
	{
		VectorSubtract(gWPArray[i]->origin, flagEnt->s.pos.trBase, a);
		testdist = VectorLength(a);

		if (testdist < bestdist)
		{
			trap_Trace(&tr, gWPArray[i]->origin, mins, maxs, flagEnt->s.pos.trBase, flagEnt->s.number, MASK_SOLID);

			if (tr.fraction == 1)
			{
				foundindex = 1;
				bestindex = i;
				bestdist = testdist;
			}
		}

		i++;
	}

	if (foundindex)
	{
		if (team == TEAM_RED)
		{
			flagRed = gWPArray[bestindex];
		}
		else
		{
			flagBlue = gWPArray[bestindex];
		}
	}
}

//See if our CTF state should take priority in our nav routines
int CTFTakesPriority(bot_state_t *bs)
{
	gentity_t *ent = NULL;
	int enemyFlag = 0;
	int myFlag = 0;
	int enemyHasOurFlag = 0;
	int weHaveEnemyFlag = 0;
	int numOnMyTeam = 0;
	int numOnEnemyTeam = 0;
	int numAttackers = 0;
	int numDefenders = 0;

	int i = 0;
	int idleWP;
	int dosw = 0;
	wpobject_t *dest_sw = NULL;
#ifdef BOT_CTF_DEBUG
	vec3_t t;

	G_Printf("CTFSTATE: %s\n", ctfStateNames[bs->ctfState]);
#endif

	if (g_gametype.integer != GT_CTF && g_gametype.integer != GT_CTY)
	{
		return 0;
	}

	if (bs->cur_ps.weapon == WP_BRYAR_PISTOL &&
		(level.time - bs->lastDeadTime) < BOT_MAX_WEAPON_GATHER_TIME)
	{ //get the nearest weapon laying around base before heading off for battle
		idleWP = GetBestIdleGoal(bs);

		if (idleWP != -1 && gWPArray[idleWP] && gWPArray[idleWP]->inuse)
		{
			if (bs->wpDestSwitchTime < level.time)
			{
				bs->wpDestination = gWPArray[idleWP];
			}
			return 1;
		}
	}
	else if (bs->cur_ps.weapon == WP_BRYAR_PISTOL &&
		(level.time - bs->lastDeadTime) < BOT_MAX_WEAPON_CHASE_CTF &&
		bs->wpDestination && bs->wpDestination->weight)
	{
		dest_sw = bs->wpDestination;
		dosw = 1;
	}

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		myFlag = PW_REDFLAG;
	}
	else
	{
		myFlag = PW_BLUEFLAG;
	}

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		enemyFlag = PW_BLUEFLAG;
	}
	else
	{
		enemyFlag = PW_REDFLAG;
	}

	if (!flagRed || !flagBlue ||
		!flagRed->inuse || !flagBlue->inuse ||
		!eFlagRed || !eFlagBlue)
	{
		return 0;
	}

#ifdef BOT_CTF_DEBUG
	VectorCopy(flagRed->origin, t);
	t[2] += 128;
	G_TestLine(flagRed->origin, t, 0x0000ff, 500);

	VectorCopy(flagBlue->origin, t);
	t[2] += 128;
	G_TestLine(flagBlue->origin, t, 0x0000ff, 500);
#endif

	if (droppedRedFlag && (droppedRedFlag->flags & FL_DROPPED_ITEM))
	{
		GetNewFlagPoint(flagRed, droppedRedFlag, TEAM_RED);
	}
	else
	{
		flagRed = oFlagRed;
	}

	if (droppedBlueFlag && (droppedBlueFlag->flags & FL_DROPPED_ITEM))
	{
		GetNewFlagPoint(flagBlue, droppedBlueFlag, TEAM_BLUE);
	}
	else
	{
		flagBlue = oFlagBlue;
	}

	if (!bs->ctfState && !(((g_gametype.integer == GT_CTF && g_ctfBotCarrierAI.integer) ||
		(g_gametype.integer == GT_CTY && g_ctyBotCarrierAI.integer)) &&
		bs->cur_ps.powerups[enemyFlag]))
	{
		return 0;
	}

	i = 0;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->client)
		{
			if (ent->client->ps.powerups[enemyFlag] && OnSameTeam(&g_entities[bs->client], ent))
			{
				weHaveEnemyFlag = 1;
			}
			else if (ent->client->ps.powerups[myFlag] && !OnSameTeam(&g_entities[bs->client], ent))
			{
				enemyHasOurFlag = 1;
			}

			if (OnSameTeam(&g_entities[bs->client], ent))
			{
				numOnMyTeam++;
			}
			else
			{
				numOnEnemyTeam++;
			}

			if (botstates[ent->s.number])
			{
				if (botstates[ent->s.number]->ctfState == CTFSTATE_ATTACKER ||
					botstates[ent->s.number]->ctfState == CTFSTATE_RETRIEVAL)
				{
					numAttackers++;
				}
				else
				{
					numDefenders++;
				}
			}
			else
			{ //assume real players to be attackers in our logic
				numAttackers++;
			}
		}
		i++;
	}

	if (bs->cur_ps.powerups[enemyFlag])
	{
		if ((g_gametype.integer == GT_CTF && g_ctfBotCarrierAI.integer) ||
			(g_gametype.integer == GT_CTY && g_ctyBotCarrierAI.integer))
		{
			// The carrier's first job is to get home.  If our objective is missing,
			// BotGetFlagHome will keep the carrier near the capture area instead
			// of sending them wandering after unrelated fights.
			bs->ctfState = CTFSTATE_GETFLAGHOME;
		}
		else if ((numOnMyTeam < 2 || !numAttackers) && enemyHasOurFlag)
		{
			bs->ctfState = CTFSTATE_RETRIEVAL;
		}
		else
		{
			bs->ctfState = CTFSTATE_GETFLAGHOME;
		}
	}
	else if (bs->ctfState == CTFSTATE_GETFLAGHOME)
	{
		bs->ctfState = 0;
	}

	if (bs->state_Forced)
	{
		bs->ctfState = bs->state_Forced;
	}

	if (bs->ctfState == CTFSTATE_DEFENDER)
	{
		if (BotDefendFlag(bs))
		{
			goto success;
		}
	}

	if (bs->ctfState == CTFSTATE_ATTACKER)
	{
		if (BotGetEnemyFlag(bs))
		{
			goto success;
		}
	}

	if (bs->ctfState == CTFSTATE_RETRIEVAL)
	{
		if (BotGetFlagBack(bs))
		{
			goto success;
		}
		else
		{ //can't find anyone on another team being a carrier, so ignore this priority
			bs->ctfState = 0;
		}
	}

	if (bs->ctfState == CTFSTATE_GUARDCARRIER)
	{
		if (BotGuardFlagCarrier(bs))
		{
			goto success;
		}
		else
		{ //can't find anyone on our team being a carrier, so ignore this priority
			bs->ctfState = 0;
		}
	}

	if (bs->ctfState == CTFSTATE_GETFLAGHOME)
	{
		if (BotGetFlagHome(bs))
		{
			goto success;
		}
	}

	return 0;

success:
	if (dosw)
	{ //allow ctf code to run, but if after a particular item then keep going after it
		bs->wpDestination = dest_sw;
	}

	return 1;
}


//RACC - Solid visual trace with two ignores
int EntityVisibleBox(vec3_t org1, vec3_t mins, vec3_t maxs, vec3_t org2, int ignore, int ignore2)
{
	trace_t tr;

	trap_Trace(&tr, org1, mins, maxs, org2, ignore, MASK_SOLID);

	if (tr.fraction == 1 && !tr.startsolid && !tr.allsolid)
	{
		return 1;
	}
	else if (tr.entityNum != ENTITYNUM_NONE && tr.entityNum == ignore2)
	{
		return 1;
	}

	return 0;
}

static qboolean Siege_ObjectiveIsUsableForBot(gentity_t *goalent)
{
	if (!goalent || !goalent->inuse || !goalent->use)
	{
		return qfalse;
	}

	if (goalent->side == SIEGETEAM_TEAM1 || goalent->side == SIEGETEAM_TEAM2)
	{
		if (goalent->objective > 0 && G_SiegeGetCompletionStatus(goalent->side, goalent->objective))
		{
			return qfalse;
		}
	}

	return qtrue;
}

static int Siege_FindClosestObjectiveWP(bot_state_t *bs, int flag)
{
	int i = 0;
	int bestindex = -1;
	float testdistance = 0;
	float bestdistance = 999999999.9f;
	gentity_t *goalent;
	vec3_t a;

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && (gWPArray[i]->flags & flag) &&
			gWPArray[i]->associated_entity != ENTITYNUM_NONE)
		{
			goalent = &g_entities[gWPArray[i]->associated_entity];

			if (Siege_ObjectiveIsUsableForBot(goalent))
			{
				VectorSubtract(gWPArray[i]->origin, bs->origin, a);
				testdistance = VectorLength(a);

				if (testdistance < bestdistance)
				{
					bestdistance = testdistance;
					bestindex = i;
				}
			}
		}

		i++;
	}

	return bestindex;
}

static int Siege_DefendClosestObjective(bot_state_t *bs, int flag)
{
	int bestindex = Siege_FindClosestObjectiveWP(bs, flag);

	if (bestindex == -1)
	{
		return 0;
	}

	bs->wpDestination = gWPArray[bestindex];
	bs->destinationGrabTime = level.time + 10000;
	bs->shootGoal = NULL;
	bs->touchGoal = NULL;

	return 1;
}

//Get the closest objective for siege and go after it
int Siege_TargetClosestObjective(bot_state_t *bs, int flag)
{
	int bestindex = -1;
	float testdistance = 0;
	gentity_t *goalent;
	vec3_t a, dif;
	vec3_t mins, maxs;

	mins[0] = -1;
	mins[1] = -1;
	mins[2] = -1;

	maxs[0] = 1;
	maxs[1] = 1;
	maxs[2] = 1;

	if ( bs->wpDestination && (bs->wpDestination->flags & flag) && bs->wpDestination->associated_entity != ENTITYNUM_NONE &&
		 Siege_ObjectiveIsUsableForBot(&g_entities[bs->wpDestination->associated_entity]) )
	{
		goto hasPoint;
	}

	bestindex = Siege_FindClosestObjectiveWP(bs, flag);

	if (bestindex != -1)
	{
		bs->wpDestination = gWPArray[bestindex];
	}
	else
	{
		return 0;
	}
hasPoint:
	goalent = &g_entities[bs->wpDestination->associated_entity];

	if (!goalent)
	{
		return 0;
	}

	VectorSubtract(bs->origin, bs->wpDestination->origin, a);

	testdistance = VectorLength(a);

	dif[0] = (goalent->r.absmax[0]+goalent->r.absmin[0])/2;
	dif[1] = (goalent->r.absmax[1]+goalent->r.absmin[1])/2;
	dif[2] = (goalent->r.absmax[2]+goalent->r.absmin[2])/2;
	//brush models can have tricky origins, so this is our hacky method of getting the center point

	if (goalent->takedamage && testdistance < BOT_MIN_SIEGE_GOAL_SHOOT &&
		EntityVisibleBox(bs->origin, mins, maxs, dif, bs->client, goalent->s.number))
	{
		bs->shootGoal = goalent;
		bs->touchGoal = NULL;
	}
	else if (goalent->use && testdistance < BOT_MIN_SIEGE_GOAL_TRAVEL)
	{
		bs->shootGoal = NULL;
		bs->touchGoal = goalent;
	}
	else
	{ //don't know how to handle this goal object!
		bs->shootGoal = NULL;
		bs->touchGoal = NULL;
	}

	if (BotGetWeaponRange(bs) == BWEAPONRANGE_MELEE ||
		BotGetWeaponRange(bs) == BWEAPONRANGE_SABER)
	{
		bs->shootGoal = NULL; //too risky
	}

	if (bs->touchGoal)
	{
		//G_Printf("Please, master, let me touch it!\n");
		VectorCopy(dif, bs->goalPosition);
	}

	return 1;
}

int Siege_DefendFromAttackers(bot_state_t *bs)
{ //this may be a little cheap, but the best way to find our defending point is probably
  //to just find the nearest person on the opposing team since they'll most likely
  //be on offense in this situation
	int wpClose = -1;
	int i = 0;
	float testdist = 999999;
	int bestindex = -1;
	float bestdist = 999999;
	gentity_t *ent;
	vec3_t a;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->client && ent->client->sess.sessionTeam != g_entities[bs->client].client->sess.sessionTeam &&
			ent->health > 0 && ent->client->sess.sessionTeam != TEAM_SPECTATOR)
		{
			VectorSubtract(ent->client->ps.origin, bs->origin, a);

			testdist = VectorLength(a);

			if (testdist < bestdist)
			{
				bestindex = i;
				bestdist = testdist;
			}
		}

		i++;
	}

	if (bestindex == -1)
	{
		return 0;
	}

	wpClose = GetNearestVisibleWP(g_entities[bestindex].client->ps.origin, -1);	

	if (wpClose != -1 && gWPArray[wpClose] && gWPArray[wpClose]->inuse)
	{
		bs->wpDestination = gWPArray[wpClose];
		bs->destinationGrabTime = level.time + 10000;
		return 1;
	}

	return 0;
}

//how many defenders on our team?
int Siege_CountDefenders(bot_state_t *bs)
{
	int i = 0;
	int num = 0;
	gentity_t *ent;
	bot_state_t *bot;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];
		bot = botstates[i];

		if (ent && ent->client && bot)
		{
			if (bot->siegeState == SIEGESTATE_DEFENDER &&
				ent->client->sess.sessionTeam == g_entities[bs->client].client->sess.sessionTeam)
			{
				num++;
			}
		}

		i++;
	}

	return num;
}

//how many other players on our team?
int Siege_CountTeammates(bot_state_t *bs)
{
	int i = 0;
	int num = 0;
	gentity_t *ent;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->client)
		{
			if (ent->client->sess.sessionTeam == g_entities[bs->client].client->sess.sessionTeam)
			{
				num++;
			}
		}

		i++;
	}

	return num;
}

//see if siege objective completion should take priority in our
//nav routines.
int SiegeTakesPriority(bot_state_t *bs)
{
	int attacker;
	int flagForDefendableObjective;
	int flagForAttackableObjective;
	int defenders, teammates;
	int idleWP;
	wpobject_t *dest_sw = NULL;
	int dosw = 0;
	gclient_t *bcl;
	vec3_t dif;
	trace_t tr;

	if (g_gametype.integer != GT_SIEGE)
	{
		return 0;
	}

	if (!g_siegeBotObjectiveAI.integer)
	{
		return 0;
	}

	bcl = g_entities[bs->client].client;

	if (!bcl)
	{
		return 0;
	}

	if (bs->cur_ps.weapon == WP_BRYAR_PISTOL &&
		(level.time - bs->lastDeadTime) < BOT_MAX_WEAPON_GATHER_TIME)
	{ //get the nearest weapon laying around base before heading off for battle
		idleWP = GetBestIdleGoal(bs);

		if (idleWP != -1 && gWPArray[idleWP] && gWPArray[idleWP]->inuse)
		{
			if (bs->wpDestSwitchTime < level.time)
			{
				bs->wpDestination = gWPArray[idleWP];
			}
			return 1;
		}
	}
	else if (bs->cur_ps.weapon == WP_BRYAR_PISTOL &&
		(level.time - bs->lastDeadTime) < BOT_MAX_WEAPON_CHASE_TIME &&
		bs->wpDestination && bs->wpDestination->weight)
	{
		dest_sw = bs->wpDestination;
		dosw = 1;
	}

	if (bcl->sess.sessionTeam == SIEGETEAM_TEAM1)
	{
		attacker = imperial_attackers;
		flagForDefendableObjective = WPFLAG_SIEGE_REBELOBJ;
		flagForAttackableObjective = WPFLAG_SIEGE_IMPERIALOBJ;
	}
	else
	{
		attacker = rebel_attackers;
		flagForDefendableObjective = WPFLAG_SIEGE_IMPERIALOBJ;
		flagForAttackableObjective = WPFLAG_SIEGE_REBELOBJ;
	}

	if (attacker)
	{
		bs->siegeState = SIEGESTATE_ATTACKER;
	}
	else
	{
		bs->siegeState = SIEGESTATE_DEFENDER;
		defenders = Siege_CountDefenders(bs);
		teammates = Siege_CountTeammates(bs);

		if (defenders > teammates/3 && teammates > 1)
		{ //devote around 1/4 of our team to completing our own side goals even if we're a defender.
		  //If we have no side goals we will realize that later on and join the defenders
			bs->siegeState = SIEGESTATE_ATTACKER;
		}
	}

	if (bs->state_Forced)
	{
		bs->siegeState = bs->state_Forced;
	}

	if (bs->siegeState == SIEGESTATE_ATTACKER)
	{
		if (!Siege_TargetClosestObjective(bs, flagForAttackableObjective))
		{ //looks like we have no goals other than to keep the other team from completing objectives
			Siege_DefendFromAttackers(bs);
			if (bs->shootGoal)
			{
				dif[0] = (bs->shootGoal->r.absmax[0]+bs->shootGoal->r.absmin[0])/2;
				dif[1] = (bs->shootGoal->r.absmax[1]+bs->shootGoal->r.absmin[1])/2;
				dif[2] = (bs->shootGoal->r.absmax[2]+bs->shootGoal->r.absmin[2])/2;
				
				if (!BotPVSCheck(bs->origin, dif))
				{
					bs->shootGoal = NULL;
				}
				else
				{
					trap_Trace(&tr, bs->origin, NULL, NULL, dif, bs->client, MASK_SOLID);

					if (tr.fraction != 1 && tr.entityNum != bs->shootGoal->s.number)
					{
						bs->shootGoal = NULL;
					}
				}
			}
		}
	}
	else if (bs->siegeState == SIEGESTATE_DEFENDER)
	{
		if (!Siege_DefendFromAttackers(bs))
		{
			Siege_DefendClosestObjective(bs, flagForDefendableObjective);
		}
		if (bs->shootGoal)
		{
			dif[0] = (bs->shootGoal->r.absmax[0]+bs->shootGoal->r.absmin[0])/2;
			dif[1] = (bs->shootGoal->r.absmax[1]+bs->shootGoal->r.absmin[1])/2;
			dif[2] = (bs->shootGoal->r.absmax[2]+bs->shootGoal->r.absmin[2])/2;
				
			if (!BotPVSCheck(bs->origin, dif))
			{
				bs->shootGoal = NULL;
			}
			else
			{
				trap_Trace(&tr, bs->origin, NULL, NULL, dif, bs->client, MASK_SOLID);

				if (tr.fraction != 1 && tr.entityNum != bs->shootGoal->s.number)
				{
					bs->shootGoal = NULL;
				}
			}
		}
	}
	else
	{ //get busy!
		Siege_TargetClosestObjective(bs, flagForAttackableObjective);
		if (bs->shootGoal)
		{
			dif[0] = (bs->shootGoal->r.absmax[0]+bs->shootGoal->r.absmin[0])/2;
			dif[1] = (bs->shootGoal->r.absmax[1]+bs->shootGoal->r.absmin[1])/2;
			dif[2] = (bs->shootGoal->r.absmax[2]+bs->shootGoal->r.absmin[2])/2;
				
			if (!BotPVSCheck(bs->origin, dif))
			{
				bs->shootGoal = NULL;
			}
			else
			{
				trap_Trace(&tr, bs->origin, NULL, NULL, dif, bs->client, MASK_SOLID);

				if (tr.fraction != 1 && tr.entityNum != bs->shootGoal->s.number)
				{
					bs->shootGoal = NULL;
				}
			}
		}
	}

	if (dosw)
	{ //allow siege objective code to run, but if after a particular item then keep going after it
		bs->wpDestination = dest_sw;
	}

	return 1;
}

//see if jedi master priorities should take priority in our nav
//routines.
int JMTakesPriority(bot_state_t *bs)
{
	int i = 0;
	int wpClose = -1;
	gentity_t *theImportantEntity = NULL;

	if (g_gametype.integer != GT_JEDIMASTER)
	{
		return 0;
	}

	if (bs->cur_ps.isJediMaster)
	{
		return 0;
	}

	//jmState becomes the index for the one who carries the saber. If jmState is -1 then the saber is currently
	//without an owner
	bs->jmState = -1;

	while (i < MAX_CLIENTS)
	{
		if (g_entities[i].client && g_entities[i].inuse &&
			g_entities[i].client->ps.isJediMaster)
		{
			bs->jmState = i;
			break;
		}

		i++;
	}

	if (bs->jmState != -1)
	{
		theImportantEntity = &g_entities[bs->jmState];
	}
	else
	{
		theImportantEntity = gJMSaberEnt;
	}

	if (theImportantEntity && theImportantEntity->inuse && bs->destinationGrabTime < level.time)
	{
		if (theImportantEntity->client)
		{
			wpClose = GetNearestVisibleWP(theImportantEntity->client->ps.origin, theImportantEntity->s.number);	
		}
		else
		{
			wpClose = GetNearestVisibleWP(theImportantEntity->r.currentOrigin, theImportantEntity->s.number);	
		}

		if (wpClose != -1 && gWPArray[wpClose] && gWPArray[wpClose]->inuse)
		{
			/*
			Com_Printf("BOT GRABBED IDEAL JM LOCATION\n");
			if (bs->wpDestination != gWPArray[wpClose])
			{
				Com_Printf("IDEAL WAS NOT ALREADY IDEAL\n");

				if (!bs->wpDestination)
				{
					Com_Printf("IDEAL WAS NULL\n");
				}
			}
			*/
			bs->wpDestination = gWPArray[wpClose];
			bs->destinationGrabTime = level.time + (g_jediMasterBotAI.integer ? 2500 : 4000);
		}
	}

	return 1;
}


//see if holocron priorities should take priority in our nav routines.
static int BotHolocronHeldCount(const playerState_t *ps)
{
	int i;
	int count = 0;

	if (!ps)
	{
		return 0;
	}

	for (i = 0; i < NUM_FORCE_POWERS; i++)
	{
		if (ps->holocronsCarried[i])
		{
			count++;
		}
	}

	return count;
}

static int BotHolocronBaseValue(int forcePower)
{
	switch (forcePower)
	{
		case FP_SPEED:
		case FP_PUSH:
		case FP_PULL:
		case FP_SABER_OFFENSE:
		case FP_SABER_DEFENSE:
		case FP_SABERTHROW:
			return 90;
		case FP_LIGHTNING:
		case FP_GRIP:
		case FP_DRAIN:
		case FP_RAGE:
			return 85;
		case FP_ABSORB:
		case FP_SEE:
		case FP_PROTECT:
			return 75;
		case FP_LEVITATION:
			return 65;
		case FP_HEAL:
		case FP_TEAM_HEAL:
		case FP_TEAM_FORCE:
		case FP_TELEPATHY:
			return 55;
		default:
			return 45;
	}
}

static int BotHolocronStyleValue(bot_state_t *bs, int forcePower)
{
	int value = BotHolocronBaseValue(forcePower);

	if (!bs)
	{
		return value;
	}

	// TAB bots pressure aggressively, so offensive and mobility powers are worth more.
	if (bs->settings.botType == BOT_TAB)
	{
		switch (forcePower)
		{
			case FP_SPEED:
			case FP_PUSH:
			case FP_PULL:
			case FP_LIGHTNING:
			case FP_GRIP:
			case FP_DRAIN:
			case FP_RAGE:
				value += 20;
				break;
			default:
				break;
		}
	}
	else if (bs->settings.botType == BOT_AOTC)
	{
		// AOTC-style bots are more positional/defensive; value survival powers more.
		switch (forcePower)
		{
			case FP_ABSORB:
			case FP_PROTECT:
			case FP_SEE:
			case FP_LEVITATION:
			case FP_SABER_DEFENSE:
				value += 20;
				break;
			default:
				break;
		}
	}
	else if (bs->settings.botType == BOT_HYBRID)
	{
		switch (forcePower)
		{
			case FP_SPEED:
			case FP_PUSH:
			case FP_PULL:
			case FP_ABSORB:
			case FP_SABER_OFFENSE:
			case FP_SABER_DEFENSE:
				value += 10;
				break;
			default:
				break;
		}
	}

	// Saber-biased bots should care about saber/mobility holocrons more than generic powers.
	if (bs->cur_ps.weapon == WP_SABER)
	{
		switch (forcePower)
		{
			case FP_SPEED:
			case FP_PUSH:
			case FP_PULL:
			case FP_SABER_OFFENSE:
			case FP_SABER_DEFENSE:
			case FP_SABERTHROW:
				value += 15;
				break;
			default:
				break;
		}
	}

	return value;
}

int HolocronTakesPriority(bot_state_t *bs)
{
	int i;
	int wpClose = -1;
	gentity_t *bestEnt = NULL;
	float bestDist2 = 999999999.9f;
	float bestScore = -999999999.9f;
	vec3_t dif;
	int grabDelay;
	int heldCount;

	if (g_gametype.integer != GT_HOLOCRON)
	{
		return 0;
	}

	if (!bs || !bs->wpCurrent)
	{
		return 0;
	}

	heldCount = BotHolocronHeldCount(&bs->cur_ps);

	// Find the best active holocron entity.
	// When g_holocronBotPriority is disabled, this preserves the old closest-holocron behavior.
	for (i = level.maxclients; i < level.num_entities; i++)
	{
		gentity_t *ent = &g_entities[i];
		int forcePower;
		float d2;
		float score;

		if (!ent || !ent->inuse)
		{
			continue;
		}

		if (!ent->item)
		{
			continue;
		}

		// MP holocron pickup is typically spawned as "item_holocron".
		if (!ent->item->classname || Q_stricmp(ent->item->classname, "item_holocron"))
		{
			continue;
		}

		forcePower = ent->count;
		if (forcePower < 0 || forcePower >= NUM_FORCE_POWERS)
		{
			continue;
		}

		if (bs->cur_ps.holocronsCarried[forcePower])
		{
			continue;
		}

		VectorSubtract(ent->r.currentOrigin, bs->origin, dif);
		d2 = dif[0]*dif[0] + dif[1]*dif[1] + dif[2]*dif[2];

		if (!g_holocronBotPriority.integer)
		{
			if (d2 < bestDist2)
			{
				bestDist2 = d2;
				bestEnt = ent;
			}
			continue;
		}

		score = (float)BotHolocronStyleValue(bs, forcePower);

		// Nearby useful holocrons should win over distant, low-value pickups.
		if (d2 < 256.0f * 256.0f)
		{
			score += 45.0f;
		}
		else if (d2 < 512.0f * 512.0f)
		{
			score += 25.0f;
		}
		else if (d2 > 1600.0f * 1600.0f)
		{
			score -= 35.0f;
		}

		// Do not run across the map for another pickup while under immediate pressure.
		if (bs->currentEnemy && bs->frame_Enemy_Len < 768.0f && d2 > 700.0f * 700.0f)
		{
			score -= 60.0f;
		}

		// Bots that are already well loaded should only chase extra powers when nearby or very valuable.
		if (heldCount >= 4 && d2 > 512.0f * 512.0f)
		{
			score -= 50.0f;
		}
		else if (heldCount >= 2 && d2 > 1200.0f * 1200.0f)
		{
			score -= 25.0f;
		}

		// Keep some distance sensitivity in the score without making it purely closest-first.
		score -= (d2 / (1024.0f * 1024.0f)) * 20.0f;

		if (score > bestScore || (score == bestScore && d2 < bestDist2))
		{
			bestScore = score;
			bestDist2 = d2;
			bestEnt = ent;
		}
	}

	if (!bestEnt)
	{
		return 0;
	}

	// If priority is enabled but every candidate is poor, keep current combat/navigation behavior.
	if (g_holocronBotPriority.integer && bestScore < 20.0f)
	{
		return 0;
	}

	// Update destination occasionally (keeps them committed but responsive).
	// Style: TAB pressures harder; AOTC commits slightly longer; HYBRID in-between.
	grabDelay = 4000;
	if (bs->settings.botType == BOT_TAB)
	{
		grabDelay = 2500;
	}
	else if (bs->settings.botType == BOT_AOTC)
	{
		grabDelay = 5000;
	}
	else if (bs->settings.botType == BOT_HYBRID)
	{
		grabDelay = 3500;
	}

	if (bs->destinationGrabTime < level.time)
	{
		wpClose = GetNearestVisibleWP(bestEnt->r.currentOrigin, bestEnt->s.number);

		if (wpClose != -1 && gWPArray[wpClose] && gWPArray[wpClose]->inuse)
		{
			bs->wpDestination = gWPArray[wpClose];
			bs->destinationGrabTime = level.time + grabDelay;
		}
	}

	return 1;
}

//see if we already have an item/powerup/etc. that is associated
//with this waypoint.
int BotHasAssociated(bot_state_t *bs, wpobject_t *wp)
{
	gentity_t *as;

	if (wp->associated_entity == ENTITYNUM_NONE)
	{ //make it think this is an item we have so we don't go after nothing
		return 1;
	}

	as = &g_entities[wp->associated_entity];

	if (!as || !as->item)
	{
		return 0;
	}

	if (as->item->giType == IT_WEAPON)
	{
		if (bs->cur_ps.stats[STAT_WEAPONS] & (1 << as->item->giTag))
		{
			return 1;
		}

		return 0;
	}
	else if (as->item->giType == IT_HOLDABLE)
	{
		if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << as->item->giTag))
		{
			return 1;
		}

		return 0;
	}
	else if (as->item->giType == IT_POWERUP)
	{
		if (bs->cur_ps.powerups[as->item->giTag])
		{
			return 1;
		}

		return 0;
	}
	else if (as->item->giType == IT_AMMO)
	{
		if (bs->cur_ps.ammo[as->item->giTag] > 10) //hack
		{
			return 1;
		}

		return 0;
	}

	return 0;
}

//[NewGameTypes][EnhancedImpliment]
/*
extern int num_flags; // Current total number of scenario flags...
int wplist[256];

//get the index to the nearest visible waypoint in the global trail
int GetNearestVisibleWPList(vec3_t org, int ignore)
{
	int i;
	float bestdist;
	float flLen;
	int bestindex;
	vec3_t a, mins, maxs;
	int wplist_number = 0;

	i = 0;
	if (g_RMG.integer)
	{
		bestdist = 300;
	}
	else
	{
		bestdist = 300;//99999;
				   //don't trace over 800 units away to avoid GIANT HORRIBLE SPEED HITS ^_^
	}
	bestindex = -1;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -1;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 1;

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && VectorDistance(org, gWPArray[i]->origin) < 300)
		{
			VectorSubtract(org, gWPArray[i]->origin, a);
			flLen = VectorLength(a);

			if (OrgVisibleBox(org, mins, maxs, gWPArray[i]->origin, ignore))
			{
				wplist[wplist_number] = i;
				wplist_number++;
				
				if (wplist_number >= 256)
					break;
			}
		}

		//i++;
		i+=Q_irand(0,4); // Search randmly for speed...
	}

	return wplist_number;
}

qboolean FlagWPsInitialized = qfalse;

int Scenario_GetBestIdleGoal(bot_state_t *bs)
{
//	int i = 0;
	int highestweight = 0;
	int desiredindex = -1;
	int dist_to_weight = 0;
	int traildist;
	int test_flag = 0;
	int flagindex = -1;

	if (!bs->wpCurrent)
	{
		return -1;
	}

	if (bs->wpDestination && bs->wpDestination->index 
		&& VectorDistance(bs->cur_ps.origin, gWPArray[bs->wpDestination->index]->origin) > 64)
		return bs->wpDestination->index; // Already have a place to go.. Don't think more...

	if (!FlagWPsInitialized)
	{
		for (test_flag = 0; test_flag < num_flags; test_flag++)
		{// Let's record waypoints close to the flag for faster access later...
			if (flag_list[test_flag].flagentity)
			{
				int num_points = GetNearestVisibleWPList(flag_list[test_flag].flagentity->s.origin, flag_list[test_flag].flagentity->s.number);
				int test = 0;

				while (test < num_points)
				{
					flag_list[test_flag].closeWaypoints[test] = wplist[test];
					test++;
				}
				flag_list[test_flag].num_waypoints = num_points;

				//G_Printf("Flag %i has %i close waypoints.\n", test_flag, num_points);
			}
		}

		FlagWPsInitialized = qtrue;
	}

	for (test_flag = 0; test_flag < num_flags; test_flag++)
	{// Find the best route to follow to a flag not owned by us...
		if (flag_list[test_flag].flagentity)
		{
			if (g_entities[bs->cur_ps.clientNum].client->sess.sessionTeam != flag_list[test_flag].flagentity->s.teamowner)
			{// Let's find the best enemy/neutral flag to head to...
				int test = 0;

				while (test < flag_list[test_flag].num_waypoints)
				{
					traildist = TotalTrailDistance(bs->wpCurrent->index, flag_list[test_flag].closeWaypoints[test], bs);

					if (traildist != -1 && traildist < dist_to_weight)
					{
						dist_to_weight = (int)traildist/10000;
						dist_to_weight = (gWPArray[wplist[test]]->weight)-dist_to_weight;

						highestweight = dist_to_weight;
						desiredindex = flag_list[test_flag].closeWaypoints[test];
						flagindex = test_flag;
						//break;
					}
					test++;
				}

				//if (desiredindex >= 0)
				//	break;
			}
		}
	}

	//set our traversal direction based on the index of the point
	bs->wpDirection = 1;

	return desiredindex;
}
*/
//[/NewGameTypes][EnhancedImpliment]


//we don't really have anything we want to do right now,
//let's just find the best thing to do given the current
//situation.
int GetBestIdleGoal(bot_state_t *bs)
{
	int i = 0;
	int highestweight = 0;
	int desiredindex = -1;
	int dist_to_weight = 0;
	int traildist;

	//[NewGameTypes][EnhancedImpliment]
	/*
	if (g_gametype.integer == GT_SCENARIO)
		return Scenario_GetBestIdleGoal(bs);
	*/
	//[/NewGameTypes][EnhancedImpliment]

	if (!bs->wpCurrent)
	{
		return -1;
	}

	if (bs->isCamper != 2)
	{
		if (bs->randomNavTime < level.time)
		{
			if (Q_irand(1, 10) < 5)
			{
				bs->randomNav = 1;
			}
			else
			{
				bs->randomNav = 0;
			}
			
			bs->randomNavTime = level.time + Q_irand(5000, 15000);
		}
	}

	if (bs->randomNav)
	{ //stop looking for items and/or camping on them
		return -1;
	}

	//[BotTweaks] UNIQUEFIX - why are we doing this?
	/*
	if (bs->wpDestination && bs->wpDestination->index 
		&& VectorDistance(bs->cur_ps.origin, gWPArray[bs->wpDestination->index]->origin) > 64)
		return bs->wpDestination->index; // Already have a place to go.. Don't think more...
	*/
	//[/BotTweaks]	

	while (i < gWPNum)
	{
		if (gWPArray[i] &&
			gWPArray[i]->inuse &&
			(gWPArray[i]->flags & WPFLAG_GOALPOINT) &&
			gWPArray[i]->weight > highestweight &&
			!BotHasAssociated(bs, gWPArray[i]))
		{
			//[Linux] type cast fixes for g++		
			traildist = (int) TotalTrailDistance(bs->wpCurrent->index, i, bs);
			//traildist = TotalTrailDistance(bs->wpCurrent->index, i, bs);
			//[/Linux]

			if (traildist != -1)
			{
				dist_to_weight = (int)traildist/10000;
				//[Linux] type cast fixes for g++
				dist_to_weight = (int) ((gWPArray[i]->weight)-dist_to_weight);
				//dist_to_weight = (gWPArray[i]->weight)-dist_to_weight;
				//[/Linux]

				if (dist_to_weight > highestweight)
				{
					highestweight = dist_to_weight;
					desiredindex = i;
				}
			}
		}

		i++;
	}

	return desiredindex;
}

//go through the list of possible priorities for navigating
//and work out the best destination point.
void GetIdealDestination(bot_state_t *bs)
{
	int tempInt, cWPIndex, bChicken, idleWP;
	float distChange, plusLen, minusLen;
	vec3_t usethisvec, a;
	gentity_t *badthing;

#ifdef _DEBUG
	trap_Cvar_Update(&bot_nogoals);

	if (bot_nogoals.integer)
	{
		return;
	}
#endif

	if (!bs->wpCurrent)
	{
		return;
	}

	if ((level.time - bs->escapeDirTime) > 4000)
	{
		badthing = GetNearestBadThing(bs);
	}
	else
	{
		badthing = NULL;
	}

	if (badthing && badthing->inuse &&
		badthing->health > 0 && badthing->takedamage)
	{
		bs->dangerousObject = badthing;
	}
	else
	{
		bs->dangerousObject = NULL;
	}

	if (!badthing && bs->wpDestIgnoreTime > level.time)
	{
		return;
	}

	if (!badthing && bs->dontGoBack > level.time)
	{
		if (bs->wpDestination)
		{
			bs->wpStoreDest = bs->wpDestination;
		}
		bs->wpDestination = NULL;
		return;
	}
	else if (!badthing && bs->wpStoreDest)
	{ //after we finish running away, switch back to our original destination
		bs->wpDestination = bs->wpStoreDest;
		bs->wpStoreDest = NULL;
	}

	if (badthing && bs->wpCamping)
	{
		bs->wpCamping = NULL;
	}

	if (bs->wpCamping)
	{
		bs->wpDestination = bs->wpCamping;
		return;
	}

	if (!badthing && CTFTakesPriority(bs))
	{
		if (bs->ctfState)
		{
			bs->runningToEscapeThreat = 1;
		}
		return;
	}
	else if (!badthing && SiegeTakesPriority(bs))
	{
		if (bs->siegeState)
		{
			bs->runningToEscapeThreat = 1;
		}
		return;
	}
	else if (!badthing && HolocronTakesPriority(bs))
	{
		bs->runningToEscapeThreat = 1;
		return;
	}
	else if (!badthing && JMTakesPriority(bs))
	{
		bs->runningToEscapeThreat = 1;
	}

	if (badthing)
	{
		bs->runningLikeASissy = level.time + 100;

		if (bs->wpDestination)
		{
			bs->wpStoreDest = bs->wpDestination;
		}
		bs->wpDestination = NULL;

		if (bs->wpDirection)
		{
			tempInt = bs->wpCurrent->index+1;
		}
		else
		{
			tempInt = bs->wpCurrent->index-1;
		}

		if (gWPArray[tempInt] && gWPArray[tempInt]->inuse && bs->escapeDirTime < level.time)
		{
			VectorSubtract(badthing->s.pos.trBase, bs->wpCurrent->origin, a);
			plusLen = VectorLength(a);
			VectorSubtract(badthing->s.pos.trBase, gWPArray[tempInt]->origin, a);
			minusLen = VectorLength(a);

			if (plusLen < minusLen)
			{
				if (bs->wpDirection)
				{
					bs->wpDirection = 0;
				}
				else
				{
					bs->wpDirection = 1;
				}

				bs->wpCurrent = gWPArray[tempInt];

				bs->escapeDirTime = level.time + Q_irand(500, 1000);//Q_irand(1000, 1400);

				//G_Printf("Escaping from scary bad thing [%s]\n", badthing->classname);
			}
		}
		//G_Printf("Run away run away run away!\n");
		return;
	}

	distChange = 0; //keep the compiler from complaining

	tempInt = BotGetWeaponRange(bs);

	if (tempInt == BWEAPONRANGE_MELEE)
	{
		distChange = 1;
	}
	else if (tempInt == BWEAPONRANGE_SABER)
	{
		distChange = 1;
	}
	else if (tempInt == BWEAPONRANGE_MID)
	{
		distChange = 128;
	}
	else if (tempInt == BWEAPONRANGE_LONG)
	{
		distChange = 300;
	}

	//RACC - Go after your revenge enemy if you can get to him from there.
	if (bs->revengeEnemy && bs->revengeEnemy->health > 0 &&
		bs->revengeEnemy->client &&
		((clientConnected_t)bs->revengeEnemy->client->pers.connected == (clientConnected_t)CA_ACTIVE ||
			(clientConnected_t)bs->revengeEnemy->client->pers.connected == (clientConnected_t)CA_AUTHORIZING))
	{
		// if we hate someone, always try to get to them
		if (bs->wpDestSwitchTime < level.time)
		{
			if (bs->revengeEnemy->client)
			{
				VectorCopy(bs->revengeEnemy->client->ps.origin, usethisvec);
			}
			else
			{
				VectorCopy(bs->revengeEnemy->s.origin, usethisvec);
			}

			tempInt = GetNearestVisibleWP(usethisvec, 0);

			if (tempInt != -1 && TotalTrailDistance(bs->wpCurrent->index, tempInt, bs) != -1)
			{
				bs->wpDestination = gWPArray[tempInt];
				bs->wpDestSwitchTime = level.time + Q_irand(5000, 10000);
			}
		}
	}
	else if (bs->squadLeader && bs->squadLeader->health > 0 &&
		bs->squadLeader->client &&
		((clientConnected_t)bs->squadLeader->client->pers.connected == (clientConnected_t)CA_ACTIVE ||
			(clientConnected_t)bs->squadLeader->client->pers.connected == (clientConnected_t)CA_AUTHORIZING))
	{
		if (bs->wpDestSwitchTime < level.time)
		{
			if (bs->squadLeader->client)
			{
				VectorCopy(bs->squadLeader->client->ps.origin, usethisvec);
			}
			else
			{
				VectorCopy(bs->squadLeader->s.origin, usethisvec);
			}

			tempInt = GetNearestVisibleWP(usethisvec, 0);

			if (tempInt != -1 && TotalTrailDistance(bs->wpCurrent->index, tempInt, bs) != -1)
			{
				bs->wpDestination = gWPArray[tempInt];
				bs->wpDestSwitchTime = level.time + Q_irand(5000, 10000);
			}
		}
	}

	else if (bs->currentEnemy)
	{
		if (bs->currentEnemy->client)
		{
			VectorCopy(bs->currentEnemy->client->ps.origin, usethisvec);
		}
		else
		{
			VectorCopy(bs->currentEnemy->s.origin, usethisvec);
		}

		bChicken = BotIsAChickenWuss(bs);
		bs->runningToEscapeThreat = bChicken;

		if (bs->frame_Enemy_Len < distChange || (bChicken && bChicken != 2))
		{
			cWPIndex = bs->wpCurrent->index;

			if (bs->frame_Enemy_Len > 400)
			{ //good distance away, start running toward a good place for an item or powerup or whatever
				idleWP = GetBestIdleGoal(bs);

				if (idleWP != -1 && gWPArray[idleWP] && gWPArray[idleWP]->inuse)
				{
					bs->wpDestination = gWPArray[idleWP];
				}
			}
			else if (gWPArray[cWPIndex-1] && gWPArray[cWPIndex-1]->inuse &&
				gWPArray[cWPIndex+1] && gWPArray[cWPIndex+1]->inuse)
			{
				VectorSubtract(gWPArray[cWPIndex+1]->origin, usethisvec, a);
				plusLen = VectorLength(a);
				VectorSubtract(gWPArray[cWPIndex-1]->origin, usethisvec, a);
				minusLen = VectorLength(a);

				if (minusLen > plusLen)
				{
					bs->wpDestination = gWPArray[cWPIndex-1];
				}
				else
				{
					bs->wpDestination = gWPArray[cWPIndex+1];
				}
			}
		}
		else if (bChicken != 2 && bs->wpDestSwitchTime < level.time)
		{
			tempInt = GetNearestVisibleWP(usethisvec, 0);

			if (tempInt != -1 && TotalTrailDistance(bs->wpCurrent->index, tempInt, bs) != -1)
			{
				bs->wpDestination = gWPArray[tempInt];

				if (g_gametype.integer == GT_SINGLE_PLAYER)
				{ //be more aggressive
					bs->wpDestSwitchTime = level.time + Q_irand(300, 1000);
				}
				else
				{
					bs->wpDestSwitchTime = level.time + Q_irand(1000, 5000);
				}
			}
		}
	}

	if (!bs->wpDestination && bs->wpDestSwitchTime < level.time)
	{
		//G_Printf("I need something to do\n");
		idleWP = GetBestIdleGoal(bs);

		if (idleWP != -1 && gWPArray[idleWP] && gWPArray[idleWP]->inuse)
		{
			bs->wpDestination = gWPArray[idleWP];
		}
	}
}

//Assign simple CTF roles to all bots on a team.  This is intentionally
//conservative: humans are never changed, player-forced bot orders are respected,
//and the old squad commander logic remains as a fallback when disabled.
static qboolean BotCTFTeamRoleAssignment(bot_state_t *bs)
{
	int myTeam;
	int myFlag;
	int enemyFlag;
	int i;
	int teamBots[MAX_CLIENTS];
	int numTeamBots = 0;
	int defendersWanted;
	int defendersAssigned = 0;
	int attackersAssigned = 0;
	int escortsAssigned = 0;
	int returnersAssigned = 0;
	int returnersWanted = 1;
	qboolean enemyHasOurFlag = qfalse;
	qboolean ownFlagDropped = qfalse;
	qboolean weHaveEnemyFlag = qfalse;
	gentity_t *carrierEnt = NULL;
	gentity_t *ent;
	gentity_t *enemyCarrier = NULL;
	gentity_t *ownDroppedFlag = NULL;
	bot_state_t *bst;

	qboolean isCTY = (g_gametype.integer == GT_CTY);

	if (!bs || bs->client < 0 || bs->client >= MAX_CLIENTS)
	{
		return qfalse;
	}

	// CTF and CTY share the same low-level flag navigation states, but CTY gets
	// its own cvar so Ysalamiri-specific tuning can evolve independently.
	if (g_gametype.integer != GT_CTF && g_gametype.integer != GT_CTY)
	{
		return qfalse;
	}

	if ((!isCTY && !g_ctfBotRoles.integer) || (isCTY && !g_ctyBotRoles.integer))
	{
		return qfalse;
	}

	ent = &g_entities[bs->client];
	if (!ent->client)
	{
		return qfalse;
	}

	myTeam = ent->client->sess.sessionTeam;
	if (myTeam != TEAM_RED && myTeam != TEAM_BLUE)
	{
		return qfalse;
	}

	if (!flagRed || !flagBlue || !eFlagRed || !eFlagBlue)
	{
		return qfalse;
	}

	//Do not churn roles every frame.  A short team-level cadence is enough for
	//flag events while preventing assignment jitter.
	if (botCTFRoleUpdateTime[myTeam] > level.time)
	{
		return qtrue;
	}
	botCTFRoleUpdateTime[myTeam] = level.time + 7000 + Q_irand(0, 3000);

	if (myTeam == TEAM_RED)
	{
		myFlag = PW_REDFLAG;
		enemyFlag = PW_BLUEFLAG;
		ownDroppedFlag = droppedRedFlag;
	}
	else
	{
		myFlag = PW_BLUEFLAG;
		enemyFlag = PW_REDFLAG;
		ownDroppedFlag = droppedBlueFlag;
	}

	if (ownDroppedFlag && (ownDroppedFlag->flags & FL_DROPPED_ITEM) &&
		ownDroppedFlag->classname && strcmp(ownDroppedFlag->classname, "freed") != 0)
	{
		ownFlagDropped = qtrue;
	}

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		gentity_t *check = &g_entities[i];

		if (!check->inuse || !check->client)
		{
			continue;
		}

		if (check->client->sess.sessionTeam == myTeam && check->client->ps.powerups[enemyFlag])
		{
			weHaveEnemyFlag = qtrue;
			carrierEnt = check;
		}
		else if (check->client->sess.sessionTeam != TEAM_SPECTATOR &&
			check->client->sess.sessionTeam != myTeam && check->client->ps.powerups[myFlag])
		{
			enemyHasOurFlag = qtrue;
			enemyCarrier = check;
		}

		if (check->client->sess.sessionTeam == myTeam && botstates[i])
		{
			teamBots[numTeamBots++] = i;
		}
	}

	if (!numTeamBots)
	{
		return qtrue;
	}

	defendersWanted = (numTeamBots >= 3) ? 1 : 0;

	//First pass: forced orders and flag carriers keep their special states.
	for (i = 0; i < numTeamBots; i++)
	{
		bst = botstates[teamBots[i]];
		if (!bst)
		{
			continue;
		}

		if (bst->state_Forced)
		{
			bst->ctfState = bst->state_Forced;
			continue;
		}

		if (g_entities[teamBots[i]].client->ps.powerups[enemyFlag])
		{
			bst->ctfState = CTFSTATE_GETFLAGHOME;
		}
		else
		{
			bst->ctfState = CTFSTATE_NONE;
		}
	}

	//Second pass: if our flag/Ysalamiri is stolen or dropped, assign the
	// closest eligible bot(s) to return it.  Avoid sending the entire team and
	// leave carriers/forced-order bots alone.
	if (((isCTY && g_ctyBotReturnPriority.integer) ||
		(!isCTY && g_ctfBotReturnFlagPriority.integer)) &&
		(enemyHasOurFlag || ownFlagDropped))
	{
		returnersWanted = (ownFlagDropped && numTeamBots >= 5) ? 2 : 1;

		while (returnersAssigned < returnersWanted)
		{
			int bestBot = -1;
			float bestDist = 999999999.0f;
			vec3_t targetOrigin;

			if (ownFlagDropped && ownDroppedFlag)
			{
				VectorCopy(ownDroppedFlag->s.pos.trBase, targetOrigin);
			}
			else if (enemyCarrier && enemyCarrier->client)
			{
				VectorCopy(enemyCarrier->client->ps.origin, targetOrigin);
			}
			else
			{
				break;
			}

			for (i = 0; i < numTeamBots; i++)
			{
				float distSq;

				bst = botstates[teamBots[i]];
				if (!bst || bst->state_Forced || bst->ctfState != CTFSTATE_NONE)
				{
					continue;
				}

				distSq = DistanceSquared(g_entities[teamBots[i]].client->ps.origin, targetOrigin);
				if (distSq < bestDist)
				{
					bestDist = distSq;
					bestBot = teamBots[i];
				}
			}

			if (bestBot == -1)
			{
				break;
			}

			botstates[bestBot]->ctfState = CTFSTATE_RETRIEVAL;
			returnersAssigned++;
		}
	}
	else if (enemyHasOurFlag)
	{
		for (i = 0; i < numTeamBots; i++)
		{
			bst = botstates[teamBots[i]];
			if (!bst || bst->state_Forced || bst->ctfState == CTFSTATE_GETFLAGHOME)
			{
				continue;
			}

			bst->ctfState = CTFSTATE_RETRIEVAL;
			returnersAssigned++;
			break;
		}
	}

	//Third pass: if we have the enemy flag, assign one or two nearby escorts
	//when escort AI is enabled.  Returners are assigned first above so our own
	//flag recovery still wins when the team is under pressure.
	if (((isCTY && g_ctyBotEscortAI.integer) || (!isCTY && g_ctfBotEscortAI.integer)) &&
		weHaveEnemyFlag && carrierEnt && carrierEnt->client)
	{
		int escortsWanted = (numTeamBots >= 5 && !enemyHasOurFlag) ? 2 : 1;

		while (escortsAssigned < escortsWanted)
		{
			int bestBot = -1;
			float bestDist = 999999999.0f;

			for (i = 0; i < numTeamBots; i++)
			{
				float distSq;

				bst = botstates[teamBots[i]];
				if (!bst || bst->state_Forced || bst->ctfState != CTFSTATE_NONE)
				{
					continue;
				}

				distSq = DistanceSquared(g_entities[teamBots[i]].client->ps.origin, carrierEnt->client->ps.origin);
				if (distSq < bestDist)
				{
					bestDist = distSq;
					bestBot = teamBots[i];
				}
			}

			if (bestBot == -1)
			{
				break;
			}

			botstates[bestBot]->ctfState = CTFSTATE_GUARDCARRIER;
			escortsAssigned++;
		}
	}

	//Fourth pass: keep a small defensive presence if enough bots exist.
	for (i = 0; i < numTeamBots && defendersAssigned < defendersWanted; i++)
	{
		bst = botstates[teamBots[i]];
		if (!bst || bst->state_Forced || bst->ctfState != CTFSTATE_NONE)
		{
			continue;
		}

		bst->ctfState = CTFSTATE_DEFENDER;
		defendersAssigned++;
	}

	//Final pass: anyone still unassigned attacks.
	for (i = 0; i < numTeamBots; i++)
	{
		bst = botstates[teamBots[i]];
		if (!bst || bst->state_Forced || bst->ctfState != CTFSTATE_NONE)
		{
			continue;
		}

		bst->ctfState = CTFSTATE_ATTACKER;
		attackersAssigned++;
	}

	if ((!isCTY && g_debugCTFBotRoles.integer) || (isCTY && g_debugCTYBotRoles.integer))
	{
		G_Printf("%s bot roles: team %s bots=%i returners=%i escort=%i defenders=%i attackers=%i ownDropped=%i enemyCarrier=%i\n",
			isCTY ? "CTY" : "CTF",
			(myTeam == TEAM_RED) ? "RED" : "BLUE", numTeamBots,
			returnersAssigned, escortsAssigned,
			defendersAssigned, attackersAssigned, ownFlagDropped ? 1 : 0,
			enemyHasOurFlag ? 1 : 0);
	}

	return qtrue;
}

//commander CTF AI - tell other bots in the so-called
//"squad" what to do.
void CommanderBotCTFAI(bot_state_t *bs)
{
	// Can be called defensively from higher-level logic; avoid null deref.
	if (!bs || bs->client < 0 || bs->client >= MAX_CLIENTS) {
		return;
	}

	if (BotCTFTeamRoleAssignment(bs))
	{
		return;
	}

	int i = 0;
	gentity_t *ent;
	int squadmates = 0;
	gentity_t *squad[MAX_CLIENTS];
	int defendAttackPriority = 0; //0 == attack, 1 == defend
	int guardDefendPriority = 0; //0 == defend, 1 == guard
	int attackRetrievePriority = 0; //0 == retrieve, 1 == attack
	int myFlag = 0;
	int enemyFlag = 0;
	int enemyHasOurFlag = 0;
	int weHaveEnemyFlag = 0;
	int numOnMyTeam = 0;
	int numOnEnemyTeam = 0;
	int numAttackers = 0;
	int numDefenders = 0;

	// Style bias for how this commander allocates roles.
	// Keep BOT_DEFAULT as close as possible to base JKA behaviour.
	if (bs)
	{
		if (bs->settings.botType == BOT_AOTC)
		{
			// Battlefront-like: start with defense.
			defendAttackPriority = 1;
		}
		else if (bs->settings.botType == BOT_TAB)
		{
			// Force Unleashed-like: start with offense/pressure.
			defendAttackPriority = 0;
		}
		// BOT_HYBRID keeps the stock alternating pattern (starts offense).
	}

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		myFlag = PW_REDFLAG;
	}
	else
	{
		myFlag = PW_BLUEFLAG;
	}

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		enemyFlag = PW_BLUEFLAG;
	}
	else
	{
		enemyFlag = PW_REDFLAG;
	}

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		//[BotTweaks] UNIQUEFIXME - any reason for this?
		//if (ent && ent->client && ent->inuse)
		if (ent && ent->client)
		//[/BotTweaks] UNIQUEFIXME - any reason for this?
		{
			if (ent->client->ps.powerups[enemyFlag] && OnSameTeam(&g_entities[bs->client], ent))
			{
				weHaveEnemyFlag = 1;
			}
			else if (ent->client->ps.powerups[myFlag] && !OnSameTeam(&g_entities[bs->client], ent))
			{
				enemyHasOurFlag = 1;
			}

			if (OnSameTeam(&g_entities[bs->client], ent))
			{
				numOnMyTeam++;
			}
			else
			{
				numOnEnemyTeam++;
			}

			if (botstates[ent->s.number])
			{
				if (botstates[ent->s.number]->ctfState == CTFSTATE_ATTACKER ||
					botstates[ent->s.number]->ctfState == CTFSTATE_RETRIEVAL)
				{
					numAttackers++;
				}
				else
				{
					numDefenders++;
				}
			}
			else
			{ //assume real players to be attackers in our logic
				numAttackers++;
			}
		}
		i++;
	}

	i = 0;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->client && botstates[i] && botstates[i]->squadLeader && botstates[i]->squadLeader->s.number == bs->client && i != bs->client)
		{
			squad[squadmates] = ent;
			squadmates++;
		}

		i++;
	}

	squad[squadmates] = &g_entities[bs->client];
	squadmates++;

	i = 0;

	if (enemyHasOurFlag && !weHaveEnemyFlag)
	{ //start off with an attacker instead of a retriever if we don't have the enemy flag yet so that they can't capture it first.
	  //after that we focus on getting our flag back.
		attackRetrievePriority = 1;
	}

	while (i < squadmates)
	{
		if (squad[i] && squad[i]->client && botstates[squad[i]->s.number])
		{
			if (botstates[squad[i]->s.number]->ctfState != CTFSTATE_GETFLAGHOME)
			{ //never tell a bot to stop trying to bring the flag to the base
				if (defendAttackPriority)
				{
					if (weHaveEnemyFlag)
					{
						if (guardDefendPriority)
						{
							botstates[squad[i]->s.number]->ctfState = CTFSTATE_GUARDCARRIER;
							guardDefendPriority = 0;
						}
						else
						{
							botstates[squad[i]->s.number]->ctfState = CTFSTATE_DEFENDER;
							guardDefendPriority = 1;
						}
					}
					else
					{
						botstates[squad[i]->s.number]->ctfState = CTFSTATE_DEFENDER;
					}
					defendAttackPriority = 0;
				}
				else
				{
					if (enemyHasOurFlag)
					{
						if (attackRetrievePriority)
						{
							botstates[squad[i]->s.number]->ctfState = CTFSTATE_ATTACKER;
							attackRetrievePriority = 0;
						}
						else
						{
							botstates[squad[i]->s.number]->ctfState = CTFSTATE_RETRIEVAL;
							attackRetrievePriority = 1;
						}
					}
					else
					{
						botstates[squad[i]->s.number]->ctfState = CTFSTATE_ATTACKER;
					}
					defendAttackPriority = 1;
				}
			}
			else if ((numOnMyTeam < 2 || !numAttackers) && enemyHasOurFlag)
			{ //I'm the only one on my team who will attack and the enemy has my flag, I have to go after him
				botstates[squad[i]->s.number]->ctfState = CTFSTATE_RETRIEVAL;
			}
		}

		i++;
	}
}

//similar to ctf ai, for siege
void CommanderBotSiegeAI(bot_state_t *bs)
{
	int i = 0;
	int squadmates = 0;
	int commanded = 0;
	int teammates = 0;
	gentity_t *squad[MAX_CLIENTS];
	gentity_t *ent;
	bot_state_t *bst;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->client && OnSameTeam(&g_entities[bs->client], ent) && botstates[ent->s.number])
		{
			bst = botstates[ent->s.number];

			if (bst && !bst->isSquadLeader && !bst->state_Forced)
			{
				squad[squadmates] = ent;
				squadmates++;
			}
			else if (bst && !bst->isSquadLeader && bst->state_Forced)
			{ //count them as commanded
				commanded++;
			}
		}

		if (ent && ent->client && OnSameTeam(&g_entities[bs->client], ent))
		{
			teammates++;
		}

		i++;
	}
	
	if (!squadmates)
	{
		return;
	}

	//tell squad mates to do what I'm doing, up to half of team, let the other half make their own decisions
	i = 0;

	while (i < squadmates && squad[i])
	{
		bst = botstates[squad[i]->s.number];

		if (commanded > teammates/2)
		{
			break;
		}

		if (bst)
		{
			bst->state_Forced = bs->siegeState;
			bst->siegeState = bs->siegeState;
			commanded++;
		}

		i++;
	}
}

//teamplay ffa squad ai
void BotDoTeamplayAI(bot_state_t *bs)
{
	if (bs->state_Forced)
	{
		bs->teamplayState = bs->state_Forced;
	}

	if (bs->teamplayState == TEAMPLAYSTATE_REGROUP)
	{ //force to find a new leader
		bs->squadLeader = NULL;
		bs->isSquadLeader = 0;
	}
}


//Assign simple GT_TEAM roles to bots on the same team.  This is intentionally
//conservative: humans are never changed, player-forced bot orders are respected,
//and the existing teamplay commander logic remains the fallback when disabled.
static qboolean BotTeamRoleAssignment(bot_state_t *bs)
{
	int myTeam;
	int i;
	int teamBots[MAX_CLIENTS];
	int numTeamBots = 0;
	int attackersAssigned = 0;
	int defendersAssigned = 0;
	int supportAssigned = 0;
	int huntersAssigned = 0;
	int defendersWanted;
	int supportWanted;
	int huntersWanted;
	int lowHealthClient = -1;
	int lowHealth = 1000;
	int leaderClient = -1;
	int leaderScore = -999999;
	int enemyLeaderClient = -1;
	int enemyLeaderScore = -999999;
	gentity_t *ent;
	bot_state_t *bst;

	if (!bs || bs->client < 0 || bs->client >= MAX_CLIENTS)
	{
		return qfalse;
	}

	if (g_gametype.integer != GT_TEAM || !g_teamBotRoles.integer)
	{
		return qfalse;
	}

	ent = &g_entities[bs->client];
	if (!ent->client)
	{
		return qfalse;
	}

	myTeam = ent->client->sess.sessionTeam;
	if (myTeam != TEAM_RED && myTeam != TEAM_BLUE)
	{
		return qfalse;
	}

	//Do not churn roles every frame.  GT_TEAM has no hard objective state, so a
	//slower cadence keeps bots organized without making them twitchy.
	if (botTeamRoleUpdateTime[myTeam] > level.time)
	{
		return qtrue;
	}
	botTeamRoleUpdateTime[myTeam] = level.time + 8000 + Q_irand(0, 4000);

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		gentity_t *check = &g_entities[i];

		if (!check->inuse || !check->client ||
			check->client->pers.connected != CON_CONNECTED ||
			check->client->sess.sessionTeam == TEAM_SPECTATOR)
		{
			continue;
		}

		if (check->client->sess.sessionTeam == myTeam)
		{
			if (check->health > 0 && check->client->ps.persistant[PERS_SCORE] > leaderScore)
			{
				leaderScore = check->client->ps.persistant[PERS_SCORE];
				leaderClient = i;
			}

			if (check->health > 0 && check->health < lowHealth)
			{
				lowHealth = check->health;
				lowHealthClient = i;
			}

			if (botstates[i])
			{
				teamBots[numTeamBots++] = i;
			}
		}
		else if (check->health > 0 && check->client->ps.persistant[PERS_SCORE] > enemyLeaderScore)
		{
			enemyLeaderScore = check->client->ps.persistant[PERS_SCORE];
			enemyLeaderClient = i;
		}
	}

	if (!numTeamBots)
	{
		return qtrue;
	}

	defendersWanted = (numTeamBots >= 3) ? 1 : 0;
	supportWanted = (numTeamBots >= 4 && lowHealthClient >= 0 && lowHealth < 75) ? 1 : 0;
	huntersWanted = (numTeamBots >= 4 && enemyLeaderClient >= 0) ? 1 : 0;

	//First pass: clear old automatic assignments, but keep player-forced bot
	//orders intact.  Forced orders are how human team leaders override this.
	for (i = 0; i < numTeamBots; i++)
	{
		bst = botstates[teamBots[i]];
		if (!bst)
		{
			continue;
		}

		if (bst->state_Forced)
		{
			bst->teamplayState = bst->state_Forced;
			continue;
		}

		bst->teamplayState = TEAMPLAYSTATE_NONE;
		bst->squadLeader = NULL;
		bst->isSquadLeader = 0;
	}

	//Support: one bot helps the most injured teammate when a real need exists.
	if (supportWanted)
	{
		for (i = 0; i < numTeamBots; i++)
		{
			bst = botstates[teamBots[i]];
			if (!bst || bst->state_Forced || teamBots[i] == lowHealthClient)
			{
				continue;
			}

			bst->teamplayState = TEAMPLAYSTATE_ASSISTING;
			bst->squadLeader = &g_entities[lowHealthClient];
			supportAssigned++;
			break;
		}
	}

	//Defender: keep one bot close to the team's strongest active player.  GT_TEAM
	//has no base objective, so this behaves as a conservative bodyguard role.
	if (defendersWanted && leaderClient >= 0)
	{
		for (i = 0; i < numTeamBots; i++)
		{
			bst = botstates[teamBots[i]];
			if (!bst || bst->state_Forced || bst->teamplayState != TEAMPLAYSTATE_NONE || teamBots[i] == leaderClient)
			{
				continue;
			}

			bst->teamplayState = TEAMPLAYSTATE_FOLLOWING;
			bst->squadLeader = &g_entities[leaderClient];
			defendersAssigned++;
			break;
		}
	}

	//Hunter: one bot pressures the highest-scoring visible enemy candidate.  This
	//uses the existing revenge enemy path rather than adding new navigation code.
	if (huntersWanted)
	{
		for (i = 0; i < numTeamBots; i++)
		{
			bst = botstates[teamBots[i]];
			if (!bst || bst->state_Forced || bst->teamplayState != TEAMPLAYSTATE_NONE)
			{
				continue;
			}

			bst->revengeEnemy = &g_entities[enemyLeaderClient];
			huntersAssigned++;
			break;
		}
	}

	//Everything left unassigned behaves as a normal attacker/frontline bot.
	for (i = 0; i < numTeamBots; i++)
	{
		bst = botstates[teamBots[i]];
		if (!bst || bst->state_Forced || bst->teamplayState != TEAMPLAYSTATE_NONE)
		{
			continue;
		}
		attackersAssigned++;
	}

	if (g_debugTeamBotRoles.integer)
	{
		G_Printf("TEAM bot roles: team %s bots=%i support=%i defenders=%i hunters=%i attackers=%i lowHealth=%i enemyLeader=%i\n",
			(myTeam == TEAM_RED) ? "RED" : "BLUE", numTeamBots,
			supportAssigned, defendersAssigned, huntersAssigned, attackersAssigned,
			lowHealthClient, enemyLeaderClient);
	}

	return qtrue;
}

//like ctf and siege commander ai, instruct the squad
void CommanderBotTeamplayAI(bot_state_t *bs)
{
	int i = 0;
	int squadmates = 0;
	int teammates = 0;
	int teammate_indanger = -1;
	int teammate_helped = 0;
	int foundsquadleader = 0;
	int worsthealth = 50;
	gentity_t *squad[MAX_CLIENTS];
	gentity_t *ent;
	bot_state_t *bst;

	if (BotTeamRoleAssignment(bs))
	{
		return;
	}

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->client && OnSameTeam(&g_entities[bs->client], ent) && botstates[ent->s.number])
		{
			bst = botstates[ent->s.number];

			if (foundsquadleader && bst && bst->isSquadLeader)
			{ //never more than one squad leader
				bst->isSquadLeader = 0;
			}

			if (bst && !bst->isSquadLeader)
			{
				squad[squadmates] = ent;
				squadmates++;
			}
			else if (bst)
			{
				foundsquadleader = 1;
			}
		}

		if (ent && ent->client && OnSameTeam(&g_entities[bs->client], ent))
		{
			teammates++;

			if (ent->health < worsthealth)
			{
				teammate_indanger = ent->s.number;
				worsthealth = ent->health;
			}
		}

		i++;
	}
	
	if (!squadmates)
	{
		return;
	}

	i = 0;

	while (i < squadmates && squad[i])
	{
		bst = botstates[squad[i]->s.number];

		if (bst && !bst->state_Forced)
		{ //only order if this guy is not being ordered directly by the real player team leader
			if (teammate_indanger >= 0 && !teammate_helped)
			{ //send someone out to help whoever needs help most at the moment
				bst->teamplayState = TEAMPLAYSTATE_ASSISTING;
				bst->squadLeader = &g_entities[teammate_indanger];
				teammate_helped = 1;
			}
			else if ((teammate_indanger == -1 || teammate_helped) && bst->teamplayState == TEAMPLAYSTATE_ASSISTING)
			{ //no teammates need help badly, but this guy is trying to help them anyway, so stop
				bst->teamplayState = TEAMPLAYSTATE_FOLLOWING;
				bst->squadLeader = &g_entities[bs->client];
			}

			if (bs->squadRegroupInterval < level.time && Q_irand(1, 10) < 5)
			{ //every so often tell the squad to regroup for the sake of variation
				if (bst->teamplayState == TEAMPLAYSTATE_FOLLOWING)
				{
					bst->teamplayState = TEAMPLAYSTATE_REGROUP;
				}

				bs->isSquadLeader = 0;
				bs->squadCannotLead = level.time + 500;
				bs->squadRegroupInterval = level.time + Q_irand(45000, 65000);
			}
		}

		i++;
	}	
}

//pick which commander ai to use based on gametype
void CommanderBotAI(bot_state_t *bs)
{
	if (g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTY)
	{
		CommanderBotCTFAI(bs);
	}
	else if (g_gametype.integer == GT_SIEGE)
	{
		CommanderBotSiegeAI(bs);
	}
	else if (g_gametype.integer == GT_TEAM)
	{
		CommanderBotTeamplayAI(bs);
	}
	//[NewGameTypes][EnhanceImpliment]
	/*
	else if (g_gametype.integer == GT_SCENARIO)
	{
		CommanderBotTeamplayAI(bs);
	}
	*/
	//[/NewGameTypes]
}

//close range combat routines
void MeleeCombatHandling(bot_state_t *bs)
{
	vec3_t usethisvec;
	vec3_t downvec;
	vec3_t midorg;
	vec3_t a;
	vec3_t fwd;
	vec3_t mins, maxs;
	trace_t tr;
	int en_down;
	int me_down;
	int mid_down;

	if (!bs->currentEnemy)
	{
		return;
	}

	if (bs->currentEnemy->client)
	{
		VectorCopy(bs->currentEnemy->client->ps.origin, usethisvec);
	}
	else
	{
		VectorCopy(bs->currentEnemy->s.origin, usethisvec);
	}

	if (bs->meleeStrafeTime < level.time)
	{
		if (bs->meleeStrafeDir)
		{
			bs->meleeStrafeDir = 0;
		}
		else
		{
			bs->meleeStrafeDir = 1;
		}

		bs->meleeStrafeTime = level.time + Q_irand(500, 1800);
	}

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -24;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 32;

	VectorCopy(usethisvec, downvec);
	downvec[2] -= 4096;

	trap_Trace(&tr, usethisvec, mins, maxs, downvec, -1, MASK_SOLID);

	en_down = (int)tr.endpos[2];

	VectorCopy(bs->origin, downvec);
	downvec[2] -= 4096;

	trap_Trace(&tr, bs->origin, mins, maxs, downvec, -1, MASK_SOLID);

	me_down = (int)tr.endpos[2];

	VectorSubtract(usethisvec, bs->origin, a);
	vectoangles(a, a);
	AngleVectors(a, fwd, NULL, NULL);

	midorg[0] = bs->origin[0] + fwd[0]*bs->frame_Enemy_Len/2;
	midorg[1] = bs->origin[1] + fwd[1]*bs->frame_Enemy_Len/2;
	midorg[2] = bs->origin[2] + fwd[2]*bs->frame_Enemy_Len/2;

	VectorCopy(midorg, downvec);
	downvec[2] -= 4096;

	trap_Trace(&tr, midorg, mins, maxs, downvec, -1, MASK_SOLID);

	mid_down = (int)tr.endpos[2];

	if (me_down == en_down &&
		en_down == mid_down)
	{
		VectorCopy(usethisvec, bs->goalPosition);
	}
}

//saber combat routines (it's simple, but it works)
void SaberCombatHandling(bot_state_t *bs)
{
	vec3_t usethisvec;
	vec3_t downvec;
	vec3_t midorg;
	vec3_t a;
	vec3_t fwd;
	vec3_t mins, maxs;
	trace_t tr;
	int en_down;
	int me_down;
	int mid_down;

	if (!bs->currentEnemy)
	{
		return;
	}

	if (bs->currentEnemy->client)
	{
		VectorCopy(bs->currentEnemy->client->ps.origin, usethisvec);
	}
	else
	{
		VectorCopy(bs->currentEnemy->s.origin, usethisvec);
	}

	if (bs->meleeStrafeTime < level.time)
	{
		if (bs->meleeStrafeDir)
		{
			bs->meleeStrafeDir = 0;
		}
		else
		{
			bs->meleeStrafeDir = 1;
		}

		bs->meleeStrafeTime = level.time + Q_irand(500, 1800);
	}

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -24;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 32;

	VectorCopy(usethisvec, downvec);
	downvec[2] -= 4096;

	trap_Trace(&tr, usethisvec, mins, maxs, downvec, -1, MASK_SOLID);

	en_down = (int)tr.endpos[2];

	if (tr.startsolid || tr.allsolid)
	{
		en_down = 1;
		me_down = 2;
	}
	else
	{
		VectorCopy(bs->origin, downvec);
		downvec[2] -= 4096;

		trap_Trace(&tr, bs->origin, mins, maxs, downvec, -1, MASK_SOLID);

		me_down = (int)tr.endpos[2];

		if (tr.startsolid || tr.allsolid)
		{
			en_down = 1;
			me_down = 2;
		}
	}

	VectorSubtract(usethisvec, bs->origin, a);
	vectoangles(a, a);
	AngleVectors(a, fwd, NULL, NULL);

	midorg[0] = bs->origin[0] + fwd[0]*bs->frame_Enemy_Len/2;
	midorg[1] = bs->origin[1] + fwd[1]*bs->frame_Enemy_Len/2;
	midorg[2] = bs->origin[2] + fwd[2]*bs->frame_Enemy_Len/2;

	VectorCopy(midorg, downvec);
	downvec[2] -= 4096;

	trap_Trace(&tr, midorg, mins, maxs, downvec, -1, MASK_SOLID);

	mid_down = (int)tr.endpos[2];

	//RACC - Both over the same level of ground
	if (me_down == en_down &&
		en_down == mid_down)
	{
		//RACC - jump up if your enemy is in the air too.
		if (usethisvec[2] > (bs->origin[2]+32) &&
			bs->currentEnemy->client &&
			bs->currentEnemy->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			bs->jumpTime = level.time + 100;
		}

		if (bs->frame_Enemy_Len > 128)
		{ //be ready to attack
			bs->saberDefending = 0;
			bs->saberDefendDecideTime = level.time + Q_irand(1000, 2000);
		}
		else
		{
			if (bs->saberDefendDecideTime < level.time)
			{
				if (bs->saberDefending)
				{
					bs->saberDefending = 0;
				}
				else
				{
					bs->saberDefending = 1;
				}

				bs->saberDefendDecideTime = level.time + Q_irand(500, 2000);
			}
		}

		if (bs->frame_Enemy_Len < 54)
		{
			VectorCopy(bs->origin, bs->goalPosition);
			bs->saberBFTime = 0;
		}
		else
		{
			VectorCopy(usethisvec, bs->goalPosition);
		}

		if (bs->currentEnemy && bs->currentEnemy->client)
		{
			if (!BG_SaberInSpecial(bs->currentEnemy->client->ps.saberMove) && bs->frame_Enemy_Len > 90 && bs->saberBFTime > level.time && bs->saberBTime > level.time && bs->beStill < level.time && bs->saberSTime < level.time)
			{
				bs->beStill = level.time + Q_irand(500, 1000);
				bs->saberSTime = level.time + Q_irand(1200, 1800);
			}
			else if (bs->currentEnemy->client->ps.weapon == WP_SABER && bs->frame_Enemy_Len < 80 && ((Q_irand(1, 10) < 8 && bs->saberBFTime < level.time) || bs->saberBTime > level.time || BG_SaberInKata(bs->currentEnemy->client->ps.saberMove) || bs->currentEnemy->client->ps.saberMove == LS_SPINATTACK || bs->currentEnemy->client->ps.saberMove == LS_SPINATTACK_DUAL))
			{
				vec3_t vs;
				vec3_t groundcheck;
				int idealDist;
				int checkIncr = 0;

				VectorSubtract(bs->origin, usethisvec, vs);
				VectorNormalize(vs);

				if (BG_SaberInKata(bs->currentEnemy->client->ps.saberMove) || bs->currentEnemy->client->ps.saberMove == LS_SPINATTACK || bs->currentEnemy->client->ps.saberMove == LS_SPINATTACK_DUAL)
				{
					idealDist = 256;
				}
				else
				{
					idealDist = 64;
				}

				while (checkIncr < idealDist)
				{
					bs->goalPosition[0] = bs->origin[0] + vs[0]*checkIncr;
					bs->goalPosition[1] = bs->origin[1] + vs[1]*checkIncr;
					bs->goalPosition[2] = bs->origin[2] + vs[2]*checkIncr;

					if (bs->saberBTime < level.time)
					{
						bs->saberBFTime = level.time + Q_irand(900, 1300);
						bs->saberBTime = level.time + Q_irand(300, 700);
					}

					VectorCopy(bs->goalPosition, groundcheck);

					groundcheck[2] -= 64;

					trap_Trace(&tr, bs->goalPosition, NULL, NULL, groundcheck, bs->client, MASK_SOLID);
					
					if (tr.fraction == 1.0f)
					{ //don't back off of a ledge
						VectorCopy(usethisvec, bs->goalPosition);
						break;
					}
					checkIncr += 64;
				}
			}
			else if (bs->currentEnemy->client->ps.weapon == WP_SABER && bs->frame_Enemy_Len >= 75)
			{
				bs->saberBFTime = level.time + Q_irand(700, 1300);
				bs->saberBTime = 0;
			}
		}

		/*AngleVectors(bs->viewangles, NULL, fwd, NULL);

		if (bs->meleeStrafeDir)
		{
			bs->goalPosition[0] += fwd[0]*16;
			bs->goalPosition[1] += fwd[1]*16;
			bs->goalPosition[2] += fwd[2]*16;
		}
		else
		{
			bs->goalPosition[0] -= fwd[0]*16;
			bs->goalPosition[1] -= fwd[1]*16;
			bs->goalPosition[2] -= fwd[2]*16;
		}*/
	}
	else if (bs->frame_Enemy_Len <= 56)
	{
		bs->doAttack = 1;
		bs->saberDefending = 0;
	}
}

//should we be "leading" our aim with this weapon? And if
//so, by how much?
float BotWeaponCanLead(bot_state_t *bs)
{
	int weap = bs->cur_ps.weapon;
	if (weap == WP_STUN_BATON)
	{
		return 0.45;
	}
	if (weap == WP_BRYAR_PISTOL)
	{
		return 0.5;
	}
	if (weap == WP_BLASTER)
	{
		return 0.35;
	}
	if (weap == WP_DISRUPTOR)
	{
		return 0.5;
	}
	if (weap == WP_BOWCASTER)
	{
		return 0.5;
	}
	if (weap == WP_REPEATER)
	{
		return 0.45;
	}
	if (weap == WP_DEMP2)
	{
		return 0.35;
	}
	if (weap == WP_FLECHETTE)
	{
		return 0.45;
	}
	if (weap == WP_ROCKET_LAUNCHER)
	{
		return 0.7;
	}
	if (weap == WP_CONCUSSION)
	{
		return 0.7;
	}

	if (weap == WP_THERMAL)
	{
		return 0.5;
	}
	if (weap == WP_TRIP_MINE)
	{
		return 0.5;
	}
	if (weap == WP_DET_PACK)
	{
		return 0.5;
	}
	if (weap == WP_BRYAR_OLD)
	{
		return 0.5;
	}

	//[NewWeapons][EnhanceImpliment]
	/*
	if (weap == WP_SBD_ARM)
	{
		return 0.35;
	}
	if (weap == WP_DKA_ARM)
	{
		return 0.35;
	}
	*/
	//[/NewWeapons]
	
	return 0;
}

//offset the desired view angles with aim leading in mind
void BotAimLeading(bot_state_t *bs, vec3_t headlevel, float leadAmount)
{
	//[BotTweaks]
	//Cleans up a bunch of type cast warnings and makes this more accurate at the same time.
	float x;
	//int x;
	//[/BotTweaks]
	vec3_t predictedSpot;
	vec3_t movementVector;
	vec3_t a, ang;
	float vtotal;

	if (!bs->currentEnemy ||
		!bs->currentEnemy->client)
	{
		return;
	}

	if (!bs->frame_Enemy_Len)
	{
		return;
	}

	vtotal = 0;

	if (bs->currentEnemy->client->ps.velocity[0] < 0)
	{
		vtotal += -bs->currentEnemy->client->ps.velocity[0];
	}
	else
	{
		vtotal += bs->currentEnemy->client->ps.velocity[0];
	}

	if (bs->currentEnemy->client->ps.velocity[1] < 0)
	{
		vtotal += -bs->currentEnemy->client->ps.velocity[1];
	}
	else
	{
		vtotal += bs->currentEnemy->client->ps.velocity[1];
	}

	if (bs->currentEnemy->client->ps.velocity[2] < 0)
	{
		vtotal += -bs->currentEnemy->client->ps.velocity[2];
	}
	else
	{
		vtotal += bs->currentEnemy->client->ps.velocity[2];
	}

	//G_Printf("Leadin target with a velocity total of %f\n", vtotal);

	VectorCopy(bs->currentEnemy->client->ps.velocity, movementVector);

	VectorNormalize(movementVector);

	x = bs->frame_Enemy_Len*leadAmount; //hardly calculated with an exact science, but it works

	if (vtotal > 400)
	{
		vtotal = 400;
	}

	if (vtotal)
	{
		x = (bs->frame_Enemy_Len*0.9)*leadAmount*(vtotal*0.0012); //hardly calculated with an exact science, but it works
	}
	else
	{
		x = (bs->frame_Enemy_Len*0.9)*leadAmount; //hardly calculated with an exact science, but it works
	}

	predictedSpot[0] = headlevel[0] + (movementVector[0]*x);
	predictedSpot[1] = headlevel[1] + (movementVector[1]*x);
	predictedSpot[2] = headlevel[2] + (movementVector[2]*x);

	VectorSubtract(predictedSpot, bs->eye, a);
	vectoangles(a, ang);
	VectorCopy(ang, bs->goalAngles);
}

//wobble our aim around based on our sk1llz
void BotAimOffsetGoalAngles(bot_state_t *bs)
{
	int i;
	float accVal;
	i = 0;

	if (bs->skills.perfectaim)
	{
		return;
	}

	if (bs->aimOffsetTime > level.time)
	{
		if (bs->aimOffsetAmtYaw)
		{
			bs->goalAngles[YAW] += bs->aimOffsetAmtYaw;
		}

		if (bs->aimOffsetAmtPitch)
		{
			bs->goalAngles[PITCH] += bs->aimOffsetAmtPitch;
		}
		
		while (i <= 2)
		{
			if (bs->goalAngles[i] > 360)
			{
				bs->goalAngles[i] -= 360;
			}

			if (bs->goalAngles[i] < 0)
			{
				bs->goalAngles[i] += 360;
			}

			i++;
		}
		return;
	}

	accVal = bs->skills.accuracy/bs->settings.skill;

	// Bot style: accuracy variance (lower accVal == more accurate).
	// Keep BOT_DEFAULT identical to stock behaviour.
	if (bs->settings.botType == BOT_AOTC)
	{
		accVal *= 0.75f;
	}
	else if (bs->settings.botType == BOT_HYBRID)
	{
		accVal *= 0.82f;
	}
	else if (bs->settings.botType == BOT_TAB)
	{
		accVal *= 0.90f;
	}


	if (bs->currentEnemy && BotMindTricked(bs->client, bs->currentEnemy->s.number))
	{ //having to judge where they are by hearing them, so we should be quite inaccurate here
		accVal *= 7;

		if (accVal < 30)
		{
			accVal = 30;
		}
	}

	if (bs->revengeEnemy && bs->revengeHateLevel &&
		bs->currentEnemy == bs->revengeEnemy)
	{ //bot becomes more skilled as anger level raises
		accVal = accVal/bs->revengeHateLevel;
	}

	if (bs->currentEnemy && bs->frame_Enemy_Vis)
	{ //assume our goal is aiming at the enemy, seeing as he's visible and all
		if (!bs->currentEnemy->s.pos.trDelta[0] &&
			!bs->currentEnemy->s.pos.trDelta[1] &&
			!bs->currentEnemy->s.pos.trDelta[2])
		{
			accVal = 0; //he's not even moving, so he shouldn't really be hard to hit.
		}
		else
		{
			accVal += accVal*0.25; //if he's moving he's this much harder to hit
		}

		if (g_entities[bs->client].s.pos.trDelta[0] ||
			g_entities[bs->client].s.pos.trDelta[1] ||
			g_entities[bs->client].s.pos.trDelta[2])
		{
			accVal += accVal*0.15; //make it somewhat harder to aim if we're moving also
		}
	}

	//[PlayerClasses][EnhanceImpliment]
	//if (mod_classes.integer == 2)
	//	accVal *= 0.15; // Much more accurate(smart) in GCLASS games...
	//[/PlayerClasses]

	if (accVal > 90)
	{
		accVal = 90;
	}
	if (accVal < 1)
	{
		accVal = 0;
	}

	if (!accVal)
	{
		bs->aimOffsetAmtYaw = 0;
		bs->aimOffsetAmtPitch = 0;
		return;
	}

	if (rand()%10 <= 5)
	{
		bs->aimOffsetAmtYaw = rand()%(int)accVal;
	}
	else
	{
		bs->aimOffsetAmtYaw = -(rand()%(int)accVal);
	}

	if (rand()%10 <= 5)
	{
		bs->aimOffsetAmtPitch = rand()%(int)accVal;
	}
	else
	{
		bs->aimOffsetAmtPitch = -(rand()%(int)accVal);
	}

	bs->aimOffsetTime = level.time + rand()%500 + 200;
}

// ============================================================================
// Thermal safety helpers (team damage / suicide avoidance)
// Keep this conservative: only used for WP_THERMAL decisions.
// ============================================================================

// Forward declaration (definition appears later in this file)
qboolean BotWeaponSelectable(bot_state_t *bs, int weapon);

#define BOT_THERMAL_FRIENDLY_SAFE_RADIUS 300.0f

static qboolean BotTeammateNearPoint(bot_state_t *bs, const vec3_t point, float radius)
{
    int i;
    int myTeam;
    float r2;
    vec3_t d;

    if (!g_entities[bs->client].client)
    {
        return qfalse;
    }

    myTeam = g_entities[bs->client].client->sess.sessionTeam;
    // In FFA there are no teammates to protect.
    if (myTeam == TEAM_FREE || myTeam == TEAM_SPECTATOR)
    {
        return qfalse;
    }

    r2 = radius * radius;

    for (i = 0; i < level.maxclients; i++)
    {
        gentity_t *ent;

        if (i == bs->client)
        {
            continue;
        }

        ent = &g_entities[i];

        if (!ent->inuse || !ent->client)
        {
            continue;
        }

        if (ent->health <= 0)
        {
            continue;
        }

        if (ent->client->sess.sessionTeam != myTeam)
        {
            continue;
        }

        VectorSubtract(ent->r.currentOrigin, point, d);
        if (VectorLengthSquared(d) <= r2)
        {
            return qtrue;
        }
    }

    return qfalse;
}

// Pick a safe non-explosive weapon to bail out of a bad thermal throw.
// This is only used as an emergency cancel and is intentionally simple.
static int BotPickSafeWeapon(bot_state_t *bs)
{
    if (BotWeaponSelectable(bs, WP_BLASTER))
        return WP_BLASTER;
    if (BotWeaponSelectable(bs, WP_BRYAR_PISTOL))
        return WP_BRYAR_PISTOL;
    if (BotWeaponSelectable(bs, WP_REPEATER))
        return WP_REPEATER;
    if (BotWeaponSelectable(bs, WP_DEMP2))
        return WP_DEMP2;
    if (BotWeaponSelectable(bs, WP_DISRUPTOR))
        return WP_DISRUPTOR;
    if (BotWeaponSelectable(bs, WP_BOWCASTER))
        return WP_BOWCASTER;
    if (BotWeaponSelectable(bs, WP_STUN_BATON))
        return WP_STUN_BATON;

    return 0;
}


static qboolean BotUsesBurstAlt(const bot_state_t *bs)
{
	const int weaponOptions = bs->cur_ps.eFlags &
		(EF_WP_OPTION_2|EF_WP_OPTION_3|EF_WP_OPTION_4);

	if(bs->cur_ps.weapon == WP_BRYAR_PISTOL)
	{
		return (weaponOptions == EF_WP_OPTION_2 ||
			weaponOptions == (EF_WP_OPTION_2|EF_WP_OPTION_4));
	}

	if(bs->cur_ps.weapon == WP_BRYAR_OLD)
	{
		return (weaponOptions == EF_WP_OPTION_2);
	}

	return qfalse;
}

//do we want to alt fire with this weapon?
int ShouldSecondaryFire(bot_state_t *bs)
{
	int weap;
	int dif;
	float rTime;

	weap = bs->cur_ps.weapon;

	if (bs->cur_ps.ammo[weaponData[weap].ammoIndex] < weaponData[weap].altEnergyPerShot)
	{
		return 0;
	}

	/*
	Release while the three-round burst state is active.  Pmove completes the
	remaining rounds automatically, then the bot may pull the trigger again.
	*/
	if(BotUsesBurstAlt(bs) && bs->cur_ps.weaponChargeSubtractTime < 0)
	{
		return 2;
	}

	if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT && bs->cur_ps.weapon == WP_ROCKET_LAUNCHER)
	{
		float heldTime = (level.time - bs->cur_ps.weaponChargeTime);

		rTime = bs->cur_ps.rocketLockTime;

		if (rTime < 1)
		{
			rTime = bs->cur_ps.rocketLastValidTime;
		}

		if (heldTime > 5000)
		{ //just give up and release it if we can't manage a lock in 5 seconds
			return 2;
		}

		if (rTime > 0)
		{
			//[Linux] type cast g++ fix
			dif = (int) (( level.time - rTime ) / ( 1200.0f / 16.0f ));
			//dif = ( level.time - rTime ) / ( 1200.0f / 16.0f );
			//[/Linux]
			
			if (dif >= 10)
			{
				return 2;
			}
			else if (bs->frame_Enemy_Len > 250)
			{
				return 1;
			}
		}
		else if (bs->frame_Enemy_Len > 250)
		{
			return 1;
		}
	}
	else if ((bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT) && (level.time - bs->cur_ps.weaponChargeTime) > bs->altChargeTime)
	{
		return 2;
	}
	else if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT)
	{
		return 1;
	}

	if (weap == WP_BRYAR_PISTOL && bs->frame_Enemy_Len < 250)
	{
		return 1;
	}
	else if (weap == WP_BRYAR_OLD && bs->frame_Enemy_Len < 250)
	{
		return 1;
	}
	else if (weap == WP_REPEATER && bs->frame_Enemy_Len < 500 && bs->frame_Enemy_Len > 250)
	{
		return 1;
	}
	else if (weap == WP_FLECHETTE && bs->frame_Enemy_Len < 500 && bs->frame_Enemy_Len > 250)
	{
		return 1;
	}
	else if (weap == WP_BLASTER && bs->frame_Enemy_Len < 250)
	{
		return 1;
	}
	else if (weap == WP_DEMP2 && bs->frame_Enemy_Len < 250)
	{
		return 1;
	}
	else if (weap == WP_CONCUSSION && bs->frame_Enemy_Len < 500 && bs->frame_Enemy_Len > 250)
	{
		return 1;
	}
	else if (weap == WP_ROCKET_LAUNCHER && bs->frame_Enemy_Len > 250)
	{
		return 1;
	}
	else if (weap == WP_STUN_BATON  && bs->frame_Enemy_Len < 128)
	{
		return 1;
	}
	return 0;
}

//standard weapon combat routines
//RACC - This really only sets the attack buttons when appropiate.
int CombatBotAI(bot_state_t *bs, float thinktime)
{
	vec3_t eorg, a;
	int secFire;
	float fovcheck;

	if (!bs->currentEnemy)
	{
		return 0;
	}

	if (bs->currentEnemy->client)
	{
		VectorCopy(bs->currentEnemy->client->ps.origin, eorg);
	}
	else
	{
		VectorCopy(bs->currentEnemy->s.origin, eorg);
	}

	VectorSubtract(eorg, bs->eye, a);
	vectoangles(a, a);

	if (BotGetWeaponRange(bs) == BWEAPONRANGE_SABER)
	{
		if (bs->frame_Enemy_Len <= SABER_ATTACK_RANGE)
		{
			bs->doAttack = 1;
		}
	}
	else if (BotGetWeaponRange(bs) == BWEAPONRANGE_MELEE)
	{
		if (bs->frame_Enemy_Len <= MELEE_ATTACK_RANGE)
		{
			bs->doAttack = 1;
		}
	}
	else
	{
		if (bs->cur_ps.weapon == WP_THERMAL || bs->cur_ps.weapon == WP_ROCKET_LAUNCHER)
		{ //be careful with the hurty weapons
			fovcheck = 40;

			if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT &&
				bs->cur_ps.weapon == WP_ROCKET_LAUNCHER)
			{ //if we're charging the weapon up then we can hold fire down within a normal fov
				fovcheck = 60;
			}
		}
		else
		{
			fovcheck = 60;
		}

		if (bs->cur_ps.weaponstate == WEAPON_CHARGING ||
			bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT)
		{
			fovcheck = 160;
		}

		if (bs->frame_Enemy_Len < 128)
		{
			fovcheck *= 2;
		}

		if (InFieldOfVision(bs->viewangles, fovcheck, a))
		{
			if (bs->cur_ps.weapon == WP_THERMAL)
			{
				// Team safety: don't throw thermals into teammates.
				// Conservative approximation: treat the enemy position as the likely blast area.
				if (BotTeammateNearPoint(bs, eorg, BOT_THERMAL_FRIENDLY_SAFE_RADIUS))
				{
					// Always block thermal usage this frame when a teammate would be in the blast.
					bs->doAttack = 0;
					bs->doAltAttack = 0;

					// If we're already cooking a thermal, try to cancel by swapping to a safe weapon.
					// This is safer than "holding" indefinitely and risking a hand explosion.
					if (bs->cur_ps.weaponstate == WEAPON_CHARGING || bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT)
					{
						int safe = BotPickSafeWeapon(bs);
						if (safe)
						{
							bs->forceWeaponSelect = safe;
						}
					}
					// Don't start cooking/throwing this frame.
					return 0;
				}
				else
				if (((level.time - bs->cur_ps.weaponChargeTime) < (bs->frame_Enemy_Len*2) &&
					(level.time - bs->cur_ps.weaponChargeTime) < 4000 &&
					bs->frame_Enemy_Len > 64) ||
					(bs->cur_ps.weaponstate != WEAPON_CHARGING &&
					bs->cur_ps.weaponstate != WEAPON_CHARGING_ALT))
				{
					if (bs->cur_ps.weaponstate != WEAPON_CHARGING && bs->cur_ps.weaponstate != WEAPON_CHARGING_ALT)
					{
						if (bs->frame_Enemy_Len > 512 && bs->frame_Enemy_Len < 800)
						{
							bs->doAltAttack = 1;
							//bs->doAttack = 1;
						}
						else
						{
							bs->doAttack = 1;
							//bs->doAltAttack = 1;
						}
					}

					if (bs->cur_ps.weaponstate == WEAPON_CHARGING)
					{
						bs->doAttack = 1;
					}
					else if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT)
					{
						bs->doAltAttack = 1;
					}
				}
			}
			else
			{
				secFire = ShouldSecondaryFire(bs);

				if (bs->cur_ps.weaponstate != WEAPON_CHARGING_ALT &&
					bs->cur_ps.weaponstate != WEAPON_CHARGING)
				{
					bs->altChargeTime = Q_irand(500, 1000);
				}

				if (secFire == 1)
				{
					bs->doAltAttack = 1;
				}
				else if (!secFire)
				{
					if (bs->cur_ps.weapon != WP_THERMAL)
					{
						if (bs->cur_ps.weaponstate != WEAPON_CHARGING ||
							bs->altChargeTime > (level.time - bs->cur_ps.weaponChargeTime))
						{
							bs->doAttack = 1;
						}
					}
					else
					{
						bs->doAttack = 1;
					}
				}

				if (secFire == 2)
				{ //released a charge
					return 1;
				}
			}
		}
	}

	return 0;
}

//we messed up and got off the normal path, let's fall
//back to jumping around and turning in random
//directions off walls to see if we can get back to a
//good place.
int BotFallbackNavigation(bot_state_t *bs)
{
	vec3_t b_angle, fwd, trto, mins, maxs;
	trace_t tr;

	if (bs->currentEnemy && bs->frame_Enemy_Vis)
	{
		return 2; //we're busy
	}

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = 0;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 32;

	bs->goalAngles[PITCH] = 0;
	bs->goalAngles[ROLL] = 0;

	VectorCopy(bs->goalAngles, b_angle);

	AngleVectors(b_angle, fwd, NULL, NULL);

	trto[0] = bs->origin[0] + fwd[0]*16;
	trto[1] = bs->origin[1] + fwd[1]*16;
	trto[2] = bs->origin[2] + fwd[2]*16;

	trap_Trace(&tr, bs->origin, mins, maxs, trto, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1)
	{
		vec3_t dropEnd;
		trace_t trDrop;

		VectorCopy(trto, dropEnd);
		dropEnd[2] -= BOT_FATAL_DROP;
		trap_Trace(&trDrop, trto, NULL, NULL, dropEnd, bs->client, MASK_SOLID);

		if (trDrop.fraction == 1.0f)
		{
			bs->goalAngles[YAW] = rand()%360;
			return 0;
		}

		VectorCopy(trto, bs->goalPosition);
		return 1; //success!
	}
	else
	{
		bs->goalAngles[YAW] = rand()%360;
	}

	return 0;
}

int BotTryAnotherWeapon(bot_state_t *bs)
{ //out of ammo, resort to the first weapon we come across that has ammo
	int i;
	i = 1;

	//[NewWeapons][EnhancedImpliment]
	/*
	while (i < MAX_PLAYER_WEAPONS)
	{
		if (BotWeaponSelectable(bs, i))
	*/

	while (i < WP_NUM_WEAPONS)
	{
		if (BotWeaponSelectable(bs, i))
	//[NewWeapons][EnhancedImpliment]
		{
			bs->virtualWeapon = i;
			BotSelectWeapon(bs->client, i);
			//bs->cur_ps.weapon = i;
			//level.clients[bs->client].ps.weapon = i;
			return 1;
		}

		i++;
	}

	if (bs->cur_ps.weapon != 1 && bs->virtualWeapon != 1)
	{ //should always have this.. shouldn't we?
		bs->virtualWeapon = 1;
		BotSelectWeapon(bs->client, 1);
		//bs->cur_ps.weapon = 1;
		//level.clients[bs->client].ps.weapon = 1;
		return 1;
	}

	return 0;
}

//is this weapon available to us?
qboolean BotWeaponSelectable(bot_state_t *bs, int weapon)
{
	if (weapon == WP_NONE)
	{
		return qfalse;
	}

	if (weapon < WP_NONE || weapon >= WP_NUM_WEAPONS)
	{
		return qfalse;
	}

	if (bs->botWeaponWeights[weapon] <= 0 && weapon != WP_MELEE)
	{
		return qfalse;
	}

	//[NewWeapons][EnhancedImpliment]
	/*
	if (G_HasWeapon(bs->cur_ps.clientNum, weapon))
	{
		if (bs->cur_ps.ammo[weaponData[weapon].ammoIndex] >= weaponData[weapon].energyPerShot)
		{
			return qtrue;
		}
	}
	*/


	if (bs->cur_ps.ammo[weaponData[weapon].ammoIndex] >= weaponData[weapon].energyPerShot &&
		(bs->cur_ps.stats[STAT_WEAPONS] & (1 << weapon)))
	{
		return qtrue;
	}
	//[/NewWeapons][EnhancedImpliment]
	
	return qfalse;
}

//select the best weapon we can
int BotSelectIdealWeapon(bot_state_t *bs)
{
	int i;
	int bestweight = -1;
	int bestweapon = 0;

	i = 0;

	//[NewWeapons][EnhancedImpliment]
	/*
	while (i < MAX_PLAYER_WEAPONS)
	{
		if (G_HasWeapon(bs->cur_ps.clientNum, i))
		{
			if (bs->cur_ps.ammo[weaponData[i].ammoIndex] >= weaponData[i].energyPerShot && bs->botWeaponWeights[i] > bestweight)			
			{
				if (i == WP_THERMAL)
				{ //special case..
					if (bs->currentEnemy && bs->frame_Enemy_Len < 700)
					{
						bestweight = bs->botWeaponWeights[i];
						bestweapon = i;
					}
				}
				else
				{
					bestweight = bs->botWeaponWeights[i];
					bestweapon = i;
				}
			}
	*/

	while (i < WP_NUM_WEAPONS)
	{
		if (bs->botWeaponWeights[i] > 0 &&
			bs->cur_ps.ammo[weaponData[i].ammoIndex] >= weaponData[i].energyPerShot &&
			bs->botWeaponWeights[i] > bestweight &&
			(bs->cur_ps.stats[STAT_WEAPONS] & (1 << i)))
		{
			if (i == WP_THERMAL)
			{ //special case..
				if (bs->currentEnemy && bs->frame_Enemy_Len < 700)
				{
					bestweight = bs->botWeaponWeights[i];
					bestweapon = i;
				}
			}
			else
			{
				bestweight = bs->botWeaponWeights[i];
				bestweapon = i;
			}
	//[/NewWeapons][EnhancedImpliment]
		}

		i++;
	}

	if ( bs->currentEnemy && bs->frame_Enemy_Len < 300 &&
		(bestweapon == WP_BRYAR_PISTOL || bestweapon == WP_BLASTER || bestweapon == WP_BOWCASTER) &&
		(bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_SABER)) )
	{
		bestweapon = WP_SABER;
		bestweight = 1;
	}
	//[NewWeapons][EnhancedImpliment]
	/*
	else if ( bs->currentEnemy && bs->frame_Enemy_Len < 300 &&
		(bestweapon == WP_BRYAR_PISTOL || bestweapon == WP_BLASTER || bestweapon == WP_BOWCASTER) &&
		(bs->cur_ps.stats[STAT_WEAPONS4] & (1 << WP_SITH_SWORD)) )
	{
		bestweapon = WP_SITH_SWORD;
		bestweight = 1;
	}
	else if ( bs->currentEnemy && bs->frame_Enemy_Len < 300 &&
		(bestweapon == WP_BRYAR_PISTOL || bestweapon == WP_BLASTER || bestweapon == WP_BOWCASTER) &&
		(bs->cur_ps.stats[STAT_WEAPONS4] & (1 << WP_VIBROBLADE)) )
	{
		bestweapon = WP_VIBROBLADE;
		bestweight = 1;
	}
	*/
	//[/NewWeapons][EnhancedImpliment]
	
	if ( bs->currentEnemy && bs->frame_Enemy_Len > 300 &&
		bs->currentEnemy->client && bs->currentEnemy->client->ps.weapon != WP_SABER &&
		//[NewWeapons][EnhancedImpliment]
		//(bestweapon == WP_SABER || bestweapon == WP_SITH_SWORD || bestweapon == WP_VIBROBLADE) )
		(bestweapon == WP_SABER) )
		//[/NewWeapons][EnhancedImpliment]
	{ //if the enemy is far away, and we have our saber selected, see if we have any good distance weapons instead
		if (BotWeaponSelectable(bs, WP_STUN_BATON))
		{
			bestweapon = WP_STUN_BATON;
			bestweight = 1;
		}
		else if (BotWeaponSelectable(bs, WP_BRYAR_PISTOL))
		{
			bestweapon = WP_BRYAR_PISTOL;
			bestweight = 1;
		}
		else if (BotWeaponSelectable(bs, WP_BLASTER))
		{
			bestweapon = WP_BLASTER;
			bestweight = 1;
		}
		else if (BotWeaponSelectable(bs, WP_DISRUPTOR))
		{
			bestweapon = WP_DISRUPTOR;
			bestweight = 1;
		}
		else if (BotWeaponSelectable(bs, WP_BOWCASTER))
		{
			bestweapon = WP_BOWCASTER;
			bestweight = 1;
		}
		else if (BotWeaponSelectable(bs, WP_REPEATER))
		{
			bestweapon = WP_REPEATER;
			bestweight = 1;
		}	
		else if (BotWeaponSelectable(bs, WP_DEMP2))
		{
			bestweapon = WP_DEMP2;
			bestweight = 1;
		}
		else if (BotWeaponSelectable(bs, WP_FLECHETTE))
		{
			bestweapon = WP_FLECHETTE;
			bestweight = 1;
		}	
		else if (BotWeaponSelectable(bs, WP_ROCKET_LAUNCHER))
		{
			bestweapon = WP_ROCKET_LAUNCHER;
			bestweight = 1;
		}
		else if (BotWeaponSelectable(bs, WP_CONCUSSION))
		{
			bestweapon = WP_CONCUSSION;
			bestweight = 1;
		}		
		else if (BotWeaponSelectable(bs, WP_BRYAR_OLD))
		{
			bestweapon = WP_BRYAR_OLD;
			bestweight = 1;
		}
		//[NewWeapons][EnhancedImpliment]
		/*
		else if (BotWeaponSelectable(bs, WP_NABOO_S5))
		{
			bestweapon = WP_NABOO_S5;
			bestweight = 1;
		}
		else if (BotWeaponSelectable(bs, WP_CLONE_BLASTER))
		{
			bestweapon = WP_CLONE_BLASTER;
			bestweight = 1;
		}
		else if (BotWeaponSelectable(bs, WP_DROID_BLASTER))
		{
			bestweapon = WP_DROID_BLASTER;
			bestweight = 1;
		}
		*/
		//[/NewWeapons][EnhancedImpliment]
	}

	//assert(bs->cur_ps.weapon > 0 && bestweapon > 0);

	if (bestweight != -1 && bs->cur_ps.weapon != bestweapon && bs->virtualWeapon != bestweapon)
	{
		bs->virtualWeapon = bestweapon;
		BotSelectWeapon(bs->client, bestweapon);
		//bs->cur_ps.weapon = bestweapon;
		//level.clients[bs->client].ps.weapon = bestweapon;
		return 1;
	}

	if (bestweight == -1)
	{
		/*
		 * All bot-file weapon weights may legitimately be zero now.
		 * Do not choose an arbitrary zero-weight/base weapon, but also do
		 * not leave the bot in an invalid weapon state. Use its current
		 * valid weapon, or the defensive melee fallback if melee is owned.
		 */
		if (bs->cur_ps.weapon > WP_NONE && bs->cur_ps.weapon < WP_NUM_WEAPONS &&
			(bs->cur_ps.stats[STAT_WEAPONS] & (1 << bs->cur_ps.weapon)))
		{
			return 0;
		}

		if ((bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_MELEE)) &&
			bs->cur_ps.weapon != WP_MELEE && bs->virtualWeapon != WP_MELEE)
		{
			bs->virtualWeapon = WP_MELEE;
			BotSelectWeapon(bs->client, WP_MELEE);
			return 1;
		}

		return 0;
	}

	//assert(bs->cur_ps.weapon > 0);

	return 0;
}

//check/select the chosen weapon
int BotSelectChoiceWeapon(bot_state_t *bs, int weapon, int doselection)
{ //if !doselection then bot will only check if he has the specified weapon and return 1 (yes) or 0 (no)
	int i;
	int hasit = 0;

	if (!bs || weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS)
	{
		return 0;
	}

	if (bs->botWeaponWeights[weapon] <= 0 && weapon != WP_MELEE)
	{
		return 0;
	}

	i = 0;

	//[NewWeapons][EnhancedImpliment]
	/*
	while (i < MAX_PLAYER_WEAPONS)
	{
		if (G_HasWeapon(bs->cur_ps.clientNum, i))
		{
			if (BG_Is_Staff_Weapon(bs->cur_ps.weapon) 
				|| (bs->cur_ps.ammo[weaponData[i].ammoIndex] > weaponData[i].energyPerShot && i == weapon) )			
			{
				hasit = 1;
				break;
			}
		}
	}
	*/
	
	while (i < WP_NUM_WEAPONS)
	{
		//[TABBot]
		//Fixing this so you can select zero ammo weapons (like the saber and melee weapons)
		if (bs->cur_ps.ammo[weaponData[i].ammoIndex] >= weaponData[i].energyPerShot &&
		//if (bs->cur_ps.ammo[weaponData[i].ammoIndex] > weaponData[i].energyPerShot &&
		//[TABBot]		
			i == weapon &&
			(bs->cur_ps.stats[STAT_WEAPONS] & (1 << i)))
		{
			hasit = 1;
			break;
	//[/NewWeapons]
		}

		i++;
	}

	if (hasit && bs->cur_ps.weapon != weapon && doselection && bs->virtualWeapon != weapon)
	{
		bs->virtualWeapon = weapon;
		BotSelectWeapon(bs->client, weapon);
		//bs->cur_ps.weapon = weapon;
		//level.clients[bs->client].ps.weapon = weapon;
		return 2;
	}

	if (hasit)
	{
		return 1;
	}

	return 0;
}


//[BotTweaks]
//This function is never used
/*
//override our standard weapon choice with a melee weapon
int BotSelectMelee(bot_state_t *bs)
{
	if (bs->cur_ps.weapon != 1 && bs->virtualWeapon != 1)
	{
		bs->virtualWeapon = 1;
		BotSelectWeapon(bs->client, 1);
		//bs->cur_ps.weapon = 1;
		//level.clients[bs->client].ps.weapon = 1;
		return 1;
	}

	return 0;
}
*/
//[/BotTweaks]

//See if we our in love with the potential bot.
int GetLoveLevel(bot_state_t *bs, bot_state_t *love)
{
	int i = 0;
	const char *lname = NULL;

	if (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_FFA || g_gametype.integer == GT_POWERDUEL)
	{ //There is no love in 1-on-1
		return 0;
	}

	if (!bs || !love || !g_entities[love->client].client)
	{
		return 0;
	}

	if (!bs->lovednum)
	{
		return 0;
	}

	if (!bot_attachments.integer)
	{
		return 1;
	}

	lname = g_entities[love->client].client->pers.netname;

	if (!lname)
	{
		return 0;
	}

	while (i < bs->lovednum)
	{
		if (strcmp(bs->loved[i].name, lname) == 0)
		{
			return bs->loved[i].level;
		}

		i++;
	}

	return 0;
}

//Our loved one was killed. We must become infuriated!
void BotLovedOneDied(bot_state_t *bs, bot_state_t *loved, int lovelevel)
{
	if (!loved->lastHurt || !loved->lastHurt->client ||
		loved->lastHurt->s.number == loved->client)
	{
		return;
	}

	if (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL || g_gametype.integer == GT_FFA)
	{ //There is no love in 1-on-1
		return;
	}

	if (!IsTeamplay())
	{
		if (lovelevel < 2)
		{
			return;
		}
	}
	else if (OnSameTeam(&g_entities[bs->client], loved->lastHurt))
	{ //don't hate teammates no matter what
		return;
	}

	if (loved->client == loved->lastHurt->s.number)
	{
		return;
	}

	if (bs->client == loved->lastHurt->s.number)
	{ //oops!
		return;
	}
	
	if (!bot_attachments.integer)
	{
		return;
	}

	if (!PassLovedOneCheck(bs, loved->lastHurt))
	{ //a loved one killed a loved one.. you cannot hate them
		bs->chatObject = loved->lastHurt;
		bs->chatAltObject = &g_entities[loved->client];
		BotDoChat(bs, "LovedOneKilledLovedOne", 0);
		return;
	}

	if (bs->revengeEnemy == loved->lastHurt)
	{
		if (bs->revengeHateLevel < bs->loved_death_thresh)
		{
			bs->revengeHateLevel++;

			if (bs->revengeHateLevel == bs->loved_death_thresh)
			{
				//broke into the highest anger level
				//CHAT: Hatred section
				bs->chatObject = loved->lastHurt;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "Hatred", 1);
			}
		}
	}
	else if (bs->revengeHateLevel < bs->loved_death_thresh-1)
	{ //only switch hatred if we don't hate the existing revenge-enemy too much
		//CHAT: BelovedKilled section
		bs->chatObject = &g_entities[loved->client];
		bs->chatAltObject = loved->lastHurt;
		BotDoChat(bs, "BelovedKilled", 0);
		bs->revengeHateLevel = 0;
		bs->revengeEnemy = loved->lastHurt;
	}
}

void BotDeathNotify(bot_state_t *bs)
{ //in case someone has an emotional attachment to us, we'll notify them
	int i = 0;
	int ltest = 0;

	while (i < MAX_CLIENTS)
	{
		if (botstates[i] && botstates[i]->lovednum)
		{
			ltest = 0;
			while (ltest < botstates[i]->lovednum)
			{
				if (strcmp(level.clients[bs->client].pers.netname, botstates[i]->loved[ltest].name) == 0)
				{
					BotLovedOneDied(botstates[i], bs, botstates[i]->loved[ltest].level);
					break;
				}

				ltest++;
			}
		}

		i++;
	}
}

//perform strafe trace checks
void StrafeTracing(bot_state_t *bs)
{
	vec3_t mins, maxs;
	vec3_t right, rorg, drorg;
	trace_t tr;

	mins[0] = -15;
	mins[1] = -15;
	//mins[2] = -24;
	mins[2] = -22;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 32;

	AngleVectors(bs->viewangles, NULL, right, NULL);

	if (bs->meleeStrafeDir)
	{
		rorg[0] = bs->origin[0] - right[0]*32;
		rorg[1] = bs->origin[1] - right[1]*32;
		rorg[2] = bs->origin[2] - right[2]*32;
	}
	else
	{
		rorg[0] = bs->origin[0] + right[0]*32;
		rorg[1] = bs->origin[1] + right[1]*32;
		rorg[2] = bs->origin[2] + right[2]*32;
	}

	trap_Trace(&tr, bs->origin, mins, maxs, rorg, bs->client, MASK_SOLID);

	if (tr.fraction != 1)
	{
		bs->meleeStrafeDisable = level.time + Q_irand(500, 1500);
		//[BotTweak]
		//no need to do another trace when you already obstructed path
		return;
		//[/BotTweak]
	}

	VectorCopy(rorg, drorg);

	drorg[2] -= 32;

	trap_Trace(&tr, rorg, NULL, NULL, drorg, bs->client, MASK_SOLID);

	if (tr.fraction == 1)
	{ //this may be a dangerous ledge, so don't strafe over it just in case
		bs->meleeStrafeDisable = level.time + Q_irand(500, 1500);
	}
}

//doing primary weapon fire
int PrimFiring(bot_state_t *bs)
{
	if (bs->cur_ps.weaponstate != WEAPON_CHARGING &&
		bs->doAttack)
	{
		return 1;
	}

	if (bs->cur_ps.weaponstate == WEAPON_CHARGING &&
		!bs->doAttack)
	{
		return 1;
	}

	return 0;
}

//should we keep our primary weapon from firing?
int KeepPrimFromFiring(bot_state_t *bs)
{
	if (bs->cur_ps.weaponstate != WEAPON_CHARGING &&
		bs->doAttack)
	{
		bs->doAttack = 0;
	}

	if (bs->cur_ps.weaponstate == WEAPON_CHARGING &&
		!bs->doAttack)
	{
		bs->doAttack = 1;
	}

	return 0;
}

//doing secondary weapon fire
int AltFiring(bot_state_t *bs)
{
	//[SaberSys]
		if (bs->cur_ps.weaponstate != WEAPON_CHARGING_ALT &&
			(bs->doAltAttack || bs->doSaberThrow))
	{
		return 1;
	}

	if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT &&
		!(bs->doAltAttack || bs->doSaberThrow))
	{
		return 1;
	}
	/*
	if (bs->cur_ps.weaponstate != WEAPON_CHARGING_ALT &&
		bs->doAltAttack)
	{
		return 1;
	}

	if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT &&
		!bs->doAltAttack)
	{
		return 1;
	}
	*/
	//[/SaberSys]

	return 0;
}

//should we keep our alt from firing?
int KeepAltFromFiring(bot_state_t *bs)
{
	
	//[SaberSys]
	if (bs->cur_ps.weaponstate != WEAPON_CHARGING_ALT &&
		(bs->doAltAttack || bs->doSaberThrow))
	{
		bs->doAltAttack = 0;
		bs->doSaberThrow = 0;
	}

	if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT &&
		!(bs->doAltAttack || bs->doSaberThrow))
	{
		bs->doAltAttack = 1;
	}
	/*
	if (bs->cur_ps.weaponstate != WEAPON_CHARGING_ALT &&
		bs->doAltAttack)
	{
		bs->doAltAttack = 0;
	}

	if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT &&
		!bs->doAltAttack)
	{
		bs->doAltAttack = 1;
	}
	*/
	//[/SaberSys]

	return 0;
}

//Try not to shoot our friends in the back. Or in the face. Or anywhere, really.
gentity_t *CheckForFriendInLOF(bot_state_t *bs)
{
	vec3_t fwd;
	vec3_t trfrom, trto;
	vec3_t mins, maxs;
	gentity_t *trent;
	trace_t tr;

	//[BotTweaks] UNIQUEFIXME - this looks sloopy 
	//and unnessicary?  need this?
	/*
	if (bs->cur_ps.clientNum > MAX_GENTITIES || bs->cur_ps.clientNum < 0 || !bs || !bs->client || bs->client > MAX_GENTITIES || bs->client < 0)
		return NULL;
	*/
	//[/BotTweaks]

	mins[0] = -3;
	mins[1] = -3;
	mins[2] = -3;

	maxs[0] = 3;
	maxs[1] = 3;
	maxs[2] = 3;

	AngleVectors(bs->viewangles, fwd, NULL, NULL);

	VectorCopy(bs->eye, trfrom);

	trto[0] = trfrom[0] + fwd[0]*2048;
	trto[1] = trfrom[1] + fwd[1]*2048;
	trto[2] = trfrom[2] + fwd[2]*2048;

	trap_Trace(&tr, trfrom, mins, maxs, trto, bs->client, MASK_PLAYERSOLID);

	if (tr.fraction != 1 && tr.entityNum <= MAX_CLIENTS)
	{
		trent = &g_entities[tr.entityNum];

		if (trent && trent->client)
		{
			if (IsTeamplay() && OnSameTeam(&g_entities[bs->client], trent))
			{
				return trent;
			}

			if (botstates[trent->s.number] && GetLoveLevel(bs, botstates[trent->s.number]) > 1)
			{
				return trent;
			}
		}
	}

	return NULL;
}

void BotScanForLeader(bot_state_t *bs)
{ //bots will only automatically obtain a leader if it's another bot using this method.
	int i = 0;
	gentity_t *ent;

	if (bs->isSquadLeader)
	{
		return;
	}

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->client && botstates[i] && botstates[i]->isSquadLeader && bs->client != i)
		{
			if (OnSameTeam(&g_entities[bs->client], ent))
			{
				bs->squadLeader = ent;
				break;
			}
			if (GetLoveLevel(bs, botstates[i]) > 1 && !IsTeamplay())
			{ //ignore love status regarding squad leaders if we're in teamplay
				bs->squadLeader = ent;
				break;
			}
		}

		i++;
	}
}

//w3rd to the p33pz.
void BotReplyGreetings(bot_state_t *bs)
{
	int i = 0;
	int numhello = 0;

	while (i < MAX_CLIENTS)
	{
		if (botstates[i] &&
			botstates[i]->canChat &&
			i != bs->client)
		{
			botstates[i]->chatObject = &g_entities[bs->client];
			botstates[i]->chatAltObject = NULL;
			if (BotDoChat(botstates[i], "ResponseGreetings", 0))
			{
				numhello++;
			}
		}

		if (numhello > 3)
		{ //don't let more than 4 bots say hello at once
			return;
		}

		i++;
	}
}

//try to move in to grab a nearby flag
void CTFFlagMovement(bot_state_t *bs)
{
	int diddrop = 0;
	gentity_t *desiredDrop = NULL;
	vec3_t a, mins, maxs;
	trace_t tr;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -7;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 7;

	if (bs->wantFlag && (bs->wantFlag->flags & FL_DROPPED_ITEM))
	{
		if (bs->staticFlagSpot[0] == bs->wantFlag->s.pos.trBase[0] &&
			bs->staticFlagSpot[1] == bs->wantFlag->s.pos.trBase[1] &&
			bs->staticFlagSpot[2] == bs->wantFlag->s.pos.trBase[2])
		{
			VectorSubtract(bs->origin, bs->wantFlag->s.pos.trBase, a);

			if (VectorLength(a) <= BOT_FLAG_GET_DISTANCE)
			{
				VectorCopy(bs->wantFlag->s.pos.trBase, bs->goalPosition);
				return;
			}
			else
			{
				bs->wantFlag = NULL;
			}
		}
		else
		{
			bs->wantFlag = NULL;
		}
	}
	else if (bs->wantFlag)
	{
		bs->wantFlag = NULL;
	}

	if (flagRed && flagBlue)
	{
		if (bs->wpDestination == flagRed ||
			bs->wpDestination == flagBlue)
		{
			if (bs->wpDestination == flagRed && droppedRedFlag && (droppedRedFlag->flags & FL_DROPPED_ITEM) && droppedRedFlag->classname && strcmp(droppedRedFlag->classname, "freed") != 0)
			{
				desiredDrop = droppedRedFlag;
				diddrop = 1;
			}
			if (bs->wpDestination == flagBlue && droppedBlueFlag && (droppedBlueFlag->flags & FL_DROPPED_ITEM) && droppedBlueFlag->classname && strcmp(droppedBlueFlag->classname, "freed") != 0)
			{
				desiredDrop = droppedBlueFlag;
				diddrop = 1;
			}

			if (diddrop && desiredDrop)
			{
				VectorSubtract(bs->origin, desiredDrop->s.pos.trBase, a);

				if (VectorLength(a) <= BOT_FLAG_GET_DISTANCE)
				{
					trap_Trace(&tr, bs->origin, mins, maxs, desiredDrop->s.pos.trBase, bs->client, MASK_SOLID);

					if (tr.fraction == 1 || tr.entityNum == desiredDrop->s.number)
					{
						VectorCopy(desiredDrop->s.pos.trBase, bs->goalPosition);
						VectorCopy(desiredDrop->s.pos.trBase, bs->staticFlagSpot);
						return;
					}
				}
			}
		}
	}
}

//see if we want to make our detpacks blow up
void BotCheckDetPacks(bot_state_t *bs)
{
	gentity_t *dp = NULL;
	gentity_t *myDet = NULL;
	vec3_t a;
	float enLen;
	float myLen;

	while ( (dp = G_Find( dp, FOFS(classname), "detpack") ) != NULL )
	{
		if (dp && dp->parent && dp->parent->s.number == bs->client)
		{
			myDet = dp;
			break;
		}
	}

	if (!myDet)
	{
		return;
	}

	if (!bs->currentEnemy || !bs->currentEnemy->client || !bs->frame_Enemy_Vis)
	{ //require the enemy to be visilbe just to be fair..

		//unless..
		if (bs->currentEnemy && bs->currentEnemy->client &&
			(level.time - bs->plantContinue) < 5000)
		{ //it's a fresh plant (within 5 seconds) so we should be able to guess
			goto stillmadeit;
		}
		return;
	}

stillmadeit:

	VectorSubtract(bs->currentEnemy->client->ps.origin, myDet->s.pos.trBase, a);
	enLen = VectorLength(a);

	VectorSubtract(bs->origin, myDet->s.pos.trBase, a);
	myLen = VectorLength(a);

	if (enLen > myLen)
	{
		return;
	}

	if (enLen < BOT_PLANT_BLOW_DISTANCE && OrgVisible(bs->currentEnemy->client->ps.origin, myDet->s.pos.trBase, bs->currentEnemy->s.number))
	{ //we could just call the "blow all my detpacks" function here, but I guess that's cheating.
		bs->plantKillEmAll = level.time + 500;
	}
}

//see if it would be beneficial at this time to use one of our inv items
int BotUseInventoryItem(bot_state_t *bs)
{
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SHIELD))
	{
		if (bs->currentEnemy && bs->frame_Enemy_Vis && bs->runningToEscapeThreat)
		{ //this will (hopefully) result in the bot placing the shield down while facing
		  //the enemy and running away
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_SHIELD, IT_HOLDABLE);
			goto wantuseitem;
		}
	}

	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SENTRY_GUN))
	{
		if (bs->currentEnemy && bs->frame_Enemy_Vis && bs->runningToEscapeThreat)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_SENTRY_GUN, IT_HOLDABLE);
			goto wantuseitem;
		}
	}
	
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
	{
		if (bs->runningToEscapeThreat)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_JETPACK, IT_HOLDABLE);
			goto wantuseitem;
		}
		if (bs->iHaveNoIdeaWhereIAmGoing)
		{ 
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_JETPACK, IT_HOLDABLE);
			goto wantuseitem;
		}
		if (bs->cur_ps.groundEntityNum == ENTITYNUM_NONE)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_JETPACK, IT_HOLDABLE);
			goto wantuseitem;
		}
		
    if (bs->wpCurrent)
    {
        vec3_t toWP;
        float distToWP;

        VectorSubtract(bs->wpCurrent->origin, bs->origin, toWP);
        distToWP = VectorLength(toWP);

        if (bs->wpCurrent->origin[2] - bs->origin[2] > 128.0f &&
            distToWP < 512.0f)
        {
            bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_JETPACK, IT_HOLDABLE);
            goto wantuseitem;
        }
    }
	}
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_GRAPPLE))
	{
		if (bs->runningToEscapeThreat)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_GRAPPLE, IT_HOLDABLE);
			goto wantuseitem;
		}
		if (bs->iHaveNoIdeaWhereIAmGoing)
		{ 
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_GRAPPLE, IT_HOLDABLE);
			goto wantuseitem;
		}
		if (bs->cur_ps.groundEntityNum == ENTITYNUM_NONE)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_GRAPPLE, IT_HOLDABLE);
			goto wantuseitem;
		}
	}		
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_MEDPAC))
	{
		if (g_entities[bs->client].client->ps.stats[STAT_HEALTH] < g_entities[bs->client].client->ps.stats[STAT_MAX_HEALTH])
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_MEDPAC, IT_HOLDABLE);
			goto wantuseitem;
		}
	}
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SHIELDBOOSTER))
	{
		if (g_entities[bs->client].client->ps.stats[STAT_ARMOR] < g_entities[bs->client].client->ps.stats[STAT_MAX_ARMOR])
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_SHIELDBOOSTER, IT_HOLDABLE);
			goto wantuseitem;
		}
	}	
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SEEKER))
	{
		if (bs->currentEnemy && bs->frame_Enemy_Vis)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_SEEKER, IT_HOLDABLE);
			goto wantuseitem;
		}
	}
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SQUADTEAM))
	{
		if (bs->currentEnemy && bs->frame_Enemy_Vis)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_SQUADTEAM, IT_HOLDABLE);
			goto wantuseitem;
		}
	}	
	
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_CLOAK))
	{
		if (bs->currentEnemy && bs->frame_Enemy_Vis)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_CLOAK, IT_HOLDABLE);
			goto wantuseitem;
		}
	}

	
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SPHERESHIELD))
	{
		if (bs->currentEnemy && bs->frame_Enemy_Vis)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_SPHERESHIELD, IT_HOLDABLE);
			goto wantuseitem;
		}
	}
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_OVERLOAD))
	{
		if (bs->currentEnemy && bs->frame_Enemy_Vis)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_OVERLOAD, IT_HOLDABLE);
			goto wantuseitem;
		}
	}
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_EWEB))
	{
		if (bs->currentEnemy && bs->frame_Enemy_Len > 128)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_EWEB, IT_HOLDABLE);
			goto wantuseitem;
		}
	}	

	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_FLAMETHROWER))
	{
		if (bs->currentEnemy && bs->frame_Enemy_Len < 256)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_FLAMETHROWER, IT_HOLDABLE);
			goto wantuseitem;
		}
	}
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_ELECTROSHOCKER))
	{
		if (bs->currentEnemy && bs->frame_Enemy_Len < 256)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_ELECTROSHOCKER, IT_HOLDABLE);
			goto wantuseitem;
		}
	}


	return 0;

wantuseitem:
	level.clients[bs->client].ps.stats[STAT_HOLDABLE_ITEM] = bs->cur_ps.stats[STAT_HOLDABLE_ITEM];

	return 1;
}

// Extra item "cover" for AOTC/HYBRID: if normal heuristics don't pick an item,
// pick something we already own (covers all holdables). Does NOT grant items.
static int BotStyle_SelectAnyHoldable(bot_state_t *bs)
{
	int bits;
	int tag;
	int itemIndex;
	qboolean hasEnemy;

	if (!bs) { return 0; }
	bits = bs->cur_ps.stats[STAT_HOLDABLE_ITEMS];
	if (!bits) { return 0; }

	hasEnemy = (bs->currentEnemy && bs->currentEnemy->client && bs->frame_Enemy_Vis) ? qtrue : qfalse;

	// Prefer sensible choices first (even if the normal BotUseInventoryItem() didn't trigger).
	if ((bits & (1 << HI_MEDPAC)) &&
		g_entities[bs->client].client &&
		g_entities[bs->client].client->ps.stats[STAT_HEALTH] < g_entities[bs->client].client->ps.stats[STAT_MAX_HEALTH])
	{
		bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_MEDPAC, IT_HOLDABLE);
		level.clients[bs->client].ps.stats[STAT_HOLDABLE_ITEM] = bs->cur_ps.stats[STAT_HOLDABLE_ITEM];
		return 1;
	}
	if ((bits & (1 << HI_SHIELDBOOSTER)) &&
		g_entities[bs->client].client &&
		g_entities[bs->client].client->ps.stats[STAT_ARMOR] < g_entities[bs->client].client->ps.stats[STAT_MAX_ARMOR])
	{
		bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_SHIELDBOOSTER, IT_HOLDABLE);
		level.clients[bs->client].ps.stats[STAT_HOLDABLE_ITEM] = bs->cur_ps.stats[STAT_HOLDABLE_ITEM];
		return 1;
	}

	if (hasEnemy)
	{
		// If we have an enemy, try offensive/defensive gadgets first.
		if (bits & (1 << HI_SEEKER)) { tag = HI_SEEKER; goto pick; }
		if (bits & (1 << HI_OVERLOAD)) { tag = HI_OVERLOAD; goto pick; }
		if (bits & (1 << HI_CLOAK)) { tag = HI_CLOAK; goto pick; }
		if (bits & (1 << HI_SPHERESHIELD)) { tag = HI_SPHERESHIELD; goto pick; }
		if (bits & (1 << HI_SHIELD)) { tag = HI_SHIELD; goto pick; }
		if (bits & (1 << HI_SENTRY_GUN)) { tag = HI_SENTRY_GUN; goto pick; }
		if (bits & (1 << HI_EWEB)) { tag = HI_EWEB; goto pick; }
		if (bits & (1 << HI_FLAMETHROWER)) { tag = HI_FLAMETHROWER; goto pick; }
		if (bits & (1 << HI_ELECTROSHOCKER)) { tag = HI_ELECTROSHOCKER; goto pick; }
	}

	// Fallback: pick *any* holdable bit we own that maps to a valid item index.
	for (tag = 0; tag < 32; tag++)
	{
		if (!(bits & (1 << tag)))
			continue;
		itemIndex = BG_GetItemIndexByTag(tag, IT_HOLDABLE);
		if (itemIndex > 0)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = itemIndex;
			level.clients[bs->client].ps.stats[STAT_HOLDABLE_ITEM] = bs->cur_ps.stats[STAT_HOLDABLE_ITEM];
			return 1;
		}
	}

	return 0;

pick:
	itemIndex = BG_GetItemIndexByTag(tag, IT_HOLDABLE);
	if (itemIndex > 0)
	{
		bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = itemIndex;
		level.clients[bs->client].ps.stats[STAT_HOLDABLE_ITEM] = bs->cur_ps.stats[STAT_HOLDABLE_ITEM];
		return 1;
	}
	return 0;
}


//trace forward to see if we can plant a detpack or something
int BotSurfaceNear(bot_state_t *bs)
{
	trace_t tr;
	vec3_t fwd;

	AngleVectors(bs->viewangles, fwd, NULL, NULL);

	fwd[0] = bs->origin[0]+(fwd[0]*64);
	fwd[1] = bs->origin[1]+(fwd[1]*64);
	fwd[2] = bs->origin[2]+(fwd[2]*64);

	trap_Trace(&tr, bs->origin, NULL, NULL, fwd, bs->client, MASK_SOLID);

	if (tr.fraction != 1)
	{
		return 1;
	}

	return 0;
}

//could we block projectiles from the weapon potentially with a light saber?
int BotWeaponBlockable(int weapon)
{
	switch (weapon)
	{
					
	case WP_STUN_BATON:
	case WP_MELEE:
		return 0;
	//[NewWeapons][EnhancedImpliment]
	//case WP_ADV_DISRUPTOR:
	//[/NewWeapons][EnhancedImpliment]
   
	case WP_ROCKET_LAUNCHER:
		return 0;
	case WP_CONCUSSION:
		return 0;	
	case WP_THERMAL:
		return 0;
	case WP_TRIP_MINE:
		return 0;
	//[NewWeapons][EnhancedImpliment]
	//case WP_TRIP_MINE_2:
	//	return 0;
	//[/NewWeapons][EnhancedImpliment]
	case WP_DET_PACK:
		return 0;
	default:
		return 1;
	}
}

void Cmd_EngageDuel_f(gentity_t *ent);
void Cmd_ToggleSaber_f(gentity_t *ent);

//movement overrides
void Bot_SetForcedMovement(int bot, int forward, int right, int up)
{
	bot_state_t *bs;

	bs = botstates[bot];

	if (!bs)
	{ //not a bot
		return;
	}

	if (forward != -1)
	{
		if (bs->forceMove_Forward)
		{
			bs->forceMove_Forward = 0;
		}
		else
		{
			bs->forceMove_Forward = forward;
		}
	}
	if (right != -1)
	{
		if (bs->forceMove_Right)
		{
			bs->forceMove_Right = 0;
		}
		else
		{
			bs->forceMove_Right = right;
		}
	}
	if (up != -1)
	{
		if (bs->forceMove_Up)
		{
			bs->forceMove_Up = 0;
		}
		else
		{
			bs->forceMove_Up = up;
		}
	}
}

extern qboolean WP_SaberCanTurnOffSomeBlades( saberInfo_t *saber );


qboolean TrySwitchWPBranch(bot_state_t* bs)
{
	int   bestIndex = -1;
	float bestScore = 9999999.0f;
	bot_nav_profile_t navp;
	int   i;
	const int prevCommit = (bs && bs->client >= 0 && bs->client < MAX_CLIENTS && botWPCommitUntil[bs->client] > 0)
		? botPrevWPCommitIndex[bs->client]
		: -1;
	const int destIndex = (bs && bs->wpDestination) ? bs->wpDestination->index : -1;

	if (!bs || !bs->wpCurrent)
		return qfalse;

	navp = BotNavProfile(bs);

	for (i = 0; i < bs->wpCurrent->neighbornum; i++)
	{
		int   testIndex;
		float score;
		float distToBot;
		int   trail;

		testIndex = bs->wpCurrent->neighbors[i].num;

		if (testIndex < 0 || testIndex >= gWPNum)
			continue;
		if (!gWPArray[testIndex] || !gWPArray[testIndex]->inuse)
			continue;
		if (!PassWayCheck(bs, testIndex))
			continue;

		// Base score: local closeness (keeps switching responsive).
		distToBot = Distance(gWPArray[testIndex]->origin, bs->origin);
		score = distToBot;

		// BotType navigation flavour: cover bias + jump aversion (movement only).
		{
			const wpobject_t *wpN = gWPArray[testIndex];
			const int isCover = (wpN->flags & (WPFLAG_SNIPEORCAMP|WPFLAG_SNIPEORCAMPSTAND|WPFLAG_NOVIS)) ? 1 : 0;
			const int isJumpy = ((wpN->flags & WPFLAG_JUMP) || wpN->forceJumpTo) ? 1 : 0;

			if (isCover && navp.coverPreference != 0.0f)
				score -= navp.coverPreference * 160.0f;
			else if (!isCover && navp.coverPreference > 0.0f)
				score += navp.coverPreference * 60.0f;

			if (isJumpy && navp.jumpAversion != 0.0f)
				score += navp.jumpAversion * 220.0f;
		}


		// Avoid immediate backtracking (ping-pong) when there are other options.
		if (prevCommit != -1 && testIndex == prevCommit && bs->wpCurrent->neighbornum > 1)
		{
			score += navp.backtrackPenalty;
		}

		// If we have a destination, prefer neighbors that reduce trail distance to it.
		if (destIndex >= 0 && destIndex < gWPNum)
		{
			trail = (int)TotalTrailDistance(testIndex, destIndex, bs);
			if (trail > 0)
			{
				// Scale modestly so we don't overreact to occasional trail calc oddities.
				score += (float)trail * 0.35f;
			}
		}

		// TAB bots like direct lines to the destination; AOTC is ok taking safer indirect routes.
		if (destIndex >= 0 && destIndex < gWPNum && gWPArray[destIndex])
		{
			const float d2 = DistanceSquared(gWPArray[testIndex]->origin, gWPArray[destIndex]->origin);
			if (d2 < (1024.0f * 1024.0f))
			{
				vec3_t mins, maxs;
				mins[0] = mins[1] = -15; mins[2] = -1;
				maxs[0] = maxs[1] =  15; maxs[2] =  1;
				if (OrgVisibleBox(gWPArray[testIndex]->origin, mins, maxs, gWPArray[destIndex]->origin, bs->client))
				{
					if (bs->settings.botType == BOT_TAB)
						score -= 80.0f;
					else if (bs->settings.botType == BOT_AOTC)
						score += 40.0f;
				}
			}
		}

		if (score < bestScore)
		{
			bestScore = score;
			bestIndex = testIndex;
		}
	}

	if (bestIndex != -1)
	{
		bs->wpCurrent = gWPArray[bestIndex];
		return qtrue;
	}

	return qfalse;
}


//the main AI loop.
//please don't be too frightened.
// -----------------------------------------------------------------
// BOT type wrappers
// -----------------------------------------------------------------
// NOTE: These wrappers exist so we can keep a single shared AI core
// (StandardBotAI) while still allowing small behaviour "patches" for
// specific bot types. These patches must NOT modify or replace:
//   - bot skill assignment
//   - weapon/item/force weight tables
// Anything here should be purely tactical / moment-to-moment behaviour.

void TAB_StandardBotAI(bot_state_t *bs, float thinktime)
{
	// TAB bots: Jedi-like flavour is implemented via botType checks in
	// shared code (weapon thresholds, Force gating, saber behaviour, etc.).
	StandardBotAI(bs, thinktime);
}

void AOTC_StandardBotAI(bot_state_t *bs, float thinktime)
{
	// AOTC bots: Gunner-like flavour is implemented via botType checks in
	// shared code.
	StandardBotAI(bs, thinktime);
}

void HYBRID_StandardBotAI(bot_state_t *bs, float thinktime)
{
	// HYBRID bots: midpoint tuning in shared code.
	StandardBotAI(bs, thinktime);
}

void StandardBotAI(bot_state_t *bs, float thinktime)
{
	int wp, enemy;
	int desiredIndex;
	int goalWPIndex;
	int doingFallback = 0;
	int fjHalt;
	vec3_t a, ang, headlevel, eorg, noz_x, noz_y, dif, a_fo;
	float reaction;
	float stylePersonality = 1.0f;
	float bLeadAmount;
	int meleestrafe = 0;
	int useTheForce = 0;
	int forceHostile = 0;
	int cBAI = 0;
	gentity_t *friendInLOF = 0;
	float mLen;
	int visResult = 0;
	int selResult = 0;
	int mineSelect = 0;
	int detSelect = 0;
	vec3_t preFrameGAngles;
	gclient_t *client = g_entities[bs->cur_ps.clientNum].client;

	//RACC - Shut down AI if doing bot routing editting.
	if (gDeactivated)
	{
		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpDirection = 0;
		return;
	}

	//RACC - Shut down AI if spectating.
	if (g_entities[bs->client].inuse &&
		g_entities[bs->client].client &&
		g_entities[bs->client].client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpDirection = 0;
		return;
	}

	// Per-bot style variance (never affects weights/loadouts).
	stylePersonality = BotStyle_GetPersonalityScalar(bs);


#ifndef FINAL_BUILD
	if (bot_getinthecarrr.integer)
	{ //stupid vehicle debug, I tire of having to connect another client to test passengers.
		gentity_t *botEnt = &g_entities[bs->client];

		if (botEnt->inuse && botEnt->client && botEnt->client->ps.m_iVehicleNum)
		{ //in a vehicle, so...
			bs->noUseTime = level.time + 5000;

			if (bot_getinthecarrr.integer != 2)
			{
				BotDirectSafeButtonMove(bs, ACTION_MOVEFORWARD);

				if (bot_getinthecarrr.integer == 3)
				{ //use alt fire
					trap_EA_Alt_Attack(bs->client);
				}
			}
		}
		else
		{ //find one, get in
			int i = 0;
			gentity_t *vehicle = NULL;
			//find the nearest, manned vehicle
			while (i < MAX_GENTITIES)
			{
				vehicle = &g_entities[i];

				if (vehicle->inuse && vehicle->client && vehicle->s.eType == ET_NPC &&
					vehicle->s.NPC_class == CLASS_VEHICLE && vehicle->m_pVehicle &&
					(vehicle->client->ps.m_iVehicleNum || bot_getinthecarrr.integer == 2))
				{ //ok, this is a vehicle, and it has a pilot/passengers
					break;
				}
				i++;
			}
			if (i != MAX_GENTITIES && vehicle)
			{ //broke before end so we must've found something
				vec3_t v;

				VectorSubtract(vehicle->client->ps.origin, bs->origin, v);
				VectorNormalize(v);
				vectoangles(v, bs->goalAngles);
				MoveTowardIdealAngles(bs);
				trap_EA_Move(bs->client, v, 5000.0f);

				if (bs->noUseTime < (level.time-400))
				{
					bs->noUseTime = level.time + 500;
				}
			}
		}

		return;
	}
#endif

	if (bot_forgimmick.integer)
	{
		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpDirection = 0;

		if (bot_forgimmick.integer == 2)
		{ //for debugging saber stuff, this is handy
			trap_EA_Attack(bs->client);
		}

		if (bot_forgimmick.integer == 3)
		{ //for testing cpu usage moving around rmg terrain without AI
			vec3_t mdir;

			VectorSubtract(bs->origin, vec3_origin, mdir);
			VectorNormalize(mdir);
			trap_EA_Attack(bs->client);
			trap_EA_Move(bs->client, mdir, 5000);
		}

		if (bot_forgimmick.integer == 4)
		{ //constantly move toward client 0
			if (g_entities[0].client && g_entities[0].inuse)
			{
				vec3_t mdir;

				VectorSubtract(g_entities[0].client->ps.origin, bs->origin, mdir);
				VectorNormalize(mdir);
				trap_EA_Move(bs->client, mdir, 5000);
			}
		}

		if (bs->forceMove_Forward)
		{
			if (bs->forceMove_Forward > 0)
			{
				BotDirectSafeButtonMove(bs, ACTION_MOVEFORWARD);
			}
			else
			{
				BotDirectSafeButtonMove(bs, ACTION_MOVEBACK);
			}
		}
		if (bs->forceMove_Right)
		{
			if (bs->forceMove_Right > 0)
			{
				BotDirectSafeButtonMove(bs, ACTION_MOVERIGHT);
			}
			else
			{
				BotDirectSafeButtonMove(bs, ACTION_MOVELEFT);
			}
		}
		if (bs->forceMove_Up)
		{
			trap_EA_Jump(bs->client);
		}
		return;
	}

	if (!bs->lastDeadTime)
	{ //just spawned in?
		bs->lastDeadTime = level.time;
	}

	if (g_entities[bs->client].health < 1)
	{
		bs->lastDeadTime = level.time;

		if (!bs->deathActivitiesDone && bs->lastHurt && bs->lastHurt->client && bs->lastHurt->s.number != bs->client)
		{
			BotDeathNotify(bs);
			if (PassLovedOneCheck(bs, bs->lastHurt))
			{
				//CHAT: Died
				bs->chatObject = bs->lastHurt;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "Died", 0);
			}
			else if (!PassLovedOneCheck(bs, bs->lastHurt) &&
				botstates[bs->lastHurt->s.number] &&
				PassLovedOneCheck(botstates[bs->lastHurt->s.number], &g_entities[bs->client]))
			{ //killed by a bot that I love, but that does not love me
				bs->chatObject = bs->lastHurt;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "KilledOnPurposeByLove", 0);
			}

			bs->deathActivitiesDone = 1;
		}
		
		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpCamping = NULL;
		bs->wpCampingTo = NULL;
		bs->wpStoreDest = NULL;
		bs->wpDestIgnoreTime = 0;
		bs->wpDestSwitchTime = 0;
		bs->wpSeenTime = 0;
		bs->wpDirection = 0;

		//RACC - Try to respawn if you're done talking.
		if (rand()%10 < 5 &&
			(!bs->doChat || bs->chatTime < level.time))
		{
			trap_EA_Attack(bs->client);
		}
		return;
	}

	if(BG_InLedgeMove(bs->cur_ps.legsAnim))
	{//we're in a ledge move, just pull up for now
		BotDirectSafeButtonMove(bs, ACTION_MOVEFORWARD);
		return;
	}



	if (bs->cur_ps.weapon == WP_SABER &&
		g_entities[bs->client].client->saber[0].model[0] &&
		g_entities[bs->client].client->saber[1].model[0])
	{
		// Dual sabers
		if (g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_DUAL)
		{
			Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
		}
	}
	else if (bs->cur_ps.weapon == WP_SABER &&
		g_entities[bs->client].client->saber[0].numBlades > 1 &&
		WP_SaberCanTurnOffSomeBlades(&g_entities[bs->client].client->saber[0]))
	{
		// Staff saber
		if (g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_STAFF)
		{
			Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
		}
	}
	else
	{
		int targetStance = Bot_SelectSingleSaberCombatStyle(bs);

		if (g_entities[bs->client].client->ps.fd.saberAnimLevel != targetStance)
		{
			Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
		}
	}




	if(bs->cur_ps.saberLockTime > level.time)
	{//bot is in a saber lock
		//bots cheat by knowing their enemy's DP level, if they're low on DP, try to super break finish them.
		if(g_entities[bs->cur_ps.saberLockEnemy].client->ps.stats[STAT_DODGE] < 50)
		{
			trap_EA_Attack(bs->client);
		}
		return;
	}	
	VectorCopy(bs->goalAngles, preFrameGAngles);

	bs->doAttack = 0;
	bs->doAltAttack = 0;
	//[SaberSys]
	bs->doSaberThrow = 0;
	//[/SaberSys]
	//reset the attack states

	if (bs->isSquadLeader)
	{
		CommanderBotAI(bs);
	}
	else
	{
		BotDoTeamplayAI(bs);
	}

	if (!bs->currentEnemy)
	{
		bs->frame_Enemy_Vis = 0;
	}

	//RACC - revenge Enemy became inactive.
// Assuming 'clientConnected_t' and 'connstate_t' are different enums,
// explicitly cast to the same type to avoid comparison issues.

	if ((bs->revengeEnemy && bs->revengeEnemy->client && bs->revengeEnemy->s.eType != ET_NPC &&
		(clientConnected_t)bs->revengeEnemy->client->pers.connected != (clientConnected_t)CA_ACTIVE &&
		(clientConnected_t)bs->revengeEnemy->client->pers.connected != (clientConnected_t)CA_AUTHORIZING))
	{
		bs->revengeEnemy = NULL;
		bs->revengeHateLevel = 0;
	}

	// RACC - current enemy became inactive
	if ((bs->currentEnemy && bs->currentEnemy->client && bs->currentEnemy->s.eType != ET_NPC &&
		(clientConnected_t)bs->currentEnemy->client->pers.connected != (clientConnected_t)CA_ACTIVE &&
		(clientConnected_t)bs->currentEnemy->client->pers.connected != (clientConnected_t)CA_AUTHORIZING))
	{
		bs->currentEnemy = NULL;
	}


	fjHalt = 0;

	//RACC - Do the Force Powers Thing.
#ifndef FORCEJUMP_INSTANTMETHOD
	if (bs->forceJumpChargeTime > level.time)
	{
			if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
		{
			client->jetPackOn = qtrue;
				client->ps.pm_type = PM_JETPACK;
				bs->cur_ps.eFlags |= EF_JETPACK;
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
			bs->jumpHoldTime = ((bs->forceJumpChargeTime - level.time)/2) + level.time;
		}
		useTheForce = 1;
		forceHostile = 0;
	}

	if (bs->currentEnemy && bs->currentEnemy->client && bs->frame_Enemy_Vis && bs->forceJumpChargeTime < level.time)
#else
	if (bs->currentEnemy && bs->currentEnemy->client && bs->frame_Enemy_Vis)
#endif
	{
		VectorSubtract(bs->currentEnemy->client->ps.origin, bs->eye, a_fo);
		vectoangles(a_fo, a_fo);
		

		//do this above all things
		if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PUSH)) && (bs->doForcePush > level.time || bs->cur_ps.fd.forceGripBeingGripped > level.time) && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH] /*&& InFieldOfVision(bs->viewangles, 50, a_fo)*/)
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_PUSH;
			useTheForce = 1;
			forceHostile = 1;
		}
			  //in order of priority top to bottom
		else if (g_entities[bs->entitynum].health <= 999
					&& bs->cur_ps.fd.forcePowersKnown & (1 << FP_HEAL) 
					&& level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_HEAL]][FP_HEAL] )
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_HEAL;
			useTheForce = 1;
			forceHostile = 0;
		}
		else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_TELEPATHY)) && bs->frame_Enemy_Len < MAX_TRICK_DISTANCE && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TELEPATHY]][FP_TELEPATHY] && InFieldOfVision(bs->viewangles, 50, a_fo) && !(bs->currentEnemy->client->ps.fd.forcePowersActive & (1 << FP_SEE)))
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_TELEPATHY;
			useTheForce = 1;
			forceHostile = 1;
		}
		else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_TEAM_HEAL)) && bs->frame_Enemy_Len < MAX_STASIS_DISTANCE && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_HEAL]][FP_TEAM_HEAL] && InFieldOfVision(bs->viewangles, 50, a_fo) )
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_HEAL;
			useTheForce = 1;
			forceHostile = 1;
		}
		else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PROTECT))  && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PROTECT]][FP_PROTECT] )
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_PROTECT;
			useTheForce = 1;
			forceHostile = 0;
		}
		else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) && bs->cur_ps.fd.forceGripCripple &&
					 level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
		{ //absorb to get out
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
			useTheForce = 1;
			forceHostile = 0;
		}
		else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) && bs->cur_ps.electrifyTime >= level.time &&
					 level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
		{ //absorb lightning
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
			useTheForce = 1;
			forceHostile = 0;
		}
		else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB] )
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
			useTheForce = 1;
			forceHostile = 0;
		}
		else if (g_entities[bs->entitynum].health <= 999
					&& (bs->cur_ps.fd.forcePowersKnown & (1 << FP_DRAIN)) && bs->frame_Enemy_Len < MAX_DRAIN_DISTANCE && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_DRAIN]][FP_DRAIN] && InFieldOfVision(bs->viewangles, 50, a_fo)  )
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_DRAIN;
			useTheForce = 1;
			forceHostile = 1;
		}
		else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_LIGHTNING)) && bs->frame_Enemy_Len < FORCE_LIGHTNING_RADIUS && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_LIGHTNING]][FP_LIGHTNING] && InFieldOfVision(bs->viewangles, 50, a_fo))
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_LIGHTNING;
			useTheForce = 1;
			forceHostile = 1;
		}
		else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_TEAM_FORCE)) && bs->frame_Enemy_Len < FORCE_DESTRUCTION_RADIUS && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_FORCE]][FP_TEAM_FORCE] && InFieldOfVision(bs->viewangles, 50, a_fo))
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_FORCE;
			useTheForce = 1;
			forceHostile = 1;
		}
		else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_GRIP)) && (bs->cur_ps.fd.forcePowersActive & (1 << FP_GRIP)) && InFieldOfVision(bs->viewangles, 50, a_fo))
		{ //already gripping someone, so hold it
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_GRIP;
			useTheForce = 1;
			forceHostile = 1;
		}
		else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_GRIP)) && bs->frame_Enemy_Len < MAX_GRIP_DISTANCE && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_GRIP]][FP_GRIP] && InFieldOfVision(bs->viewangles, 50, a_fo))
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_GRIP;
			useTheForce = 1;
			forceHostile = 1;
		}	
		else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_RAGE)) && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_RAGE]][FP_RAGE] )
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_RAGE;
			useTheForce = 1;
			forceHostile = 0;
		}
		
		if (!useTheForce)
		{ //try neutral powers
			if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PUSH)) && bs->cur_ps.fd.forceGripBeingGripped > level.time && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH] && InFieldOfVision(bs->viewangles, 50, a_fo))
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_PUSH;
				useTheForce = 1;
				forceHostile = 1;
			}
			else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_SPEED)) && g_entities[bs->client].health < 125 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_SPEED]][FP_SPEED])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_SPEED;
				useTheForce = 1;
				forceHostile = 0;
			}
			else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_SEE)) && BotMindTricked(bs->client, bs->currentEnemy->s.number) && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_SEE]][FP_SEE])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_SEE;
				useTheForce = 1;
				forceHostile = 0;
			}
			else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PULL)) && bs->frame_Enemy_Len < 250 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PULL]][FP_PULL] && InFieldOfVision(bs->viewangles, 50, a_fo))
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_PULL;
				useTheForce = 1;
				forceHostile = 1;
			}
		}
	}

	if (!useTheForce)
	{ //try powers that we don't care if we have an enemy for
		if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_HEAL)) && g_entities[bs->client].health <=  999 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_HEAL]][FP_HEAL] && bs->cur_ps.fd.forcePowerLevel[FP_HEAL] > FORCE_LEVEL_1)
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_HEAL;
			useTheForce = 1;
			forceHostile = 0;
		}
		else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_HEAL)) && g_entities[bs->client].health <=  999 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_HEAL]][FP_HEAL] && !bs->currentEnemy && bs->isCamping > level.time)
		{ //only meditate and heal if we're camping
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_HEAL;
			useTheForce = 1;
			forceHostile = 0;
		}
	}

	// ------------------------------------------------------------

	// ------------------------------------------------------------
	// Style Force "cover" pass:
	// If we didn't pick a Force power above, TAB/HYBRID get an extra chance
	// to use ANY Force power they already have (no loadout changes).
	// ------------------------------------------------------------
	if (!useTheForce && (bs->settings.botType == BOT_TAB || bs->settings.botType == BOT_HYBRID))
	{
		int now = level.time;
		int cd = (bs->settings.botType == BOT_TAB) ? 900 : 1500;
		if (botForceCoverUntil[bs->client] <= now)
		{
			int candidates[NUM_FORCE_POWERS];
			int candHostile[NUM_FORCE_POWERS];
			int candWeight[NUM_FORCE_POWERS];
			int num = 0;
			int totalW = 0;
			int p;
			qboolean hasEnemy = (bs->currentEnemy && bs->currentEnemy->client && bs->frame_Enemy_Vis) ? qtrue : qfalse;

			for (p = 0; p < NUM_FORCE_POWERS; p++)
			{
				int need;
				int lvl;
				int w = 0;
				qboolean hostile = qfalse;

				if (!(bs->cur_ps.fd.forcePowersKnown & (1 << p)))
					continue;
				lvl = level.clients[bs->client].ps.fd.forcePowerLevel[p];
				if (lvl <= 0)
					continue;
				need = forcePowerNeeded[lvl][p];
				if (level.clients[bs->client].ps.fd.forcePower <= need)
					continue;

				// Avoid selecting already-active toggles.
				if (bs->cur_ps.fd.forcePowersActive & (1 << p))
					continue;

				switch (p)
				{
					case FP_HEAL:
						if (g_entities[bs->client].client &&
							g_entities[bs->client].client->ps.stats[STAT_HEALTH] < g_entities[bs->client].client->ps.stats[STAT_MAX_HEALTH])
						{
							w = 80;
						}
						break;
					case FP_SPEED:
						w = hasEnemy ? 60 : 30;
						break;
					case FP_SEE:
						w = hasEnemy ? 20 : 35;
						break;
					case FP_PROTECT:
						w = hasEnemy ? 55 : 15;
						break;
					case FP_ABSORB:
						w = hasEnemy ? 50 : 15;
						break;
					case FP_RAGE:
						w = hasEnemy ? 45 : 10;
						break;
					case FP_PUSH:
						hostile = qtrue;
						if (hasEnemy && bs->frame_Enemy_Len < 420.0f && InFieldOfVision(bs->viewangles, 65, a_fo))
							w = 70;
						break;
					case FP_PULL:
						hostile = qtrue;
						if (hasEnemy && bs->frame_Enemy_Len < 520.0f && InFieldOfVision(bs->viewangles, 65, a_fo))
							w = 55;
						break;
					case FP_GRIP:
						hostile = qtrue;
						if (hasEnemy && bs->frame_Enemy_Len < MAX_GRIP_DISTANCE && InFieldOfVision(bs->viewangles, 55, a_fo))
							w = 60;
						break;
					case FP_LIGHTNING:
						hostile = qtrue;
						if (hasEnemy && bs->frame_Enemy_Len < 520.0f && InFieldOfVision(bs->viewangles, 50, a_fo))
							w = 55;
						break;
					case FP_DRAIN:
						hostile = qtrue;
						if (hasEnemy && bs->frame_Enemy_Len < 360.0f && InFieldOfVision(bs->viewangles, 60, a_fo))
							w = 45;
						break;
					case FP_TELEPATHY:
						hostile = qtrue;
						if (hasEnemy && bs->frame_Enemy_Len < 700.0f && InFieldOfVision(bs->viewangles, 75, a_fo))
							w = 35;
						break;
					case FP_TEAM_HEAL:
						// Simple scan: if any teammate nearby is hurt, consider it.
						{
							int i;
							for (i = 0; i < level.maxclients; i++)
							{
								if (i == bs->client) continue;
								if (!g_entities[i].inuse || !g_entities[i].client) continue;
								if (!OnSameTeam(&g_entities[bs->client], &g_entities[i])) continue;
								if (g_entities[i].health <= 0) continue;
								if (DistanceSquared(g_entities[i].client->ps.origin, bs->origin) > (768.0f * 768.0f)) continue;
								if (g_entities[i].client->ps.stats[STAT_HEALTH] < g_entities[i].client->ps.stats[STAT_MAX_HEALTH])
								{
									w = 25;
									break;
								}
							}
						}
						break;
					case FP_TEAM_FORCE:
						{
							int i;
							for (i = 0; i < level.maxclients; i++)
							{
								if (i == bs->client) continue;
								if (!g_entities[i].inuse || !g_entities[i].client) continue;
								if (!OnSameTeam(&g_entities[bs->client], &g_entities[i])) continue;
								if (g_entities[i].health <= 0) continue;
								if (DistanceSquared(g_entities[i].client->ps.origin, bs->origin) > (768.0f * 768.0f)) continue;
								if (g_entities[i].client->ps.fd.forcePower < 50)
								{
									w = 20;
									break;
								}
							}
						}
						break;
					case FP_SABERTHROW:
						hostile = qtrue;
						if (bs->cur_ps.weapon == WP_SABER && hasEnemy && bs->frame_Enemy_Len > 220.0f && bs->frame_Enemy_Len < 900.0f && InFieldOfVision(bs->viewangles, 60, a_fo))
							w = 35;
						break;
					case FP_LEVITATION:
						// Jump/levitation is handled elsewhere; don't select it here.
						w = 0;
						break;
					default:
						// Unknown/unsupported: skip to avoid selecting no-op powers.
						w = 0;
						break;
				}

				if (w > 0)
				{
					candidates[num] = p;
					candHostile[num] = hostile ? 1 : 0;
					candWeight[num] = w;
					totalW += w;
					num++;
				}
			}

			if (num > 0 && totalW > 0)
			{
				int roll = Q_irand(1, totalW);
				int acc = 0;
				int pick = candidates[0];
				int pickHostile = candHostile[0];
				int i;

				for (i = 0; i < num; i++)
				{
					acc += candWeight[i];
					if (roll <= acc)
					{
						pick = candidates[i];
						pickHostile = candHostile[i];
						break;
					}
				}

				level.clients[bs->client].ps.fd.forcePowerSelected = pick;
				useTheForce = 1;
				forceHostile = pickHostile;
				botForceCoverUntil[bs->client] = now + cd;
			}
			else
			{
				botForceCoverUntil[bs->client] = now + 400;
			}
		}
	}

	// BOT type flavour (no weight edits):
	//  TAB    -> more willing to use offensive/utility Force.
	//  AOTC   -> largely avoids offensive Force (plays more like a gunner).
	//  HYBRID -> sometimes uses offensive Force, but less often than TAB.
	// ------------------------------------------------------------
	if (useTheForce)
	{
		int sel = level.clients[bs->client].ps.fd.forcePowerSelected;
		if ((bs && bs->settings.botType == BOT_AOTC) && BotForceIsOffensive(sel))
		{
			// Gunners keep it simple: mostly defensive/utility Force only.
			useTheForce = 0;
			forceHostile = 0;
		}
		else if ((bs && bs->settings.botType == BOT_HYBRID) && BotForceIsOffensive(sel))
		{
			// Hybrids: still use offensive Force, but not constantly.
			if (Q_irand(0, 100) < 55)
			{
				useTheForce = 0;
				forceHostile = 0;
			}
		}
	}

	if (useTheForce && forceHostile)
	{
		if (bs->currentEnemy && bs->currentEnemy->client &&
			!ForcePowerUsableOn(&g_entities[bs->client], bs->currentEnemy, level.clients[bs->client].ps.fd.forcePowerSelected))
		{
			useTheForce = 0;
			forceHostile = 0;
		}
	}

	doingFallback = 0;

	bs->deathActivitiesDone = 0;

	{
		int usedItem = 0;
		int useChance = 50; // default behaviour (approx old 50%)
		int now = level.time;

		usedItem = BotUseInventoryItem(bs);

		// AOTC/HYBRID: additional "item cover" pass that can pick from ANY owned holdable.
		// (Does not grant items; only chooses among STAT_HOLDABLE_ITEMS.)
		if (!usedItem && (bs->settings.botType == BOT_AOTC || bs->settings.botType == BOT_HYBRID))
		{
			if (botItemCoverUntil[bs->client] <= now)
			{
				if (BotStyle_SelectAnyHoldable(bs))
				{
					usedItem = 1;
					botItemCoverUntil[bs->client] = now + ((bs->settings.botType == BOT_AOTC) ? 900 : 1400);
				}
				else
				{
					botItemCoverUntil[bs->client] = now + 600;
				}
			}
		}

		// Item use frequency by bot style (no loadout/skill changes).
		if (bs->settings.botType == BOT_AOTC) { useChance = 80; }
		else if (bs->settings.botType == BOT_HYBRID) { useChance = 60; }
		else if (bs->settings.botType == BOT_TAB) { useChance = 35; }

		if (usedItem)
		{
			if ((rand() % 100) < useChance)
			{
				trap_EA_Use(bs->client);
			}
		}
	}

	if (bs->cur_ps.ammo[weaponData[bs->cur_ps.weapon].ammoIndex] < weaponData[bs->cur_ps.weapon].energyPerShot)
	{
		if (BotTryAnotherWeapon(bs))
		{
			return;
		}
	}
	else
	{
		//RACC - You see your enemy, quit trying to plant mines
		if (bs->currentEnemy && bs->lastVisibleEnemyIndex == bs->currentEnemy->s.number &&
			bs->frame_Enemy_Vis && bs->forceWeaponSelect /*&& bs->plantContinue < level.time*/)
		{
			bs->forceWeaponSelect = 0;
		}


		if (bs->plantContinue > level.time)
		{
			bs->doAttack = 1;
			bs->destinationGrabTime = 0;
		}

		//RACC - you need to switch to the det pack to be able do your det detpacks command
		if (!bs->forceWeaponSelect && bs->cur_ps.hasDetPackPlanted && bs->plantKillEmAll > level.time)
		{
			bs->forceWeaponSelect = WP_DET_PACK;
		}

		if (bs->forceWeaponSelect)
		{
			selResult = BotSelectChoiceWeapon(bs, bs->forceWeaponSelect, 1);
		}

		if (selResult)
		{
			if (selResult == 2)
			{ //newly selected
				return;
			}
		}
		else if (BotSelectIdealWeapon(bs))
		{
			return;
		}
	}
	/*if (BotSelectMelee(bs))
	{
		return;
	}*/

	//RACC - Update reaction speed
	reaction = bs->skills.reflex/bs->settings.skill;


	// Bot style: tweak raw reaction for non-default types (TAB fastest, AOTC mid).
	// Keep BOT_DEFAULT identical to stock behaviour.
	if (bs->settings.botType == BOT_TAB)
	{
		reaction *= 0.65f;
	}
	else if (bs->settings.botType == BOT_HYBRID)
	{
		reaction *= 0.75f;
	}
	else if (bs->settings.botType == BOT_AOTC)
	{
		reaction *= 0.85f;
	}

	// Small per-bot variance: personality scalar <1 means slightly slower reactions, >1 slightly faster.
	// (BOT_DEFAULT always returns 1.0)
	reaction *= (1.0f / stylePersonality);


	if (reaction < 0)
	{
		reaction = 0;
	}
	if (reaction > 2000)
	{
		reaction = 2000;
	}

	if (!bs->currentEnemy)
	{
		bs->timeToReact = level.time + reaction;
	}

	//RACC - Blow detpacks
	if (bs->cur_ps.weapon == WP_DET_PACK && bs->cur_ps.hasDetPackPlanted && bs->plantKillEmAll > level.time)
	{
		bs->doAltAttack = 1;
	}

	if (bs->wpCamping)
	{
		if (bs->isCamping < level.time)
		{
			bs->wpCamping = NULL;
			bs->isCamping = 0;
		}

		if (bs->currentEnemy && bs->frame_Enemy_Vis)
		{
			bs->wpCamping = NULL;
			bs->isCamping = 0;
		}
	}

	if (bs->wpCurrent &&
		(bs->wpSeenTime < level.time || bs->wpTravelTime < level.time))
	{
		bs->wpCurrent = NULL;
	}

	if (bs->currentEnemy)
	{
		if (bs->enemySeenTime < level.time ||
			!PassStandardEnemyChecks(bs, bs->currentEnemy))
		{
			if (bs->revengeEnemy == bs->currentEnemy &&
				bs->currentEnemy->health < 1 &&
				bs->lastAttacked && bs->lastAttacked == bs->currentEnemy)
			{
				//CHAT: Destroyed hated one [KilledHatedOne section]
				bs->chatObject = bs->revengeEnemy;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "KilledHatedOne", 1);
				bs->revengeEnemy = NULL;
				bs->revengeHateLevel = 0;
			}
			else if (bs->currentEnemy->health < 1 && PassLovedOneCheck(bs, bs->currentEnemy) &&
				bs->lastAttacked && bs->lastAttacked == bs->currentEnemy)
			{
				//CHAT: Killed
				bs->chatObject = bs->currentEnemy;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "Killed", 0);
			}

			bs->currentEnemy = NULL;
		}
	}

	if (bot_honorableduelacceptance.integer)
	{
		if (bs->currentEnemy && bs->currentEnemy->client &&
			bs->cur_ps.weapon == WP_SABER &&
			g_privateDuel.integer &&
			bs->frame_Enemy_Vis &&
			bs->frame_Enemy_Len < 400 &&
			bs->currentEnemy->client->ps.weapon == WP_SABER &&
			//[BotTweaks]
			//sabers should have to be fully holstered to have this work.
			bs->currentEnemy->client->ps.saberHolstered == 2)
			//bs->currentEnemy->client->ps.saberHolstered)
			//[/BotTweaks]
		{
			vec3_t e_ang_vec;

			VectorSubtract(bs->currentEnemy->client->ps.origin, bs->eye, e_ang_vec);

			if (InFieldOfVision(bs->viewangles, 100, e_ang_vec))
			{ //Our enemy has his saber holstered and has challenged us to a duel, so challenge him back
				if (!bs->cur_ps.saberHolstered)
				{
					Cmd_ToggleSaber_f(&g_entities[bs->client]);
				}
				else
				{
					if (bs->currentEnemy->client->ps.duelIndex == bs->client &&
						bs->currentEnemy->client->ps.duelTime > level.time &&
						!bs->cur_ps.duelInProgress)
					{
						Cmd_EngageDuel_f(&g_entities[bs->client]);
					}
				}

				bs->doAttack = 0;
				bs->doAltAttack = 0;
				//[SaberSys]
				bs->doSaberThrow = 0;
				//[/SaberSys]
				bs->botChallengingTime = level.time + 100;
				bs->beStill = level.time + 100;
			}
		}
	}
	//Apparently this "allows you to cheese" when fighting against bots. I'm not sure why you'd want to con bots
	//into an easy kill, since they're bots and all. But whatever.

	//RACC - off the bot paths, try to find the nearest path point.
	if (!bs->wpCurrent)
	{
		wp = GetNearestVisibleWP(bs->origin, bs->client);

		if (wp != -1)
		{
			bs->wpCurrent = gWPArray[wp];
			BotCommitToWaypoint(bs, wp);
			bs->wpSeenTime = level.time + 1500;
			bs->wpTravelTime = level.time + 10000; //never take more than 10 seconds to travel to a waypoint
		}
	}

	if (bs->enemySeenTime < level.time || !bs->frame_Enemy_Vis || !bs->currentEnemy
		/*|| (bs->currentEnemy && bs->cur_ps.weapon == WP_SABER && bs->frame_Enemy_Len > 300)*/)
	{
		enemy = ScanForEnemies(bs);

		if (enemy != -1)
		{
			bs->currentEnemy = &g_entities[enemy];
			bs->enemySeenTime = level.time + ENEMY_FORGET_MS;
		}
	}

	if (!bs->squadLeader && !bs->isSquadLeader)
	{
		BotScanForLeader(bs);
	}

	if (!bs->squadLeader && bs->squadCannotLead < level.time)
	{ //if still no leader after scanning, then become a squad leader
		bs->isSquadLeader = 1;
	}

	if (bs->isSquadLeader && bs->squadLeader)
	{ //we don't follow anyone if we are a leader
		bs->squadLeader = NULL;
	}

	//ESTABLISH VISIBILITIES AND DISTANCES FOR THE WHOLE FRAME HERE
	if (bs->wpCurrent)
	{
		if (g_RMG.integer)
		{ //this is somewhat hacky, but in RMG we don't really care about vertical placement because points are scattered across only the terrain.
			vec3_t vecB, vecC;

			vecB[0] = bs->origin[0];
			vecB[1] = bs->origin[1];
			vecB[2] = bs->origin[2];

			vecC[0] = bs->wpCurrent->origin[0];
			vecC[1] = bs->wpCurrent->origin[1];
			vecC[2] = vecB[2];


			VectorSubtract(vecC, vecB, a);
		}
		else
		{
			VectorSubtract(bs->wpCurrent->origin, bs->origin, a);
		}
		bs->frame_Waypoint_Len = VectorLength(a);

		visResult = WPOrgVisible(&g_entities[bs->client], bs->origin, bs->wpCurrent->origin, bs->client);

		//RACC - There's a force field between you and your target WP.  turn around and go the other direction.
		if (visResult == 2)
		{
			bs->frame_Waypoint_Vis = 0;
			bs->wpSeenTime = 0;

			// try to route around this one point, but keep overall destination
			if (!TrySwitchWPBranch(bs)) {
				bs->wpDestination = NULL; // only if we *truly* can’t route
			}
		}

		else if (visResult)
		{
			bs->frame_Waypoint_Vis = 1;
		}
		else
		{
			bs->frame_Waypoint_Vis = 0;
		}
	}

	if (bs->currentEnemy)
	{
		if (bs->currentEnemy->client)
		{
			VectorCopy(bs->currentEnemy->client->ps.origin, eorg);
			eorg[2] += bs->currentEnemy->client->ps.viewheight;
		}
		else
		{
			VectorCopy(bs->currentEnemy->s.origin, eorg);
		}

		VectorSubtract(eorg, bs->eye, a);
		bs->frame_Enemy_Len = VectorLength(a);

		if (OrgVisible(bs->eye, eorg, bs->client))
		{
			bs->frame_Enemy_Vis = 1;
			VectorCopy(eorg, bs->lastEnemySpotted);
			VectorCopy(bs->origin, bs->hereWhenSpotted);
			bs->lastVisibleEnemyIndex = bs->currentEnemy->s.number;
			//VectorCopy(bs->eye, bs->lastEnemySpotted);
			bs->hitSpotted = 0;
		}
		else
		{
			bs->frame_Enemy_Vis = 0;
		}
	}
	else
	{
		bs->lastVisibleEnemyIndex = ENTITYNUM_NONE;
	}
	//END

	if (bs->frame_Enemy_Vis)
	{
		bs->enemySeenTime = level.time + ENEMY_FORGET_MS;
	}

	if (bs->wpCurrent)
	{
		int wpTouchDist = BOT_WPTOUCH_DISTANCE;
		WPConstantRoutine(bs);

		if (!bs->wpCurrent)
		{ //WPConstantRoutine has the ability to nullify the waypoint if it fails certain checks, so..
			return;
		}

		if (bs->wpCurrent->flags & WPFLAG_WAITFORFUNC)
		{
			const int cn = bs->client;
			const int wpIndex = bs->wpCurrent->index;
			const qboolean funcHere = (CheckForFunc(bs->wpCurrent->origin, -1) != 0);

			// Track how long we've been waiting at this WP.
			if (bot_waitfunc_last_wp[cn] != wpIndex)
			{
				BotWaitFunc_Reset(cn);
				bot_waitfunc_last_wp[cn] = wpIndex;
				bot_waitfunc_since[cn] = level.time;
			}
			else if (!bot_waitfunc_since[cn])
			{
				bot_waitfunc_since[cn] = level.time;
			}

			// If the platform/func brush isn't here yet, call/use it, but do NOT clog the shaft/trigger volume.
			if (!funcHere)
			{
				// Press use to call lifts/doors if applicable (handled elsewhere too, but cheap to do here).
				trap_EA_Use(cn);

				// After waiting a bit, periodically back off so multiple bots don't stack inside the shaft/door volume.
				if (bot_waitfunc_since[cn] && level.time - bot_waitfunc_since[cn] > 2500)
				{
					if (level.time >= bot_waitfunc_backoff_until[cn])
					{
						bot_waitfunc_backoff_until[cn] = level.time + 1200;
						bot_waitfunc_since[cn] = level.time; // restart wait window for next pulse
					}
				}

				// Backoff phase: actively move away.
				if (level.time < bot_waitfunc_backoff_until[cn] || BotWaitFunc_ShouldBackoff(bs, bs->wpCurrent->origin))
				{
					// Don't "beStill" while we're trying to clear the area.
					bs->beStill = 0;
					BotWaitFunc_DoBackoff(bs, bs->wpCurrent->origin);
				}
				else
				{
					// Wait nearby (not inside) until the func arrives.
					bs->beStill = level.time + 500;
				}
			}
			else
			{
				// Func brush is under the WP (lift platform present). Let normal logic continue.
				// Reset timers so we don't pulse-backoff while actually riding.
				BotWaitFunc_Reset(cn);
				bot_waitfunc_last_wp[cn] = wpIndex;
			}
		}
		if (bs->wpCurrent->flags & WPFLAG_NOMOVEFUNC)
		{
			if (CheckForFunc(bs->wpCurrent->origin, -1))
			{
				bs->beStill = level.time + 500; //func brush under.. wait
			}
		}

		if (bs->frame_Waypoint_Vis || (bs->wpCurrent->flags & WPFLAG_NOVIS))
		{
			if (g_RMG.integer)
			{
				bs->wpSeenTime = level.time + 5000; //if we lose sight of the point, we have 1.5 seconds to regain it before we drop it
			}
			else
			{
				bs->wpSeenTime = level.time + 1500; //if we lose sight of the point, we have 1.5 seconds to regain it before we drop it
			}
		}
		VectorCopy(bs->wpCurrent->origin, bs->goalPosition);
		if (bs->wpDirection)
		{
			goalWPIndex = bs->wpCurrent->index-1;
		}
		else
		{
			goalWPIndex = bs->wpCurrent->index+1;
		}

		
		//RACC - Set goalAngles
		if (bs->wpCamping)
		{//RACC - pitch the camping tent.
			VectorSubtract(bs->wpCampingTo->origin, bs->origin, a);
			vectoangles(a, ang);
			VectorCopy(ang, bs->goalAngles);

			VectorSubtract(bs->origin, bs->wpCamping->origin, a);
			if (VectorLength(a) < 64)
			{
				VectorCopy(bs->wpCamping->origin, bs->goalPosition);
				bs->beStill = level.time + 1000;

				if (!bs->campStanding)
				{
					bs->duckTime = level.time + 1000;
				}
			}
		}
		else if (gWPArray[goalWPIndex] && gWPArray[goalWPIndex]->inuse &&
			!(gLevelFlags & LEVELFLAG_NOPOINTPREDICTION))
		{
			VectorSubtract(gWPArray[goalWPIndex]->origin, bs->origin, a);
			vectoangles(a, ang);
			VectorCopy(ang, bs->goalAngles);
		}
		else
		{
			VectorSubtract(bs->wpCurrent->origin, bs->origin, a);
			vectoangles(a, ang);
			VectorCopy(ang, bs->goalAngles);
		}

		if (bs->destinationGrabTime < level.time /*&& (!bs->wpDestination || (bs->currentEnemy && bs->frame_Enemy_Vis))*/)
		{
			GetIdealDestination(bs);
		}
		
		//RACC - Can't get to destination from here
		if (bs->wpCurrent && bs->wpDestination)
		{
			if (TotalTrailDistance(bs->wpCurrent->index,
				bs->wpDestination->index,
				bs) == -1)
			{
				int idleWP = GetBestIdleGoal(bs);
				if (idleWP != -1 && gWPArray[idleWP] && gWPArray[idleWP]->inuse)
				{
					bs->wpDestination = gWPArray[idleWP];
				}
				else
				{
					bs->wpDestination = NULL;
					bs->destinationGrabTime = level.time + 3000; // shorter penalty
				}
			}

		}

		if (g_RMG.integer)
		{
			if (bs->frame_Waypoint_Vis)
			{
				if (bs->wpCurrent && !bs->wpCurrent->flags)
				{
					wpTouchDist *= 3;
				}
			}
		}

		//RACC - Touched a waypoint.  Do touch stuff and move onto next waypoint.
		if (bs->frame_Waypoint_Len < wpTouchDist || (g_RMG.integer && bs->frame_Waypoint_Len < wpTouchDist*2))
		{
			WPTouchRoutine(bs);

			if (!bs->wpDirection)
			{
				desiredIndex = bs->wpCurrent->index+1;
			}
			else
			{
				desiredIndex = bs->wpCurrent->index-1;
			}

			if (gWPArray[desiredIndex] &&
				gWPArray[desiredIndex]->inuse &&
				desiredIndex < gWPNum &&
				desiredIndex >= 0 &&
				PassWayCheck(bs, desiredIndex))
			{
				bs->wpCurrent = gWPArray[desiredIndex];
				BotCommitToWaypoint(bs, desiredIndex);
			}
			else
			{
				// Decisive navigation:
				// Don't instantly nuke our destination and flip direction (looks dumb).
				// First, respect a short commitment window unless we are clearly stalled.
				qboolean stalled = BotWaypointProgressStalled(bs);

				if (BotIsCommittedToCurrentWP(bs) && !stalled)
				{
					// Keep current intent; give it a moment longer.
					bs->wpDestIgnoreTime = level.time + 600;
					return;
				}

				// Try switching to a nearby branch without losing destination.
				if (TrySwitchWPBranch(bs))
				{
					if (bs->wpCurrent)
						BotCommitToWaypoint(bs, bs->wpCurrent->index);
					bs->wpDestIgnoreTime = level.time + BOT_WP_REROUTE_PENALTY_MS;
					return;
				}

				// Last resort: flip direction, but keep destination and only briefly penalize.
				bs->wpDirection ^= 1;
				bs->destinationGrabTime = level.time + 3000;
				bs->wpDestIgnoreTime = level.time + BOT_WP_REROUTE_PENALTY_MS;
			}
		}
	}
	else //We can't find a waypoint, going to need a fallback routine.
	{
		/*if (g_gametype.integer == GT_DUEL)*/
		{ //helps them get out of messy situations
			/*if ((level.time - bs->forceJumpChargeTime) > 3500)
			{
				bs->forceJumpChargeTime = level.time + 2000;
				BotDirectSafeButtonMove(bs, ACTION_MOVEFORWARD);
			}
			*/
			bs->jumpTime = level.time + 1500;
			bs->jumpHoldTime = level.time + 1500;
			bs->jDelay = 0;
		}
		doingFallback = BotFallbackNavigation(bs);
	}

	if (g_RMG.integer)
	{ //for RMG if the bot sticks around an area too long, jump around randomly some to spread to a new area (horrible hacky method)
		vec3_t vSubDif;

		VectorSubtract(bs->origin, bs->lastSignificantAreaChange, vSubDif);
		if (VectorLength(vSubDif) > 1500)
		{
			VectorCopy(bs->origin, bs->lastSignificantAreaChange);
			bs->lastSignificantChangeTime = level.time + 20000;
		}

		if (bs->lastSignificantChangeTime < level.time)
		{
			bs->iHaveNoIdeaWhereIAmGoing = level.time + 17000;
		}
	}

	if (bs->iHaveNoIdeaWhereIAmGoing > level.time && !bs->currentEnemy)
	{
		VectorCopy(preFrameGAngles, bs->goalAngles);
		bs->wpCurrent = NULL;
		bs->wpSwitchTime = level.time + 150;
		doingFallback = BotFallbackNavigation(bs);
		bs->jumpTime = level.time + 150;
		bs->jumpHoldTime = level.time + 150;
		bs->jDelay = 0;
		bs->lastSignificantChangeTime = level.time + 25000;
	}

	if (bs->wpCurrent && g_RMG.integer)
	{
		qboolean doJ = qfalse;

		if (bs->wpCurrent->origin[2]-192 > bs->origin[2])
		{
			doJ = qtrue;
		}
		else if ((bs->wpTravelTime - level.time) < 5000 && bs->wpCurrent->origin[2]-64 > bs->origin[2])
		{
			doJ = qtrue;
		}
		else if ((bs->wpTravelTime - level.time) < 7000 && (bs->wpCurrent->flags & WPFLAG_RED_FLAG))
		{
			if ((level.time - bs->jumpTime) > 200)
			{
				bs->jumpTime = level.time + 100;
				bs->jumpHoldTime = level.time + 100;
				bs->jDelay = 0;
			}
		}
		else if ((bs->wpTravelTime - level.time) < 7000 && (bs->wpCurrent->flags & WPFLAG_BLUE_FLAG))
		{
			if ((level.time - bs->jumpTime) > 200)
			{
				bs->jumpTime = level.time + 100;
				bs->jumpHoldTime = level.time + 100;
				bs->jDelay = 0;
			}
		}
		else if (bs->wpCurrent->index > 0)
		{
			if ((bs->wpTravelTime - level.time) < 7000)
			{
				if ((gWPArray[bs->wpCurrent->index-1]->flags & WPFLAG_RED_FLAG) ||
					(gWPArray[bs->wpCurrent->index-1]->flags & WPFLAG_BLUE_FLAG))
				{
					if ((level.time - bs->jumpTime) > 200)
					{
						bs->jumpTime = level.time + 100;
						bs->jumpHoldTime = level.time + 100;
						bs->jDelay = 0;
					}
				}
			}
		}

		if (doJ)
		{
			bs->jumpTime = level.time + 1500;
			bs->jumpHoldTime = level.time + 1500;
			bs->jDelay = 0;
		}
	}

	if (doingFallback)
	{
		bs->doingFallback = qtrue;
	}
	else
	{
		bs->doingFallback = qfalse;
	}

	if (bs->timeToReact < level.time && bs->currentEnemy && bs->enemySeenTime > level.time + (ENEMY_FORGET_MS - (ENEMY_FORGET_MS*0.2)))
	{
		if (bs->frame_Enemy_Vis)
		{
			cBAI = CombatBotAI(bs, thinktime);
		}
		else if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT)
		{ //keep charging in case we see him again before we lose track of him
			bs->doAltAttack = 1;
		}
		else if (bs->cur_ps.weaponstate == WEAPON_CHARGING)
		{ //keep charging in case we see him again before we lose track of him
			bs->doAttack = 1;
		}

		if (bs->destinationGrabTime > level.time + 100)
		{
			bs->destinationGrabTime = level.time + 100; //assures that we will continue staying within a general area of where we want to be in a combat situation
		}

		//RACC - set headlevel of the baddie
		if (bs->currentEnemy->client)
		{
			VectorCopy(bs->currentEnemy->client->ps.origin, headlevel);
			headlevel[2] += bs->currentEnemy->client->ps.viewheight;
		}
		else
		{
			VectorCopy(bs->currentEnemy->client->ps.origin, headlevel);
		}


		//RACC - try shooting a flechette alt bolt at an enemy you just lost track of.
		if (!bs->frame_Enemy_Vis)
		{
			//if (!bs->hitSpotted && VectorLength(a) > 256)
			if (OrgVisible(bs->eye, bs->lastEnemySpotted, -1))
			{
				VectorCopy(bs->lastEnemySpotted, headlevel);
				VectorSubtract(headlevel, bs->eye, a);
				vectoangles(a, ang);
				VectorCopy(ang, bs->goalAngles);

				if (bs->cur_ps.weapon == WP_FLECHETTE &&
					bs->cur_ps.weaponstate == WEAPON_READY &&
					bs->currentEnemy && bs->currentEnemy->client)
				{
					mLen = VectorLength(a) > 128;
					if (mLen > 128 && mLen < 1024)
					{
						VectorSubtract(bs->currentEnemy->client->ps.origin, bs->lastEnemySpotted, a);

						if (VectorLength(a) < 300)
						{
							bs->doAltAttack = 1;
						}
					}
				}
			}
		}
		else
		{
			bLeadAmount = BotWeaponCanLead(bs);
			// BOT type flavour: gunners lead a bit more; jedis rely less on leading.
			if (bLeadAmount)
			{
				if ((bs && bs->settings.botType == BOT_AOTC))
				{
					bLeadAmount *= 1.15f;
				}
				else if ((bs && bs->settings.botType == BOT_TAB))
				{
					bLeadAmount *= 0.90f;
				}
				else if ((bs && bs->settings.botType == BOT_HYBRID))
				{
					bLeadAmount *= 1.05f;
				}
			}
				if ((bs->skills.accuracy/bs->settings.skill) <= 8 && bLeadAmount)
			{
				BotAimLeading(bs, headlevel, bLeadAmount);
			}
			else
			{
				VectorSubtract(headlevel, bs->eye, a);
				vectoangles(a, ang);
				VectorCopy(ang, bs->goalAngles);
			}

			BotAimOffsetGoalAngles(bs);
		}
	}

	// After combat decisions, adjust positioning according to bot type.
	// This only nudges movement/spacing; it never changes loadouts/weights.
	BotStyle_ApplyCombatPositioning(bs, stylePersonality);

	if (bs->cur_ps.saberInFlight)
	{
		bs->saberThrowTime = level.time + Q_irand(4000, 10000);
	}

	if (bs->currentEnemy)
	{
		//RACC - handle saber combat (other than the attack button pressing).
		if (BotGetWeaponRange(bs) == BWEAPONRANGE_SABER)
		{
			int saberRange = SABER_ATTACK_RANGE;
			// BOT type flavour (doesn't touch any weights):
			// TAB closes for saber more aggressively; AOTC a bit less.
			if ((bs && bs->settings.botType == BOT_TAB))
			{
				saberRange = (int)(saberRange * 1.35f);
			}
			else if ((bs && bs->settings.botType == BOT_AOTC))
			{
				saberRange = (int)(saberRange * 0.80f);
			}

			VectorSubtract(bs->currentEnemy->client->ps.origin, bs->eye, a_fo);
			vectoangles(a_fo, a_fo);

			if (bs->saberPowerTime < level.time)
			{ //Don't just use strong attacks constantly, switch around a bit
				if (Q_irand(1, 10) <= 5)
				{
					bs->saberPower = qtrue;
				}
				else
				{
					bs->saberPower = qfalse;
				}

				bs->saberPowerTime = level.time + Q_irand(3000, 15000);
			}

			//RACC - switch stance if you want to.
	/*

			if (  !g_entities[bs->client].client->saber[1].model[0] && !g_entities[bs->client].client->saber[0].numBlades > 1 && !WP_SaberCanTurnOffSomeBlades(&g_entities[bs->client].client->saber[0] ) )
			{
				//RACC - switch to heavy if you're attacking a health opponent,
				//using a single saber, and have the red stance available.
				if (bs->currentEnemy->health > 250 
					&& g_entities[bs->client].client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] > 2)
				{		

					if (g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_STRONG 
						&& bs->saberPower)
					{ //if we are up against someone with a lot of health and we have a strong attack available, then h4q them
						Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
					}
				}
				else if (bs->currentEnemy->health > 100 
					&& g_entities[bs->client].client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] > 1)
				{
					if (g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_MEDIUM)
					{ //they're down on health a little, use level 2 if we can
						Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
					}
				}
				else
				{
					if (g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_FAST)
					{ //they've gone below 40 health, go at them with quick attacks
						Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
					}
				}
			}

			if (g_gametype.integer == GT_SINGLE_PLAYER)
			{
				saberRange *= 3;
			}
*/
			if (bs->frame_Enemy_Len <= saberRange)
			{
				SaberCombatHandling(bs);

				{
					int meleeStrafeDist = 80;
					if ((bs && bs->settings.botType == BOT_TAB))
					{
						meleeStrafeDist = 140;
					}
					else if ((bs && bs->settings.botType == BOT_HYBRID))
					{
						meleeStrafeDist = 110;
					}
					else if ((bs && bs->settings.botType == BOT_AOTC))
					{
						meleeStrafeDist = 60;
					}

					if (bs->frame_Enemy_Len < meleeStrafeDist)
					{
						meleestrafe = 1;
					}
				}
			}
			//RACC - Try throwing the saber
			else if (bs->saberThrowTime < level.time && !bs->cur_ps.saberInFlight &&
				(bs->cur_ps.fd.forcePowersKnown & (1 << FP_SABERTHROW)))
			{
				int saberThrowFOV = 30;
				float saberThrowMaxRange = BOT_SABER_THROW_RANGE;
				if ((bs && bs->settings.botType == BOT_TAB))
				{
					saberThrowFOV = 55;
					saberThrowMaxRange = BOT_SABER_THROW_RANGE * 1.10f;
				}
				else if ((bs && bs->settings.botType == BOT_AOTC))
				{
					saberThrowFOV = 20;
					saberThrowMaxRange = BOT_SABER_THROW_RANGE * 0.85f;
				}

				if (!InFieldOfVision(bs->viewangles, saberThrowFOV, a_fo) ||
					bs->frame_Enemy_Len >= saberThrowMaxRange)
				{
					// conditions not met - skip
				}
				else
				{
				//[SaberSys]
				bs->doSaberThrow = 1;
				bs->doAltAttack = 0;
				//bs->doAltAttack = 1;
				//[/SaberSys]
				bs->doAttack = 0;
				}
			}
			//RACC - Maintain saber throw
			else if (bs->cur_ps.saberInFlight && bs->frame_Enemy_Len > 300 && bs->frame_Enemy_Len < BOT_SABER_THROW_RANGE)
			{
				//[SaberSys]
				bs->doSaberThrow = 1;
				bs->doAltAttack = 0;
				//bs->doAltAttack = 1;
				//[/SaberSys]
				bs->doAttack = 0;
			}
		}
		else if (BotGetWeaponRange(bs) == BWEAPONRANGE_MELEE)
		{
			if (bs->frame_Enemy_Len <= MELEE_ATTACK_RANGE)
			{
				MeleeCombatHandling(bs);
				meleestrafe = 1;
			}
		}
	}

	if (doingFallback && bs->currentEnemy) //just stand and fire if we have no idea where we are
	{
		VectorCopy(bs->origin, bs->goalPosition);
	}

	//RACC - Force jumping to waypoint, you're pretty close to target so quit Force Jumping
	if (bs->forceJumping > level.time)
	{
		VectorCopy(bs->origin, noz_x);
		VectorCopy(bs->goalPosition, noz_y);

		noz_x[2] = noz_y[2];

		VectorSubtract(noz_x, noz_y, noz_x);

		if (VectorLength(noz_x) < 32)
		{
			fjHalt = 1;
		}
	}

	//RACC - Chat action code.

	//RACC - chatting ass off.  Since we don't have any enemies at the moment, just stand there.
	if (bs->doChat && bs->chatTime > level.time && (!bs->currentEnemy || !bs->frame_Enemy_Vis))
	{
		return;
	}
	//RACC - Attempting to chat but also see an enemy.  Time to abort chatting
	else if (bs->doChat && bs->currentEnemy && bs->frame_Enemy_Vis)
	{
		//bs->chatTime = level.time + bs->chatTime_stored;
		bs->doChat = 0; //do we want to keep the bot waiting to chat until after the enemy is gone?
		bs->chatTeam = 0;
	}
	//RACC - Otherwise, do the chat.
	else if (bs->doChat && bs->chatTime <= level.time)
	{
		if (bs->chatTeam)
		{
			trap_EA_SayTeam(bs->client, bs->currentChat);
			bs->chatTeam = 0;
		}
		else
		{
			trap_EA_Say(bs->client, bs->currentChat);
		}
		if (bs->doChat == 2)
		{
			BotReplyGreetings(bs);
		}
		bs->doChat = 0;
	}

	//RACC - Check for loose flags and such.
	CTFFlagMovement(bs);

	//RACC - Attacking mission objective code.
	if (/*bs->wpDestination &&*/ bs->shootGoal &&
		/*bs->wpDestination->associated_entity == bs->shootGoal->s.number &&*/
		bs->shootGoal->health > 0 && bs->shootGoal->takedamage)
	{
		dif[0] = (bs->shootGoal->r.absmax[0]+bs->shootGoal->r.absmin[0])/2;
		dif[1] = (bs->shootGoal->r.absmax[1]+bs->shootGoal->r.absmin[1])/2;
		dif[2] = (bs->shootGoal->r.absmax[2]+bs->shootGoal->r.absmin[2])/2;

		if (!bs->currentEnemy || bs->frame_Enemy_Len > 256)
		{ //if someone is close then don't stop shooting them for this
			VectorSubtract(dif, bs->eye, a);
			vectoangles(a, a);
			VectorCopy(a, bs->goalAngles);

			if (InFieldOfVision(bs->viewangles, 30, a) &&
				EntityVisibleBox(bs->origin, NULL, NULL, dif, bs->client, bs->shootGoal->s.number))
			{
				bs->doAttack = 1;
			}
		}
	}

	//RACC -  explosives code!
	if (bs->cur_ps.hasDetPackPlanted)
	{ //check if our enemy gets near it and detonate if he does
		BotCheckDetPacks(bs);
	}
	//RACC - Ok, we can't see our enemy at the moment, and we didn't just plant a mine/detpack, so try to plant some mines/bombs to get them.
	else if (bs->currentEnemy && bs->lastVisibleEnemyIndex == bs->currentEnemy->s.number && !bs->frame_Enemy_Vis && bs->plantTime < level.time &&
		//[SaberSys]
		!bs->doAttack && !bs->doAltAttack && !bs->doSaberThrow)
		//!bs->doAttack && !bs->doAltAttack)
		//[/SaberSys]
	{
		VectorSubtract(bs->origin, bs->hereWhenSpotted, a);

		if (bs->plantDecided > level.time || (bs->frame_Enemy_Len < BOT_PLANT_DISTANCE*2 && VectorLength(a) < BOT_PLANT_DISTANCE))
		{
			//RACC - check to see if we can switch to mines or det packs.
			mineSelect = BotSelectChoiceWeapon(bs, WP_TRIP_MINE, 0);
			detSelect = BotSelectChoiceWeapon(bs, WP_DET_PACK, 0);
			
			//RACC - Already planted a det pack, don't try to plant more.
			if (bs->cur_ps.hasDetPackPlanted)
			{
				detSelect = 0;
			}

			//RACC - already decided to plant mines/dets and now what correct weapon.  
			//Start trying to plant mines now.
			if (bs->plantDecided > level.time && bs->forceWeaponSelect &&
				bs->cur_ps.weapon == bs->forceWeaponSelect)
			{
				bs->doAttack = 1;
				bs->plantDecided = 0;
				bs->plantTime = level.time + BOT_PLANT_INTERVAL;
				bs->plantContinue = level.time + 500;
				bs->beStill = level.time + 500;
			}
			//RACC - decide to plant explosive
			else if (mineSelect || detSelect)
			{
				//RACC - only set if there's a floor or wall close in front of you view.
				if (BotSurfaceNear(bs))
				{
					if (!mineSelect)
					{ //if no mines use detpacks, otherwise use mines
						mineSelect = WP_DET_PACK;
					}
					else
					{
						mineSelect = WP_TRIP_MINE;
					}

					detSelect = BotSelectChoiceWeapon(bs, mineSelect, 1);

					if (detSelect && detSelect != 2)
					{ //We have it and it is now our weapon
						bs->plantDecided = level.time + 1000;
						bs->forceWeaponSelect = mineSelect;
						return;
					}
					else if (detSelect == 2)
					{
						bs->forceWeaponSelect = mineSelect;
						return;
					}
				}
			}
		}
	}
	//RACC - Deactivate the forced weapon select.
	else if (bs->plantContinue < level.time)
	{
		bs->forceWeaponSelect = 0;
	}

	//RACC - I spy the JEDIMASTER'S SABER AND IT'S LOOSE!
	if (g_gametype.integer == GT_JEDIMASTER && !bs->cur_ps.isJediMaster && bs->jmState == -1 && gJMSaberEnt && gJMSaberEnt->inuse)
	{
		vec3_t saberLen;
		float fSaberLen = 0;

		VectorSubtract(bs->origin, gJMSaberEnt->r.currentOrigin, saberLen);
		fSaberLen = VectorLength(saberLen);

		if (fSaberLen < 256)
		{
			if (OrgVisible(bs->origin, gJMSaberEnt->r.currentOrigin, bs->client))
			{
				VectorCopy(gJMSaberEnt->r.currentOrigin, bs->goalPosition);
			}
		}
	}

	//RACC - Not in beStill mode, waiting for an elevator, or dropping down onto a wp with
	//Force Jump.
	if (bs->beStill < level.time && !WaitingForNow(bs, bs->goalPosition) && !fjHalt)
	{
		VectorSubtract(bs->goalPosition, bs->origin, bs->goalMovedir);
		VectorNormalize(bs->goalMovedir);

		//RACC - Doing a nondelayed jump, stand still
		if (bs->jumpTime > level.time && bs->jDelay < level.time &&
			level.clients[bs->client].pers.cmd.upmove > 0)
		{
		//	trap_EA_Move(bs->client, bs->origin, 5000);
			bs->beStill = level.time + 200;
		}
		else
		{
			BotDirectSafeMove(bs, bs->goalMovedir, 5000);
		}

		//[BotTweaks]
		//cleaned this up to keep the traces/checks down
		if (meleestrafe && bs->meleeStrafeDisable < level.time)
		{
			StrafeTracing(bs);

			//StrafeTracing() can boost this level
			if(bs->meleeStrafeDisable < level.time)
			{
				if(bs->meleeStrafeDir)
				{
					BotDirectSafeButtonMove(bs, ACTION_MOVERIGHT);
				}
				else
				{
					BotDirectSafeButtonMove(bs, ACTION_MOVELEFT);
				}
			}
		}

		/*
		if (meleestrafe)
		{
			StrafeTracing(bs);
		}

		if (bs->meleeStrafeDir && meleestrafe && bs->meleeStrafeDisable < level.time)
		{
			BotDirectSafeButtonMove(bs, ACTION_MOVERIGHT);
		}
		else if (meleestrafe && bs->meleeStrafeDisable < level.time)
		{
			BotDirectSafeButtonMove(bs, ACTION_MOVELEFT);
		}
		*/
		//[/BotTweaks]

		//RACC - Try to jump
		if (BotTrace_Jump(bs, bs->goalPosition))
		{
			if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
			{
				client->jetPackOn = qtrue;
				client->ps.pm_type = PM_JETPACK;
				bs->cur_ps.eFlags |= EF_JETPACK;
				bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
				bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
				bs->jumpHoldTime = ((bs->forceJumpChargeTime - level.time)/2) + level.time;
				//bs->jumpHoldTime = level.time + 400;
			}
			bs->jumpTime = level.time + 100;
		}
		else if (BotTrace_Duck(bs, bs->goalPosition))
		{
			bs->duckTime = level.time + 100;
		}
		//RACC - Strafe around players that are in the way to the goalposition.
#ifdef BOT_STRAFE_AVOIDANCE
		else
		{
			int strafeAround = BotTrace_Strafe(bs, bs->goalPosition);

			if (strafeAround == STRAFEAROUND_RIGHT)
			{
				BotDirectSafeButtonMove(bs, ACTION_MOVERIGHT);
			}
			else if (strafeAround == STRAFEAROUND_LEFT)
			{
				BotDirectSafeButtonMove(bs, ACTION_MOVELEFT);
			}
		}
#endif
	}

#ifndef FORCEJUMP_INSTANTMETHOD
	if (bs->forceJumpChargeTime > level.time)
	{
		bs->jumpTime = 0;
	}
#endif

	if (bs->jumpPrep > level.time)
	{
		bs->forceJumpChargeTime = 0;
	}

	if (bs->forceJumpChargeTime > level.time)
	{
		bs->jumpHoldTime = ((bs->forceJumpChargeTime - level.time)/2) + level.time;
		bs->forceJumpChargeTime = 0;
	}

	if (bs->jumpHoldTime > level.time)

	{
		bs->jumpTime = bs->jumpHoldTime;
	}

	//RACC - Do jump code
	if (bs->jumpTime > level.time && bs->jDelay < level.time)
	{
		if ( bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
		{
			client->jetPackOn = qtrue;
				client->ps.pm_type = PM_JETPACK;
				bs->cur_ps.eFlags |= EF_JETPACK;
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
			bs->jumpHoldTime = ((bs->forceJumpChargeTime - level.time)/2) + level.time;
			//bs->jumpHoldTime = level.time + 400;

			if (bs->jumpHoldTime > level.time)
			{
				if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
				{
					g_entities[bs->client].client->ps.velocity[2] = 256;
				}
				else
				{
					trap_EA_Jump(bs->client);
				}
				if (bs->wpCurrent)
				{
					if ((bs->wpCurrent->origin[2] - bs->origin[2]) < 64)
					{
						BotDirectSafeButtonMove(bs, ACTION_MOVEFORWARD);
					}
				}
				else
				{
					BotDirectSafeButtonMove(bs, ACTION_MOVEFORWARD);
				}
				if (g_entities[bs->client].client->ps.groundEntityNum == ENTITYNUM_NONE)
				{
					g_entities[bs->client].client->ps.pm_flags |= PMF_JUMP_HELD;
				}
			}
			else if (!(bs->cur_ps.pm_flags & PMF_JUMP_HELD))
			{
				if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
				{
					g_entities[bs->client].client->ps.velocity[2] = 128;
				}
				else
				{
					trap_EA_Jump(bs->client);
				}
			}
		}
		else
		{
			if (bs->jumpHoldTime > level.time)
			{
				trap_EA_Jump(bs->client);
				if (bs->wpCurrent)
				{
					if ((bs->wpCurrent->origin[2] - bs->origin[2]) < 64)
					{
						BotDirectSafeButtonMove(bs, ACTION_MOVEFORWARD);
					}
				}
				else
				{
					BotDirectSafeButtonMove(bs, ACTION_MOVEFORWARD);
				}
				if (g_entities[bs->client].client->ps.groundEntityNum == ENTITYNUM_NONE)
				{
					g_entities[bs->client].client->ps.pm_flags |= PMF_JUMP_HELD;
				}
			}
			else if (!(bs->cur_ps.pm_flags & PMF_JUMP_HELD))
			{
				trap_EA_Jump(bs->client);
			}
		}
	}
	
	//RACC - Do couch code
	if (bs->duckTime > level.time)
	{
		trap_EA_Crouch(bs->client);
	}

	//RACC - lock onto dangerousObject and try to blast it if you're using ranged weapon and
	//not engaged with a enemy
	if ( bs->dangerousObject && bs->dangerousObject->inuse && bs->dangerousObject->health > 0 &&
		bs->dangerousObject->takedamage && (!bs->frame_Enemy_Vis || !bs->currentEnemy) &&
		(BotGetWeaponRange(bs) == BWEAPONRANGE_MID || BotGetWeaponRange(bs) == BWEAPONRANGE_LONG) &&
		bs->cur_ps.weapon != WP_DET_PACK && bs->cur_ps.weapon != WP_TRIP_MINE &&
		!bs->shootGoal )
	{
		float danLen;

		VectorSubtract(bs->dangerousObject->r.currentOrigin, bs->eye, a);

		danLen = VectorLength(a);

		if (danLen > 256)
		{
			vectoangles(a, a);
			VectorCopy(a, bs->goalAngles);

			if (Q_irand(1, 10) < 5)
			{
				bs->goalAngles[YAW] += Q_irand(0, 3);
				bs->goalAngles[PITCH] += Q_irand(0, 3);
			}
			else
			{
				bs->goalAngles[YAW] -= Q_irand(0, 3);
				bs->goalAngles[PITCH] -= Q_irand(0, 3);
			}

			if (InFieldOfVision(bs->viewangles, 30, a) &&
				EntityVisibleBox(bs->origin, NULL, NULL, bs->dangerousObject->r.currentOrigin, bs->client, bs->dangerousObject->s.number))
			{
				bs->doAttack = 1;
			}			
		}
	}

	if (PrimFiring(bs) ||
		AltFiring(bs))
	{
		friendInLOF = CheckForFriendInLOF(bs);

		if (friendInLOF)
		{
			if (PrimFiring(bs))
			{
				KeepPrimFromFiring(bs);
			}
			if (AltFiring(bs))
			{
				KeepAltFromFiring(bs);
			}
			if (useTheForce && forceHostile)
			{
				useTheForce = 0;
			}

			//[BotTweaks]
			//not really useful here.
			/*
			if (!useTheForce && friendInLOF->client)
			{ //we have a friend here and are not currently using force powers, see if we can help them out
				if (friendInLOF->health <= 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_HEAL]][FP_TEAM_HEAL])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_HEAL;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if (friendInLOF->client->ps.fd.forcePower <= 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_FORCE]][FP_TEAM_FORCE])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_FORCE;
					useTheForce = 1;
					forceHostile = 0;
				}
			}
			*/
			//[/BotTweaks]
		}
	}
	//RACC - Team force useage code
	else if (g_gametype.integer >= GT_TEAM)
	{ //still check for anyone to help..
		friendInLOF = CheckForFriendInLOF(bs);
/*
		if (!useTheForce && friendInLOF)
		{
			if (friendInLOF->health <= 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_HEAL]][FP_TEAM_HEAL])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_HEAL;
				useTheForce = 1;
				forceHostile = 0;
			}
			else if (friendInLOF->client->ps.fd.forcePower <= 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_FORCE]][FP_TEAM_FORCE])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_FORCE;
				useTheForce = 1;
				forceHostile = 0;
			}
		}
		*/
	}

	if (bs->doAttack && bs->cur_ps.weapon == WP_DET_PACK &&
		bs->cur_ps.hasDetPackPlanted)
	{ //maybe a bit hackish, but bots only want to plant one of these at any given time to avoid complications
		bs->doAttack = 0;
	}

	//RACC - "defending"
	if (bs->doAttack && bs->cur_ps.weapon == WP_SABER &&
		bs->saberDefending && bs->currentEnemy && bs->currentEnemy->client &&
		BotWeaponBlockable(bs->currentEnemy->client->ps.weapon) )
	{
		bs->doAttack = 0;
	}

	//RACC - saberlock button pressing code.
	//[BotTweaks]
	//prevent bot's from kicking your ass in saber locks when
	//sv_fps is higher
	if (bs->cur_ps.saberLockTime > level.time && bs->saberLockDebounce < level.time)
	//if (bs->cur_ps.saberLockTime > level.time)
	{
		if (rand()%10 < 5)
		{
			bs->doAttack = 1;
		}
		else
		{
			bs->doAttack = 0;
		}
		bs->saberLockDebounce = level.time + 50;
	}
	//[/BotTweaks]

	//RACC - Don't attack while trying to do a saber challenge.
	if (bs->botChallengingTime > level.time)
	{
		bs->doAttack = 0;
		bs->doAltAttack = 0;
		//[SaberSys]
		bs->doSaberThrow = 0;
		//[/SaberSys]
	}

	if (bs->cur_ps.weapon == WP_SABER &&
		bs->cur_ps.saberInFlight &&
		!bs->cur_ps.saberEntityNum)
	{ //saber knocked away, keep trying to get it back
		bs->doAttack = 1;
		bs->doAltAttack = 0;
	}

	//RACC - Do Attacks code.
	if (bs->doAttack)
	{
		trap_EA_Attack(bs->client);
	}
	else if (bs->doAltAttack)
	{
		trap_EA_Alt_Attack(bs->client);
	}


	//RACC - saber Challenging, don't use offence force powers.
	if (useTheForce && forceHostile && bs->botChallengingTime > level.time)
	{
		useTheForce = qfalse;
	}

	//RACC - Using the force.
	if (useTheForce)
	{
#ifndef FORCEJUMP_INSTANTMETHOD
		if (bs->forceJumpChargeTime > level.time)
		{
			if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
			{
				client->jetPackOn = qtrue;
				client->ps.pm_type = PM_JETPACK;
				bs->cur_ps.eFlags |= EF_JETPACK;
				bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
				bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
				bs->jumpHoldTime = ((bs->forceJumpChargeTime - level.time)/2) + level.time;
			}
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_LEVITATION;
			trap_EA_ForcePower(bs->client);
		}
		else
		{
#endif
			if (bot_forcepowers.integer && !g_forcePowerDisable.integer)
			{
				trap_EA_ForcePower(bs->client);
			}
#ifndef FORCEJUMP_INSTANTMETHOD
		}
#endif
	}

	MoveTowardIdealAngles(bs);
}

int gUpdateVars = 0;

/*
==================
BotAIStartFrame
==================
*/
//[BotTweaks]
extern void UpdateEditorMode(void);
//[/BotTweaks]
int BotAIStartFrame(int time) {
	int i;
	int elapsed_time, thinktime;
	static int local_time;
	//[BotTweak] NUAM
	//static int botlib_residual;
	//[/BotTweak]
	static int lastbotthink_time;

	if (gUpdateVars < level.time)
	{
		trap_Cvar_Update(&bot_pvstype);
		trap_Cvar_Update(&bot_camp);
		trap_Cvar_Update(&bot_attachments);
		trap_Cvar_Update(&bot_forgimmick);
		trap_Cvar_Update(&bot_honorableduelacceptance);
#ifndef FINAL_BUILD
		trap_Cvar_Update(&bot_getinthecarrr);
#endif
		//[BotTweaks]
		trap_Cvar_Update(&bot_fps);
		//[/BotTweaks]

		//[BotTweaks]
		trap_Cvar_Update(&bot_wp_edit);
		//[/BotTweaks]

		gUpdateVars = level.time + 1000;
	}

	//[BotTweaks]
	UpdateEditorMode();
	//[/BotTweaks]

	G_CheckBotSpawn();

	//rww - addl bot frame functions
	if (gBotEdit)
	{
		trap_Cvar_Update(&bot_wp_info);
		BotWaypointRender();
	}

	UpdateEventTracker();
	//end rww

	//cap the bot think time
	//if the bot think time changed we should reschedule the bots
	if (BOT_THINK_TIME != lastbotthink_time) {
		lastbotthink_time = BOT_THINK_TIME;
		BotScheduleBotThink();
	}

	elapsed_time = time - local_time;
	local_time = time;

	if (elapsed_time > BOT_THINK_TIME) thinktime = elapsed_time;
	else thinktime = BOT_THINK_TIME;

	// execute scheduled bot AI
	for( i = 0; i < MAX_CLIENTS; i++ ) {
		if( !botstates[i] || !botstates[i]->inuse ) {
			continue;
		}
		//
		botstates[i]->botthink_residual += elapsed_time;
		//
		if ( botstates[i]->botthink_residual >= thinktime ) {
			botstates[i]->botthink_residual -= thinktime;

			//[BotTweaks] UNIQUEFIXME - do we need these additional sanity checks?
			/*
			if (g_entities[i].client 
				&& g_entities[i].inuse
				&& g_entities[i].client->pers.connected == CON_CONNECTED) {
			*/
			if (g_entities[i].client->pers.connected == CON_CONNECTED) {
			//[/BotTweaks]
				BotAI(i, (float) thinktime / 1000);
			}
		}
	}

	// execute bot user commands every frame
	for( i = 0; i < MAX_CLIENTS; i++ ) {
		if( !botstates[i] || !botstates[i]->inuse ) {
			continue;
		}
		//[BotTweaks] UNIQUEFIXME - do we need these additional sanity checks?
		/*
		if( g_entities[i].client 
			&& g_entities[i].inuse
			&& g_entities[i].client->pers.connected != CON_CONNECTED ) {
		*/
		if( g_entities[i].client->pers.connected != CON_CONNECTED ) {
		//[/BotTweaks]
			continue;
		}

		BotUpdateInput(botstates[i], time, elapsed_time);
		trap_BotUserCommand(botstates[i]->client, &botstates[i]->lastucmd);
	}

	return qtrue;
}

/*
==============
BotAISetup
==============
*/
int BotAISetup( int restart ) {
	//rww - new bot cvars..
	trap_Cvar_Register(&bot_forcepowers, "bot_forcepowers", "1", CVAR_CHEAT);
	trap_Cvar_Register(&bot_forgimmick, "bot_forgimmick", "0", CVAR_CHEAT);
	trap_Cvar_Register(&bot_honorableduelacceptance, "bot_honorableduelacceptance", "0", CVAR_CHEAT);
	trap_Cvar_Register(&bot_pvstype, "bot_pvstype", "1", CVAR_CHEAT);
#ifndef FINAL_BUILD
	trap_Cvar_Register(&bot_getinthecarrr, "bot_getinthecarrr", "0", 0);
#endif

#ifdef _DEBUG
	trap_Cvar_Register(&bot_nogoals, "bot_nogoals", "0", CVAR_CHEAT);
	trap_Cvar_Register(&bot_debugmessages, "bot_debugmessages", "0", CVAR_CHEAT);
#endif

	trap_Cvar_Register(&bot_attachments, "bot_attachments", "1", 0);
	trap_Cvar_Register(&bot_camp, "bot_camp", "1", 0);

	trap_Cvar_Register(&bot_wp_info, "bot_wp_info", "1", 0);
	trap_Cvar_Register(&bot_wp_edit, "bot_wp_edit", "0", CVAR_CHEAT);
	trap_Cvar_Register(&bot_wp_clearweight, "bot_wp_clearweight", "1", 0);
	//[BotTweaks]
	//turned off the auto correction stuff because it sucks.
	trap_Cvar_Register(&bot_wp_distconnect, "bot_wp_distconnect", "0", 0);
	trap_Cvar_Register(&bot_wp_visconnect, "bot_wp_visconnect", "0", 0);
	//trap_Cvar_Register(&bot_wp_distconnect, "bot_wp_distconnect", "1", 0);
	//trap_Cvar_Register(&bot_wp_visconnect, "bot_wp_visconnect", "1", 0);

	//frame rate for Bot AI updates
	trap_Cvar_Register(&bot_fps, "bot_fps", "20", CVAR_ARCHIVE);

	//controls the clientNum of the player that receives the waypoint editor info.
	trap_Cvar_Register(&bot_wp_editornumber, "bot_wp_editornumber", "0", 0);
	//[/BotTweaks]

	trap_Cvar_Update(&bot_forcepowers);
	//end rww

	//if the game is restarted for a tournament
	if (restart) {
		return qtrue;
	}

	//initialize the bot states
	memset( botstates, 0, sizeof(botstates) );

	if (!trap_BotLibSetup())
	{
		return qfalse; //wts?!
	}

	return qtrue;
}

/*
==============
BotAIShutdown
==============
*/
int BotAIShutdown( int restart ) {

	int i;

	//if the game is restarted for a tournament
	if ( restart ) {
		//shutdown all the bots in the botlib
		for (i = 0; i < MAX_CLIENTS; i++) {
			if (botstates[i] && botstates[i]->inuse) {
				//[Linux] g++ type cast fix.
				BotAIShutdownClient(botstates[i]->client, (qboolean) restart);
				//BotAIShutdownClient(botstates[i]->client, restart);
				//[/Linux]
			}
		}
		//don't shutdown the bot library
	}
	else {
		trap_BotLibShutdown();
	}
	return qtrue;
}



 // -----------------------------------------------------------------
 // TAB bot order support (kept for compatibility with g_cmds.c)
 // -----------------------------------------------------------------
 static char *BotOrderNames[BOTORDER_MAX] =
 {
     "none",                     // BOTORDER_NONE
     "kneel before",             // BOTORDER_KNEELBEFOREZOD
     "attack",                   // BOTORDER_ATTACK
     "compete objectives",       // BOTORDER_OBJECTIVE
     "play JediMaster",          // BOTORDER_JEDIMASTER
     "enter a saber duel with"   // BOTORDER_SABERDUELCHALLENGE
 };

 void TAB_BotOrder(gentity_t *orderer, gentity_t *orderee, int order, gentity_t *objective)
 {
     bot_state_t *bs;

     if (!orderer || !orderee || !orderer->client || !orderee->client || !(orderee->r.svFlags & SVF_BOT))
     {
         return;
     }

     bs = botstates[orderee->client->ps.clientNum];
     if (!bs)
     {
         return;
     }

     // Orders with objective entities
     if (order == BOTORDER_KNEELBEFOREZOD ||
         order == BOTORDER_SABERDUELCHALLENGE ||
         (order == BOTORDER_SEARCHANDDESTROY && objective))
     {
         if (!objective)
         {
             return;
         }

         bs->botOrder = order;
         bs->orderEntity = objective;
         bs->ordererNum = orderer->client->ps.clientNum;

         if (objective->client)
         {
             G_Printf("%s ordered %s to %s %s\n",
                 orderer->client->pers.netname,
                 orderee->client->pers.netname,
                 BotOrderNames[order],
                 objective->client->pers.netname);
         }
     }
     else if (order == BOTORDER_SEARCHANDDESTROY)
     {
         bs->botOrder = order;
         bs->orderEntity = NULL;
         bs->ordererNum = orderer->client->ps.clientNum;

         G_Printf("%s ordered %s to %s\n",
             orderer->client->pers.netname,
             orderee->client->pers.netname,
             BotOrderNames[order]);
     }
     else
     {
         return;
     }

     BotDoChat(bs, "OrderAccepted", 1);
 }

 static qboolean CarryingCapObjective(const bot_state_t *bs)
{
    if (!bs)
    {
        return qfalse;
    }
    /* CTF flags are tracked as powerups in JKA. */
#ifdef PW_REDFLAG
    if (bs->cur_ps.powerups[PW_REDFLAG] || bs->cur_ps.powerups[PW_BLUEFLAG])
    {
        return qtrue;
    }
#endif
#ifdef PW_NEUTRALFLAG
    if (bs->cur_ps.powerups[PW_NEUTRALFLAG])
    {
        return qtrue;
    }
#endif
    return qfalse;
}

void TAB_BotSaberDuelChallenged(gentity_t *bot, gentity_t *player)
 { // Handle when player challenges this bot to a saber duel.
     bot_state_t *bs;

     if (!bot || !player)
     {
         return;
     }

     bs = botstates[bot->s.number];
     if (!bs)
     {
         return;
     }

     if (!bot_honorableduelacceptance.integer)
     { // don't accept duel challenges if bots aren't supposed to.
         return;
     }

     // Don't accept if preoccupied or carrying an objective.
     if (bs->botOrder != BOTORDER_NONE)
     {
         return;
     }

     if (CarryingCapObjective(bs))
     {
         return;
     }

     // Accept duel by switching to the temporary duel tactic.
     bs->currentTactic = BOTORDER_SABERDUELCHALLENGE;
     bs->tacticEntity = player;

     // Debounce so it doesn't instantly accept (matches old TAB behavior).
     bs->MiscBotFlags |= BOTFLAG_SABERCHALLENGED;
     bs->miscBotFlagsTimer = level.time + Q_irand(2000, 5000);
 }