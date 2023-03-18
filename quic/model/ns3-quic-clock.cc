#include <utility>

#include "ns3/simulator.h"
#include "ns3-quic-clock.h"
namespace quic{
template <typename T, typename O = std::nullptr_t>
class NoDestructor {
 public:
  // Not constexpr; just write static constexpr T x = ...; if the value should
  // be a constexpr.
  template <typename... Args>
  explicit NoDestructor(Args&&... args) {
    new (storage_) T(std::forward<Args>(args)...);
  }

  // Allows copy and move construction of the contained type, to allow
  // construction from an initializer list, e.g. for std::vector.
  explicit NoDestructor(const T& x) { new (storage_) T(x); }
  explicit NoDestructor(T&& x) { new (storage_) T(std::move(x)); }

  NoDestructor(const NoDestructor&) = delete;
  NoDestructor& operator=(const NoDestructor&) = delete;

  ~NoDestructor() = default;

  const T& operator*() const { return *get(); }
  T& operator*() { return *get(); }

  const T* operator->() const { return get(); }
  T* operator->() { return get(); }

  const T* get() const { return reinterpret_cast<const T*>(storage_); }
  T* get() { return reinterpret_cast<T*>(storage_); }

 private:
  alignas(T) char storage_[sizeof(T)];
};
Ns3QuicClock* Ns3QuicClock::Instance(){
    static NoDestructor<Ns3QuicClock> ins;
    return ins.get();
}
Ns3QuicClock::Ns3QuicClock()=default;
Ns3QuicClock::~Ns3QuicClock()=default;
QuicTime Ns3QuicClock::ApproximateNow() const{
    return Now();
}
QuicTime Ns3QuicClock::Now() const{
    int64_t now_us=ns3::Simulator::Now().GetMicroSeconds();
    return CreateTimeFromMicroseconds(now_us);
}
QuicWallTime Ns3QuicClock::WallNow() const{
    int64_t now_us=ns3::Simulator::Now().GetMicroSeconds();
    return QuicWallTime::FromUNIXMicroseconds(now_us);
}
}
