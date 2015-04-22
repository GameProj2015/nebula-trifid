#pragma once
//------------------------------------------------------------------------------
/**
    @class MultiplayerFeature::NetworkServer       
                
    (C) 2015 Individual contributors, see AUTHORS file
*/
#include "util/dictionary.h"
#include "timing/time.h"
#include "replicationmanager.h"
#include "threading/thread.h"
#include "FullyConnectedMesh2.h"
#include "threading/safeflag.h"
#include "attr/attributetable.h"
#include "syncpoint.h"

//FIXME these should be replaced by own implementations, leave them for the time being
#define DEFAULT_SERVER_PORT 61111
#define DEFAULT_SERVER_ADDRESS "natpunch.jenkinssoftware.com"
#define MASTER_SERVER_ADDRESS "masterserver2.raknet.com"
#define MASTER_SERVER_PORT "80"

#define IS_HOST (MultiplayerFeature::NetworkServer::Instance()->IsHost())

namespace RakNet
{
	class RakPeerInterface;
	class NetworkIDManager;
	class TCPInterface;
	class NatPunchthroughClient;
	class RPC4;	
	class HTTPConnection2;
	class ReadyEvent;
	class CloudClient;
}
//------------------------------------------------------------------------------
namespace MultiplayerFeature
{
class NetworkServer : public Core::RefCounted
{
	__DeclareClass(NetworkServer);
	__DeclareInterfaceSingleton(NetworkServer);
public:

	enum NetworkServerState
	{
		IDLE,
		NETWORK_STARTED,
		IN_LOBBY_WAITING_FOR_HOST_DETERMINATION,
		IN_LOBBY,
		IN_GAME,
	};
    /// constructor
	NetworkServer();
    /// destructor
	virtual ~NetworkServer();

    /// open the RakNetServer 
    void Open();
    /// close the RakNetServer
    void Close();
    /// perform client-side per-frame updates
    void OnFrame();

	/// setup low level network handling, NAT punch, UPNP
	bool SetupLowlevelNetworking();	
	    
    /// get rakpeer interface
	RakNet::RakPeerInterface* GetRakPeerInterface() const;

	/// are we the host of the session
	bool IsHost() const;

	/// is the host determined yet
	bool HasHost();

private:
		
	class MasterHelperThread : public Threading::Thread
	{
		__DeclareClass(MasterHelperThread)			
	public:
		Util::String gameId;
	protected:
		/// implements the actual listener method
		virtual void DoWork();
	};
	
	friend class MasterHelperThread;

	/// callback function for masterserver worker
	void MasterServerResult(Util::String response);

	/// connect to server using nat punchthrough
	void NatConnect(const RakNet::RakNetGUID &guid);
	/// start searching for games
	void SearchForGames();

	/// start the game
	void StartGame();
	/// create room
	void CreateRoom();
	/// trigger refresh of available rooms on master
	void UpdateRoomList();
	/// deal with a packet
	bool HandlePacket(RakNet::Packet * packet);


	/// hmm, lets have this for the time being
	friend class NetworkGame;

	NetworkServerState state;
	RakNet::RakPeerInterface *rakPeer;	
	Ptr<MultiplayerFeature::ReplicationManager> replicationManager;
	RakNet::NetworkIDManager *networkIDManager;
	RakNet::NatPunchthroughClient *natPunchthroughClient;
	RakNet::RPC4 *rpc;
	RakNet::FullyConnectedMesh2 *fullyConnectedMesh;
	Multiplayer::SyncPoint *readyEvent;	
	RakNet::RakNetGUID natPunchServerGuid;
	RakNet::SystemAddress natPunchServerAddress;
	RakNet::CloudClient * cloudClient;
	Ptr<MasterHelperThread> masterThread;
	Util::String natServer;
	bool connectedToNatPunchThrough;		
	Ptr<Attr::AttributeTable> masterResult;
	Threading::SafeFlag doneFlag;
	RakNet::Time lastUpdateTime;
};

//------------------------------------------------------------------------------
/**
*/
inline
RakNet::RakPeerInterface*
NetworkServer::GetRakPeerInterface() const
{
	return this->rakPeer;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
NetworkServer::IsHost() const
{	
	return this->fullyConnectedMesh->IsHostSystem();
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
NetworkServer::HasHost()
{
	return this->state > IN_LOBBY_WAITING_FOR_HOST_DETERMINATION;
}


} // namespace MultiplayerFeature
//------------------------------------------------------------------------------
