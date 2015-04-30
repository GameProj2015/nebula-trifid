#pragma once

#include "graphics/spotlightentity.h"
#include "graphics/modelentity.h"
#include "graphics/globallightentity.h"
#include "graphics/pointlightentity.h"
#include "graphicsfeatureunit.h"
#include "physicsfeatureunit.h"
#include "basegamefeatureunit.h"
#include "appgame/gameapplication.h"
#include "scriptingfeature/scriptingfeature.h"
#include "qttoolkit/qtaddons/remoteinterface/qtremoteserver.h"
#include "qttoolkit/qtaddons/remoteinterface/qtremoteclient.h"
#include "effects/effectsfeatureunit.h"
#include "ui/uifeatureunit.h"
#include "posteffect/posteffectfeatureunit.h"
#include "dynui/imguiaddon.h"
#include "dynui/console/imguiconsole.h"
#include "dynui/console/imguiconsolehandler.h"
#include "levelviewerfactorymanager.h"

//------------------------------------------------------------------------------
/**
    @class Toolkit::LevelViewer
    
    Nebula3 level viewer application

    (C) 2013-2015 Individual contributors, see AUTHORS file
*/
namespace  Tools
{
class LevelViewerGameStateApplication : public App::GameApplication
{
public:
	/// constructor
		LevelViewerGameStateApplication(void);
	/// destructor
		virtual ~LevelViewerGameStateApplication(void);
	/// open application
	virtual bool Open();
	/// close application
	virtual void Close();

private:

	/// setup application state handlers
	virtual void SetupStateHandlers();
	/// setup game features
	virtual void SetupGameFeatures();
	/// cleanup game features
	virtual void CleanupGameFeatures();

	Ptr<QtRemoteInterfaceAddon::QtRemoteServer> remoteServer;
	Ptr<QtRemoteInterfaceAddon::QtRemoteClient> remoteClient;
    
	Ptr<PhysicsFeature::PhysicsFeatureUnit> physicsFeature;
	Ptr<GraphicsFeature::GraphicsFeatureUnit> graphicsFeature;
	Ptr<BaseGameFeature::BaseGameFeatureUnit> baseGameFeature;
	Ptr<ScriptingFeature::ScriptingFeatureUnit> scriptingFeature;
	Ptr<EffectsFeature::EffectsFeatureUnit> effectFeature;
	Ptr<UI::UiFeatureUnit> uiFeature;
	Ptr<PostEffect::PostEffectFeatureUnit> postEffectFeature;

	Ptr<Dynui::ImguiAddon> imgui;	
	Ptr<Dynui::ImguiConsole> console;
	Ptr<Dynui::ImguiConsoleHandler> consoleHandler;
};

}
