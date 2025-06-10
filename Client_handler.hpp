#pragma once

#include <vector>       // vector
#include <thread>       // thread
#include <mutex>        // mutex
#include <sys/socket.h> // recv, send
#include <cerrno>       // errno
#include <cstring>      // strerror
#include <cstdio>       // fprintf

#define MSG_HELLO 1

struct header_t
{
    unsigned short len;
    unsigned short cmd;
};

using header_p = header_t *;

class Client_handler
{
    int mp_socket;
    std::thread mp_loop_tcp;
    std::vector<std::string> mp_pending_tx_messages, mp_rx_messages;
    std::mutex mp_pmsg_queue_mutex, mp_rmsg_queue_mutex;

public:
    Client_handler(int socket)
        : mp_socket{socket}
    {
        mp_loop_tcp = std::thread(&Client_handler::loop_TCP, this);
    }

    ~Client_handler()
    {
        close(mp_socket);
        mp_loop_tcp.join();
    }
    
    static char * get_class_name() { return "Client_handler"; }

private:
    void loop_TCP()
    {
        while(is_connected())
        {
            receive_messages();
            handle_messages();
            send_pending_messages();
            std::this_thread::sleep_for(std::chrono::microseconds(150));
        }
    }

    bool is_connected()
    {
        char test_buffer;
        return recv(mp_socket, &test_buffer, 1, MSG_PEEK) >= 0;
    }

    void receive_messages()
    {
        int ret;
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;

        fd_set rw_fds;
        FD_ZERO(&rw_fds);
        FD_SET(mp_socket, &rw_fds);
        ret = select(mp_socket + 1, &rw_fds, nullptr, nullptr, &timeout);

        if (ret > 0 && FD_ISSET(mp_socket, &rw_fds))
        {
            std::vector<char> buffer(1500, 0);
            ret = recv(mp_socket, buffer.data(), 1500, 0);

            if (ret < 0)
            {
                fprintf(stderr, "%s::%s() : Error occured when receiving message : %s\n", get_class_name(), __FUNCTION__, strerror(errno));
            }
            else if (ret == 0)
            {
                fprintf(stderr, "%s::%s(): connection closed by client\n", get_class_name(), __FUNCTION__);
                ret = -1;
            }
            else
            {
                auto acc = 0;
                while (acc < ret)
                {
                    header_p package_header_p = std::bit_cast<header_p>(buffer.data());
                    auto package_length = package_header_p->len;

                    std::unique_lock<std::mutex> lock(mp_rmsg_queue_mutex);
                    mp_rx_messages.push_back(std::string(buffer.data() + acc, package_length));
                    acc += package_length;
                }

                fprintf(stderr, "%s::%s() : received %d bytes from socket\n", get_class_name(), __FUNCTION__, ret);
            }
        }
    }

    void handle_messages()
    {
        while(not mp_rx_messages.empty())
        {
            std::string bytes;
            std::unique_lock<std::mutex> lock(mp_rmsg_queue_mutex);
            bytes = mp_rx_messages.front();
            lock.unlock();

            header_p header = std::bit_cast<header_p>(bytes.c_str());
            switch (header->cmd)
            {
            case MSG_HELLO:
                handle_hello();
                break;
            
            default:
                break;
            }
            lock.lock();
            mp_rx_messages.erase(mp_rx_messages.begin());
        }
    }

    void handle_hello()
    {
        header_t to_send;
        to_send.cmd = MSG_HELLO;
        to_send.len = sizeof(to_send);

        fprintf(stdout, "%s::%s(): received hello from client\n", get_class_name(), __FUNCTION__);

        std::unique_lock<std::mutex> lock(mp_pmsg_queue_mutex);
        mp_pending_tx_messages.push_back(std::string(std::bit_cast<char*>(&to_send), to_send.len));
    }

    void send_pending_messages()
    {
        int ret{};
        while (!mp_pending_tx_messages.empty())
        {
            std::unique_lock<std::mutex> lock(mp_pmsg_queue_mutex);
            std::string msg{mp_pending_tx_messages[0]};

            lock.unlock();
            ret = 0;
            while (msg.size() != 0)
            {
                ret = send_packet(msg.c_str(), msg.size());
                if(ret < 0)
                {
                    fprintf(stderr, "%s::%s() : Error occured when sending message : %s\n", get_class_name(), __FUNCTION__, strerror(errno));
                }
                try
                {
                    msg = msg.substr(ret, msg.size() - ret);
                }
                catch (const std::out_of_range &e)
                {
                    break;
                }
            }

            lock.lock();
            mp_pending_tx_messages.erase(mp_pending_tx_messages.begin());
        }
    }

    int send_packet(const char *packet, size_t len)
    {
        int ret;
        struct timeval timeout;
        timeout.tv_sec = 15;
        timeout.tv_usec = 0;

        fd_set rw_fds;
        FD_ZERO(&rw_fds);
        FD_SET(mp_socket, &rw_fds);
        ret = select(mp_socket + 1, nullptr, &rw_fds, nullptr, &timeout);

        if (ret <= 0)
        {
            fprintf(stderr, "%s::%s() : Error occured when sending message : %s\n", get_class_name(), __FUNCTION__, strerror(errno));
        }
        else if (ret > 0 and len > 0)
        {
            ret = send(mp_socket, packet, len, 0);
            if(ret < 0)
            {
                fprintf(stderr, "%s::%s() : send failed (err %d : %s)\n", get_class_name(), __FUNCTION__, errno, strerror(errno));
            } else
            {
                fprintf(stdout, "%s::%s() : sent %d bytes\n", get_class_name(), __FUNCTION__, ret);
            }
        }
        return ret;
    }
};