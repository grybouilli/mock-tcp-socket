#include "mocks.hpp"
#include <cstring>
#include <algorithm>

std::map<int, std::mutex> MockSocket::mutexes {};
std::map<int, std::array<std::vector<char>, 2>> MockSocket::pipes {};

extern "C"
{
    /**
     * @brief Mock recv - Uses MockSocket::id as sockfd argument
     * 
     * @param sockfd 
     * @param buf 
     * @param len 
     * @param flags If MSG_OOB set, this receives from the client pipe; otherwise, it receives from the server pipe
     * @return ssize_t -1 if failure, the amount read otherwise
     */
    ssize_t recv(int sockfd, void * buf, size_t len, int flags)
    {

        int ret = -1;
        int read_end = flags == MockSocket::client ? 1 : 0;

        if(flags & MSG_PEEK)
        {
            ret = MockSocket::pipes.contains(sockfd) ? 1 : -1;
        }
        else if(MockSocket::pipes.contains(sockfd))
        {
            std::unique_lock<std::mutex> lock(MockSocket::mutexes[sockfd]);
            auto& socket = MockSocket::pipes[sockfd][read_end];
            ret = std::min(len, socket.size());
            if(ret == 0)
            {
                errno = EAGAIN;
                ret = -1;
            } else
            {
                //printf("read pipe size %d\n", ret);
                std::memcpy(buf, socket.data(), ret);
                socket.erase(socket.begin(), socket.begin()+ret);
            }
        }

        return ret;
    }


    /**
     * @brief Mock send - Uses MockSocket::id as sockfd argument
     * 
     * @param sockfd A MockSocket::id
     * @param buf 
     * @param len 
     * @param flags If MSG_OOB set, this send to the server reading pipe; otherwise, it sends to the client [reading] pipe
     * @return ssize_t -1 if failure, the amount sent otherwise
     */
    ssize_t send(int sockfd, const void * buf, size_t len, int flags)
    {
        // printf("mock send(%d, %s, %ld)\n", sockfd, (char *)buf, len);

        int ret = -1;
        int write_end = (flags == MockSocket::client) ? 0 : 1;

        if(MockSocket::pipes.contains(sockfd))
        {
            std::unique_lock<std::mutex> lock(MockSocket::mutexes[sockfd]);
            auto& socket = MockSocket::pipes[sockfd][write_end];

            auto size_before_resize = socket.size();
            socket.resize(size_before_resize+len);
            memcpy(socket.data()+size_before_resize, buf, len);
            ret = len;
            if(ret == 0) ret = -1;
        }
        return ret;
    }


    // Mock implementation of select
    int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) {
        int ret = 0;
        // If readfds is provided, modify it so FD_ISSET will return true for any subsequent check
        if (readfds) {
            // Clear all bits first to ensure a clean state
            FD_ZERO(readfds);
            
            // Set all bits up to nfds
            // This ensures that FD_ISSET(id, readfds) will be true for any id < nfds
            for (int i = 0; i < nfds; i++) {
                if(MockSocket::pipes.contains(i))
                {
                    if(MockSocket::pipes[i][0].size() > 0)
                    {
                        FD_SET(i, readfds);
                        ret ++;
                    }
                }
            }
        }

        // Same for writefds
        if (writefds) {
            FD_ZERO(writefds);
            for (int i = 0; i < nfds; i++) {
                FD_SET(i, writefds);
                ret++;
            }
        }

        // Same for exceptfds
        if (exceptfds) {
            FD_ZERO(exceptfds);
            for (int i = 0; i < nfds; i++) {
                FD_SET(i, exceptfds);
                ret ++;
            }
        }

        // Always return 1 to indicate that data is available
        return ret;
    }
}