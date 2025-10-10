#include "classes/managers/updatemanager.hpp"

#include "base/config.hpp"
#include "base/game_variables.hpp"

UpdateManager& UpdateManager::get() {
	static UpdateManager _managerInstance;
	return _managerInstance;
}

namespace {
	bool isNewer(UpdateObject& update) {
		return update.versionTag > geode::Mod::get()->getVersion();
	}

	geode::Result<UpdateObject> handleRequest(geode::utils::web::WebResponse* res) {
		if (!res->ok()) {
			return geode::Err("request failed - {}", res->code());
		}

		GEODE_UNWRAP_INTO(auto json, res->json());

		GEODE_UNWRAP_INTO(auto version, json.get<std::string>("version"));
		GEODE_UNWRAP_INTO(auto downloadUrl, json.get<std::string>("download_url"));

		GEODE_UNWRAP_INTO(auto versionTag, geode::VersionInfo::parse(version));

		return geode::Ok(UpdateObject {
			.versionTag = versionTag, .downloadUrl = downloadUrl
		});
	}

	// someone forgot to export this and it's too late to fix it
	std::string VersionTag_toSuffixString(const geode::VersionTag& tag) {
		std::string res = "";
		switch (tag.value) {
			case geode::VersionTag::Alpha: res += "-alpha"; break;
			case geode::VersionTag::Beta: res += "-beta"; break;
			case geode::VersionTag::Prerelease: res += "-prerelease"; break;
		}
		if (tag.number) {
			res += "." + std::to_string(tag.number.value());
		}
		return res;
	}
}


void UpdateManager::beginUpdateCheck() {
	if (m_sentRequest) {
		return;
	}

	if (GameManager::sharedState()->getGameVariable(GameVariable::DISABLE_UPDATE_CHECK)) {
		return;
	}

	// add --geode:zmx.aura.disable-update-check to launch command
	static bool has_disabled_flag = geode::Mod::get()->getLaunchFlag("disable-update-check");
	if (has_disabled_flag) {
		return;
	}

	this->m_listener.bind([this](geode::utils::web::WebTask::Event* e) {
		if (e->getProgress()) {
			return;
		}

		if (auto res = e->getValue()) {
			auto versionRes = handleRequest(res);
			if (!versionRes) {
				geode::log::info("failed to parse update request: {}", versionRes.unwrapErr());

				m_sentRequest = false;
				return;
			}

			auto versionInfo = versionRes.unwrap();

			m_nextUpdate = versionInfo;
			if (isNewer(versionInfo)) {
				m_hasUpdate = true;
			}
		}
	});

	auto url = GDMOD_ENDPOINT_BASE_URL "/checkUpdate.php";
	auto req = geode::utils::web::WebRequest();
	req.userAgent(Config::USER_AGENT);

	m_sentRequest = true;

	this->m_listener.setFilter(req.get(url));
}

std::string UpdateManager::formatVersion(const geode::VersionInfo& version) {
	auto versionString = fmt::format("u{}.{}", version.getMajor(), version.getMinor());
	auto tag = version.getTag();
	if (version.getPatch() != 0 || tag) {
		versionString += fmt::format(".{}", version.getPatch());
	}

	if (tag) {
		versionString += VersionTag_toSuffixString(*tag);
	}

	return versionString;
}
