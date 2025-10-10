#pragma once

#ifndef CLASSES_MANAGERS_OVERLAYMANAGER_HPP
#define CLASSES_MANAGERS_OVERLAYMANAGER_HPP

#include <vector>
#include <functional>
#include <unordered_map>
#include <string>

#include <imgui.h>

namespace Overlay {
	enum class Font {
		Monospace,
		SansSerif
	};

	enum class FontScale {
		Small,
		Base,
		Medium,
		Large,
		ExtraLarge
	};
}

class OverlayManager {
	std::function<void()> _dev_overlay{};
	std::function<void()> _primary_overlay{};

	std::unordered_map<std::string, std::vector<std::uint8_t>> _fontData{};
	std::unordered_map<Overlay::Font, std::unordered_map<Overlay::FontScale, ImFont*>> _fonts{};

	float _scaleFactor{0.0f};

	float _scale{1.0f};

	Overlay::FontScale _fontScale{Overlay::FontScale::Base};

	bool _initialized{false};
	bool _overlayActive{false};

	void setupFont(Overlay::Font id, const char* fontPath, float fontScale);

	OverlayManager() {}

public:
	static OverlayManager& get();

	void setup();

	OverlayManager& registerDevOverlay(std::function<void()> overlay) {
		_dev_overlay = overlay;
		return *this;
	}

	OverlayManager& registerPrimaryOverlay(std::function<void()> overlay) {
		_primary_overlay = overlay;
		return *this;
	}

	ImFont* getFont(Overlay::Font id) const;

	void render() const;
	float scaleFactor() const;

	void setScale(float scale);
	float scale() const;

	bool initialized() const;

	void setOverlayActive(bool);

	static const char* getKeybindName();

	OverlayManager(const OverlayManager&) = delete;
	OverlayManager(OverlayManager&&) = delete;
};

#endif
