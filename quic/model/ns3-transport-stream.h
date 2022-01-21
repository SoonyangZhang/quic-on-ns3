#pragma once

#include <cstddef>
#include <memory>

#include "absl/strings/string_view.h"
#include "gquiche/quic/core/quic_session.h"
#include "gquiche/quic/core/quic_stream.h"
#include "gquiche/quic/core/quic_types.h"

namespace quic{
class  Ns3TransportStream : public QuicStream {
public:
    class Visitor {
    public:
        virtual ~Visitor() {}
        virtual void OnCanRead() = 0;
        virtual void OnCanWrite() = 0;
        virtual void OnDestroy()=0;
    };

    Ns3TransportStream(QuicStreamId id,
                        QuicSession* session);
    ~Ns3TransportStream();
    // Reads at most |buffer_size| bytes into |buffer| and returns the number of
    // bytes actually read.
    size_t Read(char* buffer, size_t buffer_size,bool &fin);
    // Reads all available data and appends it to the end of |output|.
    size_t Read(std::string* output,bool &fin);
    // Writes |data| into the stream.  Returns true on success.
    ABSL_MUST_USE_RESULT bool Write(absl::string_view data);
    ABSL_MUST_USE_RESULT bool Write(const char*data,size_t size,bool fin);
    // Sends the FIN on the stream.  Returns true on success.
    ABSL_MUST_USE_RESULT bool SendFin();

    // Indicates whether it is possible to write into stream right now.
    bool CanWrite() const;
    // Indicates the number of bytes that can be read from the stream.
    size_t ReadableBytes() const;
    
    // QuicSession method implementations.
    void OnDataAvailable() override;
    void OnCanWriteNewData() override;
    void set_visitor(std::unique_ptr<Visitor> visitor) {
        visitor_ =std::move(visitor);
    }
protected:
    std::unique_ptr<Visitor> visitor_ = nullptr;
    bool fin_read_notified_ = false;
};
}

