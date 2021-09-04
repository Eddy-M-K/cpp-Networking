#pragma once
#include "net_common.h"

namespace olc
{
  namespace net
  {
    // Message Header is sent at start of all messages.The template allows us
    // to use "enum class" to ensure that the messages are valid at compile time
    template <typename T>
    struct message_header
    {
      T id{};
      // deliberately not using size_t due to differences on 32 and 64 bit machines
      uint32_t size = 0;
    };

    template <typename T>
    struct message
    {
      message_header<T> header{};
      std::vector<uint8_t> body;

      // returns the size of the entire message packet in bytes
      size_t size() const
      {
        return sizeof(message_header<T>) + body.size();
      }

      // Override "<<" operator for std::cout compatibility - produces friendly description of message
      friend std::ostream& operator << (std::ostream& os, const message<T>& msg)
      {
        os << "ID:" << int(msg.header.id) << " Size:" << msg.header.size;
        return os;
      }

      // Pushes any POD-like data in the message buffer
      template<typename Datatype> 
      friend message<T>& operator << (message<T>& msg, const DataType& data)
      {
        // Check that the type of the data being pushed is trivially copyable
        static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");

        // Cache current size of vector, as this will be the point we insert the data
        size_t i = msg.body.size();

        // Resize the vector by the size of the data being pushed
        msg.body.resize(msg.body.size() + sizeof(DataType));

        // Physically copy the data into the newly allocated vector space
        std::memcpy(msg.body.data() + i, &data, sizeof(DataType));

        // Recalculate the message size
        msg.header.size = msg.size();

        // Return the target message so it can be "chained"
        return msg;
      }

    };
  }
}
