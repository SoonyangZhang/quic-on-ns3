#pragma once

#include <cstdint>
#include <memory>

#include "absl/strings/string_view.h"
#include "gquiche/quic/core/quic_connection.h"
#include "gquiche/quic/core/quic_crypto_server_stream_base.h"
#include "gquiche/quic/core/quic_session.h"
#include "gquiche/quic/quic_transport/quic_transport_protocol.h"

#include "ns3-quic-session-base.h"
namespace quic{
class Ns3QuicBackendBase;
class Ns3QuicEndpointBase;
class Ns3QuicServerSession:public Ns3QuicSessionBase{
public:
    Ns3QuicServerSession(std::unique_ptr<QuicConnection> connection_ptr,
                        Visitor* owner,
                        const QuicConfig& config,
                        const ParsedQuicVersionVector& supported_versions,
                        const QuicCryptoServerConfig* crypto_config,
                        QuicCompressedCertsCache* compressed_certs_cache,
                        Ns3QuicBackendBase *backend);
    ~Ns3QuicServerSession() override;
    //from Ns3QuicSessionBase 
    Ns3TransportStream* OpenOutgoingBidirectionalStream() override;
    Ns3TransportStream* OpenOutgoingUnidirectionalStream() override;
    
    std::vector<absl::string_view>::const_iterator SelectAlpn(
                    const std::vector<absl::string_view>& alpns) const override {
        return std::find(alpns.cbegin(), alpns.cend(), QuicTransportAlpn());
    }
    bool ShouldKeepConnectionAlive() const override { return true; }
    
    QuicCryptoServerStreamBase* GetMutableCryptoStream() override {
        return crypto_stream_.get();
    }
    const QuicCryptoServerStreamBase* GetCryptoStream() const override {
        return crypto_stream_.get();
    }
    QuicStream* CreateIncomingStream(QuicStreamId id) override;
    QuicStream* CreateIncomingStream(PendingStream* /*pending*/) override;
    
    bool IsSessionReady() const { return ready_; }
protected:
    void CloseSession();
    Ns3TransportStream* CreateStream(QuicStreamId id);
    Ns3QuicBackendBase *backend_=nullptr;
    std::unique_ptr<Ns3QuicEndpointBase> endpoint_=nullptr;
    std::unique_ptr<QuicConnection> connection_own_;
    std::unique_ptr<QuicCryptoServerStreamBase> crypto_stream_;
    bool ready_=false;
};
}
