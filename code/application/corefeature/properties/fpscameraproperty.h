#pragma once
//------------------------------------------------------------------------------
/**
    @class GraphicsFeature::CameraProperty

    A camera property adds the ability to manipulate the camera to an entity.
    Please note that more advanced camera properties should always be 
    derived from the class camera property if camera focus handling is desired,
    since the FocusManager will only work on game entities which have
    a CameraProperty (or a subclass) attached.

    It is completely ok though to handle camera manipulation in a property
    not derived from CameraProperty, but please be aware that the
    FocusManager will ignore those.

    The camera property will generally 
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2014 Individual contributors, see AUTHORS file
*/

#include "basegamefeature/managers/factorymanager.h"
#include "game/property.h"
#include "basegamefeature/basegameprotocol.h"
#include "graphicsfeature/graphicsattr/graphicsattributes.h"

namespace FPSCameraFeature
{
class FPSCameraProperty : public Game::Property
{
	__DeclareClass(FPSCameraProperty);
	__SetupExternalAttributes();
public:
    /// constructor
    FPSCameraProperty();
    /// destructor
    virtual ~FPSCameraProperty();
    /// called from Entity::DeactivateProperties()
    virtual void OnDeactivate();
    /// setup accepted messages
    virtual void SetupAcceptedMessages();
    /// setup callbacks for this property
    virtual void SetupCallbacks();    
    /// return true if currently has camera focus
    virtual bool HasFocus() const;    
    /// handle a single message
    virtual void HandleMessage(const Ptr<Messaging::Message>& msg);
protected:
};
__RegisterClass(FPSCameraProperty);

}; // namespace GraphicsFeature
//------------------------------------------------------------------------------
