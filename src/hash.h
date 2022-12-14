#ifndef LCRYP_HASH_H
#define LCRYP_HASH_H
#include <crypto/common.h>
#include <crypto/ripemd160.h>
#include <crypto/sha256.h>
#include <prevector.h>
#include <serialize.h>
#include <uint256.h>
#include <version.h>
#include <string>
#include <vector>
typedef uint256 ChainCode;
class CHash256 {
private:
    CSHA256 sha;
public:
    static const size_t OUTPUT_SIZE = CSHA256::OUTPUT_SIZE;
    void Finalize(Span<unsigned char> output) {
        assert(output.size() == OUTPUT_SIZE);
        unsigned char buf[CSHA256::OUTPUT_SIZE];
        sha.Finalize(buf);
        sha.Reset().Write(buf, CSHA256::OUTPUT_SIZE).Finalize(output.data());
    }
    void FinalizeA(unsigned char hash[OUTPUT_SIZE]) {
        unsigned char buf[CSHA256::OUTPUT_SIZE];
        sha.Finalize(buf);
        sha.Reset().Write(buf, sha.OUTPUT_SIZE).Finalize(hash);
    }
    CHash256& Write(Span<const unsigned char> input) {
        sha.Write(input.data(), input.size());
        return *this;
    }
    CHash256& WriteA(const unsigned char* data, size_t len) {
        sha.Write(data, len);
        return *this;
    }
    CHash256& Reset() {
        sha.Reset();
        return *this;
    }
};
class CHash160 {
private:
    CSHA256 sha;
public:
    static const size_t OUTPUT_SIZE = CRIPEMD160::OUTPUT_SIZE;
    void Finalize(Span<unsigned char> output) {
        assert(output.size() == OUTPUT_SIZE);
        unsigned char buf[CSHA256::OUTPUT_SIZE];
        sha.Finalize(buf);
        CRIPEMD160().Write(buf, CSHA256::OUTPUT_SIZE).Finalize(output.data());
    }
    CHash160& Write(Span<const unsigned char> input) {
        sha.Write(input.data(), input.size());
        return *this;
    }
    CHash160& Reset() {
        sha.Reset();
        return *this;
    }
};
template<typename T>
inline uint256 Hash(const T& in1)
{
    uint256 result;
    CHash256().Write(MakeUCharSpan(in1)).Finalize(result);
    return result;
}
template<typename T1, typename T2>
inline uint256 Hash(const T1& in1, const T2& in2) {
    uint256 result;
    CHash256().Write(MakeUCharSpan(in1)).Write(MakeUCharSpan(in2)).Finalize(result);
    return result;
}
template<typename T1>
inline uint160 Hash160(const T1& in1)
{
    uint160 result;
    CHash160().Write(MakeUCharSpan(in1)).Finalize(result);
    return result;
}
class HashWriter
{
private:
    CSHA256 ctx;
public:
    void write(Span<const std::byte> src)
    {
        ctx.Write(UCharCast(src.data()), src.size());
    }
    uint256 GetHash() {
        uint256 result;
        ctx.Finalize(result.begin());
        ctx.Reset().Write(result.begin(), CSHA256::OUTPUT_SIZE).Finalize(result.begin());
        return result;
    }
    uint256 GetSHA256() {
        uint256 result;
        ctx.Finalize(result.begin());
        return result;
    }
    inline uint64_t GetCheapHash() {
        uint256 result = GetHash();
        return ReadLE64(result.begin());
    }
    template <typename T>
    HashWriter& operator<<(const T& obj)
    {
        ::Serialize(*this, obj);
        return *this;
    }
};
class CHashWriter : public HashWriter
{
private:
    const int nType;
    const int nVersion;
public:
    CHashWriter(int nTypeIn, int nVersionIn) : nType(nTypeIn), nVersion(nVersionIn) {}
    int GetType() const { return nType; }
    int GetVersion() const { return nVersion; }
    template<typename T>
    CHashWriter& operator<<(const T& obj) {
        ::Serialize(*this, obj);
        return (*this);
    }
};
template<typename Source>
class CHashVerifier : public CHashWriter
{
private:
    Source* source;
public:
    explicit CHashVerifier(Source* source_) : CHashWriter(source_->GetType(), source_->GetVersion()), source(source_) {}
    void read(Span<std::byte> dst)
    {
        source->read(dst);
        this->write(dst);
    }
    void ignore(size_t nSize)
    {
        std::byte data[1024];
        while (nSize > 0) {
            size_t now = std::min<size_t>(nSize, 1024);
            read({data, now});
            nSize -= now;
        }
    }
    template<typename T>
    CHashVerifier<Source>& operator>>(T&& obj)
    {
        ::Unserialize(*this, obj);
        return (*this);
    }
};
template<typename T>
uint256 SerializeHash(const T& obj, int nType=SER_GETHASH, int nVersion=PROTOCOL_VERSION)
{
    CHashWriter ss(nType, nVersion);
    ss << obj;
    return ss.GetHash();
}
[[nodiscard]] uint256 SHA256Uint256(const uint256& input);
unsigned int MurmurHash3(unsigned int nHashSeed, Span<const unsigned char> vDataToHash);
void BIP32Hash(const ChainCode &chainCode, unsigned int nChild, unsigned char header, const unsigned char data[32], unsigned char output[64]);
HashWriter TaggedHash(const std::string& tag);
#endif
