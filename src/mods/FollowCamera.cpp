#include "FollowCamera.hpp"
#pragma comment(lib, "Winmm.lib")
#include <numeric>
#include "glm/gtx/polar_coordinates.hpp"


#define		GAME_WORK_SIZE			0x0200
#define sint8  int8_t
#define sint16 int16_t
#define uint8  uint8_t
#define uint16 uint16_t
#define sint32 int32_t
#define uint32 uint32_t
#define FVECTOR glm::vec4
#define float32 float
typedef struct
{
	sint8			game_mode ;				// 0x000
	sint8			select_pl ;				// 0x001

	sint16			main_timer ;			// 0x002

	float32			env_speed[2] ;			// 0x004

	sint8			difficulty ;			// 0x00C
	sint8			no_proc_flag ;			// 0x00D	bit 0:move 1:trans
	sint8			stage_clear_flag ;		// 0x00E
	sint8			work_keep_flag ;		// 0x00F

	sint8			mission_idx ;			// 0x010
	uint8			mission_no ;			// 0x011
	sint8			restart_no ;			// 0x012
	sint8			em_move_num ;			// 0x013

	sint8			equipment_flag ;		// 0x014
	sint8			cockpit_flag ;			// 0x015
	sint8			no_input_flag ;			// 0x016
	sint8			event_flag ;			// 0x017
	sint16			stage_no ;				// 0x018
	sint8			stage_hit_no ;			// 0x01A
	sint8			mini_demo_flag ;		// 0x01B

	uint32			keep_event_flag[8] ;	// 0x01C

	uint16			event_demo_no ;			// 0x03C
	uint8			em_push_off;			// 0x03E
	uint8			bike_stage_flag ;		// 0x03F

	sint8			bgm_flag ;				// 0x040
	sint8			bgm_no_chg ;			// 0x041
	sint16			bgm_timer ;				// 0x042
	sint8			stg_bgm_no[2] ;			// 0x044
	sint8			btl_bgm_no[2] ;			// 0x046
	uint16			se_reverb ;				// 0x048

	sint8			em_se_port[4] ;			// 0x04A

	uint8			fluid_flag ;			// 0x04E
	uint8			fluid_type ;			// 0x04F

	FVECTOR			fluid_pos ;				// 0x050
	FVECTOR			fluid_vec ;				// 0x060

	uint32			total_play_time ;		// 0x070
	uint32			now_play_time ;			// 0x074

	uint32			total_red_orb ;			// 0x078
	uint32			get_red_orb ;			// 0x07C

	uint8			item_num[0x40] ;		// 0x080

	uint16			stylish_result_cnt ;	// 0x0C0
	uint16			stylish_result_add ;	// 0x0C2
	uint32			damage_total ;			// 0x0C4
	sint32			use_item_point ;		// 0x0C8

	uint8			shop_item_level[12] ;	// 0x0CC

	uint8			killed_enemy[8] ;		// 0x0D8

	uint8			pl_init_flag ;			// 0x0E0
	uint8			no_disp_flag ;			// 0x0E1
	uint8			mv_proc_type ;			// 0x0E2
	uint8			stg_light_type ;		// 0x0E3

	uint32			sfd_work_adrs ;			// 0x0E4
	sint8			sfd_play_no ;			// 0x0E8
	sint8			sfd_play_ok ;			// 0x0E9

	sint8			limit_timer_on ;		// 0x0EA
	sint8			ipu_movie_no ;			// 0x0EB

	uint16			sec_room_level ;		// 0x0EC
	uint8			sec_room_flag ;			// 0x0EE
	uint8			sec_room_no ;			// 0x0EF
	uint16			practice_level ;		// 0x0F0

	sint16			save_nando ;			// 0x0F2
	uint8			sec_room_free ;			// 0x0F4

	uint8			now_clear_rank ;		// 0x0F5
	uint8			iget_ok_flag ;			// 0x0F6
	uint8			slide_ok_flag ;			// 0x0F7

	void			*ipu_mov_adrs[4] ;		// 0x0F8

	float32			se_far ;				// 0x108

	sint8			load_em_no[4] ;			// 0x10c
	uint8			em_ctrl_flag[8] ;		// 0x110

	uint16			omake_open_flag ;		// 0x118
	uint16			practice_mode ;			// 0x11a

	sint8			restart_flag ;			// 0x11c
	sint8			continue_flag ;			// 0x11d

	uint8			map_disp_flag[16] ;		// 0x11e

	uint8			clear_flag ;			// 0x12e
	uint8			max_mission[3] ;		// 0x12f
	uint8			clear_rank[3][18] ;		// 0x132

	float32			up_fire_pos ;			// 0x16c

	void			*stg_tex_adrs[4] ;		// 0x170

	uint8			super_dante_flag ;		// 0x180

	sint8			game_free[ GAME_WORK_SIZE - 0x181 ];
} GAME_WORK;

GAME_WORK* GAME_WORK_PTR;

struct fake_arg {
	glm::vec4 m1;
	glm::vec4 m2;
	glm::vec4 m3;
	glm::vec4 m4;
};

enum COL_TYPE {
	NO = 0,
	CHECK_WALLS = 1,
	NO_WALLS = 2
};

//typedef HWND (WINAPI *GETFOCUS)(VOID);
typedef __int64 (__fastcall *TRACELINE)(fake_arg& a1, glm::vec4& srcVec, glm::vec4& dstVector, COL_TYPE ct);
TRACELINE fp_trace_line = NULL;

cameraStructSomething* p_cam;
gamepadStruct* p_gpad;

emStruct00* p_ems;

Vector3f* p_plr_fwv;
Vector3f* p_plr_fwv2;
float* p_ch; // follow camera height
float* p_c_dist_min;
float* p_c_dist_max;

glm::vec4* p_bossthing;
COL_TYPE ctype = NO_WALLS;
// clang-format off
// clang-format on

FollowCamera* g_fc_instance;

std::optional<std::string> FollowCamera::on_initialize() {
	g_fc_instance = this;
  uintptr_t base = g_framework->get_module().as<uintptr_t>();

  uintptr_t ptr_one = base + 0x470E8;
  m_patch01 = Patch::create_nop(ptr_one, 7, false);

  uintptr_t ptr_two = base + 0x470DD;
  m_patch02 = Patch::create_nop(ptr_two, 7, false);

  uintptr_t ptr_three = base + 0x49ED6;
  m_patch03_lkat = Patch::create_nop(ptr_three, 4, false);

  uintptr_t ptr_lkt_pos = base + 0x46CFD;
  m_patch04_lkat_fc = Patch::create_nop(ptr_lkt_pos, 4, false);

  uintptr_t ptr_cam_pos = base + 0x46CF2;
  m_patch04_cp = Patch::create_nop(ptr_cam_pos, 3, false);

  GAME_WORK_PTR = (GAME_WORK*)(base + 0x1588B30);

  //dmc2.exe+5E8F0 - latest update exe
  fp_trace_line = (TRACELINE)(base + 0x5E8F0);

  p_cam = (cameraStructSomething*)(base + 0x158AB30);
  p_gpad = (gamepadStruct*)(base + 0x157D4F4);

  p_ch = (float*)(base + 0x5A1404);
  p_c_dist_min = (float*)(base + 0x5A1410);
  p_c_dist_max = (float*)(base + 0x4BCFC8);

  p_plr_fwv = (Vector3f*)(base + 0x158A400);
  p_plr_fwv2 = (Vector3f*)(base + 0x158A3E0);

  p_ems = (emStruct00*)(base + 0x158A330);//(base + 0x158A818);

  p_bossthing = (glm::vec4*)(base + 0x158A830);
  // some static variables are in read only memory pages
  // fucking gross but i'm not fucking with clang/icl again to make some detours
  // hope it wont crash for others
  DWORD dist_min_oldprotect;
  VirtualProtect(p_c_dist_min, sizeof(float), PAGE_READWRITE, &dist_min_oldprotect);
  
  DWORD dist_max_oldprotect;
  VirtualProtect(p_c_dist_max, sizeof(float), PAGE_READWRITE, &dist_max_oldprotect);
  
  DWORD ch_oldprotect;
  VirtualProtect(p_ch, sizeof(float), PAGE_READWRITE, &ch_oldprotect);

  return Mod::on_initialize();
}

// during load
//void ModSample::on_config_load(const utility::Config &cfg) {}
// during save
//void ModSample::on_config_save(utility::Config &cfg) {}
// do something every frame
bool force_follow_cam;
bool fly_forward;
glm::vec4 dp{ 0.0f, 0.0f, 0.0f, 0.0f };

Vector4f CameraRotate(float percent) {
	Vector3f CameraPos = p_cam->position;
	Vector3f TargetPos = p_cam->lookAt;
	Vector3f diff = CameraPos - TargetPos;

	Vector3f axis = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::quat quat = glm::angleAxis(percent, axis);

	Vector3f newVec = (quat * diff) + TargetPos;
	return glm::vec4(newVec.x,newVec.y,newVec.z,1.0f);
}

/*Vector4f CameraRotateX(float percent) {
	Vector3f CameraPos = p_cam->position;
	Vector3f TargetPos = p_cam->lookAt;
	Vector3f diff = CameraPos - TargetPos;

	Vector3f fw1 = *p_plr_fwv;//glm::vec3(0.0f, 0.0f, 1.0f);
	Vector3f fw2 = *p_plr_fwv2;//glm::vec3(0.0f, 0.0f, 1.0f);
	Vector3f dt = glm::lerp(fw2, fw1, glm::dot(glm::normalize(diff), glm::vec3(1.0f, 0.0f, 0.0f)));
	glm::quat quat = glm::angleAxis(percent, dt);

	Vector3f newVec = (quat * diff) + TargetPos;
	return glm::vec4(newVec.x,newVec.y,newVec.z,1.0f);
}*/
float percent = 0.5f;
float dy = 180.0f;
float mult = 1.2f;
bool bossfight = false;
#define BOSS_TARGET_SIZE 30

//glm::vec4 prev_em_pos;

std::array<glm::vec4, BOSS_TARGET_SIZE> prev_em_pos;
int index;
glm::vec4 g_diff;
glm::vec3 up = { 0.0f, 1.0f, 0.0f};

float m_dist_low = 600.0f;
float m_dist_hig = 800.0f;

float backup_dist_low = 375.0f;
float backup_dist_high = 480.0f;

float g_dot = 0.0f;

float ldist_double;
float hdist_double;

float min_smoothstep_distance = 1.0f;
float max_smoothstep_distance = 1000.0f;
float max_cam_height = 900.0f;

bool g_dynamic_distance = false;
bool dumbfix = false;
bool g_turn_lockon = false;


void lock_on_target() {
	if (!p_ems) { return; }
	Vector4f pl_pos = p_ems->plrCoords;
	pl_pos.y += dy;
	if ((!p_ems->emStruct01Ptr) || (!p_ems->emStruct01Ptr->emDataPtr)) { 
		//p_cam->pos_copy = pl_pos;
		if (g_dynamic_distance) {
			*p_c_dist_min = backup_dist_low;
			*p_c_dist_max = backup_dist_high;
		}
		g_fc_instance->m_patch03_lkat->disable();
		return; 
	}
	g_fc_instance->m_patch03_lkat->enable();
	//glm::vec4* p_cp = !bossfight ? &p_cam->someFltPos1 : &p_cam->N00000BC4;
	//glm::vec4* p_lp = !bossfight ? &p_cam->someFltPos2 : &p_cam->N00000BBF;
	Vector4f cp_backup = p_cam->pos_copy;
	/*if (!p_ems->emStruct01Ptr->emDataPtr) { 

		return; 
	}*/

	Vector4f em_pos = bossfight ? 
		*p_bossthing : p_ems->emStruct01Ptr->emDataPtr->curPos;
	if (bossfight) {
		glm::vec4 damped_pos = std::accumulate(prev_em_pos.begin(), prev_em_pos.end(), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		damped_pos /= 30.0f;
		prev_em_pos[index++ % BOSS_TARGET_SIZE] = em_pos;
		em_pos = damped_pos;
	}
	Vector4f res = glm::lerp(pl_pos, em_pos, percent * mult);
	Vector4f diff = pl_pos - em_pos;

	if (g_dynamic_distance) {
		Vector3f diff3 = *(Vector3f*)&diff;

		float dot = glm::smoothstep(max_smoothstep_distance, min_smoothstep_distance, glm::length(diff3));//1000.0f / glm::length(diff3);//glm::dot(glm::normalize(diff3), up);
		g_dot = dot;
		float mindist = *p_c_dist_min;
		float maxdist = *p_c_dist_max;
		float nd = glm::lerp(375.0f, m_dist_low, dot);
		//*p_c_dist_min += m_dist;

		float hd = glm::lerp(480.0f, m_dist_hig, dot);

		if (dumbfix) {
			float wew = glm::lerp(260.0f, max_cam_height, dot);
			*p_ch = wew;
		}

		//TODO REMOVE
		ldist_double = nd;//*p_c_dist_min;
		*p_c_dist_min = nd;
		*p_c_dist_max = hd;
		hdist_double = hd;//*p_c_dist_max;
		//*p_c_dist_max += h_dist;
	}

	diff /= glm::length(diff);
	//Vector4f direction = diff / glm::length(diff);
	//direction *= (*p_c_dist_min + *p_c_dist_max) / 2.0f;
	//cp_backup = pl_pos + direction;
	//cp_backup.y = pl_pos.y + dy;
	diff = glm::normalize(diff);
	diff *= *p_c_dist_max;
	diff += pl_pos;
	//diff += pl_pos + direction;
	//diff.y = pl_pos.y + dy;
	g_diff = diff;
	Vector4f cp = glm::lerp(cp_backup, diff, percent);
	p_cam->lkat_copy = res;
	p_cam->pos_copy = cp;
	//*p_cp = res;
	//*p_lp = cp;
}
__int64 check_col() {
	Vector4f CameraPos = p_cam->position;
	Vector4f TargetPos = p_cam->lookAt;
	Vector4f diff = CameraPos - TargetPos;
	diff.w = 1.0f;

	alignas(16) Vector4f cur_pos = TargetPos;
	Vector4f dp = glm::normalize(diff);
	dp *= 10.0f;
	dp.w = 1.0f;

	fake_arg fake{};
	alignas(16) Vector4f next_pos = CameraPos;
	auto res = fp_trace_line(fake, cur_pos, next_pos, ctype);
	if (res) {
		p_cam->pos_copy = fake.m3;
		p_cam->position = fake.m3;
	}
	return res;
}
__int64 collision = false;
float rx_flt = 0.0f;
float ry_flt = 0.0f;

float d_percent = 0.02f;

bool mouselook = false;

float rotation_gain = 0.01f;
float distance = 500.0f;

glm::vec3 camera_up = { 0.0f, 1.0f, 0.0f };


void ChangePitch(float degrees, float* camera_pitch) {
	//Check bounds with the max pitch rate so that we aren't moving too fast
	float max_pitch_rate = 5;
	if (degrees < -max_pitch_rate) {
		degrees = -max_pitch_rate;
	}
	else if (degrees > max_pitch_rate) {
		degrees = max_pitch_rate;
	}

	*camera_pitch += degrees;

	//Check bounds for the camera pitch
	if (*camera_pitch > 360.0f) {
		*camera_pitch -= 360.0f;
	}
	else if (*camera_pitch < -360.0f) {
		*camera_pitch += 360.0f;
	}
}
void ChangeHeading(float degrees, float* camera_heading, float camera_pitch) {
	//Check bounds with the max heading rate so that we aren't moving too fast
	float max_heading_rate = 5;
	if (degrees < -max_heading_rate) {
		degrees = -max_heading_rate;
	}
	else if (degrees > max_heading_rate) {
		degrees = max_heading_rate;
	}
	//This controls how the heading is changed if the camera is pointed straight up or down
	//The heading delta direction changes
	if (camera_pitch > 90 && camera_pitch < 270 || (camera_pitch < -90 && camera_pitch > -270)) {
		*camera_heading -= degrees;
	}
	else {
		*camera_heading += degrees;
	}
	//Check bounds for the camera heading
	if (*camera_heading > 360.0f) {
		*camera_heading -= 360.0f;
	}
	else if (*camera_heading < -360.0f) {
		*camera_heading += 360.0f;
	}
}


void FollowCamera::on_frame() {
	if (force_follow_cam) {
		
		p_cam->cameraByte = 0;//bossfight ? 2 : 0;
		
		char rx = p_gpad->rX - 127;
		char ry = p_gpad->rY - 127;

		rx_flt = (float)rx * 0.001f * m_xrot_mul->value();
		//ry_flt = (float)ry * 0.01f * m_yrot_mul->value();

		lock_on_target();

		

		collision = check_col();

		if (rx) {
			p_cam->pos_copy = CameraRotate(rx_flt);
		}
		if (p_gpad->r1) {
			if (percent < 0.5f) {
				percent += d_percent;
			}
		}
		else {
			if (!g_turn_lockon) {
				if (percent > 0.0f) {
					percent -= d_percent;
					if (percent < 0.0f) {
						percent = 0.0f;
					}
				}
			}
			else {
				if (percent > 0.08f) {
					percent -= d_percent;
				}
			}
		}
		/*if (ry) {
			float cur_height = *p_ch;
			*p_ch = cur_height + ry_flt;
		}*/

	}

	if (mouselook) {
		auto mouse = ImGui::GetMouseDragDelta();
		if (mouse.x != 0.0f || mouse.y != 0.0f) {
			Vector3f delta = glm::vec3(float(mouse.x), float(mouse.y), 0.0f) * rotation_gain;

			m_pitch -= delta.y;
			m_yaw -= delta.x;

			float limit = glm::pi<float>() / 2.0f - 0.01f;

			m_pitch = std::max(-limit, m_pitch);
			m_pitch = std::min(+limit, m_pitch);

			if (m_yaw > glm::pi<float>()) {
				m_yaw -= glm::pi<float>() * 2.0f;
			}
			else if (m_yaw < -glm::pi<float>()) {
				m_yaw += glm::pi<float>() * 2.0f;
			}
			Vector3f euclidean = glm::euclidean(glm::vec2(m_pitch, m_yaw));
			euclidean *= distance;
			Vector4f result = Vector4f(euclidean.x, euclidean.y, euclidean.z, 1.0f);
			Vector4f pl_pos = p_ems->plrCoords;
			pl_pos.y += dy;
			result += pl_pos;
			p_cam->position = result;
		}
	}

	if (fly_forward) {
		fake_arg fake;
		glm::vec4 next_pos;
		next_pos = p_cam->position + dp;
		if (fp_trace_line(fake, next_pos,p_cam->position, ctype) == 3) {
			fly_forward = false;
		}
		p_cam->position += dp;

	}

	if (m_freecam) {
		camera_direction = glm::normalize(*(glm::vec3*)&p_cam->lookAt - *(glm::vec3*)&p_cam->position);
		float multiplier = (GetAsyncKeyState(VK_LSHIFT) & 0x01) ? 5.0f : 10.0f;
		//w
		if ((GetAsyncKeyState(0x57) & 0x01)) {
			camera_position_delta += camera_direction * multiplier;
		}
		//s
		if ((GetAsyncKeyState(0x53) & 0x01)) {
			camera_position_delta -= camera_direction * multiplier;
		}
		//a
		if ((GetAsyncKeyState(0x41) & 0x01)) {
			camera_position_delta -= glm::cross(camera_direction,camera_up) * multiplier;
		}
		//d
		if ((GetAsyncKeyState(0x44) & 0x01)) {
			camera_position_delta += glm::cross(camera_direction,camera_up) * multiplier;
		}
		//q
		if ((GetAsyncKeyState(0x51) & 0x01)) {
			camera_position_delta += camera_up * multiplier;
		}
		//e
		if ((GetAsyncKeyState(0x45) & 0x01)) {
			camera_position_delta -= camera_up * multiplier;
		}
		auto mouse = ImGui::GetMouseDragDelta();
		if (mouse.x != 0.0f || mouse.y != 0.0f) {
			Vector3f delta = glm::vec3(float(-mouse.x), float(-mouse.y), 0.0f) * rotation_gain;
			
			ChangeHeading(delta.x, &camera_heading, camera_pitch);
			ChangePitch(delta.y, &camera_pitch);
			
			/*float limit = glm::pi<float>() / 2.0f - 0.01f;

			m_pitch = std::max(-limit, m_pitch);
			m_pitch = std::min(+limit, m_pitch);

			if (m_yaw > glm::pi<float>()) {
				m_yaw -= glm::pi<float>() * 2.0f;
			}
			else if (m_yaw < -glm::pi<float>()) {
				m_yaw += glm::pi<float>() * 2.0f;
			}*/

			camera_direction = glm::normalize(*(glm::vec3*)&p_cam->lookAt - *(glm::vec3*)&p_cam->position);
			glm::vec3 axis = glm::cross(camera_direction, camera_up);

			glm::quat pitch_quat = glm::angleAxis(glm::radians(camera_pitch), axis);
			glm::quat heading_quat = glm::angleAxis(glm::radians(camera_heading), camera_up);

			glm::quat temp = glm::cross(pitch_quat, heading_quat);
			temp = glm::normalize(temp);
			camera_direction = temp * camera_direction;

		}
		p_cam->position += glm::vec4(camera_position_delta.x,camera_position_delta.y, camera_position_delta.z, 0.0f);
		glm::vec4 dir4f = glm::vec4(camera_position.x, camera_position.y, camera_position.z, 0.0f) +
			glm::vec4(camera_direction.x, camera_direction.y, camera_direction.z, 0.0f);
		dir4f *= 5.0f;
		p_cam->lookAt = p_cam->position + dir4f;

		camera_heading *= .5;
		camera_pitch *= .5;
		camera_position_delta = camera_position_delta * .8f;

		/*Vector4f direction = p_cam->lookAt - p_cam->position;
		direction = glm::normalize(direction);
		direction *= 10.0f;
		if ((GetAsyncKeyState(0x57) & 0x01)) {
			p_cam->position += direction;
			p_cam->lookAt   += direction;
		}*/


	}
	p_cam->anotherFoVprob = m_fov_override->value();

}
// will show up in debug window, dump ImGui widgets you want here
//void ModSample::on_draw_debug_ui() {}
// will show up in main window, dump ImGui widgets you want here
glm::vec4 p1{ 0.0f,0.0f,0.0f,1.0f };
glm::vec4 p2{ 0.0f,0.0f,0.0f,1.0f };
void FollowCamera::on_draw_ui() {
	ImGui::SetNextTreeNodeOpen(false, ImGuiCond_::ImGuiCond_FirstUseEver);

	if (!ImGui::CollapsingHeader(get_name().data())) {
		return;
	}
	//ImGui::Checkbox("FreeCam", &m_freecam);
	if (ImGui::Checkbox("FreeCam", &m_freecam)) {
		m_patch04_lkat_fc->toggle(m_freecam);
		m_patch04_cp->toggle(m_freecam);
		m_fc_pos_backup = p_cam->position;
		m_fc_lkat_backup = p_cam->lookAt;
		GAME_WORK_PTR->no_input_flag = 0;
	}
	int slt = GAME_WORK_PTR->stg_light_type;
	ImGui::InputInt("stg_light_type", &slt);
	GAME_WORK_PTR->stg_light_type = slt;
	ImGui::SliderFloat2("env_speed", GAME_WORK_PTR->env_speed, 0.0f, 1.0f);
	ImGui::Checkbox("practice_mode", (bool*)&GAME_WORK_PTR->practice_mode);
	ImGui::Checkbox("Cock flag:", (bool*)&GAME_WORK_PTR->cockpit_flag);
	ImGui::Checkbox("NoInp flag:", (bool*)&GAME_WORK_PTR->no_input_flag);
	ImGui::Checkbox("SuperDante flag:", (bool*)&GAME_WORK_PTR->super_dante_flag);
	ImGui::Checkbox("mouselook", &mouselook);
	if (mouselook) {
		ImGui::InputFloat("rotation_gain", &rotation_gain, 0.01f, 0.1f);
		ImGui::InputFloat("distance", &distance, 50.0f, 100.0f);
	}
	if (ImGui::Checkbox("followcam", &force_follow_cam)) {
		m_patch01->toggle(&force_follow_cam);
		m_patch02->toggle(&force_follow_cam);
		p_cam->position = p_cam->lookAt + glm::vec4{ 320.0f, 0.0f,0.0f,1.0f };
	}

	if (m_cam_range_min->draw("Cam. min. dist")) {
		*p_c_dist_min = m_cam_range_min->value();
		backup_dist_low = *p_c_dist_min;

	}
	if (m_cam_range_max->draw("Cam. max. dist")) {
		*p_c_dist_max = m_cam_range_max->value();
		backup_dist_high = *p_c_dist_max;
	}
	if (m_cam_height->draw("Cam. height")) {
		*p_ch = m_cam_height->value();
	}

	m_fov_override->draw("FOV override");

	m_xrot_mul->draw("Camera X rotation multiplier");
	ImGui::InputFloat("percent", &percent, 0.1f, 0.5f);
	ImGui::InputFloat("dy", &dy, 1.0f, 0.5f);
	ImGui::InputFloat("boss target mult", &mult, 0.1f, 2.0f);
	if (ImGui::Checkbox("Bossfight", &bossfight)) {
		if (bossfight) {
			up.y = -1.0f;
		}
		else {
			up.y = 1.0f;
		}
	}
	ImGui::InputInt("ColType", (int*)&ctype, 1, 1);
	ImGui::Text("0, no col, 1 plr col, 2 projectile col");
	ImGui::Checkbox("auto rotate to lockon", &g_turn_lockon);
	ImGui::Checkbox("Dynamic distance", &g_dynamic_distance);
	if (g_dynamic_distance) {
		ImGui::Checkbox("y_scaling", &dumbfix);
		ImGui::DragFloat("m_dist_low", &m_dist_low, 1.0f, 0.0f, 3000.0f);
		ImGui::DragFloat("m_dist_hig", &m_dist_hig, 1.0f, 0.0f, 3000.0f);

		ImGui::DragFloat("min_ss_dist", &min_smoothstep_distance, 1.0f, 0.0f, 2000.0f);
		ImGui::DragFloat("max_ss_dist", &max_smoothstep_distance, 1.0f, 0.0f, 2000.0f);
	}

	ImGui::Text("lookat: x=%f, y=%f, z= %f", p_cam->lkat_copy.x, p_cam->lkat_copy.y, p_cam->lkat_copy.z);
	ImGui::Text("pos: x=%f, y=%f, z= %f", p_cam->pos_copy.x, p_cam->pos_copy.y, p_cam->pos_copy.z);

	ImGui::Text("diff: x=%f, y=%f, z=%f", g_diff.x, g_diff.y, g_diff.z);
	ImGui::Text("dot: %f", g_dot);
	ImGui::Text("ldist= %f; hdist= %f", ldist_double, hdist_double);
	

	ImGui::Text("lookat: x=%f, y=%f, z= %f", p_cam->lkat_copy.x, p_cam->lkat_copy.y, p_cam->lkat_copy.z);
	ImGui::Text("pos: x=%f, y=%f, z= %f", p_cam->pos_copy.x, p_cam->pos_copy.y, p_cam->pos_copy.z);
	//m_yrot_mul->draw("Camera Y rotation multiplier");


	/*ImGui::Checkbox("fly", &fly_forward);
	ImGui::SliderFloat3("p1", (float*)&p1, -10000.0f, 10000.0f);
	ImGui::SliderFloat3("p2", (float*)&p2, -10000.0f, 10000.0f);
	ImGui::SliderFloat3("dp", (float*)&dp, -5.0f, 5.0f);*/

	/*ImGui::Text("rx: %f", rx_flt);
	if (collision) {
		ImGui::Button("COLLISION", ImVec2(125.0f, 25.0f));
		ImGui::Text("result: %d", collision);
	}*/

}