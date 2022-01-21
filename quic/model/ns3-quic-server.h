#pragma once
#include <memory>
#include "absl/strings/string_view.h"
#include "gquiche/quic/core/quic_packet_writer.h"
#include "gquiche/quic/core/crypto/quic_crypto_server_config.h"
#include "gquiche/quic/core/quic_config.h"
#include "gquiche/quic/core/quic_version_manager.h"
#include "gquiche/quic/core/crypto/proof_source.h"
namespace quic{
class QuicDispatcher;
class Ns3QuicAlarmEngine;
class Ns3QuicBackendBase;
class Ns3QuicServer{
public:
    Ns3QuicServer(std::unique_ptr<ProofSource> proof_source,
            Ns3QuicAlarmEngine *engine,
            Ns3QuicBackendBase *backend);
    Ns3QuicServer(std::unique_ptr<ProofSource> proof_source,
                const ParsedQuicVersionVector& supported_versions,
                Ns3QuicAlarmEngine *engine,
                Ns3QuicBackendBase *backend);
    Ns3QuicServer(std::unique_ptr<ProofSource> proof_source,
            const QuicConfig& config,
            const QuicCryptoServerConfig::ConfigOptions& crypto_config_options,
            const ParsedQuicVersionVector& supported_versions,
            uint8_t expected_server_connection_id_length,
            Ns3QuicAlarmEngine *engine,
            Ns3QuicBackendBase *backend);
    Ns3QuicServer(const Ns3QuicServer&) = delete;
    Ns3QuicServer& operator=(const Ns3QuicServer&) = delete;
    ~Ns3QuicServer();

    void ProcessPacket(const QuicSocketAddress& self_address,
                     const QuicSocketAddress& peer_address,
                     const QuicReceivedPacket& packet);
    void Shutdown();
    bool ConfigureDispatcher(QuicPacketWriter *writer);
protected:
    virtual QuicDispatcher* CreateQuicDispatcher();
private:
    void Initialize();
    // config_ contains non-crypto parameters that are negotiated in the crypto
    // handshake.
    QuicConfig config_;
    // crypto_config_ contains crypto parameters for the handshake.
    QuicCryptoServerConfig crypto_config_;
    // crypto_config_options_ contains crypto parameters for the handshake.
    QuicCryptoServerConfig::ConfigOptions crypto_config_options_;
    
    // Used to generate current supported versions.
    QuicVersionManager version_manager_;
    uint8_t expected_server_connection_id_length_;
    Ns3QuicAlarmEngine *engine_=nullptr;
    Ns3QuicBackendBase *backend_=nullptr;
    QuicClock *clock_=nullptr;
    std::unique_ptr<QuicDispatcher> dispatcher_;

};
}
