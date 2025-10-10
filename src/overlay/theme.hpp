#pragma once

#include <cstdint>
#include <unordered_map>

#include <matjson.hpp>

#include "utils.hpp"

struct WindowColor {
	std::uint32_t background{};
	std::uint32_t background_hover{};
	std::uint32_t text{};
	std::uint32_t primary{};
	std::uint32_t window{};
	std::uint32_t inactive{};
	std::uint32_t active{};
	std::uint32_t title{};
};

template <>
struct matjson::Serialize<WindowColor> {
	static geode::Result<WindowColor> fromJson(const matjson::Value& value) {
		GEODE_UNWRAP_INTO(auto background, value["background"].as<std::uint32_t>());
		GEODE_UNWRAP_INTO(auto background_hover, value["background_hover"].as<std::uint32_t>());
		GEODE_UNWRAP_INTO(auto text, value["text"].as<std::uint32_t>());
		GEODE_UNWRAP_INTO(auto primary, value["primary"].as<std::uint32_t>());
		GEODE_UNWRAP_INTO(auto window, value["window"].as<std::uint32_t>());
		GEODE_UNWRAP_INTO(auto inactive, value["inactive"].as<std::uint32_t>());
		GEODE_UNWRAP_INTO(auto active, value["active"].as<std::uint32_t>());
		GEODE_UNWRAP_INTO(auto title, value["title"].as<std::uint32_t>());

		return geode::Ok(WindowColor {
			rgba(background),
			rgba(background_hover),
			rgba(text),
			rgba(primary),
			rgba(window),
			rgba(inactive),
			rgba(active),
			rgba(title)
		});
	}

	static matjson::Value toJson(const WindowColor& color) {
		return matjson::makeObject({
			{ "background", invertRGBA(color.background) },
			{ "background_hover", invertRGBA(color.background_hover) },
			{ "text", invertRGBA(color.text) },
			{ "primary", invertRGBA(color.primary) },
			{ "window", invertRGBA(color.window) },
			{ "inactive", invertRGBA(color.inactive) },
			{ "active", invertRGBA(color.active) },
			{ "title", invertRGBA(color.title) }
		});
	}
};

struct ThemeDefinition {
	std::string name{};
	std::uint32_t preview_color{};
	std::unordered_map<std::string, WindowColor> windows{};
};

template <>
struct matjson::Serialize<ThemeDefinition> {
	static geode::Result<ThemeDefinition> fromJson(const matjson::Value& value) {
		GEODE_UNWRAP_INTO(auto name, value["name"].asString());
		GEODE_UNWRAP_INTO(auto preview, value["preview"].as<std::uint32_t>());

		auto windows = value["windows"];
		if (!windows.isObject()) {
			return geode::Err("windows key is not a valid object");
		}

		std::unordered_map<std::string, WindowColor> parsed{};
		for (const auto& [id, w] : windows) {
			GEODE_UNWRAP_INTO(parsed[id], w.as<WindowColor>());
		}

		return geode::Ok(ThemeDefinition {
			name,
			rgba(preview),
			parsed
		});
	}

	static matjson::Value toJson(const ThemeDefinition& def) {
		auto windows = matjson::Value::object();
		for (const auto& [id, w] : def.windows) {
			windows.set(id, w);
		}

		return matjson::makeObject({
			{ "name", def.name },
			{ "preview", invertRGBA(def.preview_color) },
			{ "windows", windows },
		});
	}
};

struct ThemesFile {
	std::unordered_map<std::string, ThemeDefinition> themes{};
};

template <>
struct matjson::Serialize<ThemesFile> {
	static geode::Result<ThemesFile> fromJson(const matjson::Value& value) {
		if (!value.isObject()) {
			return geode::Err("beginning of themes is not a valid object");
		}

		std::unordered_map<std::string, ThemeDefinition> parsed{};
		for (const auto& [id, t] : value) {
			GEODE_UNWRAP_INTO(parsed[id], t.as<ThemeDefinition>());
		}

		return geode::Ok(ThemesFile { parsed });
	}

	static matjson::Value toJson(const ThemesFile& def) {
		auto themes = matjson::Value::object();
		for (const auto& [id, t] : def.themes) {
			themes.set(id, t);
		}

		return themes;
	}
};

class ThemeManager {
	ThemesFile m_themes{};
	ThemeDefinition m_active_theme{};

	std::string m_active_window_id{};
	std::string m_active_theme_id{};

	void reset_default();

public:
	WindowColor& theme_for_id(const std::string& id);

	std::string get_active_window_id() const {
		return m_active_window_id;
	}

	void set_active_window_id(std::string x) {
		m_active_window_id = x;
	}

	void set_active_theme_id(std::string x);

	std::string get_active_theme_id() const {
		return m_active_theme_id;
	}

	const ThemeDefinition& get_active_theme() const {
		return m_active_theme;
	}

	WindowColor& get_active_window() {
		return m_active_theme.windows[m_active_window_id];
	}

	std::vector<std::string> get_theme_ids();
	std::vector<std::string> get_window_ids();

	std::vector<std::pair<std::string, ThemeDefinition*>> get_themes();

	WindowColor& get_default_colors();

	void reload_themes();
	void init();

	static ThemeManager& get();
};
