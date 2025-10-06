#pragma once
#include "g_local.h"

struct {
	int	thawed;
	bool	update;
	gtime_t	last_update;
	int	frozen;
	int	alive;
	gtime_t	break_time;
} freeze[2];

static const char* freeze_team[] = { "Red", "Blue", "Green", "Yellow", "None" };

ctfteam_t get_team_enum(int i);
uint32_t get_team_int(ctfteam_t t);
void playerWeapon(edict_t* ent);
bool playerDamage(edict_t* targ, edict_t* attacker, int damage, mod_t mod);
bool freezeCheck(edict_t* ent, mod_t mod);
void freezeAnim(edict_t* ent);
bool gibCheck();
void gibThink(edict_t* ent);
void freezeMain(edict_t* ent);
void freezeIntermission(void);
bool endCheck();
void cmdMoan(edict_t* ent);
void playerShell(edict_t* ent, ctfteam_t team);
void freezeEffects(edict_t* ent);
void cmdHook(edict_t* ent);
void freezeSpawn();
void cvarFreeze();
bool humanPlaying(edict_t* ent);
