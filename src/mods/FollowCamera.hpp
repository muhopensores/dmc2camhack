#pragma once
#include "Mod.hpp"
#include "sdk/ReClass.hpp"
#include "utility/Patch.hpp"
class FollowCamera : public Mod {
public:
	FollowCamera() = default;
  // mod name string for config
  std::string_view get_name() const override { return "FollowCamera"; }
  // called by m_mods->init() you'd want to override this
  std::optional<std::string> on_initialize() override;

  // Override this things if you want to store values in the config file
  //void on_config_load(const utility::Config& cfg) override;
  //void on_config_save(utility::Config& cfg) override;

  // on_frame() is called every frame regardless whether the gui shows up.
  void on_frame() override;
  // on_draw_ui() is called only when the gui shows up
  // you are in the imgui window here.
  void on_draw_ui() override;
  // on_draw_debug_ui() is called when debug window shows up
  //void on_draw_debug_ui() override;
  std::unique_ptr<Patch> m_patch01;
  std::unique_ptr<Patch> m_patch02;
  std::unique_ptr<Patch> m_patch03_lkat;
  std::unique_ptr<Patch> m_patch04_cp;
  std::unique_ptr<Patch> m_patch04_lkat_fc;

  float m_pitch, m_yaw;
  
private:

	//some cam stuff
	float camera_heading;
	float camera_pitch;
	float camera_scale = .5f;

	glm::vec3 camera_direction;
	glm::vec3 camera_position_delta{ 0.0f, 0.0f, 0.0f};
	glm::vec3 camera_position;
	
	bool m_freecam = false;

	glm::vec4 m_fc_pos_backup;
	glm::vec4 m_fc_lkat_backup;

	const ModSlider::Ptr m_cam_range_min { ModSlider::create(generate_name("CameraRangeMin"), 0.0f, 1000.0f, 375.0f) };
	const ModSlider::Ptr m_cam_range_max { ModSlider::create(generate_name("CameraRangeMax"), 0.0f, 1000.0f, 480.0f) };
	const ModSlider::Ptr m_fov_override  { ModSlider::create(generate_name("FOV"), 0.0f, 3.0f, 0.96f) };
	const ModSlider::Ptr m_xrot_mul{ ModSlider::create(generate_name("CameraXrotationMultiplier"), 0.1f, 3.0f, 1.0f) };
	const ModSlider::Ptr m_yrot_mul{ ModSlider::create(generate_name("CameraYrotationMultiplier"), 0.1f, 3.0f, 1.0f) };
	const ModSlider::Ptr m_cam_height{ ModSlider::create(generate_name("CamHeight"), -1.0f, 1000.0f , 260.0f) };
  // function hook instance for our detour, convinient wrapper 
  // around minhook
  // std::unique_ptr<FunctionHook> m_function_hook;
};