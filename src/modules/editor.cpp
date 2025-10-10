#include <Geode/Geode.hpp>
#include <Geode/modify/CCControlUtils.hpp>
#include <Geode/modify/DrawGridLayer.hpp>

#include "base/game_variables.hpp"

struct YellowCCControlUtils : geode::Modify<YellowCCControlUtils, cocos2d::extension::CCControlUtils> {
	static cocos2d::extension::RGBA RGBfromHSV(cocos2d::extension::HSV c) {
		if (std::isnan(c.h))
			c.h = 0.0f;

		if (std::isnan(c.s))
			c.s = 0.0f;

		if (std::isnan(c.v))
			c.v = 0.0f;

		return cocos2d::extension::CCControlUtils::RGBfromHSV(c);
	}
};

struct FixDrawGridLayer : geode::Modify<FixDrawGridLayer, DrawGridLayer> {
	void loadTimeMarkers(gd::string markers) {
		DrawGridLayer::loadTimeMarkers(markers);

		// another bugfix, xPosForTime does not properly handle custom start speeds unless guidelines are set
		auto startSpeed = m_levelEditorLayer->m_levelSettings->m_startSpeed;

		switch (startSpeed) {
			case 0:
			default:
				this->m_guidelineSpacing = this->m_normalGuidelineSpacing;
				break;
			case 1:
				this->m_guidelineSpacing = this->m_slowGuidelineSpacing;
				break;
			case 2:
				this->m_guidelineSpacing = this->m_fastGuidelineSpacing;
				break;
			case 3:
				this->m_guidelineSpacing = this->m_fasterGuidelineSpacing;
				break;
		}
	}
};
