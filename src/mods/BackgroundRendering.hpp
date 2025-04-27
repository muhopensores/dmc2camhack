#pragma once
#include "Mod.hpp"

class BackgroundRendering : public Mod {
public:
	BackgroundRendering() = default;

	std::string_view get_name() const override { return "BackgroundRendering"; };

	std::optional<std::string> on_initialize() override;

	void on_config_load(const utility::Config& cfg) override;
	void on_config_save(utility::Config& cfg) override;

	void on_draw_ui() override;
};