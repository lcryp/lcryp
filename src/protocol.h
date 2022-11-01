#ifndef LCRYP_PROTOCOL_H
#define LCRYP_PROTOCOL_H
#include <netaddress.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <streams.h>
#include <uint256.h>
#include <util/time.h>
#include <cstdint>
#include <limits>
#include <string>
class CMessageHeader
{
public:
    static constexpr size_t MESSAGE_START_SIZE = 4;
    static constexpr size_t COMMAND_SIZE = 12;
    static constexpr size_t MESSAGE_SIZE_SIZE = 4;
    static constexpr size_t CHECKSUM_SIZE = 4;
    static constexpr size_t MESSAGE_SIZE_OFFSET = MESSAGE_START_SIZE + COMMAND_SIZE;
    static constexpr size_t CHECKSUM_OFFSET = MESSAGE_SIZE_OFFSET + MESSAGE_SIZE_SIZE;
    static constexpr size_t HEADER_SIZE = MESSAGE_START_SIZE + COMMAND_SIZE + MESSAGE_SIZE_SIZE + CHECKSUM_SIZE;
    typedef unsigned char MessageStartChars[MESSAGE_START_SIZE];
    explicit CMessageHeader() = default;
    CMessageHeader(const MessageStartChars& pchMessageStartIn, const char* pszCommand, unsigned int nMessageSizeIn);
    std::string GetCommand() const;
    bool IsCommandValid() const;
    SERIALIZE_METHODS(CMessageHeader, obj) { READWRITE(obj.pchMessageStart, obj.pchCommand, obj.nMessageSize, obj.pchChecksum); }
    char pchMessageStart[MESSAGE_START_SIZE]{};
    char pchCommand[COMMAND_SIZE]{};
    uint32_t nMessageSize{std::numeric_limits<uint32_t>::max()};
    uint8_t pchChecksum[CHECKSUM_SIZE]{};
};
namespace NetMsgType {
extern const char* VERSION;
extern const char* VERACK;
extern const char* ADDR;
extern const char *ADDRV2;
extern const char *SENDADDRV2;
extern const char* INV;
extern const char* GETDATA;
extern const char* MERKLEBLOCK;
extern const char* GETBLOCKS;
extern const char* GETHEADERS;
extern const char* TX;
extern const char* HEADERS;
extern const char* BLOCK;
extern const char* GETADDR;
extern const char* MEMPOOL;
extern const char* PING;
extern const char* PONG;
extern const char* NOTFOUND;
extern const char* FILTERLOAD;
extern const char* FILTERADD;
extern const char* FILTERCLEAR;
extern const char* SENDHEADERS;
extern const char* FEEFILTER;
extern const char* SENDCMPCT;
extern const char* CMPCTBLOCK;
extern const char* GETBLOCKTXN;
extern const char* BLOCKTXN;
extern const char* GETCFILTERS;
extern const char* CFILTER;
extern const char* GETCFHEADERS;
extern const char* CFHEADERS;
extern const char* GETCFCHECKPT;
extern const char* CFCHECKPT;
extern const char* WTXIDRELAY;
};
const std::vector<std::string>& getAllNetMessageTypes();
enum ServiceFlags : uint64_t {
    NODE_NONE = 0,
    NODE_NETWORK = (1 << 0),
    NODE_BLOOM = (1 << 2),
    NODE_WITNESS = (1 << 3),
    NODE_COMPACT_FILTERS = (1 << 6),
    NODE_NETWORK_LIMITED = (1 << 10),
};
std::vector<std::string> serviceFlagsToStr(uint64_t flags);
ServiceFlags GetDesirableServiceFlags(ServiceFlags services);
void SetServiceFlagsIBDCache(bool status);
static inline bool HasAllDesirableServiceFlags(ServiceFlags services)
{
    return !(GetDesirableServiceFlags(services) & (~services));
}
static inline bool MayHaveUsefulAddressDB(ServiceFlags services)
{
    return (services & NODE_NETWORK) || (services & NODE_NETWORK_LIMITED);
}
class CAddress : public CService
{
    static constexpr std::chrono::seconds TIME_INIT{100000000};
    static constexpr uint32_t DISK_VERSION_INIT{220000};
    static constexpr uint32_t DISK_VERSION_IGNORE_MASK{0b00000000'00000111'11111111'11111111};
    /** The version number written in disk serialized addresses to indicate V2 serializations.
     * It must be exactly 1<<29, as that is the value that historical versions used for this
     * (they used their internal ADDRV2_FORMAT flag here). */
    static constexpr uint32_t DISK_VERSION_ADDRV2{1 << 29};
    static_assert((DISK_VERSION_INIT & ~DISK_VERSION_IGNORE_MASK) == 0, "DISK_VERSION_INIT must be covered by DISK_VERSION_IGNORE_MASK");
    static_assert((DISK_VERSION_ADDRV2 & DISK_VERSION_IGNORE_MASK) == 0, "DISK_VERSION_ADDRV2 must not be covered by DISK_VERSION_IGNORE_MASK");
public:
    CAddress() : CService{} {};
    CAddress(CService ipIn, ServiceFlags nServicesIn) : CService{ipIn}, nServices{nServicesIn} {};
    CAddress(CService ipIn, ServiceFlags nServicesIn, NodeSeconds time) : CService{ipIn}, nTime{time}, nServices{nServicesIn} {};
    SERIALIZE_METHODS(CAddress, obj)
    {
        // CAddress has a distinct network serialization and a disk serialization, but it should never
        // be hashed (except through CHashWriter in addrdb.cpp, which sets SER_DISK), and it's
        assert(!(s.GetType() & SER_GETHASH));
        bool use_v2;
        if (s.GetType() & SER_DISK) {
            uint32_t stored_format_version = DISK_VERSION_INIT;
            if (s.GetVersion() & ADDRV2_FORMAT) stored_format_version |= DISK_VERSION_ADDRV2;
            READWRITE(stored_format_version);
            stored_format_version &= ~DISK_VERSION_IGNORE_MASK;
            if (stored_format_version == 0) {
                use_v2 = false;
            } else if (stored_format_version == DISK_VERSION_ADDRV2 && (s.GetVersion() & ADDRV2_FORMAT)) {
                use_v2 = true;
            } else {
                throw std::ios_base::failure("Unsupported CAddress disk format version");
            }
        } else {
            assert(s.GetType() & SER_NETWORK);
            use_v2 = s.GetVersion() & ADDRV2_FORMAT;
        }
        READWRITE(Using<LossyChronoFormatter<uint32_t>>(obj.nTime));
        if (use_v2) {
            uint64_t services_tmp;
            SER_WRITE(obj, services_tmp = obj.nServices);
            READWRITE(Using<CompactSizeFormatter<false>>(services_tmp));
            SER_READ(obj, obj.nServices = static_cast<ServiceFlags>(services_tmp));
        } else {
            READWRITE(Using<CustomUintFormatter<8>>(obj.nServices));
        }
        OverrideStream<Stream> os(&s, s.GetType(), use_v2 ? ADDRV2_FORMAT : 0);
        SerReadWriteMany(os, ser_action, ReadWriteAsHelper<CService>(obj));
    }
    NodeSeconds nTime{TIME_INIT};
    ServiceFlags nServices{NODE_NONE};
    friend bool operator==(const CAddress& a, const CAddress& b)
    {
        return a.nTime == b.nTime &&
               a.nServices == b.nServices &&
               static_cast<const CService&>(a) == static_cast<const CService&>(b);
    }
};
const uint32_t MSG_WITNESS_FLAG = 1 << 30;
const uint32_t MSG_TYPE_MASK = 0xffffffff >> 2;
enum GetDataMsg : uint32_t {
    UNDEFINED = 0,
    MSG_TX = 1,
    MSG_BLOCK = 2,
    MSG_WTX = 5,
    MSG_FILTERED_BLOCK = 3,
    MSG_CMPCT_BLOCK = 4,
    MSG_WITNESS_BLOCK = MSG_BLOCK | MSG_WITNESS_FLAG,
    MSG_WITNESS_TX = MSG_TX | MSG_WITNESS_FLAG,
};
class CInv
{
public:
    CInv();
    CInv(uint32_t typeIn, const uint256& hashIn);
    SERIALIZE_METHODS(CInv, obj) { READWRITE(obj.type, obj.hash); }
    friend bool operator<(const CInv& a, const CInv& b);
    std::string GetCommand() const;
    std::string ToString() const;
    bool IsMsgTx() const { return type == MSG_TX; }
    bool IsMsgBlk() const { return type == MSG_BLOCK; }
    bool IsMsgWtx() const { return type == MSG_WTX; }
    bool IsMsgFilteredBlk() const { return type == MSG_FILTERED_BLOCK; }
    bool IsMsgCmpctBlk() const { return type == MSG_CMPCT_BLOCK; }
    bool IsMsgWitnessBlk() const { return type == MSG_WITNESS_BLOCK; }
    bool IsGenTxMsg() const
    {
        return type == MSG_TX || type == MSG_WTX || type == MSG_WITNESS_TX;
    }
    bool IsGenBlkMsg() const
    {
        return type == MSG_BLOCK || type == MSG_FILTERED_BLOCK || type == MSG_CMPCT_BLOCK || type == MSG_WITNESS_BLOCK;
    }
    uint32_t type;
    uint256 hash;
};
GenTxid ToGenTxid(const CInv& inv);
#endif
