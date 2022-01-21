#pragma once
#include <map>
#include "absl/strings/string_view.h"
#include "gquiche/quic/core/quic_dispatcher.h"
namespace quic{
class Ns3QuicBackendBase;
class Ns3QuicDispatcher:public QuicDispatcher{
public:
    Ns3QuicDispatcher(
        const QuicConfig* config,
        const QuicCryptoServerConfig* crypto_config,
        QuicVersionManager* version_manager,
        std::unique_ptr<QuicConnectionHelperInterface> helper,
        std::unique_ptr<QuicCryptoServerStreamBase::Helper> session_helper,
        std::unique_ptr<QuicAlarmFactory> alarm_factory,
        uint8_t expected_server_connection_id_length,
        Ns3QuicBackendBase *backend);
    
    ~Ns3QuicDispatcher() override;
    
    int GetRstErrorCount(QuicRstStreamErrorCode rst_error_code) const;
    
    void OnRstStreamReceived(const QuicRstStreamFrame& frame) override;
    //GetSupportedVersions OnConnectionClosed  OnWriteBlocked OnStopSendingReceived
protected:
    std::unique_ptr<QuicSession> CreateQuicSession(
      QuicConnectionId server_connection_id,
      const QuicSocketAddress& self_address,
      const QuicSocketAddress& peer_address,
      absl::string_view alpn,
      const ParsedQuicVersion& version,
      absl::string_view sni) override;
private:
    Ns3QuicBackendBase *backend_;
    // The map of the reset error code with its counter.
    std::map<QuicRstStreamErrorCode, int> rst_error_map_;
};
}
