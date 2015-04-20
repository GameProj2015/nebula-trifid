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
#include "basegamefeature/basegameprotocol.h"
#include "input/mouse.h"
#include "corefeature/coreattr/coreattributes.h"

#include "graphics/modelentity.h"
#include "characters/character.h"
#include "characters/base/skinnedmeshrendererbase.h"
#include "graphicsfeature/managers/attachmentmanager.h"
#include "graphicsfeature/graphicsattr/graphicsattributes.h"

namespace FPSCameraFeature
{
__ImplementClass(FPSCameraFeature::FPSCameraProperty, 'FPSC', GraphicsFeature::CameraProperty);

using namespace Game;
using namespace Math;
using namespace Input;
using namespace BaseGameFeature;
using namespace GraphicsFeature;

//------------------------------------------------------------------------------
/**
*/
FPSCameraProperty::FPSCameraProperty()
{
	this->cameraEntity = Graphics::CameraEntity::Create();
	rotx = 0;
	roty = 0;
	fov = 0;
	closeplane = 0;
	farplane = 0;
	sensitivity = 0;
}

//------------------------------------------------------------------------------
/**
*/
FPSCameraProperty::~FPSCameraProperty()
{
	this->cameraEntity = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
FPSCameraProperty::SetupCallbacks()
{
	this->entity->RegisterPropertyCallback(this, Render);
	this->entity->RegisterPropertyCallback(this, BeginFrame);
}

//------------------------------------------------------------------------------
/**
*/
void
FPSCameraProperty::SetupAcceptedMessages()
{
	this->RegisterMessage(CameraFocus::Id);
	this->RegisterMessage(SetTransform::Id);
	Game::Property::SetupAcceptedMessages();
}

//------------------------------------------------------------------------------
/**
*/
void
FPSCameraProperty::OnStart()
{
	Property::OnStart();

	if (this->entity->GetBool(Attr::CameraFocus))
	{
		FocusManager::Instance()->SetCameraFocusEntity(this->entity);
	}
	head = this->entity->GetString(Attr::HeadJoint);
	fov = this->entity->GetFloat(Attr::Fov);
	closeplane = this->entity->GetFloat(Attr::ClosePlane);
	farplane = this->entity->GetFloat(Attr::FarPlane);
	sensitivity = this->entity->GetFloat(Attr::Sensitivity);

	Ptr<GraphicsFeature::GetModelEntity> msg = GraphicsFeature::GetModelEntity::Create();
	__SendSync(this->entity, msg);
	modelEntity = msg->GetEntity();

	//Hide this model
	//TODO: SPANK GUGGE TO FIX THIS!
	Ptr<GraphicsFeature::SetSkinVisible> msg_hide = GraphicsFeature::SetSkinVisible::Create();
	msg_hide->SetVisible(false);
	msg_hide->SetSkin("dummyChar");
	__SendSync(this->entity, msg_hide);

	//Camera settings
	Graphics::CameraSettings settings = this->cameraEntity->GetCameraSettings();
	float aspect = settings.GetAspect();
	settings.SetupPerspectiveFov(n_deg2rad(fov), aspect, closeplane, farplane);
	this->cameraEntity->SetCameraSettings(settings);

	//Locks and hide the cursor
	InputServer::Instance()->SetCursorLocked(true);
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
	n_assert(msg != 0);

	if (msg->CheckId(CameraFocus::Id))
	{
		const Ptr<CameraFocus>& focusMsg = msg.downcast<CameraFocus>();
		if (focusMsg->GetObtainFocus())
		{
			this->OnObtainCameraFocus();
		}
		else
		{
			this->OnLoseCameraFocus();
		}
	}
	Property::HandleMessage(msg);
}

//------------------------------------------------------------------------------
/**
This method is called by the FocusManager when our entity gains the
camera focus. Override this method if your subclass needs to do
some initialization when gaining the camera focus.
*/
void
FPSCameraProperty::OnObtainCameraFocus()
{
	// update focus attribute
	this->entity->SetBool(Attr::CameraFocus, true);
	this->cameraEntity->SetTransform(this->GetEntity()->GetMatrix44(Attr::Transform));

	this->defaultStage = GraphicsFeature::GraphicsFeatureUnit::Instance()->GetDefaultStage();
	this->defaultView = GraphicsFeature::GraphicsFeatureUnit::Instance()->GetDefaultView();
	this->defaultStage->AttachEntity(this->cameraEntity.cast<Graphics::GraphicsEntity>());
	this->defaultView->SetCameraEntity(this->cameraEntity);
}

//------------------------------------------------------------------------------
/**
This method is called by the FocusManager when our entity loses
the camera focus. Override this method if your subclass needs to do any
cleanup work here.
*/
void
FPSCameraProperty::OnLoseCameraFocus()
{
	// clear cam in default view
	if (this->defaultView->GetCameraEntity() == this->cameraEntity)
	{
		this->defaultView->SetCameraEntity(0);
	}
	this->defaultStage->RemoveEntity(this->cameraEntity.cast<Graphics::GraphicsEntity>());
	this->defaultStage = 0;
	this->defaultView = 0;

	// update focus attribute
	this->entity->SetBool(Attr::CameraFocus, false);
}

//------------------------------------------------------------------------------
/**
This method returns true if our entity has the camera focus. This
implementation makes sure that 2 properties cannot report that they
have the camera focus by accident.
*/
bool
FPSCameraProperty::HasFocus() const
{
	return this->entity->GetBool(Attr::CameraFocus);
}

//------------------------------------------------------------------------------
/**
Called before camera is "rendered". The default camera properties
applies shake effects to the camera.
*/
void
FPSCameraProperty::OnRender()
{
	// do just, if we got focus
	if (FocusManager::Instance()->GetCameraFocusEntity() == this->entity)
	{

		// update audio
		this->UpdateAudioListenerPosition();

		// get transform
		const Math::matrix44& trans = this->entity->GetMatrix44(Attr::Transform);

		// set point of interest in post effect manager
		PostEffect::PostEffectManager::Instance()->SetPointOfInterest(trans.get_position());
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FPSCameraProperty::UpdateAudioListenerPosition() const
{
	matrix44 transform = this->cameraEntity->GetTransform();
	if (this->GetEntity()->HasAttr(Attr::Transform))
	{
		const matrix44& enityTransform = this->GetEntity()->GetMatrix44(Attr::Transform);
		transform.translate((enityTransform.get_position() - transform.get_position()) * 0.5f);
	}
	FAudio::AudioListener::Instance()->SetTransform(transform);
}

void FPSCameraProperty::OnObtainInputFocus()
{

}

void FPSCameraProperty::OnLoseInputFocus()
{

}

void FPSCameraProperty::OnBeginFrame()
{
	// only do something if we have the input focus
	if (FocusManager::Instance()->GetInputFocusEntity() == this->entity)
	{
		InputServer* inputServer = InputServer::Instance();
		const Ptr<Mouse>& mouse = inputServer->GetDefaultMouse();

		Math::float2 mouseMovement = mouse->GetMovement();
		Math::float2 screenPos = mouse->GetScreenPosition();

		n_printf("x: %f          y:%f", mouseMovement.x(), mouseMovement.y());

		if(modelEntity->HasCharacter())
		{
			headIndex = modelEntity->GetCharacter()->Skeleton().GetJointIndexByName(head);
			Math::vector headPos = GetJointPos(headIndex);
			//This method is alot more responsive in havok
			#if(__USE_HAVOK__)	
				Ptr<SetTransform> msg = SetTransform::Create();
				Math::matrix44 trans = this->cameraEntity->GetTransform();
				Math::float4 pos = this->entity->GetMatrix44(Attr::Transform).get_position();
				trans.set_position(0);
				if (mouseMovement.x() != 0 || mouseMovement.y() != 0)
				{
					rotx += mouseMovement.y() * this->sensitivity;
					roty += mouseMovement.x() * this->sensitivity;
					trans = matrix44::multiply(Math::matrix44::rotationx(rotx), Math::matrix44::rotationy(roty));
				}
				Math::matrix44 test = Math::matrix44::rotationx(roty);
				test.set_position(pos);
				msg->SetMatrix(test);
				this->entity->SendSync(msg.cast<Messaging::Message>());

				trans.set_position(pos + headPos);
				this->cameraEntity->SetTransform(trans);
			//Above method do not work in havok, hence this method is necessary, REQUIRES ACTORPHYSICS!
			#elif (__USE_BULLET__)
				//Send rotation message to entity
				Ptr<MoveRotate> rot_msg = MoveRotate::Create();
				float rotsx = mouseMovement.x() * sensitivity;
				rot_msg->SetAngle(rotsx);
				__SendSync(this->entity, rot_msg);

				//Now rotate camera 
				Math::matrix44 trans = this->entity->GetMatrix44(Attr::Transform);
				trans.set_position(trans.get_position() + headPos);
				if (mouseMovement.x() != 0 || mouseMovement.y() != 0)
					rotx += mouseMovement.y() * sensitivity;
				this->cameraEntity->SetTransform(Math::matrix44::multiply(Math::matrix44::rotationx(rotx) ,trans));
			#endif
		}
	}

}

Math::vector FPSCameraProperty::GetJointPos(IndexT index)
{
	//Get joint position
	const Ptr<Characters::CharacterInstance>& charInst = modelEntity.cast<Graphics::ModelEntity>()->GetCharacterInstance();
	charInst->WaitUpdateDone();
	return charInst->Skeleton().GetJointMatrix(index).get_position();
}

}; // namespace GraphicsFeature
