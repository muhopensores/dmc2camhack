#pragma once

// Game can't use virtual keys unless the menu is open.
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include <vector>
#include <unordered_map>
#include <memory>

#include <imgui/imgui.h>

#include "sdk/ReClass.hpp"
#include "utility/Config.hpp"

#include "ModFramework.hpp"

class IModValue {
public:
    using Ptr = std::unique_ptr<IModValue>;

    virtual ~IModValue() {};
    virtual bool draw(std::string_view name) = 0;
    virtual void draw_value(std::string_view name) = 0;
    virtual void config_load(const utility::Config& cfg) = 0;
    virtual void config_save(utility::Config& cfg) = 0;
};

// Convenience classes for imgui
template <typename T>
class ModValue : public IModValue {
public:
    using Ptr = std::unique_ptr<ModValue<T>>;

    static auto create(std::string_view config_name, T default_value = T{}) {
        return std::make_unique<ModValue<T>>(config_name, default_value);
    }

    ModValue(std::string_view config_name, T default_value) 
        : m_config_name{ config_name },
        m_value{ default_value }
    {
    }

    virtual ~ModValue() override {};

    virtual void config_load(const utility::Config& cfg) override {
        auto v = cfg.get<T>(m_config_name);

        if (v) {
            m_value = *v;
        }
    };

    virtual void config_save(utility::Config& cfg) override {
        cfg.set<T>(m_config_name, m_value);
    };

    operator T&() {
        return m_value;
    }

    T& value() {
        return m_value;
    }

    const auto& get_config_name() const {
        return m_config_name;
    }

protected:
    T m_value{};
    std::string m_config_name{ "Default_ModValue" };
};

class ModToggle : public ModValue<bool> {
public:
    using Ptr = std::unique_ptr<ModToggle>;

    ModToggle(std::string_view config_name, bool default_value) 
        : ModValue<bool>{ config_name, default_value } 
    { 
    }

    static auto create(std::string_view config_name, bool default_value = false) {
        return std::make_unique<ModToggle>(config_name, default_value);
    }

    bool draw(std::string_view name) override {
        ImGui::PushID(this);
        auto ret = ImGui::Checkbox(name.data(), &m_value);
        ImGui::PopID();

        return ret;
    }

    void draw_value(std::string_view name) override {
        ImGui::Text("%s: %i", name.data(), m_value);
    }

    bool toggle() {
        return m_value = !m_value;
    }
};

class ModFloat : public ModValue<float> {
public:
    using Ptr = std::unique_ptr<ModFloat>;

    ModFloat(std::string_view config_name, float default_value) 
        : ModValue<float>{ config_name, default_value } { }

    static auto create(std::string_view config_name, float default_value = 0.0f) {
        return std::make_unique<ModFloat>(config_name, default_value);
    }

    bool draw(std::string_view name) override {
        ImGui::PushID(this);
        auto ret = ImGui::InputFloat(name.data(), &m_value);
        ImGui::PopID();

        return ret;
    }

    void draw_value(std::string_view name) override {
        ImGui::Text("%s: %f", name.data(), m_value);
    }
};

class ModSlider : public ModFloat {
public:
    using Ptr = std::unique_ptr<ModSlider>;

    static auto create(std::string_view config_name, float mn = 0.0f, float mx = 1.0f, float default_value = 0.0f) {
        return std::make_unique<ModSlider>(config_name, mn, mx, default_value);
    }

    ModSlider(std::string_view config_name, float mn = 0.0f, float mx = 1.0f, float default_value = 0.0f)
        : ModFloat{ config_name, default_value },
        m_range{ mn, mx }
    {
    }

    bool draw(std::string_view name) override {
        ImGui::PushID(this);
        auto ret = ImGui::SliderFloat(name.data(), &m_value, m_range.x, m_range.y);
        ImGui::PopID();

        return ret;
    }

    void draw_value(std::string_view name) override {
        ImGui::Text("%s: %f [%f, %f]", name.data(), m_value, m_range.x, m_range.y);
    }

    auto& range() {
        return m_range;
    }

protected:
    Vector2f m_range{ 0.0f, 1.0f };
};

class ModInt32 : public ModValue<int32_t> {
public:
    using Ptr = std::unique_ptr<ModInt32>;

    static auto create(std::string_view config_name, uint32_t default_value = 0) {
        return std::make_unique<ModInt32>(config_name, default_value);
    }

    ModInt32(std::string_view config_name, uint32_t default_value = 0)
        : ModValue{ config_name, static_cast<int>(default_value) }
    {
    }

    bool draw(std::string_view name) override {
        ImGui::PushID(this);
        auto ret = ImGui::InputInt(name.data(), &m_value);
        ImGui::PopID();

        return ret;
    }

    void draw_value(std::string_view name) override {
        ImGui::Text("%s: %i", name.data(), m_value);
    }
};

class ModKey: public ModInt32 {
public:
    using Ptr = std::unique_ptr<ModKey>;

    static auto create(std::string_view config_name, int32_t default_value = UNBOUND_KEY) {
        return std::make_unique<ModKey>(config_name, default_value);
    }

    ModKey(std::string_view config_name, int32_t default_value = UNBOUND_KEY)
        : ModInt32{ config_name, static_cast<uint32_t>(default_value) }
    {
    }

    bool draw(std::string_view name) override {
        if (name.empty()) {
            return false;
        }

        ImGui::PushID(this);
        ImGui::Button(name.data());

        if (ImGui::IsItemHovered()) {
            auto& keys = g_framework->get_keyboard_state();

            for (auto k = 0; k < keys.size(); ++k) {
                if (keys[k]) {
                    m_value = is_erase_key(k) ? UNBOUND_KEY : k;
                    break;
                }
            }

            ImGui::SameLine();
            ImGui::Text("Press any key");
        }
        else {
            ImGui::SameLine();

            if (m_value >= 0 && m_value <= 255) {
                ImGui::Text("%i", m_value);
            }
            else {
                ImGui::Text("Not bound");
            }
        }

        ImGui::PopID();

        return true;
    }

    bool is_key_down() const {
        if (m_value < 0 || m_value > 255) {
            return false;
        }

        return g_framework->get_keyboard_state()[(uint8_t)m_value] != 0;
    }

    bool is_key_down_once() {
        auto down = is_key_down();

        if (!m_was_key_down && down) {
            m_was_key_down = true;
            return true;
        }

        if (!down) {
            m_was_key_down = false;
        }

        return false;
    }

    bool is_erase_key(int k) const {
        switch (k) {
        case DIK_ESCAPE:
        case DIK_BACKSPACE:
            return true;

        default:
            return false;
        }
    }

    static constexpr int32_t UNBOUND_KEY = -1;

protected:
    bool m_was_key_down{ false };
};

class Mod {
protected:
    using ValueList = std::vector<std::reference_wrapper<IModValue>>;

public:
    virtual ~Mod() {};
    virtual std::string_view get_name() const { return "UnknownMod"; };

    // can be used for ModValues, like Mod_ValueName
    virtual std::string generate_name(std::string_view name) { return std::string{ get_name() } + "_" + name.data(); }

	bool install_hook(ptrdiff_t offset, std::unique_ptr<FunctionHook>& hook, void* detour, uintptr_t* ret, ptrdiff_t next_instruction_offset = 0) {
		uintptr_t base = g_framework->get_module().as<uintptr_t>();
		uintptr_t location = base + offset;
		// TODO(ce): enable logging
		//spdlog::info("installing hook at location={:x}", (void*)location);
		// TODO(ce): check in debug mode for installing multiple hooks with one instance
#ifdef _DEBUG
		if (hook) {
			throw std::runtime_error("Install hook called multiple times with same instance\n");
		}
		printf("Installing offset hook at location: %p\n", location);
#endif
		hook = std::make_unique<FunctionHook>(location, detour);
		if (!hook->create()) {
			spdlog::error("Failed to queue hook!");
			return false;
		}

		if (next_instruction_offset) {
			*ret = location + next_instruction_offset;
		}
		else {
			*ret = hook->get_original();
		}
		return true;
	}


    // Called when ModFramework::initialize finishes in the first render frame
    // Returns an error string if it fails
    virtual std::optional<std::string> on_initialize() { return std::nullopt; };

    // Functionally equivalent, but on_frame will always get called, on_draw_ui can be disabled by ModFramework
    virtual void on_frame() {};
    virtual void on_draw_ui() {};

    virtual void on_config_load(const utility::Config& cfg) {};
    virtual void on_config_save(utility::Config& cfg) {};

    // Game-specific callbacks
    virtual void on_pre_update_transform(RETransform* transform) {};
    virtual void on_update_transform(RETransform* transform) {};
    virtual void on_pre_update_camera_controller(RopewayPlayerCameraController* controller) {};
    virtual void on_update_camera_controller(RopewayPlayerCameraController* controller) {};
    virtual void on_pre_update_camera_controller2(RopewayPlayerCameraController* controller) {};
    virtual void on_update_camera_controller2(RopewayPlayerCameraController* controller) {};
};

