#pragma once
// The Linux socket and thread extensions the GS uses, with a fallback for
// hosts that lack them (macOS: the Palianytsia desktop GS). Linux and
// Android take the native call unchanged; only __APPLE__ takes the
// fallback.
//   socket_nb      socket(type | SOCK_NONBLOCK [| SOCK_CLOEXEC])
//   accept_nb      accept4(.., SOCK_NONBLOCK)
//   kMsgNoSignal   MSG_NOSIGNAL (macOS: 0, the socket carries SO_NOSIGPIPE)
//   set_thread_name  pthread_setname_np on the calling thread
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

namespace mabur {

#if defined(__APPLE__)

namespace posix_compat_detail {
// O_NONBLOCK (and FD_CLOEXEC) on fd, SO_NOSIGPIPE where the socket type
// takes it; closes fd and returns -1 when a flag cannot be set.
inline int finish_nb(int fd, bool cloexec) {
  if (fd < 0) return fd;
  const int fl = ::fcntl(fd, F_GETFL);
  if (fl < 0 || ::fcntl(fd, F_SETFL, fl | O_NONBLOCK) != 0 ||
      (cloexec && ::fcntl(fd, F_SETFD, FD_CLOEXEC) != 0)) {
    ::close(fd);
    return -1;
  }
  int one = 1;
  (void)::setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof one);
  return fd;
}
}  // namespace posix_compat_detail

inline int socket_nb(int domain, int type, bool cloexec = false) {
  return posix_compat_detail::finish_nb(::socket(domain, type, 0), cloexec);
}
inline int accept_nb(int fd) {
  return posix_compat_detail::finish_nb(::accept(fd, nullptr, nullptr), false);
}
inline constexpr int kMsgNoSignal = 0;
inline void set_thread_name(const char* name) { ::pthread_setname_np(name); }

#else

inline int socket_nb(int domain, int type, bool cloexec = false) {
  return ::socket(domain, type | SOCK_NONBLOCK | (cloexec ? SOCK_CLOEXEC : 0), 0);
}
inline int accept_nb(int fd) { return ::accept4(fd, nullptr, nullptr, SOCK_NONBLOCK); }
inline constexpr int kMsgNoSignal = MSG_NOSIGNAL;
inline void set_thread_name(const char* name) { ::pthread_setname_np(::pthread_self(), name); }

#endif

}  // namespace mabur
