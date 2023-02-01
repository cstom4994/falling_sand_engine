

#ifndef METADOT_NETWORKING_H
#define METADOT_NETWORKING_H

#include "core/stl.h"
#include "libs/cute/cute_net.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

typedef struct cn_client_t METAENGINE_Client;
typedef struct cn_server_t METAENGINE_Server;
typedef struct cn_crypto_key_t METAENGINE_CryptoKey;
typedef struct cn_crypto_sign_public_t METAENGINE_CryptoSignPublic;
typedef struct cn_crypto_sign_secret_t METAENGINE_CryptoSignSecret;
typedef struct cn_crypto_signature_t METAENGINE_CryptoSignature;

//--------------------------------------------------------------------------------------------------
// ENDPOINT

typedef struct cn_endpoint_t METAENGINE_Endpoint;
typedef enum cn_address_type_t METAENGINE_AddressType;

#define METAENGINE_ADDRESS_TYPE_DEFS      \
    METAENGINE_ENUM(ADDRESS_TYPE_NONE, 0) \
    METAENGINE_ENUM(ADDRESS_TYPE_IPV4, 1) \
    METAENGINE_ENUM(ADDRESS_TYPE_IPV6, 2)

enum {
#define METAENGINE_ENUM(K, V) METAENGINE_##K = V,
    METAENGINE_ADDRESS_TYPE_DEFS
#undef METAENGINE_ENUM
};

int metadot_endpoint_init(METAENGINE_Endpoint* endpoint, const char* address_and_port_string);
void METAENGINE_Endpointo_string(METAENGINE_Endpoint endpoint, char* buffer, int buffer_size);
int metadot_endpoint_equals(METAENGINE_Endpoint a, METAENGINE_Endpoint b);

//--------------------------------------------------------------------------------------------------
// CONNECT TOKEN

#define METADOT_CONNECT_TOKEN_SIZE 1114
#define METADOT_CONNECT_TOKEN_USER_DATA_SIZE 256

/**
 * Generates a cryptography key in a cryptographically secure way.
 */
METAENGINE_CryptoKey metadot_crypto_generate_key();

/**
 * Fills a buffer in a cryptographically secure way (i.e. a slow way).
 */
void metadot_crypto_random_bytes(void* data, int byte_count);

/**
 * Generates a cryptographically secure keypair, used for facilitating connect tokens.
 */
void metadot_crypto_sign_keygen(METAENGINE_CryptoSignPublic* public_key, METAENGINE_CryptoSignSecret* secret_key);

/**
 * Generates a connect token, useable by clients to authenticate and securely connect to
 * a server. You can use this function whenever a validated client wants to join your game servers.
 *
 * It's recommended to setup a web service specifically for allowing players to authenticate
 * themselves (login). Once authenticated, the webservice can call this function and hand
 * the connect token to the client. The client can then read the public section of the
 * connect token and see the `address_list` of servers to try and connect to. The client then
 * sends the connect token to one of these servers to start the connection handshake. If the
 * handshake completes successfully, the client will connect to the server.
 *
 * The connect token is protected by an AEAD primitive (https://en.wikipedia.org/wiki/Authenticated_encryption),
 * which means the token cannot be modified or forged as long as the `shared_secret_key` is
 * not leaked. In the event your secret key is accidentally leaked, you can always roll a
 * new one and distribute it to your webservice and game servers.
 */
METAENGINE_Result metadot_generate_connect_token(uint64_t application_id,                           // A unique number to identify your game, can be whatever value you like.
                                                                                                    // This must be the same number as in `make_client` and `make_server`.
                                                 uint64_t creation_timestamp,                       // A unix timestamp of the current time.
                                                 const METAENGINE_CryptoKey* client_to_server_key,  // A unique key for this connect token for the client to encrypt packets, and server to
                                                                                                    // decrypt packets. This can be generated with `crypto_generate_key` on your web service.
                                                 const METAENGINE_CryptoKey* server_to_client_key,  // A unique key for this connect token for the server to encrypt packets, and the client to
                                                                                                    // decrypt packets. This can be generated with `crypto_generate_key` on your web service.
                                                 uint64_t expiration_timestamp,                     // A unix timestamp for when this connect token expires and becomes invalid.
                                                 uint32_t handshake_timeout,                        // The number of seconds the connection will stay alive during the handshake process before
                                                                                                    // the client and server reject the handshake process as failed.
                                                 int address_count,                                 // Must be from 1 to 32 (inclusive). The number of addresses in `address_list`.
                                                 const char** address_list,                         // A list of game servers the client can try connecting to, of length `address_count`.
                                                 uint64_t client_id,                                // The unique client identifier.
                                                 const uint8_t* user_data,  // Optional buffer of data of `METADOT_PROTOCOL_CONNECT_TOKEN_USER_DATA_SIZE` (256) bytes. Can be NULL.
                                                 const METAENGINE_CryptoSignSecret* shared_secret_key,  // Only your webservice and game servers know this key.
                                                 uint8_t* token_ptr_out                                 // Pointer to your buffer, should be `METADOT_CONNECT_TOKEN_SIZE` bytes large.
);

//--------------------------------------------------------------------------------------------------
// CLIENT

METAENGINE_Client* metadot_make_client(uint16_t port,             // Port for opening a UDP socket.
                                       uint64_t application_id,   // A unique number to identify your game, can be whatever value you like.
                                                                  // This must be the same number as in `server_create`.
                                       bool use_ipv6 /*= false*/  // Whether or not the socket should turn on ipv6. Some users will not have
                                                                  // ipv6 enabled, so this defaults to false.
);
void metadot_destroy_client(METAENGINE_Client* client);

/**
 * The client will make an attempt to connect to all servers listed in the connect token, one after
 * another. If no server can be connected to the client's state will be set to an error state. Call
 * `client_state_get` to get the client's state. Once `client_connect` is called then successive calls to
 * `client_update` is expected, where `client_update` will perform the connection handshake and make
 * connection attempts to your servers.
 */
METAENGINE_Result metadot_client_connect(METAENGINE_Client* client, const uint8_t* connect_token);
void metadot_client_disconnect(METAENGINE_Client* client);

/**
 * You should call this one per game loop after calling `client_connect`.
 */
void metadot_client_update(METAENGINE_Client* client, double dt, uint64_t current_time);

/**
 * Returns a packet from the server if one is available. You must free this packet when you're done by
 * calling `client_free_packet`.
 */
bool metadot_client_pop_packet(METAENGINE_Client* client, void** packet, int* size, bool* was_sent_reliably /*= NULL*/);
void metadot_client_free_packet(METAENGINE_Client* client, void* packet);

/**
 * Sends a packet to the server. If the packet size is too large (over 1k bytes) it will be split up
 * and sent in smaller chunks.
 *
 * `send_reliably` as true means the packet will be sent reliably an in-order relative to other
 * reliable packets. Under packet loss the packet will continually be sent until an acknowledgement
 * from the server is received. False means to send a typical UDP packet, with no special mechanisms
 * regarding packet loss.
 *
 * Reliable packets are significantly more expensive than unreliable packets, so try to send any data
 * that can be lost due to packet loss as an unreliable packet. Of course, some packets are required
 * to be sent, and so reliable is appropriate. As an optimization some kinds of data, such as frequent
 * transform updates, can be sent unreliably.
 */
METAENGINE_Result metadot_client_send(METAENGINE_Client* client, const void* packet, int size, bool send_reliably);

#define METAENGINE_CLIENT_STATE_DEFS                               \
    METAENGINE_ENUM(CLIENT_STATE_CONNECT_TOKEN_EXPIRED, -6)        \
    METAENGINE_ENUM(CLIENT_STATE_INVALID_CONNECT_TOKEN, -5)        \
    METAENGINE_ENUM(CLIENT_STATE_CONNECTION_TIMED_OUT, -4)         \
    METAENGINE_ENUM(CLIENT_STATE_CHALLENGE_RESPONSE_TIMED_OUT, -3) \
    METAENGINE_ENUM(CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT, -2) \
    METAENGINE_ENUM(CLIENT_STATE_CONNECTION_DENIED, -1)            \
    METAENGINE_ENUM(CLIENT_STATE_DISCONNECTED, 0)                  \
    METAENGINE_ENUM(CLIENT_STATE_SENDING_CONNECTION_REQUEST, 1)    \
    METAENGINE_ENUM(CLIENT_STATE_SENDING_CHALLENGE_RESPONSE, 2)    \
    METAENGINE_ENUM(CLIENT_STATE_CONNECTED, 3)

typedef enum METAENGINE_ClientState {
#define METAENGINE_ENUM(K, V) METAENGINE_##K = V,
    METAENGINE_CLIENT_STATE_DEFS
#undef METAENGINE_ENUM
} METAENGINE_ClientState;

METADOT_INLINE const char* metadot_client_state_to_string(METAENGINE_ClientState state) {
    switch (state) {
#define METAENGINE_ENUM(K, V) \
    case METAENGINE_##K:      \
        return METAENGINE_STRINGIZE(METAENGINE_##K);
        METAENGINE_CLIENT_STATE_DEFS
#undef METAENGINE_ENUM
        default:
            return NULL;
    }
}

METAENGINE_ClientState metadot_client_state_get(const METAENGINE_Client* client);
const char* metadot_client_state_string(METAENGINE_ClientState state);
void metadot_client_enable_network_simulator(METAENGINE_Client* client, double latency, double jitter, double drop_chance, double duplicate_chance);

//--------------------------------------------------------------------------------------------------
// SERVER

// Modify this value as seen fit.
#define METADOT_SERVER_MAX_CLIENTS 32

typedef struct METAENGINE_ServerConfig {
    uint64_t application_id;  // A unique number to identify your game, can be whatever value you like.
                              // This must be the same number as in `client_make`.
    int max_incoming_bytes_per_second;
    int max_outgoing_bytes_per_second;
    int connection_timeout;                  // The number of seconds before consider a connection as timed out when not
                                             // receiving any packets on the connection.
    double resend_rate;                      // The number of seconds to wait before resending a packet that has not been
                                             // acknowledge as received by a client.
    METAENGINE_CryptoSignPublic public_key;  // The public part of your public key cryptography used for connect tokens.
                                             // This can be safely shared with your players publicly.
    METAENGINE_CryptoSignSecret secret_key;  // The secret part of your public key cryptography used for connect tokens.
                                             // This must never be shared publicly and remain a complete secret only know to your servers.
    void* user_allocator_context;
} METAENGINE_ServerConfig;

METADOT_INLINE METAENGINE_ServerConfig metadot_server_config_defaults() {
    METAENGINE_ServerConfig config;
    config.application_id = 0;
    config.max_incoming_bytes_per_second = 0;
    config.max_outgoing_bytes_per_second = 0;
    config.connection_timeout = 10;
    config.resend_rate = 0.1f;
    return config;
}

METAENGINE_Server* metadot_make_server(METAENGINE_ServerConfig config);
void metadot_destroy_server(METAENGINE_Server* server);

/**
 * Starts up the server, ready to receive new client connections.
 *
 * Please note that not all users will be able to access an ipv6 server address, so it might
 * be good to also provide a way to connect through ipv4.
 */
METAENGINE_Result metadot_server_start(METAENGINE_Server* server, const char* address_and_port);
void metadot_server_stop(METAENGINE_Server* server);

#define METAENGINE_SERVER_EVENT_TYPE_DEFS                                                            \
    METAENGINE_ENUM(METAENGINE_SERVER_EVENT_TYPE_NEW_CONNECTION, 0) /* A new incoming connection. */ \
    METAENGINE_ENUM(METAENGINE_SERVER_EVENT_TYPE_DISCONNECTED, 1)   /* A disconnecting client. */    \
    METAENGINE_ENUM(METAENGINE_SERVER_EVENT_TYPE_PAYLOAD_PACKET, 2) /* An incoming packet from a client. */

typedef enum METAENGINE_ServerEventType {
#define METAENGINE_ENUM(K, V) METAENGINE_##K = V,
    METAENGINE_SERVER_EVENT_TYPE_DEFS
#undef METAENGINE_ENUM
} METAENGINE_ServerEventType;

METADOT_INLINE const char* metadot_server_event_type_to_string(METAENGINE_ServerEventType type) {
    switch (type) {
#define METAENGINE_ENUM(K, V) \
    case METAENGINE_##K:      \
        return METAENGINE_STRINGIZE(METAENGINE_##K);
        METAENGINE_SERVER_EVENT_TYPE_DEFS
#undef METAENGINE_ENUM
        default:
            return NULL;
    }
}

typedef struct METAENGINE_ServerEvent {
    METAENGINE_ServerEventType type;
    union {
        struct {
            int client_index;              // An index representing this particular client.
            uint64_t client_id;            // A unique identifier for this particular client, as read from the connect token.
            METAENGINE_Endpoint endpoint;  // The address and port of the incoming connection.
        } new_connection;

        struct {
            int client_index;  // An index representing this particular client.
        } disconnected;

        struct {
            int client_index;  // An index representing this particular client.
            void* data;        // Pointer to the packet's payload data. Send this back to `server_free_packet` when done.
            int size;          // Size of the packet at the data pointer.
        } payload_packet;
    } u;
} METAENGINE_ServerEvent;

/**
 * Server events notify of when a client connects/disconnects, or has sent a payload packet.
 * You must free the payload packets with `server_free_packet` when done.
 */
bool metadot_server_pop_event(METAENGINE_Server* server, METAENGINE_ServerEvent* event);
void metadot_server_free_packet(METAENGINE_Server* server, int client_index, void* data);

void metadot_server_update(METAENGINE_Server* server, double dt, uint64_t current_time);
void metadot_server_disconnect_client(METAENGINE_Server* server, int client_index, bool notify_client /* = true */);
void metadot_server_send(METAENGINE_Server* server, const void* packet, int size, int client_index, bool send_reliably);

bool metadot_server_is_client_connected(METAENGINE_Server* server, int client_index);
void metadot_server_enable_network_simulator(METAENGINE_Server* server, double latency, double jitter, double drop_chance, double duplicate_chance);

#ifdef __cplusplus
}
#endif  // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

namespace MetaEngine {

using Client = METAENGINE_Client;
using Server = METAENGINE_Server;
using CryptoKey = METAENGINE_CryptoKey;
using CryptoSignPublic = METAENGINE_CryptoSignPublic;
using CryptoSignSecret = METAENGINE_CryptoSignSecret;
using CryptoSignSignature = METAENGINE_CryptoSignature;

//--------------------------------------------------------------------------------------------------
// ENDPOINT

using endpoint_t = METAENGINE_Endpoint;
using address_type_t = METAENGINE_AddressType;

enum : int {
#define METAENGINE_ENUM(K, V) K = V,
    METAENGINE_ADDRESS_TYPE_DEFS
#undef METAENGINE_ENUM
};

METADOT_INLINE int endpoint_init(endpoint_t* endpoint, const char* address_and_port_string) { return metadot_endpoint_init(endpoint, address_and_port_string); }
METADOT_INLINE void endpoint_to_string(endpoint_t endpoint, char* buffer, int buffer_size) { METAENGINE_Endpointo_string(endpoint, buffer, buffer_size); }
METADOT_INLINE int endpoint_equals(endpoint_t a, endpoint_t b) { return metadot_endpoint_equals(a, b); }

//--------------------------------------------------------------------------------------------------
// CONNECT TOKEN

METADOT_INLINE CryptoKey crypto_generate_key() { return metadot_crypto_generate_key(); }
METADOT_INLINE void crypto_random_bytes(void* data, int byte_count) { metadot_crypto_random_bytes(data, byte_count); }
METADOT_INLINE void crypto_sign_keygen(CryptoSignPublic* public_key, CryptoSignSecret* secret_key) { metadot_crypto_sign_keygen(public_key, secret_key); }
METADOT_INLINE Result generate_connect_token(uint64_t application_id, uint64_t creation_timestamp, const CryptoKey* client_to_server_key, const CryptoKey* server_to_client_key,
                                             uint64_t expiration_timestamp, uint32_t handshake_timeout, int address_count, const char** address_list, uint64_t client_id, const uint8_t* user_data,
                                             const CryptoSignSecret* shared_secret_key, uint8_t* token_ptr_out) {
    return metadot_generate_connect_token(application_id, creation_timestamp, client_to_server_key, server_to_client_key, expiration_timestamp, handshake_timeout, address_count, address_list,
                                          client_id, user_data, shared_secret_key, token_ptr_out);
}

//--------------------------------------------------------------------------------------------------
// CLIENT

using ClientState = METAENGINE_ClientState;
#define METAENGINE_ENUM(K, V) METADOT_INLINE constexpr ClientState K = METAENGINE_##K;
METAENGINE_CLIENT_STATE_DEFS
#undef METAENGINE_ENUM

METADOT_INLINE const char* to_string(ClientState state) {
    switch (state) {
#define METAENGINE_ENUM(K, V) \
    case METAENGINE_##K:      \
        return #K;
        METAENGINE_CLIENT_STATE_DEFS
#undef METAENGINE_ENUM
        default:
            return NULL;
    }
}

METADOT_INLINE Client* make_client(uint16_t port, uint64_t application_id, bool use_ipv6 = false) { return metadot_make_client(port, application_id, use_ipv6); }
METADOT_INLINE void destroy_client(Client* client) { metadot_destroy_client(client); }
METADOT_INLINE Result client_connect(Client* client, const uint8_t* connect_token) { return metadot_client_connect(client, connect_token); }
METADOT_INLINE void client_disconnect(Client* client) { metadot_client_disconnect(client); }
METADOT_INLINE void client_update(Client* client, double dt, uint64_t current_time) { metadot_client_update(client, dt, current_time); }
METADOT_INLINE bool client_pop_packet(Client* client, void** packet, int* size, bool* was_sent_reliably = NULL) { return metadot_client_pop_packet(client, packet, size, was_sent_reliably); }
METADOT_INLINE void client_free_packet(Client* client, void* packet) { metadot_client_free_packet(client, packet); }
METADOT_INLINE Result client_send(Client* client, const void* packet, int size, bool send_reliably) { return metadot_client_send(client, packet, size, send_reliably); }
METADOT_INLINE ClientState client_state_get(const Client* client) { return metadot_client_state_get(client); }
METADOT_INLINE const char* client_state_string(ClientState state) { return metadot_client_state_string(state); }
METADOT_INLINE void client_enable_network_simulator(Client* client, double latency, double jitter, double drop_chance, double duplicate_chance) {
    metadot_client_enable_network_simulator(client, latency, jitter, drop_chance, duplicate_chance);
}

//--------------------------------------------------------------------------------------------------
// SERVER

using ServerConfig = METAENGINE_ServerConfig;
using ServerEvent = METAENGINE_ServerEvent;

using ServerEventType = METAENGINE_ServerEventType;
#define METAENGINE_ENUM(K, V) METADOT_INLINE constexpr ServerEventType K = METAENGINE_##K;
METAENGINE_SERVER_EVENT_TYPE_DEFS
#undef METAENGINE_ENUM

METADOT_INLINE const char* to_string(ServerEventType type) {
    switch (type) {
#define METAENGINE_ENUM(K, V) \
    case METAENGINE_##K:      \
        return #K;
        METAENGINE_SERVER_EVENT_TYPE_DEFS
#undef METAENGINE_ENUM
        default:
            return NULL;
    }
}

METADOT_INLINE ServerConfig server_config_defaults() { return metadot_server_config_defaults(); }
METADOT_INLINE Server* make_server(ServerConfig config) { return metadot_make_server(config); }
METADOT_INLINE void destroy_server(Server* server) { metadot_destroy_server(server); }
METADOT_INLINE Result server_start(Server* server, const char* address_and_port) { return metadot_server_start(server, address_and_port); }
METADOT_INLINE void server_stop(Server* server) { metadot_server_stop(server); }
METADOT_INLINE bool server_pop_event(Server* server, ServerEvent* event) { return metadot_server_pop_event(server, event); }
METADOT_INLINE void server_free_packet(Server* server, int client_index, void* data) { metadot_server_free_packet(server, client_index, data); }
METADOT_INLINE void server_update(Server* server, double dt, uint64_t current_time) { metadot_server_update(server, dt, current_time); }
METADOT_INLINE void server_disconnect_client(Server* server, int client_index, bool notify_client = true) { metadot_server_disconnect_client(server, client_index, notify_client); }
METADOT_INLINE void server_send(Server* server, const void* packet, int size, int client_index, bool send_reliably) { metadot_server_send(server, packet, size, client_index, send_reliably); }
METADOT_INLINE bool server_is_client_connected(Server* server, int client_index) { return metadot_server_is_client_connected(server, client_index); }
METADOT_INLINE void server_enable_network_simulator(Server* server, double latency, double jitter, double drop_chance, double duplicate_chance) {
    metadot_server_enable_network_simulator(server, latency, jitter, drop_chance, duplicate_chance);
}

}  // namespace MetaEngine

#endif  // METADOT_NETWORKING_H
