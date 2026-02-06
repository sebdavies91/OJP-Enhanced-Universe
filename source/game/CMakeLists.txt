# Make sure the user is not executing this script directly
if(NOT InSDK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InSDK)

set(SDKGAMEHEADERS
	"${SRCDir}/game/ai_main.h"
	"${SRCDir}/game/ai.h"
	"${SRCDir}/game/anims.h"
	"${SRCDir}/game/be_aas.h"
	"${SRCDir}/game/be_ai_char.h"
	"${SRCDir}/game/be_ai_chat.h"
	"${SRCDir}/game/be_ai_gen.h"
	"${SRCDir}/game/be_ai_goal.h"
	"${SRCDir}/game/be_ai_move.h"
	"${SRCDir}/game/be_ai_weap.h"
	"${SRCDir}/game/be_ea.h"
	"${SRCDir}/game/bg_local.h"
	"${SRCDir}/game/bg_public.h"
	"${SRCDir}/game/bg_saga.h"
	"${SRCDir}/game/bg_strap.h"
	"${SRCDir}/game/bg_vehicles.h"
	"${SRCDir}/game/bg_weapons.h"
	"${SRCDir}/game/botlib.h"
	"${SRCDir}/game/chars.h"
	"${SRCDir}/game/g_camera.h"
	"${SRCDir}/game/g_dynmusic.h"
	"${SRCDir}/game/g_headers.h"
	"${SRCDir}/game/g_ICARUScb.h"
	"${SRCDir}/game/g_local.h"
	"${SRCDir}/game/g_nav.h"
	"${SRCDir}/game/g_public.h"
	"${SRCDir}/game/g_roff.h"
	"${SRCDir}/game/g_saberbeh.h"
	"${SRCDir}/game/g_team.h"
	"${SRCDir}/game/inv.h"
	"${SRCDir}/game/match.h"
	"${SRCDir}/game/q_shared.h"
	"${SRCDir}/game/surfaceflags.h"
	"${SRCDir}/game/syn.h"
	"${SRCDir}/game/w_saber.h"

	"${SRCDir}/cgame/animtable.h"
	"${SRCDir}/cgame/cg_local.h"
	"${SRCDir}/cgame/cg_public.h"
	"${SRCDir}/cgame/tr_types.h"

	"${SRCDir}/qcommon/tags.h"

	"${SRCDir}/ghoul2/g2.h"

	"${SRCDir}/icarus/interpreter.h"
	"${SRCDir}/icarus/Q3_Interface.h"
)

set(SDKGAMESOURCES
	"${SRCDir}/game/g_main.c" # keep this on top (qvm compiler needs VM_Main as the first compiled function)
	"${SRCDir}/game/ai_main.c"
	"${SRCDir}/game/ai_util.c"
	"${SRCDir}/game/ai_wpnav.c"
	"${SRCDir}/game/AnimalNPC.c"
	"${SRCDir}/game/aotctc_holocron.c"
	"${SRCDir}/game/bg_g2_utils.c"
	"${SRCDir}/game/bg_misc.c"
	"${SRCDir}/game/bg_panimate.c"
	"${SRCDir}/game/bg_pmove.c" 
	"${SRCDir}/game/bg_saber.c"
	"${SRCDir}/game/bg_saberLoad.c"
	"${SRCDir}/game/bg_saga.c"
	"${SRCDir}/game/bg_slidemove.c"
	"${SRCDir}/game/bg_vehicleLoad.c"
	"${SRCDir}/game/bg_weapons.c"
	"${SRCDir}/game/FighterNPC.c"
	"${SRCDir}/game/g_active.c"
	"${SRCDir}/game/g_arenas.c"
	"${SRCDir}/game/g_autosave.c"
	"${SRCDir}/game/g_bot.c"
	"${SRCDir}/game/g_breakable.c"
	"${SRCDir}/game/g_camera.c"
	"${SRCDir}/game/g_client.c"
	"${SRCDir}/game/g_cmds.c"
	"${SRCDir}/game/g_combat.c"
	#"${SRCDir}/game/g_crash.c"
	"${SRCDir}/game/g_dynmusic.c"
	"${SRCDir}/game/g_exphysics.c"
	"${SRCDir}/game/g_ICARUScb.c"
	"${SRCDir}/game/g_items.c"
	"${SRCDir}/game/g_log.c"
	"${SRCDir}/game/g_mem.c"
	"${SRCDir}/game/g_misc.c"
	"${SRCDir}/game/g_missile.c"
	"${SRCDir}/game/g_mover.c"
	"${SRCDir}/game/g_nav.c"
	"${SRCDir}/game/g_navnew.c"
	"${SRCDir}/game/g_object.c"
	"${SRCDir}/game/g_roff.c"
	"${SRCDir}/game/g_saberbeh.c"
	"${SRCDir}/game/g_saga.c"
	"${SRCDir}/game/g_session.c"
	"${SRCDir}/game/g_spawn.c"
	"${SRCDir}/game/g_strap.c"
	"${SRCDir}/game/g_svcmds.c"
	"${SRCDir}/game/g_syscalls.c"
	"${SRCDir}/game/g_target.c"
	"${SRCDir}/game/g_team.c"
	"${SRCDir}/game/g_timer.c"
	"${SRCDir}/game/g_trigger.c"
	"${SRCDir}/game/g_turret.c"
	"${SRCDir}/game/g_turret_G2.c"
	"${SRCDir}/game/g_utils.c"
	"${SRCDir}/game/g_vehicles.c"
	"${SRCDir}/game/g_vehicleTurret.c"
	"${SRCDir}/game/g_weapon.c"

	"${SRCDir}/game/NPC.c"
	"${SRCDir}/game/NPC_AI_Animal.c"
    "${SRCDir}/game/NPC_AI_AssassinDroid.c"
    "${SRCDir}/game/NPC_AI_Atst.c"
    "${SRCDir}/game/NPC_AI_Civilian.c"
    "${SRCDir}/game/NPC_AI_Default.c"
    "${SRCDir}/game/NPC_AI_Droid.c"
    "${SRCDir}/game/NPC_AI_GalakMech.c"
    "${SRCDir}/game/NPC_AI_Grenadier.c"
    "${SRCDir}/game/NPC_AI_Howler.c"
    "${SRCDir}/game/NPC_AI_ImperialProbe.c"
    "${SRCDir}/game/NPC_AI_Interrogator.c"
    "${SRCDir}/game/NPC_AI_Jedi.c"
    "${SRCDir}/game/NPC_AI_Mark1.c"
    "${SRCDir}/game/NPC_AI_Mark2.c"
    "${SRCDir}/game/NPC_AI_MineMonster.c"
    "${SRCDir}/game/NPC_AI_Rancor.c"
    "${SRCDir}/game/NPC_AI_Remote.c"
    "${SRCDir}/game/NPC_AI_RocketTrooper.c"
    "${SRCDir}/game/NPC_AI_SaberDroid.c"
    "${SRCDir}/game/NPC_AI_Sand_creature.c"
    "${SRCDir}/game/NPC_AI_Seeker.c"
    "${SRCDir}/game/NPC_AI_Sentry.c"
    "${SRCDir}/game/NPC_AI_Sniper.c"
    "${SRCDir}/game/NPC_AI_Stormtrooper.c"
    "${SRCDir}/game/NPC_AI_Utils.c"
    "${SRCDir}/game/NPC_AI_Wampa.c"
    "${SRCDir}/game/NPC_behavior.c"
    "${SRCDir}/game/NPC_combat.c"
    "${SRCDir}/game/NPC_goal.c"
    "${SRCDir}/game/NPC_misc.c"
    "${SRCDir}/game/NPC_move.c"
    "${SRCDir}/game/NPC_reactions.c"
    "${SRCDir}/game/NPC_senses.c"
    "${SRCDir}/game/NPC_sounds.c"
    "${SRCDir}/game/NPC_spawn.c"
    "${SRCDir}/game/NPC_stats.c"
    "${SRCDir}/game/NPC_utils.c"
    "${SRCDir}/game/NPC_Vehicle.c"
	"${SRCDir}/game/q_math.c"
	"${SRCDir}/game/q_shared.c"
	"${SRCDir}/game/SpeederNPC.c"
	"${SRCDir}/game/tri_coll_test.c"
	"${SRCDir}/game/w_force.c"
	"${SRCDir}/game/w_saber.c"
	"${SRCDir}/game/WalkerNPC.c"
)

set(SDKGAMEFILES
	${SDKGAMEHEADERS}
	${SDKGAMESOURCES}
)

add_library(${SDKGAME} SHARED ${SDKGAMEFILES})

# Defines
set(SDKGAMEDefines ${SDKGAMEDefines} "_JK2" "QAGAME")
#set(SDKGAMEIncludeDirs ${MVGLOBALIncludeDirs})

set_target_properties(${SDKGAME} PROPERTIES PREFIX "") # remove "lib" prefix for .so/.dylib files

set_property(TARGET ${SDKGAME} APPEND PROPERTY COMPILE_DEFINITIONS
  $<$<CONFIG:Debug>:${GlobalDefines} ${SDKGAMEDefines} ${DebugDefines}>
  $<$<CONFIG:Release>:${GlobalDefines} ${SDKGAMEDefines} ${ReleaseDefines}>
  $<$<CONFIG:RelWithDebInfo>:${GlobalDefines} ${SDKGAMEDefines} ${ReleaseDefines}>
)

#set_target_properties(${SDKGAME} PROPERTIES INCLUDE_DIRECTORIES "${SDKGAMEIncludeDirs}")
set_target_properties(${SDKGAME} PROPERTIES PROJECT_LABEL "Game Module")
