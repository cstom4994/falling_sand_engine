// Copyright(c) 2022, KaoruXun All rights reserved.


#include "InEngine.h"

#include "Game/Core.hpp"

#define ENET_IMPLEMENTATION
#include <enet.h>

#include "Networking.hpp"


bool Networking::init() {
    if (enet_initialize() != 0) {
        METADOT_ERROR("An error occurred while initializing ENet.");
        return false;
    }
    atexit(enet_deinitialize);
    return true;
}

Server *Server::start(enet_uint16 port) {
    Server *server = new Server();
    server->address = ENetAddress();
    server->address.host = ENET_HOST_ANY;
    //enet_address_set_host_ip(&server->address, "172.23.16.150");
    server->address.port = port;

    server->server = enet_host_create(&server->address,
                                      32,// max client count
                                      2, // number of channels
                                      0, // unlimited incoming bandwidth
                                      0);// unlimited outgoing bandwidth

    if (server->server == NULL) {
        METADOT_ERROR("An error occurred while trying to create an ENet server host.");
        delete server;
        return NULL;
    } else {
        char ch[20];
        enet_address_get_host_ip(&server->server->address, ch, 20);
        METADOT_INFO("[SERVER] Started on {}:{}.", ch, server->server->address.port);
    }


    return server;
}

void Server::tick() {
    ENetEvent event;
    // poll for events
    while (enet_host_service(server, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                METADOT_INFO("[SERVER] A new client connected from {1}.",
                             (int)event.peer->address.port);

                // arbitrary client data
                event.peer->data = (void *) "Client data";
                enet_peer_timeout(event.peer, 16, 3000, 10000);

                break;
            case ENET_EVENT_TYPE_RECEIVE:
                METADOT_BUG(fmt::format("[SERVER] A packet of length {0} containing {1} was received from {2} on channel {3}.",
                                        (int) event.packet->dataLength,
                                        (int) event.packet->data,
                                        (int) event.peer->data,
                                        (int) event.channelID)
                                    .c_str());

                // done using packet
                enet_packet_destroy(event.packet);

                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                METADOT_BUG("[SERVER] {} disconnected.", event.peer->data);

                // clear arbitrary data
                event.peer->data = NULL;
        }
    }
    //METADOT_BUG("[SERVER] done tick");
}

Server::~Server() {
    if (server != NULL) enet_host_destroy(server);
}

Client *Client::start() {

    Client *client = new Client();

    client->client = enet_host_create(NULL,// NULL means to make a client
                                      1,   // number of connections
                                      2,   // number of channels
                                      0,   // unlimited incoming bandwidth
                                      0);  // unlimited outgoing bandwidth

    if (client->client == NULL) {
        METADOT_ERROR("An error occurred while trying to create an ENet client host.");
        delete client;
        return NULL;
    }

    return client;
}

bool Client::connect(const char *ip, enet_uint16 port) {
    ENetEvent event;

    enet_address_set_host(&address, ip);
    //address.host = ENET_HOST_BROADCAST;
    address.port = port;

    peer = enet_host_connect(client, &address, 2, 0);
    if (peer == NULL) {
        METADOT_ERROR("[CLIENT] No available peers for initiating an ENet connection.");

        return false;
    }

    // wait for connection to succeed
    if (enet_host_service(client, &event, 2000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT) {
        char ch[20];
        enet_address_get_host_ip(&peer->address, ch, 20);
        METADOT_INFO("[CLIENT] Connection to {}:{} succeeded.", ch, peer->address.port);

        return true;
    } else {
        /* Either the 5 seconds are up or a disconnect event was */
        /* received. Reset the peer in the event the 5 seconds   */
        /* had run out without any significant event.            */
        enet_peer_reset(peer);
        char ch[20];
        enet_address_get_host_ip(&address, ch, 20);
        METADOT_ERROR("[CLIENT] Connection to {}:{} failed.", ch, address.port);
    }

    return false;
}

void Client::disconnect() {
    if (peer != NULL) enet_peer_disconnect(peer, 0);
}

void Client::tick() {
    ENetEvent event;

    // poll for events
    while (enet_host_service(client, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                METADOT_INFO("[CLIENT] Connected to server at {}.",
                             (int)event.peer->address.port);

                // arbitrary client data
                event.peer->data = (void *) "Client information";

                break;
            case ENET_EVENT_TYPE_RECEIVE:
                METADOT_BUG(fmt::format("[CLIENT] A packet of length {0} containing {1} was received from {2} on channel {3}.",
                                        (int) event.packet->dataLength,
                                        (int) event.packet->data,
                                        (int) event.peer->data,
                                        (int) event.channelID)
                                    .c_str());

                // done with packet
                enet_packet_destroy(event.packet);

                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                METADOT_BUG("[CLIENT] {} disconnected.\n", event.peer->data);

                // clear arbitrary data
                event.peer->data = NULL;
        }
    }
}

Client::~Client() {
    if (client != NULL) enet_host_destroy(client);
}
