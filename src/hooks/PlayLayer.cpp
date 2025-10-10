#include "hooks/PlayLayer.hpp"

#include "base/game_variables.hpp"

#include "classes/managers/speedhackmanager.hpp"
#include "classes/managers/secretmanager.hpp"

#include "classes/extensions/checkpointobjectext.hpp"

#ifdef GEODE_IS_ANDROID
#include "classes/managers/controllermanager.hpp"
#endif

#include <unordered_map>

std::unordered_map<std::uint32_t, geode::Patch*> g_patches {};

void perform_conditional_patch(const ConditionalBytepatch& patch, bool option) {
	if (!g_patches.contains(patch.rel_addr)) {
		if (!option) {
			return;
		}

		auto res = geode::Mod::get()->patch(
			reinterpret_cast<void*>(geode::base::get() + patch.rel_addr),
			patch.patch_bytes
		);

		if (!res) {
			geode::log::warn("Patch at addr {:#x} failed: {}", patch.rel_addr, res.unwrapErr());
			return;
		}

		g_patches[patch.rel_addr] = res.unwrap();
		return;
	}

	auto created = g_patches[patch.rel_addr];
	if (created->isEnabled() && !option) {
		(void)created->disable();
		return;
	}

	if (!created->isEnabled() && option) {
		(void)created->enable();
		return;
	}
}

void CustomPlayLayer::togglePracticeMode(bool change) {
	auto gm = GameManager::sharedState();
	bool practice_music_enabled = gm->getGameVariable(GameVariable::PRACTICE_MUSIC);

#if defined(GEODE_IS_ANDROID)
	const std::vector<ConditionalBytepatch> patches {
		{
			0x1B9050, // PlayLayer::togglePracticeMode
			{ 0x03, 0xE0 }, // nop
			{ 0x1D, 0xB1 } // cbz
		},
		{
			0x1B8EBA, // PlayLayer::resetLevel
			{ 0x00, 0xBF }, // nop
			{ 0x79, 0xD1 } // bne
		},
		{
			0x1B60C6, // PlayLayer::destroyPlayer
			{ 0x00, 0xBF }, // nop
			{ 0x93, 0xB9 } // cbnz
		},
		{
			0x1B60F2, // PlayLayer::destroyPlayer
			{ 0x00, 0xBF }, // nop
			{ 0x1B, 0xB9 } // cbnz
		},
		{
			0x1B6498, // PlayLayer::updateVisibility
			{ 0x1B, 0x45 }, // cmp r3, r3
			{ 0x00, 0x2B } // cmp r3, #0x0
		},
		{
			0x1B7666, // PlayLayer::resume
			{ 0x00, 0xbf }, // nop
			{ 0x44, 0xd1 } // bne
		},
		{
			0x1b757e, // PlayLayer::pauseGame
			{ 0x04, 0xe0 }, // b
			{ 0x23, 0xb1 } // cbz
		}
	};
#elif defined(GEODE_IS_WINDOWS)
	const std::vector<ConditionalBytepatch> patches {
		{
			0xf3663, // PlayLayer::togglePracticeMode
			{ 0x90, 0x90 }, // nop
			{ 0x75, 0x41 } // jnz
		},
		{
			0xf284f, // PlayLayer::resetLevel
			{ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }, // nop
			{ 0x0f, 0x85, 0x4d, 0x07, 0x00, 0x00 } // jnz
		},
		{
			0xf0699, // PlayLayer::destroyPlayer
			{ 0x90, 0x90 }, // nop
			{ 0x75, 0x3c } // jnz
		},
		{
			0xf06cb, // PlayLayer::destroyPlayer
			{ 0x90, 0x90 }, // nop
			{ 0x75, 0x0c } // jnz
		},
		{
			0xeb441, // PlayLayer::updateVisibility
			{ 0xeb, 0x16 }, // jmp
			{ 0x74, 0x16 } // jz
		},
		{
			0xf3a96, // PlayLayer::resume
			{ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }, // nop
			{ 0x0f, 0x85, 0xb5, 0x00, 0x00, 0x00 } // jnz
		},
		{
			0xf3943, // PlayLayer::pauseGame
			{ 0xeb, 0x11 }, // jmp
			{ 0x74, 0x11 } // jnz
		}
	};
#else
#error Missing patches for PlayLayer::togglePracticeMode (practice music)
#endif

	for (auto& patch : patches) {
		perform_conditional_patch(patch, practice_music_enabled && change);
	}

	if (practice_music_enabled && change && !this->m_practiceMode) {
		this->m_practiceMode = change;

#ifdef GEODE_IS_WINDOWS
		if (gm->getGameVariable(GameVariable::HIDE_PRACTICE_UI)) {
			change = false;
		}
#endif

		this->m_uiLayer->toggleCheckpointsMenu(change);

		this->stopActionByTag(18);
		return;
	}

	PlayLayer::togglePracticeMode(change);
}

void CustomPlayLayer::destroyPlayer(PlayerObject* player) {
	auto gm = GameManager::sharedState();
	bool noclip_enabled = gm->getGameVariable(GameVariable::IGNORE_DAMAGE);

	// yes this disables special noclip
	if (noclip_enabled)
		return;

	auto illegitimate_run = false;

	auto fields = m_fields.self();
	illegitimate_run = fields->m_isIllegitimate || fields->m_isFalseCompletion;

	// show hitboxes if enabled
	auto hitboxes_enabled = GameManager::sharedState()->getGameVariable(GameVariable::SHOW_HITBOXES_ON_DEATH);
	if (hitboxes_enabled) {
		auto hitboxes = fields->m_hitboxLayer;

		hitboxes->setVisible(true);
		hitboxes->beginUpdate();

		// just assume this is the leftmost point
		auto camera = this->m_cameraPos;

		auto width = cocos2d::CCDirector::sharedDirector()->getWinSize().width;

		auto beginSection = static_cast<int>(std::floor(camera.x / 100.0f));
		auto endSection = static_cast<int>(std::ceil((camera.x + width) / 100.0f));

		auto lvlSections = this->m_levelSections;
		auto sectionCount = lvlSections->count();

		beginSection = std::max(beginSection, 0);
		endSection = std::min(endSection, static_cast<int>(sectionCount - 1));

		for (auto i = beginSection; i <= endSection; i++) {
			auto sectionObjs = reinterpret_cast<cocos2d::CCArray*>(lvlSections->objectAtIndex(i));
			auto objCount = sectionObjs->count();
			for (auto j = 0u; j < objCount; j++) {
				auto obj = reinterpret_cast<GameObject*>(sectionObjs->objectAtIndex(j));
				hitboxes->drawObject(obj, -1);
			}
		}

		hitboxes->drawPlayer(this->m_player);
		if (this->m_dualMode) {
			hitboxes->drawPlayer(this->m_player2);
		}
	}

	auto was_testmode = this->m_testMode;

	this->m_testMode = was_testmode || illegitimate_run;
	PlayLayer::destroyPlayer(player);

	// we don't have to care about new best, at least
	bool fast_reset_enabled = gm->getGameVariable(GameVariable::FAST_PRACTICE_RESET);
	if (this->m_practiceMode && this->m_resetQueued && fast_reset_enabled) {
		this->stopActionByTag(16);
		auto resetSequence = cocos2d::CCSequence::createWithTwoActions(
			cocos2d::CCDelayTime::create(0.5f),
			cocos2d::CCCallFunc::create(this, static_cast<cocos2d::SEL_CallFunc>(&PlayLayer::delayedResetLevel))
		);
		resetSequence->setTag(16);
		this->runAction(resetSequence);
	}

	this->m_testMode = was_testmode;
}

void CustomPlayLayer::showSecretAchievement() {
	auto achievement = AchievementBar::create("Congratulations!", "The Gnome has been unlocked.", nullptr);
	auto gnomeSprite = cocos2d::CCSprite::create("gnome.png"_spr);

	auto iconSprite = achievement->m_layerColor->getChildByType<cocos2d::CCSprite>(0);
	achievement->m_layerColor->addChild(gnomeSprite);

	if (iconSprite) {
		gnomeSprite->setPosition(iconSprite->getPosition());
	}

	gnomeSprite->setZOrder(4);
	gnomeSprite->setScale(0.75f);

	auto an = AchievementNotifier::sharedState();
	an->m_achievementBarArray->addObject(achievement);
	if (an->m_activeAchievementBar == nullptr) {
		an->showNextAchievement();
	}
}

void CustomPlayLayer::levelComplete() {
	bool ignore_completion = false;

	auto fields = m_fields.self();
	if (fields->m_isIllegitimate || fields->m_isFalseCompletion) {
		geode::log::info("legitimacy tripped, exiting level");
		ignore_completion = true;
	}

	auto was_testmode = this->m_testMode;

	if (!ignore_completion && !was_testmode && !m_practiceMode && m_level->m_levelType == GJLevelType::Local) {
		auto& secretManager = SecretManager::get();
		secretManager.completeLevel(m_level->m_levelID);

		auto gnomeCollected = GameStatsManager::sharedState()->hasUniqueItem("gnome02");
		auto gnomeUnlocked = GameManager::sharedState()->getGameVariable(GameVariable::SECRET_COMPLETED) || gnomeCollected;
		if (secretManager.finishedRun() && !gnomeUnlocked) {
			showSecretAchievement();
			GameManager::sharedState()->setGameVariable(GameVariable::SECRET_COMPLETED, true);
		}
	}

	this->m_testMode = was_testmode || ignore_completion;
	PlayLayer::levelComplete();

	this->m_testMode = was_testmode;
}

void CustomPlayLayer::updatePercentageLabel() {
	auto player = this->m_player;
	auto max_level_size = this->m_levelLength;

	auto percentage = std::min((player->getPositionX() / max_level_size) * 100.0f, 100.0f);

	auto fields = m_fields.self();
	auto label = fields->m_percentageLabel;
	if (!label) {
		return;
	}

	if (fields->m_precisePercentage) {
		auto percentage_string = fmt::format("{:.2f}%", percentage);
		label->setString(percentage_string.c_str(), true);
	} else {
		auto percentage_string = fmt::format("{}%", static_cast<int>(percentage));
		label->setString(percentage_string.c_str(), true);
	}
}

bool CustomPlayLayer::init(GJGameLevel* level) {
	if (!PlayLayer::init(level)) {
		return false;
	}

	auto gm = GameManager::sharedState();

#ifdef GEODE_IS_ANDROID
	if (!gm->getGameVariable(GameVariable::SHOW_CURSOR)) {
		if (PlatformToolbox::isControllerConnected()) {
			ControllerManager::getManager().hideCursor();
		}
	}
#endif

	bool secret_enabled = gm->getGameVariable(GameVariable::REPLAY_CONTROLS);

	auto fields = m_fields.self();

	if (secret_enabled) {
		this->m_recordActions = true;
		geode::log::info("recording feature force enabled");
	}

	// bug in pre 1.93, would allow for completion of deleted levels
	auto lvl_objs = this->m_levelSections;
	uint32_t obj_count = 0;
	for (uint32_t i = 0; i < lvl_objs->count(); i++) {
		obj_count += reinterpret_cast<cocos2d::CCArray*>(lvl_objs->objectAtIndex(i))->count();
		// don't count all the objects in the level if we know there's enough to get past the check
		if (obj_count > 1) {
			break;
		}
	}

	if (this->m_testMode) {
		// determine the start positions in the level. we know there's at least one
		for (auto i = 0u; i < lvl_objs->count(); i++) {
			auto sectionObjs = static_cast<cocos2d::CCArray*>(lvl_objs->objectAtIndex(i));
			for (auto j = 0u; j < sectionObjs->count(); j++) {
				auto obj = static_cast<GameObject*>(sectionObjs->objectAtIndex(j));
				if (obj->m_objectID == 31) {
					auto startPos = static_cast<StartPosObject*>(obj);
					fields->m_startPositions.push_back(startPos);
					// should this vector be sorted after?
				}
			}
		}

		// the player automatically starts at the last start position
		fields->m_startPositionIdx = fields->m_startPositions.size();
	}

	// end portal counts as an object, don't count it here obviously
	if (obj_count <= 1 && this->m_level->m_stars >= 1) {
		fields->m_isFalseCompletion = true;

		auto warning_text = cocos2d::CCLabelBMFont::create("I see you :)", "bigFont.fnt");
		this->m_gameLayer->addChild(warning_text, 4);
		warning_text->setPosition(700.0f, 240.0f);
	}

	if (level->m_audioTrack >= 18) {
		GameSoundManager::sharedManager()->enableMetering();
		this->m_meteringEnabled = true;
	}

	auto director = cocos2d::CCDirector::sharedDirector();
	auto w_size = director->getWinSize();

	auto percentage_label = cocos2d::CCLabelBMFont::create("0%", "bigFont.fnt");
	this->addChild(percentage_label, 21);
	percentage_label->setID("percentage-label"_spr);

	fields->m_precisePercentage = GameManager::sharedState()->getGameVariable(GameVariable::ACCURATE_PERCENTAGE);
	fields->m_percentageLabel = percentage_label;

	if (gm->m_showProgressBar) {
		percentage_label->setPositionX((w_size.width / 2) + 110.0f);
		percentage_label->setAnchorPoint(cocos2d::CCPoint(0.0f, 0.0f));
	} else {
		percentage_label->setPositionX(w_size.width / 2);
		percentage_label->setAnchorPoint(cocos2d::CCPoint(0.5f, 0.0f));
	}

	percentage_label->setPositionY(director->getScreenTop() - 16.0f);
	percentage_label->setScale(0.5f);

	if (!gm->getGameVariable(GameVariable::SHOW_PERCENTAGE)) {
		percentage_label->setVisible(false);
	}

	this->updatePercentageLabel();

	auto hitboxNode = EditorHitboxLayer::create();
	this->m_gameLayer->addChild(hitboxNode, 9);
	hitboxNode->setVisible(false);
	hitboxNode->setSkipHitboxUpdates(true);

	fields->m_hitboxLayer = hitboxNode;

	auto time_sprite = cocos2d::CCSprite::createWithSpriteFrameName("GJ_timeIcon_001.png");
	this->addChild(time_sprite, 21);
	fields->m_timeIcon = time_sprite;

	auto screen_side = gm->getGameVariable(GameVariable::FLIP_PAUSE) ? director->getScreenRight() : director->getScreenLeft();
	auto screen_positioning_scale = gm->getGameVariable(GameVariable::FLIP_PAUSE) ? -1 : 1;
	auto screen_top = director->getScreenTop();

	time_sprite->setPositionX(screen_side + screen_positioning_scale * 13.0f);
	time_sprite->setPositionY(screen_top - 13.0f);

	time_sprite->setOpacity(127);
	time_sprite->setScale(0.9f);
	time_sprite->setID("speedhack-icon"_spr);

	auto speedhack_interval = SpeedhackManager::get().getSpeedhackInterval();
	auto speedhack_string = cocos2d::CCString::createWithFormat("%.2fx", speedhack_interval);
	auto speedhack_label = cocos2d::CCLabelBMFont::create(
			speedhack_string->getCString(), "bigFont.fnt");

	this->addChild(speedhack_label, 21);
	speedhack_label->setPosition(screen_side + screen_positioning_scale * 27.0f, screen_top - 12.0f);
	speedhack_label->setScale(0.5f);
	speedhack_label->setOpacity(127);
	speedhack_label->setAnchorPoint({0.0f, 0.5f});
	speedhack_label->setID("speedhack-label"_spr);
	fields->m_timeLabel = speedhack_label;

	if (gm->getGameVariable(GameVariable::FLIP_PAUSE)) {
		speedhack_label->setAlignment(cocos2d::CCTextAlignment::kCCTextAlignmentRight);
		speedhack_label->setAnchorPoint({1.0f, 0.5f});
	}

	auto cheat_indicator = cocos2d::CCLabelBMFont::create(".", "bigFont.fnt");
	this->addChild(cheat_indicator, 21);
	cheat_indicator->setPosition(
		director->getScreenRight() - 20.0f,
		director->getScreenBottom() + 20.0f
	);
	fields->m_cheatIndicator = cheat_indicator;
	cheat_indicator->setID("cheat-indicator"_spr);

	SpeedhackManager::get().setSpeedhackValue(speedhack_interval);

	this->resetCheats();

	auto infoLabel = cocos2d::CCLabelBMFont::create("", "chatFont.fnt");
	this->addChild(infoLabel, 21);
	infoLabel->setPosition(
		director->getScreenLeft() + 20.0f,
		director->getScreenTop() - 20.0f
	);
	fields->m_infoLabel = infoLabel;
	infoLabel->setID("info-label"_spr);

	infoLabel->setScale(0.5f);

	infoLabel->setAnchorPoint({0.0f, 1.0f});

	return true;
}

#ifndef GEODE_IS_WINDOWS
void CustomPlayLayer::setupReplay(gd::string replay) {
	PlayLayer::setupReplay(replay);

	if (this->getPlaybackMode()) {
		auto label = cocos2d::CCLabelBMFont::create("Playback", "bigFont.fnt");
		this->addChild(label, 10);

		auto director = cocos2d::CCDirector::sharedDirector();
		auto screen_left = director->getScreenLeft();
		auto screen_top = director->getScreenTop();

		label->setPositionX(screen_left + 43.0f);
		label->setPositionY(screen_top - 10.0f);

		label->setScale(0.5f);
		label->setOpacity(127);
	}
}
#endif

void CustomPlayLayer::updateProgressbar() {
	PlayLayer::updateProgressbar();
	this->updatePercentageLabel();
}

// updateProgressBar is inlined on windows in these places
void CustomPlayLayer::resetLevel() {
	if (m_level->m_levelType == GJLevelType::Local) {
		SecretManager::get().playLevel(m_level->m_levelID);
	} else {
		SecretManager::get().clearStatus();
	}

	if (this->m_practiceMode || this->m_testMode) {
		m_fields->m_smoothFrames = 2;
	}

	PlayLayer::resetLevel();

	auto fields = m_fields.self();
	if (!this->m_practiceMode) {
		this->resetCheats();
	}

	if (auto hitboxes = m_fields->m_hitboxLayer; hitboxes != nullptr) {
		hitboxes->beginUpdate();
		hitboxes->setVisible(false);
	}

#ifdef GEODE_IS_WINDOWS
	this->updatePercentageLabel();
#endif

	SpeedhackManager::get().setGameplayActive(true);
}

void CustomPlayLayer::update(float dt) {
	if (this->m_practiceMode || this->m_testMode) {
		auto& fields = m_fields;
		if (fields->m_smoothFrames > 0) {
			auto ideal_dt = cocos2d::CCDirector::sharedDirector()->getAnimationInterval();

			if (dt - ideal_dt < 1) {
				fields->m_smoothFrames--;
			}

			dt = ideal_dt;
		}
	}

	PlayLayer::update(dt);

#ifdef GEODE_IS_WINDOWS
	this->updatePercentageLabel();
#endif
}

float CustomPlayLayer::timeForXPos(float xPos, bool clean) {
	if (this->m_practiceMode) {
		// this causes practice music to be based on checked speed portals on unpause
		// not ideal, but this was already the case on respawn so... whatever
		// (the game clears the list of activated speed portals on reset)
		clean = true;
	}

	return PlayLayer::timeForXPos(xPos, clean);
}

CheckpointObject* CustomPlayLayer::createCheckpoint() {
	auto chk = PlayLayer::createCheckpoint();
	if (!chk) {
		return chk;
	}

	auto ext = new CheckpointObjectExt();
	ext->autorelease();

	chk->setUserObject("ext"_spr, ext);

	ext->m_portalRef = m_cameraPortal;
	ext->m_dualPortalRef = m_dualModeCamera;

	/*
	unsigned int songPos;
	auto audioEngine = FMODAudioEngine::sharedEngine();
	audioEngine->m_globalChannel->getPosition(&songPos, FMOD_TIMEUNIT_MS);

	if (audioEngine->m_musicOffset > 0) {
		songPos += audioEngine->m_musicOffset;
	}

	ext->m_songTime = songPos / 1000.0f;
	*/

	return chk;
}

void CustomPlayLayer::loadLastCheckpoint() {
	// fix for the practice blending bug
	// the game stores the current coloraction based on the current blending status,
	// but doesn't restore blending until after creating the color action
	if (this->m_checkpoints->count() > 0) {
		auto checkpoint = static_cast<CheckpointObject*>(this->m_checkpoints->lastObject());

		this->updateCustomColorBlend(3, checkpoint->m_customColor01Action->m_blend);
		this->updateCustomColorBlend(4, checkpoint->m_customColor02Action->m_blend);
		this->updateCustomColorBlend(6, checkpoint->m_customColor03Action->m_blend);
		this->updateCustomColorBlend(7, checkpoint->m_customColor04Action->m_blend);
		this->updateCustomColorBlend(8, checkpoint->m_dLineColorAction->m_blend);
	}

	PlayLayer::loadLastCheckpoint();

	// silly bugfix for restarting while endscreen is active
	if (this->m_checkpoints->count() > 0) {
		auto checkpoint = static_cast<CheckpointObject*>(this->m_checkpoints->lastObject());

		if (m_endPortalObject->m_spawnXPosition <= checkpoint->m_playerCheck01->m_playerPos.x) {
			m_endPortalObject->triggerObject();
		}

		if (auto ext = static_cast<CheckpointObjectExt*>(checkpoint->getUserObject("ext"_spr))) {
			m_cameraPortal = ext->m_portalRef;
			m_dualModeCamera = ext->m_dualPortalRef;
		}
	}
}

void CustomPlayLayer::resume() {
	SpeedhackManager::get().setGameplayActive(true);

	PlayLayer::resume();

	// i don't know what bug rob was fixing with this one, but i'm unfixing it. sorry robtop
	if (AppDelegate::get()->m_paused || m_isDead || m_player->getPositionX() > 0.0f) {
		return;
	}

	auto audioEngine = FMODAudioEngine::sharedEngine();
	if (!audioEngine->isBackgroundMusicPlaying()) {
		auto fadeMusic = m_levelSettings->m_fadeIn;

		auto filename = m_level->getAudioFileName();
		if (fadeMusic) {
			audioEngine->pauseBackgroundMusic();
		}

		GameSoundManager::sharedManager()->playBackgroundMusic(filename, false, false);

		auto offset = m_levelSettings->m_songOffset;
		if (offset > 0.0f) {
			audioEngine->setBackgroundMusicTime(offset);
		}

		if (fadeMusic) {
			audioEngine->fadeBackgroundMusic(true, 2.0f);
		}
	}
}

void CustomPlayLayer::onQuit() {
	SpeedhackManager::get().setGameplayActive(false);
	PlayLayer::onQuit();
}

void CustomPlayLayer::pauseGame(bool p1) {
	if (!m_fields->m_pausingSafe) {
		return;
	}

	SpeedhackManager::get().setGameplayActive(false);

	PlayLayer::pauseGame(p1);
}

void CustomPlayLayer::onEnterTransitionDidFinish() {
	SpeedhackManager::get().setGameplayActive(true);

	PlayLayer::onEnterTransitionDidFinish();

	m_fields->m_pausingSafe = true;
}

bool CustomPlayLayer::determineCheatStatus() {
	if (GameManager::sharedState()->getGameVariable(GameVariable::IGNORE_DAMAGE)) {
		return true;
	}

	if (SpeedhackManager::get().speedhackWillBeActive()) {
		return true;
	}

	if (m_fields->m_isFalseCompletion) {
		return true;
	}

	return false;
}

void CustomPlayLayer::updateIndicators() {
	auto fields = m_fields.self();
	if (fields->m_timeLabel && fields->m_timeIcon) {
		auto speedhack_interval = SpeedhackManager::get().getSpeedhackInterval();
		auto speedhack_string = fmt::format("{:.2f}x", speedhack_interval);

		fields->m_timeLabel->setString(speedhack_string.c_str());

		auto speedhack_active = SpeedhackManager::get().speedhackWillBeActive();
		fields->m_timeLabel->setVisible(speedhack_active);
		fields->m_timeIcon->setVisible(speedhack_active);
	}

	if (fields->m_cheatIndicator) {
		auto cheat_status = this->determineCheatStatus();
		if (cheat_status) {
			fields->m_cheatIndicator->setColor({0xff, 0x00, 0x00});
			fields->m_cheatIndicator->setVisible(true);
		} else if (fields->m_isIllegitimate) {
			fields->m_cheatIndicator->setColor({0xff, 0xaa, 0x00});
		} else {
			fields->m_cheatIndicator->setColor({0x00, 0xff, 0x00});
			fields->m_cheatIndicator->setVisible(false);
		}
	}
}

void CustomPlayLayer::resetCheats() {
	m_fields->m_isIllegitimate = this->determineCheatStatus();
	this->updateIndicators();
}

void CustomPlayLayer::updateCheats() {
	auto fields = m_fields.self();
	fields->m_isIllegitimate = fields->m_isIllegitimate || this->determineCheatStatus();
	this->updateIndicators();
}

