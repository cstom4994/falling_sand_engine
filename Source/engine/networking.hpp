// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_NETWORKING_HPP_
#define _METADOT_NETWORKING_HPP_

#include "core/core.hpp"

#include <functional>

class Networking {
public:
    static bool init();
};

class Client {
public:
    ~Client();

    static Client *start();

    bool connect(const char *ip, U16 port);
    void disconnect();

    void tick();
};

class Server {
public:
    ~Server();

    static Server *start(U16 port);

    void tick();
};

#endif