#include <spdlog/spdlog.h>

//#inlcude "YourMod.hpp"
#include "mods/BackgroundRendering.hpp"
#include "mods/FollowCamera.hpp"
#include "Mods.hpp"

Mods::Mods()
{

    //m_mods.emplace_back(std::make_unique<YourMod>());
	m_mods.emplace_back(std::make_unique<BackgroundRendering>());
	m_mods.emplace_back(std::make_unique<FollowCamera>());
#ifdef DEVELOPER
    m_mods.emplace_back(std::make_unique<DeveloperTools>());
#endif
}

std::optional<std::string> Mods::on_initialize() const {
    for (auto& mod : m_mods) {
        spdlog::info("{:s}::on_initialize()", mod->get_name().data());

        if (auto e = mod->on_initialize(); e != std::nullopt) {
            spdlog::info("{:s}::on_initialize() has failed: {:s}", mod->get_name().data(), *e);
            return e;
        }
    }

    utility::Config cfg{ "re2_fw_config.txt" };

    for (auto& mod : m_mods) {
        spdlog::info("{:s}::on_config_load()", mod->get_name().data());
        mod->on_config_load(cfg);
    }

    return std::nullopt;
}

void Mods::on_frame() const {
    for (auto& mod : m_mods) {
        mod->on_frame();
    }
}

void Mods::on_draw_ui() const {
    for (auto& mod : m_mods) {
        mod->on_draw_ui();
    }
}

