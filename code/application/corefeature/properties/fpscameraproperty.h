#pragma once
//------------------------------------------------------------------------------
/**
    @class FPSCameraFeature::FPSCameraProperty

    FPS Camera property

	Requires ActorPhysics or an property that can handle MoveRotate messages!

	(C) Patrik Nyman, Mariusz Waclawek
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
		Math::matrix44 GetJointPos(IndexT index);

		/// update audio listener position
		void UpdateAudioListenerPosition() const;
		Ptr<Graphics::ModelEntity> modelEntity;
		float rotx, fov, closeplane, farplane, sensitivity, ylimit, rotOffset;
		Util::StringAtom head;
		Util::String hip;
		IndexT headIndex;
		IndexT hipIndex;
		Ptr<Game::Entity> ent;
		/// debugging
		bool debug;
	};
	__RegisterClass(FPSCameraProperty);

}; // namespace FPSCameraFeature
//------------------------------------------------------------------------------
