#pragma once

#include <Geode/Geode.hpp>

struct CheckpointObjectExt : cocos2d::CCObject {
	GameObject* m_portalRef;
	GameObject* m_dualPortalRef;

	float m_songTime;
};
