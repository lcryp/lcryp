#include <node/connection_types.h>
#include <cassert>
std::string ConnectionTypeAsString(ConnectionType conn_type)
{
    switch (conn_type) {
    case ConnectionType::INBOUND:
        return "inbound";
    case ConnectionType::MANUAL:
        return "manual";
    case ConnectionType::FEELER:
        return "feeler";
    case ConnectionType::OUTBOUND_FULL_RELAY:
        return "outbound-full-relay";
    case ConnectionType::BLOCK_RELAY:
        return "block-relay-only";
    case ConnectionType::ADDR_FETCH:
        return "addr-fetch";
    }
    assert(false);
}
