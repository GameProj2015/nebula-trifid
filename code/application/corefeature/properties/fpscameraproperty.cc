//------------------------------------------------------------------------------
//  properties/cameraproperty.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2014 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "fpscameraproperty.h"
#include "managers/focusmanager.h"
#include "game/entity.h"
#include "graphics/graphicsserver.h"
#include "graphicsfeature/graphicsfeatureunit.h"
#include "graphics/cameraentity.h"
#include "graphics/view.h"
#include "graphics/stage.h"
#include "graphicsfeature/graphicsfeatureprotocol.h"
#include "basegamefeature/basegameattr/basegameattributes.h"
#include "faudio/audiolistener.h"

namespace FPSCameraFeature
{
__ImplementClass(FPSCameraFeature::FPSCameraProperty, 'FPSC', Game::Property);

using namespace Game;
using namespace Math;
using namespace BaseGameFeature;

//------------------------------------------------------------------------------
/**
*/
FPSCameraProperty::FPSCameraProperty()
{
    
}

//------------------------------------------------------------------------------
/**
*/
FPSCameraProperty::~FPSCameraProperty()
{

}

//------------------------------------------------------------------------------
/**
*/
void
FPSCameraProperty::SetupCallbacks()
{
}

//------------------------------------------------------------------------------
/**
*/
void
FPSCameraProperty::SetupAcceptedMessages()
{
    Game::Property::SetupAcceptedMessages();
}

//------------------------------------------------------------------------------
/**
*/
void
FPSCameraProperty::OnDeactivate()
{
    // clear camera focus, if we are the camera focus object
    if (this->HasFocus())
    {
        FocusManager::Instance()->SetCameraFocusEntity(0);
    }
    
    Property::OnDeactivate();
}

//------------------------------------------------------------------------------
/**
*/
void
FPSCameraProperty::HandleMessage(const Ptr<Messaging::Message>& msg)
{

}
}; // namespace GraphicsFeature
