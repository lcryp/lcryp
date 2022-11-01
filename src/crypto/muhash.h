#ifndef LCRYP_CRYPTO_MUHASH_H
#define LCRYP_CRYPTO_MUHASH_H
#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <serialize.h>
#include <uint256.h>
#include <stdint.h>
class Num3072
{
private:
    void FullReduce();
    bool IsOverflow() const;
    Num3072 GetInverse() const;
public:
    static constexpr size_t BYTE_SIZE = 384;
#ifdef __SIZEOF_INT128__
    typedef unsigned __int128 double_limb_t;
    typedef uint64_t limb_t;
    static constexpr int LIMBS = 48;
    static constexpr int LIMB_SIZE = 64;
#else
    typedef uint64_t double_limb_t;
    typedef uint32_t limb_t;
    static constexpr int LIMBS = 96;
    static constexpr int LIMB_SIZE = 32;
#endif
    limb_t limbs[LIMBS];
    static_assert(LIMB_SIZE * LIMBS == 3072, "Num3072 isn't 3072 bits");
    static_assert(sizeof(double_limb_t) == sizeof(limb_t) * 2, "bad size for double_limb_t");
    static_assert(sizeof(limb_t) * 8 == LIMB_SIZE, "LIMB_SIZE is incorrect");
    static_assert(sizeof(limb_t) == 4 || sizeof(limb_t) == 8, "bad size for limb_t");
    void Multiply(const Num3072& a);
    void Divide(const Num3072& a);
    void SetToOne();
    void Square();
    void ToBytes(unsigned char (&out)[BYTE_SIZE]);
    Num3072() { this->SetToOne(); };
    Num3072(const unsigned char (&data)[BYTE_SIZE]);
    SERIALIZE_METHODS(Num3072, obj)
    {
        for (auto& limb : obj.limbs) {
            READWRITE(limb);
        }
    }
};
class MuHash3072
{
private:
    Num3072 m_numerator;
    Num3072 m_denominator;
    Num3072 ToNum3072(Span<const unsigned char> in);
public:
    MuHash3072() noexcept {};
    explicit MuHash3072(Span<const unsigned char> in) noexcept;
    MuHash3072& Insert(Span<const unsigned char> in) noexcept;
    MuHash3072& Remove(Span<const unsigned char> in) noexcept;
    MuHash3072& operator*=(const MuHash3072& mul) noexcept;
    MuHash3072& operator/=(const MuHash3072& div) noexcept;
    void Finalize(uint256& out) noexcept;
    SERIALIZE_METHODS(MuHash3072, obj)
    {
        READWRITE(obj.m_numerator);
        READWRITE(obj.m_denominator);
    }
};
#endif
