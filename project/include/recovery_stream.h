#ifndef RECOVERY_STREAM_H
#define RECOVERY_STREAM_H

#include <asio.hpp>
#include <cstdint>
#include <string>

namespace ECProject
{

inline std::string recovery_stream_key(const std::string &block_key, int block_id,
                                       int recovery_offset, int recovery_size,
                                       const std::string &commit_token)
{
  return block_key + "\n" + std::to_string(block_id) + "\n" +
         std::to_string(recovery_offset) + "\n" +
         std::to_string(recovery_size) + "\n" + commit_token;
}

inline bool write_recovery_stream_key(asio::ip::tcp::socket &socket,
                                      const std::string &key,
                                      asio::error_code &error)
{
  const uint32_t size = static_cast<uint32_t>(key.size());
  asio::write(socket, asio::buffer(&size, sizeof(size)), error);
  if (error)
    return false;
  asio::write(socket, asio::buffer(key.data(), key.size()), error);
  return !error;
}

inline bool read_recovery_stream_key(asio::ip::tcp::socket &socket,
                                     std::string &key,
                                     asio::error_code &error)
{
  uint32_t size = 0;
  asio::read(socket, asio::buffer(&size, sizeof(size)), error);
  if (error || size == 0 || size > 64 * 1024)
    return false;
  key.resize(size);
  asio::read(socket, asio::buffer(key.data(), key.size()), error);
  return !error;
}

} // namespace ECProject

#endif
