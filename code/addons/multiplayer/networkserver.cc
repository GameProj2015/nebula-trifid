//------------------------------------------------------------------------------
//  networkserver.cc
//  (C) 2015 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "networkserver.h"
#include "debug/debugserver.h"            
#include "http/html/htmlpagewriter.h"
#include "http/svg/svglinechartwriter.h"
#include "threading/thread.h"
#include <time.h>
#include "FullyConnectedMesh2.h"
#include "NetworkIDManager.h"
#include "RakPeerInterface.h"
#include "TCPInterface.h"
#include "NatPunchthroughClient.h"
#include "RPC4Plugin.h"
#include "ReadyEvent.h"
#include "HTTPConnection2.h"
#include "networkgame.h"
#include "MessageIdentifiers.h"
#include "PacketLogger.h"
#include "networkplayer.h"
#include "multiplayerfeatureunit.h"
//FIXME i dont feel like adding another global define just for this
#define MINIUPNP_STATICLIB
#include "miniupnp/miniupnpc/upnpcommands.h"
#include "GetTime.h"
#include "miniupnp/miniupnpc/miniupnpc.h"
#include "miniupnp/miniupnpc/upnperrors.h"
#include "app/application.h"
#include "http/httpclient.h"
#include "io/memorystream.h"
#include "jansson/src/jansson.h"
#include "attr/attributetable.h"
#include "multiplayerattrs.h"
#include "basegamefeature/basegameattr/basegameattributes.h"
#include "CloudClient.h"
#include "syncpoint.h"
#include "networkentity.h"
#include "bitreader.h"

namespace MultiplayerFeature
{
__ImplementClass(MultiplayerFeature::NetworkServer, 'MNNS', Core::RefCounted);
__ImplementClass(MultiplayerFeature::NetworkServer::MasterHelperThread, 'msht', Threading::Thread);


__ImplementInterfaceSingleton(MultiplayerFeature::NetworkServer);

using namespace Util;

using namespace Timing;
using namespace Http;
using namespace Debug;
using namespace Math;
using namespace RakNet;


#if NEBULA3_DEBUG
#define CONNECTION_TIMEOUT 3000000 // ms 3000s
#else
#define CONNECTION_TIMEOUT 10000 // ms 10s
#endif

//------------------------------------------------------------------------------
/**
*/
static void
DispatchNetworkMessage(RakNet::BitStream *bitStream, RakNet::Packet *packet)
{
	n_assert(NetworkServer::HasInstance());
	NetworkServer::Instance()->DispatchMessageStream(bitStream, packet);
}

RPC4GlobalRegistration __NebulaMessageDispatch("NebulaMessage", DispatchNetworkMessage, 0);

//------------------------------------------------------------------------------
/**
*/
NetworkServer::NetworkServer() :
    state(IDLE),
	rakPeer(NULL),
	networkIDManager(NULL),	
	natPunchthroughClient(NULL),
	rpc(NULL),
	fullyConnectedMesh(NULL),
	readyEvent(NULL),
	natServer(DEFAULT_SERVER_ADDRESS),
	connectedToNatPunchThrough(false)
{
	__ConstructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
NetworkServer::~NetworkServer()
{
	__DestructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
NetworkServer::Open()
{
	// init raknet
	this->rakPeer = RakPeerInterface::GetInstance();
	this->fullyConnectedMesh = FullyConnectedMesh2::GetInstance();
	this->networkIDManager = NetworkIDManager::GetInstance();
	this->natPunchthroughClient = NatPunchthroughClient::GetInstance();
	this->rpc = RPC4::GetInstance();
	this->readyEvent = Multiplayer::SyncPoint::GetInstance();
	this->replicationManager = ReplicationManager::Create();

	Multiplayer::SyncPoint::SetupSyncPoint(this->readyEvent);
	// attach raknet plugins 
	this->rakPeer->AttachPlugin(this->fullyConnectedMesh);
	this->rakPeer->AttachPlugin(this->natPunchthroughClient);
	this->rakPeer->AttachPlugin(this->rpc);
	this->rakPeer->AttachPlugin(this->readyEvent);
	this->rakPeer->AttachPlugin(this->replicationManager);	
	//// set lastUpdateTime
	//this->lastUpdateTime =	RakNet::GetTime();
}

//------------------------------------------------------------------------------
/**
*/
bool
NetworkServer::SetupLowlevelNetworking()
{
	n_assert2(NetworkGame::HasInstance(), "No NetworkGame or subclass instance exists, cant continue\n");

	this->fullyConnectedMesh->SetAutoparticipateConnections(false);
	this->fullyConnectedMesh->SetConnectOnNewRemoteConnection(false, "");
	this->replicationManager->SetNetworkIDManager(this->networkIDManager);
	this->replicationManager->SetAutoManageConnections(false, true);

	Ptr<NetworkGame> game = NetworkGame::Instance();
	game->SetNetworkIDManager(this->networkIDManager);
	game->SetNetworkID(0);
	this->replicationManager->Reference(game);

	Ptr<NetworkPlayer> user = MultiplayerFeature::MultiplayerFeatureUnit::Instance()->GetPlayer();
	user->SetNetworkIDManager(this->networkIDManager);
	user->SetUniqueId(this->rakPeer->GetMyGUID());
	n_printf("my id: %s\n", this->rakPeer->GetMyGUID().ToString());

	RakNet::SocketDescriptor sd;
	sd.socketFamily = AF_INET; // Only IPV4 supports broadcast on 255.255.255.255
	sd.port = 0;
	StartupResult sr = rakPeer->Startup(8, &sd, 1);
	RakAssert(sr == RAKNET_STARTED);
	this->rakPeer->SetMaximumIncomingConnections(8);
	this->rakPeer->SetTimeoutTime(CONNECTION_TIMEOUT, RakNet::UNASSIGNED_SYSTEM_ADDRESS);
	n_printf("Our guid is %s\n", this->rakPeer->GetGuidFromSystemAddress(RakNet::UNASSIGNED_SYSTEM_ADDRESS).ToString());
	n_printf("Started on %s\n", this->rakPeer->GetMyBoundAddress().ToString(true));

	this->natPunchServerAddress.FromStringExplicitPort(this->natServer.AsCharPtr(), DEFAULT_SERVER_PORT);

	this->state = NETWORK_STARTED;
	ConnectionAttemptResult car = this->rakPeer->Connect(this->natServer.AsCharPtr(), DEFAULT_SERVER_PORT, 0, 0);
	if (car != RakNet::CONNECTION_ATTEMPT_STARTED)
	{
		this->state = IDLE;
		return false;
	}	
	return true;
}

//------------------------------------------------------------------------------
/**
*/
void
NetworkServer::ShutdownLowlevelNetworking()
{
	this->replicationManager->Clear();
	this->fullyConnectedMesh->Clear();
	this->rakPeer->Shutdown(100, 0);	
}

//------------------------------------------------------------------------------
/**
*/
void
NetworkServer::Close()
{
	this->rakPeer->Shutdown(100, 0);
	RakNet::RakPeerInterface::DestroyInstance(this->rakPeer);
	delete this->fullyConnectedMesh;
	delete this->networkIDManager;
	delete this->natPunchthroughClient;
	delete this->rpc;
	delete this->readyEvent;
	this->replicationManager = 0;	
}

//------------------------------------------------------------------------------
/**
*/
void 
NetworkServer::OnFrame()
{        
	if (this->state != IDLE)
	{	
		if (this->deferredMessages.Size() > 0)
		{
			for (int i = this->deferredMessages.Size() -1  ; i >=0 ; i--)
			{
				RakNet::NetworkID id = this->deferredMessages.KeyAtIndex(i);
				RakNet::Replica3* replica = this->LookupReplica(id);				
				MultiplayerFeature::NetworkEntity * entity = dynamic_cast<MultiplayerFeature::NetworkEntity*>(replica);
				if (entity && entity->IsActive())
				{
					Util::Array<Ptr<Messaging::Message>> & msgs = this->deferredMessages.ValueAtIndex(i);
					for (int j = 0; j < msgs.Size(); j++)
					{
						entity->SendSync(msgs[j]);
					}
					this->deferredMessages.EraseAtIndex(i);
				}
			}
		}
		Packet *packet;
		for (packet = this->rakPeer->Receive(); packet; this->rakPeer->DeallocatePacket(packet), packet = this->rakPeer->Receive())
		{
			this->HandlePacket(packet);
		}		
	}
	if (this->doneFlag.TestAndClearIfSet())
	{
		NetworkGame::Instance()->ReceiveMasterList(this->masterResult);
		this->masterResult = 0;
	}
	NetworkGame::Instance()->OnFrame();	
}

//------------------------------------------------------------------------------
/**
*/
bool
NetworkServer::HandlePacket(RakNet::Packet * packet)
{
	Util::String targetName = packet->systemAddress.ToString(true);
	switch (packet->data[0])
	{
	case ID_IP_RECENTLY_CONNECTED:
	{
		n_printf("This IP address recently connected from %s\n", targetName.AsCharPtr());
		if (packet->systemAddress == this->natPunchServerAddress)
		{
			n_printf("Multiplayer will not work without the NAT punchthrough server!");
		}			
	}
	break;
	case ID_INCOMPATIBLE_PROTOCOL_VERSION:
	{
		n_printf("Incompatible protocol version from %s\n", targetName.AsCharPtr());
		if (packet->systemAddress == this->natPunchServerAddress)
		{
			n_printf("Multiplayer will not work without the NAT punchthrough server!");
		}
	}
	break;
	case ID_DISCONNECTION_NOTIFICATION:
	{
		n_printf("Disconnected from %s\n", targetName.AsCharPtr());
		NetworkGame::Instance()->OnPlayerDisconnect(packet->guid);
		if (packet->systemAddress == this->natPunchServerAddress)
		{
			this->connectedToNatPunchThrough = false;
		}			
	}
	break;
	case ID_CONNECTION_LOST:
	{
		n_printf("Connection to %s lost\n", targetName.AsCharPtr());
		if (packet->systemAddress == this->natPunchServerAddress)
		{
			this->connectedToNatPunchThrough = false;
		}			
	}
	break;
	case ID_NO_FREE_INCOMING_CONNECTIONS:
	{
		n_printf("No free incoming connections to %s\n", targetName.AsCharPtr());
		if (packet->systemAddress == this->natPunchServerAddress)
		{
			n_printf("Multiplayer will not work without the NAT punchthrough server!");
		}			
	}
	break;
	case ID_NEW_INCOMING_CONNECTION:
	{
		if (this->fullyConnectedMesh->IsHostSystem())
		{
			n_printf("Sending player list to new connection: %s\n",packet->guid.ToString());
			this->fullyConnectedMesh->StartVerifiedJoin(packet->guid);
		}
	}
	break;
	case ID_FCM2_VERIFIED_JOIN_START:
	{
		DataStructures::List<RakNet::SystemAddress> addresses;
		DataStructures::List<RakNet::RakNetGUID> guids;
		DataStructures::List<RakNet::BitStream*> streams;
		this->fullyConnectedMesh->GetVerifiedJoinRequiredProcessingList(packet->guid, addresses, guids, streams);
		for (unsigned int i = 0; i < guids.Size(); i++)
		{
			natPunchthroughClient->OpenNAT(guids[i], this->natPunchServerAddress);
		}			
	}
	break;
	case ID_FCM2_VERIFIED_JOIN_FAILED:
	{
		NetworkGame::Instance()->OnJoinFailed("Connection failed");
	}
	break;
	case ID_FCM2_VERIFIED_JOIN_REJECTED:	
	{
		RakNet::BitStream bs(packet->data, packet->length, false);
		Ptr<Multiplayer::BitReader> br = Multiplayer::BitReader::Create();
		br->SetStream(&bs);
		br->ReadChar();
		Util::String answer = br->ReadString();

		n_printf("Failed to join game session: %s", answer.AsCharPtr());
		NetworkGame::Instance()->OnJoinFailed(answer);
	}
	break;
	case ID_FCM2_VERIFIED_JOIN_CAPABLE:
	{
		//If server not full and you're in lobby or allowed to join while game has started
		if (this->fullyConnectedMesh->GetParticipantCount() + 1 < NetworkGame::Instance()->GetMaxPlayers() && IsInGameJoinUnLocked())
		{
			this->fullyConnectedMesh->RespondOnVerifiedJoinCapable(packet, true, 0);
		}
		else
		{
			RakNet::BitStream answer;			
			answer.Write("Server Full\n");
			this->fullyConnectedMesh->RespondOnVerifiedJoinCapable(packet, false, &answer);			
		}		
	}
	break;
	case ID_FCM2_VERIFIED_JOIN_ACCEPTED:
	{
		DataStructures::List<RakNet::RakNetGUID> systemsAccepted;
		bool thisSystemAccepted;
		this->fullyConnectedMesh->GetVerifiedJoinAcceptedAdditionalData(packet, &thisSystemAccepted, systemsAccepted, 0);
		if (thisSystemAccepted)
		{
			n_printf("Game join request accepted\n");			
		}			
		else
		{			
			n_printf("System %s joined the mesh\n", systemsAccepted[0].ToString());
		}		

		for (unsigned int i = 0; i < systemsAccepted.Size(); i++)
		{
			this->replicationManager->PushConnection(this->replicationManager->AllocConnection(rakPeer->GetSystemAddressFromGuid(systemsAccepted[i]), systemsAccepted[i]));
		}			
	}
	break;
	case ID_CONNECTION_REQUEST_ACCEPTED:
	{
		n_printf("Connection request to %s accepted\n", targetName.AsCharPtr());
		n_printf("connection guid: %s\n", packet->guid.ToString());		
		if (packet->systemAddress == this->natPunchServerAddress)
		{
			this->connectedToNatPunchThrough = true;

			// Open UPNP.
			struct UPNPDev * devlist = 0;
			devlist = upnpDiscover(1000, 0, 0, 0,0,0);
			if (devlist)
			{
				char lanaddr[64];	/* my ip address on the LAN */
				struct UPNPUrls urls;
				struct IGDdatas data;
				if (UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr)) == 1)
				{
					// External port is the port people will be connecting to us on. This is our port as seen by the directory server
					// Internal port is the port RakNet was internally started on
					char eport[32], iport[32];
					natPunchthroughClient->GetUPNPPortMappings(eport, iport, this->natPunchServerAddress);

					int r = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
						eport, iport, lanaddr, 0, "UDP", 0,"0");

					if (r == UPNPCOMMAND_SUCCESS)
					{
						// UPNP done
					}

				}
			}
		}
	}
	break;
	case ID_FCM2_NEW_HOST:
	{
		if (packet->guid == rakPeer->GetMyGUID())
		{
			n_printf("we are the host\n");
			this->state = IN_LOBBY;
			// Original host dropped. I am the new session host. Upload to the cloud so new players join this system.
			//RakNet::CloudKey cloudKey(NetworkGame::Instance()->GetGameID().AsCharPtr(), 0);
			//cloudClient->Post(&cloudKey, 0, 0, rakPeer->GetGuidFromSystemAddress(this->natPunchServerAddress));			
		}
		else
		{
			this->state = IN_LOBBY;
			n_printf("the new host is %s\n", packet->guid.ToString());		
		}
	}
	break;	
	case ID_CONNECTION_ATTEMPT_FAILED:
	{
		n_printf("Connection attempt to %s failed\n", targetName.AsCharPtr());
		if (packet->systemAddress == this->natPunchServerAddress)
		{
			n_printf("Multiplayer will not work without the NAT punchthrough server!");
		}			
	}
	break;
	case ID_NAT_TARGET_NOT_CONNECTED:
	{
		RakNet::RakNetGUID recipientGuid;
		RakNet::BitStream bs(packet->data, packet->length, false);
		bs.IgnoreBytes(sizeof(RakNet::MessageID));
		bs.Read(recipientGuid);
		targetName = recipientGuid.ToString();
		n_printf("NAT target %s not connected\n", targetName.AsCharPtr());		
	}
	break;
	case ID_NAT_TARGET_UNRESPONSIVE:
	{
		RakNet::RakNetGUID recipientGuid;
		RakNet::BitStream bs(packet->data, packet->length, false);
		bs.IgnoreBytes(sizeof(RakNet::MessageID));
		bs.Read(recipientGuid);
		targetName = recipientGuid.ToString();
		n_printf("NAT target %s unresponsive\n", targetName.AsCharPtr());
	}
	break;
	case ID_NAT_CONNECTION_TO_TARGET_LOST:
	{
		RakNet::RakNetGUID recipientGuid;
		RakNet::BitStream bs(packet->data, packet->length, false);
		bs.IgnoreBytes(sizeof(RakNet::MessageID));
		bs.Read(recipientGuid);
		targetName = recipientGuid.ToString();
		n_printf("NAT target connection to %s lost\n", targetName.AsCharPtr());		
	}
	break;
	case ID_NAT_ALREADY_IN_PROGRESS:
	{
		RakNet::RakNetGUID recipientGuid;
		RakNet::BitStream bs(packet->data, packet->length, false);
		bs.IgnoreBytes(sizeof(RakNet::MessageID));
		bs.Read(recipientGuid);
		targetName = recipientGuid.ToString();
		n_printf("NAT target connection to %s already in progress, skipping\n", targetName.AsCharPtr());
	}
	break;

	case ID_NAT_PUNCHTHROUGH_SUCCEEDED:
	{
		if (packet->data[1] == 1)
		{
			n_printf("Connecting to existing game instance");
			RakNet::ConnectionAttemptResult car = rakPeer->Connect(packet->systemAddress.ToString(false), packet->systemAddress.GetPort(), 0, 0);
			if (car != ALREADY_CONNECTED_TO_ENDPOINT || car != RakNet::CONNECTION_ATTEMPT_STARTED)
			{
				n_warning("Nat punchthrough failed\n");
			}
		}
	}
	break;

	case ID_ADVERTISE_SYSTEM:
		if (packet->guid != rakPeer->GetGuidFromSystemAddress(RakNet::UNASSIGNED_SYSTEM_ADDRESS))
		{
			char hostIP[32];
			packet->systemAddress.ToString(false, hostIP);
			RakNet::ConnectionAttemptResult car = rakPeer->Connect(hostIP, packet->systemAddress.GetPort(), 0, 0);
			RakAssert(car == RakNet::CONNECTION_ATTEMPT_STARTED);
		}
		break;
	case ID_READY_EVENT_ALL_SET:			
		break;

	case ID_READY_EVENT_SET:		
		break;

	case ID_READY_EVENT_UNSET:		
		break;
	}
	return true;
}
	
//------------------------------------------------------------------------------
/**
*/
void
NetworkServer::SearchForGames()
{	
	this->UpdateRoomList();
}

//------------------------------------------------------------------------------
/**
*/
void
NetworkServer::MasterServerResult(Util::String response)
{	
	n_printf("got master server results:\n%s", response.AsCharPtr());
	json_error_t error;
	json_t * root = json_loads(response.AsCharPtr(), JSON_REJECT_DUPLICATES, &error);
	if (NULL == root)
	{
		n_warning("error parsing json from master server\n");
		// FIXME which state now
		return;
	}

	Ptr<Attr::AttributeTable> table = Attr::AttributeTable::Create();
	table->BeginAddColumns(false);
	table->AddColumn(Attr::GameRow, false);
	table->AddColumn(Attr::RoomName, false);
	table->AddColumn(Attr::GameID, false);
	table->AddColumn(Attr::GameAge, false);
	table->AddColumn(Attr::GameServerAddress, false);
	table->AddColumn(Attr::MaxPlayers, false);
	table->AddColumn(Attr::CurrentPlayers, false);
	table->AddColumn(Attr::Id, false);
	table->EndAddColumns();

	void *iter = json_object_iter(root);
	while (iter)
	{
		String firstKey = json_object_iter_key(iter);


		if (firstKey == "GET")
		{
			json_t* jsonArray = json_object_iter_value(iter);
			size_t arraySize = json_array_size(jsonArray);
			for (unsigned int i = 0; i < arraySize; i++)
			{
				IndexT row = table->AddRow();
				json_t* object = json_array_get(jsonArray, i);

				json_t* val = json_object_get(object, "roomName");
				n_assert(val->type == JSON_STRING);
				Util::String room = json_string_value(val);		
				room.Append("\r");
				Util::String decRoom = Util::String::FromBase64(room);
				table->SetString(Attr::RoomName, row, decRoom);

				val = json_object_get(object, "__gameId");
				n_assert(val->type == JSON_STRING);
				Util::String gameId = json_string_value(val);
				table->SetString(Attr::GameID, row, gameId);

				val = json_object_get(object, "__timeoutSec");
				n_assert(val->type == JSON_STRING);
				Util::String strval = json_string_value(val);
				int timeout = strval.AsInt();
				table->SetInt(Attr::GameAge, row, timeout);

				val = json_object_get(object, "__addr");
				n_assert(val->type == JSON_STRING);
				Util::String ipAddr = json_string_value(val);
				table->SetString(Attr::GameServerAddress, row, ipAddr);

				val = json_object_get(object, "guid");
				n_assert(val->type == JSON_STRING);
				Util::String guid = json_string_value(val);
				table->SetString(Attr::Id, row, guid);

				val = json_object_get(object, "__rowId");
				n_assert(val->type == JSON_INTEGER);
				int gameRow = (int)json_integer_value(val);
				table->SetInt(Attr::GameRow, row, gameRow);

				val = json_object_get(object, "currentPlayers");
				n_assert(val->type == JSON_INTEGER);
				int current = (int)json_integer_value(val);
				table->SetInt(Attr::CurrentPlayers, row, current);

				val = json_object_get(object, "maxPlayers");
				n_assert(val->type == JSON_INTEGER);
				int maxplayers = (int)json_integer_value(val);
				table->SetInt(Attr::MaxPlayers, row, maxplayers);				
				n_printf("Room name: %s, Server: %s, players: %d/%d\n", room.AsCharPtr(), ipAddr.AsCharPtr(), current, maxplayers);
			}
			iter = json_object_iter_next(root, iter);
		}
		else
		{
			iter = json_object_iter_next(root, iter);
		}
	}
	json_decref(root);
	NetworkServer::Instance()->masterResult = table;
	NetworkServer::Instance()->doneFlag.Set();	
}

//------------------------------------------------------------------------------
/**
*/
void
NetworkServer::UpdateRoomList()
{
	this->masterThread = MasterHelperThread::Create();
	this->masterThread->gameId = NetworkGame::Instance()->GetGameID();	
	this->masterThread->Start();
}

//------------------------------------------------------------------------------
/**
*/
void
NetworkServer::NatConnect(const RakNet::RakNetGUID &guid)
{
	this->natPunchthroughClient->OpenNAT(guid, this->natPunchServerAddress);
}

//------------------------------------------------------------------------------
/**
*/
void
NetworkServer::CreateRoom()
{
	this->state = IN_LOBBY_WAITING_FOR_HOST_DETERMINATION;
}


//------------------------------------------------------------------------------
/**
*/
void
NetworkServer::CancelRoom()
{
	this->state = NETWORK_STARTED;
}


//------------------------------------------------------------------------------
/**
*/
void
NetworkServer::StartGame()
{	
	this->state = IN_GAME;	
}

//------------------------------------------------------------------------------
/**
*/
RakNet::Replica3 *
NetworkServer::LookupReplica(RakNet::NetworkID replicaId)
{
	RakNet::Replica3 * replica = this->networkIDManager->GET_OBJECT_FROM_ID<RakNet::Replica3*>(replicaId);	
	return replica;
}


//------------------------------------------------------------------------------
/**
Returns a user defined flag when IN_GAME so that the user can decide if it's allowed to join or not.
*/
bool NetworkServer::IsInGameJoinUnLocked()
{
	if (this->state == IN_GAME)
	{
		return NetworkGame::Instance()->CanJoinInGame();
	}
	else
	{
		return true;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
NetworkServer::DispatchMessageStream(RakNet::BitStream * msgStream, Packet *packet)
{
	RakNet::NetworkID entityId;
	msgStream->Read(entityId);
	RakNet::Replica3 * replica = this->LookupReplica(entityId);

	MultiplayerFeature::NetworkEntity * entity = dynamic_cast<MultiplayerFeature::NetworkEntity*>(replica);

	Ptr<Multiplayer::BitReader> reader = Multiplayer::BitReader::Create();
	reader->SetStream(msgStream);

	// deserialize messages
	int count = reader->ReadUChar();
	Ptr<IO::BinaryReader> breader = IO::BinaryReader::Create();
	breader->SetStream(reader.cast<IO::Stream>());
	breader->Open();
	for (int i = 0; i < count; i++)
	{
		Util::FourCC fcc = reader->ReadUInt();
		Ptr<Core::RefCounted> cmsg = Core::Factory::Instance()->Create(fcc);
		Ptr<Messaging::Message> msg = cmsg.cast<Messaging::Message>();
		msg->SetDistribute(false);
		msg->Decode(breader);
		if (entity)
		{
			entity->SendSync(msg);
		}
		else
		{
			this->AddDeferredMessage(entityId, msg);
		}
		
	}
	breader->Close();

}

//------------------------------------------------------------------------------
/**
*/
void
NetworkServer::SendMessageStream(RakNet::BitStream* msgStream)
{
	DataStructures::List<RakNetGUID> participantList;
	this->fullyConnectedMesh->GetParticipantList(participantList);

	if (participantList.Size() > 0)
	{		
		for (unsigned int i = 0; i < participantList.Size(); i++)
			this->rpc->Signal("NebulaMessage", msgStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, participantList[i], false, false);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
NetworkServer::AddDeferredMessage(RakNet::NetworkID entityId, const Ptr<Messaging::Message> &msg)
{
	if (!this->deferredMessages.Contains(entityId))
	{
		this->deferredMessages.Add(entityId, Util::Array<Ptr<Messaging::Message>>());
	}
	this->deferredMessages[entityId].Append(msg);
}

//------------------------------------------------------------------------------
/**
*/
void
NetworkServer::MasterHelperThread::DoWork()
{
	Ptr<Http::HttpClient> client = Http::HttpClient::Create();
	Util::String serverString = "http://" MASTER_SERVER_ADDRESS;
	Util::String requestString;
	requestString.Format("testServer?__gameId=%s", this->gameId.AsCharPtr());
	IO::URI serverUri(serverString);
	IO::URI requestUri(serverString);
	requestUri.SetLocalPath(requestString);
	
	n_printf("%s\n", requestUri.LocalPath().AsCharPtr());
	Ptr<IO::MemoryStream> stream = IO::MemoryStream::Create();
	stream->SetAccessMode(IO::Stream::ReadWriteAccess);
	stream->Open();
	client->Connect(serverUri);
	Http::HttpStatus::Code res = client->SendRequest(Http::HttpMethod::Get, requestUri, stream.cast<IO::Stream>());	
	
	if (res != HttpStatus::OK)
	{
		n_warning("Failed to do http request to masterserver\n");
		//FIXME what status should we go to
		return;
	}
	Util::String response;
 	response.Set((const char*)stream->GetRawPointer(), stream->GetSize());
	NetworkServer::Instance()->MasterServerResult(response);
}
} // namespace RakNet
