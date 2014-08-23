#ifndef __TutorialApplication_h_
#define __TutorialApplication_h_

#include "BaseApplication.h"
#include "PolygonRaycast.hpp"

// CEGUI
#include <CEGUI/CEGUI.h>
#include <CEGUI/System.h>
#include <CEGUI/SchemeManager.h>
#include <CEGUI/RendererModules/Ogre/Renderer.h>


class TutorialApplication : public BaseApplication
{
public:
	TutorialApplication(void);
	virtual ~TutorialApplication(void);

protected:
	virtual void createScene(void);
	virtual void createFrameListener(void);
	// Mouse listener
	virtual bool mouseMoved(const OIS::MouseEvent &arg);
	virtual bool mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
	virtual bool mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id);

private:
	Ogre::RaySceneQuery *mRaySceneQuery;// The ray scene query pointer
	bool mLMouseDown, mRMouseDown;		// True if the mouse buttons are down
	Ogre::SceneNode *mCurrentObject;	// The newly created object
	CEGUI::Renderer *mGUIRenderer;		// CEGUI renderer
	float mRotateSpeed;
};

#endif // #ifndef __TutorialApplication_h_
