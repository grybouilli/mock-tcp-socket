#include <thread>       // thread
#include <cerrno>       // errno
#include <cstring>      // strerror
#include <cstdio>       // fprintf
#include <bit>          // std::bit_cast

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "Client_handler.hpp"
#include "mocks.hpp"

struct TestStruct
{
    TestStruct()
    : socket {}
    , client_handler (socket.id)
    {
    }

    ~TestStruct()
    {
    }

    MockSocket socket;
    mutable Client_handler client_handler;
};

TEST_CASE_PERSISTENT_FIXTURE( TestStruct, "Testing Client_handler functionalities") 
{
    SECTION( "Unit test: handle_hello" )
    {
        header_t to_send;
        to_send.cmd = MSG_HELLO;
        to_send.len = sizeof to_send;
        send(socket.id, &to_send, to_send.len, MockSocket::client);

        // Leave time for handler thread to respond
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); 

        std::vector<char> client_receive_buffer (to_send.len, 0);
        recv(socket.id, client_receive_buffer.data(), to_send.len, MockSocket::client);

        // Check that we received data and that it is equal to what we sent
        REQUIRE_THAT(std::string(std::bit_cast<char*>(&to_send), to_send.len), 
            Catch::Matchers::Equals(std::string(client_receive_buffer.begin(), client_receive_buffer.end())));

        std::unique_lock<std::mutex> lock(MockSocket::mutexes[socket.id]);
        MockSocket::pipes.erase(socket.id); // invalidate mock socket
    }
}