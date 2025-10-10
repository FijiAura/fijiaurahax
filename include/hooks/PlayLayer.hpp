#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include "classes/editorhitboxlayer.hpp"

struct ConditionalBytepatch {
	std::uint32_t rel_addr;
	std::vector<uint8_t> patch_bytes;
	std::vector<uint8_t> orig_bytes;
};

void perform_conditional_patch(const ConditionalBytepatch& patch, bool option);

struct CustomPlayLayer : geode::Modify<CustomPlayLayer, PlayLayer> {
	struct Fields {
		int m_smoothFrames{0};

		bool m_isIllegitimate{false};
		EditorHitboxLayer* m_hitboxLayer{nullptr};
		bool m_isFalseCompletion{false};
		cocos2d::CCSprite* m_timeIcon{nullptr};
		cocos2d::CCLabelBMFont* m_timeLabel{nullptr};
		cocos2d::CCLabelBMFont* m_cheatIndicator{nullptr};
		std::vector<StartPosObject*> m_startPositions{};
		std::uint32_t m_startPositionIdx{0u};
		bool m_pausingSafe{false};

		cocos2d::CCLabelBMFont* m_percentageLabel{};
		bool m_precisePercentage{false};

		cocos2d::CCLabelBMFont* m_infoLabel{};
	};

	// practice music
	void togglePracticeMode(bool change);

	// noclip
	void destroyPlayer(PlayerObject* player);

	void showSecretAchievement();

	// restart button
	void levelComplete();

	void updatePercentageLabel();

	bool init(GJGameLevel* level);

#ifndef GEODE_IS_WINDOWS
	void setupReplay(gd::string replay);
#endif

	void updateProgressbar();

	// updateProgressBar is inlined on windows in these places
	void resetLevel();

	virtual void update(float dt) override;

	float timeForXPos(float xPos, bool clean);

	CheckpointObject* createCheckpoint();
	void loadLastCheckpoint();

	void resume();
	void pauseGame(bool p1);
	virtual void onEnterTransitionDidFinish() override;

	void onQuit();

	void updateIndicators();

public:
	// determines if any modifications that are cheats are enabled, but does not set status
	bool determineCheatStatus();

	// "reset" cheat status, should be run on the beginning of an attempt
	void resetCheats();

	// "updates" cheat status, intended to run mid-attempt and can only enable cheat status
	void updateCheats();
};
