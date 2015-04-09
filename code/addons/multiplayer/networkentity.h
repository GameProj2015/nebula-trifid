#pragma once
//------------------------------------------------------------------------------
/**
@class Multiplayer::NetworkEntity

The networked game entity class. 

(C) 2015 2015 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/ptr.h"
#include "game/entity.h"
#include "ReplicaManager3.h"

#define __SendNetworkMessage(OBJ, MSG) n_assert2(OBJ->IsA(MultiplayerFeature::NetworkEntity::RTTI.GetFourCC()), "Not a NetworkEntity");OBJ.cast<MultiplayerFeature::NetworkEntity>()->SendNetwork(MSG.cast<Messaging::Message>());
#define __DistributeNetworkMessage(OBJ,MSG) if(MSG->IsLocal() && OBJ->IsA(MultiplayerFeature::NetworkEntity::RTTI.GetFourCC())){OBJ.cast<MultiplayerFeature::NetworkEntity>()->SendNetwork(MSG.cast<Messaging::Message>());}
		


//------------------------------------------------------------------------------
namespace MultiplayerFeature
{

class NetworkEntity : public Game::Entity, public RakNet::Replica3
{
	__DeclareClass(NetworkEntity);
public:
	/// constructor
	NetworkEntity();
	/// destructor
	virtual ~NetworkEntity();

	/// called when attached to world
	virtual void OnActivate();
	/// called when removed from world
	virtual void OnDeactivate();

	/// send a message over the network to the mirrored entities
	void SendNetwork(const Ptr<Messaging::Message> & message);
	/// is system the creator of the entity
	/// note: this is only valid after onactivate has been called
	bool IsCreator();


	/// raknet sync stuff below many of these dont need to be overloaded in this case but are pure virtual
	///
	virtual void WriteAllocationID(RakNet::Connection_RM3 *destinationConnection, RakNet::BitStream *allocationIdBitstream) const;
	///
	virtual RakNet::RM3ConstructionState QueryConstruction(RakNet::Connection_RM3 *destinationConnection, RakNet::ReplicaManager3 *replicaManager3);
	///
	virtual bool QueryRemoteConstruction(RakNet::Connection_RM3 *sourceConnection);
	///
	virtual void SerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection);
	///
	virtual void SerializeConstructionExisting(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection);
	///
	virtual void PostDeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection);
	///
	virtual bool DeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection);
	///
	virtual void DeserializeConstructionExisting(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection);
	///
	virtual void SerializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *destinationConnection);
	///
	virtual bool DeserializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *sourceConnection);
	///
	virtual RakNet::RM3ActionOnPopConnection QueryActionOnPopConnection(RakNet::Connection_RM3 *droppedConnection) const;
	///
	virtual void DeallocReplica(RakNet::Connection_RM3 *sourceConnection);
	///
	virtual RakNet::RM3QuerySerializationResult QuerySerialization(RakNet::Connection_RM3 *destinationConnection);
	///
	virtual RakNet::RM3SerializationResult Serialize(RakNet::SerializeParameters *serializeParameters);
	///
	virtual void Deserialize(RakNet::DeserializeParameters *deserializeParameters);
private:
	/// determine if serialization is required because of configuration (does not take queued messages into account)
	RakNet::RM3QuerySerializationResult QuerySerialize(RakNet::Connection_RM3 *destinationConnection);

	bool levelEntity;	
	Util::Array<Ptr<Messaging::Message>> queuedMessages;
};

}// namespace MultiplayerFeature
//------------------------------------------------------------------------------

