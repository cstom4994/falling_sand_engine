// Copyright(c) 2022, KaoruXun All rights reserved.

#pragma once
#include <enet/enet.h>
#include <functional>

enum NetworkMode {
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
    ENetHost *client = nullptr;

    ENetAddress address{};
    ENetPeer *peer = nullptr;

    ~Client();

    static Client *start();

    bool connect(const char *ip, enet_uint16 port);
    void disconnect();

    void tick();
};

class Server {
public:
    ENetAddress address{};
    ENetHost *server = nullptr;

    ~Server();

    static Server *start(enet_uint16 port);

    void tick();
};
