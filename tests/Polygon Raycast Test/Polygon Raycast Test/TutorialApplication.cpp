#include "TutorialApplication.h"

//-------------------------------------------------------------------------------------
TutorialApplication::TutorialApplication(void) :
mCurrentObject(nullptr),
mLMouseDown(false),
mRMouseDown(false),
// Reduce rotate speed
mRotateSpeed(0.1f)
{
}
//-------------------------------------------------------------------------------------
TutorialApplication::~TutorialApplication(void)
{
	// We created the query, and we are also responsible for deleting it.
	mSceneMgr->destroyQuery(mRaySceneQuery);
}

//-------------------------------------------------------------------------------------
void TutorialApplication::createScene(void)
{
	// CEGUI setup
	mGUIRenderer = &CEGUI::OgreRenderer::bootstrapSystem();

	// Mouse
	CEGUI::SchemeManager::getSingleton().createFromFile("TaharezLook.scheme");
	CEGUI::System::getSingleton().getDefaultGUIContext().getMouseCursor().setImage("TaharezLook/MouseArrow");


	// Set the scene's ambient light
	mSceneMgr->setAmbientLight(Ogre::ColourValue(0.5f, 0.5f, 0.5f));

	// Create an Entity
	Ogre::Entity* ogreHead = mSceneMgr->createEntity("Head", "ogrehead.mesh");

	// Create a SceneNode and attach the Entity to it
	Ogre::SceneNode* headNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("HeadNode");
	headNode->attachObject(ogreHead);

	// Create a Light and set its position
	Ogre::Light* light = mSceneMgr->createLight("MainLight");
	light->setPosition(20.0f, 80.0f, 50.0f);
}


//
// createFrameListener
//
void TutorialApplication::createFrameListener(void)
{
	BaseApplication::createFrameListener();

	// Create RaySceneQuery
	mRaySceneQuery = mSceneMgr->createRayQuery(Ogre::Ray());
}


//
// mouseMoved
//
bool TutorialApplication::mouseMoved(const OIS::MouseEvent &arg)
{
	// Update CEGUI with the mouse motion
	float sensitivity = 2.0f;
	CEGUI::System &sys = CEGUI::System::getSingleton();
	sys.getDefaultGUIContext().injectMouseMove(arg.state.X.rel * sensitivity, arg.state.Y.rel * sensitivity);

	// If we are dragging the right mouse button.
	if (mRMouseDown)
	{
		mCamera->yaw(Ogre::Degree(-arg.state.X.rel * mRotateSpeed));
		mCamera->pitch(Ogre::Degree(-arg.state.Y.rel * mRotateSpeed));
	}

	return true;
}


//
// mousePressed
//
bool TutorialApplication::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
	//show that the current object has been deselected by removing the bounding box visual
	if (mCurrentObject)
	{
		mCurrentObject->showBoundingBox(false);
	}


	// Left mouse button down
	if (id == OIS::MB_Left)
	{
		// Setup the ray scene query, use CEGUI's mouse position
		CEGUI::Vector2f mousePos = CEGUI::System::getSingleton().getDefaultGUIContext().getMouseCursor().getPosition();
		Ogre::Ray mouseRay = mCamera->getCameraToViewportRay(mousePos.d_x / float(arg.state.width), mousePos.d_y / float(arg.state.height));
		mRaySceneQuery->setSortByDistance(true);
		mRaySceneQuery->setQueryTypeMask(Ogre::SceneManager::ENTITY_TYPE_MASK);

		// Reset the current object
		mCurrentObject = nullptr;
		Ogre::Vector3 point;

		OgreUtility::polygonRaycast(mRaySceneQuery, mouseRay, mCurrentObject, point);

		mLMouseDown = true;
	}

	// Right mouse button down
	else if (id == OIS::MB_Right)
	{
		CEGUI::System::getSingleton().getDefaultGUIContext().getMouseCursor().hide();
		mRMouseDown = true;
	}


	//now we show the bounding box so the user can see that this object is selected
	if (mCurrentObject)
	{
		mCurrentObject->showBoundingBox(true);
	}

	return true;
}


//
// mouseReleased
//
bool TutorialApplication::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
	// Left mouse button up
	if (id == OIS::MB_Left)
	{
		mLMouseDown = false;
	} // if

	// Right mouse button up
	else if (id == OIS::MB_Right)
	{
		CEGUI::System::getSingleton().getDefaultGUIContext().getMouseCursor().show();
		mRMouseDown = false;
	} // else if

	return true;
}





#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT)
#else
	int main(int argc, char *argv[])
#endif
	{
		// Create application object
		TutorialApplication app;

		try {
			app.go();
		}
		catch (Ogre::Exception& e) {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
			MessageBox(NULL, e.getFullDescription().c_str(), "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
			std::cerr << "An exception has occured: " <<
				e.getFullDescription().c_str() << std::endl;
#endif
		}

		return 0;
	}

#ifdef __cplusplus
}
#endif
