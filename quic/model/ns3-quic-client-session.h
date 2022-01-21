#pragma once

#include <cstdint>
#include <memory>
#include "absl/strings/string_view.h"
#include "gquiche/quic/core/crypto/quic_crypto_client_config.h"
#include "gquiche/quic/core/quic_config.h"
#include "gquiche/quic/core/quic_connection.h"
#include "gquiche/quic/core/quic_crypto_client_stream.h"
#include "gquiche/quic/core/quic_crypto_stream.h"
#include "gquiche/quic/core/quic_server_id.h"
#include "gquiche/quic/core/quic_session.h"
#include "gquiche/quic/core/quic_stream.h"
#include "gquiche/quic/core/quic_versions.h"
#include "gquiche/quic/platform/api/quic_bug_tracker.h"
#include "gquiche/quic/platform/api/quic_containers.h"
#include "gquiche/quic/quic_transport/quic_transport_protocol.h"

#include "ns3/ns3-quic-session-base.h"
namespace quic{
class Ns3QuicBackendBase;
class Ns3QuicEndpointBase;
class Ns3QuicClientSession:public Ns3QuicSessionBase,
public QuicCryptoClientStream::ProofHandler{
public:
    Ns3QuicClientSession(std::unique_ptr<QuicConnection> connection_ptr,
                        Visitor* owner,
                        const QuicConfig& config,
                        const ParsedQuicVersionVector& supported_versions,
                        const QuicServerId& server_id,
                        QuicCryptoClientConfig* crypto_config,
                        Ns3QuicBackendBase *backend);
    ~Ns3QuicClientSession() override;
    //from Ns3QuicSessionBase 
    Ns3TransportStream* OpenOutgoingBidirectionalStream() override;
    Ns3TransportStream* OpenOutgoingUnidirectionalStream() override;
    
    void Initialize() override; 
    std::vector<std::string> GetAlpnsToOffer() const override {
        return std::vector<std::string>({QuicTransportAlpn()});
    }
    void OnAlpnSelected(absl::string_view alpn) override;
    bool alpn_received() const { return alpn_received_; }
    
    void CryptoConnect() { crypto_stream_->CryptoConnect(); }
    
    bool ShouldKeepConnectionAlive() const override { return true; }
    
    QuicCryptoStream* GetMutableCryptoStream() override {
        return crypto_stream_.get();
    }
    const QuicCryptoStream* GetCryptoStream() const override {
        return crypto_stream_.get();
    }
    QuicStream* CreateIncomingStream(QuicStreamId id) override;
    QuicStream* CreateIncomingStream(PendingStream* /*pending*/) override;
    void SetDefaultEncryptionLevel(EncryptionLevel level) override;
    void OnTlsHandshakeComplete() override;
    void OnMessageReceived(absl::string_view message) override;

    // QuicCryptoClientStream::ProofHandler implementation.
    void OnProofValid(const QuicCryptoClientConfig::CachedState& cached) override;
    void OnProofVerifyDetailsAvailable(const ProofVerifyDetails& verify_details) override;
    bool IsSessionReady() const{return ready_;}
    // Returns the number of client hello messages that have been sent on the
    // crypto stream. If the handshake has completed then this is one greater
    // than the number of round-trips needed for the handshake.
    int GetNumSentClientHellos() const;
    // Returns true if early data (0-RTT data) was sent and the server accepted
    // it.
    bool EarlyDataAccepted() const;
    
    // Returns true if the handshake was delayed one round trip by the server
    // because the server wanted proof the client controls its source address
    // before progressing further. In Google QUIC, this would be due to an
    // inchoate REJ in the QUIC Crypto handshake; in IETF QUIC this would be due
    // to a Retry packet.
    // TODO(nharper): Consider a better name for this method.
    bool ReceivedInchoateReject() const;
    
    int GetNumReceivedServerConfigUpdates() const;
    // Returns true if the session has active request streams.
    bool HasActiveRequestStreams() const;
protected:
    Ns3TransportStream* CreateStream(QuicStreamId id);
    void CloseSession();
    void OnCanCreateNewOutgoingStream(bool unidirectional) override;
    std::unique_ptr<QuicCryptoClientStream> crypto_stream_;
    Ns3QuicBackendBase *backend_;
    std::unique_ptr<Ns3QuicEndpointBase> endpoint_;
    bool alpn_received_ = false;
    bool ready_=false;
    std::unique_ptr<QuicConnection> connection_own_;
};
}
