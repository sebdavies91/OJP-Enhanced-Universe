#include "g_local.h"
#include "q_shared.h"
#include "botlib.h"
#include "ai_main.h"

#ifdef BOT_ZMALLOC
#define MAX_BALLOC 8192

void *BAllocList[MAX_BALLOC];
#endif

char gBotChatBuffer[MAX_CLIENTS][MAX_CHAT_BUFFER_SIZE];

void *B_TempAlloc(int size)
{
	return BG_TempAlloc(size);
}

void B_TempFree(int size)
{
	BG_TempFree(size);
}


void *B_Alloc(int size)
{
#ifdef BOT_ZMALLOC
	void *ptr = NULL;
	int i = 0;

#ifdef BOTMEMTRACK
	int free = 0;
	int used = 0;

	while (i < MAX_BALLOC)
	{
		if (!BAllocList[i])
		{
			free++;
		}
		else
		{
			used++;
		}

		i++;
	}

	G_Printf("Allocations used: %i\nFree allocation slots: %i\n", used, free);

	i = 0;
#endif

	ptr = trap_BotGetMemoryGame(size);

	while (i < MAX_BALLOC)
	{
		if (!BAllocList[i])
		{
			BAllocList[i] = ptr;
			break;
		}
		i++;
	}

	if (i == MAX_BALLOC)
	{
		//If this happens we'll have to rely on this chunk being freed manually with B_Free, which it hopefully will be
#ifdef DEBUG
		G_Printf("WARNING: MAXIMUM B_ALLOC ALLOCATIONS EXCEEDED\n");
#endif
	}

	return ptr;
#else

	return BG_Alloc(size);

#endif
}

void B_Free(void *ptr)
{
#ifdef BOT_ZMALLOC
	int i = 0;

#ifdef BOTMEMTRACK
	int free = 0;
	int used = 0;

	while (i < MAX_BALLOC)
	{
		if (!BAllocList[i])
		{
			free++;
		}
		else
		{
			used++;
		}

		i++;
	}

	G_Printf("Allocations used: %i\nFree allocation slots: %i\n", used, free);

	i = 0;
#endif

	while (i < MAX_BALLOC)
	{
		if (BAllocList[i] == ptr)
		{
			BAllocList[i] = NULL;
			break;
		}

		i++;
	}

	if (i == MAX_BALLOC)
	{
		//Likely because the limit was exceeded and we're now freeing the chunk manually as we hoped would happen
#ifdef DEBUG
		G_Printf("WARNING: Freeing allocation which is not in the allocation structure\n");
#endif
	}

	trap_BotFreeMemoryGame(ptr);
#endif
}

void B_InitAlloc(void)
{
#ifdef BOT_ZMALLOC
	memset(BAllocList, 0, sizeof(BAllocList));
#endif

	memset(gWPArray, 0, sizeof(gWPArray));
}

void B_CleanupAlloc(void)
{
#ifdef BOT_ZMALLOC
	int i = 0;

	while (i < MAX_BALLOC)
	{
		if (BAllocList[i])
		{
			trap_BotFreeMemoryGame(BAllocList[i]);
			BAllocList[i] = NULL;
		}

		i++;
	}
#endif
}

int GetValueGroup(char *buf, char *group, char *outbuf)
{
	char *place, *placesecond;
	int iplace;
	int failure;
	int i;
	int startpoint, startletter;
	int subg = 0;

	i = 0;

	iplace = 0;

	place = strstr(buf, group);

	if (!place)
	{
		return 0;
	}

	startpoint = place - buf + strlen(group) + 1;
	startletter = (place - buf) - 1;

	failure = 0;

	while (buf[startpoint+1] != '{' || buf[startletter] != '\n')
	{
		placesecond = strstr(place+1, group);

		if (placesecond)
		{
			startpoint += (placesecond - place);
			startletter += (placesecond - place);
			place = placesecond;
		}
		else
		{
			failure = 1;
			break;
		}
	}

	if (failure)
	{
		return 0;
	}

	//we have found the proper group name if we made it here, so find the opening brace and read into the outbuf
	//until hitting the end brace

	while (buf[startpoint] != '{')
	{
		startpoint++;
	}

	startpoint++;

	while (buf[startpoint] != '}' || subg)
	{
		if (buf[startpoint] == '{')
		{
			subg++;
		}
		else if (buf[startpoint] == '}')
		{
			subg--;
		}
		outbuf[i] = buf[startpoint];
		i++;
		startpoint++;
	}
	outbuf[i] = '\0';

	return 1;
}

int GetPairedValue(char *buf, char *key, char *outbuf)
{
	char *place, *placesecond;
	int startpoint, startletter;
	int i, found;

	if (!buf || !key || !outbuf)
	{
		return 0;
	}

	i = 0;

	while (buf[i] && buf[i] != '\0')
	{
		if (buf[i] == '/')
		{
			if (buf[i+1] && buf[i+1] != '\0' && buf[i+1] == '/')
			{
				while (buf[i] != '\n')
				{
					buf[i] = '/';
					i++;
				}
			}
		}
		i++;
	}

	place = strstr(buf, key);

	if (!place)
	{
		return 0;
	}
	//tab == 9
	startpoint = place - buf + strlen(key);
	startletter = (place - buf) - 1;

	found = 0;

	while (!found)
	{
		if (startletter == 0 || !buf[startletter] || buf[startletter] == '\0' || buf[startletter] == 9 || buf[startletter] == ' ' || buf[startletter] == '\n')
		{
			if (buf[startpoint] == '\0' || buf[startpoint] == 9 || buf[startpoint] == ' ' || buf[startpoint] == '\n')
			{
				found = 1;
				break;
			}
		}

		placesecond = strstr(place+1, key);

		if (placesecond)
		{
			startpoint += placesecond - place;
			startletter += placesecond - place;
			place = placesecond;
		}
		else
		{
			place = NULL;
			break;
		}

	}

	if (!found || !place || !buf[startpoint] || buf[startpoint] == '\0')
	{
		return 0;
	}

	while (buf[startpoint] == ' ' || buf[startpoint] == 9 || buf[startpoint] == '\n')
	{
		startpoint++;
	}

	i = 0;

	while (buf[startpoint] && buf[startpoint] != '\0' && buf[startpoint] != '\n')
	{
		outbuf[i] = buf[startpoint];
		i++;
		startpoint++;
	}

	outbuf[i] = '\0';

	return 1;
}


//[BotTweaks]
int G_CountHumanPlayers( int ignoreClientNum, int team );
//[/BotTweaks]
int BotDoChat(bot_state_t *bs, char *section, int always)
{
	char *chatgroup;
	int rVal;
	int inc_1;
	int inc_2;
	int inc_n;
	int lines;
	int checkedline;
	int getthisline;
	gentity_t *cobject;

	if (!bs->canChat)
	{
		return 0;
	}

	if (bs->doChat)
	{ //already have a chat scheduled
		return 0;
	}

	if (trap_Cvar_VariableIntegerValue("se_language"))
	{ //no chatting unless English.
		return 0;
	}

	if (Q_irand(1, 10) > bs->chatFrequency && !always)
	{
		return 0;
	}

	//[BotTweaks]
	//don't talk unless there are human players in the game.  
	//Otherwise, we're just clogging up the games.log with crap
	if(!G_CountHumanPlayers( -1, -1 ))
	{//no humans in the game.  don't chat.
		return 0;
	}
	//[/BotTweaks]

	bs->chatTeam = 0;

	chatgroup = (char *)B_TempAlloc(MAX_CHAT_BUFFER_SIZE);

	rVal = GetValueGroup(gBotChatBuffer[bs->client], section, chatgroup);

	if (!rVal) //the bot has no group defined for the specified chat event
	{
		B_TempFree(MAX_CHAT_BUFFER_SIZE); //chatgroup
		return 0;
	}

	inc_1 = 0;
	inc_2 = 2;

	while (chatgroup[inc_2] && chatgroup[inc_2] != '\0')
	{
		if (chatgroup[inc_2] != 13 && chatgroup[inc_2] != 9)
		{
			chatgroup[inc_1] = chatgroup[inc_2];
			inc_1++;
		}
		inc_2++;
	}
	chatgroup[inc_1] = '\0';

	inc_1 = 0;

	lines = 0;

	while (chatgroup[inc_1] && chatgroup[inc_1] != '\0')
	{
		if (chatgroup[inc_1] == '\n')
		{
			lines++;
		}
		inc_1++;
	}

	if (!lines)
	{
		B_TempFree(MAX_CHAT_BUFFER_SIZE); //chatgroup
		return 0;
	}

	getthisline = Q_irand(0, (lines+1));

	if (getthisline < 1)
	{
		getthisline = 1;
	}
	if (getthisline > lines)
	{
		getthisline = lines;
	}

	checkedline = 1;

	inc_1 = 0;

	while (checkedline != getthisline)
	{
		if (chatgroup[inc_1] && chatgroup[inc_1] != '\0')
		{
			if (chatgroup[inc_1] == '\n')
			{
				inc_1++;
				checkedline++;
			}
		}

		if (checkedline == getthisline)
		{
			break;
		}

		inc_1++;
	}

	//we're at the starting position of the desired line here
	inc_2 = 0;

	while (chatgroup[inc_1] != '\n')
	{
		chatgroup[inc_2] = chatgroup[inc_1];
		inc_2++;
		inc_1++;
	}
	chatgroup[inc_2] = '\0';

	//trap_EA_Say(bs->client, chatgroup);
	inc_1 = 0;
	inc_2 = 0;

	if (strlen(chatgroup) > MAX_CHAT_LINE_SIZE)
	{
		B_TempFree(MAX_CHAT_BUFFER_SIZE); //chatgroup
		return 0;
	}

	while (chatgroup[inc_1])
	{
		if (chatgroup[inc_1] == '%' && chatgroup[inc_1+1] != '%')
		{
			inc_1++;

			if (chatgroup[inc_1] == 's' && bs->chatObject)
			{
				cobject = bs->chatObject;
			}
			else if (chatgroup[inc_1] == 'a' && bs->chatAltObject)
			{
				cobject = bs->chatAltObject;
			}
			else
			{
				cobject = NULL;
			}

			if (cobject && cobject->client)
			{
				inc_n = 0;

				while (cobject->client->pers.netname[inc_n])
				{
					bs->currentChat[inc_2] = cobject->client->pers.netname[inc_n];
					inc_2++;
					inc_n++;
				}
				inc_2--; //to make up for the auto-increment below
			}
		}
		else
		{
			bs->currentChat[inc_2] = chatgroup[inc_1];
		}
		inc_2++;
		inc_1++;
	}
	bs->currentChat[inc_2] = '\0';

	if (strcmp(section, "GeneralGreetings") == 0)
	{
		bs->doChat = 2;
	}
	else
	{
		bs->doChat = 1;
	}
	bs->chatTime_stored = (strlen(bs->currentChat)*45)+Q_irand(1300, 1500);
	bs->chatTime = level.time + bs->chatTime_stored;

	B_TempFree(MAX_CHAT_BUFFER_SIZE); //chatgroup

	return 1;
}

void ParseEmotionalAttachments(bot_state_t *bs, char *buf)
{
	int i = 0;
	int i_c = 0;
	char tbuf[16];

	while (buf[i] && buf[i] != '}')
	{
		while (buf[i] == ' ' || buf[i] == '{' || buf[i] == 9 || buf[i] == 13 || buf[i] == '\n')
		{
			i++;
		}

		if (buf[i] && buf[i] != '}')
		{
			i_c = 0;
			while (buf[i] != '{' && buf[i] != 9 && buf[i] != 13 && buf[i] != '\n')
			{
				bs->loved[bs->lovednum].name[i_c] = buf[i];
				i_c++;
				i++;
			}
			bs->loved[bs->lovednum].name[i_c] = '\0';

			while (buf[i] == ' ' || buf[i] == '{' || buf[i] == 9 || buf[i] == 13 || buf[i] == '\n')
			{
				i++;
			}

			i_c = 0;

			while (buf[i] != '{' && buf[i] != 9 && buf[i] != 13 && buf[i] != '\n')
			{
				tbuf[i_c] = buf[i];
				i_c++;
				i++;
			}
			tbuf[i_c] = '\0';

			bs->loved[bs->lovednum].level = atoi(tbuf);

			bs->lovednum++;
		}
		else
		{
			break;
		}

		if (bs->lovednum >= MAX_LOVED_ONES)
		{
			return;
		}

		i++;
	}
}

int ReadChatGroups(bot_state_t *bs, char *buf)
{
	char *cgroupbegin;
	int cgbplace;
	int i;

	cgroupbegin = strstr(buf, "BEGIN_CHAT_GROUPS");

	if (!cgroupbegin)
	{
		return 0;
	}

	if (strlen(cgroupbegin) >= MAX_CHAT_BUFFER_SIZE)
	{
		G_Printf(S_COLOR_RED "Error: Personality chat section exceeds max size\n");
		return 0;
	}

	cgbplace = cgroupbegin - buf+1;

	while (buf[cgbplace] != '\n')
	{
		cgbplace++;
	}

	i = 0;

	while (buf[cgbplace] && buf[cgbplace] != '\0')
	{
		gBotChatBuffer[bs->client][i] = buf[cgbplace];
		i++;
		cgbplace++;
	}

	gBotChatBuffer[bs->client][i] = '\0';

	return 1;
}

void BotUtilizePersonality(bot_state_t *bs)
{
	fileHandle_t f;
	int len, rlen;
	int failed;
	int i;
	//char buf[131072];
	char *buf = (char *)B_TempAlloc(131072);
	char *readbuf, *group;

	len = trap_FS_FOpenFile(bs->settings.personalityfile, &f, FS_READ);

	failed = 0;

	if (!f)
	{
		G_Printf(S_COLOR_RED "Error: Specified personality not found\n");
		B_TempFree(131072); //buf
		return;
	}

	if (len >= 131072)
	{
		G_Printf(S_COLOR_RED "Personality file exceeds maximum length\n");
trap_FS_FCloseFile(f);//[TicketFix143]
		B_TempFree(131072); //buf
		return;
	}

	trap_FS_Read(buf, len, f);

	rlen = len;

	while (len < 131072)
	{ //kill all characters after the file length, since sometimes FS_Read doesn't do that entirely (or so it seems)
		buf[len] = '\0';
		len++;
	}

	len = rlen;

	readbuf = (char *)B_TempAlloc(1024);
	group = (char *)B_TempAlloc(65536);

	if (!GetValueGroup(buf, "GeneralBotInfo", group))
	{
		G_Printf(S_COLOR_RED "Personality file contains no GeneralBotInfo group\n");
		failed = 1; //set failed so we know to set everything to default values
	}

	if (!failed && GetPairedValue(group, "reflex", readbuf))
	{
		bs->skills.reflex = atoi(readbuf);
	}
	else
	{
		bs->skills.reflex = 500; //default
	}

	if (!failed && GetPairedValue(group, "accuracy", readbuf))
	{
		bs->skills.accuracy = atof(readbuf);
	}
	else
	{
		bs->skills.accuracy = 1; //default
	}

	if (!failed && GetPairedValue(group, "turnspeed", readbuf))
	{
		bs->skills.turnspeed = atof(readbuf);
	}
	else
	{
		bs->skills.turnspeed = 0.01f; //default
	}

	if (!failed && GetPairedValue(group, "turnspeed_combat", readbuf))
	{
		bs->skills.turnspeed_combat = atof(readbuf);
	}
	else
	{
		bs->skills.turnspeed_combat = 0.05f; //default
	}

	if (!failed && GetPairedValue(group, "maxturn", readbuf))
	{
		bs->skills.maxturn = atof(readbuf);
	}
	else
	{
		bs->skills.maxturn = 360; //default
	}

	if (!failed && GetPairedValue(group, "perfectaim", readbuf))
	{
		bs->skills.perfectaim = atoi(readbuf);
	}
	else
	{
		bs->skills.perfectaim = 1; //default
	}

	if (!failed && GetPairedValue(group, "chatability", readbuf))
	{
		bs->canChat = atoi(readbuf);
	}
	else
	{
		bs->canChat = 1; //default
	}

	if (!failed && GetPairedValue(group, "chatfrequency", readbuf))
	{
		bs->chatFrequency = atoi(readbuf);
	}
	else
	{
		bs->chatFrequency = 5; //default
	}

	if (!failed && GetPairedValue(group, "hatelevel", readbuf))
	{
		bs->loved_death_thresh = atoi(readbuf);
	}
	else
	{
		bs->loved_death_thresh = 3; //default
	}

	if (!failed && GetPairedValue(group, "camper", readbuf))
	{
		bs->isCamper = atoi(readbuf);
	}
	else
	{
		bs->isCamper = 0; //default
	}

	if (!failed && GetPairedValue(group, "saberspecialist", readbuf))
	{
		bs->saberSpecialist = atoi(readbuf);
	}
	else
	{
		bs->saberSpecialist = 0; //default
	}

	if (!failed && GetPairedValue(group, "forceinfo", readbuf))
	{
		Com_sprintf(bs->forceinfo, sizeof(bs->forceinfo), "%s\0", readbuf);
	}
	else
	{
		Com_sprintf(bs->forceinfo, sizeof(bs->forceinfo), "%s\0", DEFAULT_FORCEPOWERS);
	}

	//[ExpSys]
	//boost size of forceinfo to match the current number of skills (backwards compatibility thingy)
	if( strlen(bs->forceinfo) < (NUM_TOTAL_SKILLS + 4) )
	{//forceinfo isn't long enough, boost the size...
		for(i=NUM_FORCE_POWERS+4; i < (NUM_TOTAL_SKILLS + 4); i++)
		{
			if(bs->forceinfo[i] < '0' || bs->forceinfo[i] > '3')
			{//bad value, reset
				bs->forceinfo[i] = '0';
			}
		}
		bs->forceinfo[NUM_TOTAL_SKILLS + 4 + 1] = '\0';
	}
	
	
	
	int skillLevel = FORCE_LEVEL_0;
	if (bs->settings.skill <= 1)
	{	
	skillLevel = FORCE_LEVEL_0;	
	}
	else if (bs->settings.skill <= 3)
	{	
	skillLevel = FORCE_LEVEL_1;	
	}
	else if (bs->settings.skill <= 4)
	{	
	skillLevel = FORCE_LEVEL_2;	
	}
	else if (bs->settings.skill >= 5)
	{	
	skillLevel = FORCE_LEVEL_3;	
	}	
	bs->forceinfo[NUM_FORCE_POWERS+SK_HEALTH + 4] = '0' + skillLevel;
	bs->forceinfo[NUM_FORCE_POWERS+SK_SHIELDS + 4] = '0' + skillLevel;
	bs->forceinfo[NUM_FORCE_POWERS+SK_AGILITY + 4] = '0' + skillLevel;
	bs->forceinfo[NUM_FORCE_POWERS+SK_STRENGTH + 4] = '0' + skillLevel;	
	bs->forceinfo[NUM_FORCE_POWERS+SK_POWER + 4] = '0' + skillLevel;
	bs->forceinfo[NUM_FORCE_POWERS+SK_RESISTANCE + 4] = '0' + skillLevel;		
	
	i = 4;
	for(i = 4; (i - 4) < NUM_FORCE_POWERS; i++)
	{
		if(bs->forceinfo[i] != '0')
		{//bot is using force power, make sure that they have at least one rank of force seeing
			if(bs->forceinfo[FP_SEE + 4] == '0')
			{//bot file doesn't normally have force seeing, give them some
				bs->forceinfo[FP_SEE + 4] = '1';
			}
		}
	}

	//[/ExpSys]

	//[StanceSelection]
	//add stance skills for Bots
	if(bs->forceinfo[FP_SABER_OFFENSE + 4] > 0)
	{

		int count;
		//just bump all the NPC's other saber styles to their saber offense skill level
		
		for(count = SK_BLUESTYLE; count <= SK_STAFFSTYLE; count++)
		{
			bs->forceinfo[NUM_FORCE_POWERS + count + 4] = bs->forceinfo[FP_SABER_OFFENSE + 4];
		}
		
	}
	//[/StanceSelection]	

	i = 0;

	while (i < MAX_CHAT_BUFFER_SIZE)
	{ //clear out the chat buffer for this bot
		gBotChatBuffer[bs->client][i] = '\0';
		i++;
	}

	if (bs->canChat)
	{
		if (!ReadChatGroups(bs, buf))
		{
			bs->canChat = 0;
		}
	}

	if (GetValueGroup(buf, "BotWeaponWeights", group))
	{
		if (GetPairedValue(group, "WP_MELEE", readbuf))
		{
			bs->botWeaponWeights[WP_MELEE] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WP_STUN_BATON", readbuf))
		{
			bs->botWeaponWeights[WP_STUN_BATON] = atoi(readbuf);

		}

		if (GetPairedValue(group, "WP_SABER", readbuf))
		{
			bs->botWeaponWeights[WP_SABER] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_BRYAR_PISTOL", readbuf))
		{
			bs->botWeaponWeights[WP_BRYAR_PISTOL] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_BLASTER", readbuf))
		{
			bs->botWeaponWeights[WP_BLASTER] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_DISRUPTOR", readbuf))
		{
			bs->botWeaponWeights[WP_DISRUPTOR] = atoi(readbuf);
		}

		
		if (GetPairedValue(group, "WP_BOWCASTER", readbuf))
		{
			bs->botWeaponWeights[WP_BOWCASTER] = atoi(readbuf);
		}
		
		if (GetPairedValue(group, "WP_REPEATER", readbuf))
		{
			bs->botWeaponWeights[WP_REPEATER] = atoi(readbuf);
		}
		
		if (GetPairedValue(group, "WP_DEMP2", readbuf))
		{
			bs->botWeaponWeights[WP_DEMP2] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_FLECHETTE", readbuf))
		{
			bs->botWeaponWeights[WP_FLECHETTE] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WP_ROCKET_LAUNCHER", readbuf))
		{
			bs->botWeaponWeights[WP_ROCKET_LAUNCHER] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WP_CONCUSSION", readbuf))
		{
			bs->botWeaponWeights[WP_CONCUSSION] = atoi(readbuf);
		}


		if (GetPairedValue(group, "WP_THERMAL", readbuf))
		{
			bs->botWeaponWeights[WP_THERMAL] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_TRIP_MINE", readbuf))
		{
			bs->botWeaponWeights[WP_TRIP_MINE] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_DET_PACK", readbuf))
		{
			bs->botWeaponWeights[WP_DET_PACK] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WP_BRYAR_OLD", readbuf))
		{
			bs->botWeaponWeights[WP_BRYAR_OLD] = atoi(readbuf);
			}
	}

	//[ExpSys]
	if(!bs->saberSpecialist)
	{//give bots weapon skills based on their weapon weights if they aren't a saber specialist.
		for(i=0; i < WP_NUM_WEAPONS; i++)
		{
			int skillLevel = FORCE_LEVEL_0;
			if(bs->botWeaponWeights[i] >= 30)
			{//master level
				skillLevel = FORCE_LEVEL_3;
			}
			else if(bs->botWeaponWeights[i] >= 20)
			{
				skillLevel = FORCE_LEVEL_2;
			}
			else if(bs->botWeaponWeights[i] > 0)
			{//has weapon
				skillLevel = FORCE_LEVEL_1;
			}
			else
			{//don't want this weapon.
				continue;
			}

			switch(i)
			{	
			case WP_STUN_BATON:
				bs->forceinfo[NUM_FORCE_POWERS+SK_WRIST + 4] = '0' + skillLevel;
				break;
			case WP_BRYAR_PISTOL:
				bs->forceinfo[NUM_FORCE_POWERS+SK_PISTOL + 4] = '0' + skillLevel;
				break;
			case WP_BLASTER:
				bs->forceinfo[NUM_FORCE_POWERS+SK_BLASTER + 4] = '0' + skillLevel;
				break;
			case WP_DISRUPTOR:
				bs->forceinfo[NUM_FORCE_POWERS+SK_DISRUPTOR + 4] = '0' + skillLevel;
				break;
			case WP_REPEATER:
				bs->forceinfo[NUM_FORCE_POWERS+SK_REPEATER + 4] = '0' + skillLevel;
				break;
			case WP_BOWCASTER:
				bs->forceinfo[NUM_FORCE_POWERS+SK_BOWCASTER + 4] = '0' + skillLevel;
				break;
			case WP_DEMP2:
				bs->forceinfo[NUM_FORCE_POWERS+SK_DEMP2 + 4] = '0' + skillLevel;
				break;	
			case WP_FLECHETTE:
				bs->forceinfo[NUM_FORCE_POWERS+SK_FLECHETTE + 4] = '0' + skillLevel;
				break;
			case WP_ROCKET_LAUNCHER:
				bs->forceinfo[NUM_FORCE_POWERS+SK_ROCKET + 4] = '0' + skillLevel;
				break;				
			case WP_CONCUSSION:
				bs->forceinfo[NUM_FORCE_POWERS+SK_CONCUSSION + 4] = '0' + skillLevel;
				break;	
			case WP_THERMAL:
				bs->forceinfo[NUM_FORCE_POWERS+SK_THERMAL + 4] = '0' + skillLevel;
				break;
			case WP_TRIP_MINE:
				bs->forceinfo[NUM_FORCE_POWERS+SK_TRIPMINE + 4] = '0' + skillLevel;
				break;
			case WP_DET_PACK:
				bs->forceinfo[NUM_FORCE_POWERS+SK_DETPACK + 4] = '0' + skillLevel;
				break;
			case WP_BRYAR_OLD:
				bs->forceinfo[NUM_FORCE_POWERS+SK_OLD + 4] = '0' + skillLevel;
				break;
			};
		}
	}
	//[/ExpSys]


	if (GetValueGroup(buf, "BotItemWeights", group))
	{
		if (GetPairedValue(group, "HI_SEEKER", readbuf))
		{
			bs->botItemWeights[HI_SEEKER] = atoi(readbuf);
		}
		if (GetPairedValue(group, "HI_SHIELD", readbuf))
		{
			bs->botItemWeights[HI_SHIELD] = atoi(readbuf);

		}

		if (GetPairedValue(group, "HI_MEDPAC", readbuf))
		{
			bs->botItemWeights[HI_MEDPAC] = atoi(readbuf);
		}
		
		if (GetPairedValue(group, "HI_SHIELDBOOSTER", readbuf))
		{
			bs->botItemWeights[HI_SHIELDBOOSTER] = atoi(readbuf);
		}
		
		if (GetPairedValue(group, "HI_BINOCULARS", readbuf))
		{
			bs->botItemWeights[HI_BINOCULARS] = atoi(readbuf);
		}

		if (GetPairedValue(group, "HI_SENTRY_GUN", readbuf))
		{
			bs->botItemWeights[HI_SENTRY_GUN] = atoi(readbuf);
		}

		if (GetPairedValue(group, "HI_JETPACK", readbuf))
		{
			bs->botItemWeights[HI_JETPACK] = atoi(readbuf);
		}
		if (GetPairedValue(group, "HI_SQUADTEAM", readbuf))
		{
			bs->botItemWeights[HI_SQUADTEAM] = atoi(readbuf);
		}
		
		if (GetPairedValue(group, "HI_VEHICLEMOUNT", readbuf))
		{
			bs->botItemWeights[HI_VEHICLEMOUNT] = atoi(readbuf);
		}
		
		if (GetPairedValue(group, "HI_EWEB", readbuf))
		{
			bs->botItemWeights[HI_EWEB] = atoi(readbuf);
		}
		
		if (GetPairedValue(group, "HI_CLOAK", readbuf))
		{
			bs->botItemWeights[HI_CLOAK] = atoi(readbuf);
		}
		
		if (GetPairedValue(group, "HI_FLAMETHROWER", readbuf))
		{
			bs->botItemWeights[HI_FLAMETHROWER] = atoi(readbuf);
		}

		if (GetPairedValue(group, "HI_ELECTROSHOCKER", readbuf))
		{
			bs->botItemWeights[HI_ELECTROSHOCKER] = atoi(readbuf);
		}
		if (GetPairedValue(group, "HI_SPHERESHIELD", readbuf))
		{
			bs->botItemWeights[HI_SPHERESHIELD] = atoi(readbuf);
		}


		if (GetPairedValue(group, "HI_OVERLOAD", readbuf))
		{
			bs->botItemWeights[HI_OVERLOAD] = atoi(readbuf);
		}

		if (GetPairedValue(group, "HI_GRAPPLE", readbuf))
		{
			bs->botItemWeights[HI_GRAPPLE] = atoi(readbuf);
		}

	}
	//[ExpSys]
	if(!bs->saberSpecialist)
	{//give bots item skills based on their item weights if they aren't a saber specialist.
		for(i=0; i < HI_NUM_HOLDABLE; i++)
		{
			int skillLevel = FORCE_LEVEL_0;
			if(bs->botItemWeights[i] >= 30)
			{//master level
				skillLevel = FORCE_LEVEL_3;
			}
			else if(bs->botItemWeights[i] >= 20)
			{
				skillLevel = FORCE_LEVEL_2;
			}
			else if(bs->botItemWeights[i] > 0)
			{//has weapon
				skillLevel = FORCE_LEVEL_1;
			}
			else
			{//don't want this weapon.
				continue;
			}

			switch(i)
			{	
			case HI_SEEKER:
				bs->forceinfo[NUM_FORCE_POWERS+SK_SEEKER + 4] = '0' + skillLevel;
				break;
			case HI_SHIELD:
				bs->forceinfo[NUM_FORCE_POWERS+SK_FORCEFIELD + 4] = '0' + skillLevel;
				break;
			case HI_MEDPAC:
				bs->forceinfo[NUM_FORCE_POWERS+SK_BACTA + 4] = '0' + skillLevel;
				break;
			case HI_SHIELDBOOSTER:
				bs->forceinfo[NUM_FORCE_POWERS+SK_REPAIR + 4] = '0' + skillLevel;
				break;
			case HI_BINOCULARS:
				bs->forceinfo[NUM_FORCE_POWERS+SK_BINOCULARS + 4] = '0' + skillLevel;
				break;
			case HI_SENTRY_GUN:
				bs->forceinfo[NUM_FORCE_POWERS+SK_SENTRY + 4] = '0' + skillLevel;
				break;
			case HI_JETPACK:
				bs->forceinfo[NUM_FORCE_POWERS+SK_JETPACK + 4] = '0' + skillLevel;
				break;
			case HI_SQUADTEAM:
				bs->forceinfo[NUM_FORCE_POWERS+SK_SQUADTEAM + 4] = '0' + skillLevel;
				break;	
			case HI_VEHICLEMOUNT:
				bs->forceinfo[NUM_FORCE_POWERS+SK_VEHICLEMOUNT + 4] = '0' + skillLevel;
				break;	
			case HI_EWEB:
				bs->forceinfo[NUM_FORCE_POWERS+SK_EWEB + 4] = '0' + skillLevel;
				break;	
			case HI_CLOAK:
				bs->forceinfo[NUM_FORCE_POWERS+SK_CLOAK + 4] = '0' + skillLevel;
				break;
			case HI_FLAMETHROWER:
				bs->forceinfo[NUM_FORCE_POWERS+SK_FLAMETHROWER + 4] = '0' + skillLevel;
				break;					
			case HI_ELECTROSHOCKER:
				bs->forceinfo[NUM_FORCE_POWERS+SK_ELECTROSHOCKER + 4] = '0' + skillLevel;
				break;
			case HI_SPHERESHIELD:
				bs->forceinfo[NUM_FORCE_POWERS+SK_SPHERESHIELD + 4] = '0' + skillLevel;
				break;
			case HI_OVERLOAD:
				bs->forceinfo[NUM_FORCE_POWERS+SK_OVERLOAD + 4] = '0' + skillLevel;
				break;
			case HI_GRAPPLE:
				bs->forceinfo[NUM_FORCE_POWERS+SK_GRAPPLE + 4] = '0' + skillLevel;
				break;
			};
			
		}
	}
	


	if (GetValueGroup(buf, "BotForceTypeWeights", group))
	{
		if (GetPairedValue(group, "FT_PUSHA", readbuf))
		{
			bs->botForceTypeWeights[FT_PUSHA] = atoi(readbuf);
		}
		if (GetPairedValue(group, "FT_PULLA", readbuf))
		{
			bs->botForceTypeWeights[FT_PULLA] = atoi(readbuf);
		}
		if (GetPairedValue(group, "FT_HEALA", readbuf))
		{
			bs->botForceTypeWeights[FT_HEALA] = atoi(readbuf);
		}
		if (GetPairedValue(group, "FT_PROTECTA", readbuf))
		{
			bs->botForceTypeWeights[FT_PROTECTA] = atoi(readbuf);
		}		
		if (GetPairedValue(group, "FT_ABSORBA", readbuf))
		{
			bs->botForceTypeWeights[FT_ABSORBA] = atoi(readbuf);
		}		
		if (GetPairedValue(group, "FT_TELEPATHYA", readbuf))
		{
			bs->botForceTypeWeights[FT_TELEPATHYA] = atoi(readbuf);
		}	
		if (GetPairedValue(group, "FT_STASISA", readbuf))
		{
			bs->botForceTypeWeights[FT_STASISA] = atoi(readbuf);
		}	
		if (GetPairedValue(group, "FT_GRIPA", readbuf))
		{
			bs->botForceTypeWeights[FT_GRIPA] = atoi(readbuf);
		}
		if (GetPairedValue(group, "FT_LIGHTNINGA", readbuf))
		{
			bs->botForceTypeWeights[FT_LIGHTNINGA] = atoi(readbuf);
		}	
		if (GetPairedValue(group, "FT_DRAINA", readbuf))
		{
			bs->botForceTypeWeights[FT_DRAINA] = atoi(readbuf);
		}
		if (GetPairedValue(group, "FT_RAGEA", readbuf))
		{
			bs->botForceTypeWeights[FT_RAGEA] = atoi(readbuf);
		}
		if (GetPairedValue(group, "FT_DESTRUCTIONA", readbuf))
		{
			bs->botForceTypeWeights[FT_DESTRUCTIONA] = atoi(readbuf);
		}	

	}

	//[ExpSys]
	if(bs->saberSpecialist)
	{//give bots item skills based on their item weights if they aren't a saber specialist.
		for(i=0; i < FT_NUM_TYPE; i++)
		{
			int skillLevel = FORCE_LEVEL_0;
			if(bs->botForceTypeWeights[i] >= 30)
			{//master level
				skillLevel = FORCE_LEVEL_3;
			}
			else if(bs->botForceTypeWeights[i] >= 20)
			{
				skillLevel = FORCE_LEVEL_2;
			}
			else if(bs->botForceTypeWeights[i] > 0)
			{//has weapon
				skillLevel = FORCE_LEVEL_1;
			}
			else
			{//don't want this weapon.
				continue;
			}

			switch(i)
			{	
			case FT_PUSHA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_PUSHA + 4] = '0' + skillLevel;
				break;
			case FT_PULLA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_PULLA + 4] = '0' + skillLevel;
				break;
			case FT_HEALA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_HEALA + 4] = '0' + skillLevel;
				break;
			case FT_PROTECTA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_PROTECTA + 4] = '0' + skillLevel;
				break;				
			case FT_ABSORBA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_ABSORBA + 4] = '0' + skillLevel;
				break;	
			case FT_TELEPATHYA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_TELEPATHYA + 4] = '0' + skillLevel;
				break;
			case FT_STASISA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_STASISA + 4] = '0' + skillLevel;
				break;	
			case FT_GRIPA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_GRIPA + 4] = '0' + skillLevel;
				break;
			case FT_LIGHTNINGA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_LIGHTNINGA + 4] = '0' + skillLevel;
				break;
			case FT_DRAINA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_DRAINA + 4] = '0' + skillLevel;
				break;
			case FT_RAGEA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_RAGEA + 4] = '0' + skillLevel;
				break;
			case FT_DESTRUCTIONA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_DESTRUCTIONA + 4] = '0' + skillLevel;
				break;		
			};
			
		}
	}



	if (GetValueGroup(buf, "BotWeaponTypeWeights", group))
	{
		if (GetPairedValue(group, "WT_WRISTA", readbuf))
		{
			bs->botWeaponTypeWeights[WT_WRISTA] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_WRISTB", readbuf))
		{
			bs->botWeaponTypeWeights[WT_WRISTB] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_PISTOLA", readbuf))
		{
			bs->botWeaponTypeWeights[WT_PISTOLA] = atoi(readbuf);
		}	
		if (GetPairedValue(group, "WT_PISTOLB", readbuf))
		{
			bs->botWeaponTypeWeights[WT_PISTOLB] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_BLASTERA", readbuf))
		{
			bs->botWeaponTypeWeights[WT_BLASTERA] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_BLASTERB", readbuf))
		{
			bs->botWeaponTypeWeights[WT_BLASTERB] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_DISRUPTORA", readbuf))
		{
			bs->botWeaponTypeWeights[WT_DISRUPTORA] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_DISRUPTORB", readbuf))
		{
			bs->botWeaponTypeWeights[WT_DISRUPTORB] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_BOWCASTERA", readbuf))
		{
			bs->botWeaponTypeWeights[WT_BOWCASTERA] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_BOWCASTERB", readbuf))
		{
			bs->botWeaponTypeWeights[WT_BOWCASTERB] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_REPEATERA", readbuf))
		{
			bs->botWeaponTypeWeights[WT_REPEATERA] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_REPEATERB", readbuf))
		{
			bs->botWeaponTypeWeights[WT_REPEATERB] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_DEMP2A", readbuf))
		{
			bs->botWeaponTypeWeights[WT_DEMP2A] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_DEMP2B", readbuf))
		{
			bs->botWeaponTypeWeights[WT_DEMP2B] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_FLECHETTEA", readbuf))
		{
			bs->botWeaponTypeWeights[WT_FLECHETTEA] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_FLECHETTEB", readbuf))
		{
			bs->botWeaponTypeWeights[WT_FLECHETTEB] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_CONCUSSIONA", readbuf))
		{
			bs->botWeaponTypeWeights[WT_CONCUSSIONA] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_CONCUSSIONB", readbuf))
		{
			bs->botWeaponTypeWeights[WT_CONCUSSIONB] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_ROCKETA", readbuf))
		{
			bs->botWeaponTypeWeights[WT_ROCKETA] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_ROCKETB", readbuf))
		{
			bs->botWeaponTypeWeights[WT_ROCKETB] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_THERMALA", readbuf))
		{
			bs->botWeaponTypeWeights[WT_THERMALA] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_THERMALB", readbuf))
		{
			bs->botWeaponTypeWeights[WT_THERMALB] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_TRIPMINEA", readbuf))
		{
			bs->botWeaponTypeWeights[WT_TRIPMINEA] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_TRIPMINEB", readbuf))
		{
			bs->botWeaponTypeWeights[WT_TRIPMINEB] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_DETPACKA", readbuf))
		{
			bs->botWeaponTypeWeights[WT_DETPACKA] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_DETPACKB", readbuf))
		{
			bs->botWeaponTypeWeights[WT_DETPACKB] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_OLDA", readbuf))
		{
			bs->botWeaponTypeWeights[WT_OLDA] = atoi(readbuf);
		}
		if (GetPairedValue(group, "WT_OLDB", readbuf))
		{
			bs->botWeaponTypeWeights[WT_OLDB] = atoi(readbuf);
		}
	}



	//[ExpSys]
	if(!bs->saberSpecialist)
	{//give bots item skills based on their item weights if they aren't a saber specialist.
		for(i=0; i < WT_NUM_TYPE; i++)
		{
			int skillLevel = FORCE_LEVEL_0;
			if(bs->botWeaponTypeWeights[i] >= 30)
			{//master level
				skillLevel = FORCE_LEVEL_3;
			}
			else if(bs->botWeaponTypeWeights[i] >= 20)
			{
				skillLevel = FORCE_LEVEL_2;
			}
			else if(bs->botWeaponTypeWeights[i] > 0)
			{//has weapon
				skillLevel = FORCE_LEVEL_1;
			}
			else
			{//don't want this weapon.
				continue;
			}

			switch(i)
			{							
			case WT_PISTOLA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_PISTOLA + 4] = '0' + skillLevel;
				break;
			case WT_PISTOLB:
				bs->forceinfo[NUM_FORCE_POWERS+SK_PISTOLB + 4] = '0' + skillLevel;
				break;	
			case WT_BLASTERA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_BLASTERA + 4] = '0' + skillLevel;
				break;
			case WT_BLASTERB:
				bs->forceinfo[NUM_FORCE_POWERS+SK_BLASTERB + 4] = '0' + skillLevel;
				break;		
			case WT_DISRUPTORA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_DISRUPTORA + 4] = '0' + skillLevel;
				break;
			case WT_DISRUPTORB:
				bs->forceinfo[NUM_FORCE_POWERS+SK_DISRUPTORB + 4] = '0' + skillLevel;
				break;	
			case WT_BOWCASTERA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_BOWCASTERA + 4] = '0' + skillLevel;
				break;
			case WT_BOWCASTERB:
				bs->forceinfo[NUM_FORCE_POWERS+SK_BOWCASTERB + 4] = '0' + skillLevel;
				break;	
			case WT_REPEATERA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_REPEATERA + 4] = '0' + skillLevel;
				break;
			case WT_REPEATERB:
				bs->forceinfo[NUM_FORCE_POWERS+SK_REPEATERB + 4] = '0' + skillLevel;
				break;	
			case WT_DEMP2A:
				bs->forceinfo[NUM_FORCE_POWERS+SK_DEMP2A + 4] = '0' + skillLevel;
				break;
			case WT_DEMP2B:
				bs->forceinfo[NUM_FORCE_POWERS+SK_DEMP2B + 4] = '0' + skillLevel;
				break;	
			case WT_FLECHETTEA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_FLECHETTEA + 4] = '0' + skillLevel;
				break;
			case WT_FLECHETTEB:
				bs->forceinfo[NUM_FORCE_POWERS+SK_FLECHETTEB + 4] = '0' + skillLevel;
				break;	
			case WT_CONCUSSIONA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_CONCUSSIONA + 4] = '0' + skillLevel;
				break;
			case WT_CONCUSSIONB:
				bs->forceinfo[NUM_FORCE_POWERS+SK_CONCUSSIONB + 4] = '0' + skillLevel;
				break;	
			case WT_ROCKETA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_ROCKETA + 4] = '0' + skillLevel;
				break;
			case WT_ROCKETB:
				bs->forceinfo[NUM_FORCE_POWERS+SK_ROCKETB + 4] = '0' + skillLevel;
				break;		
			case WT_THERMALA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_THERMALA + 4] = '0' + skillLevel;
				break;
			case WT_THERMALB:
				bs->forceinfo[NUM_FORCE_POWERS+SK_THERMALB + 4] = '0' + skillLevel;
				break;	
			case WT_TRIPMINEA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_TRIPMINEA + 4] = '0' + skillLevel;
				break;
			case WT_TRIPMINEB:
				bs->forceinfo[NUM_FORCE_POWERS+SK_TRIPMINEB + 4] = '0' + skillLevel;
				break;	
			case WT_DETPACKA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_DETPACKA + 4] = '0' + skillLevel;
				break;
			case WT_DETPACKB:
				bs->forceinfo[NUM_FORCE_POWERS+SK_DETPACKB + 4] = '0' + skillLevel;
				break;		
			case WT_OLDA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_OLDA + 4] = '0' + skillLevel;
				break;
			case WT_OLDB:
				bs->forceinfo[NUM_FORCE_POWERS+SK_OLDB + 4] = '0' + skillLevel;
				break;					
			};
			
		}
	}



	if (GetValueGroup(buf, "BotItemTypeWeights", group))
	{
		if (GetPairedValue(group, "HT_FLAMETHROWERA", readbuf))
		{
			bs->botItemTypeWeights[HT_FLAMETHROWERA] = atoi(readbuf);
		}
		if (GetPairedValue(group, "HT_ELECTROSHOCKERA", readbuf))
		{
			bs->botItemTypeWeights[HT_ELECTROSHOCKERA] = atoi(readbuf);
		}
		if (GetPairedValue(group, "HT_JETPACKA", readbuf))
		{
			bs->botItemTypeWeights[HT_JETPACKA] = atoi(readbuf);
		}
		if (GetPairedValue(group, "HT_JETPACKB", readbuf))
		{
			bs->botItemTypeWeights[HT_JETPACKB] = atoi(readbuf);
		}
		if (GetPairedValue(group, "HT_JETPACKC", readbuf))
		{
			bs->botItemTypeWeights[HT_JETPACKC] = atoi(readbuf);
		}
		if (GetPairedValue(group, "HT_SQUADTEAMA", readbuf))
		{
			bs->botItemTypeWeights[HT_SQUADTEAMA] = atoi(readbuf);
		}
		if (GetPairedValue(group, "HT_SQUADTEAMB", readbuf))
		{
			bs->botItemTypeWeights[HT_SQUADTEAMB] = atoi(readbuf);
		}		
		if (GetPairedValue(group, "HT_SQUADTEAMC", readbuf))
		{
			bs->botItemTypeWeights[HT_SQUADTEAMC] = atoi(readbuf);
		}
	}



	//[ExpSys]
	if(!bs->saberSpecialist)
	{//give bots item skills based on their item weights if they aren't a saber specialist.
		for(i=0; i < HT_NUM_TYPE; i++)
		{
			int skillLevel = FORCE_LEVEL_0;
			if(bs->botItemTypeWeights[i] >= 30)
			{//master level
				skillLevel = FORCE_LEVEL_3;
			}
			else if(bs->botItemTypeWeights[i] >= 20)
			{
				skillLevel = FORCE_LEVEL_2;
			}
			else if(bs->botItemTypeWeights[i] > 0)
			{//has weapon
				skillLevel = FORCE_LEVEL_1;
			}
			else
			{//don't want this weapon.
				continue;
			}

			switch(i)
			{	
			case HT_FLAMETHROWERA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_FLAMETHROWERA + 4] = '0' + skillLevel;
				break;
			case HT_ELECTROSHOCKERA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_ELECTROSHOCKERA + 4] = '0' + skillLevel;
				break;
			case HT_JETPACKA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_JETPACKA + 4] = '0' + skillLevel;
				break;
			case HT_JETPACKB:
				bs->forceinfo[NUM_FORCE_POWERS+SK_JETPACKB + 4] = '0' + skillLevel;
				break;	
			case HT_JETPACKC:
				bs->forceinfo[NUM_FORCE_POWERS+SK_JETPACKC + 4] = '0' + skillLevel;
				break;	
			case HT_SQUADTEAMA:
				bs->forceinfo[NUM_FORCE_POWERS+SK_SQUADTEAMA + 4] = '0' + skillLevel;
				break;
			case HT_SQUADTEAMB:
				bs->forceinfo[NUM_FORCE_POWERS+SK_SQUADTEAMB + 4] = '0' + skillLevel;
				break;				
			case HT_SQUADTEAMC:
				bs->forceinfo[NUM_FORCE_POWERS+SK_SQUADTEAMC + 4] = '0' + skillLevel;
				break;				
			};
			
		}
	}
	
	
	


	
	bs->lovednum = 0;

	if (GetValueGroup(buf, "EmotionalAttachments", group))
	{
		ParseEmotionalAttachments(bs, group);
	}

	B_TempFree(131072); //buf
	B_TempFree(1024); //readbuf
	B_TempFree(65536); //group
	trap_FS_FCloseFile(f);
}


//[AotCAI]
extern bot_state_t	*botstates[MAX_CLIENTS];
int FindBotType(int clientNum)
{//find and return the type of bot this player is.  We use this in the bg code for game only
	//bot stuff for the AotCAI.
	if(!(g_entities[clientNum].r.svFlags & SVF_BOT))
	{//not a bot
		return -1;
	}
	
	return botstates[clientNum]->settings.botType;
}
//[/AotCAI]


//[TABBots]
//Return the closest human player to position that is on this team.
//acts just like FindClosestPlayer
gentity_t *FindClosestHumanPlayer(vec3_t position, int enemyTeam)
{
	gentity_t *player;
	int i;
	float dist;
	float bestdist = 9999;
	gentity_t *closestplayer = NULL;
	for(i = 0; i < MAX_CLIENTS; i++)
	{
		player = &g_entities[i];
		if(!player || !player->client || !player->inuse)
		{//player not active
			continue;
		}

		if(player->r.svFlags & SVF_BOT)
		{//player bot.
			continue;
		}

		if(player->client->playerTeam != enemyTeam)
		{//this player isn't on the team I hate.
			continue;
		}

		if(player->health <= 0 || player->s.eFlags & EF_DEAD)
		{//player is dead, dont use
			continue;
		}

		dist = Distance(player->client->ps.origin, position);
		if(dist < bestdist)
		{
			closestplayer = player;
			bestdist = dist;
		}
	}

	return closestplayer;
}
//[/TABBots]

	