#include <Geode/Geode.hpp>

#include "classes/managers/speedhackmanager.hpp"

#include "base/game_variables.hpp"

SpeedhackManager::SpeedhackManager() {}

SpeedhackManager& SpeedhackManager::get() {
	static SpeedhackManager _managerInstance;
	return _managerInstance;
}

float SpeedhackManager::getIntervalForValue(float value) {
	return (value + 1000.0f) / 1000.0f;
}

int SpeedhackManager::getSaveValueForInterval(int old_val, int change) {
	return old_val + change;
}

void SpeedhackManager::setGlobalTimeScale(float scale) {
	if (scale == 0.0f) {
		// reset scale if it's a bad value
		scale = 1.0f;
	}

	// for other mods, if we don't want to reset the timeScale, then don't do it
	if (m_lastScale == scale) {
		return;
	}
	m_lastScale = scale;

	cocos2d::CCDirector::sharedDirector()->getScheduler()->setTimeScale(scale);

	auto control = FMODAudioEngine::sharedEngine()->m_globalChannel;
	control->setPitch(scale);
}

void SpeedhackManager::setSpeedhackActive(bool active) {
	m_hackActive = active;
}

bool SpeedhackManager::speedhackWillBeActive() {
	auto interval = getSpeedhackInterval();
	return interval != 1.0f && GameManager::sharedState()->getGameVariable(GameVariable::OVERLAY_SPEEDHACK_ENABLED);
}

void SpeedhackManager::updateSpeedhack() {
	if (m_gameplayActive || m_playtestActive) {
		auto interval = getSpeedhackInterval();
		auto active = interval != 1.0f && GameManager::sharedState()->getGameVariable(GameVariable::OVERLAY_SPEEDHACK_ENABLED);

		setSpeedhackActive(active);
		setSpeedhackValue(interval);
	} else {
		setSpeedhackActive(false);
		setSpeedhackValue(1.0f);
	}
}

bool SpeedhackManager::isSpeedhackActive() {
	return m_hackActive;
}

float SpeedhackManager::getSpeedhackInterval() {
	auto gm = GameManager::sharedState();
	auto speedhack_interval = gm->getIntGameVariable(GameVariable::SPEED_INTERVAL);

	return getIntervalForValue(speedhack_interval);
}

void SpeedhackManager::setSpeedhackValue(float interval) {
	// nothing wants speedhack
	if (!(m_gameplayActive || m_playtestActive) || !GameManager::sharedState()->getGameVariable(GameVariable::OVERLAY_SPEEDHACK_ENABLED)) {
		setSpeedhackActive(false);
		setGlobalTimeScale(1.0f);

		return;
	}

	// enables speedhack if interval is not 1
	setSpeedhackActive(interval != 1.0f);
	setGlobalTimeScale(interval);
}