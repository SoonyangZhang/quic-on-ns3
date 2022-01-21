#include "ns3-quic-dispatcher.h"
#include "ns3-quic-backend.h"
#include "ns3-quic-server-session.h"
namespace quic{
Ns3QuicDispatcher::Ns3QuicDispatcher(
const QuicConfig* config,
const QuicCryptoServerConfig* crypto_config,
QuicVersionManager* version_manager,
std::unique_ptr<QuicConnectionHelperInterface> helper,
std::unique_ptr<QuicCryptoServerStreamBase::Helper> session_helper,
std::unique_ptr<QuicAlarmFactory> alarm_factory,
uint8_t expected_server_connection_id_length,
Ns3QuicBackendBase *backend):
QuicDispatcher(config,crypto_config,version_manager,std::move(helper),
std::move(session_helper),std::move(alarm_factory),expected_server_connection_id_length),
backend_(backend){}

Ns3QuicDispatcher::~Ns3QuicDispatcher()=default;

int Ns3QuicDispatcher::GetRstErrorCount(
    QuicRstStreamErrorCode error_code) const {
    auto it = rst_error_map_.find(error_code);
    if (it == rst_error_map_.end()) {
        return 0;
    }
    return it->second;
}
void Ns3QuicDispatcher::OnRstStreamReceived(const QuicRstStreamFrame& frame) {
    auto it = rst_error_map_.find(frame.error_code);
    if (it == rst_error_map_.end()) {
        rst_error_map_.insert(std::make_pair(frame.error_code, 1));
    } else {
        it->second++;
    }
}
std::unique_ptr<QuicSession> Ns3QuicDispatcher::CreateQuicSession(
QuicConnectionId server_connection_id,
const QuicSocketAddress& self_address,
const QuicSocketAddress& peer_address,
absl::string_view /*alpn*/,
const ParsedQuicVersion& version,
absl::string_view /*sni*/){
  // The QuicServerSessionBase takes ownership of |connection| below.
  std::unique_ptr<QuicConnection> connection;
  connection.reset(new QuicConnection(
      server_connection_id,self_address, peer_address, helper(), alarm_factory(), writer(),
      /* owns_writer= */ false, Perspective::IS_SERVER,ParsedQuicVersionVector{version}));

  auto session = std::make_unique<Ns3QuicServerSession>(std::move(connection),this,
      config(), GetSupportedVersions(),crypto_config(), compressed_certs_cache(),
      backend_);
  session->Initialize();
  return session;
}
}
