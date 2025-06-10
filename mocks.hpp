#pragma once

#include <sys/socket.h> // recv, send
#include <map>          // map
#include <vector>       // vector
#include <mutex>        // mutex
#include <array>

struct MockSocket
{
    MockSocket()
    : id { socket(AF_INET, SOCK_STREAM, 0) }
    {
        pipes[id];
        mutexes[id];
    }

    static std::map<int, std::mutex> mutexes;
    static std::map<int, std::array<std::vector<char>, 2>> pipes;
    int id; 
    
    static const size_t client = 1;
    static const size_t server = 0;
};