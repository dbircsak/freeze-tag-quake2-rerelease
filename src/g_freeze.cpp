#include "m_player.h"
#include "g_freeze.h"

#define	DotProduct(a, b)	a.dot(b)
#define	VectorSubtract(a, b, c)	c = a - b
#define	VectorAdd(a, b, c)	c = a + b
#define	VectorCopy(a, b)	b = a
#define	VectorSet(v, x, y, z)	v = {x, y, z}
#define	VectorClear(v)	v = {}
#define	VectorNormalize(x)	x.normalize()
#define	VectorScale(a, b, c)	c = a * b
#define	VectorLength(a)	a.length()
#define	VectorMA(veca, scale, vecb, vecc)	vecc = veca + (vecb * scale)
float VectorNormalize2(const vec3_t& v, vec3_t& out) {
	float length = v.length();

	if (length > 0.0f)
		out = v / length;
	return length;
}

// I just wish they'd made CTF_TEAM1 0
ctfteam_t get_team_enum(int i) {
	if (i == 0)
		return CTF_TEAM1;
	if (i == 1)
		return CTF_TEAM2;
	return CTF_TEAM2;
}
uint32_t get_team_int(ctfteam_t t) {
	if (t == CTF_TEAM1)
		return 0;
	if (t == CTF_TEAM2)
		return 1;
	return 1;
}

#define	hook_on	0x00000001
#define	hook_in	0x00000002
#define	shrink_on	0x00000004
#define	grow_on	0x00000008
#define	motor_off	0
#define	motor_start	1
#define	motor_on	2
#define	chan_hook	CHAN_AUX

#define	_drophook	"medic/medatck5.wav"
#define	_motorstart	"parasite/paratck2.wav"
#define	_motoron	"parasite/paratck3.wav"
#define	_motoroff	"parasite/paratck4.wav"
#define	_hooktouch	"parasite/paratck1.wav"
#define	_touchsolid	"medic/medatck3.wav"
#define	_firehook	"medic/medatck2.wav"

#define	_shotgun	0x00000001 // 1
#define	_supershotgun	0x00000002 // 2
#define	_machinegun	0x00000004 // 4
#define	_chaingun	0x00000008 // 8
#define	_grenadelauncher	0x00000010 // 16
#define	_rocketlauncher	0x00000020 // 32
#define	_hyperblaster	0x00000040 // 64
#define	_railgun	0x00000080 // 128
#define _chainfist	0x00000100 // 256
#define _etfrifle	0x00000200 // 512
#define _proxlauncher	0x00000400 // 1024
#define _ionripper	0x00000800 // 2048
#define _plasmabeam	0x00001000 // 4096
#define _phalanx	0x00002000 // 8192
#define _disruptor	0x00004000 // 16384

#define	ready_help	0x00000001
#define	thaw_help	0x00000002
#define	frozen_help	0x00000004
#define	chase_help	0x00000008

cvar_t* hook_max_len;
cvar_t* hook_rpf;
cvar_t* hook_min_len;
cvar_t* hook_speed;
cvar_t* frozen_time;
cvar_t* start_weapon;
cvar_t* start_armor;
cvar_t* grapple_wall;
static int	gib_queue;
static int	moan[8];

void putInventory(const char* s, edict_t* ent)
{
	gitem_t* item = nullptr;
	gitem_t* ammo = nullptr;
	int	index;

	item = FindItem(s);
	if (item)
	{
		index = item->id;
		ent->client->pers.inventory[index] = 1;

		ammo = GetItemByIndex(item->ammo);
		if (ammo)
		{
			index = ammo->id;
			ent->client->pers.inventory[index] = ammo->quantity;
		}
		ent->client->newweapon = item;
	}
}

void playerWeapon(edict_t* ent)
{
	gitem_t* item = FindItem("blaster");

	item = FindItem("blaster");
	ent->client->pers.inventory[item->id] = 1;
	ent->client->newweapon = item;

	if (start_armor->value)
	{
		int	index = FindItem("jacket armor")->id;
		ent->client->pers.inventory[index] = (int)(start_armor->value / 2) * 2;
	}

	if (start_weapon->value)
	{
		if ((int)start_weapon->value & _shotgun)
			putInventory("shotgun", ent);
		if ((int)start_weapon->value & _supershotgun)
			putInventory("super shotgun", ent);
		if ((int)start_weapon->value & _machinegun)
			putInventory("machinegun", ent);
		if ((int)start_weapon->value & _chaingun)
			putInventory("chaingun", ent);
		if ((int)start_weapon->value & _grenadelauncher)
			putInventory("grenade launcher", ent);
		if ((int)start_weapon->value & _rocketlauncher)
			putInventory("rocket launcher", ent);
		if ((int)start_weapon->value & _hyperblaster)
			putInventory("hyperblaster", ent);
		if ((int)start_weapon->value & _railgun)
			putInventory("railgun", ent);
		if ((int)start_weapon->value & _chainfist)
			putInventory("chainfist", ent);
		if ((int)start_weapon->value & _etfrifle)
			putInventory("etf rifle", ent);
		if ((int)start_weapon->value & _proxlauncher)
			putInventory("prox launcher", ent);
		if ((int)start_weapon->value & _ionripper)
			putInventory("ionripper", ent);
		if ((int)start_weapon->value & _plasmabeam)
			putInventory("plasmabeam", ent);
		if ((int)start_weapon->value & _phalanx)
			putInventory("phalanx", ent);
		if ((int)start_weapon->value & _disruptor)
			putInventory("disruptor", ent);
	}
	ChangeWeapon(ent);
}

bool playerDamage(edict_t* targ, edict_t* attacker, int damage, mod_t mod)
{
	if (!targ->client)
		return false;
	if (mod.id == MOD_TELEFRAG)
		return false;
	if (!attacker->client)
		return false;
	if (targ->client->hookstate && frandom() < 0.2)
		targ->client->hookstate = 0;
	if (targ->health > 0)
	{
		if (targ == attacker)
			return false;
		if (targ->client->resp.ctf_team != attacker->client->resp.ctf_team && targ->client->respawn_time + 3_sec < level.time)
			return false;
	}
	else
	{
		if (targ->client->frozen)
		{
			if (frandom() < 0.1)
				ThrowGib(targ, "models/objects/debris2/tris.md2", damage, GIB_NONE, 1);
			return true;
		}
		else
			return false;
	}
	if (g_friendly_fire->integer)
		return true;
	//meansOfDeath |= MOD_FRIENDLY_FIRE;
	return false;
}

bool freezeCheck(edict_t* ent, mod_t mod)
{
	if (ent->deadflag)
		return false;
	if (mod.friendly_fire)
		return false;
	switch (mod.id)
	{
	case MOD_FALLING:
	case MOD_SLIME:
	case MOD_LAVA:
		if (frandom() < 0.08)
			break;
	case MOD_SUICIDE:
	case MOD_CRUSH:
	case MOD_WATER:
	case MOD_EXIT:
	case MOD_TRIGGER_HURT:
	case MOD_BFG_LASER:
	case MOD_BFG_EFFECT:
	case MOD_TELEFRAG:
	case MOD_NUKE:
		return false;
	}
	return true;
}

void freezeAnim(edict_t* ent)
{
	ent->client->anim_priority = ANIM_DEATH;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		if (rand() & 1)
		{
			ent->s.frame = FRAME_crpain1 - 1;
			ent->client->anim_end = FRAME_crpain1 + rand() % 4;
		}
		else
		{
			ent->s.frame = FRAME_crdeath1 - 1;
			ent->client->anim_end = FRAME_crdeath1 + rand() % 5;
		}
	}
	else
	{
		switch (rand() % 8)
		{
		case 0:
			ent->s.frame = FRAME_run1 - 1;
			ent->client->anim_end = FRAME_run1 + rand() % 6;
			break;
		case 1:
			ent->s.frame = FRAME_pain101 - 1;
			ent->client->anim_end = FRAME_pain101 + rand() % 4;
			break;
		case 2:
			ent->s.frame = FRAME_pain201 - 1;
			ent->client->anim_end = FRAME_pain201 + rand() % 4;
			break;
		case 3:
			ent->s.frame = FRAME_pain301 - 1;
			ent->client->anim_end = FRAME_pain301 + rand() % 4;
			break;
		case 4:
			ent->s.frame = FRAME_jump1 - 1;
			ent->client->anim_end = FRAME_jump1 + rand() % 6;
			break;
		case 5:
			ent->s.frame = FRAME_death101 - 1;
			ent->client->anim_end = FRAME_death101 + rand() % 6;
			break;
		case 6:
			ent->s.frame = FRAME_death201 - 1;
			ent->client->anim_end = FRAME_death201 + rand() % 6;
			break;
		case 7:
			ent->s.frame = FRAME_death301 - 1;
			ent->client->anim_end = FRAME_death301 + rand() % 6;
			break;
		}
	}

	if (frandom() < 0.2)
		gi.sound(ent, CHAN_BODY, gi.soundindex("player/lava2.wav"), 1, ATTN_NORM, 0);
	else
		gi.sound(ent, CHAN_BODY, gi.soundindex("boss3/d_hit.wav"), 1, ATTN_NORM, 0);
	ent->client->frozen = true;
	ent->client->frozen_time = level.time + gtime_t::from_sec(frozen_time->value);
	ent->client->resp.thawer = nullptr;
	ent->client->thaw_time = HOLD_FOREVER;
	if (frandom() > 0.3)
		ent->client->hookstate -= ent->client->hookstate & (grow_on | shrink_on);
	ent->deadflag = true;
	gi.linkentity(ent);
}

bool gibCheck()
{
	if (gib_queue > 35)
		return true;
	else
	{
		gib_queue++;
		return false;
	}
}

THINK(gibThink) (edict_t* ent) -> void
{
	gib_queue--;
	G_FreeEdict(ent);
}

void playerView(edict_t* ent)
{
	edict_t* other;
	vec3_t	ent_origin;
	vec3_t	forward;
	vec3_t	other_origin;
	vec3_t	dist;
	trace_t	trace;
	float	dot;
	float	other_dot;
	edict_t* best_other;

	if ((level.time.milliseconds() / 100) % 8)
		return;

	other_dot = 0.3f;
	best_other = nullptr;
	VectorCopy(ent->s.origin, ent_origin);
	ent_origin[2] += ent->viewheight;
	AngleVectors(ent->s.angles, forward, nullptr, nullptr);

	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		other = g_edicts + 1 + i;
		if (!other->inuse)
			continue;
		if (other->client->resp.spectator)
			continue;
		if (other == ent)
			continue;
		if (other->health <= 0 && !other->client->frozen)
			continue;
		VectorCopy(other->s.origin, other_origin);
		other_origin[2] += other->viewheight;
		VectorSubtract(other_origin, ent_origin, dist);
		if (VectorLength(dist) > 800)
			continue;
		trace = gi.trace(ent_origin, vec3_origin, vec3_origin, other_origin, ent, MASK_OPAQUE);
		if (trace.fraction != 1)
			continue;
		VectorNormalize(dist);
		dot = DotProduct(dist, forward);
		if (dot > other_dot)
		{
			other_dot = dot;
			best_other = other;
		}
	}
	if (best_other)
		ent->client->viewed = best_other;
	else
		ent->client->viewed = nullptr;
}

void playerThaw(edict_t* ent)
{
	edict_t* other;
	int	j;
	vec3_t	eorg;

	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		other = g_edicts + 1 + i;
		if (!other->inuse)
			continue;
		if (other->client->resp.spectator)
			continue;
		if (other == ent)
			continue;
		if (other->health <= 0)
			continue;
		if (other->client->resp.ctf_team != ent->client->resp.ctf_team)
			continue;
		for (j = 0; j < 3; j++)
			eorg[j] = ent->s.origin[j] - (other->s.origin[j] + (other->mins[j] + other->maxs[j]) * 0.5);
		if (VectorLength(eorg) > MELEE_DISTANCE)
			continue;
		if (!(other->client->resp.help & thaw_help))
		{
			other->client->showscores = false;
			other->client->resp.help |= thaw_help;
			gi.LocCenter_Print(other, "Wait here a second to free them.");
			gi.sound(other, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_STATIC, 0);
		}
		ent->client->resp.thawer = other;
		if (ent->client->thaw_time == HOLD_FOREVER)
		{
			ent->client->thaw_time = level.time + 3_sec;
			gi.sound(ent, CHAN_BODY, gi.soundindex("world/steam3.wav"), 1, ATTN_NORM, 0);
		}
		return;
	}
	ent->client->resp.thawer = nullptr;
	ent->client->thaw_time = HOLD_FOREVER;
}

void playerBreak(edict_t* ent, int force)
{
	int	n;

	ent->client->respawn_time = level.time + 1_sec;
	if (ent->waterlevel == 3)
		gi.sound(ent, CHAN_BODY, gi.soundindex("misc/fhit3.wav"), 1, ATTN_NORM, 0);
	else
		gi.sound(ent, CHAN_BODY, gi.soundindex("world/brkglas.wav"), 1, ATTN_NORM, 0);
	n = rand() % (gib_queue > 10 ? 5 : 3);
	if (rand() & 1)
	{
		switch (n)
		{
		case 0:
			ThrowGib(ent, "models/objects/gibs/arm/tris.md2", force, GIB_NONE, 1);
			break;
		case 1:
			ThrowGib(ent, "models/objects/gibs/bone/tris.md2", force, GIB_NONE, 1);
			break;
		case 2:
			ThrowGib(ent, "models/objects/gibs/bone2/tris.md2", force, GIB_NONE, 1);
			break;
		case 3:
			ThrowGib(ent, "models/objects/gibs/chest/tris.md2", force, GIB_NONE, 1);
			break;
		case 4:
			ThrowGib(ent, "models/objects/gibs/leg/tris.md2", force, GIB_NONE, 1);
			break;
		}
	}
	while (n--)
		ThrowGib(ent, "models/objects/debris1/tris.md2", force, GIB_NONE, 1);
	ent->takedamage = false;
	ent->movetype = MOVETYPE_TOSS;
	ThrowClientHead(ent, force);
	ent->client->frozen = false;
	freeze[get_team_int(ent->client->resp.ctf_team)].update = true;
	ent->client->ps.stats[STAT_CHASE] = 0;
}

void playerUnfreeze(edict_t* ent)
{
	if (level.time > ent->client->frozen_time && level.time > ent->client->respawn_time)
	{
		playerBreak(ent, 50);
		return;
	}
	if (ent->waterlevel == 3 && !((level.time.milliseconds() / 100) % 4))
		ent->client->frozen_time -= 150_ms;
	if (level.time > ent->client->thaw_time)
	{
		if (!ent->client->resp.thawer || !ent->client->resp.thawer->inuse)
		{
			ent->client->resp.thawer = nullptr;
			ent->client->thaw_time = HOLD_FOREVER;
		}
		else
		{
			ent->client->resp.thawer->client->resp.score++;
			ent->client->resp.thawer->client->resp.thawed++;
			freeze[get_team_int(ent->client->resp.ctf_team)].thawed++;
			if (rand() & 1)
				gi.LocBroadcast_Print(PRINT_HIGH, "{} thaws {} like a package of frozen peas.\n", ent->client->resp.thawer->client->pers.netname, ent->client->pers.netname);
			else
				gi.LocBroadcast_Print(PRINT_HIGH, "{} evicts {} from their igloo.\n", ent->client->resp.thawer->client->pers.netname, ent->client->pers.netname);
			playerBreak(ent, 100);
		}
	}
}

void playerMove(edict_t* ent)
{
	edict_t* other;
	vec3_t	forward;
	float	dist;
	int	j;
	vec3_t	eorg;

	if (ent->client->hookstate)
		return;
	AngleVectors(ent->s.angles, forward, nullptr, nullptr);
	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		other = g_edicts + 1 + i;
		if (!other->inuse)
			continue;
		if (other->client->resp.spectator)
			continue;
		if (other == ent)
			continue;
		if (!other->client->frozen)
			continue;
		if (other->client->resp.ctf_team == ent->client->resp.ctf_team)
			continue;
		if (other->client->hookstate)
			continue;
		for (j = 0; j < 3; j++)
			eorg[j] = ent->s.origin[j] - (other->s.origin[j] + (other->mins[j] + other->maxs[j]) * 0.5);
		dist = VectorLength(eorg);
		if (dist > MELEE_DISTANCE)
			continue;
		VectorScale(forward, 600, other->velocity);
		other->velocity[2] = 200;
		gi.linkentity(other);
	}
}

void freezeMain(edict_t* ent)
{
	if (!ent->inuse)
		return;
	playerView(ent);
	if (ent->client->resp.spectator)
		return;
	if (ent->client->frozen)
	{
		playerThaw(ent);
		playerUnfreeze(ent);
	}
	else if (ent->health > 0)
		playerMove(ent);
}

void freezeIntermission(void)
{
	int	i, j, k;
	int	team;

	i = j = k = 0;
	for (i = 0; i < 2; i++)
		if (get_team_score(i) > j)
			j = get_team_score(i);

	for (i = 0; i < 2; i++)
		if (get_team_score(i) == j)
		{
			k++;
			team = i;
		}

	if (k > 1)
	{
		i = j = k = 0;
		for (i = 0; i < 2; i++)
			if (freeze[i].thawed > j)
				j = freeze[i].thawed;

		for (i = 0; i < 2; i++)
			if (freeze[i].thawed == j)
			{
				k++;
				team = i;
			}
	}
	if (k != 1)
	{
		gi.LocBroadcast_Print(PRINT_HIGH, "Stalemate!\n");
		return;
	}
	gi.LocBroadcast_Print(PRINT_HIGH, "{} team is the winner!\n", freeze_team[team]);
}

void playerHealth(edict_t* ent)
{
	ent->client->pers.inventory.fill(0);

	ent->client->quad_time = 0_ms;
	ent->client->invincible_time = 0_ms;
	ent->flags &= ~FL_POWER_ARMOR;

	ent->health = ent->client->pers.max_health;

	ent->s.sound = 0;
	ent->client->weapon_sound = 0;
}

void breakTeam(int team)
{
	edict_t* ent;
	gtime_t	break_time;

	break_time = level.time;
	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse)
			continue;
		if (ent->client->frozen)
		{
			ent->client->frozen_time = break_time;
			break_time += 250_ms;
			continue;
		}
		if (ent->health > 0)
		{
			playerHealth(ent);
			playerWeapon(ent);
		}
	}
	freeze[team].break_time = break_time + 1_sec;
	if (rand() & 1)
		gi.LocBroadcast_Print(PRINT_HIGH, "{} team was run circles around by their foe.\n", freeze_team[team]);
	else
		gi.LocBroadcast_Print(PRINT_HIGH, "{} team was less than a match for their foe.\n", freeze_team[team]);
}

void updateTeam(int team)
{
	edict_t* ent;
	int	frozen, alive;
	int	play_sound = 0;

	frozen = alive = 0;
	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse)
			continue;
		if (ent->client->resp.spectator)
			continue;
		if (ent->client->resp.ctf_team != get_team_enum(team))
			continue;
		if (ent->client->frozen)
			frozen++;
		if (ent->health > 0)
			alive++;
	}
	freeze[team].frozen = frozen;
	freeze[team].alive = alive;

	if (frozen && !alive)
	{
		for (int i = 0; i < 2; i++)
		{
			if (freeze[i].alive)
			{
				play_sound++;
				G_AdjustTeamScore(get_team_enum(i), 1);
				freeze[i].update = true;
			}
		}
		breakTeam(team);

		if (play_sound <= 1)
			gi.positioned_sound(vec3_origin, world, CHAN_VOICE | CHAN_RELIABLE, gi.soundindex("world/xian1.wav"), 1, ATTN_NONE, 0);
	}
}

bool endCheck()
{
	int	i;

	for (i = 0; i < 2; i++)
		if (/*freeze[i].update && */level.time > freeze[i].last_update)
		{
			updateTeam(i);
			freeze[i].update = false;
			freeze[i].last_update = level.time + 3_sec;
		}

	if (capturelimit->value)
	{
		for (i = 0; i < 2; i++)
			if (get_team_score(i) >= capturelimit->value)
				return true;
	}

	return false;
}

void cmdMoan(edict_t* ent)
{
	if (!(ent->client->resp.help & frozen_help) && !(ent->svflags & SVF_BOT))
	{
		ent->client->showscores = false;
		ent->client->resp.help |= frozen_help;
		gi.LocCenter_Print(ent, "You have been frozen.\nWait to be saved.");
		gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_STATIC, 0);
	}
	//else if (!(ent->client->chase_target || ent->client->resp.help & chase_help) && !(ent->svflags & SVF_BOT))
	//{
	//	GetChaseTarget(ent);
	//	ent->client->showscores = false;
	//	ent->client->resp.help |= chase_help;
	//	gi.LocCenter_Print(ent, "Use the chase camera with\nyour inventory keys.");
	//	gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_STATIC, 0);
	//	return;
	//}
	if (ent->client->moan_time > level.time)
		return;
	ent->client->moan_time = level.time + 2_sec;
	if (ent->svflags & SVF_BOT)
		ent->client->moan_time += random_time(5_sec, 30_sec);
	if (ent->waterlevel == 3)
	{
		if (rand() & 1)
			gi.sound(ent, CHAN_AUTO, gi.soundindex("flipper/flpidle1.wav"), 1, ATTN_NORM, 0);
		else
			gi.sound(ent, CHAN_AUTO, gi.soundindex("flipper/flpsrch1.wav"), 1, ATTN_NORM, 0);
	}
	else
		gi.sound(ent, CHAN_AUTO, moan[rand() % 8], 1, ATTN_NORM, 0);
}

void playerShell(edict_t* ent, ctfteam_t team)
{
	ent->s.effects |= EF_COLOR_SHELL;
	if (team == CTF_TEAM1)
		ent->s.renderfx |= RF_SHELL_RED;
	else
		ent->s.renderfx |= RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE;
}

void freezeEffects(edict_t* ent)
{
	if (level.intermissiontime)
		return;
	if (!ent->client->frozen)
		return;
	if (!ent->client->resp.thawer || ((level.time.milliseconds() / 100) % 16) < 8)
		playerShell(ent, ent->client->resp.ctf_team);
}

void p_projectsourcereverse(gclient_t* client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t& result)
{
	vec3_t	_distance;

	VectorCopy(distance, _distance);

	if (client->pers.hand == LEFT_HANDED)
		;
	else if (client->pers.hand == CENTER_HANDED)
		_distance[1] = 0;
	else
		_distance[1] *= -1;
	result = G_ProjectSource(point, _distance, forward, right);
}

void drophook(edict_t* ent)
{
	ent->owner->client->hookstate = 0;
	ent->owner->client->hooker = 0;
	gi.sound(ent->owner, chan_hook, gi.soundindex(_drophook), 1, ATTN_IDLE, 0);
	G_FreeEdict(ent);
}

void maintainlinks(edict_t* ent)
{
	float	multiplier;
	vec3_t	norm_hookvel, pred_hookpos;
	vec3_t	forward, right, offset;
	vec3_t	start = {}, chainvec;
	vec3_t	norm_chainvec;

	multiplier = VectorLength(ent->velocity) / 22;
	VectorNormalize2(ent->velocity, norm_hookvel);
	VectorMA(ent->s.origin, multiplier, norm_hookvel, pred_hookpos);

	AngleVectors(ent->owner->client->v_angle, forward, right, nullptr);
	VectorSet(offset, 8, 8, (float)ent->owner->viewheight - 8);
	p_projectsourcereverse(ent->owner->client, ent->owner->s.origin, offset, forward, right, start);
	VectorSubtract(pred_hookpos, start, chainvec);
	VectorNormalize2(chainvec, norm_chainvec);
	VectorMA(pred_hookpos, -20, norm_chainvec, pred_hookpos);
	VectorMA(start, 10, norm_chainvec, start);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_MEDIC_CABLE_ATTACK);
	gi.WriteShort(ent - g_edicts);
	gi.WritePosition(pred_hookpos);
	gi.WritePosition(start);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);
}

THINK(hookbehavior) (edict_t* ent) -> void
{
	edict_t* targ;
	bool	chain_moving;
	vec3_t	forward, right;
	vec3_t	offset, start = {};
	vec3_t	end;
	vec3_t	chainvec, velpart;
	float	chainlen;
	float	force;
	int	_hook_rpf = hook_rpf->value;

	if (!_hook_rpf)
		_hook_rpf = 80;

	targ = ent->owner;
	if (!targ->inuse || !(targ->client->hookstate & hook_on) || ent->enemy->solid == SOLID_NOT ||
		(targ->health <= 0 && !targ->client->frozen) || level.intermissiontime || targ->s.event == EV_PLAYER_TELEPORT)
	{
		drophook(ent);
		return;
	}

	VectorCopy(ent->enemy->velocity, ent->velocity);

	chain_moving = false;
	if (targ->client->hookstate & grow_on && ent->angle < hook_max_len->value)
	{
		ent->angle += _hook_rpf;
		if (ent->angle > hook_max_len->value)
			ent->angle = hook_max_len->value;
		chain_moving = true;
	}
	if (targ->client->hookstate & shrink_on && ent->angle > hook_min_len->value)
	{
		ent->angle -= _hook_rpf;
		if (ent->angle < hook_min_len->value)
			ent->angle = hook_min_len->value;
		chain_moving = true;
	}
	if (chain_moving)
	{
		if (ent->sounds == motor_off)
		{
			gi.sound(targ, chan_hook, gi.soundindex(_motorstart), 1, ATTN_IDLE, 0);
			ent->sounds = motor_start;
		}
		else if (ent->sounds == motor_start)
		{
			gi.sound(targ, chan_hook, gi.soundindex(_motoron), 1, ATTN_IDLE, 0);
			ent->sounds = motor_on;
		}
	}
	else if (ent->sounds != motor_off)
	{
		gi.sound(targ, chan_hook, gi.soundindex(_motoroff), 1, ATTN_IDLE, 0);
		ent->sounds = motor_off;
	}

	AngleVectors(ent->owner->client->v_angle, forward, right, nullptr);
	VectorSet(offset, 8, 8, (float)ent->owner->viewheight - 8);
	p_projectsourcereverse(ent->owner->client, ent->owner->s.origin, offset, forward, right, start);

	targ = nullptr;
	if (ent->enemy->client)
	{
		targ = ent->enemy;
		if (!targ->inuse || (targ->health <= 0 && !targ->client->frozen) ||
			(targ->client->buttons & 4 && frandom() < 0.3) || targ->s.event == EV_PLAYER_TELEPORT)
		{
			drophook(ent);
			return;
		}
		VectorCopy(ent->s.origin, end);
		VectorCopy(start, ent->s.origin);
		VectorCopy(end, start);
		targ = ent->owner;
		ent->owner = ent->enemy;
		ent->enemy = targ;
	}

	VectorSubtract(ent->s.origin, start, chainvec);
	chainlen = VectorLength(chainvec);
	if (chainlen > ent->angle)
	{
		VectorScale(chainvec, DotProduct(ent->owner->velocity, chainvec) / DotProduct(chainvec, chainvec), velpart);
		force = (chainlen - ent->angle) * 5;
		if (DotProduct(ent->owner->velocity, chainvec) < 0)
		{
			if (chainlen > ent->angle + 25)
				VectorSubtract(ent->owner->velocity, velpart, ent->owner->velocity);
		}
		else
		{
			if (VectorLength(velpart) < force)
				force -= VectorLength(velpart);
			else
				force = 0;
		}
	}
	else
		force = 0;

	VectorNormalize(chainvec);
	VectorMA(ent->owner->velocity, force, chainvec, ent->owner->velocity);
	SV_CheckVelocity(ent->owner);

	if (targ)
	{
		targ = ent->enemy;
		ent->enemy = ent->owner;
		ent->owner = targ;
		VectorCopy(ent->enemy->s.origin, ent->s.origin);
	}
	else if (!ent->owner->client->resp.old_hook &&
		ent->owner->client->hookstate & shrink_on && chain_moving)
		ent->owner->velocity -= ent->owner->gravityVector * (ent->owner->gravity * level.gravity * gi.frame_time_s);
	maintainlinks(ent);
	ent->nextthink = level.time + FRAME_TIME_S;
}

TOUCH(hooktouch) (edict_t* ent, edict_t* other, const trace_t& tr, bool other_touching_self) -> void
{
	vec3_t	forward, right;
	vec3_t	offset, start = {};
	vec3_t	chainvec;

	AngleVectors(ent->owner->client->v_angle, forward, right, nullptr);
	VectorSet(offset, 8, 8, (float)ent->owner->viewheight - 8);
	p_projectsourcereverse(ent->owner->client, ent->owner->s.origin, offset, forward, right, start);
	VectorSubtract(ent->s.origin, start, chainvec);
	ent->angle = VectorLength(chainvec);
	if (tr.surface && tr.surface->flags & SURF_SKY)
	{
		drophook(ent);
		return;
	}
	if (other->takedamage)
		T_Damage(other, ent, ent->owner, ent->velocity, ent->s.origin, tr.plane.normal, ent->dmg, 100, DAMAGE_NONE, MOD_HIT);
	if (other->solid == SOLID_BBOX)
	{
		if (other->client && ent->owner->client->hooker < 2)
		{
			ent->owner->client->hooker++;
			other->s.origin[2] += 9;
			gi.sound(ent, CHAN_VOICE, gi.soundindex(_hooktouch), 1, ATTN_IDLE, 0);
		}
		else
		{
			drophook(ent);
			return;
		}
	}
	else if (other->solid == SOLID_BSP && grapple_wall->value)
	{
		if (!ent->owner->client->resp.old_hook)
			VectorClear(ent->owner->velocity);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_SHOTGUN);
		gi.WritePosition(ent->s.origin);
		if (!tr.plane.normal)
			gi.WriteDir(vec3_origin);
		else
			gi.WriteDir(tr.plane.normal);
		gi.multicast(ent->s.origin, MULTICAST_PVS, false);
		gi.sound(ent, CHAN_VOICE, gi.soundindex(_touchsolid), 1, ATTN_IDLE, 0);
	}
	else
	{
		drophook(ent);
		return;
	}
	VectorCopy(other->velocity, ent->velocity);
	ent->owner->client->hookstate |= hook_in;
	ent->enemy = other;
	ent->touch = nullptr;
	ent->think = hookbehavior;
	ent->nextthink = level.time + FRAME_TIME_S;
}

THINK(hookairborne) (edict_t* ent) -> void
{
	vec3_t	chainvec;
	float	chainlen;

	VectorSubtract(ent->s.origin, ent->owner->s.origin, chainvec);
	chainlen = VectorLength(chainvec);
	if (!(ent->owner->client->hookstate & hook_on) || chainlen > hook_max_len->value)
	{
		drophook(ent);
		return;
	}
	maintainlinks(ent);
	ent->nextthink = level.time + FRAME_TIME_S;
}

void firehook(edict_t* ent)
{
	int	damage;
	vec3_t	forward, right;
	vec3_t	offset, start = {};
	edict_t* newhook;

	damage = 10;

	AngleVectors(ent->client->v_angle, forward, right, nullptr);
	VectorSet(offset, 8, 8, (float)ent->viewheight - 8);
	p_projectsourcereverse(ent->client, ent->s.origin, offset, forward, right, start);

	newhook = G_Spawn();
	newhook->svflags = SVF_DEADMONSTER;
	VectorCopy(start, newhook->s.origin);
	VectorCopy(start, newhook->s.old_origin);
	newhook->s.angles = vectoangles(forward);
	VectorScale(forward, hook_speed->value, newhook->velocity);
	VectorCopy(forward, newhook->movedir);
	newhook->movetype = MOVETYPE_FLYMISSILE;
	newhook->clipmask = MASK_SHOT;
	newhook->solid = SOLID_BBOX;
	VectorClear(newhook->mins);
	VectorClear(newhook->maxs);
	newhook->owner = ent;
	newhook->dmg = damage;
	newhook->sounds = 0;
	newhook->touch = hooktouch;
	newhook->think = hookairborne;
	newhook->nextthink = level.time + FRAME_TIME_S;
	gi.linkentity(newhook);
	gi.sound(ent, chan_hook, gi.soundindex(_firehook), 1, ATTN_IDLE, 0);
}

void cmdHook(edict_t* ent)
{
	const char* s;
	int* hookstate;

	s = gi.argv(1);
	if (!*s)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "hook <value> [action / stop / grow / shrink] : control hook\n");
		if (ent->client->resp.old_hook)
			ent->client->resp.old_hook = false;
		else
			ent->client->resp.old_hook = true;
		return;
	}
	if (ent->health <= 0 || ent->client->resp.spectator)
		return;
	hookstate = &ent->client->hookstate;
	if (!(*hookstate & hook_on) && Q_strcasecmp(s, "action") == 0)
	{
		*hookstate = hook_on;
		firehook(ent);
		return;
	}
	if (*hookstate & hook_on)
	{
		if (Q_strcasecmp(s, "action") == 0)
		{
			*hookstate = 0;
			return;
		}
		if (Q_strcasecmp(s, "stop") == 0)
		{
			if (ent->client->resp.old_hook || ent->client->hooker)
				*hookstate -= *hookstate & (grow_on | shrink_on);
			else
				*hookstate = 0;
			return;
		}
		if (Q_strcasecmp(s, "grow") == 0)
		{
			*hookstate |= grow_on;
			*hookstate -= *hookstate & shrink_on;
			return;
		}
		if (Q_strcasecmp(s, "shrink") == 0)
		{
			*hookstate |= shrink_on;
			*hookstate -= *hookstate & grow_on;
			return;
		}
	}
}

void freezeSpawn()
{
	int	i;

	memset(freeze, 0, sizeof(freeze));
	for (i = 0; i < 2; i++) {
		freeze[i].update = true;
		freeze[i].last_update = level.time;
	}
	gib_queue = 0;

	moan[0] = gi.soundindex("insane/insane1.wav");
	moan[1] = gi.soundindex("insane/insane2.wav");
	moan[2] = gi.soundindex("insane/insane3.wav");
	moan[3] = gi.soundindex("insane/insane4.wav");
	moan[4] = gi.soundindex("insane/insane6.wav");
	moan[5] = gi.soundindex("insane/insane8.wav");
	moan[6] = gi.soundindex("insane/insane9.wav");
	moan[7] = gi.soundindex("insane/insane10.wav");

	gi.configstring(CS_GENERAL + 5, ">");
}

void cvarFreeze()
{
	hook_max_len = gi.cvar("hook_max_len", "1000", CVAR_NOFLAGS);
	hook_rpf = gi.cvar("hook_rpf", "50", CVAR_NOFLAGS);
	hook_min_len = gi.cvar("hook_min_len", "40", CVAR_NOFLAGS);
	hook_speed = gi.cvar("hook_speed", "1000", CVAR_NOFLAGS);
	frozen_time = gi.cvar("frozen_time", "180", CVAR_NOFLAGS);
	start_weapon = gi.cvar("start_weapon", "0", CVAR_NOFLAGS);
	start_armor = gi.cvar("start_armor", "0", CVAR_NOFLAGS);
	grapple_wall = gi.cvar("grapple_wall", "1", CVAR_NOFLAGS);
}

bool humanPlaying(edict_t* ent) {
	edict_t* other;
	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		other = g_edicts + 1 + i;
		if (!other->inuse)
			continue;
		if (other == ent)
			continue;
		if (other->svflags & SVF_BOT)
			continue;
		return true;
	}
	return false;
}
