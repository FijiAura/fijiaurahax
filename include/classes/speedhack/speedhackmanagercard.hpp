#pragma once

#ifndef CLASSES_SPEEDHACK_SPEEDHACKMANAGERCARD_HPP
#define CLASSES_SPEEDHACK_SPEEDHACKMANAGERCARD_HPP

#include <Geode/Geode.hpp>
#include <fmt/format.h>

#include "base/game_variables.hpp"

#include "speedhackcarddelegate.hpp"

class SpeedhackManagerCard : public cocos2d::CCNode {
private:
	cocos2d::CCLabelBMFont* optionsLabel_{nullptr};
	cocos2d::CCMenu* itemsMenu_{nullptr};

	SpeedhackCardDelegate* delegate_{nullptr};

public:
	void updateSpeedhackLabel();

	void onBtnDown(cocos2d::CCObject * /* target */);
	void onBtnUp(cocos2d::CCObject * /* target */);

	void setDelegate(SpeedhackCardDelegate*);
	void fixPriority();

	bool init() override;

	CREATE_FUNC(SpeedhackManagerCard);
};

#endif //CLASSES_SPEEDHACK_SPEEDHACKMANAGERCARD_HPP
