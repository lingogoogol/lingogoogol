#pragma once
// The socket ownership code (request_control_t and the curl worker) is written
// against Winsock names. On POSIX systems sockets are file descriptors, and
// shutdown(SHUT_RDWR) likewise wakes a blocked TLS read without closing a
// descriptor that curl may still reference.
#if defined(_WIN32)
#error "Windows code includes <winsock2.h> directly."
#endif
#include <sys/socket.h>
#include <unistd.h>

using SOCKET = int;
inline constexpr SOCKET INVALID_SOCKET{-1};
inline constexpr int SD_BOTH{SHUT_RDWR};
inline auto closesocket(SOCKET socket) -> int { return ::close(socket); }
