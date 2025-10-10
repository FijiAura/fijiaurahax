#include <fstream>

#include "base/platform_helper.hpp"
#include "classes/callbacks/levelimportcallback.hpp"

#include <launcher-utils/jni.hpp>

bool PlatformHelper::is_gd_installed() {
	return launcher_utils::jni::callStaticMethod<bool>("com/kyurime/geometryjump/ModGlue", "isGeometryDashInstalled", "()Z").unwrapOr(false);
}

void PlatformHelper::keep_screen_awake() {
	(void)launcher_utils::jni::callStaticMethod<void>("com/kyurime/geometryjump/ModGlue", "keepScreenAwake", "()V");
}

void PlatformHelper::remove_screen_awake() {
	(void)launcher_utils::jni::callStaticMethod<void>("com/kyurime/geometryjump/ModGlue", "removeScreenAwake", "()V");
}

void PlatformHelper::open_texture_picker() {
	(void)launcher_utils::jni::callStaticMethod<void>("com/kyurime/geometryjump/ModGlue", "showTexturePicker", "()V");
}

void PlatformHelper::apply_classic_pack() {
	(void)launcher_utils::jni::callStaticMethod<void>("com/kyurime/geometryjump/ModGlue", "applyClassicPack", "()V");
}

void PlatformHelper::wipe_textures_directory() {
	(void)launcher_utils::jni::callStaticMethod<void>("com/kyurime/geometryjump/ModGlue", "wipeTexturesDirectory", "()V");
}

namespace {
void pick_level_export(std::string_view name) {
	if (name.empty()) {
		return;
	}

	auto env = launcher_utils::jni::getEnv();
	if (!env) {
		return;
	}

	auto strRef = launcher_utils::jni::toJString(*env, name);
	if (!strRef) {
		return;
	}

	(void)launcher_utils::jni::callStaticMethod<void>("com/kyurime/geometryjump/ModGlue", "onExportLevel", "(Ljava/lang/String;)V", (*strRef).get());
}

std::string get_level_export_path() {
	return launcher_utils::jni::callStaticMethod<std::string>("com/kyurime/geometryjump/ModGlue", "getLevelExportPath", "()Ljava/lang/String;").unwrapOrDefault();
}
}

void PlatformHelper::export_level(std::string name, std::string export_string) {
	if (name.empty()) {
		name = "Unnamed";
	}

	auto output_path = get_level_export_path();

	auto out = std::ofstream(output_path, std::ofstream::out);

	out << export_string;

	out.close();

	pick_level_export(name);
}

void PlatformHelper::import_level() {
	using namespace geode::utils;

	file::pickReadBytes({
		.defaultPath = std::nullopt,
		.filters = {
			{"GD Level Files", {"*.gmd"}}
		},
	}).listen(
		[=](geode::Result<std::span<const std::uint8_t>>* result) {
			if (!result->ok()) {
				return;
			}

			LevelImportCallback::importLevelBytes(result->unwrap());
		}
	);
}

void PlatformHelper::loaded_to_menu() {
	(void)launcher_utils::jni::callStaticMethod<void>("com/kyurime/geometryjump/ModGlue", "loadedToMenu", "()V");
}

std::string PlatformHelper::get_textures_directory() {
	return launcher_utils::jni::callStaticMethod<std::string>("com/kyurime/geometryjump/ModGlue", "getTexturesDirectory", "()Ljava/lang/String;").unwrapOrDefault();
}

bool PlatformHelper::is_controller_connected() {
	return launcher_utils::jni::callStaticMethod<bool>("com/kyurime/geometryjump/ModGlue", "isControllerConnected", "()Z").unwrapOr(false);
}

std::string PlatformHelper::get_secondary_assets_directory() {
	return launcher_utils::jni::callStaticMethod<std::string>("com/kyurime/geometryjump/ModGlue", "getSecondaryAssetsDirectory", "()Ljava/lang/String;").unwrapOrDefault();
}

std::string PlatformHelper::get_save_directory() {
	return launcher_utils::jni::callStaticMethod<std::string>("com/kyurime/geometryjump/ModGlue", "getSaveDirectory", "()Ljava/lang/String;").unwrapOrDefault();
}

bool PlatformHelper::is_launcher_build() {
	return launcher_utils::jni::callStaticMethod<bool>("com/kyurime/geometryjump/ModGlue", "isLauncherBuild", "()Z").unwrapOr(false);
}

bool PlatformHelper::is_screen_restricted() {
	return launcher_utils::jni::callStaticMethod<bool>("com/kyurime/geometryjump/ModGlue", "isScreenRestricted", "()Z").unwrapOr(false);
}

void PlatformHelper::toggle_is_screen_restricted() {
	(void)launcher_utils::jni::callStaticMethod<void>("com/kyurime/geometryjump/ModGlue", "toggleIsScreenRestricted", "()V");
}

void PlatformHelper::capture_cursor() {
	(void)launcher_utils::jni::callStaticMethod<void>("com/kyurime/geometryjump/ModGlue", "captureCursor", "()V");
}

void PlatformHelper::release_cursor() {
	(void)launcher_utils::jni::callStaticMethod<void>("com/kyurime/geometryjump/ModGlue", "releaseCursor", "()V");
}
