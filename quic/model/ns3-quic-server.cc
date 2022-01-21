#include "gquiche/quic/core/quic_crypto_server_stream_base.h"
#include "gquiche/quic/core/quic_connection_id.h"
#include "gquiche/quic/core/crypto/quic_random.h"
#include "gquiche/quic/core/quic_constants.h"
#include "gquiche/common/platform/api/quiche_logging.h"

#include "ns3-quic-backend.h"
#include "ns3-quic-server.h"
#include "ns3-quic-dispatcher.h"
#include "ns3-quic-alarm-engine.h"
#include "ns3-quic-connection-helper.h"
#include "ns3-quic-clock.h"
namespace quic{
namespace {
    const char kSourceAddressTokenSecret[] = "secret";
    const size_t kNumSessionsToCreatePerSocketEvent = 16;
// If an initial flow control window has not explicitly been set, then use a
// sensible value for a server: 1 MB for session, 64 KB for each stream.
    const uint32_t kInitialSessionFlowControlWindow = 1 * 1024 * 1024;  // 1 MB
    const uint32_t kInitialStreamFlowControlWindow = 64 * 1024;         // 64 KB
}
class Ns3QuicCryptoServerStreamHelper
    : public QuicCryptoServerStreamBase::Helper {
 public:
  Ns3QuicCryptoServerStreamHelper(){}

  ~Ns3QuicCryptoServerStreamHelper() override{}

  bool CanAcceptClientHello(const CryptoHandshakeMessage& message,
                            const QuicSocketAddress& client_address,
                            const QuicSocketAddress& peer_address,
                            const QuicSocketAddress& self_address,
                            std::string* error_details) const override{
    return true;
}
};

Ns3QuicServer::Ns3QuicServer(std::unique_ptr<ProofSource> proof_source,
Ns3QuicAlarmEngine *engine,
Ns3QuicBackendBase *backend):
Ns3QuicServer(std::move(proof_source),
            AllSupportedVersions(),
            engine,
            backend){}

Ns3QuicServer::Ns3QuicServer(std::unique_ptr<ProofSource> proof_source,
const ParsedQuicVersionVector& supported_versions,
Ns3QuicAlarmEngine *engine,
Ns3QuicBackendBase *backend):
Ns3QuicServer(std::move(proof_source),
              QuicConfig(),
              QuicCryptoServerConfig::ConfigOptions(),
              supported_versions,
              kQuicDefaultConnectionIdLength,
              engine,
              backend){}

Ns3QuicServer::Ns3QuicServer(std::unique_ptr<ProofSource> proof_source,
const QuicConfig& config,
const QuicCryptoServerConfig::ConfigOptions& crypto_config_options,
const ParsedQuicVersionVector& supported_versions,
uint8_t expected_server_connection_id_length,
Ns3QuicAlarmEngine *engine,
Ns3QuicBackendBase *backend):
config_(config),
crypto_config_(kSourceAddressTokenSecret,
               QuicRandom::GetInstance(),
               std::move(proof_source),
               KeyExchangeSource::Default()),
crypto_config_options_(crypto_config_options),
version_manager_(supported_versions),
expected_server_connection_id_length_(expected_server_connection_id_length),
engine_(engine),
backend_(backend)
{
    clock_=Ns3QuicClock::Instance();
    Initialize();
}
      
Ns3QuicServer::~Ns3QuicServer()=default;

void Ns3QuicServer::ProcessPacket(const QuicSocketAddress& self_address,
                 const QuicSocketAddress& peer_address,
                 const QuicReceivedPacket& packet){
    //TODO
    if(dispatcher_){
        dispatcher_->ProcessBufferedChlos(kNumSessionsToCreatePerSocketEvent);
        dispatcher_->ProcessPacket(self_address,peer_address,packet);
    }
}
void Ns3QuicServer::Shutdown(){
    if(dispatcher_){
        dispatcher_->Shutdown();
    }
}
bool Ns3QuicServer::ConfigureDispatcher(QuicPacketWriter *writer){
    bool ret=false;
    if(!dispatcher_){
        QUICHE_DCHECK(writer!=nullptr);
        dispatcher_.reset(CreateQuicDispatcher());
        dispatcher_->InitializeWithWriter(writer);
        ret=true;
    }
    return ret;
}
QuicDispatcher* Ns3QuicServer::CreateQuicDispatcher(){
    std::unique_ptr<Ns3AlarmFactory> alarm_factory(new Ns3AlarmFactory(engine_));
    std::unique_ptr<QuicCryptoServerStreamBase::Helper> session_helper(new Ns3QuicCryptoServerStreamHelper);
    return new Ns3QuicDispatcher(&config_,
                                &crypto_config_,
                                &version_manager_,
                                std::make_unique<Ns3QuicConnectionHelper>(QuicAllocatorType::BUFFER_POOL),
                                std::move(session_helper),
                                std::move(alarm_factory),
                                expected_server_connection_id_length_,
                                backend_);
}
void Ns3QuicServer::Initialize(){
    if (config_.GetInitialStreamFlowControlWindowToSend() ==
        kDefaultFlowControlSendWindow) {
        config_.SetInitialStreamFlowControlWindowToSend(
        kInitialStreamFlowControlWindow);
    }
    if (config_.GetInitialSessionFlowControlWindowToSend() ==
        kDefaultFlowControlSendWindow) {
        config_.SetInitialSessionFlowControlWindowToSend(
        kInitialSessionFlowControlWindow);
    }
}
}
