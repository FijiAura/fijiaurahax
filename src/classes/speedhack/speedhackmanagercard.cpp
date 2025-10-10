#include "classes/speedhack/speedhackmanagercard.hpp"

#include "classes/managers/speedhackmanager.hpp"

void SpeedhackManagerCard::setDelegate(SpeedhackCardDelegate* delegate) {
	this->delegate_ = delegate;
}

void SpeedhackManagerCard::updateSpeedhackLabel() {
	auto& speedhackManager = SpeedhackManager::get();

	auto speedhack_interval = speedhackManager.getSpeedhackInterval();
	auto speedhack_string = fmt::format("{:.2f}x", speedhack_interval);

	optionsLabel_->setString(speedhack_string.c_str(), true);

	speedhackManager.setSpeedhackValue(speedhack_interval);

	auto speedhackEnabled = GameManager::sharedState()->getGameVariable(GameVariable::OVERLAY_SPEEDHACK_ENABLED);
	if (speedhackEnabled || speedhack_interval == 1.0f) {
		optionsLabel_->setColor({0xff, 0xff, 0xff});
	} else {
		optionsLabel_->setColor({0xbb, 0xbb, 0xbb});
	}

	if (this->delegate_) {
		delegate_->onSpeedhackValueChanged(speedhack_interval);
	}
}

void SpeedhackManagerCard::fixPriority() {
	auto prio = cocos2d::CCDirector::sharedDirector()->getTouchDispatcher()->getTargetPrio();
	itemsMenu_->setTouchPriority(prio - 1);
}

void SpeedhackManagerCard::onBtnDown(cocos2d::CCObject * /* target */) {
	auto gm = GameManager::sharedState();
	auto speedhack_interval = gm->getIntGameVariable(GameVariable::SPEED_INTERVAL);

	if (speedhack_interval > 2000) {
		speedhack_interval = 2000;
	}

	auto saveValue = SpeedhackManager::getSaveValueForInterval(speedhack_interval, -250);
	saveValue = std::clamp(saveValue, -900, 2000);

	gm->setIntGameVariable(GameVariable::SPEED_INTERVAL, saveValue);
	gm->setGameVariable(GameVariable::OVERLAY_SPEEDHACK_ENABLED, saveValue != 0);

	this->updateSpeedhackLabel();
}

void SpeedhackManagerCard::onBtnUp(cocos2d::CCObject * /* target */) {
	auto gm = GameManager::sharedState();
	auto speedhack_interval = gm->getIntGameVariable(GameVariable::SPEED_INTERVAL);

	// silly way of setting <= 0.1x to 0.25x on the next increment
	if (speedhack_interval <= -900) {
		speedhack_interval = -1000;
	}

	auto saveValue = SpeedhackManager::getSaveValueForInterval(speedhack_interval, 250);
	saveValue = std::clamp(saveValue, -900, 2000);

	gm->setIntGameVariable(GameVariable::SPEED_INTERVAL, saveValue);
	gm->setGameVariable(GameVariable::OVERLAY_SPEEDHACK_ENABLED, saveValue != 0);

	this->updateSpeedhackLabel();
}

bool SpeedhackManagerCard::init() {
	auto time_sprite = cocos2d::CCSprite::createWithSpriteFrameName("GJ_timeIcon_001.png");
	this->addChild(time_sprite);
	time_sprite->setPosition(
		cocos2d::CCPoint(-25.0f, 0.0f));

	auto items_menu = cocos2d::CCMenu::create();
	this->addChild(items_menu);
	items_menu->setPosition(0.0f, 0.0f);

	this->itemsMenu_ = items_menu;

	// arrow begins pointing up
	auto speedhack_down = cocos2d::CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
	speedhack_down->setFlipY(true);
	speedhack_down->setRotation(90.0f);
	speedhack_down->setScale(0.8f);

	auto speedhack_up = cocos2d::CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
	speedhack_up->setFlipX(true);
	speedhack_up->setFlipY(true);
	speedhack_up->setRotation(90.0f);
	speedhack_up->setScale(0.8f);

	auto speedhack_down_btn = CCMenuItemSpriteExtra::create(
		speedhack_down, nullptr, this,
		static_cast<cocos2d::SEL_MenuHandler>(&SpeedhackManagerCard::onBtnDown));
	items_menu->addChild(speedhack_down_btn);
	speedhack_down_btn->setPositionY(-30.0f);

	auto speedhack_up_btn = CCMenuItemSpriteExtra::create(
		speedhack_up, nullptr, this,
		static_cast<cocos2d::SEL_MenuHandler>(
			&SpeedhackManagerCard::onBtnUp));
	items_menu->addChild(speedhack_up_btn);
	speedhack_up_btn->setPositionY(30.0f);

	auto speedhack_interval = SpeedhackManager::get().getSpeedhackInterval();
	auto speedhack_string = cocos2d::CCString::createWithFormat("%.2fx", speedhack_interval);

	optionsLabel_ = cocos2d::CCLabelBMFont::create(speedhack_string->getCString(), "bigFont.fnt");
	this->addChild(optionsLabel_);

	optionsLabel_->setPosition(-8.0f, 0.0f);
	optionsLabel_->setScale(0.5f);
	optionsLabel_->setAnchorPoint({0.0f, 0.5f});

	// the overlay can create a state where the speedhack is disabled but shown as a non disabled value
	// so just create an inactive state just in case
	auto speedhackEnabled = GameManager::sharedState()->getGameVariable(GameVariable::OVERLAY_SPEEDHACK_ENABLED);
	if (speedhackEnabled || speedhack_interval == 1.0f) {
		optionsLabel_->setColor({0xff, 0xff, 0xff});
	} else {
		optionsLabel_->setColor({0xbb, 0xbb, 0xbb});
	}

	return true;
}
