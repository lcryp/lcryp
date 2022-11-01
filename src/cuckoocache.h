#ifndef LCRYP_CUCKOOCACHE_H
#define LCRYP_CUCKOOCACHE_H
#include <util/fastrange.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>
namespace CuckooCache
{
class bit_packed_atomic_flags
{
    std::unique_ptr<std::atomic<uint8_t>[]> mem;
public:
    bit_packed_atomic_flags() = delete;
    explicit bit_packed_atomic_flags(uint32_t size)
    {
        size = (size + 7) / 8;
        mem.reset(new std::atomic<uint8_t>[size]);
        for (uint32_t i = 0; i < size; ++i)
            mem[i].store(0xFF);
    };
    inline void setup(uint32_t b)
    {
        bit_packed_atomic_flags d(b);
        std::swap(mem, d.mem);
    }
    inline void bit_set(uint32_t s)
    {
        mem[s >> 3].fetch_or(uint8_t(1 << (s & 7)), std::memory_order_relaxed);
    }
    inline void bit_unset(uint32_t s)
    {
        mem[s >> 3].fetch_and(uint8_t(~(1 << (s & 7))), std::memory_order_relaxed);
    }
    inline bool bit_is_set(uint32_t s) const
    {
        return (1 << (s & 7)) & mem[s >> 3].load(std::memory_order_relaxed);
    }
};
template <typename Element, typename Hash>
class cache
{
private:
    std::vector<Element> table;
    uint32_t size;
    mutable bit_packed_atomic_flags collection_flags;
    mutable std::vector<bool> epoch_flags;
    uint32_t epoch_heuristic_counter;
    uint32_t epoch_size;
    uint8_t depth_limit;
    const Hash hash_function;
    inline std::array<uint32_t, 8> compute_hashes(const Element& e) const
    {
        return {{FastRange32(hash_function.template operator()<0>(e), size),
                 FastRange32(hash_function.template operator()<1>(e), size),
                 FastRange32(hash_function.template operator()<2>(e), size),
                 FastRange32(hash_function.template operator()<3>(e), size),
                 FastRange32(hash_function.template operator()<4>(e), size),
                 FastRange32(hash_function.template operator()<5>(e), size),
                 FastRange32(hash_function.template operator()<6>(e), size),
                 FastRange32(hash_function.template operator()<7>(e), size)}};
    }
    constexpr uint32_t invalid() const
    {
        return ~(uint32_t)0;
    }
    inline void allow_erase(uint32_t n) const
    {
        collection_flags.bit_set(n);
    }
    inline void please_keep(uint32_t n) const
    {
        collection_flags.bit_unset(n);
    }
    void epoch_check()
    {
        if (epoch_heuristic_counter != 0) {
            --epoch_heuristic_counter;
            return;
        }
        uint32_t epoch_unused_count = 0;
        for (uint32_t i = 0; i < size; ++i)
            epoch_unused_count += epoch_flags[i] &&
                                  !collection_flags.bit_is_set(i);
        if (epoch_unused_count >= epoch_size) {
            for (uint32_t i = 0; i < size; ++i)
                if (epoch_flags[i])
                    epoch_flags[i] = false;
                else
                    allow_erase(i);
            epoch_heuristic_counter = epoch_size;
        } else
            epoch_heuristic_counter = std::max(1u, std::max(epoch_size / 16,
                        epoch_size - epoch_unused_count));
    }
public:
    cache() : table(), size(), collection_flags(0), epoch_flags(),
    epoch_heuristic_counter(), epoch_size(), depth_limit(0), hash_function()
    {
    }
    uint32_t setup(uint32_t new_size)
    {
        size = std::max<uint32_t>(2, new_size);
        depth_limit = static_cast<uint8_t>(std::log2(static_cast<float>(size)));
        table.resize(size);
        collection_flags.setup(size);
        epoch_flags.resize(size);
        epoch_size = std::max((uint32_t)1, (45 * size) / 100);
        epoch_heuristic_counter = epoch_size;
        return size;
    }
    std::optional<std::pair<uint32_t, size_t>> setup_bytes(size_t bytes)
    {
        size_t requested_num_elems = bytes / sizeof(Element);
        if (std::numeric_limits<uint32_t>::max() < requested_num_elems) {
            return std::nullopt;
        }
        auto num_elems = setup(bytes/sizeof(Element));
        size_t approx_size_bytes = num_elems * sizeof(Element);
        return std::make_pair(num_elems, approx_size_bytes);
    }
    inline void insert(Element e)
    {
        epoch_check();
        uint32_t last_loc = invalid();
        bool last_epoch = true;
        std::array<uint32_t, 8> locs = compute_hashes(e);
        for (const uint32_t loc : locs)
            if (table[loc] == e) {
                please_keep(loc);
                epoch_flags[loc] = last_epoch;
                return;
            }
        for (uint8_t depth = 0; depth < depth_limit; ++depth) {
            for (const uint32_t loc : locs) {
                if (!collection_flags.bit_is_set(loc))
                    continue;
                table[loc] = std::move(e);
                please_keep(loc);
                epoch_flags[loc] = last_epoch;
                return;
            }
            last_loc = locs[(1 + (std::find(locs.begin(), locs.end(), last_loc) - locs.begin())) & 7];
            std::swap(table[last_loc], e);
            bool epoch = last_epoch;
            last_epoch = epoch_flags[last_loc];
            epoch_flags[last_loc] = epoch;
            locs = compute_hashes(e);
        }
    }
    inline bool contains(const Element& e, const bool erase) const
    {
        std::array<uint32_t, 8> locs = compute_hashes(e);
        for (const uint32_t loc : locs)
            if (table[loc] == e) {
                if (erase)
                    allow_erase(loc);
                return true;
            }
        return false;
    }
};
}
#endif
