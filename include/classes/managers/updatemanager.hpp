#pragma once

#ifndef CLASSES_MANAGERS_UPDATEMANAGER_HPP
#define CLASSES_MANAGERS_UPDATEMANAGER_HPP

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/VersionInfo.hpp>

#include <optional>

struct UpdateObject {
	geode::VersionInfo versionTag{};
	std::string downloadUrl{};
};

class UpdateManager {
	geode::EventListener<geode::utils::web::WebTask> m_listener{};
	bool m_sentRequest{false};

	bool m_hasUpdate{false};
	std::optional<UpdateObject> m_nextUpdate{};

	UpdateManager() = default;

public:
	static UpdateManager& get();

	UpdateManager(const UpdateManager&) = delete;
	UpdateManager(UpdateManager&&) = delete;

	void beginUpdateCheck();

	bool hasUpdate() const {
		return m_hasUpdate;
	}

	std::optional<UpdateObject> latestUpdate() const {
		return m_nextUpdate;
	}

	static std::string formatVersion(const geode::VersionInfo& version);
};

#endif
