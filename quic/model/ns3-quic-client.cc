#include <string>
#include "gquiche/quic/core/quic_config.h"
#include "url/gurl.h"

#include "ns3-quic-client.h"
#include "ns3-quic-backend.h"
#include "ns3-quic-alarm-engine.h"
#include "ns3-client-network-helper.h"
#include "ns3-quic-connection-helper.h"
#include "ns3-quic-client-session.h"
namespace quic{
Ns3QuicClient::Ns3QuicClient(QuicSession::Visitor *owner,
QuicSocketAddress server_address,
const QuicServerId& server_id,
const ParsedQuicVersionVector& supported_versions,
std::unique_ptr<ProofVerifier> proof_verifier,
Ns3QuicAlarmEngine *engine,
Ns3QuicBackendBase *backend,
Ns3QuicPollServer *poller):
Ns3QuicClient(owner,server_address,server_id,supported_versions,QuicConfig(),
std::move(proof_verifier),nullptr,engine,backend,poller){}

Ns3QuicClient::Ns3QuicClient(QuicSession::Visitor *owner,
QuicSocketAddress server_address,
const QuicServerId& server_id,
const ParsedQuicVersionVector& supported_versions,
const QuicConfig& config,
std::unique_ptr<ProofVerifier> proof_verifier,
std::unique_ptr<SessionCache> session_cache,
Ns3QuicAlarmEngine *engine,
Ns3QuicBackendBase *backend,
Ns3QuicPollServer *poller):
QuicClientBase(server_id,supported_versions,config,
new Ns3QuicConnectionHelper(QuicAllocatorType::SIMPLE),
new Ns3AlarmFactory(engine),
std::make_unique<Ns3ClientNetworkHelper>(poller,this),
std::move(proof_verifier),
std::move(session_cache)),
backend_(backend),
owner_(owner){
    set_server_address(server_address);
}

Ns3QuicClient::~Ns3QuicClient()=default;
void Ns3QuicClient::AsynConnect(){
    if(!connected()){
        StartConnect();
    }
}
Ns3QuicClientSession* Ns3QuicClient::client_session(){
    return static_cast<Ns3QuicClientSession*>(QuicClientBase::session());
}

std::unique_ptr<QuicSession> Ns3QuicClient::CreateQuicClientSession(
    const quic::ParsedQuicVersionVector& supported_versions,
    QuicConnection* connection){
    std::unique_ptr<QuicConnection> connection_ptr(connection);
    std::unique_ptr<Ns3QuicClientSession> session(new Ns3QuicClientSession(
            std::move(connection_ptr),owner_,*config(),supported_versions,
            server_id(),crypto_config(),backend_));
    return session;
}
int Ns3QuicClient::GetNumSentClientHellosFromSession(){
    return client_session()->GetNumSentClientHellos();
}
int Ns3QuicClient::GetNumReceivedServerConfigUpdatesFromSession(){
    return client_session()->GetNumReceivedServerConfigUpdates();
}
bool Ns3QuicClient::EarlyDataAccepted() {
  return client_session()->EarlyDataAccepted();
}

bool Ns3QuicClient::ReceivedInchoateReject() {
  return client_session()->ReceivedInchoateReject();
}
bool Ns3QuicClient::HasActiveRequests() {
  return client_session()->HasActiveRequestStreams();
}

}
