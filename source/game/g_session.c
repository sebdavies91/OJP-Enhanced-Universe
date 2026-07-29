// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"


/*
=======================================================================

  SESSION DATA

Session data is the only data that stays persistant across level loads
and tournament restarts.
=======================================================================
*/

/*
================
G_WriteClientSessionData

Called on game shutdown
================
*/
void G_WriteClientSessionData( gclient_t *client ) {
	const char	*s;
	const char	*var;
	int			i = 0;
	char		siegeClass[64];
	char		siegeClassTeam1[64];
	char		siegeClassTeam2[64];
	char		saberType[64];
	char		saber2Type[64];

	Q_strncpyz(siegeClass, client->sess.siegeClass, sizeof(siegeClass));

	while (siegeClass[i])
	{ //sort of a hack.. we don't want spaces by siege class names have spaces so convert them all to unused chars
		if (siegeClass[i] == ' ')
		{
			siegeClass[i] = 1;
		}

		i++;
	}

	if (!siegeClass[0])
	{ //make sure there's at least something
		Q_strncpyz(siegeClass, "none", sizeof(siegeClass));
	}

	Q_strncpyz(siegeClassTeam1, client->sess.siegeClassTeam1, sizeof(siegeClassTeam1));
	i = 0;
	while (siegeClassTeam1[i])
	{
		if (siegeClassTeam1[i] == ' ')
		{
			siegeClassTeam1[i] = 1;
		}
		i++;
	}
	if (!siegeClassTeam1[0])
	{
		Q_strncpyz(siegeClassTeam1, "none", sizeof(siegeClassTeam1));
	}

	Q_strncpyz(siegeClassTeam2, client->sess.siegeClassTeam2, sizeof(siegeClassTeam2));
	i = 0;
	while (siegeClassTeam2[i])
	{
		if (siegeClassTeam2[i] == ' ')
		{
			siegeClassTeam2[i] = 1;
		}
		i++;
	}
	if (!siegeClassTeam2[0])
	{
		Q_strncpyz(siegeClassTeam2, "none", sizeof(siegeClassTeam2));
	}

	//Do the same for the saber
	Q_strncpyz(saberType, client->sess.saberType, sizeof(saberType));

	i = 0;
	while (saberType[i])
	{
		if (saberType[i] == ' ')
		{
			saberType[i] = 1;
		}

		i++;
	}

	Q_strncpyz(saber2Type, client->sess.saber2Type, sizeof(saber2Type));

	i = 0;
	while (saber2Type[i])
	{
		if (saber2Type[i] == ' ')
		{
			saber2Type[i] = 1;
		}

		i++;
	}

	//[ExpSys]
	s = va("%i %i %i %i %i %i %i %i %i %i %i %i %s %s %s %s %s %f",
	//s = va("%i %i %i %i %i %i %i %i %i %i %i %i %s %s %s",
	//[/ExpSys]
		client->sess.sessionTeam,
		client->sess.spectatorTime,
		client->sess.spectatorState,
		client->sess.spectatorClient,
		client->sess.wins,
		client->sess.losses,
		client->sess.teamLeader,
		client->sess.setForce,
		client->sess.saberLevel,
		client->sess.selectedFP,
		client->sess.duelTeam,
		client->sess.siegeDesiredTeam,
		siegeClass,
		siegeClassTeam1,
		siegeClassTeam2,
		saberType,
		//[ExpSys]
		saber2Type,
		client->sess.skillPoints
		//saber2Type
		//[/ExpSys]
		);

	var = va( "session%i", client - level.clients );

	trap_Cvar_Set( var, s );
}

/*
================
G_ReadSessionData

Called on a reconnect
================
*/
void G_ReadSessionData(gclient_t* client) {
	char s[MAX_STRING_CHARS];
	const char* var;
	int i = 0;

	// bk001205 - format
	int teamLeader;
	int spectatorState;
	int sessionTeam;

	var = va("session%i", client - level.clients);
	trap_Cvar_VariableStringBuffer(var, s, sizeof(s));

	// Newer session data remembers the last valid Siege class per side.
	// Fall back cleanly to the older 16-field format for existing configs.
	if (sscanf(s, "%i %i %i %i %i %i %i %i %i %i %i %i %s %s %s %s %s %f",
		&sessionTeam,
		&client->sess.spectatorTime,
		&spectatorState,
		&client->sess.spectatorClient,
		&client->sess.wins,
		&client->sess.losses,
		&teamLeader,
		&client->sess.setForce,
		&client->sess.saberLevel,
		&client->sess.selectedFP,
		&client->sess.duelTeam,
		&client->sess.siegeDesiredTeam,
		&client->sess.siegeClass[0],
		&client->sess.siegeClassTeam1[0],
		&client->sess.siegeClassTeam2[0],
		&client->sess.saberType[0],
		&client->sess.saber2Type[0],
		&client->sess.skillPoints
	) != 18)
	{
		client->sess.siegeClassTeam1[0] = '\0';
		client->sess.siegeClassTeam2[0] = '\0';

		if (sscanf(s, "%i %i %i %i %i %i %i %i %i %i %i %i %s %s %s %f",
			&sessionTeam,
			&client->sess.spectatorTime,
			&spectatorState,
			&client->sess.spectatorClient,
			&client->sess.wins,
			&client->sess.losses,
			&teamLeader,
			&client->sess.setForce,
			&client->sess.saberLevel,
			&client->sess.selectedFP,
			&client->sess.duelTeam,
			&client->sess.siegeDesiredTeam,
			&client->sess.siegeClass[0],
			&client->sess.saberType[0],
			&client->sess.saber2Type[0],
			&client->sess.skillPoints
		) != 16)
		{
			client->sess.siegeClass[0] = '\0';
			client->sess.saberType[0] = '\0';
			client->sess.saber2Type[0] = '\0';
		}
	}


	while (client->sess.siegeClass[i]) { // Convert back to spaces from unused chars
		if (client->sess.siegeClass[i] == 1) {
			client->sess.siegeClass[i] = ' ';
		}
		i++;
	}

	i = 0;
	while (client->sess.siegeClassTeam1[i]) {
		if (client->sess.siegeClassTeam1[i] == 1) {
			client->sess.siegeClassTeam1[i] = ' ';
		}
		i++;
	}
	if (!Q_stricmp(client->sess.siegeClassTeam1, "none")) {
		client->sess.siegeClassTeam1[0] = '\0';
	}

	i = 0;
	while (client->sess.siegeClassTeam2[i]) {
		if (client->sess.siegeClassTeam2[i] == 1) {
			client->sess.siegeClassTeam2[i] = ' ';
		}
		i++;
	}
	if (!Q_stricmp(client->sess.siegeClassTeam2, "none")) {
		client->sess.siegeClassTeam2[0] = '\0';
	}

	i = 0;
	// And do the same for the saber type
	while (client->sess.saberType[i]) {
		if (client->sess.saberType[i] == 1) {
			client->sess.saberType[i] = ' ';
		}
		i++;
	}

	i = 0;
	while (client->sess.saber2Type[i]) {
		if (client->sess.saber2Type[i] == 1) {
			client->sess.saber2Type[i] = ' ';
		}
		i++;
	}

	// bk001205 - format issues
	client->sess.sessionTeam = (team_t)sessionTeam;
	client->sess.spectatorState = (spectatorState_t)spectatorState;
	client->sess.teamLeader = (qboolean)teamLeader;

	client->ps.fd.saberAnimLevel = client->sess.saberLevel;
	client->ps.fd.saberDrawAnimLevel = client->sess.saberLevel;
	client->ps.fd.forcePowerSelected = client->sess.selectedFP;
}



/*
================
G_InitSessionData

Called on a first-time connect
================
*/
//[ExpSys]
//added firsttime input so we'll know if we need to reset our skill point totals or not.
void G_InitSessionData(gclient_t* client, char* userinfo, qboolean isBot, qboolean firstTime) {
	clientSession_t* sess;
	const char* value;

	sess = &client->sess;

	client->sess.siegeDesiredTeam = TEAM_FREE;
	client->sess.siegeClassTeam1[0] = '\0';
	client->sess.siegeClassTeam2[0] = '\0';

	// initial team determination
	if (g_gametype.integer >= GT_SINGLE_PLAYER) {
		if (g_teamAutoJoin.integer) {
			sess->sessionTeam = PickTeam(-1, isBot);
			BroadcastTeamChange(client, -1);
		}
		else {
			if (!isBot) {
				sess->sessionTeam = TEAM_SPECTATOR;
			}
			else {
				value = Info_ValueForKey(userinfo, "team");
				if (value[0] == 'r' || value[0] == 'R') {
					sess->sessionTeam = TEAM_RED;
				}
				else if (value[0] == 'b' || value[0] == 'B') {
					sess->sessionTeam = TEAM_BLUE;
				}
				else {
					sess->sessionTeam = PickTeam(-1, isBot);
				}
				BroadcastTeamChange(client, -1);
			}
		}
	}
	else {
		value = Info_ValueForKey(userinfo, "team");
		if (value[0] == 's') {
			sess->sessionTeam = TEAM_SPECTATOR;
		}
		else {
			switch (g_gametype.integer) {
			default:
			case GT_FFA:
			case GT_HOLOCRON:
			case GT_JEDIMASTER:
				// Match Duel/Power Duel startup behavior: when auto-join is
				// disabled, human players should connect as free spectators and
				// explicitly press Join Game before spawning.  Bots may still
				// auto-fill the match.
				if (!g_teamAutoJoin.integer && !isBot) {
					sess->sessionTeam = TEAM_SPECTATOR;
				}
				else if (g_maxGameClients.integer > 0 && level.numNonSpectatorClients >= g_maxGameClients.integer) {
					sess->sessionTeam = TEAM_SPECTATOR;
				}
				else {
					sess->sessionTeam = TEAM_FREE;
				}
				break;
			case GT_DUEL:
				// Match the auto-join behavior used by other modes: when
				// g_teamAutoJoin is disabled, human clients should enter as
				// free spectators instead of being immediately queued/spawned.
				if (!g_teamAutoJoin.integer && !isBot) {
					sess->sessionTeam = TEAM_SPECTATOR;
				}
				else if (level.numNonSpectatorClients >= 2) {
					sess->sessionTeam = TEAM_SPECTATOR;
				}
				else {
					sess->sessionTeam = TEAM_FREE;
				}
				break;
			case GT_POWERDUEL:
			{
				int loners = 0;
				int doubles = 0;

				G_PowerDuelCount(&loners, &doubles, qtrue);

				if (!doubles || loners > (doubles / 2)) {
					sess->duelTeam = DUELTEAM_DOUBLE;
				}
				else {
					sess->duelTeam = DUELTEAM_LONE;
				}
			}
			sess->sessionTeam = TEAM_SPECTATOR;
			break;
			}
		}
	}

	sess->spectatorState = SPECTATOR_FREE;
	sess->spectatorTime = level.time;

	sess->siegeClass[0] = 0;
	sess->saberType[0] = 0;
	sess->saber2Type[0] = 0;

	// Reset skill points for new players
	if (firstTime) {
		sess->skillPoints = g_minForceRank.value;
	}
	else {
		// Remember the data from the last time
		char s[MAX_STRING_CHARS];
		const char* var;
		int tempInt;
		char tempChar[64];

		var = va("session%i", client - level.clients);
		trap_Cvar_VariableStringBuffer(var, s, sizeof(s));


		// Optional check for correctness: ensure 16 items were read
		if (sscanf(s, "%i %i %i %i %i %i %i %i %i %i %i %i %s %s %s %f",
			&tempInt, &tempInt, &tempInt, &tempInt, &tempInt,
			&tempInt, &tempInt, &tempInt, &tempInt, &tempInt,
			&tempInt, &tempInt, &tempChar[0], &tempChar[0],
			&tempChar[0], &client->sess.skillPoints) != 16) {
		}
	}

	G_WriteClientSessionData(client);
}


/*
==================
G_InitWorldSession

==================
*/
void G_InitWorldSession( void ) {
	char	s[MAX_STRING_CHARS];
	int			gt;

	trap_Cvar_VariableStringBuffer( "session", s, sizeof(s) );
	gt = atoi( s );
	
	// if the gametype changed since the last session, don't use any
	// client sessions
	if ( g_gametype.integer != gt ) {
		level.newSession = qtrue;
		G_Printf( "Gametype changed, clearing session data.\n" );
	}
}

/*
==================
G_WriteSessionData

==================
*/
void G_WriteSessionData( void ) {
	int		i;

	trap_Cvar_Set( "session", va("%i", g_gametype.integer) );

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			G_WriteClientSessionData( &level.clients[i] );
		}
	}
}
