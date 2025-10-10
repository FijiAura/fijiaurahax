#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/EditLevelLayer.hpp>
#include <Geode/modify/EndLevelLayer.hpp>
#include <Geode/modify/FMODAudioEngine.hpp>
#include <Geode/modify/ShareLevelSettingsLayer.hpp>
#include <Geode/modify/ShareLevelLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/GJGameLevel.hpp>
#include <Geode/modify/UILayer.hpp>

#include <unordered_map>
#include <fmt/format.h>

#include "base/platform_helper.hpp"
#include "base/game_variables.hpp"

#include "classes/gameplaysettingspopup.hpp"
#include "classes/managers/secretmanager.hpp"

#include "hooks/PlayLayer.hpp"

bool updatePrevStartPos() {
	auto playLayer = static_cast<CustomPlayLayer*>(GameManager::sharedState()->m_playLayer);

	auto startPos = playLayer->m_fields->m_startPositions;
	auto startPosIdx = playLayer->m_fields->m_startPositionIdx;

	if (startPosIdx == 0) {
		return false;
	} else {
		startPosIdx--;
	}

	if (startPosIdx > 0) {
		auto currentPos = startPos[startPosIdx - 1];
		playLayer->m_startPos = currentPos->getPosition();
		playLayer->m_startPosObject = currentPos;
	} else {
		playLayer->m_startPos = cocos2d::CCPoint(0.0f, 0.0f);
		playLayer->m_startPosObject = nullptr;
	}

	playLayer->m_fields->m_startPositionIdx = startPosIdx;

	return true;
}

bool updateNextStartPos() {
	auto playLayer = static_cast<CustomPlayLayer*>(GameManager::sharedState()->m_playLayer);

	auto startPos = playLayer->m_fields->m_startPositions;
	auto startPosIdx = playLayer->m_fields->m_startPositionIdx;

	if (startPosIdx == startPos.size()) {
		return false;
	} else {
		startPosIdx++;
	}

	if (startPosIdx > 0) {
		auto currentPos = startPos[startPosIdx - 1];
		playLayer->m_startPos = currentPos->getPosition();
		playLayer->m_startPosObject = currentPos;
	} else {
		playLayer->m_startPos = cocos2d::CCPoint(0.0f, 0.0f);
		playLayer->m_startPosObject = nullptr;
	}

	playLayer->m_fields->m_startPositionIdx = startPosIdx;

	return true;
}

struct CustomPauseLayer : geode::Modify<CustomPauseLayer, PauseLayer> {
	struct Fields {
		cocos2d::CCLabelBMFont* m_startPosIndicator{nullptr};
		CCMenuItemSpriteExtra* m_startPosPrev{nullptr};
		CCMenuItemSpriteExtra* m_startPosNext{nullptr};

		bool m_hidden{false};
		cocos2d::CCMenu* m_hideMenu{nullptr};
	};

	void onGameplayOptions(cocos2d::CCObject*) {
		GameplaySettingsPopup::create()->show();
	}

	void onHide(cocos2d::CCObject*) {
		auto& fields = m_fields;
		for (auto child : geode::cocos::CCArrayExt<cocos2d::CCNode>(this->getChildren())) {
			if (child != fields->m_hideMenu) {
				child->setVisible(fields->m_hidden);
			}
		}

		if (fields->m_hidden) {
			this->setOpacity(75);
			fields->m_hideMenu->setOpacity(255);
		} else {
			this->setOpacity(0);
			fields->m_hideMenu->setOpacity(75);
		}

		fields->m_hidden = !fields->m_hidden;
	}

	// restart button
	void customSetup() {
		auto gm = GameManager::sharedState();
		bool restart_button_enabled = gm->getGameVariable(GameVariable::SHOW_RESTART);

#if defined(GEODE_IS_ANDROID)
		ConditionalBytepatch patch = {
			0x1ED2F0, // PauseLayer::customSetup
			{ 0x00, 0xBF }, // nop
			{ 0x12, 0xD1 } // ble
		};
#elif defined(GEODE_IS_WINDOWS)
		ConditionalBytepatch patch = {
			0xd64d9, // PauseLayer::customSetup
			{ 0x90, 0x90 }, // nop
			{ 0x75, 0x29 } // jnz
		};
#else
#error Missing patches for PauseLayer::customSetup (restart button)
#endif

		perform_conditional_patch(patch, restart_button_enabled);

		PauseLayer::customSetup();
		auto director = cocos2d::CCDirector::sharedDirector();

		auto options_sprite = cocos2d::CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
		options_sprite->setScale(0.75f);

		auto options_btn = CCMenuItemSpriteExtra::create(
			options_sprite, nullptr, this,
			static_cast<cocos2d::SEL_MenuHandler>(&CustomPauseLayer::onGameplayOptions)
		);
		options_btn->setSizeMult(1.1f);

		auto menu = this->getChildByID("right-button-menu");

		menu->addChild(options_btn);
		menu->updateLayout();

		options_btn->setID("options-btn"_spr);

		auto hide_sprite = cocos2d::CCSprite::createWithSpriteFrameName("hideBtn.png"_spr);
		hide_sprite->setScale(0.75f);
		hide_sprite->setOpacity(75);

		auto hide_btn = CCMenuItemSpriteExtra::create(hide_sprite, nullptr, this, static_cast<cocos2d::SEL_MenuHandler>(&CustomPauseLayer::onHide));

		auto hide_menu = cocos2d::CCMenu::createWithItem(hide_btn);
		this->addChild(hide_menu);
		hide_menu->setPosition({director->getScreenLeft() + 12.5f, director->getScreenBottom() + 10.0f});

		hide_menu->setID("hide-menu"_spr);
		hide_btn->setID("hide-btn"_spr);

		m_fields->m_hideMenu = hide_menu;

		auto playLayer = static_cast<CustomPlayLayer*>(GameManager::sharedState()->m_playLayer);

		auto startPos = playLayer->m_fields->m_startPositions;
		auto startPosIdx = playLayer->m_fields->m_startPositionIdx;

		if (!startPos.empty()) {
			auto actionsMenu = this->getChildByID("center-button-menu");

			auto spriteLeft = cocos2d::CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");

			auto btnLeft = CCMenuItemSpriteExtra::create(
				spriteLeft, nullptr, this, static_cast<cocos2d::SEL_MenuHandler>(&CustomPauseLayer::onPrevStartPos)
			);
			auto leftMostItem = static_cast<cocos2d::CCNode*>(actionsMenu->getChildren()->firstObject());
			auto leftItemPos = leftMostItem->getPosition();

			m_fields->m_startPosPrev = btnLeft;

			auto spriteRight = cocos2d::CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
			spriteRight->setFlipX(true);

			auto btnRight = CCMenuItemSpriteExtra::create(
				spriteRight, nullptr, this,
				static_cast<cocos2d::SEL_MenuHandler>(&CustomPauseLayer::onNextStartPos)
			);

			auto rightItem = static_cast<cocos2d::CCNode*>(actionsMenu->getChildren()->lastObject());
			auto rightItemPos = rightItem->getPosition();

			actionsMenu->addChild(btnLeft);
			btnLeft->setPosition({leftItemPos.x - 60.0f, 0.0f});
			btnLeft->setVisible(false);
			btnLeft->setID("start-pos-prev"_spr);

			actionsMenu->addChild(btnRight);
			btnRight->setPosition({rightItemPos.x + 60.0f, 0.0f});
			btnRight->setVisible(false);
			btnRight->setID("start-pos-next"_spr);

			m_fields->m_startPosNext = btnRight;

			auto startPosLabel = cocos2d::CCLabelBMFont::create("", "bigFont.fnt");

			auto posX = director->getScreenLeft() + 30.0f;
			auto posY = director->getScreenTop() - 40.0f;

			this->addChild(startPosLabel, 2);
			startPosLabel->setScale(0.75f);
			startPosLabel->setPosition({posX, posY});
			startPosLabel->setAnchorPoint({0.0f, 0.5f});
			startPosLabel->setID("start-pos-label"_spr);
			startPosLabel->setOpacity(0);

			m_fields->m_startPosIndicator = startPosLabel;

			updateStartPosIndicators();
		}
	}

	void onFullRestart() {
		auto playLayer = GameManager::sharedState()->m_playLayer;

		while (playLayer->m_checkpoints->count() > 0) {
			playLayer->removeLastCheckpoint();
		}

		// force audio to remain paused
		auto fmod = FMODAudioEngine::sharedEngine();
		auto volume = fmod->m_backgroundMusicVolume;
		fmod->m_backgroundMusicVolume = 0.0f;

		this->onRestart(nullptr);

		fmod->m_backgroundMusicVolume = volume;

		playLayer->pauseGame(false);
	}

	void onPrevStartPos(cocos2d::CCObject*) {
		updatePrevStartPos();

		this->updateStartPosIndicators();

		this->onFullRestart();
	}

	void onNextStartPos(cocos2d::CCObject*) {
		updateNextStartPos();

		this->updateStartPosIndicators();

		this->onFullRestart();
	}

	void updateStartPosIndicators() {
		auto& fields = m_fields;

		auto playLayer = static_cast<CustomPlayLayer*>(GameManager::sharedState()->m_playLayer);

		auto startPos = playLayer->m_fields->m_startPositions;
		auto startPosIdx = playLayer->m_fields->m_startPositionIdx;

		fields->m_startPosPrev->setVisible(startPosIdx != 0);
		fields->m_startPosNext->setVisible(startPosIdx != startPos.size());

		auto startPosText = fmt::format("({}/{})", startPosIdx, startPos.size());
		fields->m_startPosIndicator->setString(startPosText.c_str());

		fields->m_startPosIndicator->setScale(0.75f);
		fields->m_startPosIndicator->limitLabelWidth(75.0f, 0.75f, 0.25f);
	}

	void onProgressBar(cocos2d::CCObject* target) {
		PauseLayer::onProgressBar(target);

		auto gm = GameManager::sharedState();

		if (auto pl = gm->m_playLayer; pl != nullptr) {
			if (auto percent_label = static_cast<CustomPlayLayer*>(pl)->m_fields->m_percentageLabel) {
				auto director = cocos2d::CCDirector::sharedDirector();
				auto w_size = director->getWinSize();

				if (gm->m_showProgressBar) {
					percent_label->setPositionX((w_size.width / 2) + 110.0f);
					percent_label->setAnchorPoint(cocos2d::CCPoint(0.0f, 0.0f));
				} else {
					percent_label->setPositionX(w_size.width / 2);
					percent_label->setAnchorPoint(cocos2d::CCPoint(0.5f, 0.0f));
				}
			}
		}
	}
};

#ifdef GDMOD_ENABLE_LOGGING

struct BypassEditLevelLayer : geode::Modify<BypassEditLevelLayer, EditLevelLayer> {
	void onShare(cocos2d::CCObject* target) {
		auto gm = GameManager::sharedState();
		bool vfb_enabled = gm->getGameVariable(GameVariable::BYPASS_VERIFY);

		if (vfb_enabled) {
			auto was_verified = this->m_level->m_isVerified;
			this->m_level->m_isVerified = true;
			EditLevelLayer::onShare(target);
			this->m_level->m_isVerified = was_verified;

			return;
		}

		EditLevelLayer::onShare(target);
	}
};

#endif

struct AwakeFMODAudioEngine : geode::Modify<AwakeFMODAudioEngine, FMODAudioEngine> {
	void playBackgroundMusic(gd::string path, bool loop, bool paused) {
		if (path != "menuLoop.mp3") {
#ifdef GEODE_IS_ANDROID
			// ignore menu song
			if (GameManager::sharedState()->getGameVariable(GameVariable::KEEP_AWAKE)) {
				PlatformHelper::keep_screen_awake();
			}
#endif
		} else {
			if (GameManager::sharedState()->getGameVariable(GameVariable::DISABLE_MENU_MUSIC)) {
				return;
			}
		}

		FMODAudioEngine::playBackgroundMusic(path, loop, paused);
	}

#ifdef GEODE_IS_ANDROID
	void pauseBackgroundMusic() {
		FMODAudioEngine::pauseBackgroundMusic();

		if (GameManager::sharedState()->getGameVariable(GameVariable::KEEP_AWAKE)) {
			PlatformHelper::remove_screen_awake();
		}
	}

	void stopBackgroundMusic(bool p1) {
		FMODAudioEngine::stopBackgroundMusic(p1);

		if (GameManager::sharedState()->getGameVariable(GameVariable::KEEP_AWAKE)) {
			PlatformHelper::remove_screen_awake();
		}
	}
#endif

	static void onModify(auto& self) {
		if (!self.setHookPriority("FMODAudioEngine::playBackgroundMusic", geode::Priority::Early)) {
			geode::log::warn("failed to set hook priority for FMODAudioEngine::playBackgroundMusic");
		}
	}
};

struct CustomLevelInfoLayer : geode::Modify<CustomLevelInfoLayer, LevelInfoLayer> {
	bool init(GJGameLevel* lvl) {
		bool ch_enabled = GameManager::sharedState()->getGameVariable(GameVariable::COPY_HACK);

		if (ch_enabled) {
			int old_password = lvl->m_password;
			lvl->m_password = 1;

			auto r = LevelInfoLayer::init(lvl);
			lvl->m_password = old_password;
			return r;
		}

		return LevelInfoLayer::init(lvl);
	}

	void tryCloneLevel(cocos2d::CCObject* target) {
		// TODO: this was not the lazy way
		bool ch_enabled = GameManager::sharedState()->getGameVariable(GameVariable::COPY_HACK);

		if (ch_enabled) {
			int old_password = this->m_level->m_password;
			this->m_level->m_password = 1;

			LevelInfoLayer::tryCloneLevel(target);
			this->m_level->m_password = old_password;

			return;
		}

		LevelInfoLayer::tryCloneLevel(target);
	}
};

struct CopyShareLevelSettingsLayer : geode::Modify<CopyShareLevelSettingsLayer, ShareLevelSettingsLayer> {
	bool init(GJGameLevel* lvl) {
		bool ch_enabled = GameManager::sharedState()->getGameVariable(GameVariable::COPY_HACK);
		if (ch_enabled) {
			lvl->m_password = 1;
		}

		if (!ShareLevelSettingsLayer::init(lvl)) {
			return false;
		}

		if (ch_enabled) {
			auto password_check = this->m_passwordToggle;
			password_check->m_notClickable = true;
			password_check->setEnabled(false);
			password_check->setColor({ 0xAB, 0xAB, 0xAB });

			auto password_label = this->m_passwordToggleLabel;
			password_label->setColor({ 0xAB, 0xAB, 0xAB });

			auto copyable_check = reinterpret_cast<CCMenuItemToggler*>(this->m_buttonMenu->getChildren()->objectAtIndex(1));
			copyable_check->m_notClickable = true;
			copyable_check->setEnabled(false);
			copyable_check->setColor({ 0xAB, 0xAB, 0xAB });

			auto copyable_label = reinterpret_cast<cocos2d::CCLabelBMFont*>(this->m_mainLayer->getChildren()->objectAtIndex(4));
			copyable_label->setColor({ 0xAB, 0xAB, 0xAB });

			auto sorry = cocos2d::CCLabelBMFont::create("Copy protection broke :(\nSorry...", "goldFont.fnt");
			this->getInternalLayer()->addChild(sorry);

			auto director = cocos2d::CCDirector::sharedDirector();

			sorry->setPositionX(director->getScreenRight() / 2);
			sorry->setPositionY(director->getScreenTop() / 2);
			sorry->setScale(0.8f);
		}

		return true;
	}
};

struct CopyShareLevelLayer : geode::Modify<CopyShareLevelLayer, ShareLevelLayer> {
	bool init(GJGameLevel* lvl) {
		bool ch_enabled = GameManager::sharedState()->getGameVariable(GameVariable::COPY_HACK);
		if (ch_enabled) {
			lvl->m_password = 1;
		}

		return ShareLevelLayer::init(lvl);
	}
};

struct CustomEndLevelLayer : geode::Modify<CustomEndLevelLayer, EndLevelLayer> {
	void customSetup() {
		auto cheated = false;
		auto playLayer = static_cast<CustomPlayLayer*>(GameManager::sharedState()->m_playLayer);

		auto cheats_enabled = playLayer->determineCheatStatus();
		auto is_cheating = playLayer->m_fields->m_isIllegitimate;
		auto attemptIgnored = playLayer->m_practiceMode || playLayer->m_testMode;

		if ((cheats_enabled || is_cheating) && !attemptIgnored) {
			cheated = true;
		}

		if (cheated) {
			// honestly, probably the easiest way to get it to not show the coins
			auto level = playLayer->m_level;
			auto oldCoins = level->m_coins;
			level->m_coins = 0;

			EndLevelLayer::customSetup();

			level->m_coins = oldCoins;

			auto message = "You cannot complete a level after using cheats.";
			auto isVerifying = playLayer->m_level->m_levelType == GJLevelType::Editor;
			if (isVerifying) {
				message = "You cannot verify a level after using cheats.";
			}

			auto textArea = static_cast<TextArea*>(this->m_mainLayer->getChildByID("complete-message"));
			if (!textArea) {
				// this is the case for beating a level in normal mode, a label is used instead
				// so we remake the textarea :)

				auto messageArea = TextArea::create(" ", 620.0f, 0, {0.5f, 1.0f}, "bigFont.fnt", 20.0f);
				this->m_mainLayer->addChild(messageArea);
				messageArea->setID("cheats-message"_spr);

				messageArea->setScale(0.55f);
				messageArea->m_height = 34.0f;

				auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();
				messageArea->setPosition({winSize.width * 0.5f, winSize.height * 0.5f - 51.0f});

				messageArea->setString(message);

				// hide the message that is originally being shown
				auto winLabel = this->m_mainLayer->getChildByType<cocos2d::CCLabelBMFont>(-1);
				winLabel->setVisible(false);

				return;
			}

			textArea->setString(message);
			return;
		}

		EndLevelLayer::customSetup();

		if (playLayer->m_practiceMode) {
			auto buttonMenu = static_cast<cocos2d::CCMenu*>(m_mainLayer->getChildByID("button-menu"));
			auto restartSprite = cocos2d::CCSprite::createWithSpriteFrameName("restartCheckBtn.png"_spr);
			auto restartBtn = CCMenuItemSpriteExtra::create(restartSprite, nullptr, this, static_cast<cocos2d::SEL_MenuHandler>(&CustomEndLevelLayer::onReplayLastCheckpoint));

			auto retryBtn = buttonMenu->getChildByID("retry-button");

			buttonMenu->addChild(restartBtn);
			restartBtn->setID("practice-retry-btn"_spr);

			restartBtn->setPositionX(-183.0f);
			restartBtn->setPositionY(retryBtn->getPositionY());
		}
	}

	void onReplayLastCheckpoint(cocos2d::CCObject*) {
		if (!GameManager::sharedState()->getGameVariable(GameVariable::SHOW_CURSOR)) {
			PlatformToolbox::hideCursor();
		}

		GameSoundManager::sharedManager()->stopBackgroundMusic();
		GameSoundManager::sharedManager()->playEffect("playSound_01.ogg", 1.0f, 0.0f, 0.3f);

		this->exitLayer(nullptr);

		auto playLayer = GameManager::sharedState()->m_playLayer;

		auto sequence = cocos2d::CCSequence::createWithTwoActions(
			cocos2d::CCDelayTime::create(0.5f),
			cocos2d::CCCallFunc::create(playLayer, static_cast<cocos2d::SEL_CallFunc>(&PlayLayer::resetLevel))
		);

		playLayer->runAction(sequence);
	}
};

struct QuickUILayer : geode::Modify<QuickUILayer, UILayer> {
	virtual void keyUp(cocos2d::enumKeyCodes key) override {
		UILayer::keyUp(key);

		auto playLayer = GameManager::sharedState()->m_playLayer;

		if (key == cocos2d::enumKeyCodes::KEY_W) {
			this->m_p1Jumping = false;
			playLayer->releaseButton(1, true);
			return;
		}
	}

	virtual void keyDown(cocos2d::enumKeyCodes key) override {
		UILayer::keyDown(key);

		auto playLayer = GameManager::sharedState()->m_playLayer;

		if (key == cocos2d::enumKeyCodes::KEY_W) {
			if (!this->m_p1Jumping) {
				this->m_p1Jumping = true;
				playLayer->pushButton(1, true);
			}
			return;
		}

		auto canPause = !playLayer->m_endTriggered && !playLayer->m_showingEndLayer;

		if (!canPause) {
			return;
		}

		if (key == cocos2d::enumKeyCodes::KEY_R) {
			auto ctrlPressed = cocos2d::CCDirector::sharedDirector()->getKeyboardDispatcher()->getControlKeyPressed();
			if (ctrlPressed) {
				playLayer->fullReset();
			} else {
				playLayer->resetLevel();
			}
		}

		if (key == cocos2d::enumKeyCodes::KEY_F) {
			if (updatePrevStartPos()) {
				while (playLayer->m_checkpoints->count() > 0) {
					playLayer->removeLastCheckpoint();
				}

				playLayer->resetLevel();
			}
		}

		if (key == cocos2d::enumKeyCodes::KEY_H) {
			if (updateNextStartPos()) {
				while (playLayer->m_checkpoints->count() > 0) {
					playLayer->removeLastCheckpoint();
				}

				playLayer->resetLevel();
			}
		}
	}
};

#include <Geode/modify/LevelSettingsObject.hpp>

// fixes a bug where strings that fail to unzip crash the game
struct FixLevelSettingsObject : geode::Modify<FixLevelSettingsObject, LevelSettingsObject> {
	static LevelSettingsObject* objectFromDict(cocos2d::CCDictionary* dict) {
		if (dict == nullptr) {
			return LevelSettingsObject::create();
		}

		return LevelSettingsObject::objectFromDict(dict);
	}
};
