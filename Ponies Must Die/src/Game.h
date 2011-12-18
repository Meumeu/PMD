#ifndef GAME_H
#define GAME_H

#include <Ogre.h>
#include <OIS.h>

namespace pmd
{
	class Game :
		public Ogre::FrameListener,
		public Ogre::WindowEventListener,
		public OIS::KeyListener,
		public OIS::MouseListener
	{
	public:
		Game(void);
		~Game(void);
		
		void go(void);
	protected:
		// Ogre::FrameListener
		virtual bool frameRenderingQueued(const Ogre::FrameEvent& evt);

		// OIS::KeyListener
		virtual bool keyPressed(const OIS::KeyEvent &arg);
		virtual bool keyReleased(const OIS::KeyEvent &arg);
		// OIS::MouseListener
		virtual bool mouseMoved(const OIS::MouseEvent &arg);
		virtual bool mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
		virtual bool mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id);

		virtual void windowResized(Ogre::RenderWindow * rw);
		virtual void windowClosed(Ogre::RenderWindow* rw);

	private:
		bool setup(void);
		void cleanup(void);
		void setupResources(void);
		void setupFrameListener(void);

		Ogre::Root * mRoot;
		Ogre::Camera * mCamera;
		Ogre::SceneManager * mSceneMgr;
		Ogre::RenderWindow * mWindow;
		Ogre::Viewport * mViewport;

		//OIS Input devices
		OIS::InputManager* mInputManager;
		OIS::Mouse*    mMouse;
		OIS::Keyboard* mKeyboard;

		Ogre::String mResourcesCfg;
		Ogre::String mPluginsCfg;
		Ogre::String mOgreCfg;
		Ogre::String mOgreLog;

		bool mShutdown;
	};
}

#endif // GAME_H
