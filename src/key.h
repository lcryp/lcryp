#ifndef LCRYP_KEY_H
#define LCRYP_KEY_H
#include <pubkey.h>
#include <serialize.h>
#include <support/allocators/secure.h>
#include <uint256.h>
#include <stdexcept>
#include <vector>
typedef std::vector<unsigned char, secure_allocator<unsigned char> > CPrivKey;
class CKey
{
public:
    static const unsigned int SIZE            = 279;
    static const unsigned int COMPRESSED_SIZE = 214;
    static_assert(
        SIZE >= COMPRESSED_SIZE,
        "COMPRESSED_SIZE is larger than SIZE");
private:
    bool fValid;
    bool fCompressed;
    std::vector<unsigned char, secure_allocator<unsigned char> > keydata;
    bool static Check(const unsigned char* vch);
public:
    CKey() : fValid(false), fCompressed(false)
    {
        keydata.resize(32);
    }
    friend bool operator==(const CKey& a, const CKey& b)
    {
        return a.fCompressed == b.fCompressed &&
            a.size() == b.size() &&
            memcmp(a.keydata.data(), b.keydata.data(), a.size()) == 0;
    }
    template <typename T>
    void Set(const T pbegin, const T pend, bool fCompressedIn)
    {
        if (size_t(pend - pbegin) != keydata.size()) {
            fValid = false;
        } else if (Check(&pbegin[0])) {
            memcpy(keydata.data(), (unsigned char*)&pbegin[0], keydata.size());
            fValid = true;
            fCompressed = fCompressedIn;
        } else {
            fValid = false;
        }
    }
    unsigned int size() const { return (fValid ? keydata.size() : 0); }
    const std::byte* data() const { return reinterpret_cast<const std::byte*>(keydata.data()); }
    const unsigned char* begin() const { return keydata.data(); }
    const unsigned char* end() const { return keydata.data() + size(); }
    bool IsValid() const { return fValid; }
    bool IsCompressed() const { return fCompressed; }
    void MakeNewKey(bool fCompressed);
    bool Negate();
    CPrivKey GetPrivKey() const;
    CPubKey GetPubKey() const;
    bool Sign(const uint256& hash, std::vector<unsigned char>& vchSig, bool grind = true, uint32_t test_case = 0) const;
    bool SignCompact(const uint256& hash, std::vector<unsigned char>& vchSig) const;
    bool SignSchnorr(const uint256& hash, Span<unsigned char> sig, const uint256* merkle_root, const uint256& aux) const;
    [[nodiscard]] bool Derive(CKey& keyChild, ChainCode &ccChild, unsigned int nChild, const ChainCode& cc) const;
    bool VerifyPubKey(const CPubKey& vchPubKey) const;
    bool Load(const CPrivKey& privkey, const CPubKey& vchPubKey, bool fSkipCheck);
};
struct CExtKey {
    unsigned char nDepth;
    unsigned char vchFingerprint[4];
    unsigned int nChild;
    ChainCode chaincode;
    CKey key;
    friend bool operator==(const CExtKey& a, const CExtKey& b)
    {
        return a.nDepth == b.nDepth &&
            memcmp(a.vchFingerprint, b.vchFingerprint, sizeof(vchFingerprint)) == 0 &&
            a.nChild == b.nChild &&
            a.chaincode == b.chaincode &&
            a.key == b.key;
    }
    void Encode(unsigned char code[BIP32_EXTKEY_SIZE]) const;
    void Decode(const unsigned char code[BIP32_EXTKEY_SIZE]);
    [[nodiscard]] bool Derive(CExtKey& out, unsigned int nChild) const;
    CExtPubKey Neuter() const;
    void SetSeed(Span<const std::byte> seed);
};
void ECC_Start();
void ECC_Stop();
bool ECC_InitSanityCheck();
#endif
