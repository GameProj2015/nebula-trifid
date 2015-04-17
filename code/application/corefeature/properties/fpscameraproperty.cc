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
	hip = this->entity->GetString(Attr::HipJoint);
	fov = this->entity->GetFloat(Attr::Fov);
	closeplane = this->entity->GetFloat(Attr::ClosePlane);
	farplane = this->entity->GetFloat(Attr::FarPlane);
	sensitivity = this->entity->GetFloat(Attr::Sensitivity);
	//Debug code
	//Util::String entName = this->entity->GetString(Attr::CharacterId);
	//Attr::Attribute attr = Attr::Attribute(Attr::Id, entName);
	//ent = BaseGameFeature::EntityManager::Instance()->GetEntityByAttr(attr);
	//n_assert(ent!=NULL && ent.isvalid());
	//End Debug	

	Ptr<GraphicsFeature::GetModelEntity> msg = GraphicsFeature::GetModelEntity::Create();
	__SendSync(this->entity, msg);
	modelEntity = msg->GetEntity();
	/*headIndex = modelEntity->GetCharacter()->Skeleton().GetJointIndexByName(head);
	hipIndex = modelEntity->GetCharacter()->Skeleton().GetJointIndexByName(hip);*/

		

	//Translate camera to joint pos
		
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
		/* ========================================================================================= NEB 2 STUFF
		TODO!?!?! shaker effect stuff

		this->shakeEffectHelper.SetCameraTransform(camera->GetTransform());
		this->shakeEffectHelper.Update();
		camera->SetTransform(this->shakeEffectHelper.GetShakeCameraTransform());

		// if enity has transform set the current position between camera and entity as audio listener position
		// otherwise only use camera transform

		=========================================================================================== NEB 2 STUFF */

		// update audio
		this->UpdateAudioListenerPosition();

		// get transform
		const Math::matrix44& trans = this->entity->GetMatrix44(Attr::Transform);

		// set point of interest in post effect manager
		PostEffect::PostEffectManager::Instance()->SetPointOfInterest(trans.get_position());

		// apply transform		
		//this->cameraEntity->SetTransform(trans);
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

		n_printf("\nMouse movement: %f %f", mouseMovement.x(), mouseMovement.y());
		n_printf("\nMouse screen pos: %f %f", screenPos.x(), screenPos.y());

		if(modelEntity->HasCharacter())
		{
		headIndex = modelEntity->GetCharacter()->Skeleton().GetJointIndexByName(head);
		Math::vector headPos = GetJointPos(headIndex);
		Ptr<SetTransform> msg = SetTransform::Create();
		Math::matrix44 trans = this->cameraEntity->GetTransform();
		Math::float4 pos = this->entity->GetMatrix44(Attr::Transform).get_position();
		trans.set_position(0); //
		this->sensitivity = 0.003f;
		if (mouseMovement.x() != 0 || mouseMovement.y() != 0)
		{
			//Math::float4 dir = trans.get_zaxis();
			//Math::float2 normalizedMouseMovement = Math::float2::normalize(mouseMovement);
			////Math::float4 upAxis = trans.get_yaxis();
			////Math::float4 sideAxis = trans.get_xaxis();
			rotx += mouseMovement.y()*sensitivity;
			roty += mouseMovement.x()*sensitivity;
			trans = matrix44::multiply(Math::matrix44::rotationx(rotx), Math::matrix44::rotationy(roty));
		}
		trans.set_position(pos);
		msg->SetMatrix(trans);
		this->entity->SendSync(msg.cast<Messaging::Message>());
		trans.set_position(pos + headPos);
		this->cameraEntity->SetTransform(trans);

		/*msg->SetMatrix(trans);
		this->entity->SendSync(msg.cast<Messaging::Message>());*/
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
