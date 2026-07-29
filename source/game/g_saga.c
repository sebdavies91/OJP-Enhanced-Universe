// Copyright (C) 2000-2002 Raven Software, Inc.
//
/*****************************************************************************
 * name:		g_saga.c
 *
 * desc:		Game-side module for Siege gametype.
 *****************************************************************************/
#include "g_local.h"
#include "bg_saga.h"

#define SIEGEITEM_STARTOFFRADAR 8

static char		team1[512];
static char		team2[512];

siegePers_t	g_siegePersistant = {qfalse, 0, 0};

int			imperial_goals_required = 0;
int			imperial_goals_completed = 0;
int			rebel_goals_required = 0;
int			rebel_goals_completed = 0;

int			imperial_time_limit = 0;
int			rebel_time_limit = 0;

int			gImperialCountdown = 0;
int			gRebelCountdown = 0;

int			rebel_attackers = 0;
int			imperial_attackers = 0;

qboolean	gSiegeRoundBegun = qfalse;
qboolean	gSiegeRoundEnded = qfalse;
qboolean	gSiegeRoundWinningTeam = 0;
int			gSiegeBeginTime = Q3_INFINITE;

int			g_preroundState = 0; //default to starting as spec (1 is starting ingame)

//[TABBot]
//data area for objective depandancy
//we assume that the dependancy stuff is only for the attacking team since the defenders 
//never seem to have attackable objectives.
int ObjectiveDependancy[MAX_OBJECTIVES][MAX_OBJECTIVEDEPENDANCY];
//[/TABBot]


void G_LogExit( const char *string );
void SetTeamQuick(gentity_t *ent, int team, qboolean doBegin);

static char gParseObjectives[MAX_SIEGE_INFO_SIZE];

//RACC - holds the completed objective data.
//[TABBot]
//used in ai_tab.c don't do static anymore.
char gObjectiveCfgStr[1024];
//static char gObjectiveCfgStr[1024];
//[/TABBot]

static int gSiegeNextObjectiveHintTime = 0;
static char gSiegeLastObjectiveHintCfg[sizeof(gObjectiveCfgStr)];

//go through all classes on a team and register their
//weapons and items for precaching.
void G_SiegeRegisterWeaponsAndHoldables(int team)
{
	siegeTeam_t *stm = BG_SiegeFindThemeForTeam(team);

	if (stm)
	{
		int i = 0;
		siegeClass_t *scl;
		while (i < stm->numClasses)
		{
			scl = stm->classes[i];

			if (scl)
			{
				int j = 0;
				while (j < WP_NUM_WEAPONS)
				{
					if (scl->weapons & (1 << j))
					{ //we use this weapon so register it.
						RegisterItem(BG_FindItemForWeapon(j));
					}
					j++;
				}
				j = 0;
				while (j < HI_NUM_HOLDABLE)
				{
					if (scl->invenItems & (1 << j))
					{ //we use this item so register it.
						RegisterItem(BG_FindItemForHoldable(j));
					}
					j++;
				}
			}
			i++;
		}
	}
}

//tell clients that this team won and print it on their scoreboard for intermission
//or whatever.
void SiegeSetCompleteData(int team)
{
	trap_SetConfigstring(CS_SIEGE_WINTEAM, va("%i", team));
}

void InitSiegeMode(void)
{
    char levelname[512];
    char teamIcon[128];
    char goalreq[64];

				   
    char* teams = (char*)BG_TempAlloc(2048);  // Using BG_TempAlloc instead of BG_Alloc
    char* objective = (char*)BG_TempAlloc(MAX_SIEGE_INFO_SIZE);  // Using BG_TempAlloc
    char* objecStr = (char*)BG_TempAlloc(8192);  // Using BG_TempAlloc

    int x = 0;
    int y = 0;
    char dependsOn[64];
    int len = 0;
    int i = 0;
    int objectiveNumTeam1 = 0;
    int objectiveNumTeam2 = 0;
    fileHandle_t f;

if (!teams || !objective || !objecStr) {
    // Free anything that did succeed to avoid leaking temp memory
    if (teams)     BG_TempFree(2048);
    if (objective) BG_TempFree(MAX_SIEGE_INFO_SIZE);
    if (objecStr)  BG_TempFree(8192);

    G_Error("InitSiegeMode: Memory allocation failed");
    return;
}

// Optional: actually clear the buffers if you want them zeroed
memset(teams, 0, 2048);
memset(objective, 0, MAX_SIEGE_INFO_SIZE);
memset(objecStr, 0, 8192);


    if (g_gametype.integer != GT_SIEGE)
    {
        goto failure;
    }

    // Reset objective dependency data
    for (x = 0; x < MAX_OBJECTIVES; x++)
    {
        for (y = 0; y < MAX_OBJECTIVEDEPENDANCY; y++)
        {
            ObjectiveDependancy[x][y] = 0;  // Zero out each entry
        }
    }

    // Reset
    SiegeSetCompleteData(0);

    // Get persistent data if it existed from last level
    if (g_siegeTeamSwitch.integer)
    {
        trap_SiegePersGet(&g_siegePersistant);
        if (g_siegePersistant.beatingTime)
        {
            trap_SetConfigstring(CS_SIEGE_TIMEOVERRIDE, va("%i", g_siegePersistant.lastTime));
        }
        else
        {
            trap_SetConfigstring(CS_SIEGE_TIMEOVERRIDE, "0");
        }
    }
    else
    {
        trap_SetConfigstring(CS_SIEGE_TIMEOVERRIDE, "0");
    }

    imperial_goals_completed = 0;
    rebel_goals_completed = 0;

    Com_sprintf(levelname, sizeof(levelname), "maps/%s.siege", level.rawmapname);

    if (!levelname[0])
    {
        goto failure;
    }

    len = trap_FS_FOpenFile(levelname, &f, FS_READ);

    if (!f || len >= MAX_SIEGE_INFO_SIZE)
    {
				
        trap_FS_FCloseFile(f);
		 
        goto failure;
    }

    trap_FS_Read(siege_info, len, f);

    trap_FS_FCloseFile(f);

    siege_valid = 1;

    // See if players should be specs or ingame preround
    if (BG_SiegeGetPairedValue(siege_info, "preround_state", teams))
    {
        if (teams[0])
        {
            g_preroundState = atoi(teams);
        }
    }

    if (BG_SiegeGetValueGroup(siege_info, "Teams", teams))
    {
        char defaultTeam1[512];
        char defaultTeam2[512];

        defaultTeam1[0] = '\0';
        defaultTeam2[0] = '\0';
        BG_SiegeGetPairedValue(teams, "team1", defaultTeam1);
        BG_SiegeGetPairedValue(teams, "team2", defaultTeam2);

        if (g_siegeTeam1.string[0] && Q_stricmp(g_siegeTeam1.string, "none"))
        {
            Q_strncpyz(team1, g_siegeTeam1.string, sizeof(team1));
        }
        else
        {
            Q_strncpyz(team1, defaultTeam1, sizeof(team1));
        }

        /* OBP: g_siegeTeam1/g_siegeTeam2 can persist from another Siege map.
         * If the requested team block does not exist in this map's .siege
         * file, fall back to the default block from the Teams group. */
        if (!BG_SiegeGetValueGroup(siege_info, team1, gParseObjectives) && defaultTeam1[0])
        {
            Q_strncpyz(team1, defaultTeam1, sizeof(team1));
        }

        if (g_siegeTeam2.string[0] && Q_stricmp(g_siegeTeam2.string, "none"))
        {
            Q_strncpyz(team2, g_siegeTeam2.string, sizeof(team2));
        }
        else
        {
            Q_strncpyz(team2, defaultTeam2, sizeof(team2));
        }

        if (!BG_SiegeGetValueGroup(siege_info, team2, gParseObjectives) && defaultTeam2[0])
        {
            Q_strncpyz(team2, defaultTeam2, sizeof(team2));
        }
    }
    else
    {
        G_Error("Siege teams not defined");
    }

    if (BG_SiegeGetValueGroup(siege_info, team2, gParseObjectives))
    {
        if (BG_SiegeGetPairedValue(gParseObjectives, "TeamIcon", teamIcon))
        {
            trap_Cvar_Set("team2_icon", teamIcon);
        }

        if (BG_SiegeGetPairedValue(gParseObjectives, "RequiredObjectives", goalreq))
        {
            rebel_goals_required = atoi(goalreq);
        }
        if (BG_SiegeGetPairedValue(gParseObjectives, "Timed", goalreq))
        {
            rebel_time_limit = atoi(goalreq) * 1000;
            if (g_siegeTeamSwitch.integer &&
                g_siegePersistant.beatingTime)
            {
                gRebelCountdown = level.time + g_siegePersistant.lastTime;
            }
            else
            {
                gRebelCountdown = level.time + rebel_time_limit;
            }
        }
        if (BG_SiegeGetPairedValue(gParseObjectives, "attackers", goalreq))
        {
            rebel_attackers = atoi(goalreq);
        }
    }

    if (BG_SiegeGetValueGroup(siege_info, team1, gParseObjectives))
    {
        if (BG_SiegeGetPairedValue(gParseObjectives, "TeamIcon", teamIcon))
        {
            trap_Cvar_Set("team1_icon", teamIcon);
        }

        if (BG_SiegeGetPairedValue(gParseObjectives, "RequiredObjectives", goalreq))
        {
            imperial_goals_required = atoi(goalreq);
        }
        if (BG_SiegeGetPairedValue(gParseObjectives, "Timed", goalreq))
        {
            if (rebel_time_limit)
            {
                Com_Printf("Tried to set imperial time limit, but there's already a rebel time limit!\nOnly one team can have a time limit.\n");
            }
            else
            {
                imperial_time_limit = atoi(goalreq) * 1000;
                if (g_siegeTeamSwitch.integer &&
                    g_siegePersistant.beatingTime)
                {
                    gImperialCountdown = level.time + g_siegePersistant.lastTime;
                }
                else
                {
                    gImperialCountdown = level.time + imperial_time_limit;
                }
            }
        }
        if (BG_SiegeGetPairedValue(gParseObjectives, "attackers", goalreq))
        {
            imperial_attackers = atoi(goalreq);
        }
    }

    // Load the player class types
    BG_SiegeLoadClasses(NULL);

    if (!bgNumSiegeClasses)
    {
        G_Error("Couldn't find any player classes for Siege");
    }

    // Now load the teams since we have class data.
    BG_SiegeLoadTeams();

    if (!bgNumSiegeTeams)
    {
        G_Error("Couldn't find any player teams for Siege");
    }

    // Get and set the team themes for each team. This will control which classes can be used on each team.
    if (BG_SiegeGetValueGroup(siege_info, team1, gParseObjectives))
    {
        if (BG_SiegeGetPairedValue(gParseObjectives, "UseTeam", goalreq))
        {
            BG_SiegeSetTeamTheme(SIEGETEAM_TEAM1, goalreq);
        }

        // Count objectives for this team
        i = 1;
        Com_sprintf( objecStr, sizeof( objecStr ), "Objective%i", i );
        while (BG_SiegeGetValueGroup(gParseObjectives, objecStr, objective))
        {
            // Check for objective dependency
            x = 0;
            Q_strncpyz( dependsOn, "DependsOn1", sizeof(dependsOn) );            while (x < MAX_OBJECTIVEDEPENDANCY && imperial_attackers)
            {
                if (BG_SiegeGetPairedValue(objective, dependsOn, goalreq))
                {
                    ObjectiveDependancy[i - 1][x] = atoi(goalreq);
                }
                x++;
                Q_strncpyz( dependsOn, va("DependsOn%i", x + 1), sizeof(dependsOn) );            }

            objectiveNumTeam1++;
            i++;
            Com_sprintf( objecStr, sizeof( objecStr ), "Objective%i", i );
        }
    }

												 
    if (BG_SiegeGetValueGroup(siege_info, team2, gParseObjectives))
    {
        if (BG_SiegeGetPairedValue(gParseObjectives, "UseTeam", goalreq))
        {
            BG_SiegeSetTeamTheme(SIEGETEAM_TEAM2, goalreq);
        }

        // Count objectives for this team
        i = 1;
        Com_sprintf( objecStr, sizeof( objecStr ), "Objective%i", i );
        while (BG_SiegeGetValueGroup(gParseObjectives, objecStr, objective))
        {
            // Check for objective dependency
            x = 0;
            Q_strncpyz( dependsOn, "DependsOn1", sizeof(dependsOn) );            while (x < MAX_OBJECTIVEDEPENDANCY && rebel_attackers)
            {
                if (BG_SiegeGetPairedValue(objective, dependsOn, goalreq))
                {
                    ObjectiveDependancy[i - 1][x] = atoi(goalreq);
                }
                x++;
                Q_strncpyz( dependsOn, va("DependsOn%i", x + 1), sizeof(dependsOn) );            }

            objectiveNumTeam2++;
            i++;
            Com_sprintf( objecStr, sizeof( objecStr ), "Objective%i", i );
        }
    }

    // Set configstring to show status of all current objectives
    Q_strncpyz( gObjectiveCfgStr, "t1", sizeof(gObjectiveCfgStr) );    while (objectiveNumTeam1 > 0)
    {
        Q_strcat(gObjectiveCfgStr, 1024, "-0");
        objectiveNumTeam1--;
    }
    Q_strcat(gObjectiveCfgStr, 1024, "|t2");
    while (objectiveNumTeam2 > 0)
    {
        Q_strcat(gObjectiveCfgStr, 1024, "-0");
        objectiveNumTeam2--;
    }

    // Set the actual config string
    trap_SetConfigstring(CS_SIEGE_OBJECTIVES, gObjectiveCfgStr);

    gSiegeLastObjectiveHintCfg[0] = '\0';
    gSiegeNextObjectiveHintTime = level.time + 7000;

    // Precache saber data for classes that use sabers on both teams
    BG_PrecacheSabersForSiegeTeam(SIEGETEAM_TEAM1);
    BG_PrecacheSabersForSiegeTeam(SIEGETEAM_TEAM2);

    G_SiegeRegisterWeaponsAndHoldables(SIEGETEAM_TEAM1);
    G_SiegeRegisterWeaponsAndHoldables(SIEGETEAM_TEAM2);

    // Free allocated memory using BG_TempFree
    BG_TempFree(2048);
    BG_TempFree(MAX_SIEGE_INFO_SIZE);
    BG_TempFree(8192);

    return;

failure:
    siege_valid = 0;
    // BG_TempAlloc does not need free in this case, assuming the memory management is handled elsewhere
    if (teams)     BG_TempFree(2048);
    if (objective) BG_TempFree(MAX_SIEGE_INFO_SIZE);
    if (objecStr)  BG_TempFree(8192);										  
										 
}




void G_SiegeSetObjectiveComplete(int team, int objective, qboolean failIt)
{
	char *p = NULL;
	int onObjective = 0;

	if (team == SIEGETEAM_TEAM1)
	{
		p = strstr(gObjectiveCfgStr, "t1");
	}
	else if (team == SIEGETEAM_TEAM2)
	{
		p = strstr(gObjectiveCfgStr, "t2");
	}

	if (!p)
	{
		assert(0);
		return;
	}

	//Parse from the beginning of this team's objectives until we get to the desired objective
	//number.
	while (p && *p && *p != '|')
	{
		if (*p == '-')
		{
			onObjective++;
		}

		if (onObjective == objective)
		{ //this is the one we want
			//Move to the next char, the status of this objective
			p++;

			//Now change it from '0' to '1' if we are completeing the objective
			//or vice versa if the objective has been taken away
			if (failIt)
			{
				*p = '0';
			}
			else
			{
				*p = '1';
			}
			break;
		}

		p++;
	}

	//Now re-update the configstring.
	trap_SetConfigstring(CS_SIEGE_OBJECTIVES, gObjectiveCfgStr);
}

//Returns qtrue if objective complete currently, otherwise qfalse
qboolean G_SiegeGetCompletionStatus(int team, int objective)
{
	char *p = NULL;
	int onObjective = 0;

	if (team == SIEGETEAM_TEAM1)
	{
		p = strstr(gObjectiveCfgStr, "t1");
	}
	else if (team == SIEGETEAM_TEAM2)
	{
		p = strstr(gObjectiveCfgStr, "t2");
	}

	if (!p)
	{
		assert(0);
		return qfalse;
	}

	//Parse from the beginning of this team's objectives until we get to the desired objective
	//number.
	while (p && *p && *p != '|')
	{
		if (*p == '-')
		{
			onObjective++;
		}

		if (onObjective == objective)
		{ //this is the one we want
			//Move to the next char, the status of this objective
			p++;

			//return qtrue if it's '1', qfalse if it's anything else
			if (*p == '1')
			{
				return qtrue;
			}
			else
			{
				return qfalse;
			}
			break;
		}

		p++;
	}

	return qfalse;
}


static qboolean G_SiegeObjectiveDependenciesMet(int team, int objective)
{
	int i;

	if (objective <= 0 || objective > MAX_OBJECTIVES)
	{
		return qfalse;
	}

	for (i = 0; i < MAX_OBJECTIVEDEPENDANCY; i++)
	{
		int dep = ObjectiveDependancy[objective - 1][i];

		if (!dep)
		{
			continue;
		}

		if (!G_SiegeGetCompletionStatus(team, dep))
		{
			return qfalse;
		}
	}

	return qtrue;
}

static qboolean G_SiegeObjectiveExists(int team, int objective)
{
	char *teamName;
	char objName[64];
	char *teamObjectives;
	char *objectiveData;
	qboolean result = qfalse;

	if (team == SIEGETEAM_TEAM1)
	{
		teamName = team1;
	}
	else if (team == SIEGETEAM_TEAM2)
	{
		teamName = team2;
	}
	else
	{
		return qfalse;
	}

	teamObjectives = (char *)BG_TempAlloc(MAX_SIEGE_INFO_SIZE);
	objectiveData = (char *)BG_TempAlloc(MAX_SIEGE_INFO_SIZE);
	if (!teamObjectives || !objectiveData)
	{
		if (teamObjectives)
		{
			BG_TempFree(MAX_SIEGE_INFO_SIZE);
		}
		if (objectiveData)
		{
			BG_TempFree(MAX_SIEGE_INFO_SIZE);
		}
		return qfalse;
	}

	Com_sprintf(objName, sizeof(objName), "Objective%i", objective);
	if (BG_SiegeGetValueGroup(siege_info, teamName, teamObjectives) &&
		BG_SiegeGetValueGroup(teamObjectives, objName, objectiveData))
	{
		result = qtrue;
	}

	BG_TempFree(MAX_SIEGE_INFO_SIZE);
	BG_TempFree(MAX_SIEGE_INFO_SIZE);
	return result;
}

static qboolean G_SiegeFindCurrentObjective(int team, int *objective)
{
	int i;

	if (!objective)
	{
		return qfalse;
	}

	for (i = 1; i <= MAX_OBJECTIVES; i++)
	{
		if (!G_SiegeObjectiveExists(team, i))
		{
			break;
		}

		if (!G_SiegeGetCompletionStatus(team, i) &&
			G_SiegeObjectiveDependenciesMet(team, i))
		{
			*objective = i;
			return qtrue;
		}
	}

	return qfalse;
}

static void G_SiegeSanitizeMessage(char *dst, int dstSize, const char *src)
{
	int i = 0;
	int o = 0;

	if (!dst || dstSize <= 0)
	{
		return;
	}

	dst[0] = '\0';

	if (!src)
	{
		return;
	}

	while (src[i] && o < dstSize - 1)
	{
		if (src[i] == '"')
		{
			dst[o++] = '\'';
		}
		else if (src[i] == '\n' || src[i] == '\r')
		{
			dst[o++] = ' ';
		}
		else
		{
			dst[o++] = src[i];
		}
		i++;
	}

	dst[o] = '\0';
}

static qboolean G_SiegeGetObjectiveDescription(int team, int objective, char *buffer, int bufferSize)
{
	char *teamName;
	char objName[64];
	char *teamObjectives;
	char *objectiveData;
	char raw[MAX_STRING_CHARS];
	qboolean result = qfalse;

	if (!buffer || bufferSize <= 0)
	{
		return qfalse;
	}

	buffer[0] = '\0';

	if (team == SIEGETEAM_TEAM1)
	{
		teamName = team1;
	}
	else if (team == SIEGETEAM_TEAM2)
	{
		teamName = team2;
	}
	else
	{
		return qfalse;
	}

	teamObjectives = (char *)BG_TempAlloc(MAX_SIEGE_INFO_SIZE);
	objectiveData = (char *)BG_TempAlloc(MAX_SIEGE_INFO_SIZE);
	if (!teamObjectives || !objectiveData)
	{
		if (teamObjectives)
		{
			BG_TempFree(MAX_SIEGE_INFO_SIZE);
		}
		if (objectiveData)
		{
			BG_TempFree(MAX_SIEGE_INFO_SIZE);
		}
		return qfalse;
	}

	Com_sprintf(objName, sizeof(objName), "Objective%i", objective);
	if (BG_SiegeGetValueGroup(siege_info, teamName, teamObjectives) &&
		BG_SiegeGetValueGroup(teamObjectives, objName, objectiveData))
	{
		if (BG_SiegeGetPairedValue(objectiveData, "objdesc", raw) ||
			BG_SiegeGetPairedValue(objectiveData, "briefing", raw))
		{
			G_SiegeSanitizeMessage(buffer, bufferSize, raw);
			result = qtrue;
		}
	}

	BG_TempFree(MAX_SIEGE_INFO_SIZE);
	BG_TempFree(MAX_SIEGE_INFO_SIZE);
	return result;
}

static const char *G_SiegeClassRoleName(const siegeClass_t *scl)
{
	if (!scl)
	{
		return "class";
	}

	switch (scl->playerClass)
	{
	case SPC_INFANTRY:
		return "infantry";
	case SPC_VANGUARD:
		return "vanguard";
	case SPC_SUPPORT:
		return "support";
	case SPC_JEDI:
		return "Jedi";
	case SPC_DEMOLITIONIST:
		return "demolition";
	case SPC_HEAVY_WEAPONS:
		return "heavy weapons";
	default:
		return "class";
	}
}

static const char *G_SiegeClassObjectiveAdvice(const siegeClass_t *scl, int team)
{
	if (!scl)
	{
		return "Choose a valid Siege class for the current objective.";
	}

	switch (scl->playerClass)
	{
	case SPC_SUPPORT:
		return "support/escort teammates.";
	case SPC_DEMOLITIONIST:
		return "handle demolition targets.";
	case SPC_HEAVY_WEAPONS:
		return "clear defenders.";
	case SPC_JEDI:
		return "protect objective players.";
	case SPC_VANGUARD:
		return "lead/escort the push.";
	case SPC_INFANTRY:
		return "fight near the objective.";
	default:
		break;
	}

	if (team == SIEGETEAM_TEAM1 || team == SIEGETEAM_TEAM2)
	{
		return "stay with your team.";
	}

	return "help your team.";
}

static void G_SiegeBuildClassObjectiveHint(gentity_t *ent, char *buffer, int bufferSize)
{
	siegeClass_t *scl = NULL;

	if (!buffer || bufferSize <= 0)
	{
		return;
	}

	buffer[0] = '\0';

	if (!g_siegeClassObjectiveHints.integer || !ent || !ent->client)
	{
		return;
	}

	if (ent->client->siegeClass >= 0 && ent->client->siegeClass < bgNumSiegeClasses)
	{
		scl = &bgSiegeClasses[ent->client->siegeClass];
	}
	else if (ent->client->sess.siegeClass[0] && Q_stricmp(ent->client->sess.siegeClass, "none"))
	{
		int classIndex = BG_SiegeFindClassIndexByName(ent->client->sess.siegeClass);

		if (classIndex >= 0 && classIndex < bgNumSiegeClasses)
		{
			scl = &bgSiegeClasses[classIndex];
		}
	}

	if (!scl)
	{
		return;
	}

	Com_sprintf(buffer, bufferSize, "\nClass: %s - %s",
		G_SiegeClassRoleName(scl),
		G_SiegeClassObjectiveAdvice(scl, ent->client->sess.sessionTeam));
}

void G_SiegeSendObjectiveHint(gentity_t *ent, qboolean force)
{
	int objective = 0;
	char objectiveText[MAX_STRING_CHARS];
	char objectiveShort[128];
	char hint[MAX_STRING_CHARS];
	char classHint[MAX_STRING_CHARS];
	int team;

	if (!g_siegeObjectiveHints.integer || g_gametype.integer != GT_SIEGE)
	{
		return;
	}

	if (!ent || !ent->inuse || !ent->client || (ent->r.svFlags & SVF_BOT))
	{
		return;
	}

	team = ent->client->sess.sessionTeam;
	if (team != SIEGETEAM_TEAM1 && team != SIEGETEAM_TEAM2)
	{
		return;
	}

	if (!force && g_siegeObjectiveHints.integer < 2)
	{
		return;
	}

	if (G_SiegeFindCurrentObjective(team, &objective))
	{
		if (!G_SiegeGetObjectiveDescription(team, objective, objectiveText, sizeof(objectiveText)) || !objectiveText[0])
		{
			Com_sprintf(objectiveText, sizeof(objectiveText), "Objective %i", objective);
		}
	}
	else
	{
		// Some Siege scripts do not expose a clean active objective through the
		// parsed objective dependency table at the exact moment the first player
		// spawns.  Still send a useful, visible hint instead of failing silently.
		Com_sprintf(objectiveText, sizeof(objectiveText),
			"Check briefing/objectives.");

		if (g_debugSiegeObjectives.integer)
		{
			G_Printf("Siege objective hint: no active objective found for %s on team %i; using fallback hint\n",
				ent->client->pers.netname, team);
		}
	}

	if (strlen(objectiveText) > 72)
	{
		Com_sprintf(objectiveShort, sizeof(objectiveShort), "%.72s...", objectiveText);
	}
	else
	{
		Q_strncpyz(objectiveShort, objectiveText, sizeof(objectiveShort));
	}

	G_SiegeBuildClassObjectiveHint(ent, classHint, sizeof(classHint));
	Com_sprintf(hint, sizeof(hint), "Siege objective: %s%s", objectiveShort, classHint);

	// Keep this short enough for centerprint.  Console/debug output is only
	// emitted when explicitly requested, to avoid spam.
	trap_SendServerCommand(ent - g_entities, va("cp \"%s\n\"", hint));
	if (g_debugSiegeObjectives.integer)
	{
		trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", hint));
	}
}

static void G_SiegeBroadcastObjectiveHints(qboolean force)
{
	int i;

	for (i = 0; i < level.maxclients; i++)
	{
		G_SiegeSendObjectiveHint(&g_entities[i], force);
	}
}

void G_SiegeObjectiveHintsThink(void)
{
	int interval;
	qboolean objectiveStateChanged = qfalse;

	if (!g_siegeObjectiveHints.integer || g_gametype.integer != GT_SIEGE)
	{
		return;
	}

	if (Q_stricmp(gSiegeLastObjectiveHintCfg, gObjectiveCfgStr))
	{
		objectiveStateChanged = qtrue;
		Q_strncpyz(gSiegeLastObjectiveHintCfg, gObjectiveCfgStr, sizeof(gSiegeLastObjectiveHintCfg));
	}

	interval = g_siegeObjectiveHintInterval.integer;
	if (interval < 10)
	{
		interval = 10;
	}
	else if (interval > 120)
	{
		interval = 120;
	}

	if (objectiveStateChanged)
	{
		if (g_debugSiegeObjectives.integer)
		{
			G_Printf("Siege objective state changed: %s\n", gObjectiveCfgStr);
		}

		G_SiegeBroadcastObjectiveHints(qtrue);
		gSiegeNextObjectiveHintTime = level.time + interval * 1000;
		return;
	}

	// g_siegeObjectiveHints 1 = spawn/objective-change hints only.
	// g_siegeObjectiveHints 2 = also repeat periodically.
	if (g_siegeObjectiveHints.integer >= 2 && level.time >= gSiegeNextObjectiveHintTime)
	{
		G_SiegeBroadcastObjectiveHints(qtrue);
		gSiegeNextObjectiveHintTime = level.time + interval * 1000;
	}
}

void UseSiegeTarget(gentity_t *other, gentity_t *en, char *target)
{ //actually use the player which triggered the object which triggered the siege objective to trigger the target
	gentity_t		*t;
	gentity_t		*ent;

	if ( !en || !en->client )
	{ //looks like we don't have access to a player, so just use the activating entity
		ent = other;
	}
	else
	{
		ent = en;
	}

	if (!en)
	{
		return;
	}

	if ( !target )
	{
		return;
	}

	t = NULL;
	while ( (t = G_Find (t, FOFS(targetname), target)) != NULL )
	{
		if ( t == ent )
		{
			G_Printf ("WARNING: Entity used itself.\n");
		}
		else
		{
			if ( t->use )
			{
				GlobalUse(t, ent, ent);
			}
		}
		if ( !ent->inuse )
		{
			G_Printf("entity was removed while using targets\n");
			return;
		}
	}
}

void SiegeBroadcast_OBJECTIVECOMPLETE(int team, int client, int objective)
{
	gentity_t *te;
	vec3_t nomatter;

	VectorClear(nomatter);

	te = G_TempEntity( nomatter, EV_SIEGE_OBJECTIVECOMPLETE );
	te->r.svFlags |= SVF_BROADCAST;
	te->s.eventParm = team;
	te->s.weapon = client;
	te->s.trickedentindex = objective;
}

void SiegeBroadcast_ROUNDOVER(int winningteam, int winningclient)
{
	gentity_t *te;
	vec3_t nomatter;

	VectorClear(nomatter);

	te = G_TempEntity( nomatter, EV_SIEGE_ROUNDOVER );
	te->r.svFlags |= SVF_BROADCAST;
	te->s.eventParm = winningteam;
	te->s.weapon = winningclient;
}

void BroadcastObjectiveCompletion(int team, int objective, int final, int client)
{
	if (client != ENTITYNUM_NONE && g_entities[client].client && g_entities[client].client->sess.sessionTeam == team)
	{ //guy who completed this objective gets points, providing he's on the opposing team
		AddScore(&g_entities[client], g_entities[client].client->ps.origin, SIEGE_POINTS_OBJECTIVECOMPLETED);
	}

	SiegeBroadcast_OBJECTIVECOMPLETE(team, client, objective);
	//G_Printf("Broadcast goal completion team %i objective %i final %i\n", team, objective, final);
}

void AddSiegeWinningTeamPoints(int team, int winner)
{
	int i = 0;
	gentity_t *ent;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->client && ent->client->sess.sessionTeam == team)
		{
			if (i == winner)
			{
				AddScore(ent, ent->client->ps.origin, SIEGE_POINTS_TEAMWONROUND+SIEGE_POINTS_FINALOBJECTIVECOMPLETED);
			}
			else
			{
				AddScore(ent, ent->client->ps.origin, SIEGE_POINTS_TEAMWONROUND);
			}
		}

		i++;
	}
}

void SiegeClearSwitchData(void)
{
	memset(&g_siegePersistant, 0, sizeof(g_siegePersistant));
	trap_SiegePersSet(&g_siegePersistant);
}

void SiegeDoTeamAssign(void)
{
	int i = 0;
	gentity_t *ent;

	//yeah, this is great...
	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent->inuse && ent->client &&
			ent->client->pers.connected == CON_CONNECTED)
		{ //a connected client, switch his frickin teams around
			if (ent->client->sess.siegeDesiredTeam == SIEGETEAM_TEAM1)
			{
				ent->client->sess.siegeDesiredTeam = SIEGETEAM_TEAM2;
			}
			else if (ent->client->sess.siegeDesiredTeam == SIEGETEAM_TEAM2)
			{
				ent->client->sess.siegeDesiredTeam = SIEGETEAM_TEAM1;
			}

			if (ent->client->sess.sessionTeam == SIEGETEAM_TEAM1)
			{
				SetTeamQuick(ent, SIEGETEAM_TEAM2, qfalse);
			}
			else if (ent->client->sess.sessionTeam == SIEGETEAM_TEAM2)
			{
				SetTeamQuick(ent, SIEGETEAM_TEAM1, qfalse);
			}
		}
		i++;
	}
}

void SiegeTeamSwitch(int winTeam, int winTime)
{
	trap_SiegePersGet(&g_siegePersistant);
	if (g_siegePersistant.beatingTime)
	{ //was already in "switched" mode, change back
		//announce the winning team.
		//either the first team won again, or the second
		//team beat the time set by the initial team. In any
		//case the winTeam here is the overall winning team.
		SiegeSetCompleteData(winTeam);
		SiegeClearSwitchData();
	}
	else
	{ //go into "beat their time" mode
		g_siegePersistant.beatingTime = qtrue;
        g_siegePersistant.lastTeam = winTeam;
		g_siegePersistant.lastTime = winTime;

		trap_SiegePersSet(&g_siegePersistant);
	}
}

void SiegeRoundComplete(int winningteam, int winningclient)
{
	vec3_t nomatter;
	char teamstr[1024];
	int originalWinningClient = winningclient;

	//G_Printf("Team %i won\n", winningteam);

	if (winningclient != ENTITYNUM_NONE && g_entities[winningclient].client &&
		g_entities[winningclient].client->sess.sessionTeam != winningteam)
	{ //this person just won the round for the other team..
		winningclient = ENTITYNUM_NONE;
	}

	VectorClear(nomatter);

	SiegeBroadcast_ROUNDOVER(winningteam, winningclient);

	AddSiegeWinningTeamPoints(winningteam, winningclient);

	//Instead of exiting like this, fire off a target, and let it handle things.
	//Can be a script or whatever the designer wants.
   	if (winningteam == SIEGETEAM_TEAM1)
	{
		Com_sprintf(teamstr, sizeof(teamstr), team1);
	}
	else
	{
		Com_sprintf(teamstr, sizeof(teamstr), team2);
	}

	trap_SetConfigstring(CS_SIEGE_STATE, va("3|%i", level.time)); //ended
	gSiegeRoundBegun = qfalse;
	gSiegeRoundEnded = qtrue;
	gSiegeRoundWinningTeam = winningteam;

	if (BG_SiegeGetValueGroup(siege_info, teamstr, gParseObjectives))
	{
		if (!BG_SiegeGetPairedValue(gParseObjectives, "roundover_target", teamstr))
		{ //didn't find the name of the thing to target upon win, just logexit now then.
			G_LogExit( "Objectives completed" );
			return;
		}
		
		if (originalWinningClient == ENTITYNUM_NONE)
		{ //oh well, just find something active and use it then.
            int i = 0;
			gentity_t *ent;

			while (i < MAX_CLIENTS)
			{
				ent = &g_entities[i];

				if (ent->inuse)
				{ //sure, you'll do.
                    originalWinningClient = ent->s.number;
					break;
				}

				i++;
			}
		}
		G_UseTargets2(&g_entities[originalWinningClient], &g_entities[originalWinningClient], teamstr);
	}

	if (g_siegeTeamSwitch.integer &&
		(imperial_time_limit || rebel_time_limit))
	{ //handle stupid team switching crap
		int time = 0;
		if (imperial_time_limit)
		{
			time = imperial_time_limit-(gImperialCountdown-level.time);
		}
		else if (rebel_time_limit)
		{
			time = rebel_time_limit-(gRebelCountdown-level.time);
		}

		if (time < 1)
		{
			time = 1;
		}
		SiegeTeamSwitch(winningteam, time);
	}
	else
	{ //assure it's clear for next round
		SiegeClearSwitchData();
	}
}

void G_ValidateSiegeClassForTeam(gentity_t *ent, int team)
{
	siegeClass_t *scl;
	siegeTeam_t *stm;
	int newClassIndex = -1;
	if (ent->client->siegeClass == -1)
	{ //uh.. sure.
		return;
	}

	scl = &bgSiegeClasses[ent->client->siegeClass];

	stm = BG_SiegeFindThemeForTeam(team);
	if (stm)
	{
		int i = 0;

		while (i < stm->numClasses)
		{ //go through the team and see its valid classes, can we find one that matches our current player class?
			if (stm->classes[i])
			{
				if (!Q_stricmp(scl->name, stm->classes[i]->name))
				{ //the class we're using is already ok for this team.
					return;
				}
				if (stm->classes[i]->playerClass == scl->playerClass ||
					newClassIndex == -1)
				{
					newClassIndex = i;
				}
			}
			i++;
		}

		if (newClassIndex != -1)
		{ //ok, let's find it in the global class array
			ent->client->siegeClass = BG_SiegeFindClassIndexByName(stm->classes[newClassIndex]->name);
			Q_strncpyz( ent->client->sess.siegeClass, stm->classes[newClassIndex]->name,
					 sizeof( ent->client->sess.siegeClass ) );
		}
	}
}

//bypass most of the normal checks in SetTeam
void SetTeamQuick(gentity_t *ent, int team, qboolean doBegin)
{
	char userinfo[MAX_INFO_STRING];

	trap_GetUserinfo( ent->s.number, userinfo, sizeof( userinfo ) );

	if (g_gametype.integer == GT_SIEGE)
	{
		G_ValidateSiegeClassForTeam(ent, team);

		// Starting the Siege round here makes the round active before the
		// successful team move/ClientBegin spawn path completes.  This avoids the
		// visible spawned-then-round-start delay when the first player joins an
		// otherwise empty Siege match.
		if ((team == TEAM_RED || team == TEAM_BLUE) &&
			!gSiegeRoundBegun && !gSiegeRoundEnded)
		{
			SiegeStartRoundNow(ent->s.number);
		}
	}

	ent->client->sess.sessionTeam = team;

	if (team == TEAM_SPECTATOR)
	{
		ent->client->sess.spectatorState = SPECTATOR_FREE;
		Info_SetValueForKey(userinfo, "team", "s");
	}
	else
	{
		ent->client->sess.spectatorState = SPECTATOR_NOT;
		if (team == TEAM_RED)
		{
			Info_SetValueForKey(userinfo, "team", "r");
		}
		else if (team == TEAM_BLUE)
		{
			Info_SetValueForKey(userinfo, "team", "b");
		}
		else
		{
			Info_SetValueForKey(userinfo, "team", "?");
		}
	}

	trap_SetUserinfo( ent->s.number, userinfo );

	ent->client->sess.spectatorClient = 0;

	ent->client->pers.teamState.state = TEAM_BEGIN;

	ClientUserinfoChanged( ent->s.number );

	if (doBegin)
	{
		ClientBegin( ent->s.number, qfalse );
	}
}

void SiegeRespawn(gentity_t *ent)
{
	gentity_t *tent;

	if (!ent || !ent->client)
	{
		return;
	}

	// If Siege is configured to allow a player to join before the other team
	// has members, begin the round immediately when the first real spawn is
	// requested.  Do this here instead of waiting for SiegeCheckTimers() so the
	// player does not enter a spawned-but-pre-round state for a frame or more.
	if (g_gametype.integer == GT_SIEGE &&
		ent && ent->client &&
		!gSiegeRoundBegun && !gSiegeRoundEnded &&
		(ent->client->sess.siegeDesiredTeam == TEAM_RED ||
		 ent->client->sess.siegeDesiredTeam == TEAM_BLUE))
	{
		SiegeStartRoundNow(ent->s.number);
	}

	if (ent->client->sess.sessionTeam != ent->client->sess.siegeDesiredTeam)
	{
		SetTeamQuick(ent, ent->client->sess.siegeDesiredTeam, qtrue);
	}
	else
	{
		ClientSpawn(ent);
		// add a teleportation effect
		tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_IN );
		tent->s.clientNum = ent->s.clientNum;
	}
}

static void SiegeInitRoundCountdowns(void)
{
	if (g_siegeTeamSwitch.integer &&
		g_siegePersistant.beatingTime)
	{
		gImperialCountdown = level.time + g_siegePersistant.lastTime;
		gRebelCountdown = level.time + g_siegePersistant.lastTime;
	}
	else
	{
		gImperialCountdown = level.time + imperial_time_limit;
		gRebelCountdown = level.time + rebel_time_limit;
	}
}

static void SiegeBeginRoundTasks(int entNum)
{
	char targname[1024];

	//Now check if there's something to fire off at the round start, if so do it.
	if (BG_SiegeGetPairedValue(siege_info, "roundbegin_target", targname))
	{
		if (targname[0])
		{
			G_UseTargets2(&g_entities[entNum], &g_entities[entNum], targname);
		}
	}

	trap_SetConfigstring(CS_SIEGE_STATE, va("0|%i", level.time)); //we're ready to g0g0g0
}

void SiegeStartRoundNow(int entNum)
{
	if (g_gametype.integer != GT_SIEGE ||
		gSiegeRoundBegun ||
		gSiegeRoundEnded)
	{
		return;
	}

	gSiegeRoundBegun = qtrue;
	gSiegeBeginTime = level.time;
	SiegeInitRoundCountdowns();
	SiegeBeginRoundTasks(entNum);
}

void SiegeBeginRound(int entNum)
{ //entNum is just used as something to fire targets from.
	if (!g_preroundState)
	{ //if players are not ingame on round start then respawn them now
		int i = 0;
		gentity_t *ent;
		qboolean spawnEnt = qfalse;

		//respawn everyone now
		while (i < MAX_CLIENTS)
		{
			ent = &g_entities[i];

			if (ent->inuse && ent->client)
			{
				if (ent->client->sess.sessionTeam != TEAM_SPECTATOR &&
					!(ent->client->ps.pm_flags & PMF_FOLLOW))
				{ //not a spec, just respawn them
					spawnEnt = qtrue;
				}
				else if (ent->client->sess.sessionTeam == TEAM_SPECTATOR &&
					(ent->client->sess.siegeDesiredTeam == TEAM_RED ||
					 ent->client->sess.siegeDesiredTeam == TEAM_BLUE))
				{ //spectator but has a desired team
					spawnEnt = qtrue;
				}
			}

			if (spawnEnt)
			{
				SiegeRespawn(ent);

				spawnEnt = qfalse;
			}
			i++;
		}
	}

	SiegeBeginRoundTasks(entNum);
}

void SiegeCheckTimers(void)
{
	int i=0;
	gentity_t *ent;
	int numTeam1 = 0;
	int numTeam2 = 0;

	if (g_gametype.integer != GT_SIEGE)
	{
		return;
	}

	if (level.intermissiontime)
	{
		return;
	}

	if (gSiegeRoundEnded)
	{
		return;
	}

	if (!gSiegeRoundBegun)
	{ //check if anyone is active on this team - if not, keep the timer set up.
		i = 0;

		while (i < MAX_CLIENTS)
		{
			ent = &g_entities[i];

			if (ent && ent->inuse && ent->client &&
				ent->client->pers.connected == CON_CONNECTED &&
				ent->client->sess.siegeDesiredTeam == SIEGETEAM_TEAM1)
			{
				numTeam1++;
			}
			i++;
		}

		i = 0;

		while (i < MAX_CLIENTS)
		{
			ent = &g_entities[i];

			if (ent && ent->inuse && ent->client &&
				ent->client->pers.connected == CON_CONNECTED &&
				ent->client->sess.siegeDesiredTeam == SIEGETEAM_TEAM2)
			{
				numTeam2++;
			}
			i++;
		}

		SiegeInitRoundCountdowns();
	}

	if (imperial_time_limit)
	{ //team1
		if (gImperialCountdown < level.time)
		{
			SiegeRoundComplete(SIEGETEAM_TEAM2, ENTITYNUM_NONE);
			imperial_time_limit = 0;
			return;
		}
	}

	if (rebel_time_limit)
	{ //team2
		if (gRebelCountdown < level.time)
		{
			SiegeRoundComplete(SIEGETEAM_TEAM1, ENTITYNUM_NONE);
			rebel_time_limit = 0;
			return;
		}
	}

	if (!gSiegeRoundBegun)
	{
		if (!numTeam1 && !numTeam2)
		{ //don't have any joined players yet.
			gSiegeBeginTime = level.time + SIEGE_ROUND_BEGIN_TIME;
			trap_SetConfigstring(CS_SIEGE_STATE, "1"); //"waiting for players"
		}
		else if (gSiegeBeginTime < level.time)
		{ //mark the round as having begun
			gSiegeRoundBegun = qtrue;
			SiegeBeginRound(i); //perform any round start tasks
		}
		else if (gSiegeBeginTime > (level.time + SIEGE_ROUND_BEGIN_TIME))
		{
			gSiegeBeginTime = level.time + SIEGE_ROUND_BEGIN_TIME;
		}
		else
		{
			trap_SetConfigstring(CS_SIEGE_STATE, va("2|%i", gSiegeBeginTime - SIEGE_ROUND_BEGIN_TIME)); //getting ready to begin
		}
	}
}

void SiegeObjectiveCompleted(int team, int objective, int final, int client)
{
	int goals_completed, goals_required;

	if (gSiegeRoundEnded)
	{
		return;
	}

	//Update the configstring status
	G_SiegeSetObjectiveComplete(team, objective, qfalse);
	//[TABBots]
	//Completing an objective for one side completes the objective for both sides so the objective valid stuff works right. 
	//(which scans to see if the objective is finished for both sides.)
	if (team == SIEGETEAM_TEAM1)
	{
		G_SiegeSetObjectiveComplete(SIEGETEAM_TEAM2, objective, qfalse);
	}
	else
	{
		G_SiegeSetObjectiveComplete(SIEGETEAM_TEAM1, objective, qfalse);
	}
	//[/TABBots]


	if (final != -1)
	{
		if (team == SIEGETEAM_TEAM1)
		{
			imperial_goals_completed++;
		}
		else
		{
			rebel_goals_completed++;
		}
	}

	if (team == SIEGETEAM_TEAM1)
	{
		goals_completed = imperial_goals_completed;
		goals_required = imperial_goals_required;
	}
	else
	{
		goals_completed = rebel_goals_completed;
		goals_required = rebel_goals_required;
	}

	if (final == 1 || goals_completed >= goals_required)
	{
		SiegeRoundComplete(team, client);
	}
	else
	{
		BroadcastObjectiveCompletion(team, objective, final, client);
	}
}

void siegeTriggerUse(gentity_t* ent, gentity_t* other, gentity_t* activator)
{
	char            teamstr[64];
	char            objectivestr[64];
	char* desiredobjective;  // Changed from stack to pointer
	int             clUser = ENTITYNUM_NONE;
	int             final = 0;
	int             i = 0;
	int             size;

	if (!siege_valid)
	{
		return;
	}

	// Allocate large buffer on heap using BG_TempAlloc
	desiredobjective = (char*)BG_TempAlloc(MAX_SIEGE_INFO_SIZE);
	if (!desiredobjective) {
		// Allocation failed, just return safely
		return;
	}

	if (!(ent->s.eFlags & EF_RADAROBJECT))
	{ // toggle radar on and exit if it is not showing up already
		ent->s.eFlags |= EF_RADAROBJECT;
		// No free required, just clear the allocated memory
		size = MAX_SIEGE_INFO_SIZE;  // Pass the size for BG_TempFree
		BG_TempFree(size);  // Clear memory after usage
		return;
	}

	if (activator && activator->client)
	{ // activator will hopefully be the person who triggered this event
		clUser = activator->s.number;
	}

	if (ent->side == SIEGETEAM_TEAM1)
	{
		Com_sprintf(teamstr, sizeof(teamstr), team1);
	}
	else
	{
		Com_sprintf(teamstr, sizeof(teamstr), team2);
	}

	if (BG_SiegeGetValueGroup(siege_info, teamstr, gParseObjectives))
	{
		Com_sprintf(objectivestr, sizeof(objectivestr), "Objective%i", ent->objective);

		if (BG_SiegeGetValueGroup(gParseObjectives, objectivestr, desiredobjective))
		{
			if (BG_SiegeGetPairedValue(desiredobjective, "final", teamstr))
			{
				final = atoi(teamstr);
			}

			if (BG_SiegeGetPairedValue(desiredobjective, "target", teamstr))
			{
				while (teamstr[i])
				{
					if (teamstr[i] == '\r' || teamstr[i] == '\n')
					{
						teamstr[i] = '\0';
					 
						
					}
														  
			 

					i++;
				}
				UseSiegeTarget(other, activator, teamstr);
			}

			if (ent->target && ent->target[0])
			{ // use this too
				UseSiegeTarget(other, activator, ent->target);
			}

			SiegeObjectiveCompleted(ent->side, ent->objective, final, clUser);
		}
	}

	// Free memory after use
	size = MAX_SIEGE_INFO_SIZE;  // Pass the size for BG_TempFree
	BG_TempFree(size);  // Clear memory after usage
}






/*QUAKED info_siege_objective (1 0 1) (-16 -16 -24) (16 16 32) ? x x STARTOFFRADAR
STARTOFFRADAR - start not displaying on radar, don't display until used.

"objective" - specifies the objective to complete upon activation
"side" - set to 1 to specify an imperial goal, 2 to specify rebels
"icon" - icon that represents the objective on the radar
*/
void SP_info_siege_objective (gentity_t *ent)
{
	char* s;

	if (!siege_valid || g_gametype.integer != GT_SIEGE)
	{
		G_FreeEntity(ent);
		return;
	}

	ent->use = siegeTriggerUse;
	G_SpawnInt( "objective", "0", &ent->objective);
	G_SpawnInt( "side", "0", &ent->side);

	if (!ent->objective || !ent->side)
	{ //j00 fux0red something up
		G_FreeEntity(ent);
		G_Printf("ERROR: info_siege_objective without an objective or side value\n");
		return;
	}

	//Set it up to be drawn on radar
	if (!(ent->spawnflags & SIEGEITEM_STARTOFFRADAR))
	{
		ent->s.eFlags |= EF_RADAROBJECT;
	}

	//All clients want to know where it is at all times for radar
	ent->r.svFlags |= SVF_BROADCAST;

	G_SpawnString( "icon", "", &s );
	
	if (s && s[0])
	{ 
		// We have an icon, so index it now.  We are reusing the genericenemyindex
		// variable rather than adding a new one to the entity state.
		ent->s.genericenemyindex = G_IconIndex(s);
	}

	ent->s.brokenLimbs = ent->side;
	ent->s.frame = ent->objective;
	trap_LinkEntity(ent);
}


void SiegeIconUse(gentity_t *ent, gentity_t *other, gentity_t *activator)
{ //toggle it on and off
	if (ent->s.eFlags & EF_RADAROBJECT)
	{
		ent->s.eFlags &= ~EF_RADAROBJECT;
		ent->r.svFlags &= ~SVF_BROADCAST;
	}
	else
	{
		ent->s.eFlags |= EF_RADAROBJECT;
		ent->r.svFlags |= SVF_BROADCAST;
	}
}

/*QUAKED info_siege_radaricon (1 0 1) (-16 -16 -24) (16 16 32) ? 
Used to arbitrarily display radar icons at placed location. Can be used
to toggle on and off.

"icon" - icon that represents the objective on the radar
"startoff" - if 1 start off
*/
void SP_info_siege_radaricon (gentity_t *ent)
{
	char* s;
	int i;

	if (!siege_valid || g_gametype.integer != GT_SIEGE)
	{
		G_FreeEntity(ent);
		return;
	}

	G_SpawnInt("startoff", "0", &i);

	if (!i)
	{ //start on then
		ent->s.eFlags |= EF_RADAROBJECT;
		ent->r.svFlags |= SVF_BROADCAST;
	}

	G_SpawnString( "icon", "", &s );
	if (!s || !s[0])
	{ //that's the whole point of the entity
        Com_Error(ERR_DROP, "misc_siege_radaricon without an icon");
		return;
	}

	ent->use = SiegeIconUse;

	ent->s.genericenemyindex = G_IconIndex(s);

	trap_LinkEntity(ent);
}

void decompTriggerUse(gentity_t* ent, gentity_t* other, gentity_t* activator)
{
    int final = 0;
    char teamstr[1024];
    char objectivestr[64];
    char* desiredobjective = (char*)BG_TempAlloc(MAX_SIEGE_INFO_SIZE);  // Replaced BG_Alloc with BG_TempAlloc

    if (!desiredobjective) {
        // Handle allocation failure gracefully
        return;
    }

    if (gSiegeRoundEnded)
    {
        BG_TempFree(MAX_SIEGE_INFO_SIZE);  // Free the memory
        return;
    }

    if (!G_SiegeGetCompletionStatus(ent->side, ent->objective))
    {
        BG_TempFree(MAX_SIEGE_INFO_SIZE);  // Free the memory
        return;
    }

    G_SiegeSetObjectiveComplete(ent->side, ent->objective, qtrue);

    //[TABBots]
    if (ent->side == SIEGETEAM_TEAM1)
    {
        G_SiegeSetObjectiveComplete(SIEGETEAM_TEAM2, ent->objective, qtrue);
    }
    else
    {
        G_SiegeSetObjectiveComplete(SIEGETEAM_TEAM1, ent->objective, qtrue);
    }
    //[/TABBots]

    if (ent->side == SIEGETEAM_TEAM1)
    {
        Com_sprintf(teamstr, sizeof(teamstr), team1);
    }
    else
    {
        Com_sprintf(teamstr, sizeof(teamstr), team2);
    }

    if (BG_SiegeGetValueGroup(siege_info, teamstr, gParseObjectives))
    {
        Com_sprintf(objectivestr, sizeof(objectivestr), "Objective%i", ent->objective);

        if (BG_SiegeGetValueGroup(gParseObjectives, objectivestr, desiredobjective))
        {
            if (BG_SiegeGetPairedValue(desiredobjective, "final", teamstr))
            {
                final = atoi(teamstr);
            }
        }
    }

    if (final != -1)
    {
        if (ent->side == SIEGETEAM_TEAM1)
        {
            imperial_goals_completed--;
        }
        else
        {
            rebel_goals_completed--;
        }
    }

    // Free the memory after use
    BG_TempFree(MAX_SIEGE_INFO_SIZE);  // Free the memory
}




/*QUAKED info_siege_decomplete (1 0 1) (-16 -16 -24) (16 16 32)
"objective" - specifies the objective to decomplete upon activation
"side" - set to 1 to specify an imperial (team1) goal, 2 to specify rebels (team2)
*/
void SP_info_siege_decomplete (gentity_t *ent)
{
	if (!siege_valid || g_gametype.integer != GT_SIEGE)
	{
		G_FreeEntity(ent);
		return;
	}

	ent->use = decompTriggerUse;
	G_SpawnInt( "objective", "0", &ent->objective);
	G_SpawnInt( "side", "0", &ent->side);

	if (!ent->objective || !ent->side)
	{ //j00 fux0red something up
		G_FreeEntity(ent);
		G_Printf("ERROR: info_siege_objective_decomplete without an objective or side value\n");
		return;
	}
}

void siegeEndUse(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	G_LogExit("Round ended");
}

/*QUAKED target_siege_end (1 0 1) (-16 -16 -24) (16 16 32)
Do a logexit for siege when used.
*/
void SP_target_siege_end (gentity_t *ent)
{
	if (!siege_valid || g_gametype.integer != GT_SIEGE)
	{
		G_FreeEntity(ent);
		return;
	}

	ent->use = siegeEndUse;
}

#define SIEGE_ITEM_RESPAWN_TIME 20000

void SiegeItemRemoveOwner(gentity_t *ent, gentity_t *carrier)
{
	ent->genericValue2 = 0; //Remove picked-up flag

	ent->genericValue8 = ENTITYNUM_NONE; //Mark entity carrying us as none

	if (carrier)
	{
		carrier->client->holdingObjectiveItem = 0; //The carrier is no longer carrying us
		carrier->r.svFlags &= ~SVF_BROADCAST;
	}
}

static void SiegeItemRespawnEffect(gentity_t *ent, vec3_t newOrg)
{
	vec3_t upAng;

	if (ent->target5 && ent->target5[0])
	{
		G_UseTargets2(ent, ent, ent->target5);
	}

	if (!ent->genericValue10)
	{ //no respawn effect
		return;
	}

	VectorSet(upAng, 0, 0, 1);

	//Play it once on the current origin, and once on the origin we're respawning to.
	G_PlayEffectID(ent->genericValue10, ent->r.currentOrigin, upAng);
	G_PlayEffectID(ent->genericValue10, newOrg, upAng);
}

static void SiegeItemRespawnOnOriginalSpot(gentity_t *ent, gentity_t *carrier)
{
	SiegeItemRespawnEffect(ent, ent->pos1);
	G_SetOrigin(ent, ent->pos1);
	SiegeItemRemoveOwner(ent, carrier);
	
	// Stop the item from flashing on the radar
	ent->s.time2 = 0;
}

void SiegeItemThink(gentity_t *ent)
{
	gentity_t *carrier = NULL;

	if (ent->genericValue12)
	{ //recharge health
		if (ent->health > 0 && ent->health < ent->maxHealth && ent->genericValue14 < level.time)
		{
            ent->genericValue14 = level.time + ent->genericValue13;
			ent->health += ent->genericValue12;
			if (ent->health > ent->maxHealth)
			{
				ent->health = ent->maxHealth;
			}
		}
	}

	if (ent->genericValue8 != ENTITYNUM_NONE)
	{ //Just keep sticking it on top of the owner. We need it in the same PVS as him so it will render bolted onto him properly.
		carrier = &g_entities[ent->genericValue8];

		if (carrier->inuse && carrier->client)
		{
			VectorCopy(carrier->client->ps.origin, ent->r.currentOrigin);
			trap_LinkEntity(ent);
		}
	}
	else if (ent->genericValue1)
	{ //this means we want to run physics on the object
		G_RunExPhys(ent, ent->radius, ent->mass, ent->random, qfalse, NULL, 0);
	}

	//Bolt us to whoever is carrying us if a client
	if (ent->genericValue8 < MAX_CLIENTS)
	{
		ent->s.boltToPlayer = ent->genericValue8+1;
	}
	else
	{
		ent->s.boltToPlayer = 0;
	}


	if (carrier)
	{
		gentity_t *carrier = &g_entities[ent->genericValue8];

		//This checking can be a bit iffy on the death stuff, but in theory we should always
		//get a think in before the default minimum respawn time is exceeded.
		if (!carrier->inuse || !carrier->client ||
			(carrier->client->sess.sessionTeam != SIEGETEAM_TEAM1 && carrier->client->sess.sessionTeam != SIEGETEAM_TEAM2) ||
			(carrier->client->ps.pm_flags & PMF_FOLLOW))
		{ //respawn on the original spot
			SiegeItemRespawnOnOriginalSpot(ent, NULL);
		}
		else if (carrier->health < 1)
		{ //The carrier died so pop out where he is (unless in nodrop).
			if (ent->target6 && ent->target6[0])
			{
				G_UseTargets2(ent, ent, ent->target6);
			}

			if ( trap_PointContents(carrier->client->ps.origin, carrier->s.number) & CONTENTS_NODROP )
			{ //In nodrop land, go back to the original spot.
				SiegeItemRespawnOnOriginalSpot(ent, carrier);
			}
			
			else
			{
				//[BugFix7]
				//perform a startsolid check to make sure the seige item doesn't get stuck
				//in a wall or something
				trace_t tr;
				trap_Trace(&tr, carrier->client->ps.origin, ent->r.mins, ent->r.maxs, carrier->client->ps.origin, ent->s.number, ent->clipmask);

				if(tr.startsolid)
				{//bad spawning area, try again with the trace up a bit.
					vec3_t TracePoint;
					VectorCopy(carrier->client->ps.origin, TracePoint);
					TracePoint[2] += 30;
					trap_Trace(&tr, TracePoint, ent->r.mins, ent->r.maxs, TracePoint, ent->s.number, ent->clipmask);
					
					if(tr.startsolid)
					{//hmm, well that didn't work. try one last time with the item back 
						//away from where the dude was facing (in case the carrier was
						//close to something they were attacking.)
						vec3_t fwd;
						AngleVectors(carrier->client->ps.viewangles,fwd, NULL, NULL);
						VectorMA(TracePoint, -30, fwd, TracePoint);
						trap_Trace(&tr, TracePoint, ent->r.mins, ent->r.maxs, TracePoint, ent->s.number, ent->clipmask);
						
						if(tr.startsolid)
						{
							SiegeItemRespawnOnOriginalSpot(ent, carrier);
							return;
						}
					}

					G_SetOrigin(ent, TracePoint);
				}
				else
				{//we're good at the player's origin
					G_SetOrigin(ent, carrier->client->ps.origin);
				}
				

				//G_SetOrigin(ent, carrier->client->ps.origin);
				//[/BugFix7]
				ent->epVelocity[0] = Q_irand(-80, 80);
				ent->epVelocity[1] = Q_irand(-80, 80);
				ent->epVelocity[2] = Q_irand(40, 80);

				//We're in a nonstandard place, so if we go this long without being touched,
				//assume we may not be reachable and respawn on the original spot.
				ent->genericValue9 = level.time + SIEGE_ITEM_RESPAWN_TIME;

				SiegeItemRemoveOwner(ent, carrier);
			}
		}
	}

	if (ent->genericValue9 && ent->genericValue9 < level.time)
	{ //time to respawn on the original spot then
		SiegeItemRespawnEffect(ent, ent->pos1);
		G_SetOrigin(ent, ent->pos1);
		ent->genericValue9 = 0;
		
		// stop flashing on radar
		ent->s.time2 = 0;
	}

	ent->nextthink = level.time + FRAMETIME/2;
}

void SiegeItemTouch( gentity_t *self, gentity_t *other, trace_t *trace )
{
	if (!other || !other->inuse ||
		!other->client || other->s.eType == ET_NPC)
	{
		if (trace && trace->startsolid)
		{ //let me out! (ideally this should not happen, but such is life)
			vec3_t escapePos;
			VectorCopy(self->r.currentOrigin, escapePos);
			escapePos[2] += 1.0f;

			//I hope you weren't stuck in the ceiling.
			G_SetOrigin(self, escapePos);
		}
		return;
	}

	if (other->health < 1)
	{ //dead people can't pick us up.
		return;
	}

	if (other->client->holdingObjectiveItem)
	{ //this guy's already carrying a siege item
		return;
	}

	if ( other->client->ps.pm_type == PM_SPECTATOR )
	{//spectators don't pick stuff up
		return;
	}

	if (self->genericValue2)
	{ //Am I already picked up?
		return;
	}

	if (self->genericValue6 == other->client->sess.sessionTeam)
	{ //Set to not be touchable by players on this team.
		return;
	}

	if (!gSiegeRoundBegun)
	{ //can't pick it up if round hasn't started yet
		return;
	}

	if (self->noise_index)
	{ //play the pickup noise.
		G_Sound(other, CHAN_AUTO, self->noise_index);
	}

	self->genericValue2 = 1; //Mark it as picked up.

	other->client->holdingObjectiveItem = self->s.number;
	other->r.svFlags |= SVF_BROADCAST; //broadcast player while he carries this
	self->genericValue8 = other->s.number; //Keep the index so we know who is "carrying" us

	self->genericValue9 = 0; //So it doesn't think it has to respawn.

	if (self->target2 && self->target2[0] && (!self->genericValue4 || !self->genericValue5))
	{ //fire the target for pickup, if it's set to fire every time, or set to only fire the first time and the first time has not yet occured.
		G_UseTargets2(self, self, self->target2);
		self->genericValue5 = 1; //mark it as having been picked up
	}	
	
	// time2 set to -1 will blink the item on the radar indefinately
	self->s.time2 = 0xFFFFFFFF;
}

void SiegeItemPain(gentity_t *self, gentity_t *attacker, int damage)
{
	// Time 2 is used to pulse the radar icon to show its under attack
	self->s.time2 = level.time;
}

void SiegeItemDie( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath )
{
	self->takedamage = qfalse; //don't die more than once

	if (self->genericValue3)
	{ //An indexed effect to play on death
		vec3_t upAng;

		VectorSet(upAng, 0, 0, 1);
		G_PlayEffectID(self->genericValue3, self->r.currentOrigin, upAng);
	}

	self->neverFree = qfalse;
	self->think = G_FreeEntity;
	self->nextthink = level.time;

	//Fire off the death target if we've got one.
	if (self->target4 && self->target4[0])
	{
		G_UseTargets2(self, self, self->target4);
	}
}

void SiegeItemUse(gentity_t *ent, gentity_t *other, gentity_t *activator)
{ //once used, become active
	if (ent->spawnflags & SIEGEITEM_STARTOFFRADAR)
	{ //start showing on radar
		ent->s.eFlags |= EF_RADAROBJECT;

		if (!(ent->s.eFlags & EF_NODRAW))
		{ //we've nothing else to do here
			return;
		}
	}
	else
	{ //make sure it's showing up
		ent->s.eFlags |= EF_RADAROBJECT;
	}

	if (ent->genericValue11 || !ent->takedamage)
	{ //We want to be able to walk into it to pick it up then.
		ent->r.contents = CONTENTS_TRIGGER;
		ent->clipmask = CONTENTS_SOLID|CONTENTS_TERRAIN;
		if (ent->genericValue11)
		{
			ent->touch = SiegeItemTouch;
		}
	}
	else
	{ //Make it solid.
		ent->r.contents = MASK_PLAYERSOLID;
		ent->clipmask = MASK_PLAYERSOLID;
	}

	ent->think = SiegeItemThink;
	ent->nextthink = level.time + FRAMETIME/2;

	//take off nodraw
	ent->s.eFlags &= ~EF_NODRAW;

	if (ent->paintarget && ent->paintarget[0])
	{ //want to be on this guy's origin now then
		gentity_t *targ = G_Find (NULL, FOFS(targetname), ent->paintarget);
		
		if (targ && targ->inuse)
		{
			//[BugFix7]
			//perform a startsolid check to make sure the seige item doesn't get stuck
			//in a wall or something
			trace_t tr;
			vec3_t TracePoint;
			VectorCopy(targ->r.currentOrigin, TracePoint);
			trap_Trace(&tr, targ->r.currentOrigin, ent->r.mins, ent->r.maxs, 
				targ->r.currentOrigin, targ->s.number, ent->clipmask);

			if(tr.startsolid)
			{//bad spawning area, try again with the trace up a bit.
				TracePoint[2] += 30;
				trap_Trace(&tr, TracePoint, ent->r.mins, ent->r.maxs, TracePoint, 
					ent->s.number, ent->clipmask);
				
				if(tr.startsolid)
				{//hmm, well that didn't work. try one last time with the item back 
					//away from where the dude was facing (in case the carrier was
					//close to something they were attacking.)
					vec3_t fwd;
					if(targ->client)
					{
						AngleVectors(targ->client->ps.viewangles,fwd, NULL, NULL);
					}
					else
					{
						AngleVectors(targ->r.currentAngles,fwd, NULL, NULL);
					}
					VectorMA(TracePoint, -30, fwd, TracePoint);
					trap_Trace(&tr, TracePoint, ent->r.mins, ent->r.maxs, TracePoint, ent->s.number, ent->clipmask);
					
					if(tr.startsolid)
					{//crap, that's all we got.  just spawn at the defualt location.
						return;
					}
				}
			}
			G_SetOrigin(ent, TracePoint);
			//G_SetOrigin(ent, targ->r.currentOrigin);
			//[/BugFix7]
			trap_LinkEntity(ent);
		}
	}
}

/*QUAKED misc_siege_item (1 0 1) (-16 -16 -24) (16 16 32) ? x x STARTOFFRADAR
STARTOFFRADAR - start not displaying on radar, don't display until used.

"model"				Name of model to use for the object
"mins"				Actual mins of the object. Careful not to place it into a solid,
					as these new mins will not be reflected visually in the editor.
					Default value is "-16 -16 -24".
"maxs"				Same as above for maxs. Default value is "16 16 32".
"targetname"		If it has a targetname, it will only spawn upon being used.
"target2"			Target to fire upon pickup. If none, nothing will happen.
"pickuponlyonce"	If non-0, target2 will only be fired on the first pickup. If the item is
					dropped and picked up again later, the target will not be fired off on
					the sequential pickup. Default value is 1.
"target3"			Target to fire upon delivery of the item to the goal point.
					If none, nothing will happen. (but you should always want something to happen)
"health"			If > 0, object can be damaged and will die once health reaches 0. Default is 0.
"showhealth"		if health > 0, will show a health meter for this item
"teamowner"			Which team owns this item, used only for deciding what color to make health meter
"target4"			Target to fire upon death, if damageable. Default is none.
"deathfx"			Effect to play on death, if damageable. Default is none.
"canpickup"			If non-0, item can be picked up. Otherwise it will just be solid and sit on the
					ground. Default is 1.
"pickupsound"		Sound to play on pickup, if any.
"goaltarget"		Must be the targetname of a trigger_multi/trigger_once. Once a player carrying
					this object is brought inside the specified trigger, then that trigger will be
					allowed to fire. Ideally it will target a siege objective or something like that.
"usephysics"		If non-0, run standard physics on the object. Default is 1.
"mass"				If usephysics, this will be the factored object mass. Default is 0.09.
"gravity"			If usephysics, this will be the factored gravitational pull. Default is 3.0.
"bounce"			If usephysics, this will be the factored bounce amount. Default is 1.3.
"teamnotouch"		If 1 don't let team 1 pickup, if 2 don't let team 2. By default both teams
					can pick this object up and carry it around until death.
"teamnocomplete"	Same values as above, but controls if this object can be taken into the objective
					area by said team.
"respawnfx"			Plays this effect when respawning (e.g. it is left in an unknown area too long
					and goes back to the original spot). If this is not specified there will be
					no effect. (expected format is .efx file)
"paintarget"		plop self on top of this guy's origin when we are used (only applies if the siege
					item has a targetname)
"noradar"			if non-0 this thing will not show up on radar

"forcelimit"		if non-0, while carrying this item, the carrier's force powers will be crippled.
"target5"			target to fire when respawning.
"target6"			target to fire when dropped by someone carrying this item.

"icon"				icon that represents the gametype item on the radar

health charge things only work with showhealth 1 on siege items that take damage.
"health_chargeamt"	if non-0 will recharge this much health every...
"health_chargerate"	...this many milliseconds
*/
void SP_misc_siege_item (gentity_t *ent)
{
	int		canpickup;
	int		noradar;
	char	*s;

	if (!siege_valid || g_gametype.integer != GT_SIEGE)
	{
		G_FreeEntity(ent);
		return;
	}

	if (!ent->model || !ent->model[0])
	{
		G_Error("You must specify a model for misc_siege_item types.");
	}

	G_SpawnInt("canpickup", "1", &canpickup);
	G_SpawnInt("usephysics", "1", &ent->genericValue1);

	if (ent->genericValue1)
	{ //if we're using physics we want lerporigin smoothing
		ent->s.eFlags |= EF_CLIENTSMOOTH;
	}

	G_SpawnInt("noradar", "0", &noradar);
	//Want it to always show up as a goal object on radar
	if (!noradar && !(ent->spawnflags & SIEGEITEM_STARTOFFRADAR))
	{
		ent->s.eFlags |= EF_RADAROBJECT;
	}

	//All clients want to know where it is at all times for radar
	ent->r.svFlags |= SVF_BROADCAST;

	G_SpawnInt("pickuponlyonce", "1", &ent->genericValue4);

	G_SpawnInt("teamnotouch", "0", &ent->genericValue6);
	G_SpawnInt("teamnocomplete", "0", &ent->genericValue7);
	
	//Get default physics values.
	G_SpawnFloat("mass", "0.09", &ent->mass);
	G_SpawnFloat("gravity", "3.0", &ent->radius);
	G_SpawnFloat("bounce", "1.3", &ent->random);

	G_SpawnString( "pickupsound", "", &s );

	if (s && s[0])
	{ //We have a pickup sound, so index it now.
		ent->noise_index = G_SoundIndex(s);
	}

	G_SpawnString( "deathfx", "", &s );

	if (s && s[0])
	{ //We have a death effect, so index it now.
		ent->genericValue3 = G_EffectIndex(s);
	}

	G_SpawnString( "respawnfx", "", &s );

	if (s && s[0])
	{ //We have a respawn effect, so index it now.
		ent->genericValue10 = G_EffectIndex(s);
	}

	G_SpawnString( "icon", "", &s );
	
	if (s && s[0])
	{ 
		// We have an icon, so index it now.  We are reusing the genericenemyindex
		// variable rather than adding a new one to the entity state.
		ent->s.genericenemyindex = G_IconIndex(s);
	}
	
	ent->s.modelindex = G_ModelIndex(ent->model);

	//Is the model a ghoul2 model?
	if (!Q_stricmp(&ent->model[strlen(ent->model) - 4], ".glm"))
	{ //apparently so.
        ent->s.modelGhoul2 = 1;
	}

	ent->s.eType = ET_GENERAL;

	//Set the mins/maxs with default values.
	G_SpawnVector("mins", "-16 -16 -24", ent->r.mins);
	G_SpawnVector("maxs", "16 16 32", ent->r.maxs);

	VectorCopy(ent->s.origin, ent->pos1); //store off the initial origin for respawning
	G_SetOrigin(ent, ent->s.origin);

	VectorCopy(ent->s.angles, ent->r.currentAngles);
	VectorCopy(ent->s.angles, ent->s.apos.trBase);

	G_SpawnInt("forcelimit", "0", &ent->genericValue15);

	if (ent->health > 0)
	{ //If it has health, it can be killed.
		int t;

		ent->pain = SiegeItemPain;
		ent->die = SiegeItemDie;
		ent->takedamage = qtrue;

		G_SpawnInt( "showhealth", "0", &t );
		if (t)
		{ //a non-0 maxhealth value will mean we want to show the health on the hud
			ent->maxHealth = ent->health;
			G_ScaleNetHealth(ent);

			G_SpawnInt( "health_chargeamt", "0", &ent->genericValue12);
			G_SpawnInt( "health_chargerate", "0", &ent->genericValue13);
		}
	}
	else
	{ //Otherwise no.
		ent->takedamage = qfalse;
	}

	if (ent->spawnflags & SIEGEITEM_STARTOFFRADAR)
	{
		ent->use = SiegeItemUse;
	}
	else if (ent->targetname && ent->targetname[0])
	{
		ent->s.eFlags |= EF_NODRAW; //kind of hacky, but whatever
		ent->genericValue11 = canpickup;
        ent->use = SiegeItemUse;
		ent->s.eFlags &= ~EF_RADAROBJECT;
	}

	if ( (!ent->targetname || !ent->targetname[0]) ||
		 (ent->spawnflags & SIEGEITEM_STARTOFFRADAR) )
	{
		if (canpickup || !ent->takedamage)
		{ //We want to be able to walk into it to pick it up then.
			ent->r.contents = CONTENTS_TRIGGER;
			ent->clipmask = CONTENTS_SOLID|CONTENTS_TERRAIN;
			if (canpickup)
			{
				ent->touch = SiegeItemTouch;
			}
		}
		else
		{ //Make it solid.
			ent->r.contents = MASK_PLAYERSOLID;
			ent->clipmask = MASK_PLAYERSOLID;
		}

		ent->think = SiegeItemThink;
		ent->nextthink = level.time + FRAMETIME/2;
	}

	ent->genericValue8 = ENTITYNUM_NONE; //initialize the carrier to none

	ent->neverFree = qtrue; //never free us unless we specifically request it.

	trap_LinkEntity(ent);
}

//sends extra data about other client's in this client's PVS
//used for support guy etc.
//with this formatting:
//sxd 16,999,999,999|17,999,999,999
//assumed max 2 chars for cl num, 3 chars per ammo/health/maxhealth, even a single string full of
//info for all 32 clients should not get much past 450 bytes, which is well within a
//reasonable range. We don't need to send anything about the max ammo or current weapon, because
//currentState.weapon can be checked for the ent in question on the client. -rww
void G_SiegeClientExData(gentity_t *msgTarg)
{
	gentity_t *ent;
	int count = 0;
	int i = 0;
	char str[MAX_STRING_CHARS];
	char scratch[MAX_STRING_CHARS];

	while (i < level.num_entities && count < MAX_EXDATA_ENTS_TO_SEND)
	{
		ent = &g_entities[i];

		if (ent->inuse && ent->client && msgTarg->s.number != ent->s.number &&
			ent->s.eType == ET_PLAYER && msgTarg->client->sess.sessionTeam == ent->client->sess.sessionTeam &&
			trap_InPVS(msgTarg->client->ps.origin, ent->client->ps.origin))
		{ //another client in the same pvs, send his jive
            if (count)
			{ //append a seperating space if we are not the first in the list
				Q_strcat(str, sizeof(str), " ");
			}
			else
			{ //otherwise create the prepended chunk
				Q_strncpyz( str, "sxd ", sizeof(str) );			}

			//append the stats
			Com_sprintf(scratch, sizeof(scratch), "%i|%i|%i|%i", ent->s.number, ent->client->ps.stats[STAT_HEALTH],
				ent->client->ps.stats[STAT_MAX_HEALTH], ent->client->ps.ammo[weaponData[ent->client->ps.weapon].ammoIndex]);
			Q_strcat(str, sizeof(str), scratch);
			count++;
		}
		i++;
	}

	if (!count)
	{ //nothing to send
		return;
	}

	//send the string to him
	trap_SendServerCommand(msgTarg-g_entities, str);
}
