#ifndef LCRYP_NODE_CONNECTION_TYPES_H
#define LCRYP_NODE_CONNECTION_TYPES_H
#include <string>
enum class ConnectionType {
    INBOUND,
    OUTBOUND_FULL_RELAY,
    MANUAL,
    FEELER,
    BLOCK_RELAY,
    ADDR_FETCH,
};
std::string ConnectionTypeAsString(ConnectionType conn_type);
#endif
