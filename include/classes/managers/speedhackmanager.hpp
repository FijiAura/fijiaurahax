#pragma once

class SpeedhackManager {
	SpeedhackManager();

	bool m_hackActive{false};

	bool m_gameplayActive{false};
	bool m_playtestActive{false};

	float m_lastScale{1.0f};

	static float getIntervalForValue(float value);

	void setGlobalTimeScale(float scale);

	void setSpeedhackActive(bool active);

public:
	static SpeedhackManager& get();

	SpeedhackManager(const SpeedhackManager&) = delete;
	SpeedhackManager(SpeedhackManager&&) = delete;

	void updateSpeedhack();

	bool speedhackWillBeActive();

	void setGameplayActive(bool active) {
		m_gameplayActive = active;
		this->updateSpeedhack();
	}

	void setPlaytestActive(bool active) {
		m_playtestActive = active;
		this->updateSpeedhack();
	}

	bool isSpeedhackActive();
	float getSpeedhackInterval();
	void setSpeedhackValue(float interval);

	static int getSaveValueForInterval(int old_val, int change);
};
