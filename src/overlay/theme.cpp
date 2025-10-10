#include "theme.hpp"

#include <Geode/Geode.hpp>

WindowColor default_color {
	.background = rgb(0x1f2020),
	.background_hover = rgba(0x000000a0),
	.text = rgb(0xdfdfd7),
	.primary = rgb(0x92ceae),
	// window is slighty darker to help with readability
	.window = rgb(0x91b7a3),
	.inactive = rgb(0x525351),
	.active = rgb(0x264e38),
	.title = rgb(0xffffff)
};

WindowColor& ThemeManager::theme_for_id(const std::string& id) {
	if (auto it = m_active_theme.windows.find(id); it != m_active_theme.windows.end()) {
		return it->second;
	}

	return default_color;
}

void ThemeManager::reset_default() {
	auto selected_theme = geode::Mod::get()->getSavedValue<std::string>("active-overlay-theme", "default");
	this->set_active_theme_id(selected_theme);
}

void ThemeManager::set_active_theme_id(std::string x) {
	if (auto it = m_themes.themes.find(x); it != m_themes.themes.end()) {
		m_active_theme = it->second;
	}

	if (!m_active_theme.windows.empty()) {
		auto windows = this->get_window_ids();
		m_active_window_id = windows[0];
	} else {
		m_active_window_id = "";
	}

	m_active_theme_id = x;

	geode::Mod::get()->setSavedValue("active-overlay-theme", x);
}

void ThemeManager::reload_themes() {
	auto res = geode::utils::file::readJsonFromResources("overlay-themes.json"_spr);
	if (!res) {
		geode::log::warn("Failed to read themes: {}", res.unwrapErr());
		return;
	}

	auto res2 = res.unwrap().as<ThemesFile>();
	if (!res2) {
		geode::log::warn("Failed to parse themes file: {}", res2.unwrapErr());
	}

	m_themes = std::move(res2.unwrap());

	this->reset_default();
}

std::vector<std::string> ThemeManager::get_theme_ids() {
	auto& map = m_themes.themes;

	std::vector<std::string> ids{};
	ids.reserve(map.size());

	for (const auto& x : map) {
		ids.push_back(x.first);
	}

	return ids;
}

std::vector<std::string> ThemeManager::get_window_ids() {
	auto& map = m_active_theme.windows;

	std::vector<std::string> ids{};
	ids.reserve(map.size());

	for (const auto& x : map) {
		ids.push_back(x.first);
	}

	return ids;
}

std::vector<std::pair<std::string, ThemeDefinition*>> ThemeManager::get_themes() {
	auto& map = m_themes.themes;

	std::vector<std::pair<std::string, ThemeDefinition*>> ids{};
	ids.reserve(map.size());

	for (auto& [id, theme] : map) {
		ids.push_back({id, &theme});
	}

	return ids;
}


WindowColor& ThemeManager::get_default_colors() {
	return default_color;
}

void ThemeManager::init() {
	this->reload_themes();
	this->reset_default();
}


ThemeManager& ThemeManager::get() {
	static ThemeManager theme_manager;
	return theme_manager;
}
