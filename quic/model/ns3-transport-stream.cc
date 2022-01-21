#include "ns3-transport-stream.h"
#include <sys/types.h>

#include "gquiche/quic/core/quic_buffer_allocator.h"
#include "gquiche/quic/core/quic_error_codes.h"
#include "gquiche/quic/core/quic_types.h"
#include "gquiche/quic/core/quic_utils.h"
#include "gquiche/quic/platform/api/quic_bug_tracker.h"
#include "gquiche/quic/platform/api/quic_logging.h"
namespace quic {

Ns3TransportStream::Ns3TransportStream(
    QuicStreamId id,
    QuicSession* session)
    : QuicStream(id,
                 session,
                 /*is_static=*/false,
                 QuicUtils::GetStreamType(id,
                                          session->connection()->perspective(),
                                          session->IsIncomingStream(id),
                                          session->version())){}
Ns3TransportStream::~Ns3TransportStream(){
    if(visitor_){
        visitor_->OnDestroy();
    }
}
size_t Ns3TransportStream::Read(char* buffer, size_t buffer_size,bool &fin) {
  iovec iov;
  iov.iov_base = buffer;
  iov.iov_len = buffer_size;
  const size_t result = sequencer()->Readv(&iov, 1);
  if (sequencer()->IsClosed()) {
    OnFinRead();
    fin=true;
  }
  return result;
}

size_t Ns3TransportStream::Read(std::string* output,bool &fin) {
  const size_t old_size = output->size();
  const size_t bytes_to_read = ReadableBytes();
  output->resize(old_size + bytes_to_read);
  size_t bytes_read = Read(&(*output)[old_size], bytes_to_read,fin);
  QUICHE_DCHECK_EQ(bytes_to_read, bytes_read);
  output->resize(old_size + bytes_read);
  return bytes_read;
}

bool Ns3TransportStream::Write(absl::string_view data) {
  if (!CanWrite()) {
    return false;
  }

  QuicUniqueBufferPtr buffer = MakeUniqueBuffer(
      session()->connection()->helper()->GetStreamSendBufferAllocator(),
      data.size());
  memcpy(buffer.get(), data.data(), data.size());
  QuicMemSlice memslice(std::move(buffer), data.size());
  QuicConsumedData consumed =
      WriteMemSlices(QuicMemSliceSpan(&memslice), /*fin=*/false);


  if (consumed.bytes_consumed == data.size()) {
    return true;
  }
  if (consumed.bytes_consumed == 0) {
    return false;
  }
  QUIC_BUG(Ns3TransportStream)<< "WriteMemSlices() unexpectedly partially consumed the input "
              "data, provided: "
           << data.size() << ", written: " << consumed.bytes_consumed;
  OnUnrecoverableError(
      QUIC_INTERNAL_ERROR,
      "WriteMemSlices() unexpectedly partially consumed the input data");
  return false;
}
bool Ns3TransportStream::Write(const char*data,size_t size,bool fin){
  if(!data&&fin){
      return SendFin();
  }
  if (!CanWrite()) {
      return false;
  }

  QuicUniqueBufferPtr buffer = MakeUniqueBuffer(
      session()->connection()->helper()->GetStreamSendBufferAllocator(),
      size);
  memcpy(buffer.get(), data, size);
  QuicMemSlice memslice(std::move(buffer),size);
  QuicConsumedData consumed =
      WriteMemSlices(QuicMemSliceSpan(&memslice),fin);

  if (consumed.bytes_consumed ==size) {
    return true;
  }
  if (consumed.bytes_consumed == 0) {
    return false;
  }
  QUIC_BUG(Ns3TransportStream)<< "WriteMemSlices() unexpectedly partially consumed the input "
              "data, provided: "
           << size << ", written: " << consumed.bytes_consumed;
  OnUnrecoverableError(
      QUIC_INTERNAL_ERROR,
      "WriteMemSlices() unexpectedly partially consumed the input data");
  return false;    
}
bool Ns3TransportStream::SendFin() {
  if (!CanWrite()) {
    return false;
  }

  QuicMemSlice empty;
  QuicConsumedData consumed =
      WriteMemSlices(QuicMemSliceSpan(&empty), /*fin=*/true);
  QUICHE_DCHECK_EQ(consumed.bytes_consumed, 0u);
  return consumed.fin_consumed;
}

bool Ns3TransportStream::CanWrite() const {
  return CanWriteNewData() &&!write_side_closed();
}

size_t Ns3TransportStream::ReadableBytes() const {
  return sequencer()->ReadableBytes();
}

void Ns3TransportStream::OnDataAvailable() {
    if (sequencer()->IsClosed()) {
        OnFinRead();
        return;
    }
    
    if (visitor_ == nullptr) {
        return;
    }
    if (ReadableBytes() == 0) {
        return;
    }
    visitor_->OnCanRead();
}

void Ns3TransportStream::OnCanWriteNewData() {
  // Ensure the origin check has been completed, as the stream can be notified
  // about being writable before that.
  if (!CanWrite()) {
    return;
  }
  if (visitor_ != nullptr) {
    visitor_->OnCanWrite();
  }
}
}  // namespace quic
