#ifndef LCRYP_BLOCKFILTER_H
#define LCRYP_BLOCKFILTER_H
#include <stdint.h>
#include <string>
#include <set>
#include <unordered_set>
#include <vector>
#include <attributes.h>
#include <primitives/block.h>
#include <serialize.h>
#include <uint256.h>
#include <undo.h>
#include <util/bytevectorhash.h>
class GCSFilter
{
public:
    typedef std::vector<unsigned char> Element;
    typedef std::unordered_set<Element, ByteVectorHash> ElementSet;
    struct Params
    {
        uint64_t m_siphash_k0;
        uint64_t m_siphash_k1;
        uint8_t m_P;
        uint32_t m_M;
        Params(uint64_t siphash_k0 = 0, uint64_t siphash_k1 = 0, uint8_t P = 0, uint32_t M = 1)
            : m_siphash_k0(siphash_k0), m_siphash_k1(siphash_k1), m_P(P), m_M(M)
        {}
    };
private:
    Params m_params;
    uint32_t m_N;
    uint64_t m_F;
    std::vector<unsigned char> m_encoded;
    uint64_t HashToRange(const Element& element) const;
    std::vector<uint64_t> BuildHashedSet(const ElementSet& elements) const;
    bool MatchInternal(const uint64_t* sorted_element_hashes, size_t size) const;
public:
    explicit GCSFilter(const Params& params = Params());
    GCSFilter(const Params& params, std::vector<unsigned char> encoded_filter, bool skip_decode_check);
    GCSFilter(const Params& params, const ElementSet& elements);
    uint32_t GetN() const { return m_N; }
    const Params& GetParams() const LIFETIMEBOUND { return m_params; }
    const std::vector<unsigned char>& GetEncoded() const LIFETIMEBOUND { return m_encoded; }
    bool Match(const Element& element) const;
    bool MatchAny(const ElementSet& elements) const;
};
constexpr uint8_t BASIC_FILTER_P = 19;
constexpr uint32_t BASIC_FILTER_M = 784931;
enum class BlockFilterType : uint8_t
{
    BASIC = 0,
    INVALID = 255,
};
const std::string& BlockFilterTypeName(BlockFilterType filter_type);
bool BlockFilterTypeByName(const std::string& name, BlockFilterType& filter_type);
const std::set<BlockFilterType>& AllBlockFilterTypes();
const std::string& ListBlockFilterTypes();
class BlockFilter
{
private:
    BlockFilterType m_filter_type = BlockFilterType::INVALID;
    uint256 m_block_hash;
    GCSFilter m_filter;
    bool BuildParams(GCSFilter::Params& params) const;
public:
    BlockFilter() = default;
    BlockFilter(BlockFilterType filter_type, const uint256& block_hash,
                std::vector<unsigned char> filter, bool skip_decode_check);
    BlockFilter(BlockFilterType filter_type, const CBlock& block, const CBlockUndo& block_undo);
    BlockFilterType GetFilterType() const { return m_filter_type; }
    const uint256& GetBlockHash() const LIFETIMEBOUND { return m_block_hash; }
    const GCSFilter& GetFilter() const LIFETIMEBOUND { return m_filter; }
    const std::vector<unsigned char>& GetEncodedFilter() const LIFETIMEBOUND
    {
        return m_filter.GetEncoded();
    }
    uint256 GetHash() const;
    uint256 ComputeHeader(const uint256& prev_header) const;
    template <typename Stream>
    void Serialize(Stream& s) const {
        s << static_cast<uint8_t>(m_filter_type)
          << m_block_hash
          << m_filter.GetEncoded();
    }
    template <typename Stream>
    void Unserialize(Stream& s) {
        std::vector<unsigned char> encoded_filter;
        uint8_t filter_type;
        s >> filter_type
          >> m_block_hash
          >> encoded_filter;
        m_filter_type = static_cast<BlockFilterType>(filter_type);
        GCSFilter::Params params;
        if (!BuildParams(params)) {
            throw std::ios_base::failure("unknown filter_type");
        }
        m_filter = GCSFilter(params, std::move(encoded_filter),  false);
    }
};
#endif
