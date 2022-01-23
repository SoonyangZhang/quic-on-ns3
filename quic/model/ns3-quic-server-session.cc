#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "gquiche/quic/core/quic_error_codes.h"
#include "gquiche/quic/core/quic_stream.h"
#include "gquiche/quic/core/quic_types.h"
#include "gquiche/quic/quic_transport/quic_transport_protocol.h"

#include "ns3-quic-session-base.h"
#include "ns3-transport-stream.h"
#include "ns3-quic-backend.h"
#include "ns3-quic-server-session.h"
namespace quic{
namespace {
class Ns3QuicTransportServerCryptoHelper
    : public QuicCryptoServerStreamBase::Helper {
 public:
  bool CanAcceptClientHello(const CryptoHandshakeMessage& /*message*/,
                            const QuicSocketAddress& /*client_address*/,
                            const QuicSocketAddress& /*peer_address*/,
                            const QuicSocketAddress& /*self_address*/,
                            std::string* /*error_details*/) const override {
    return true;
  }
};

}  // namespace
Ns3QuicServerSession::Ns3QuicServerSession(
std::unique_ptr<QuicConnection> connection_ptr,
Visitor* owner,
const QuicConfig& config,
const ParsedQuicVersionVector& supported_versions,
const QuicCryptoServerConfig* crypto_config,
QuicCompressedCertsCache* compressed_certs_cache,
Ns3QuicBackendBase *backend):
Ns3QuicSessionBase(connection_ptr.get(),owner,config,supported_versions,0),
backend_(backend),
connection_own_(std::move(connection_ptr)){
    /*for (const ParsedQuicVersion& version : supported_versions){
        QUIC_BUG_IF(Ns3QuicServerSession,version.handshake_protocol != PROTOCOL_TLS1_3)
        << "QuicTransport requires TLS 1.3 handshake";
    }*/
    static Ns3QuicTransportServerCryptoHelper* helper =new Ns3QuicTransportServerCryptoHelper();
    crypto_stream_ = CreateCryptoServerStream(
                    crypto_config, compressed_certs_cache,this, helper);
}
Ns3QuicServerSession::~Ns3QuicServerSession(){
    if(endpoint_){
        endpoint_->OnSessionDestroy(this);
    }
}
Ns3TransportStream* Ns3QuicServerSession::OpenOutgoingBidirectionalStream(){
    if (!CanOpenNextOutgoingBidirectionalStream()) {
        QUIC_BUG(Ns3QuicServerSession)<< "Attempted to open a stream in violation of flow control";
        return nullptr;
    }
    return CreateStream(GetNextOutgoingBidirectionalStreamId());
}
Ns3TransportStream* Ns3QuicServerSession::OpenOutgoingUnidirectionalStream(){
    if (!CanOpenNextOutgoingUnidirectionalStream()) {
        QUIC_BUG(Ns3QuicServerSession)<< "Attempted to open a stream in violation of flow control";
        return nullptr;
    }
    return CreateStream(GetNextOutgoingUnidirectionalStreamId());
}
QuicStream* Ns3QuicServerSession::CreateIncomingStream(QuicStreamId id){
    Ns3TransportStream *stream=nullptr;
    if(!ready_&&!endpoint_){
        ready_=true;
        endpoint_.reset(backend_->CreateEndpoint(this));
        if(!endpoint_){
            CloseSession();
            return stream;
        }
        endpoint_->OnSessionReady(this);
    }
    stream=CreateStream(id);
    if(BIDIRECTIONAL==stream->type()){
        endpoint_->OnIncomingBidirectionalStream(stream);
    }else{
        endpoint_->OnIncomingUnidirectionalStream(stream);
    }
    return stream;
}
QuicStream* Ns3QuicServerSession::CreateIncomingStream(PendingStream* /*pending*/){
    return nullptr;
}
void Ns3QuicServerSession::CloseSession(){
    if(connection()&&connection()->connected()){
        connection()->CloseConnection(
        QUIC_PEER_GOING_AWAY,"Session disconnecting",
        ConnectionCloseBehavior::SEND_CONNECTION_CLOSE_PACKET);        
    }
}
Ns3TransportStream* Ns3QuicServerSession::CreateStream(QuicStreamId id){
    auto stream = std::make_unique<Ns3TransportStream>(id, this);
    Ns3TransportStream* stream_ptr = stream.get();
    ActivateStream(std::move(stream));
    return stream_ptr;
}
}
