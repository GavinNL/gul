/*
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org>
 */

#ifndef GUL_SOCKET__H
#define GUL_SOCKET__H

#ifdef _MSC_VER
#define _WINSOCKAPI_
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif


#include <cstdio>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <string>

#if defined _MSC_VER

    #include <winsock2.h>
    #pragma comment(lib,"wsock32.lib")

#else
    #include <sys/ioctl.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/select.h>
    #include <sys/socket.h>
    #include <sys/un.h>
#endif


namespace gul
{

////////////////////////////////////////////////////////////////////////////////
/// New Implementations here!
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief The socket_address class
 *
 * The socket address class is used to identify the address of the
 * connected socket. It is used by the udp_socket to indicate which
 * client has sent a packet and to indicate which client to send a message
 * to.
 */
class socket_address
{
protected:

#if defined _MSC_VER
    using address_t = struct ::sockaddr_in;
#else
    using address_t = struct sockaddr_in;
#endif

public:
    socket_address()
    {
        memset( reinterpret_cast<char*>(&m_address), 0, sizeof(m_address));
    }

    socket_address(uint16_t _port)
    {
        memset( reinterpret_cast<char *>(&m_address), 0, sizeof(m_address));
        m_address.sin_family = AF_INET;
        m_address.sin_port = htons(_port);
        m_address.sin_addr.s_addr = INADDR_ANY;
    }

    socket_address(char const * ip_address, uint16_t _port)
    {
        //setup address structure
        memset( reinterpret_cast<char *>(&m_address), 0, sizeof(m_address));
        m_address.sin_family = AF_INET;
        m_address.sin_port = htons(_port);


        #if defined _MSC_VER
        m_address.sin_addr.S_un.S_addr = inet_addr(ip_address);
        #else
        m_address.sin_addr.s_addr = inet_addr(ip_address);

        #endif
    }

    operator bool()
    {
        return m_address.sin_port == 0;
    }

    /**
     * @brief native_address
     * @return
     * Returns the native address handle of the address strut
     */
    address_t const & native_address() const
    {
        return m_address;
    }

    /**
     * @brief native_address
     * @return
     * Returns the native address handle of the address strut
     */
    address_t & native_address()
    {
        return m_address;
    }

    /**
     * @brief ip
     * @return
     * Returns the ipaddress as a character string
     */
    char const * ip() const
    {
        return inet_ntoa(m_address.sin_addr);
    }

    /**
     * @brief port
     * @return
     * Returns the port number
     */
    uint16_t port() const
    {
        return ntohs(m_address.sin_port);
    }

    operator address_t()
    {
        return m_address;
        static_assert( sizeof(address_t) == sizeof(socket_address), "struct sizes are not the same");
    }
protected:
    address_t m_address;
};

enum class socket_domain
{
    NET,
    UNIX
};

enum class socket_type
{
    STREAM,
    DGRAM
};

class socket
{
protected:
#if defined _WIN32
    using native_msg_size_input_t  = int;// the input message length type for send/recv/sendto/recvfrom
    using native_msg_size_return_t = int;// the return type for send/recv/sendto/recvfrom
    using native_raw_buffer_t      = char;
    using socket_t                 = SOCKET;
#else
    using native_msg_size_input_t  = size_t; // the input message length type for send/recv/sendto/recvfrom
    using native_msg_size_return_t = ssize_t;// the return type for send/recv/sendto/recvfrom
    using native_raw_buffer_t      = void;
    using socket_t                 = int;    //
#endif

public:
#if defined _WIN32
    static const socket_t     invalid_socket = INVALID_SOCKET;
    static const int          socket_error   = SOCKET_ERROR;
    static const int          msg_error      = -1;
#else
    static const socket_t     invalid_socket = -1;
    static const int          socket_error   = -1;
    static const ssize_t      msg_error      = -1;
#endif

    static const int          bind_error     = -1;
    static const int          listen_error   = -1;
    static const int          connect_error   = -1;

    typedef std::int32_t   msg_size_t;

    static const msg_size_t error = -1;

    socket() : m_fd( invalid_socket )
    {
    }

    socket(socket const & other) : m_fd(other.m_fd)
    {
    }
    socket(socket && other) : m_fd(other.m_fd)
    {
        other.m_fd=invalid_socket;
    }
    socket & operator=(socket && other)
    {
        if( this != &other)
        {
            m_fd = other.m_fd;
            other.m_fd=invalid_socket;
        }
        return *this;
    }

    bool create(socket_domain d, socket_type t)
    {
        if( d == socket_domain::NET )
        {
            if( t == socket_type::STREAM)
            {
                return _create(AF_INET, SOCK_STREAM, 0);
            }
            else if( t==socket_type::DGRAM)
            {
                return _create(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            }
        }
        else if( d == socket_domain::UNIX)
        {
            if( t == socket_type::STREAM)
            {
                return _create(AF_UNIX, SOCK_STREAM, 0);
            }
            else if( t==socket_type::DGRAM)
            {
                return _create(AF_UNIX, SOCK_DGRAM, IPPROTO_UDP);
            }
        }
        return false;
    }

    bool bind(const std::string endpoint)
    {
        const auto separator = endpoint.find_last_of(':');

        //Check if input wasn't missformed
        if (separator == std::string::npos)
        {
            // possibly domain socket
            //throw std::runtime_error("string is not of address:port form");

            if( !create(socket_domain::UNIX, socket_type::STREAM) )
            {
                return false;
            }

            struct sockaddr_un d_name;
            memset(&d_name, 0 ,sizeof(struct sockaddr_un) );
            d_name.sun_family = AF_UNIX;
            strcpy(d_name.sun_path, endpoint.c_str());

            ::unlink( d_name.sun_path );
            int ret = ::bind(m_fd, reinterpret_cast<const struct sockaddr *>(&d_name),
                           sizeof(struct sockaddr_un));
            if( ret == bind_error)
                return false;

            return true;

        }
        else
        {
            if (separator == endpoint.size() - 1)
            {
                return false;
                //throw std::runtime_error("string has ':' as last character. Expected port number here");
            }

            //Isolate address
            std::string address = endpoint.substr(0, separator);

            //Read from string as unsigned
            const auto port = static_cast<uint16_t>( strtoul(endpoint.substr(separator + 1).c_str(), nullptr, 10) );

            if( !create(socket_domain::NET, socket_type::STREAM) )
            {
                return false;
            }
            return _bind( socket_address(port));
        }
    }

    socket accept()
    {
        socket client;

        int length   = sizeof( m_address.native_address() );

#if defined _MSC_VER
        using socklen_t = int;
#endif
        client.m_fd  = ::accept(m_fd, reinterpret_cast<struct sockaddr*>(&m_address.native_address()), reinterpret_cast<socklen_t*>(&length));

        ::getpeername(client.m_fd , reinterpret_cast<struct sockaddr *>(&client.m_address.native_address()), reinterpret_cast<socklen_t*>(&length) );

        return client;

    }

    /**
     * @brief close
     * Closes the socket. The socket will cast to false after this has been
     * called.
     */
    void close()
    {
        #ifdef _MSC_VER
            ::closesocket(m_fd);
        #else
            ::shutdown(m_fd, SHUT_RDWR);
            ::close(m_fd);
        #endif
        m_fd = invalid_socket;
    }

    /**
     * @brief native_handle
     * @return
     *
     * Returns the native handle of the socket descriptor
     */
    socket_t native_handle() const
    {
        return m_fd;
    }


    bool set_recv_timeout(std::chrono::microseconds ms)
    {
        #ifdef _MSC_VER
        // WINDOWS
        DWORD timeout = ms.count() / 1000;
        if( setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof timeout) != 0)
        {
            return false;
        }
        #else
        struct timeval timeout;
        timeout.tv_sec  = static_cast< decltype(timeout.tv_usec) >( ms.count() / 1000000u );
        timeout.tv_usec = static_cast< decltype(timeout.tv_usec) >( ms.count() % 1000000u );

        if (setsockopt (m_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        {
            return false;
            //error("setsockopt failed\n");
        }
        #endif

        return true;
    }


    bool set_send_timeout(std::chrono::microseconds ms)
    {
        #ifdef _MSC_VER
        // WINDOWS
        DWORD timeout = ms.count() / 1000;
        if( setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof timeout) != 0)
        {
            return false;
        }
        #else
        struct timeval timeout;
        timeout.tv_sec  = static_cast< decltype(timeout.tv_usec) >( ms.count() / 1000000u );
        timeout.tv_usec = static_cast< decltype(timeout.tv_usec) >( ms.count() % 1000000u );

        if (setsockopt (m_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        {
            return false;
            //error("setsockopt failed\n");
        }
        #endif

        return true;
    }

    bool listen( std::size_t max_connections)
    {
        decltype(socket_error) code = ::listen( m_fd, static_cast<int>(max_connections));

        if( code == socket_error)
        {
            return false;
        }
        return true;
    }

    operator bool() const
    {
        return !( ( m_fd == invalid_socket ) );
    }

    //=============================================================================
    msg_size_t  sendto(void const * data, size_t length, socket_address const & addr)
    {
        native_msg_size_return_t ret = ::sendto(m_fd,
                                                reinterpret_cast<native_raw_buffer_t const*>(data),
                                                static_cast<native_msg_size_input_t>(length&0xFFFFFFFF) ,
                                                0 ,
                                                reinterpret_cast<struct sockaddr const *>(&addr.native_address()),
                                                sizeof(struct sockaddr_in ));
        if ( ret == msg_error)
        {
            #ifdef _MSC_VER
            //printf("Send failed with error code : %d" , WSAGetLastError() );
            #else
            //printf("Send failed with error code : %d : %s" , errno,  strerror(errno) );
            #endif
            return msg_size_t(ret);
        }
        return msg_size_t(ret);
    }

    msg_size_t recvfrom(void * buf, size_t length, socket_address & addr)
    {
#if defined _MSC_VER
        int slen =  sizeof(struct sockaddr_in);
#else
        socklen_t slen = sizeof(struct sockaddr_in);
#endif
        native_msg_size_return_t ret  = ::recvfrom( m_fd, reinterpret_cast<native_raw_buffer_t*>(buf), static_cast<native_msg_size_input_t>(length&0xFFFFFFFF), 0, reinterpret_cast<struct sockaddr *>(&addr.native_address()), &slen);

        if (ret == msg_error)
        {
            #ifdef _MSC_VER
            //printf("Recv failed with error code : %d" , WSAGetLastError() );
            #else
            //printf("Recv failed with error code : %d : %s" , errno,  strerror(errno) );
            #endif
            //return msg_error;
        }
        return msg_size_t(ret);
    }



    msg_size_t send( void const * data, size_t _size)
    {
        native_msg_size_return_t ret = ::send(m_fd, reinterpret_cast<const native_raw_buffer_t*>(data), static_cast<native_msg_size_input_t>(_size&0xFFFFFFFF), 0);

        return msg_size_t(ret);
    }

    msg_size_t recv(void * data, size_t _size, bool wait_for_all=true)
    {
        //bool wait_for_all = true; // default for now.

        native_msg_size_return_t t = ::recv( m_fd, reinterpret_cast<native_raw_buffer_t*>(data), static_cast<native_msg_size_input_t>(_size&0xFFFFFFFF), wait_for_all ? MSG_WAITALL : 0 );

        if( t == 0 && _size != 0 ) // gracefully closed
        {
            m_fd = invalid_socket;
        }
        return msg_size_t(t);
    }

    //=============================================================================
    protected:
        socket_t m_fd = invalid_socket;
        socket_address m_address;

        bool _create(int _domain, int _type, int _protocol)
        {
        #ifdef _MSC_VER
            WSADATA wsa;
            if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
            {
                //printf("Failed. Error Code : %d",WSAGetLastError());
                return false;
            }
        #endif
            if ( (m_fd=::socket(_domain, _type, _protocol)) == invalid_socket)
            {
                #ifdef _MSC_VER
                //printf("Create failed with error code : %d\n" , WSAGetLastError() );
                #else
                //printf("Create failed with error code : %d : %s\n" , errno,  strerror(errno) );
                #endif
                return false;
            }
            return true;
        }
        bool _bind(socket_address const & addr)
        {
            auto ret = ::bind(m_fd, reinterpret_cast<struct sockaddr const*>(&addr.native_address()) , sizeof(socket_address));

            if( ret == bind_error)
            {
                #ifdef _MSC_VER
                //printf("Bind failed with error code : %d" , WSAGetLastError() );
                #else
                //printf("Bind failed with error code : %d : %s" , errno,  strerror(errno) );
                #endif
                return false;
            }
            return true;
        }

};

}

#endif


