// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_NETWORKING_HPP_
#define _METADOT_NETWORKING_HPP_

#include "Game/Core.hpp"

#include <functional>

enum NetworkMode {
    ERROR = -1,
    HOST = 0,
    CLIENT = 1,
    SERVER = 2
};

class Networking {
public:
    static bool init();
};

class Client {
public:

    ~Client();

    static Client *start();

    bool connect(const char *ip, UInt16 port);
    void disconnect();

    void tick();
};

class Server {
public:

    ~Server();

    static Server *start(UInt16 port);

    void tick();
};

#endif