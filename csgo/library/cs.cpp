#include "globals.h"

#ifdef _KERNEL_MODE
int _fltused;
#endif

namespace cs
{
	vm_handle csgo_handle              = 0;
	BOOL      use_dormant_check        = 0;
	DWORD     IInputSystem             = 0;

	namespace input
	{
		DWORD m_dwButton       = 0;
		DWORD m_nLastPollTick  = 0;
		DWORD m_mouseRawAccum  = 0;
	}

	namespace entity
	{
		DWORD get_client_entity(int index);
	}

	DWORD     VEngineCvar              = 0;
	DWORD     sensitivity              = 0;
	DWORD     mp_teammates_are_enemies = 0;

	DWORD     C_BasePlayer             = 0;
	DWORD     g_TeamCount              = 0;
	DWORD     g_Teams                  = 0;
	DWORD     dwViewAngles             = 0;
	DWORD     VClientEntityList        = 0;
	DWORD     dwGetAllClasses          = 0;
	DWORD     dwClientState            = 0;

	BOOL      netvar_status            = 0;
	DWORD     m_iHealth                = 0;
	DWORD     m_vecViewOffset          = 0;
	DWORD     m_lifeState              = 0;
	DWORD     m_vecPunch               = 0;
	DWORD     m_iFOV                   = 0;
	DWORD     m_iTeamNum               = 0;
	DWORD     m_bSpottedByMask         = 0;
	DWORD     m_vecOrigin              = 0;
	DWORD     m_hActiveWeapon          = 0;
	DWORD     m_iShotsFired            = 0;
	DWORD     m_iCrossHairID           = 0;
	DWORD     m_bHasDefuser            = 0;
	DWORD     m_bIsDefusing            = 0;
	DWORD     m_dwBoneMatrix           = 0;


	//
	// csgo engine init functions
	//
	DWORD get_interface_factory(DWORD module_address);
	DWORD get_interface_factory2(PVOID dumped_dll);
	DWORD get_interface(DWORD factory, PCSTR interface_name);
	DWORD get_interface_function(DWORD ptr, DWORD index);
	DWORD get_convar(PCSTR convar_name);
	int   get_convar_int(DWORD cvar);
	float get_convar_float(DWORD cvar);
	BOOL  dump_netvar_tables(BOOL (*callback)(PCSTR, DWORD, PVOID), PVOID buffer);
	DWORD dump_netvars(DWORD table, BOOL (*callback)(PCSTR, DWORD, PVOID), PVOID parameters);

	BOOL  dump_netvar_table_callback(PCSTR value, DWORD address, PVOID params);
	BOOL  dump_baseplayer_callback(PCSTR netvar_name, DWORD offset, PVOID params);
	BOOL  dump_baseentity_callback(PCSTR netvar_name, DWORD offset, PVOID params);
	BOOL  dump_csplayer_callback(PCSTR netvar_name, DWORD offset, PVOID params);
	BOOL  dump_baseanimating_callback(PCSTR netvar_name, DWORD offset, PVOID params);

	namespace player
	{
		cs::WEAPON_CLASS get_weapon_class_0(C_Player local_player);
		cs::WEAPON_CLASS get_weapon_class_1(C_Player local_player);
	}
	

	BOOL  initialize(void);
}

BOOL cs::running(void)
{
	return cs::initialize();
}

C_Player cs::teams::get_local_player(void)
{
	return vm::read_i32(csgo_handle, C_BasePlayer);
}

C_Team cs::teams::get_team_list(void)
{
	return vm::read_i32(csgo_handle, g_Teams);
}

int cs::teams::get_team_count(void)
{
	return vm::read_i32(csgo_handle, g_TeamCount);
}

C_Team cs::teams::get_team(C_TeamList team_list, int index)
{
	return vm::read_i32(csgo_handle, team_list + (index * 4));
}

int cs::teams::get_team_num(C_Team team)
{
	return vm::read_i32(csgo_handle, team + 0xB68);
}

int cs::teams::get_player_count(C_Team team)
{
	return vm::read_i32(csgo_handle, team + 0x9E4);
}

C_PlayerList cs::teams::get_player_list(C_Team team)
{
	return vm::read_i32(csgo_handle, team + 0x9E8);
}

C_Player cs::teams::get_player(C_PlayerList player_list, int index)
{
	DWORD player_index = vm::read_i32(csgo_handle, player_list + (index * 4) ) - 1;
	return cs::entity::get_client_entity(player_index);
}

BOOL cs::teams::contains_player(C_PlayerList player_list, int player_count, int index)
{
	for (int i = 0; i < player_count; i++ )
	{
		if (vm::read_i32(csgo_handle, player_list + (i * 4)) == (DWORD)index)
		{
			return 1;
		}
	}
	return 0;
}

vec2 cs::engine::get_viewangles(void)
{
	vec2 viewangles;
	if (!vm::read(csgo_handle, dwViewAngles, &viewangles, sizeof(viewangles)))
	{
		viewangles.x = 0;
		viewangles.y = 0;
	}
	return viewangles;
}

float cs::engine::get_sensitivity(void)
{
	return get_convar_float(sensitivity);
}

BOOL cs::engine::is_gamemode_ffa(void)
{
	return get_convar_int(mp_teammates_are_enemies);
}

DWORD cs::engine::get_current_tick(void)
{
	return vm::read_i32(csgo_handle, IInputSystem + input::m_nLastPollTick);
}

DWORD cs::entity::get_client_entity(int index)
{
	index = index + 1;
	index = index + 0xFFFFDFFF;
	index = index + index;
	return vm::read_i32(csgo_handle, VClientEntityList + index * 8);
}

BOOL cs::input::get_button_state(DWORD button)
{
	DWORD v = vm::read_i32(csgo_handle, IInputSystem + (((button >> 5 ) * 4) + input::m_dwButton));
	return (v >> (button & 31)) & 1;
}

void cs::input::mouse_move(int x, int y)
{
	typedef struct { int x, y; } mouse_data;
	mouse_data data;

	data.x = x;
	data.y = y;
	vm::write(csgo_handle, IInputSystem + input::m_mouseRawAccum, &data, sizeof(mouse_data));
}

BOOL cs::player::is_valid(C_Player player_address)
{
	if (player_address == 0)
	{
		return 0;
	}

	DWORD player_health = get_health(player_address);

	if (player_health < 1)
	{
		return 0;
	}

	if (player_health > 150)
	{
		return 0;
	}

	int player_lifestate = get_life_state(player_address);
	if (player_lifestate != 0)
	{
		return 0;
	}

	return get_dormant(player_address) == 0;
}

BOOL cs::player::is_visible(C_Player local_player, C_Player player)
{
	int mask = vm::read_i32(csgo_handle, player + m_bSpottedByMask);
	int base = get_player_id(local_player) - 1;
	return (mask & (1 << base)) != 0;
}

BOOL cs::player::is_defusing(C_Player player_address)
{
	return vm::read_i32(csgo_handle, player_address + m_bIsDefusing);
}

BOOL cs::player::has_defuser(C_Player player_address)
{
	return vm::read_i32(csgo_handle, player_address + m_bHasDefuser);
}

int cs::player::get_player_id(C_Player player_address)
{
	return vm::read_i32(csgo_handle, player_address + 0x64);
}

int cs::player::get_crosshair_id(C_Player player_address)
{
	return vm::read_i32(csgo_handle, player_address + m_iCrossHairID);
}

BOOL cs::player::get_dormant(C_Player player_address)
{
	if (use_dormant_check)
	{
		return vm::read_i32(csgo_handle, player_address + 0xED);
	}
	return 0;
}

int cs::player::get_life_state(C_Player player_address)
{
	if (use_dormant_check)
	{
		return vm::read_i32(csgo_handle, player_address + m_lifeState);
	}
	return 0;
}

int cs::player::get_health(C_Player player_address)
{
	return vm::read_i32(csgo_handle, player_address + m_iHealth);
}

int cs::player::get_shots_fired(C_Player player_address)
{
	return vm::read_i32(csgo_handle, player_address + m_iShotsFired);
}

vec2 cs::player::get_vec_punch(C_Player player_address)
{
	vec2 vec_punch;
	if (!vm::read(csgo_handle, player_address + m_vecPunch, &vec_punch, sizeof(vec_punch)))
	{
		vec_punch.x = 0;
		vec_punch.y = 0;
	}
	return vec_punch;
}

int cs::player::get_fov(C_Player player_address)
{
	return vm::read_i32(csgo_handle, player_address + m_iFOV);
}

DWORD cs::player::get_weapon_handle(C_Player player_address)
{
	DWORD a0 = vm::read_i32(csgo_handle, player_address + m_hActiveWeapon);
	if (a0 == 0)
	{
		return 0;
	}
	return entity::get_client_entity(((a0 & 0xFFF) - 1));
}

cs::WEAPON_CLASS cs::player::get_weapon_class_0(C_Player local_player)
{
	DWORD weapon_class = get_weapon_handle(local_player);

	if (weapon_class == 0)
	{
		return cs::WEAPON_CLASS::Invalid;
	}

	DWORD GetClientClass = vm::read_i32(csgo_handle, weapon_class+0x8);
	if (GetClientClass == 0)
	{
		return cs::WEAPON_CLASS::Invalid;
	}

	GetClientClass = vm::read_i32(csgo_handle, GetClientClass + 0x8);
	if (GetClientClass == 0)
	{
		return cs::WEAPON_CLASS::Invalid;
	}

	GetClientClass = vm::read_i32(csgo_handle, GetClientClass + 0x1);
	if (GetClientClass == 0)
	{
		return cs::WEAPON_CLASS::Invalid;
	}

	DWORD m_pMapClassname = vm::read_i32(csgo_handle, GetClientClass + 0x18);
	if (m_pMapClassname == 0)
	{
		return cs::WEAPON_CLASS::Invalid;
	}

	char weapon_buffer[260];
	vm::read(csgo_handle, m_pMapClassname, weapon_buffer, 260);


	char *index = (char *)weapon_buffer + 7;


	typedef struct {
		const char *weapon_name;
	} COMPARISON ;


	/* knife */
	{
		COMPARISON data[] = {
			{"knife"},
			{"knife_t"},
			{"knifegg"},
		};
		for (int i = 0; i < sizeof(data) / sizeof(COMPARISON); i++)
		{
			if (!strcmp(index, data[i].weapon_name))
			{
				return cs::WEAPON_CLASS::Knife;
			}
		}
	}

	/* grenade */
	{
		COMPARISON data[] = {
			{"hegrenade"},
			{"flashbang"},
			{"smokegrenade"},
			{"decoy"},
			{"molotov"},
			{"incgrenade"},
			{"c4"},
		};
		for (int i = 0; i < sizeof(data) / sizeof(COMPARISON); i++)
		{
			if (!strcmp(index, data[i].weapon_name))
			{
				return cs::WEAPON_CLASS::Grenade;
			}
		}
	}

	/* pistol */
	{
		COMPARISON data[] = {
			{"ssg08"}, // scout and deagle, in my opinion definitely belongs to same category
			{"hkp2000"},
			{"deagle"},
			{"p250"},
			{"elite"},
			{"fiveseven"},
			{"glock"},
			{"tec9"},
		};
		for (int i = 0; i < sizeof(data) / sizeof(COMPARISON); i++)
		{
			if (!strcmp(index, data[i].weapon_name))
			{
				return cs::WEAPON_CLASS::Pistol;
			}
		}
	}

	/* sniper */
	{
		COMPARISON data[] = {
			{"awp"},
			{"scar20"},
			{"g3sg1"},
		};
		for (int i = 0; i < sizeof(data) / sizeof(COMPARISON); i++)
		{
			if (!strcmp(index, data[i].weapon_name))
			{
				return cs::WEAPON_CLASS::Sniper;
			}
		}
	}

	return cs::WEAPON_CLASS::Rifle;
}

//
// backup just in case
//
cs::WEAPON_CLASS cs::player::get_weapon_class_1(C_Player local_player)
{
	DWORD weapon_class = cs::player::get_weapon_handle(local_player);
	if (weapon_class == 0)
	{
		return cs::WEAPON_CLASS::Invalid;
	}

	DWORD weapon = vm::read_i32(csgo_handle, weapon_class + 0x2FAA);

	/* knife */
	{
		DWORD data[] = {31, 42, 49, 59, 500, 505, 506, 507, 508, 509, 512, 514, 515, 516};
		for (int i = 0; i < sizeof(data) / 4; i++)
			if (data[i] == weapon)
				return cs::WEAPON_CLASS::Knife;
	}

	/* grenade */
	{
		DWORD data[] = {43, 44, 45, 46, 47, 48};
		for (int i = 0; i < sizeof(data) / 4; i++)
			if (data[i] == weapon)
				return cs::WEAPON_CLASS::Grenade;
	}

	/* pistol */
	{
		DWORD data[] = {1, 2, 3, 4, 30, 32, 36, 61, 63};
		for (int i = 0; i < sizeof(data) / 4; i++)
			if (data[i] == weapon)
				return cs::WEAPON_CLASS::Pistol;
	}

	/* sniper */
	{
		DWORD data[] = {9, 11, 38, 40};
		for (int i = 0; i < sizeof(data) / 4; i++)
			if (data[i] == weapon)
				return cs::WEAPON_CLASS::Sniper;
	}

	return cs::WEAPON_CLASS::Rifle;
}

cs::WEAPON_CLASS cs::player::get_weapon_class(C_Player player_address)
{
	WEAPON_CLASS weapon_class = get_weapon_class_0(player_address);
	if (weapon_class == WEAPON_CLASS::Invalid)
	{
		weapon_class = get_weapon_class_1(player_address);
	}
	return weapon_class;
}

vec3 cs::player::get_eye_position(C_Player player_address)
{
	vec3 origin;
	if (!vm::read(csgo_handle, player_address + m_vecOrigin, &origin, sizeof(origin)))
	{
		return vec3{0, 0, 0};
	}
	origin.z += vm::read_float(csgo_handle, player_address + m_vecViewOffset + 8);
	return origin;
}

BOOL cs::player::get_bone_position(C_Player player_address, int bone_index, matrix3x4_t *matrix)
{
	DWORD bonematrix = vm::read_i32(csgo_handle, player_address + m_dwBoneMatrix);

	if (bonematrix == 0)
	{
		return 0;
	}

	return vm::read(csgo_handle, bonematrix + (0x30 * bone_index), matrix, sizeof(matrix3x4_t));
}

int cs::get_convar_int(DWORD cvar)
{
	return vm::read_i32(csgo_handle, cvar + 0x30) ^ cvar;
}

float cs::get_convar_float(DWORD cvar)
{
	DWORD a0 = vm::read_i32(csgo_handle, cvar + 0x2C) ^ cvar;
	return *(float*)&a0;
}

BOOL cs::initialize(void)
{
	DWORD client_dll, engine_dll;
	PVOID client_dump = 0;
	DWORD GetLocalTeam;

	PVOID engine_dump = 0;
	DWORD VEngineClient;

	int   counter;


	if (csgo_handle)
	{
		if (vm::running(csgo_handle))
		{
			return 1;
		}
		csgo_handle = 0;
	}

	csgo_handle = vm::open_process_ex("csgo.exe", "client.dll");
	if (!csgo_handle)
	{
#ifdef DEBUG
		LOG("[-] csgo process not found\n");
#endif
		return 0;
	}

	use_dormant_check = 1;
	if (vm::process_exists("5EClient.exe"))
	{
		use_dormant_check = 0;
	}

	if (vm::process_exists("5EArena.exe"))
	{
		use_dormant_check = 0;
	}

	client_dll = (DWORD)vm::get_module(csgo_handle, "client.dll");
	if (client_dll == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find client.dll\n");
#endif
		goto cleanup;
	}

	engine_dll = (DWORD)vm::get_module(csgo_handle, "engine.dll");
	if (engine_dll == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find engine.dll\n");
#endif
		goto cleanup;
	}

	IInputSystem = get_interface(
		get_interface_factory((DWORD)vm::get_module(csgo_handle, "inputsystem.dll")),
		"InputSystemVersion0");

	if (IInputSystem == 0)
	{
#ifdef DEBUG
		LOG("[-] vt_input not found\n");
#endif
		goto cleanup;
	}

	input::m_dwButton= vm::read_i32(csgo_handle, get_interface_function(IInputSystem, 28) + 0xC1 + 2);
	input::m_nLastPollTick = vm::read_i32(csgo_handle, get_interface_function(IInputSystem, 13) + 0x44);
	input::m_mouseRawAccum = vm::read_i32(csgo_handle, get_interface_function(IInputSystem, 61) + 8);

	VEngineCvar = get_interface(
		get_interface_factory((DWORD)vm::get_module(csgo_handle, "vstdlib.dll")),
		"VEngineCvar0");

	if (VEngineCvar == 0)
	{
#ifdef DEBUG
		LOG("[-] VEngineCvar not found\n");
#endif
		goto cleanup;
	}

	sensitivity = get_convar("sensitivity");
	if (!sensitivity)
	{
#ifdef DEBUG
		LOG("[-] sensitivity not found\n");
#endif
		goto cleanup;
	}

	mp_teammates_are_enemies = get_convar("mp_teammates_are_enemies");
	if (!mp_teammates_are_enemies)
	{
#ifdef DEBUG
		LOG("[-] mp_teammates_are_enemies not found\n");
#endif
		goto cleanup;
	}
	
	client_dump = vm::dump_module(csgo_handle, client_dll , VM_MODULE_TYPE::CodeSectionsOnly);
	if (client_dump == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to dump client.dll\n");
#endif
		goto cleanup;
	}

	GetLocalTeam = (DWORD)vm::scan_pattern(client_dump,
		"\xE8\x00\x00\x00\x00\x85\xC0\x74\x11\x5F", "x????xxxxx", 10);

	if (GetLocalTeam == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find GetLocalTeam\n");
#endif
		goto cleanup2;
	}

	GetLocalTeam = (DWORD)vm::get_relative_address(csgo_handle, GetLocalTeam, 1, 5);

	C_BasePlayer = vm::read_i32(csgo_handle, GetLocalTeam + 0xB + 0x2);

	g_TeamCount = vm::read_i32(csgo_handle,
		vm::get_relative_address(csgo_handle, GetLocalTeam + 0x1D, 1, 5) + 0x6 + 2);

	g_Teams = vm::read_i32(csgo_handle,
		vm::get_relative_address(csgo_handle, GetLocalTeam + 0x1D, 1, 5) + 0x10 + 1);


	dwViewAngles = (DWORD)vm::scan_pattern(client_dump, "\x74\x51\x8B\x75\x0C", "xxxxx", 5);
	if (dwViewAngles == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find dwViewAngles\n");
#endif
		goto cleanup2;
	}

	dwViewAngles = vm::read_i32(csgo_handle, dwViewAngles + 0x2A + 3 + 1);
	if (dwViewAngles == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find dwViewAngles\n");
#endif
		goto cleanup2;
	}

	vm::free_module(client_dump);
	client_dump = 0;

	engine_dump = vm::dump_module(csgo_handle, engine_dll, VM_MODULE_TYPE::CodeSectionsOnly);
	if (engine_dump == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to dump engine.dll\n");
#endif
		goto cleanup;
	}

	VClientEntityList = (DWORD)vm::scan_pattern(engine_dump, "\x8A\x47\x12\x8B\x0D", "xxxxx", 5);
	if (VClientEntityList == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find VClientEntityList\n");
#endif
		goto cleanup3;
	}

	VClientEntityList = vm::read_i32(csgo_handle, VClientEntityList + 5);
	VClientEntityList = vm::read_i32(csgo_handle, VClientEntityList);

	if (VClientEntityList == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find VClientEntityList\n");
#endif
		goto cleanup3;
	}

	dwGetAllClasses = (DWORD)vm::scan_pattern(engine_dump,
		"\x8B\x0D\x00\x00\x00\x00\x0F\x57\xC0\xC7\x45", "xx????xxxxx", 11);

	if (dwGetAllClasses == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find dwGetAllClasses\n");
#endif
		goto cleanup3;
	}

	dwGetAllClasses = vm::read_i32(csgo_handle, vm::read_i32(csgo_handle, dwGetAllClasses + 2));
	dwGetAllClasses = vm::read_i32(csgo_handle, vm::read_i32(csgo_handle, get_interface_function(dwGetAllClasses, 8) + 1));

	VEngineClient = get_interface(get_interface_factory2(engine_dump), "VEngineClient0");
	if (VEngineClient == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find VEngineClient\n");
#endif
		goto cleanup3;
	}

	dwClientState = vm::read_i32(csgo_handle, vm::read_i32(csgo_handle, get_interface_function(VEngineClient, 7) + 3 + 1));
	if (dwClientState == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find dwClientState\n");
#endif
		goto cleanup3;
	}

	vm::free_module(engine_dump);
	engine_dump = 0;

	//
	// once should be enough :P
	//
	if (netvar_status == 0)
	{
		counter = 0;
		if (!dump_netvar_tables(dump_netvar_table_callback, &counter))
		{
			return 0;
		}
		netvar_status = 1;
	}

#ifdef DEBUG
	LOG("[+] csgo.exe is running\n");
#endif

	return 1;
cleanup3:
	if (engine_dump)
		vm::free_module(engine_dump);
cleanup2:
	if (client_dump)
		vm::free_module(client_dump);
cleanup:
	if (csgo_handle)
		vm::close(csgo_handle);
	csgo_handle = 0;
	return 0;
}

DWORD cs::get_interface_factory(DWORD module_address)
{
	DWORD factory = (DWORD)vm::get_module_export(csgo_handle, (QWORD)module_address, "CreateInterface");
	if (factory == 0)
	{
		return 0;
	}
	return vm::read_i32(csgo_handle, vm::read_i32(csgo_handle, factory - 0x6A));
}

DWORD cs::get_interface_factory2(PVOID dumped_dll)
{
	DWORD CreateInterface = (DWORD)vm::scan_pattern(dumped_dll,
		"\x8B\x35\x00\x00\x00\x00\x57\x85\xF6\x74\x38",
		"xx????xxxxx", 11);

	if (CreateInterface) {
		CreateInterface = vm::read_i32(csgo_handle, CreateInterface + 2);
		CreateInterface = vm::read_i32(csgo_handle, CreateInterface);
	}
	return CreateInterface;
}

DWORD cs::get_interface(DWORD factory, PCSTR interface_name)
{
	CHAR  buffer[120];
	QWORD name_length = strlen(interface_name);

	while (factory != 0)
	{
		vm::read(csgo_handle, vm::read_i32(csgo_handle, factory + 0x04), &buffer, name_length);
		buffer[name_length] = 0;

		if (!strcmp(buffer, interface_name))
		{
			return vm::read_i32(csgo_handle, vm::read_i32(csgo_handle, factory) + 1);
		}

		factory = vm::read_i32(csgo_handle, factory + 0x8);
	}
	return 0;
}

DWORD cs::get_interface_function(DWORD ptr, DWORD index)
{
	return vm::read_i32(csgo_handle, vm::read_i32(csgo_handle, ptr) + index * 4);
}

DWORD cs::get_convar(PCSTR convar_name)
{
	DWORD a0 = vm::read_i32(csgo_handle, vm::read_i32(csgo_handle,
			vm::read_i32(csgo_handle, VEngineCvar + 0x34)) + 0x4);

	QWORD cvar_length = strlen(convar_name);

	while (a0 != 0)
	{
		char name[120];

		vm::read(csgo_handle, vm::read_i32(csgo_handle, a0 + 0x0C), name, cvar_length);
		name[cvar_length] = 0;

		if (!strcmp(name, convar_name))
		{
			break;
		}

		a0 = vm::read_i32(csgo_handle, a0 + 0x4);
	}

	return a0;
}

BOOL cs::dump_netvar_tables(BOOL (*callback)(PCSTR, DWORD, PVOID), PVOID buffer)
{
	DWORD a0 = dwGetAllClasses;
	while (a0 != 0)
	{
		DWORD a1 = vm::read_i32(csgo_handle, a0 + 0x0C), a2[30];
		vm::read(csgo_handle, vm::read_i32(csgo_handle, a1 + 0x0C), a2, sizeof(a2));
		
		if (callback((PCSTR)a2, a1, buffer) != 0)
			return 1;

		a0 = vm::read_i32(csgo_handle, a0 + 0x10);
	}
	return 0;
}

DWORD cs::dump_netvars(DWORD table, BOOL (*callback)(PCSTR, DWORD, PVOID), PVOID parameters)
{
	DWORD a0 = 0, a1, a2, a3, a4, a5, a6[30];
	for (a1 = vm::read_i32(csgo_handle, table + 0x4); a1--;)
	{
		a2 = a1 * 60 + vm::read_i32(csgo_handle, table);
		a3 = vm::read_i32(csgo_handle, a2 + 0x2C);
		a4 = vm::read_i32(csgo_handle, a2 + 0x28);
		if (a4 && vm::read_i32(csgo_handle, a4 + 0x4))
		{
			a5 = dump_netvars(a4, callback, parameters);
			if (a5)
				a0 += a3 + a5;
		}
		vm::read(csgo_handle, vm::read_i32(csgo_handle, a2), a6, sizeof(a6));
		if (callback((PCSTR)a6, a3 + a0, parameters) != 0)
			return 1;
	}
	return a0;
}

BOOL cs::dump_netvar_table_callback(PCSTR value, DWORD address, PVOID params)
{
	if (!strcmp(value, "DT_BasePlayer"))
	{
		int counter = 0;
		*(int *)params = *(int *)params + dump_netvars(address, dump_baseplayer_callback, &counter);
	}
	if (!strcmp(value, "DT_BaseEntity"))
	{
		int counter = 0;
		*(int *)params = *(int *)params + dump_netvars(address, dump_baseentity_callback, &counter);
	}
	if (!strcmp(value, "DT_CSPlayer"))
	{
		int counter = 0;
		*(int *)params = *(int *)params + dump_netvars(address, dump_csplayer_callback, &counter);
	}
	if (!strcmp(value, "DT_BaseAnimating"))
	{
		int counter = 0;
		*(int *)params = *(int *)params + dump_netvars(address, dump_baseanimating_callback, &counter);
	}
	return *(int *)params == 4;
}

BOOL cs::dump_baseplayer_callback(PCSTR netvar_name, DWORD offset, PVOID params)
{
	if (!strcmp(netvar_name, "m_iHealth"))
	{
		m_iHealth = offset;
		*(int *)params = *(int *)params + 1;
	}
	if (!strcmp(netvar_name, "m_vecViewOffset[0]"))
	{
		m_vecViewOffset = offset;
		*(int *)params = *(int *)params + 1;
	}
	if (!strcmp(netvar_name, "m_lifeState"))
	{
		m_lifeState = offset;
		*(int *)params = *(int *)params + 1;
	}
	if (!strcmp(netvar_name, "m_Local"))
	{
		offset += 0x70;
		m_vecPunch = offset;
		*(int *)params = *(int *)params + 1;
	}
	if (!strcmp(netvar_name, "m_iFOV"))
	{
		m_iFOV = offset;
		*(int *)params = *(int *)params + 1;
	}
	return *(int *)params == 5;
}

BOOL cs::dump_baseentity_callback(PCSTR netvar_name, DWORD offset, PVOID params)
{
	if (!strcmp(netvar_name, "m_iTeamNum"))
	{
		m_iTeamNum = offset;
		*(int *)params = *(int *)params + 1;
	}
	if (!strcmp(netvar_name, "m_bSpottedByMask"))
	{
		m_bSpottedByMask = offset;
		*(int *)params = *(int *)params + 1;
	}
	if (!strcmp(netvar_name, "m_vecOrigin"))
	{
		m_vecOrigin = offset;
		*(int *)params = *(int *)params + 1;
	}
	return *(int *)params == 3;
}

BOOL cs::dump_csplayer_callback(PCSTR netvar_name, DWORD offset, PVOID params)
{
	if (!strcmp(netvar_name, "m_hActiveWeapon"))
	{
		m_hActiveWeapon = offset;
		*(int *)params = *(int *)params + 1;
	}
	if (!strcmp(netvar_name, "m_iShotsFired"))
	{
		m_iShotsFired = offset;
		*(int *)params = *(int *)params + 1;
	}
	if (!strcmp(netvar_name, "m_bHasDefuser"))
	{
		m_iCrossHairID = offset + 0x5C;
		m_bHasDefuser = offset;
		*(int *)params = *(int *)params + 1;
	}
	if (!strcmp(netvar_name, "m_bIsDefusing"))
	{
		m_bIsDefusing = offset;
		*(int *)params = *(int *)params + 1;
	}
	return *(int *)params == 4;
}

BOOL cs::dump_baseanimating_callback(PCSTR netvar_name, DWORD offset, PVOID params)
{
	if (!strcmp(netvar_name, "m_nSequence"))
	{
		m_dwBoneMatrix = offset + 0x54;
		*(int *)params = *(int *)params + 1;
	}
	return *(int *)params == 1;
}
