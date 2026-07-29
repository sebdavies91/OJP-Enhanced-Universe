//This file contains functions relate to the saber impact behavior of Open Battlefront Project's saber system.
#include "g_local.h"
#include "g_saberbeh.h"
#include "ai_main.h"

static int SabBeh_StyleClashPower(int saberStyle)
{
	switch (saberStyle)
	{
	case SS_FAST:
		// SORESU: clean recovery and defense, low clash pressure.
		return 0;
	case SS_MEDIUM:
		// SHII-CHO: reliable baseline.
		return 1;
	case SS_TAVION:
		// MAKASHI: precise duelist pressure, not raw force.
		return 2;
	case SS_STAFF:
		// NIMAN: stable, controlled staff flow.
		return 2;
	case SS_DUAL:
		// JAR'KAI: repeated pressure from two blades.
		return 3;
	case SS_DESANN:
		// JUYO: aggressive medium-heavy pressure.
		return 4;
	case SS_STRONG:
		// DJEM SO: strongest committed clash pressure.
		return 5;
	default:
		return 1;
	}
}

static int SabBeh_StyleClashRecovery(int saberStyle)
{
	switch (saberStyle)
	{
	case SS_FAST:
		return 2;
	case SS_TAVION:
		return 1;
	case SS_MEDIUM:
	case SS_STAFF:
		return 0;
	case SS_DUAL:
		return -1;
	case SS_DESANN:
		return -1;
	case SS_STRONG:
		return -2;
	default:
		return 0;
	}
}

static void SabBeh_ApplyStyleClash(gentity_t *self, sabmech_t *mechSelf, gentity_t *otherOwner, sabmech_t *mechOther)
{
	int selfStyle, otherStyle;
	int selfScore, otherScore, diff;

	if (!self || !self->client || !otherOwner || !otherOwner->client || !mechSelf || !mechOther)
	{
		return;
	}

	selfStyle = self->client->ps.fd.saberAnimLevel;
	otherStyle = otherOwner->client->ps.fd.saberAnimLevel;

	selfScore = SabBeh_StyleClashPower(selfStyle) + SabBeh_StyleClashRecovery(selfStyle);
	otherScore = SabBeh_StyleClashPower(otherStyle) + SabBeh_StyleClashRecovery(otherStyle);
	diff = selfScore - otherScore;

	// Clash DP pressure: power forms push guards harder, recovery forms bleed less DP.
	if (diff > 0)
	{
		G_DodgeDrain(otherOwner, self, diff);
	}
	else if (diff < 0)
	{
		G_DodgeDrain(self, otherOwner, -diff);
	}

	// Keep rare hard stagger: only clearly superior clash pressure should force a slow bounce.
	// This makes Djem So/Juyo/Jar'Kai feel forceful without creating constant mishaps.
	if (diff >= 4 && !mechOther->doSlowBounce && !mechOther->doHeavySlowBounce && !mechOther->doStun && Q_irand(0, 99) < 12)
	{
		mechOther->doSlowBounce = qtrue;
	}
	else if (diff <= -4 && !mechSelf->doSlowBounce && !mechSelf->doHeavySlowBounce && !mechSelf->doStun && Q_irand(0, 99) < 12)
	{
		mechSelf->doSlowBounce = qtrue;
	}
}


GAME_INLINE void ClearSabMech( sabmech_t *sabmech)
{
	if ( !sabmech )
	{
		return;
	}
	sabmech->doStun = qfalse;
	sabmech->doKnockdown = qfalse;
	sabmech->doButterFingers = qfalse;
	sabmech->doParry = qfalse;
	sabmech->doSlowBounce = qfalse;
	sabmech->doHeavySlowBounce = qfalse;
#ifdef _DEBUG
	sabmech->behaveMode = SABBEHAVE_NONE;
#endif
}



extern qboolean BG_SaberInNonIdleDamageMove(playerState_t *ps, int AnimIndex);
qboolean SabBeh_RollBalance(gentity_t *self, sabmech_t *mechSelf, qboolean forceMishap)
{
	int randNum; 
	/*
	Racc - Bugfix:
		This function previously used "randNum < 0" checks after Q_irand(0, 99),
		which can never succeed. That effectively disabled random mishaps and only
		allowed forced mishaps.
	
		Keep this conservative: low default chances, and forced mishaps still work.
	*/
	const int kChanceHeavyBounce = 8; /* percent */
	const int kChanceSlowBounce  = 10; /* percent */

	if ( !self || !self->client || !mechSelf )
	{
		return qfalse;
	}

	if( self->client->ps.MISHAP_VARIABLE <= MISHAPLEVEL_FULL )
	{//hard mishap.
		//mechSelf->doKnockdown = qtrue;
		mechSelf->doButterFingers= qtrue;
		//self->client->ps.MISHAP_VARIABLE = MISHAPLEVEL_HEAVY;
		return qtrue;
	}
	
	else if( self->client->ps.stats[STAT_DODGE] < DODGE_CRITICALLEVEL )//added by JRHockney to do more heavybounces like old times
	{//heavy slow bounce
		randNum = Q_irand(0, 99);
		if(randNum < kChanceHeavyBounce || forceMishap)
		{
			mechSelf->doHeavySlowBounce = qtrue;
			return qtrue;
		}
	}
	else if( self->client->ps.MISHAP_VARIABLE <= MISHAPLEVEL_HEAVY )
	{//heavy slow bounce
		randNum = Q_irand(0, 99);
		if(randNum < kChanceHeavyBounce || forceMishap)
		{
			mechSelf->doHeavySlowBounce = qtrue;
			//self->client->ps.MISHAP_VARIABLE = MISHAPLEVEL_LIGHT;
			return qtrue;
		}
	}
	else if( self->client->ps.MISHAP_VARIABLE <= MISHAPLEVEL_LIGHT )
	{//slow bounce
		randNum = Q_irand(0, 99);
		if(randNum < kChanceSlowBounce || forceMishap)
		{
			mechSelf->doSlowBounce = qtrue;
			//self->client->ps.MISHAP_VARIABLE = MISHAPLEVEL_NONE;
			return qtrue;
		}
	}
	else if( forceMishap )
	{//perform a slow bounce even if we don't have enough mishap for it.
		mechSelf->doSlowBounce = qtrue;
		//self->client->ps.MISHAP_VARIABLE = MISHAPLEVEL_NONE;
		return qtrue;
	}

	return qfalse;
}


//[WeapAccuracy]
extern qboolean /*GAME_INLINE*/ WalkCheck( gentity_t * self );
void G_AddMercBalance(gentity_t *self, int amount)
{//mercs don't suffer mishaps, but they do lose/gain MP
	/*
	if(!WalkCheck(self))
	{//running or moving very fast, can't balance as well
		if(amount > 0)
		{
			amount *= 2;
		}
		else
		{
			amount = amount * .5f;
		}
	}

	//G_Printf("%i: %i: %i Mishap Points\n", level.time, self->s.number, amount);

	self->client->ps.MISHAP_VARIABLE -= amount;

	if(self->client->ps.MISHAP_VARIABLE < 0)
	{
		self->client->ps.MISHAP_VARIABLE = MISHAPLEVEL_NONE;
	}
	else if(self->client->ps.MISHAP_VARIABLE > MISHAPLEVEL_MAX)
	{
		self->client->ps.MISHAP_VARIABLE = MISHAPLEVEL_MAX;
	}
	*/
}
//[/WeapAccuracy]


extern qboolean saberKnockOutOfHand(gentity_t *saberent, gentity_t *saberOwner, vec3_t velocity);
extern void AnimateKnockdown( gentity_t * self, gentity_t * inflictor );
void G_RollBalance(gentity_t *self, gentity_t *inflictor, qboolean forceMishap)
{//This function is for rolling saber mishaps outside the saber behavior code.
	sabmech_t mechSelf;

	ClearSabMech(&mechSelf);

	if(SabBeh_RollBalance(self, &mechSelf, forceMishap))
	{//mishap occurred, animate it
		if(mechSelf.doButterFingers)
		{
			//RAFIXME - impliment lose vector handling.
			//ButterFingers(&g_entities[self->client->ps.saberEntityNum], self, inflictor, &tr);
			if ( self && self->client
				&& self->client->ps.saberEntityNum > 0
				&& self->client->ps.saberEntityNum < MAX_GENTITIES
				&& g_entities[self->client->ps.saberEntityNum].inuse )
			{
				saberKnockOutOfHand(&g_entities[self->client->ps.saberEntityNum], self, vec3_origin);
			}
		}

		if(mechSelf.doKnockdown)
		{
			AnimateKnockdown(self, inflictor);
		}
			else if (mechSelf.doStun)
			{
				// RAFIXME - implement impact point properly.
				// Be defensive: self/client can be NULL in unusual damage callback flows.
				if ( self && self->client )
				{
					AnimateStun(self, inflictor, self->client->ps.origin);
				}
				else
				{
					AnimateStun(self, inflictor, vec3_origin);
				}
			}
		else if(mechSelf.doSlowBounce)
		{
			SabBeh_AnimateSlowBounce(self, inflictor);
		}
		else if(mechSelf.doHeavySlowBounce)
		{
			SabBeh_AnimateHeavySlowBounce(self, inflictor);
		}
	}
}


void SabBeh_AddBalance(gentity_t *self, sabmech_t *mechSelf, int amount, qboolean attack)
{
	/*
	if(!WalkCheck(self))
	{//running or moving very fast, can't balance as well
		if(amount > 0)
		{
			amount *= 2;
		}
		else
		{
			amount = amount * .5f;
		}
	}

	//G_Printf("%i: %i: %i Mishap Points\n", level.time, self->s.number, amount);

	self->client->ps.MISHAP_VARIABLE -= amount;

	if(self->client->ps.MISHAP_VARIABLE < 0)
	{
		self->client->ps.MISHAP_VARIABLE = MISHAPLEVEL_NONE;
	}
	else if(self->client->ps.MISHAP_VARIABLE > MISHAPLEVEL_MAX)
	{//overflowing causes a full mishap.
		int randNum = Q_irand(0, 2);
		switch (randNum)
		{
		case 0:
			mechSelf->doButterFingers = qtrue;
			break;
		case 1:
			mechSelf->doKnockdown = qtrue;
			break;
		};
		self->client->ps.MISHAP_VARIABLE = MISHAPLEVEL_HEAVY;
	}
	*/
}


extern qboolean WP_SabersCheckLock( gentity_t *ent1, gentity_t *ent2 );
void SabBeh_AttackVsAttack( gentity_t *self, sabmech_t *mechSelf, 
								gentity_t *otherOwner, sabmech_t *mechOther, qboolean *selfMishap, qboolean *otherMishap )
{//set the saber behavior for two attacking blades hitting each other
	qboolean atkfake = (self->client->ps.userInt3 & (1 << FLAG_ATTACKFAKE)) 
		? qtrue : qfalse;
	qboolean otherfake = (otherOwner->client->ps.userInt3 & (1 << FLAG_ATTACKFAKE)) 
		? qtrue : qfalse;

	if(atkfake && !otherfake)
	{//self is sololy faking
		//set self
		SabBeh_AddBalance(self, mechSelf, 1, qtrue);
#ifdef _DEBUG
		mechSelf->behaveMode = SABBEHAVE_BLOCKFAKED;
#endif

		//set otherOwner
		if (WP_SabersCheckLock(self, otherOwner))
		{	
			self->client->ps.userInt3 |= ( 1 << FLAG_SABERLOCK_ATTACKER );
			self->client->ps.saberBlocked = BLOCKED_NONE;
			otherOwner->client->ps.saberBlocked = BLOCKED_NONE;
		}
		SabBeh_AddBalance(otherOwner, mechOther, -1, qtrue);
#ifdef _DEBUG
		mechOther->behaveMode = SABBEHAVE_ATTACK;
#endif
	}
	else if(otherfake && !atkfake)
	{//only otherOwner is faking
		//set self
		if (WP_SabersCheckLock(otherOwner, self))
		{	
			self->client->ps.saberBlocked = BLOCKED_NONE;
			otherOwner->client->ps.userInt3 |= ( 1 << FLAG_SABERLOCK_ATTACKER );
			otherOwner->client->ps.saberBlocked = BLOCKED_NONE;
		}
		SabBeh_AddBalance(self, mechSelf, -1, qtrue);
#ifdef _DEBUG
		mechSelf->behaveMode = SABBEHAVE_ATTACK;
#endif

		//set otherOwner
		SabBeh_AddBalance(otherOwner, mechOther, 1, qtrue);
#ifdef _DEBUG
		mechOther->behaveMode = SABBEHAVE_BLOCKFAKED;
#endif
	}
	else
	{//either both are faking or neither is faking.  Either way, it's cancelled out
		//set self
		SabBeh_AddBalance(self, mechSelf, 1, qtrue);
#ifdef _DEBUG
		mechSelf->behaveMode = SABBEHAVE_ATTACK;
#endif

		//set otherOwner
		SabBeh_AddBalance(otherOwner, mechOther, 1, qtrue);
#ifdef _DEBUG
		mechOther->behaveMode = SABBEHAVE_ATTACK;
#endif

		SabBeh_ApplyStyleClash(self, mechSelf, otherOwner, mechOther);
	}
}

extern bot_state_t *botstates[MAX_CLIENTS];
extern qboolean BG_SuperBreakWinAnim( int anim );
extern stringID_table_t SaberMoveTable[];
extern stringID_table_t animTable [MAX_ANIMATIONS+1];
extern qboolean BG_InSlowBounce(playerState_t *ps);
extern qboolean G_InAttackParry(gentity_t *self);
extern int OBP_SaberBlockCost(gentity_t *defender, gentity_t *attacker, vec3_t hitLoc);
extern void WP_SaberBlockNonRandom( gentity_t *self, vec3_t hitloc, qboolean missileBlock );
extern qboolean G_BlockIsParry( gentity_t *self, gentity_t *attacker, vec3_t hitLoc );
extern qboolean G_BlockIsQuickParry( gentity_t *self, gentity_t *attacker, vec3_t hitLoc );
extern void BG_AddFatigue( playerState_t * ps, int Fatigue);
void SabBeh_AttackVsBlock( gentity_t *attacker, sabmech_t *mechAttacker, 
								gentity_t *blocker, sabmech_t *mechBlocker, vec3_t hitLoc, qboolean hitSaberBlade,
								qboolean *attackerMishap, qboolean *blockerMishap)
{//set the saber behavior for an attacking vs blocking/parrying blade impact
	qboolean startSaberLock = qfalse;
	qboolean parried = G_BlockIsParry(blocker, attacker, hitLoc);
//	qboolean quickParried = G_BlockIsQuickParry(blocker,attacker,hitLoc);
	qboolean atkparry = G_InAttackParry(blocker);
	qboolean atkfake = (attacker->client->ps.userInt3 & (1 << FLAG_ATTACKFAKE)) 
		? qtrue : qfalse;

	if(parried && blocker->r.svFlags & SVF_BOT 
		&& BOT_ATTACKPARRYRATE * botstates[blocker->s.number]->settings.skill > Q_irand(0,999))
	{//bot performed an attack parry (by cheating a bit)
		//G_Printf("%i: %i: Bot Cheat Attack Parried\n", level.time, blocker->s.number);
		atkparry = qtrue;
	}

	/*
	if(parried && atkparry)
	{
		G_Printf("%i: %i: Attack Parried\n", level.time, blocker->s.number);
	}
	*/

	if(BG_SuperBreakWinAnim(attacker->client->ps.torsoAnim))
	{//attacker was attempting a superbreak and he hit someone who could block the move, rail him for screwing up.
		*attackerMishap = SabBeh_RollBalance(attacker, mechAttacker, qtrue);
		SabBeh_AddBalance(attacker, mechAttacker, 2, qtrue);
#ifdef _DEBUG
			mechAttacker->behaveMode = SABBEHAVE_ATTACKPARRIED;
#endif

		SabBeh_AddBalance(blocker, mechBlocker, -1, qfalse);
#ifdef _DEBUG
			mechBlocker->behaveMode = SABBEHAVE_BLOCK;
#endif
	}
	else if(atkfake)
	{//attacker faked before making this attack, treat like standard attack/attack
		if(parried)
		{//defender parried the attack fake.
			*attackerMishap = SabBeh_RollBalance(attacker, mechAttacker, atkparry);
			SabBeh_AddBalance(attacker, mechAttacker, MPCOST_PARRIED_ATTACKFAKE, qtrue);
#ifdef _DEBUG
			mechAttacker->behaveMode = SABBEHAVE_ATTACK;
#endif

//			if (blocker->client->pers.cmd.buttons & BUTTON_15)
//			{
//				attacker->client->ps.userInt3 |= (1 << FLAG_QUICKPARRY);
//			}
//			else
			{
			attacker->client->ps.userInt3 |= ( 1 << FLAG_PARRIED );
			}

			SabBeh_AddBalance(blocker, mechBlocker, MPCOST_PARRYING_ATTACKFAKE, qfalse);
			BG_AddFatigue(&blocker->client->ps, FPCOST_PARRYING_PURE);
#ifdef _DEBUG
			mechBlocker->behaveMode = SABBEHAVE_BLOCK;
#endif
		}
		else
		{//otherwise, the defender stands a good chance of having his defensives broken.	
			SabBeh_AddBalance(attacker, mechAttacker, -1, qtrue);

			if(attacker->client->ps.fd.saberAnimLevel == SS_DESANN)
			{// JUYO: attack fakes carry strong guard pressure if not parried.
				SabBeh_AddBalance(blocker, mechBlocker, 2, qfalse);
				G_DodgeDrain(blocker, attacker, 2);
			}
			else if(attacker->client->ps.fd.saberAnimLevel == SS_STRONG)
			{// DJEM SO: punishing if the defender fails to parry.
				G_DodgeDrain(blocker, attacker, 3);
			}
			else if(attacker->client->ps.fd.saberAnimLevel == SS_DUAL)
			{// JAR_KAI: repeated blade pressure.
				G_DodgeDrain(blocker, attacker, 1);
			}

#ifdef _DEBUG
			mechAttacker->behaveMode = SABBEHAVE_ATTACK;
#endif
			if (WP_SabersCheckLock(attacker, blocker))
			{	
				attacker->client->ps.userInt3 |= ( 1 << FLAG_SABERLOCK_ATTACKER );
				attacker->client->ps.saberBlocked = BLOCKED_NONE;
				blocker->client->ps.saberBlocked = BLOCKED_NONE;
				startSaberLock = qtrue;
			}
			
#ifdef _DEBUG
			mechBlocker->behaveMode = SABBEHAVE_BLOCKFAKED;
#endif
		}

	}
	else if(hitSaberBlade && BG_InSlowBounce(&blocker->client->ps) 
		&& blocker->client->ps.userInt3 & (1 << FLAG_OLDSLOWBOUNCE)
		&& attacker->client->ps.fd.saberAnimLevel == SS_TAVION)
	{//blocker's saber was directly hit while in a slow bounce, disarm the blocker!
		mechBlocker->doButterFingers = qtrue;
		blocker->client->ps.MISHAP_VARIABLE = 0;
#ifdef _DEBUG
		mechBlocker->behaveMode = SABBEHAVE_BLOCKFAKED;
#endif

		//set attacker
		SabBeh_AddBalance(attacker, mechAttacker, -3, qtrue);
#ifdef _DEBUG
		mechAttacker->behaveMode = SABBEHAVE_ATTACK;
#endif
	}
	else
	{//standard attack
		//set blocker
#ifdef _DEBUG
		mechBlocker->behaveMode = SABBEHAVE_BLOCK;
#endif

		//set attacker
		if(parried)
		{
		//parry values
			if(attacker->client->ps.saberMove == LS_A_LUNGE
			|| attacker->client->ps.saberMove == LS_SPINATTACK
			|| attacker->client->ps.saberMove == LS_SPINATTACK_DUAL)
			{//attacker's lunge was parried, force mishap.
				*attackerMishap = SabBeh_RollBalance(attacker, mechAttacker, qtrue);
			}
			else
			{
				*attackerMishap = SabBeh_RollBalance(attacker, mechAttacker, atkparry);
			}
			SabBeh_AddBalance(attacker, mechAttacker, MPCOST_PARRIED, qtrue);
#ifdef _DEBUG
			mechAttacker->behaveMode = SABBEHAVE_ATTACKPARRIED;
#endif
			//[QuickParry]
//			if (blocker->client->pers.cmd.buttons & BUTTON_15)
//			{
//				attacker->client->ps.userInt3 |= ( 1 << FLAG_QUICKPARRY);
//			}
//			else
			{
			attacker->client->ps.userInt3 |= ( 1 << FLAG_PARRIED );
			}
			//[/QuickParry]

			SabBeh_AddBalance(blocker, mechBlocker, MPCOST_PARRYING, qfalse);
			BG_AddFatigue(&blocker->client->ps, FPCOST_PARRYING_PURE);

			// Style-specific parry results: precise/defensive forms recover cleaner,
			// power/aggression forms are punished harder when cleanly read.
			switch(attacker->client->ps.fd.saberAnimLevel)
			{
			case SS_STRONG:
			case SS_DESANN:
				G_DodgeDrain(attacker, blocker, 3);
				break;
			case SS_DUAL:
				G_DodgeDrain(attacker, blocker, 2);
				break;
			case SS_TAVION:
				G_DodgeDrain(attacker, blocker, 1);
				break;
			default:
				break;
			}
			
		}
		else
		{//blocked values
			SabBeh_AddBalance(attacker, mechAttacker, -1, qtrue);
			if(attacker->client->ps.fd.saberAnimLevel == SS_TAVION)
			{// MAKASHI: precise pressure against an ordinary block, but not raw knockback.
				SabBeh_AddBalance(blocker, mechBlocker, 1, qfalse);
				G_DodgeDrain(blocker, attacker, 1);
			}
			else if(attacker->client->ps.fd.saberAnimLevel == SS_DESANN)
			{// JUYO: aggressive guard pressure when not cleanly parried.
				SabBeh_AddBalance(blocker, mechBlocker, 2, qfalse);
				G_DodgeDrain(blocker, attacker, 2);
			}
			else if(attacker->client->ps.fd.saberAnimLevel == SS_STRONG)
			{// DJEM SO: heavy counter-pressure, but only on failed parries/ordinary blocks.
				blocker->client->ps.fd.forcePower -= 2;
				G_DodgeDrain(blocker, attacker, 3);
			}
			else if(attacker->client->ps.fd.saberAnimLevel == SS_DUAL)
			{// JAR_KAI: dual-blade pressure stacks through repeated contacts.
				G_DodgeDrain(blocker, attacker, 1);
			}
			else if(attacker->client->ps.fd.saberAnimLevel == SS_STAFF)
			{// NIMAN: stable, controlled staff pressure.
				G_DodgeDrain(blocker, attacker, 1);
			}
			
#ifdef _DEBUG
			mechAttacker->behaveMode = SABBEHAVE_ATTACKBLOCKED;
#endif

			//SabBeh_AddBalance(blocker, 1, qfalse);
		}
	}

	if(!OnSameTeam(attacker, blocker) || g_friendlySaber.integer)
	{//don't do parries or charge/regen DP unless we're in a situation where we can actually hurt the target.
		if(parried)
		{//parries don't cost any DP and they have a special animation
			//qboolean regenSound = qfalse;
			mechBlocker->doParry = qtrue;
		}
		else if(!startSaberLock)
		{//normal saber blocks
			//update the blocker's block move
			blocker->client->ps.saberLockFrame = 0; //break out of saberlocks.
			WP_SaberBlockNonRandom(blocker, hitLoc, qfalse);
		}
	}

	//do saber DP cost.

	/*
	// debugger message.
	G_Printf("%i: %i: Saber Block Cost: %i atk: %s %s blk: %s %s\n", level.time, blocker->s.number, OBP_SaberBlockCost(blocker, attacker, hitLoc), 
		GetStringForID( animTable, attacker->client->ps.torsoAnim ), GetStringForID( SaberMoveTable, attacker->client->ps.saberMove ), 
		GetStringForID( animTable, blocker->client->ps.torsoAnim ), GetStringForID( SaberMoveTable, blocker->client->ps.saberMove ) );
	*/

	//[ExpSys] -- [DUALRAWR]
	G_DodgeDrain(blocker, attacker, OBP_SaberBlockCost(blocker, attacker, hitLoc));
	//[/ExpSys]

	//costs FP as well.
	BG_AddFatigue(&blocker->client->ps, 1);
}


extern int OBP_SaberCanBlock(gentity_t *self, gentity_t *atk, qboolean checkBBoxBlock, vec3_t point, int rSaberNum, int rBladeNum);
void SabBeh_RunSaberBehavior( gentity_t *self, sabmech_t *mechSelf, 
								gentity_t *otherOwner, sabmech_t *mechOther, vec3_t hitLoc, 
								qboolean *didHit, qboolean otherHitSaberBlade )
{	
	qboolean selfMishap = qfalse;
	qboolean otherMishap = qfalse;

	//initalize the sab mechanic data.
	ClearSabMech(mechSelf);
	ClearSabMech(mechOther);

	if(!otherOwner)
	{//not a saber-on-saber hit, no mishap handling.
		return;
	}

	//G_Printf("BG_SaberInNonIdleDamageMove\n");
	if(BG_SaberInNonIdleDamageMove(&self->client->ps, self->localAnimIndex) )
	{//self is attacking
		//G_Printf("(y)\n");
		if(BG_SaberInNonIdleDamageMove(&otherOwner->client->ps, otherOwner->localAnimIndex)) 
		{//and otherOwner is attacking
			SabBeh_AttackVsAttack(self, mechSelf, otherOwner, mechOther, &selfMishap, &otherMishap);
		}
		else if(OBP_SaberCanBlock(otherOwner, self, qfalse, hitLoc, -1, -1))
		{//and otherOwner is blocking or parrying
			//this is called with dual with both sabers[DUALRAWR]
			SabBeh_AttackVsBlock(self, mechSelf, otherOwner, mechOther, hitLoc, otherHitSaberBlade,
				&selfMishap, &otherMishap);
			*didHit = qfalse;
		}
		else
		{//otherOwner in some other state
			//no mishaps for this at the moment.
		}
	}
	else if( OBP_SaberCanBlock(self, otherOwner, qfalse, hitLoc, -1, -1) )
	{//self is blocking or parrying
		if(BG_SaberInNonIdleDamageMove(&otherOwner->client->ps, otherOwner->localAnimIndex))
		{//and otherOwner is attacking
			SabBeh_AttackVsBlock(otherOwner, mechOther, self, mechSelf, hitLoc, qtrue,
				&otherMishap, &selfMishap);
		}
		else if(OBP_SaberCanBlock(otherOwner, self, qfalse, hitLoc, -1, -1))
		{//and otherOwner is blocking or parrying
		}
		else
		{//otherOwner in some other state
			//no mishaps for this at the moment.
		}
	}
	else
	{//whatever other states self can be in.  (returns, bounces, or something)
		//just act like no mishaps can happen
	}
}


extern void NPC_Pain(gentity_t *self, gentity_t *attacker, int damage);
void SabBeh_AnimateSlowBounce(gentity_t* self, gentity_t *inflictor)
{
	self->client->ps.userInt3 |= ( 1 << FLAG_SLOWBOUNCE );
	if ( self->s.number < MAX_CLIENTS )
	{
		G_AddEvent( self, Q_irand(EV_PUSHED1, EV_PUSHED3), 0 );
	}
	else
	{//npc
		NPC_Pain( self, inflictor, 0 );
	}
	self->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
}


void SabBeh_AnimateHeavySlowBounce(gentity_t* self, gentity_t *inflictor)
{
	self->client->ps.userInt3 |= ( 1 << FLAG_SLOWBOUNCE );
	self->client->ps.userInt3 |= ( 1 << FLAG_OLDSLOWBOUNCE );
	if ( self->s.number < MAX_CLIENTS )
	{
		G_AddEvent( self, Q_irand(EV_PUSHED1, EV_PUSHED3), 0 );
	}
	else
	{//npc
		NPC_Pain( self, inflictor, 0 );
	}
	self->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
}


//[SaberLockSys]
qboolean SabBeh_ButtonforSaberLock(gentity_t* self)
{//checks to see if the player is pressing the correct direction to advance the saber lock for this saber lock animation.

	//must be holding down alt-attack
	if(!(self->client->pers.cmd.buttons & BUTTON_ALT_ATTACK))
	{
		return qfalse;
	}

	switch(self->client->ps.torsoAnim)
	{
	//top attacks
	case BOTH_BF1LOCK:
	case BOTH_BF2LOCK:
	case BOTH_LK_S_DL_T_L_1:
	case BOTH_LK_S_ST_T_L_1:
	case BOTH_LK_S_S_T_L_1:
	case BOTH_LK_DL_DL_T_L_1:
	case BOTH_LK_DL_ST_T_L_1:
	case BOTH_LK_DL_S_T_L_1:
	case BOTH_LK_ST_DL_T_L_1:
	case BOTH_LK_ST_ST_T_L_1:
	case BOTH_LK_ST_S_T_L_1:
		if(self->client->pers.cmd.forwardmove > 0)
		{
			return qtrue;
		}
		break;
	//right
	case BOTH_CWCIRCLELOCK:
	case BOTH_CCWCIRCLELOCK:
	case BOTH_LK_S_DL_S_L_1:
	case BOTH_LK_S_ST_S_L_1:
	case BOTH_LK_S_S_S_L_1:
	case BOTH_LK_DL_DL_S_L_1:
	case BOTH_LK_DL_ST_S_L_1:
	case BOTH_LK_DL_S_S_L_1:
	case BOTH_LK_ST_DL_S_L_1:
	case BOTH_LK_ST_ST_S_L_1:
	case BOTH_LK_ST_S_S_L_1:
		if(self->client->pers.cmd.rightmove > 0)
		{
			return qtrue;
		}
		break;
	default:
		return qfalse;
	};

	return qfalse;
}
//[/SaberLockSys]


//[SaberSys]
void BG_ReduceMishapLevel(playerState_t *ps)
{//reduces a player's mishap meter by one level
	/*
	if(ps->MISHAP_VARIABLE <= MISHAPLEVEL_FULL)
	{
		ps->MISHAP_VARIABLE = MISHAPLEVEL_HEAVY;
	}
	else if(ps->MISHAP_VARIABLE <= MISHAPLEVEL_HEAVY)
	{
		ps->MISHAP_VARIABLE = MISHAPLEVEL_LIGHT;
	}
	else if(ps->MISHAP_VARIABLE <= MISHAPLEVEL_LIGHT)
	{
		ps->MISHAP_VARIABLE = MISHAPLEVEL_NONE;
	}
	else
	{
		ps->MISHAP_VARIABLE = MISHAPLEVEL_NONE;
	}
	*/

}
//[/SaberSys]

