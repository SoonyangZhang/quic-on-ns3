#include "gquiche/quic/platform/api/quic_logging.h"
#include "ns3-quic-client-session.h"
#include "ns3-transport-stream.h"
#include "ns3-quic-backend.h"
namespace quic{
Ns3QuicClientSession::Ns3QuicClientSession(std::unique_ptr<QuicConnection> connection_ptr,
Visitor* owner,
const QuicConfig& config,
const ParsedQuicVersionVector& supported_versions,
const QuicServerId& server_id,
QuicCryptoClientConfig* crypto_config,
Ns3QuicBackendBase *backend)
:Ns3QuicSessionBase(connection_ptr.get(),owner,config,supported_versions,0),
backend_(backend),
connection_own_(std::move(connection_ptr)){
    /*for (const ParsedQuicVersion& version : supported_versions) {
        QUIC_BUG_IF(Ns3QuicClientSession,version.handshake_protocol != PROTOCOL_TLS1_3)
            << "QuicTransport requires TLS 1.3 handshake";
    }*/
    //std::cout<<"max uni: "<<config.GetMaxUnidirectionalStreamsToSend()<<std::endl;
    //std::cout<<"max bi: "<<config.GetMaxBidirectionalStreamsToSend()<<std::endl;
    crypto_stream_ = std::make_unique<QuicCryptoClientStream>(
    server_id, this,crypto_config->proof_verifier()->CreateDefaultContext(),
    crypto_config,/*proof_handler=*/this, /*has_application_state = */ true);
    endpoint_.reset(backend_->CreateEndpoint(this));
    if(!endpoint_){
        QUIC_LOG(ERROR)<<"endpoint is null "<<connection_own_.get()<<" "<<connection();
        connection()->CloseConnection(QUIC_INTERNAL_ERROR, "Create Endpoint Failed",
                    ConnectionCloseBehavior::SEND_CONNECTION_CLOSE_PACKET);  
    }
}
Ns3QuicClientSession::~Ns3QuicClientSession(){
    if(endpoint_){
        endpoint_->OnSessionDestroy(this);
    }
}
Ns3TransportStream* Ns3QuicClientSession::OpenOutgoingBidirectionalStream(){
    if (!CanOpenNextOutgoingBidirectionalStream()) {
        QUIC_BUG(Ns3QuicClientSession)<< "Attempted to open a stream in violation of flow control";
        return nullptr;
    }
    return CreateStream(GetNextOutgoingBidirectionalStreamId());
}
Ns3TransportStream* Ns3QuicClientSession::OpenOutgoingUnidirectionalStream(){
    if (!CanOpenNextOutgoingUnidirectionalStream()) {
        QUIC_BUG(Ns3QuicClientSession)<< "Attempted to open a stream in violation of flow control";
        return nullptr;
    }
    return CreateStream(GetNextOutgoingUnidirectionalStreamId());
}
void Ns3QuicClientSession::Initialize(){
    QuicSession::Initialize();
    CryptoConnect();
}
void Ns3QuicClientSession::OnAlpnSelected(absl::string_view alpn) {
  // Defense in-depth: ensure the ALPN selected is the desired one.
  if (alpn != QuicTransportAlpn()) {
    QUIC_BUG(Ns3QuicClientSession)<< "QuicTransport negotiated non-QuicTransport ALPN: " << alpn;
    connection()->CloseConnection(
        QUIC_INTERNAL_ERROR, "QuicTransport negotiated non-QuicTransport ALPN",
        ConnectionCloseBehavior::SEND_CONNECTION_CLOSE_PACKET);
    return;
  }
  alpn_received_ = true;
}
QuicStream* Ns3QuicClientSession::CreateIncomingStream(QuicStreamId id){
  QUIC_DVLOG(1) << "Creating incoming QuicTransport stream " << id;
  Ns3TransportStream* stream = CreateStream(id);
  if(BIDIRECTIONAL==stream->type()){
        endpoint_->OnIncomingBidirectionalStream(stream);
  }else{
        endpoint_->OnIncomingUnidirectionalStream(stream);
  }
  return stream;
}
QuicStream* Ns3QuicClientSession::CreateIncomingStream(PendingStream* /*pending*/){
    return nullptr;
}
void Ns3QuicClientSession::SetDefaultEncryptionLevel(EncryptionLevel level){
    if(ENCRYPTION_FORWARD_SECURE==level){
        ready_=true;
        endpoint_->OnSessionReady(this);
    }
}
void Ns3QuicClientSession::OnTlsHandshakeComplete(){
    QuicSession::OnTlsHandshakeComplete();
    ready_=true;
    endpoint_->OnSessionReady(this);
}
void Ns3QuicClientSession::OnMessageReceived(absl::string_view message){
    //done nothing
}
void Ns3QuicClientSession::OnProofValid(const QuicCryptoClientConfig::CachedState& cached){
    
}
void Ns3QuicClientSession::OnProofVerifyDetailsAvailable(const ProofVerifyDetails& verify_details){
    
}

int Ns3QuicClientSession::GetNumSentClientHellos() const{
     return crypto_stream_->num_sent_client_hellos();
}
bool Ns3QuicClientSession::EarlyDataAccepted() const{
     return crypto_stream_->EarlyDataAccepted();
}
bool Ns3QuicClientSession::ReceivedInchoateReject() const{
     return crypto_stream_->ReceivedInchoateReject();
}
int Ns3QuicClientSession::GetNumReceivedServerConfigUpdates() const{
     return crypto_stream_->num_scup_messages_received();
}
bool Ns3QuicClientSession::HasActiveRequestStreams() const{
    return GetNumActiveStreams() + num_draining_streams() > 0;
}
Ns3TransportStream* Ns3QuicClientSession::CreateStream(QuicStreamId id){
    auto stream = std::make_unique<Ns3TransportStream>(id, this);
    Ns3TransportStream* stream_ptr = stream.get();
    ActivateStream(std::move(stream));
    return stream_ptr;
}
void Ns3QuicClientSession::CloseSession(){
    if(connection()&&connection()->connected()){
        connection()->CloseConnection(
        QUIC_PEER_GOING_AWAY,"Session disconnecting",
        ConnectionCloseBehavior::SEND_CONNECTION_CLOSE_PACKET);        
    }
}
void Ns3QuicClientSession::OnCanCreateNewOutgoingStream(bool unidirectional){
    
}
}
