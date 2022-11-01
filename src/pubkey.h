#ifndef LCRYP_PUBKEY_H
#define LCRYP_PUBKEY_H
#include <hash.h>
#include <serialize.h>
#include <span.h>
#include <uint256.h>
#include <cstring>
#include <optional>
#include <vector>
const unsigned int BIP32_EXTKEY_SIZE = 74;
const unsigned int BIP32_EXTKEY_WITH_VERSION_SIZE = 78;
class CKeyID : public uint160
{
public:
    CKeyID() : uint160() {}
    explicit CKeyID(const uint160& in) : uint160(in) {}
};
typedef uint256 ChainCode;
class CPubKey
{
public:
    static constexpr unsigned int SIZE                   = 65;
    static constexpr unsigned int COMPRESSED_SIZE        = 33;
    static constexpr unsigned int SIGNATURE_SIZE         = 72;
    static constexpr unsigned int COMPACT_SIGNATURE_SIZE = 65;
    static_assert(
        SIZE >= COMPRESSED_SIZE,
        "COMPRESSED_SIZE is larger than SIZE");
private:
    unsigned char vch[SIZE];
    unsigned int static GetLen(unsigned char chHeader)
    {
        if (chHeader == 2 || chHeader == 3)
            return COMPRESSED_SIZE;
        if (chHeader == 4 || chHeader == 6 || chHeader == 7)
            return SIZE;
        return 0;
    }
    void Invalidate()
    {
        vch[0] = 0xFF;
    }
public:
    bool static ValidSize(const std::vector<unsigned char> &vch) {
      return vch.size() > 0 && GetLen(vch[0]) == vch.size();
    }
    CPubKey()
    {
        Invalidate();
    }
    template <typename T>
    void Set(const T pbegin, const T pend)
    {
        int len = pend == pbegin ? 0 : GetLen(pbegin[0]);
        if (len && len == (pend - pbegin))
            memcpy(vch, (unsigned char*)&pbegin[0], len);
        else
            Invalidate();
    }
    template <typename T>
    CPubKey(const T pbegin, const T pend)
    {
        Set(pbegin, pend);
    }
    explicit CPubKey(Span<const uint8_t> _vch)
    {
        Set(_vch.begin(), _vch.end());
    }
    unsigned int size() const { return GetLen(vch[0]); }
    const unsigned char* data() const { return vch; }
    const unsigned char* begin() const { return vch; }
    const unsigned char* end() const { return vch + size(); }
    const unsigned char& operator[](unsigned int pos) const { return vch[pos]; }
    friend bool operator==(const CPubKey& a, const CPubKey& b)
    {
        return a.vch[0] == b.vch[0] &&
               memcmp(a.vch, b.vch, a.size()) == 0;
    }
    friend bool operator!=(const CPubKey& a, const CPubKey& b)
    {
        return !(a == b);
    }
    friend bool operator<(const CPubKey& a, const CPubKey& b)
    {
        return a.vch[0] < b.vch[0] ||
               (a.vch[0] == b.vch[0] && memcmp(a.vch, b.vch, a.size()) < 0);
    }
    friend bool operator>(const CPubKey& a, const CPubKey& b)
    {
        return a.vch[0] > b.vch[0] ||
               (a.vch[0] == b.vch[0] && memcmp(a.vch, b.vch, a.size()) > 0);
    }
    template <typename Stream>
    void Serialize(Stream& s) const
    {
        unsigned int len = size();
        ::WriteCompactSize(s, len);
        s.write(AsBytes(Span{vch, len}));
    }
    template <typename Stream>
    void Unserialize(Stream& s)
    {
        const unsigned int len(::ReadCompactSize(s));
        if (len <= SIZE) {
            s.read(AsWritableBytes(Span{vch, len}));
            if (len != size()) {
                Invalidate();
            }
        } else {
            s.ignore(len);
            Invalidate();
        }
    }
    CKeyID GetID() const
    {
        return CKeyID(Hash160(Span{vch}.first(size())));
    }
    uint256 GetHash() const
    {
        return Hash(Span{vch}.first(size()));
    }
    bool IsValid() const
    {
        return size() > 0;
    }
    bool IsFullyValid() const;
    bool IsCompressed() const
    {
        return size() == COMPRESSED_SIZE;
    }
    bool Verify(const uint256& hash, const std::vector<unsigned char>& vchSig) const;
    static bool CheckLowS(const std::vector<unsigned char>& vchSig);
    bool RecoverCompact(const uint256& hash, const std::vector<unsigned char>& vchSig);
    bool Decompress();
    [[nodiscard]] bool Derive(CPubKey& pubkeyChild, ChainCode &ccChild, unsigned int nChild, const ChainCode& cc) const;
};
class XOnlyPubKey
{
private:
    uint256 m_keydata;
public:
    XOnlyPubKey() = default;
    XOnlyPubKey(const XOnlyPubKey&) = default;
    XOnlyPubKey& operator=(const XOnlyPubKey&) = default;
    bool IsFullyValid() const;
    bool IsNull() const { return m_keydata.IsNull(); }
    explicit XOnlyPubKey(Span<const unsigned char> bytes);
    explicit XOnlyPubKey(const CPubKey& pubkey) : XOnlyPubKey(Span{pubkey}.subspan(1, 32)) {}
    bool VerifySchnorr(const uint256& msg, Span<const unsigned char> sigbytes) const;
    uint256 ComputeTapTweakHash(const uint256* merkle_root) const;
    bool CheckTapTweak(const XOnlyPubKey& internal, const uint256& merkle_root, bool parity) const;
    std::optional<std::pair<XOnlyPubKey, bool>> CreateTapTweak(const uint256* merkle_root) const;
    std::vector<CKeyID> GetKeyIDs() const;
    const unsigned char& operator[](int pos) const { return *(m_keydata.begin() + pos); }
    const unsigned char* data() const { return m_keydata.begin(); }
    static constexpr size_t size() { return decltype(m_keydata)::size(); }
    const unsigned char* begin() const { return m_keydata.begin(); }
    const unsigned char* end() const { return m_keydata.end(); }
    unsigned char* begin() { return m_keydata.begin(); }
    unsigned char* end() { return m_keydata.end(); }
    bool operator==(const XOnlyPubKey& other) const { return m_keydata == other.m_keydata; }
    bool operator!=(const XOnlyPubKey& other) const { return m_keydata != other.m_keydata; }
    bool operator<(const XOnlyPubKey& other) const { return m_keydata < other.m_keydata; }
    SERIALIZE_METHODS(XOnlyPubKey, obj) { READWRITE(obj.m_keydata); }
};
struct CExtPubKey {
    unsigned char version[4];
    unsigned char nDepth;
    unsigned char vchFingerprint[4];
    unsigned int nChild;
    ChainCode chaincode;
    CPubKey pubkey;
    friend bool operator==(const CExtPubKey &a, const CExtPubKey &b)
    {
        return a.nDepth == b.nDepth &&
            memcmp(a.vchFingerprint, b.vchFingerprint, sizeof(vchFingerprint)) == 0 &&
            a.nChild == b.nChild &&
            a.chaincode == b.chaincode &&
            a.pubkey == b.pubkey;
    }
    friend bool operator!=(const CExtPubKey &a, const CExtPubKey &b)
    {
        return !(a == b);
    }
    friend bool operator<(const CExtPubKey &a, const CExtPubKey &b)
    {
        if (a.pubkey < b.pubkey) {
            return true;
        } else if (a.pubkey > b.pubkey) {
            return false;
        }
        return a.chaincode < b.chaincode;
    }
    void Encode(unsigned char code[BIP32_EXTKEY_SIZE]) const;
    void Decode(const unsigned char code[BIP32_EXTKEY_SIZE]);
    void EncodeWithVersion(unsigned char code[BIP32_EXTKEY_WITH_VERSION_SIZE]) const;
    void DecodeWithVersion(const unsigned char code[BIP32_EXTKEY_WITH_VERSION_SIZE]);
    [[nodiscard]] bool Derive(CExtPubKey& out, unsigned int nChild) const;
};
class ECCVerifyHandle
{
    static int refcount;
public:
    ECCVerifyHandle();
    ~ECCVerifyHandle();
};
typedef struct secp256k1_context_struct secp256k1_context;
const secp256k1_context* GetVerifyContext();
#endif
