//------------------------------------------------------------------------------
//  ogl4instanceserver.cc
//  (C) 2013 gscept
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "ogl4instanceserver.h"
#include "ogl4instancerenderer.h"
#include "models/nodes/transformnodeinstance.h"
#include "framesync/framesynctimer.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/shaderserver.h"
#include "models/modelinstance.h"

using namespace Util;
using namespace OpenGL4;
using namespace CoreGraphics;
using namespace Models;
namespace Instancing
{
__ImplementSingleton(Instancing::OGL4InstanceServer);
__ImplementClass(Instancing::OGL4InstanceServer, 'O4IS', Instancing::InstanceServerBase);

//------------------------------------------------------------------------------
/**
*/
OGL4InstanceServer::OGL4InstanceServer()
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
OGL4InstanceServer::~OGL4InstanceServer()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
bool 
OGL4InstanceServer::Open()
{
	if (InstanceServerBase::Open())
	{
		this->renderer = OGL4InstanceRenderer::Create();
		this->instancingFeatureBits = ShaderServer::Instance()->FeatureStringToMask("Instanced");
		return true;
	}
	return false;
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4InstanceServer::Close()
{
	this->renderer = 0;
	InstanceServerBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4InstanceServer::Render()
{
	n_assert(this->IsOpen());
	n_assert(this->modelNode.isvalid());
	n_assert(this->renderer.isvalid());

	// get render device
	RenderDevice* renderDev = RenderDevice::Instance();
	ShaderServer* shaderServer = ShaderServer::Instance();

	// get frame index
	IndexT frameIndex = FrameSync::FrameSyncTimer::Instance()->GetFrameIndex();

	// abort early if we don't have any instances
	if (this->instancesByCode.Size() == 0)
	{
		return;
	}

	// set shader in renderer
	const Ptr<ShaderInstance>& shader = shaderServer->GetActiveShaderInstance();
	this->renderer->SetShader(shader);

	// get string of active variation
	shaderServer->SetFeatureBits(this->instancingFeatureBits);
	shader->SelectActiveVariation(shaderServer->GetFeatureBits());

	// begin pass of shader
	shader->Begin();
	shader->BeginPass(0);
	shader->SetWireframe(renderDev->GetRenderWireframe());

	// go through each code instance, update transforms in batches based on their code
	IndexT codeIndex;
	for (codeIndex = 0; codeIndex < this->instancesByCode.Size(); codeIndex++)
	{
		// get array of nodes
		const Array<Ptr<ModelNodeInstance> >& nodeInstances = this->instancesByCode.ValueAtIndex(codeIndex);

		// start by activating renderer
		this->renderer->BeginUpdate(nodeInstances.Size());

		// go through each model instance and add it to renderer
		IndexT i;
		for (i = 0; i < nodeInstances.Size(); i++)
		{
			// get node
			const Ptr<ModelNodeInstance>& nodeInstance = nodeInstances[i];

			// upcast to transform node
			Ptr<TransformNodeInstance> transNode = nodeInstance.downcast<TransformNodeInstance>();

			// add transform to renderer
			this->renderer->AddTransform(transNode->GetModelTransform());		
            this->renderer->AddId(transNode->GetModelInstance()->GetPickingId());
		}

		// finish updating transforms
		this->renderer->EndUpdate();

		// apply the state for the first node, this will then be active for all of them
		nodeInstances[0]->ApplyState();

		// now render
		this->renderer->Render(this->multiplier);
	}

	// end pass of shader
	shader->EndPass();
	shader->End();
}
} // namespace Instancing