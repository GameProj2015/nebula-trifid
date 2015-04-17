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

#include "game/property.h"
#include "graphicsfeature/graphicsattr/graphicsattributes.h"
#include "graphicsfeatureproperties.h"

namespace FPSCameraFeature
{
	class FPSCameraProperty : public GraphicsFeature::CameraProperty
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
		/// called from within Entity::OnStart() after OnLoad when the complete world exist
		virtual void OnStart();
		/// called when camera focus is obtained
		virtual void OnObtainCameraFocus();
		/// called when camera focus is lost
		virtual void OnLoseCameraFocus();
		/// called before rendering happens
		virtual void OnRender();
		/// return true if currently has camera focus
		virtual bool HasFocus() const;
		/// handle a single message
		virtual void HandleMessage(const Ptr<Messaging::Message>& msg);
		/// called when input focus is gained
		virtual void OnObtainInputFocus();
		/// called when input focus is lost
		virtual void OnLoseInputFocus();
		/// return true if currently has input focus
		/// called on begin of frame
		virtual void OnBeginFrame();
	protected:

		///Get joint position by index
		Math::vector GetJointPos(IndexT index);

		/// update audio listener position
		void UpdateAudioListenerPosition() const;
		Ptr<Graphics::ModelEntity> modelEntity;
		float rotx, roty, fov, closeplane, farplane, sensitivity;
		Util::StringAtom head;
		Util::String hip;
		IndexT headIndex;
		IndexT hipIndex;
		Ptr<Game::Entity> ent;
	};
	__RegisterClass(FPSCameraProperty);

}; // namespace FPSCameraFeature
//------------------------------------------------------------------------------
