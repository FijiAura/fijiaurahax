#include <Geode/Geode.hpp>
#include <Geode/modify/GameSoundManager.hpp>

#include "classes/managers/speedhackmanager.hpp"

struct SpeedhackGameSoundManager : geode::Modify<SpeedhackGameSoundManager, GameSoundManager> {
	void playBackgroundMusic(gd::string music, bool p1, bool p2) {
		GameSoundManager::playBackgroundMusic(music, p1, p2);

		auto control = FMODAudioEngine::sharedEngine()->m_globalChannel;
		auto& speedhackManager = SpeedhackManager::get();
		if (speedhackManager.isSpeedhackActive()) {
			auto interval = speedhackManager.getSpeedhackInterval();
			control->setPitch(interval);

			return;
		}
	}
};
