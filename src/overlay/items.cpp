#include "menu.hpp"

#include <iostream>
#include <numbers>
#include <fmt/format.h>
#include <misc/cpp/imgui_stdlib.h>

#include "classes/managers/overlaymanager.hpp"
#include "classes/managers/speedhackmanager.hpp"

void Window::draw() {
	auto& theme = get_theme();

	ImGui::SetNextWindowSize(ImVec2(theme.width, -1));
	if (m_dirty_position) {
		if (m_animating) {
			const auto easing = [](float x) {
				return 1.f - std::pow(1.f - x, 3.f);
			};
			float t = easing(m_animation_time / m_animation_duration);
			auto pos = m_start_pos * (1.f - t) + m_target_pos * t;
			ImGui::SetNextWindowPos(pos);
			m_animation_time += ImGui::GetIO().DeltaTime;
			if (m_animation_time >= m_animation_duration) {
				m_animation_time = 0.f;
				m_dirty_position = false;
				m_animating = false;
			}
		} else {
			ImGui::SetNextWindowPos(m_pos);
			m_dirty_position = false;
		}
	} else {
		m_target_pos = m_pos;
	}

	// PushID doesnt seem to work with windows

	ImGui::PushStyleColor(ImGuiCol_Text, theme.colors.title);

	if (ImGui::Begin(fmt::format("{}##{}", m_name, static_cast<void*>(this)).c_str(), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
		ImGui::PopStyleColor(1);
		for (auto& item : m_items) {
			item->render();
		}
	} else {
		ImGui::PopStyleColor(1);
	}

	m_open = !ImGui::IsWindowCollapsed();

	m_pos = ImGui::GetWindowPos();
	m_size = ImGui::GetWindowSize();

	ImGui::End();
}

void CheckboxMenuItem::render() {
	auto draw = ImGui::GetWindowDrawList();

	auto& theme = get_theme();

	const auto height = theme.height;

	if (ImGui::InvisibleButton(this->label.c_str(), ImVec2(-1, height))) {
		this->value = !this->value;
		this->pressed();
	}

	const bool hovering = ImGui::IsItemHovered();
	const auto item_min = ImGui::GetItemRectMin();
	const auto item_max = ImGui::GetItemRectMax();
	const auto item_mid = (item_min + item_max) / 2.f;
	const float width = item_max.x - item_min.x;

	const auto text_size = ImGui::CalcTextSize(label.c_str());

	if (this->value) {
		// TODO: hardcoded colors D:
		auto colBg = mixRGB(theme.colors.active, theme.colors.background, 0x20);
		draw->AddRectFilled(item_min, item_max, colBg);

		auto colRight = mixRGB(theme.colors.active, theme.colors.background, 0x50);
		draw->AddRectFilledMultiColor(ImVec2(item_min.x + width * 0.56f, item_min.y), item_max,
			colBg, colRight, colRight, colBg
		);
	}
	if (hovering) {
		draw->AddRectFilled(item_min, item_max, theme.colors.background_hover);
	}
	draw->AddText(item_min + ImVec2(5, height / 2.f - text_size.y / 2.f), this->value ? theme.colors.primary : theme.colors.text, this->label.c_str());
	draw->AddRectFilled(ImVec2(item_max.x - (6 * OverlayManager::get().scale()), item_min.y + 3), item_max - ImVec2(3, 3), this->value ? theme.colors.primary : theme.colors.inactive);

	if (false) {
		// checkmark
		const auto check_height = height - 6;
		const auto check_origin = ImVec2(item_max.x - check_height - 9, (item_min.y + item_max.y) / 2.f - check_height / 2.f);
		ImVec2 points[] = {
			ImVec2(0.10f, 0.51f) * check_height + check_origin,
			ImVec2(0.40f, 0.80f) * check_height + check_origin,
			ImVec2(0.90f, 0.14f) * check_height + check_origin,
		};
		if (this->value)
			draw->AddPolyline(points, 3, this->value ? theme.colors.primary : theme.colors.inactive, 0, 3.0f);
	}
}

void ButtonMenuItem::render() {
	auto draw = ImGui::GetWindowDrawList();

	if (ImGui::InvisibleButton(this->label.c_str(), ImVec2(-1, get_theme().height))) {
		this->callback();
	}

	const bool hovering = ImGui::IsItemHovered();
	const auto item_min = ImGui::GetItemRectMin();
	const auto item_max = ImGui::GetItemRectMax();

	const auto text_size = ImGui::CalcTextSize(label.c_str());

	auto& theme = get_theme();
	if (hovering) {
		draw->AddRectFilled(item_min, item_max, theme.colors.background_hover);
	}
	draw->AddText((item_min + item_max) / 2.0f - text_size / 2.f, theme.colors.text, this->label.c_str());
}

void InputMenuItem::render() {
	auto& theme = get_theme();

	ImGui::SetNextItemWidth(-1);

	ImGui::PushID(this);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, theme.colors.background_hover);

	this->render_input();

	ImGui::PopStyleColor();
	ImGui::PopID();
}

void InputMenuItem::render_input() {
	static std::string buffer;
	ImGui::InputText("", &buffer);
}

void updateFPSValue() {
#ifdef GEODE_IS_WINDOWS
	auto bypassEnabled = GameManager::sharedState()->getIntGameVariable(GameVariable::ENABLE_FPS_BYPASS);
	auto currentFps = GameManager::sharedState()->getIntGameVariable(GameVariable::FPS_BYPASS);

	if (currentFps == 0) {
		GameManager::sharedState()->setIntGameVariable(GameVariable::FPS_BYPASS, 60);
		currentFps = 60;
	}

	if (bypassEnabled) {
		cocos2d::CCApplication::sharedApplication()->toggleVerticalSync(false);
		cocos2d::CCDirector::sharedDirector()->setAnimationInterval(1 / static_cast<double>(currentFps));
	} else {
		cocos2d::CCDirector::sharedDirector()->setAnimationInterval(1 / 60.0);
		if (GameManager::sharedState()->getGameVariable(GameVariable::VERTICAL_SYNC)) {
			cocos2d::CCApplication::sharedApplication()->toggleVerticalSync(true);
		}
	}
#endif
}

void FPSInputMenuItem::render_input() {
	ImGui::InputScalar("##", ImGuiDataType_U32, &m_value, nullptr, nullptr, "%d FPS", ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_ParseEmptyRefVal);
	if (ImGui::IsItemDeactivated()) {
		if (m_value == 0)
			m_value = 60;

		if (m_value <= 15) {
			m_value = 15;
		}

		GameManager::sharedState()->setIntGameVariable(GameVariable::FPS_BYPASS, m_value);

		updateFPSValue();
	}
}

void updateSpeedhack() {
	auto currentInterval = (GameManager::sharedState()->getIntGameVariable(GameVariable::SPEED_INTERVAL) + 1000.0f) / 1000.0f;
	SpeedhackManager::get().setSpeedhackValue(currentInterval);

	updateOverlayHacks();
}

void SpeedInputMenuItem::render_input() {
	ImGui::InputScalar("##", ImGuiDataType_Float, &m_value, nullptr, nullptr, "%.2fx", ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_ParseEmptyRefVal);
	if (ImGui::IsItemDeactivated()) {
		if (m_value <= 0)
			m_value = 1.0f;

		auto storedValue = static_cast<int>((m_value * 1000.0f) - 1000.0f);
		GameManager::sharedState()->setIntGameVariable(GameVariable::SPEED_INTERVAL, storedValue);

		updateSpeedhack();
	}
}

void BindingMenuItem::callback() {
	auto& menu = Menu::get();
	if (menu.is_listening()) {
		menu.set_listening(false);
		this->load();

		return;
	}

	menu.set_listening(true);
	this->label = "Listening...";
}

void RowMenuItem::render() {
	ImGui::PushID(this);
	auto avail = ImGui::GetContentRegionAvail();
	if (ImGui::BeginTable("", 2, ImGuiTableFlags_SizingFixedFit)) {
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, avail.x / 2.f);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, avail.x / 2.f);
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		m_left->render();
		ImGui::TableSetColumnIndex(1);
		m_right->render();
		ImGui::EndTable();
	}
	ImGui::PopID();
}

void ColorPickerMenuItem::render() {
	auto draw = ImGui::GetWindowDrawList();

	auto& theme = get_theme();

	ImGui::NewLine();

	auto width_offset = theme.width - theme.height;
	ImGui::SameLine(width_offset);

	if (ImGui::ColorEdit3(label.c_str(), &m_color.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
		this->m_cb(*this);
	}

	const auto item_min = ImGui::GetItemRectMin();
	const auto item_max = ImGui::GetItemRectMax();

	const auto text_size = ImGui::CalcTextSize(label.c_str());

	draw->AddText(item_min + ImVec2(5 - width_offset, theme.height / 2.f - text_size.y / 2.f), theme.colors.text, this->label.c_str());
}

void ComboMenuItem::set_selected_id(std::string x) {
	m_selected_id = x;

	auto it = std::find_if(m_items.begin(), m_items.end(), [&x](const auto& p) {
		return p.id == x;
	});

	if (it != m_items.end()) {
		m_selected_name = fmt::format("{}: {}", label, it->label);
	}
}

void ComboMenuItem::draw_select_button(std::string id, std::string label, bool selected, std::uint32_t preview_color) {
	auto draw = ImGui::GetWindowDrawList();

	auto& theme = get_theme();

	const auto height = theme.height;

	if (ImGui::InvisibleButton(label.c_str(), ImVec2(-1, height)) && !selected) {
		this->on_select(id);
	}

	const bool hovering = ImGui::IsItemHovered();
	const auto item_min = ImGui::GetItemRectMin();
	const auto item_max = ImGui::GetItemRectMax();
	const auto item_mid = (item_min + item_max) / 2.f;

	const float width = item_max.x - item_min.x;

	const auto text_size = ImGui::CalcTextSize(label.c_str());

	draw->AddRectFilled(item_min, item_max, theme.colors.background);

	const auto scale = OverlayManager::get().scale();
	const auto preview_square_pad = 4.0f * scale;

	if (selected) {
		auto colBg = mixRGB(theme.colors.active, theme.colors.background, 0x20);
		draw->AddRectFilled(item_min, item_max, colBg);

		auto colRight = mixRGB(theme.colors.active, theme.colors.background, 0x50);
		draw->AddRectFilledMultiColor(ImVec2(item_min.x + width * 0.56f, item_min.y), item_max,
			colBg, colRight, colRight, colBg
		);
	}
	if (hovering) {
		draw->AddRectFilled(item_min, item_max, theme.colors.background_hover);
	}

	if (preview_color) {
		draw->AddRectFilled(
			ImVec2(item_min.x + preview_square_pad, item_min.y + preview_square_pad),
			ImVec2(item_min.x + height - preview_square_pad, item_max.y - preview_square_pad),
			preview_color
		);
	}

	auto text_offset = preview_color ? height - preview_square_pad : 0;
	draw->AddText(item_min + ImVec2(7.0f * scale + text_offset, height / 2.f - text_size.y / 2.f), selected ? theme.colors.primary : theme.colors.text, label.c_str());
	draw->AddRectFilled(ImVec2(item_max.x - (6 * scale), item_min.y + 3), item_max - ImVec2(3, 3), selected ? theme.colors.primary : theme.colors.inactive);
}

void ComboMenuItem::render() {
	ImGui::SetNextItemWidth(-1);

	auto& theme = get_theme();

	ImGui::PushStyleColor(ImGuiCol_PopupBg, 0);

	ImGui::PushStyleColor(ImGuiCol_FrameBg, theme.colors.background);
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, 0);

	ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0);

	bool combo_open = false;
	if (ImGui::BeginCombo(fmt::format("##{}", label).c_str(), "", ImGuiComboFlags_NoArrowButton)) {
		combo_open = true;

		for (const auto& item : m_items) {
			auto is_selected = item.id == m_selected_id;
			this->draw_select_button(item.id, item.label, is_selected, item.preview_color);

			if (is_selected) {
				ImGui::SetItemDefaultFocus();
			}
		}

		ImGui::EndCombo();
	}

	ImGui::PopStyleVar(1);
	ImGui::PopStyleColor(3);

	auto draw = ImGui::GetWindowDrawList();

	const bool hovering = ImGui::IsItemHovered();
	const auto item_min = ImGui::GetItemRectMin();
	const auto item_max = ImGui::GetItemRectMax();

	const auto text_size = ImGui::CalcTextSize(m_selected_name.c_str());

	if (hovering || combo_open) {
		draw->AddRectFilled(item_min, item_max, theme.colors.background_hover);
	}
	draw->AddText((item_min + item_max) / 2.0f - text_size / 2.f, theme.colors.text, m_selected_name.c_str());
}

void ThemeMenuItem::on_select(std::string id) {
	ThemeManager::get().set_active_theme_id(id);

	Menu::get().reload_all();
}

void ThemeMenuItem::load() {
	auto& theme_manager = ThemeManager::get();
	auto theme_ids = theme_manager.get_themes();

	this->reset_items();

	for (const auto& [id, theme] : theme_ids) {
		this->add_item(id, theme->name, theme->preview_color);
	}

	this->set_selected_id(theme_manager.get_active_theme_id());
}