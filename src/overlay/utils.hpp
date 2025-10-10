#pragma once

#include <imgui.h>

#include <cstdint>
#include <algorithm>

inline ImVec2 operator+(const ImVec2& a, const ImVec2& b) { return {a.x + b.x, a.y + b.y}; }
inline ImVec2 operator-(const ImVec2& a, const ImVec2& b) { return {a.x - b.x, a.y - b.y}; }
inline ImVec2 operator/(ImVec2 const& a, float b) { return { a.x / b, a.y / b }; }
inline ImVec2 operator*(ImVec2 const& a, float b) { return { a.x * b, a.y * b }; }

constexpr ImU32 rgb(std::size_t value) {
	const auto r = value >> 16;
	const auto g = (value >> 8) & 0xFF;
	const auto b = value & 0xFF;
	return 0xFF000000 | (b << 16) | (g << 8) | r;
}

constexpr ImU32 rgba(std::size_t value) {
	const auto r = value >> 24;
	const auto g = (value >> 16) & 0xFF;
	const auto b = (value >> 8) & 0xFF;
	const auto a = value & 0xFF;
	return (a << 24) | (b << 16) | (g << 8) | r;
}

constexpr ImU32 modAlpha(ImU32 v, std::uint8_t a) {
	return (v & ~(0xff << 24)) | (a << 24);
}

constexpr size_t invertRGBA(ImU32 value) {
	const auto r = value & 0xff;
	const auto g = (value >> 8) & 0xFF;
	const auto b = (value >> 16) & 0xFF;
	const auto a = value >> 24;
	return (r << 24) | (g << 16) | (b << 8) | a;
}

constexpr size_t invertRGB(ImU32 value) {
	const auto r = value & 0xff;
	const auto g = (value >> 8) & 0xFF;
	const auto b = (value >> 16) & 0xFF;
	const auto a = value >> 24;
	return (r << 16) | (g << 8) | b;
}

constexpr size_t clampToByte(auto x) {
	return static_cast<std::size_t>(std::clamp(x, decltype(x){0}, decltype(x){255}));
}

constexpr size_t mixRGB(ImU32 x, ImU32 y, std::uint8_t a) {
	const auto xr = x & 0xff;
	const auto xg = (x >> 8) & 0xFF;
	const auto xb = (x >> 16) & 0xFF;

	const auto yr = y & 0xff;
	const auto yg = (y >> 8) & 0xFF;
	const auto yb = (y >> 16) & 0xFF;

	const auto alpha_mod = a / 255.0f;
	const auto rr = clampToByte(xr * alpha_mod + yr * (1 - alpha_mod));
	const auto rg = clampToByte(xg * alpha_mod + yg * (1 - alpha_mod));
	const auto rb = clampToByte(xb * alpha_mod + yb * (1 - alpha_mod));

	return 0xFF000000 | (rb << 16) | (rg << 8) | rr;
}
