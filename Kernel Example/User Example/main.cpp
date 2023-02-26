#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <memory>
#include <string_view>
#include <cstdint>
#include <vector>
#include "vectors.h"
#include <list>
#include <cmath>
#include <algorithm>

typedef struct _EXMP_MEMORY
{
	void* buffer_address;
	UINT_PTR address;
	ULONGLONG size;
	ULONG pid;
	BOOLEAN read;
	BOOLEAN write;
	BOOLEAN req_base;
	BOOLEAN draw_box;
	void* output;
	const char* module_name;
	ULONG64 base_address;
	PVOID address1;
}EXMP_MEMORY;

typedef struct EntityData
{
	uintptr_t entity;
	float angle;
	float dist;
}Entity_Data;

#define PI 2 * acos(0.0)
#define smoothness 12.0f

uintptr_t base_address = 0;
std::uint32_t process_id = 0;

//typename ...Args declares a parameter pack for a variadic template
template<typename ... Arg>
uint64_t call_hook(const Arg ... args)
{
	//void* hooked_func = GetProcAddress(LoadLibrary("win32u.dll"), "NtQueryCompositionSurfaceStatistics");
	void* hooked_func = GetProcAddress(LoadLibrary("win32u.dll"), "NtOpenCompositionSurfaceSectionInfo");

	auto func = static_cast<uint64_t(__stdcall*)(Arg...)>(hooked_func);

	return func(args ...);
}

struct HandleDisposer
{
	using pointer = HANDLE;
	void operator()(HANDLE handle) const
	{
		if (handle != NULL || handle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(handle);
		}
	}
};

using unique_handle = std::unique_ptr<HANDLE, HandleDisposer>;

std::uint32_t get_process_id(std::string_view process_name)
{
	PROCESSENTRY32 processentry;
	const unique_handle snapshot_handle(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));

	if (snapshot_handle.get() == INVALID_HANDLE_VALUE)
	{
		return NULL;
	}

	//processentry.dwSize = sizeof(MODULEENTRY32);
	processentry.dwSize = sizeof(PROCESSENTRY32);

	while (Process32Next(snapshot_handle.get(), &processentry) == TRUE)
	{	
		if (process_name.compare(processentry.szExeFile) == 0)
		{
			std::cout << processentry.szExeFile << std::endl;
			return processentry.th32ProcessID;
		}
	}
	return NULL;
}

//static ULONG64 get_module_base_address(const char* module_name)
//{
//	process_id = get_process_id(module_name);
//	std::cout << process_id << std::endl;
//	EXMP_MEMORY instructions = { 0 };
//	instructions.pid = process_id;
//	instructions.req_base = TRUE;
//	instructions.read = FALSE;
//	instructions.write = FALSE;
//	instructions.module_name = module_name;
//	call_hook(&instructions);
//
//	ULONG64 base = NULL;
//	base = instructions.base_address;
//	std::cout << "base is " + base  << std::endl;
//	std::cout << "pid is" + instructions.pid << std::endl;
//	PVOID temp = instructions.address1;
//	std::cout << temp << std::endl;
//	//if (instructions.req_base == FALSE)
//	//{
//	//	std::cout << "The value is changed" << std::endl;
//	//}
//	return base;
//}

static PVOID get_module_base_address(const char* module_name)
{
	EXMP_MEMORY instructions = { 0 };
	instructions.pid = process_id;
	instructions.req_base = TRUE;
	instructions.read = FALSE;
	instructions.write = FALSE;
	instructions.module_name = module_name;
	call_hook(&instructions);

	ULONG64 base = NULL;
	base = instructions.base_address;

	PVOID temp = instructions.address1;
	std::cout << std::dec << "pid is" << std::hex << temp << std::endl;
	return temp;
}

bool draw_box()
{
	EXMP_MEMORY instructions;
	instructions.read = FALSE;
	instructions.write = FALSE;
	instructions.req_base = FALSE;
	instructions.draw_box = TRUE;
	call_hook(&instructions);

	return true;
}

//template <class T>
//T Read(UINT_PTR read_address)
//{
//	T response{};
//	EXMP_MEMORY instructions;
//	instructions.pid = process_id;
//	instructions.size = sizeof(T);
//	instructions.address = read_address;
//	instructions.read = TRUE;
//	instructions.write = FALSE;
//	instructions.req_base = FALSE;
//	instructions.draw_box = FALSE;
//	instructions.output = &response;
//	call_hook(&instructions);
//}

template <class T>
T Read(UINT_PTR read_address)
{
	T response{};
	EXMP_MEMORY instructions;
	instructions.pid = process_id;
	instructions.size = sizeof(T);
	instructions.address = read_address;
	instructions.read = TRUE;
	instructions.write = FALSE;
	instructions.req_base = FALSE;
	instructions.draw_box = FALSE;
	instructions.output = &response;
	call_hook(&instructions);

	return response;
}

bool write_memory(UINT_PTR write_address, UINT_PTR source_address, SIZE_T write_size)
{
	EXMP_MEMORY instructions;
	instructions.address = write_address;
	instructions.pid = process_id;
	instructions.read = FALSE;
	instructions.write = TRUE;
	instructions.req_base = FALSE;
	instructions.draw_box = FALSE;
	instructions.buffer_address = (void*)source_address;
	instructions.size = write_size;
	call_hook(&instructions);

	return true;
}

template<typename S>
bool Write(UINT_PTR write_address, const S& value)
{
	return write_memory(write_address, (UINT_PTR)&value, sizeof(S));
}

struct view_matrix_t {
	float matrix[16];
};

enum class WeaponIndex : uint32_t {
	R301 = 0,
	SENTINEL = 1,
	BOCEK = 2,
	MELEE_SURVIVAL = 14,
	ALTERNATOR = 94,
	RE45,
	CHARGE_RIFLE,
	DEVOTION,
	LONGBOW,
	HAVOC,
	EVA8_AUTO,
	FLATLINE,
	G7_SCOUT,
	HEMLOK,
	KRABER,
	LSTAR,
	MASTIFF,
	MOZAMBIQUE,
	PROWLER,
	PEACEKEEPER,
	R99,
	P2020,
	SPITFIRE,
	TRIPLE_TAKE,
	WINGMAN,
	VOLT,
	REPEATER,
};

uintptr_t FindLocalPlayer(uintptr_t base_address, uintptr_t offset_1, uintptr_t offset_2)
{
	uintptr_t local_player = 0;
	local_player = Read<uintptr_t>(base_address + offset_1 + offset_2);

	if (!local_player)
		std::cout << std::dec << "Local player not found" << std::endl;

	return local_player;
}

vec3_t get_bone_pos_old(uintptr_t entity, vec3_t origin_pos, int id) {
	uintptr_t bones_array = Read<uintptr_t>(entity + 0x0e98 + 0x48);   //m_nForceBone + 0x48
	vec3_t bone_pos{};

	bone_pos.x = Read<float>(bones_array + 0xCC + (id * 0x30)) + origin_pos.x;
	bone_pos.y = Read<float>(bones_array + 0xDC + (id * 0x30)) + origin_pos.y;
	bone_pos.z = Read<float>(bones_array + 0xEC + (id * 0x30)) + origin_pos.z;

	return bone_pos;
}

typedef struct Bone
{
	uint8_t pad1[0xCC];
	float x;
	uint8_t pad2[0xC];
	float y;
	uint8_t pad3[0xC];
	float z;
}Bone;

vec3_t get_bone_pos_new(uintptr_t entity, vec3_t origin_pos, int id) {
	uintptr_t bones_array = Read<uintptr_t>(entity + 0x0e98 + 0x48);   //m_nForceBone + 0x48
	vec3_t bone_pos;

	uintptr_t boneloc = (id * 0x30);
	Bone bo = {};
	bo = Read<Bone>(bones_array + boneloc);
	bone_pos.x = bo.x + origin_pos.x;
	bone_pos.y = bo.y + origin_pos.y;
	bone_pos.z = bo.z + origin_pos.z;
	return bone_pos;
}

//Doesnt work somehow //Maybe offset need to update
vec3_t GetBonePosByHitBox(uintptr_t entity, vec3_t origin_pos, int id)
{
	uint64_t Model = Read<uint64_t>(entity + 0x10f0);   //CBaseAnimating!m_pStudioHdr
	uint64_t StudioHdr = Read<uint64_t>(Model + 0x8);
	uint16_t HitboxCache = Read<uint16_t>(StudioHdr + 0x34);
	uint64_t HitBoxsArray = StudioHdr + ((uint64_t)(HitboxCache & 0xFFFE) << (4 * (HitboxCache & 1)));
	uint16_t IndexCache = Read<uint16_t>(HitBoxsArray + 0x4);
	int HitboxIndex = ((uint16_t)(IndexCache & 0xFFFE) << (4 * (IndexCache & 1)));
	uint16_t Bone = Read<uint16_t>(HitBoxsArray + HitboxIndex + (id * 0x20));
	std::cout << Bone << std::endl;
	if (Bone < 0 || Bone > 255)
		return vec3_t(0, 0, 0);

	uintptr_t BoneArray = Read<uintptr_t>(entity + 0x0e98 + 0x48);

	matrix_t Matrix = {};
	Matrix = Read<matrix_t>(BoneArray + Bone * sizeof(matrix_t));

	return vec3_t(Matrix.mat_val[0][3] + origin_pos.x, Matrix.mat_val[1][3] + origin_pos.y, Matrix.mat_val[2][3] + origin_pos.z);
}

bool world_to_screen(float* view_matrix, vec3_t world, vec2_t& screen) {
	float* m_vMatrix = view_matrix;
	float w = m_vMatrix[12] * world.x + m_vMatrix[13] * world.y + m_vMatrix[14] * world.z + m_vMatrix[15];

	if (w < 0.01f)
		return false;

	screen.x = m_vMatrix[0] * world.x + m_vMatrix[1] * world.y + m_vMatrix[2] * world.z + m_vMatrix[3];
	screen.y = m_vMatrix[4] * world.x + m_vMatrix[5] * world.y + m_vMatrix[6] * world.z + m_vMatrix[7];

	float invw = 1.0f / w;
	screen.x *= invw;
	screen.y *= invw;

	float x = 1920 / 2;
	float y = 1080 / 2;

	x += 0.5 * screen.x * 1920 + 0.5;
	y -= 0.5 * screen.y * 1080 + 0.5;

	screen.x = x;
	screen.y = y;

	if (screen.x > 1920 || screen.x < 0 || screen.y > 1080 || screen.y < 0)
		return false;

	return true;
}

enum ShootMode
{
	None,
	Energency,     // < 8
	Close,         // < 30
	Middle,        // < 80
	Far            // >= 80
};

uintptr_t FindLeastAngleEntity(std::list<EntityData> datas)
{
	float dist_debug = 0;
	float angle_debug = 0;
	float angle = 1;
	uintptr_t entity = 0;
	if (datas.size() == 0) return entity;
	std::list<EntityData>::iterator itr;
	for (itr = datas.begin(); itr != datas.end(); ++itr)
	{
		if (itr->angle < angle)
		{
			angle = itr->angle;
			entity = itr->entity;
			dist_debug = itr->dist;
			angle_debug = itr->angle;
		}
	}
	std::cout << "dist" << dist_debug << std::endl;
	std::cout << "dist" << angle_debug << std::endl;
	return entity;
}

uintptr_t LockTarget(std::list<EntityData> datas)
{
	float enmergency_angle = 0.13f;  // 30Åã
	float enmergency_dist = 20;

	float close_angle = 0.061f;   // 20Åã
	float close_dist = 40;

	float far_angle = 0.022f;   // 12Åã

	float middle_angle = 0.034f;   // 15Åã
	float middle_dist = 80;

	if (datas.size() == 0)
		return 0;

	std::list<EntityData>::iterator itr;
	std::list<EntityData> temp;
	ShootMode mode = None;

	//for (const EntityData& elem : temp)   //range based loop
	//{
	//	if (temp->dist < 0)
	//}

	for (itr = datas.begin(); itr != datas.end(); ++itr)
	{
		if (itr->dist < enmergency_dist && itr->angle < enmergency_angle)   // < 8
		{
			temp.push_back(*itr);
			mode = Energency;
		}
	}

	if (mode == Energency)
	{
		uintptr_t optimal_entity = FindLeastAngleEntity(temp);
		if (optimal_entity)
		{
			std::cout << "Energency  " << std::endl;
			return FindLeastAngleEntity(temp);
		}
	}

	temp.clear();

	for (itr = datas.begin(); itr != datas.end(); ++itr)
	{
		if (itr->dist < close_dist && itr->angle < close_angle)   // < 30
		{
			temp.push_back(*itr);
			mode = Close;
		}
	}

	if (mode == Close)
	{
		uintptr_t optimal_entity = FindLeastAngleEntity(temp);
		if (optimal_entity)
		{
			std::cout << "Close  " << std::endl;
			return FindLeastAngleEntity(temp);
		}
	}

	temp.clear();

	for (itr = datas.begin(); itr != datas.end(); ++itr)   // >80
	{
		int exponential = (int)(itr->dist / middle_dist);
		exponential = std::clamp(exponential, 1 , 1);
		if (itr->angle < far_angle && itr->dist > middle_dist )
		{
			temp.push_back(*itr);
			mode = Far;
		}
	}

	if (mode == Far)
	{
		uintptr_t optimal_entity = FindLeastAngleEntity(temp);
		if (optimal_entity)
		{
			std::cout << "Far  " << std::endl;
			return FindLeastAngleEntity(temp);
		}
	}

	temp.clear();

	for (itr = datas.begin(); itr != datas.end(); ++itr)   // <80
	{
		if (itr->angle < middle_angle && itr->dist <= middle_dist)
		{
			temp.push_back(*itr);
			mode = Middle;
		}
	}

	if (mode == Middle)
	{
		uintptr_t optimal_entity = FindLeastAngleEntity(temp);
		if (optimal_entity)
		{
			std::cout << "Middle  " << std::endl;
			return FindLeastAngleEntity(temp);
		}
	}
	return 0;
}

vec3_t VectorToAngle(vec3_t src, vec3_t dst)
{
	vec3_t offset = src - dst;

	double temp = sqrt(pow(offset.x, 2) + pow(offset.y, 2));

	float x = atan(offset.z / temp) * (180.0f / M_PI);
	float y = atan(offset.y / offset.x) * (180.0f / M_PI);

	if (offset.x >= 0.0)
	{
		if (y < 0)
		{
			y += 180.0f;
		}
		else
		{
			y -= 180.0f;
		}
	}
	return vec3_t(x, y, 0);
}

vec3_t AngleToVector(vec3_t angle)
{
	/*float x = 1, y = 0, z = 0;
	float degree_axis_z = angle.y * PI / 180;
	float degree_axis_y = angle.x * PI / 180;
	x = cos(degree_axis_z) * x - sin(degree_axis_z) * y;
	y = sin(degree_axis_z) * x + cos(degree_axis_z) * y;
	z = z;
	x = cos(degree_axis_y) * x + sin(degree_axis_y) * z;
	y = y;
	z = -sin(degree_axis_y) * x + cos(degree_axis_y) * z;*/

	float angle_xy = angle.y * M_PI / 180;
	float x = cos(angle_xy);
	float y = sin(angle_xy);
	float z = -tan(angle.x * M_PI / 180);

	return vec3_t(x, y, z);
}

bool OptimalPitch(float x, float y, float v, float g, float* out_pitch)
{
	float root = v * v * v * v - g * (g * x * x + 2.0f * y * v * v);
	if (root >= 0.f)
	{
		*out_pitch = atan((v * v - sqrt(root)) / (g * x));
		return true;
	}
	return false;
}

bool ComputeTrajectory(vec3_t offset, float v, float g, float* travel_time, vec3_t* out_angle)
{
	float x = sqrt(offset.x * offset.x + offset.y * offset.y);
	float y = offset.z;
	float current_pitch = 0;
	if (!OptimalPitch(x, y, v, g, &current_pitch))
	{
		return false;
	}

	*travel_time = x / (cos(current_pitch) * v);
	*out_angle = vec3_t(current_pitch, atan2(y, x), 0);
	return true;
}

float GetTimeStepRateByDistance(int rate_temp)
{
	float rate = 1;
	if (rate_temp > 0)
	{
		if (rate_temp < 2)
		{
			rate = 0.25f;
		}
		if (rate_temp >= 2 && rate_temp < 8)
		{
			rate = 0.5f;
		}
		if (rate_temp >= 8 && rate_temp < 16)
		{
			rate = 1.0f;
		}
		if (rate_temp >= 16 && rate_temp < 32)
		{
			rate = 4.0f;
		}
		if (rate_temp >= 32 && rate_temp < 64)
		{
			rate = 8.0f;
		}
	}
	return rate;
}

bool calculate_predict_angle(vec3_t src, vec3_t dst, vec3_t target_v, float v, float g, vec3_t* aim_angle)
{
	float time = dst.distance_to(src) / v;
	float min_time = 0;
	float max_time = 0;
	float temp = dst.distance_to(src) - target_v.length();
	int rate_temp = dst.distance_to(src) / target_v.length();   //range is between 7 and 100 tested
	float rate = GetTimeStepRateByDistance(rate_temp);
	if (rate == 0) rate = 1;
	if (temp > 0) min_time = temp / v;
	max_time = 2.0f * time;
	if (max_time < 0.01f) max_time = 0.01f;
	float time_step = 1.f / 256.f / rate;
	for (float t = min_time; t <= max_time; t += time_step)
	{
		vec3_t target_pos = dst + target_v * t;
		vec3_t offset = target_pos - src;
		float travel_time;
		vec3_t out_angle;
		if (!ComputeTrajectory(offset, v, g, &travel_time, &out_angle))
		{
			return false;
		}
		
		if (travel_time < t)
		{
			vec3_t angle = VectorToAngle(src, target_pos);
			*aim_angle = vec3_t(-out_angle.x * (180.0f / M_PI), angle.y, 0);
			return true;
		}
	}
	return false;
}

bool aim = false;
std::list<Entity_Data> stored_entities;
uintptr_t locked_target = 0;
uintptr_t local_player = 0;
float smoothTime = 0.001F;
vec3_t velocity = vec3_t(0, 0, 0);
bool no_recoil = true;

void Reset()
{
	aim = false;
	locked_target = 0;
	stored_entities.clear();
	//velocity = vec3_t(0, 0, 0);
}

bool SearchForEntities()
{
	Reset();
	for (int i = 0; i < 100; i++)
	{
		uintptr_t entity_list = base_address + 0x1b2a578;
		uint64_t entity = Read<uint64_t>(entity_list + ((uint64_t)i << 5));
		if (entity == 0)
			continue;
		if (Read<uint64_t>(entity + 0x0589) != 125780153691248)       //m_iname
			continue;
		if (entity == local_player)
			continue;
		if (Read<int>(entity + 0x044c) == Read<int>(local_player + 0x044c))   //m_iTeamNum
			continue;
		if (!((Read<int>(entity + 0x0798)) == 0) || Read<int>(entity + 0x2738) > 0)   //m_lifeState, >0 = dead , m_bleedoutState, >0 = knocked
			continue;

		vec3_t local_origin = Read<vec3_t>(local_player + 0x014c);   //m_vecAbsOrigin
		vec3_t entity_origin = Read<vec3_t>(entity + 0x014c);
		float dist = local_origin.distance_to(entity_origin);
		if (dist * 0.01905f > 400)
			continue;

		//vec3_t camera_origin = Read<vec3_t>(local_player + 0x1f48);   //CPlayer!camera_origin
		//vec3_t aim_angle = GetAngleBetween(camera_origin, entity_origin);
		vec3_t aim_angle = VectorToAngle(local_origin, entity_origin);
		vec3_t view_angle = Read<vec3_t>(local_player + 0x25a4 - 0x14);   //m_ammoPoolCapacity - 0x14
		vec3_t aim_dir = entity_origin - local_origin;
		vec3_t view_dir = AngleToVector(view_angle);
		float angle_offset = aim_dir.normalized().dot(view_dir.normalized()); //cos a
		float angle_min = abs(angle_offset - 1);
		if (angle_min < 0.23f)   //about 40Å⁄
		{
			Entity_Data data;
			data.entity = entity;
			data.angle = angle_min;
			data.dist = dist * 0.01905f;
			stored_entities.push_back(data);
		}
	}
	if (stored_entities.size() == 0)
	{
		//std::cout << std::dec << "No entities found" << std::endl;
		return false;
	}
	return true;
}

int main()
{
	LoadLibrary("user32.dll");
	//process_id = get_process_id("msedge.exe");
	//std::cout << "prcoess_id :" << process_id << std::endl;
	//base_address = (UINT_PTR)get_module_base_address("msedge.exe");
	//std::cout << std::dec << "base adress:" << std::hex << base_address << std::endl;
	//Sleep(50000);     //Test use
	process_id = get_process_id("r5apex.exe");
	std::cout << "prcoess_id :" << process_id << std::endl;
	base_address = (UINT_PTR)get_module_base_address("r5apex.exe");
	std::cout << std::dec << "base adress:" << std::hex << base_address << std::endl;
	if (!base_address) 
		std::cout << std::dec << "Base adress not found" << std::endl;

	while (true)
	{
		if (!base_address)
			continue;
		
		if (GetAsyncKeyState(VK_LMENU) & 1)
		{
			std::cout << "turn on" << std::endl;
			aim = !aim;
		}

		if (aim)
		{
			local_player = FindLocalPlayer(base_address, 0x1EDB670, 0x8);
			if (!local_player)
			{
				Reset();
				continue;
			}

			/*uintptr_t latest_primary_weapons = Read<int>(local_player + 0x1a14);   //m_latestPrimaryWeapons
			latest_primary_weapons &= 0xffff;
			
			uintptr_t entity_list = base_address + 0x1b2a578;   //cl_entitylist
			uintptr_t weapon_entity = Read<uintptr_t>(entity_list + (latest_primary_weapons << 5));
			//std::cout << std::hex << weapon_entity << std::endl;
			float projectile_speed = Read<float>(weapon_entity + 0x1ef0);   //CWeaponX!m_flProjectileSpeed
			float projectile_scale = Read<float>(weapon_entity + 0x1ef8);   //CWeaponX!m_flProjectileScale
			float projectile_gravity = 750.0f * projectile_scale;
			float zoom_fov = Read<float>(weapon_entity + 0x16b8 + 0x00b8);   //m_playerData + m_curZoomFOV
			*/

			if (!SearchForEntities())
				continue;

			locked_target = LockTarget(stored_entities);
			if (!locked_target)
			{
				Reset();
				std::cout << std::dec << "No target to lock" << std::endl;
				continue;
			}

			while (true)
			{
				if (!local_player)
				{
					Reset();
					break;
				}
				if (!((Read<int>(local_player + 0x0798)) == 0) || Read<int>(local_player + 0x2738) > 0)
				{
					Reset();
					break;
				}

				vec3_t locked_target_origin = Read<vec3_t>(locked_target + 0x014c);
				vec3_t head_pos = get_bone_pos_new(locked_target, locked_target_origin, 8);
				vec3_t camera_origin = Read<vec3_t>(local_player + 0x1f48);
				//aim angle1
				vec3_t aim_angle = VectorToAngle(camera_origin, head_pos);
	
				vec3_t view_angle = Read<vec3_t>(local_player + 0x25a4 - 0x14);
				vec3_t sway_angle = Read<vec3_t>(local_player + 0x25a4 - 0x14 - 0x10);   //OFFSET_VIEWANGLES - 0x10 (m_ammoPoolCapacity - 0x14 = OFFSET_VIEWANGLES)
				//aim angle2
				uintptr_t latest_primary_weapons = Read<int>(local_player + 0x1a14);   //m_latestPrimaryWeapons
				latest_primary_weapons &= 0xffff;
				uintptr_t entity_list = base_address + 0x1b2a578;   //cl_entitylist
				uintptr_t weapon_entity = Read<uintptr_t>(entity_list + (latest_primary_weapons << 5));
				float projectile_speed = Read<float>(weapon_entity + 0x1ef0);   //CWeaponX!m_flProjectileSpeed
				if (projectile_speed != 1 && camera_origin.distance_to(head_pos) * 0.01905f > 30)
				{
					float projectile_scale = Read<float>(weapon_entity + 0x1ef8);   //CWeaponX!m_flProjectileScale
					float projectile_gravity = 750.0f * projectile_scale;
					//projectile_speed = projectile_speed - (projectile_speed * 0.08);
					//projectile_gravity = projectile_gravity + (projectile_gravity * 0.05);
					vec3_t abs_velocity = Read<vec3_t>(locked_target + 0x140);   //m_vecAbsVelocity
					vec3_t out_angle;
					if (calculate_predict_angle(camera_origin, head_pos, abs_velocity, projectile_speed, projectile_gravity, &out_angle))
					{
						aim_angle = out_angle;
					}
					else {
						std::cout << " predict false" << std::endl;
					}
				}
				//Add
				if (camera_origin.distance_to(head_pos) * 0.01905f <= 25)
				{
					vec3_t body_pos = get_bone_pos_new(locked_target, locked_target_origin, 2);
					aim_angle = VectorToAngle(camera_origin, body_pos);
				}
				//Add
				if (no_recoil)
					aim_angle -= sway_angle - view_angle;

				vec3_t smooth_angle = view_angle.SmoothDamp(view_angle, aim_angle, velocity, smoothTime, 0.001f);
				Write(local_player + 0x25a4 - 0x14, smooth_angle);

				if (!((Read<int>(locked_target + 0x0798)) == 0) || Read<int>(locked_target + 0x2738) > 0)   //m_lifeState, >0 = dead , m_bleedoutState, >0 = knocked
				{
					Sleep(500);
					if (!SearchForEntities())
					{
						Reset();
						std::cout << "turn off" << std::endl;
						break;
					}
					else
					{
						locked_target = LockTarget(stored_entities);
						if (!locked_target)
						{
							Reset();
							std::cout << "turn off" << std::endl;
							break;
						}
					}
				}
				if (GetAsyncKeyState(VK_LMENU) & 1)
				{
					Reset();
					std::cout << "turn off" << std::endl;
					break;
				}
				Sleep(1);
			}
		}
		Sleep(50);
	}
	/*//uintptr_t view_renderer = Read<uintptr_t>(base_address + 0x7664e80);   //ViewRender
//uintptr_t view_matrix_ = Read<uintptr_t>(base_address + 0x11a210);   //ViewMatrix
//view_matrix_t view_matrix = Read<view_matrix_t>(view_matrix_);

//vec3_t head_pos = get_bone_pos(entity, entity_origin, 8);
//vec2_t head_pos_2d{};

/*if (!world_to_screen(view_matrix.matrix, head_pos, head_pos_2d))
	continue;*/
	//while (true)
	//{
	//	draw_box();
	//}

	Sleep(50000);
	return 0;
}
