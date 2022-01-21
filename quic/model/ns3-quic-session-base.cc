#include "ns3-quic-session-base.h"
#include <iostream>
namespace quic{
Ns3QuicSessionBase::Ns3QuicSessionBase(QuicConnection* connection,
                      QuicSession::Visitor* owner,
                    const QuicConfig& config,
                    const ParsedQuicVersionVector& supported_versions,
                    QuicStreamCount num_expected_unidirectional_static_streams)
              :QuicSession(connection,owner,config,
                        supported_versions,num_expected_unidirectional_static_streams){}
}
