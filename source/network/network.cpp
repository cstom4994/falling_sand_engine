
#include "network.h"

#define CUTE_NET_IMPLEMENTATION
#include "libs/cute/cute_net.h"

static METADOT_INLINE METAENGINE_Result metadot_wrap(cn_result_t cn_result) {
    METAENGINE_Result result;
    result.code = cn_result.code;
    result.details = cn_result.details;
    return result;
}

METADOT_STATIC_ASSERT(METADOT_CONNECT_TOKEN_SIZE == CN_CONNECT_TOKEN_SIZE, "Must be equal.");
METADOT_STATIC_ASSERT(METADOT_CONNECT_TOKEN_USER_DATA_SIZE == CN_CONNECT_TOKEN_USER_DATA_SIZE, "Must be equal.");

int metadot_endpoint_init(METAENGINE_Endpoint* endpoint, const char* address_and_port_string) { return cn_endpoint_init(endpoint, address_and_port_string); }

void METAENGINE_Endpointo_string(METAENGINE_Endpoint endpoint, char* buffer, int buffer_size) { cn_endpoint_to_string(endpoint, buffer, buffer_size); }

int metadot_endpoint_equals(METAENGINE_Endpoint a, METAENGINE_Endpoint b) { return cn_endpoint_equals(a, b); }

METAENGINE_CryptoKey metadot_crypto_generate_key() { return cn_crypto_generate_key(); }

void metadot_crypto_random_bytes(void* data, int byte_count) { cn_crypto_random_bytes(data, byte_count); }

void metadot_crypto_sign_keygen(METAENGINE_CryptoSignPublic* public_key, METAENGINE_CryptoSignSecret* secret_key) { cn_crypto_sign_keygen(public_key, secret_key); }

METAENGINE_Result metadot_generate_connect_token(uint64_t application_id, uint64_t creation_timestamp, const METAENGINE_CryptoKey* client_to_server_key, const METAENGINE_CryptoKey* server_to_client_key,
                                            uint64_t expiration_timestamp, uint32_t handshake_timeout, int address_count, const char** address_list, uint64_t client_id, const uint8_t* user_data,
                                            const METAENGINE_CryptoSignSecret* shared_secret_key, uint8_t* token_ptr_out) {
    cn_result_t result = cn_generate_connect_token(application_id, creation_timestamp, client_to_server_key, server_to_client_key, expiration_timestamp, handshake_timeout, address_count, address_list,
                                                   client_id, user_data, shared_secret_key, token_ptr_out);
    return metadot_wrap(result);
}

METAENGINE_Client* metadot_make_client(uint16_t port, uint64_t application_id, bool use_ipv6 /* = false */
) {
    return cn_client_create(port, application_id, use_ipv6, NULL);
}

void metadot_destroy_client(METAENGINE_Client* client) { cn_client_destroy(client); }

METAENGINE_Result metadot_client_connect(METAENGINE_Client* client, const uint8_t* connect_token) { return metadot_wrap(cn_client_connect(client, connect_token)); }

void metadot_client_disconnect(METAENGINE_Client* client) { cn_client_disconnect(client); }

void metadot_client_update(METAENGINE_Client* client, double dt, uint64_t current_time) { cn_client_update(client, dt, current_time); }

bool metadot_client_pop_packet(METAENGINE_Client* client, void** packet, int* size, bool* was_sent_reliably /* = NULL */) { return cn_client_pop_packet(client, packet, size, was_sent_reliably); }

void metadot_client_free_packet(METAENGINE_Client* client, void* packet) { cn_client_free_packet(client, packet); }

METAENGINE_Result metadot_client_send(METAENGINE_Client* client, const void* packet, int size, bool send_reliably) { return metadot_wrap(cn_client_send(client, packet, size, send_reliably)); }

METAENGINE_ClientState metadot_client_state_get(const METAENGINE_Client* client) { return (METAENGINE_ClientState)cn_client_state_get(client); }

const char* metadot_client_state_string(METAENGINE_ClientState state) { return cn_client_state_string((cn_client_state_t)state); }

void metadot_client_enable_network_simulator(METAENGINE_Client* client, double latency, double jitter, double drop_chance, double duplicate_chance) {
    cn_client_enable_network_simulator(client, latency, jitter, drop_chance, duplicate_chance);
}

//--------------------------------------------------------------------------------------------------
// SERVER

METADOT_STATIC_ASSERT(METADOT_SERVER_MAX_CLIENTS == CN_SERVER_MAX_CLIENTS, "Must be equal.");

METAENGINE_Server* metadot_make_server(METAENGINE_ServerConfig config) {
    cn_server_config_t cn_config;
    cn_config.application_id = config.application_id;
    cn_config.max_incoming_bytes_per_second = config.max_incoming_bytes_per_second;
    cn_config.max_outgoing_bytes_per_second = config.max_outgoing_bytes_per_second;
    cn_config.connection_timeout = config.connection_timeout;
    cn_config.resend_rate = config.resend_rate;
    cn_config.public_key = config.public_key;
    cn_config.secret_key = config.secret_key;
    cn_config.user_allocator_context = config.user_allocator_context;
    return cn_server_create(cn_config);
}

void metadot_destroy_server(METAENGINE_Server* server) { cn_server_destroy(server); }

METAENGINE_Result metadot_server_start(METAENGINE_Server* server, const char* address_and_port) { return metadot_wrap(cn_server_start(server, address_and_port)); }

void metadot_server_stop(METAENGINE_Server* server) { return cn_server_stop(server); }

METADOT_STATIC_ASSERT(sizeof(METAENGINE_ServerEvent) == sizeof(cn_server_event_t), "Must be equal.");

bool metadot_server_pop_event(METAENGINE_Server* server, METAENGINE_ServerEvent* event) { return cn_server_pop_event(server, (cn_server_event_t*)event); }

void metadot_server_free_packet(METAENGINE_Server* server, int client_index, void* data) { cn_server_free_packet(server, client_index, data); }

void metadot_server_update(METAENGINE_Server* server, double dt, uint64_t current_time) { cn_server_update(server, dt, current_time); }

void metadot_server_disconnect_client(METAENGINE_Server* server, int client_index, bool notify_client /* = true */) { cn_server_disconnect_client(server, client_index, notify_client); }

void metadot_server_send(METAENGINE_Server* server, const void* packet, int size, int client_index, bool send_reliably) { cn_server_send(server, packet, size, client_index, send_reliably); }
bool metadot_server_is_client_connected(METAENGINE_Server* server, int client_index) { return cn_server_is_client_connected(server, client_index); }

void metadot_server_enable_network_simulator(METAENGINE_Server* server, double latency, double jitter, double drop_chance, double duplicate_chance) {
    cn_server_enable_network_simulator(server, latency, jitter, drop_chance, duplicate_chance);
}