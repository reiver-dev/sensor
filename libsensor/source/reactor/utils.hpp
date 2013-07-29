#ifndef UTILS_HPP_
#define UTILS_HPP_

#include "socket.hpp"
#include "buffer.hpp"

namespace net {
namespace Sock {


size_t bufferedSend(Socket sock, Buffer &buffer);
size_t bufferedRecv(Socket sock, Buffer &buffer);

}
}

#endif /* UTILS_HPP_ */
