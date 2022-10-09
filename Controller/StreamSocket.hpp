//
//  StreamSocket.hpp
//  Controller
//
//  Created by FireWolf on 2/24/21.
//  Revised by FireWolf on 2/21/22.
//      - Use RAII-style resource management.
//

#ifndef StreamSocket_hpp
#define StreamSocket_hpp

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <exception>
#include <string>
#include <fmt/format.h>
#include <cerrno>
#include <optional>

using SocketAddress4 = std::pair<in_addr_t, in_port_t>;
using SocketAddress6 = std::pair<in6_addr, in_port_t>;

struct SocketException: std::exception
{
    std::string message;

    explicit SocketException(std::string message) : message(std::move(message)) {}

    template <typename... Args>
    explicit SocketException(std::string format, Args&&... args) : message(fmt::vformat(format, fmt::make_format_args(std::forward<Args>(args)...))) {}

    [[nodiscard]]
    const char* what() const noexcept override
    {
        return this->message.c_str();
    }
};

struct SocketAddressConverter
{
    sockaddr_in operator()(SocketAddress4 address)
    {
        return { .sin_family = AF_INET, .sin_port = htons(address.second), .sin_addr = { htonl(address.first) } };
    }

    sockaddr_in6 operator()(SocketAddress6 address)
    {
        return { .sin6_family = AF_INET6, .sin6_port = htons(address.second), .sin6_addr = address.first };
    }
};

struct SocketAddressPrinter
{
    std::string operator()(SocketAddress4 address)
    {
        char buffer[INET_ADDRSTRLEN] = {};

        in_addr inAddr = { htonl(address.first) };

        inet_ntop(AF_INET, &inAddr, buffer, sizeof(buffer));

        return fmt::format("{}:{}", buffer, address.second);
    }

    std::string operator()(SocketAddress6 address)
    {
        char buffer[INET6_ADDRSTRLEN] = {};

        inet_ntop(AF_INET6, &address.first, buffer, sizeof(buffer));

        return fmt::format("{}:{}", buffer, address.second);
    }
};

struct StreamSocket
{
private:
    /// The socket descriptor managed by this class
    int descriptor;

    //
    // MARK: - Constructor & Destructor
    //

private:
    ///
    /// Create a stream socket from the given socket descriptor
    ///
    /// @param descriptor A socket descriptor
    ///
    explicit StreamSocket(int descriptor) : descriptor(descriptor) {}

    ///
    /// Create a stream socket from the given pair of local and remote socket address
    ///
    /// @tparam SocketAddress Specify the type of the socket address: either `SocketAddress4` or `SocketAddress6`
    /// @param domain Pass `PF_INET` for IPv4 domain or `PF_INET6` for IPv6 domain
    /// @param local The socket address on the local machine
    /// @param remote The socket address on the remote machine
    /// @throws SocketException if failed to create the socket descriptor;
    ///                         if failed to bind the socket to the given local address;
    ///                         if failed to connect the socket to the given remote address.
    /// @note The caller should pass
    ///
    template <typename SocketAddress>
    requires std::same_as<SocketAddress, SocketAddress4> || std::same_as<SocketAddress, SocketAddress6>
    StreamSocket(int domain, SocketAddress local, SocketAddress remote)
    {
        // Guard: Create a socket descriptor
        this->descriptor = socket(domain, SOCK_STREAM, 0);

        if (this->descriptor < 0)
        {
            throw SocketException("Failed to create a socket descriptor.");
        }

        // Guard: Bind the socket to the given local address
        auto laddr = SocketAddressConverter{}(local);

        if (bind(this->descriptor, reinterpret_cast<sockaddr*>(&laddr), sizeof(laddr)) != 0)
        {
            close(this->descriptor);

            throw SocketException("Failed to bind the socket to {}. Reason: {}.",
                                  SocketAddressPrinter{}(local), strerror(errno));
        }

        // Guard: Connect the socket to the given remote address
        auto raddr = SocketAddressConverter{}(remote);

        if (connect(this->descriptor, reinterpret_cast<sockaddr*>(&raddr), sizeof(raddr)) != 0)
        {
            close(this->descriptor);

            throw SocketException("Failed to connect the socket to {}. Reason: {}.",
                                  SocketAddressPrinter{}(remote), strerror(errno));
        }
    }

public:
    ///
    /// Create a stream socket with the given IPv4 socket addresses
    ///
    /// @param local The socket address on the local machine
    /// @param remote The socket address on the remote machine
    /// @throws SocketException if failed to create the socket descriptor;
    ///                         if failed to bind the socket to the given local address;
    ///                         if failed to connect the socket to the given remote address.
    ///
    StreamSocket(SocketAddress4 local, SocketAddress4 remote) : StreamSocket(PF_INET, local, remote) {}

    ///
    /// Create a stream socket with the given IPv6 socket addresses
    ///
    /// @param local The socket address on the local machine
    /// @param remote The socket address on the remote machine
    /// @throws SocketException if failed to create the socket descriptor;
    ///                         if failed to bind the socket to the given local address;
    ///                         if failed to connect the socket to the given remote address.
    ///
    StreamSocket(SocketAddress6 local, SocketAddress6 remote) : StreamSocket(PF_INET6, local, remote) {}

    /// The copy constructor is not available
    StreamSocket(const StreamSocket& other) = delete;

    /// The move constructor transfers the ownership of the managed socket descriptor
    StreamSocket(StreamSocket&& other) noexcept : descriptor(other.descriptor)
    {
        other.descriptor = -1;
    }

    ///
    /// Release the stream socket
    ///
    /// @note The destructor closes the managed socket if the descriptor is valid.
    ///
    ~StreamSocket()
    {
        if (this->descriptor >= 0)
        {
            close(this->descriptor);
        }
    }

    /// Copy assignment is not available
    StreamSocket& operator=(const StreamSocket& other) = delete;

    /// Move assignment transfers the ownership of the socket descriptor
    StreamSocket& operator=(StreamSocket&& other) noexcept
    {
        if (this != &other)
        {
            this->descriptor = other.descriptor;

            other.descriptor = -1;
        }

        return *this;
    }

    //
    // MARK: - Socket Communication
    //

    ///
    /// Send the given data to the remote host
    ///
    /// @param data The data to send
    /// @param length The number of bytes to sent
    /// @return `true` if the operation completes successfully, `false` otherwise.
    ///
    inline bool send(const void* data, size_t length) const
    {
        ssize_t result = ::send(this->descriptor, data, length, 0);

        return result >= 0 && static_cast<size_t>(result) == length;
    }

    ///
    /// Send the given object to the remote host
    ///
    /// @param object An object to send
    /// @return `true` if the operation completes successfully, `false` otherwise.
    ///
    template <typename Object>
    bool send(const Object& object) const
    {
        return this->send(&object, sizeof(Object));
    }

    ///
    /// Receive data from the remote host
    ///
    /// @param data A non-null buffer to hold the received data
    /// @param length The maximum number of bytes that the buffer can hold;
    ///               The actual number of bytes received on return
    /// @return `true` on success, `false` otherwise.
    ///
    inline bool receive(void* data, size_t& length) const
    {
        ssize_t result = recv(this->descriptor, data, length, 0);

        if (result <= 0)
        {
            return false;
        }

        length = static_cast<size_t>(result);

        return true;
    }

    ///
    /// Receive a fixed amount of data from the remote host
    ///
    /// @param data A non-null buffer that is large enough to hold `length` bytes
    /// @param length The number of bytes to receive from the remote host
    /// @return `true` on success, `false` otherwise.
    /// @note This caller remains blocked until the designated number of bytes is received from the remote host.
    ///
    inline bool receiveWithLength(void* data, size_t length) const
    {
        size_t offset = 0;

        while (offset < length)
        {
            ssize_t result = recv(this->descriptor, reinterpret_cast<uint8_t*>(data) + offset, length - offset, 0);

            if (result <= 0)
            {
                return false;
            }

            offset += static_cast<size_t>(result);
        }

        return true;
    }

    ///
    /// Receive an object from the remote host
    ///
    /// @return The object on success, `std::nullopt` otherwise.
    ///
    template <typename Object>
    requires std::default_initializable<Object>
    std::optional<Object> receive() const
    {
        Object object;

        if (this->receiveWithLength(&object, sizeof(Object)))
        {
            return object;
        }
        else
        {
            return std::nullopt;
        }
    }
};

#endif /* StreamSocket_hpp */
