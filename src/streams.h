#ifndef LCRYP_STREAMS_H
#define LCRYP_STREAMS_H
#include <serialize.h>
#include <span.h>
#include <support/allocators/zeroafterfree.h>
#include <util/overflow.h>
#include <algorithm>
#include <assert.h>
#include <cstdio>
#include <ios>
#include <limits>
#include <optional>
#include <stdint.h>
#include <string.h>
#include <string>
#include <utility>
#include <vector>
template<typename Stream>
class OverrideStream
{
    Stream* stream;
    const int nType;
    const int nVersion;
public:
    OverrideStream(Stream* stream_, int nType_, int nVersion_) : stream(stream_), nType(nType_), nVersion(nVersion_) {}
    template<typename T>
    OverrideStream<Stream>& operator<<(const T& obj)
    {
        ::Serialize(*this, obj);
        return (*this);
    }
    template<typename T>
    OverrideStream<Stream>& operator>>(T&& obj)
    {
        ::Unserialize(*this, obj);
        return (*this);
    }
    void write(Span<const std::byte> src)
    {
        stream->write(src);
    }
    void read(Span<std::byte> dst)
    {
        stream->read(dst);
    }
    int GetVersion() const { return nVersion; }
    int GetType() const { return nType; }
    size_t size() const { return stream->size(); }
    void ignore(size_t size) { return stream->ignore(size); }
};
class CVectorWriter
{
 public:
    CVectorWriter(int nTypeIn, int nVersionIn, std::vector<unsigned char>& vchDataIn, size_t nPosIn) : nType(nTypeIn), nVersion(nVersionIn), vchData(vchDataIn), nPos(nPosIn)
    {
        if(nPos > vchData.size())
            vchData.resize(nPos);
    }
    template <typename... Args>
    CVectorWriter(int nTypeIn, int nVersionIn, std::vector<unsigned char>& vchDataIn, size_t nPosIn, Args&&... args) : CVectorWriter(nTypeIn, nVersionIn, vchDataIn, nPosIn)
    {
        ::SerializeMany(*this, std::forward<Args>(args)...);
    }
    void write(Span<const std::byte> src)
    {
        assert(nPos <= vchData.size());
        size_t nOverwrite = std::min(src.size(), vchData.size() - nPos);
        if (nOverwrite) {
            memcpy(vchData.data() + nPos, src.data(), nOverwrite);
        }
        if (nOverwrite < src.size()) {
            vchData.insert(vchData.end(), UCharCast(src.data()) + nOverwrite, UCharCast(src.end()));
        }
        nPos += src.size();
    }
    template<typename T>
    CVectorWriter& operator<<(const T& obj)
    {
        ::Serialize(*this, obj);
        return (*this);
    }
    int GetVersion() const
    {
        return nVersion;
    }
    int GetType() const
    {
        return nType;
    }
private:
    const int nType;
    const int nVersion;
    std::vector<unsigned char>& vchData;
    size_t nPos;
};
class SpanReader
{
private:
    const int m_type;
    const int m_version;
    Span<const unsigned char> m_data;
public:
    SpanReader(int type, int version, Span<const unsigned char> data)
        : m_type(type), m_version(version), m_data(data) {}
    template<typename T>
    SpanReader& operator>>(T&& obj)
    {
        ::Unserialize(*this, obj);
        return (*this);
    }
    int GetVersion() const { return m_version; }
    int GetType() const { return m_type; }
    size_t size() const { return m_data.size(); }
    bool empty() const { return m_data.empty(); }
    void read(Span<std::byte> dst)
    {
        if (dst.size() == 0) {
            return;
        }
        if (dst.size() > m_data.size()) {
            throw std::ios_base::failure("SpanReader::read(): end of data");
        }
        memcpy(dst.data(), m_data.data(), dst.size());
        m_data = m_data.subspan(dst.size());
    }
};
class CDataStream
{
protected:
    using vector_type = SerializeData;
    vector_type vch;
    vector_type::size_type m_read_pos{0};
    int nType;
    int nVersion;
public:
    typedef vector_type::allocator_type   allocator_type;
    typedef vector_type::size_type        size_type;
    typedef vector_type::difference_type  difference_type;
    typedef vector_type::reference        reference;
    typedef vector_type::const_reference  const_reference;
    typedef vector_type::value_type       value_type;
    typedef vector_type::iterator         iterator;
    typedef vector_type::const_iterator   const_iterator;
    typedef vector_type::reverse_iterator reverse_iterator;
    explicit CDataStream(int nTypeIn, int nVersionIn)
        : nType{nTypeIn},
          nVersion{nVersionIn} {}
    explicit CDataStream(Span<const uint8_t> sp, int type, int version) : CDataStream{AsBytes(sp), type, version} {}
    explicit CDataStream(Span<const value_type> sp, int nTypeIn, int nVersionIn)
        : vch(sp.data(), sp.data() + sp.size()),
          nType{nTypeIn},
          nVersion{nVersionIn} {}
    template <typename... Args>
    CDataStream(int nTypeIn, int nVersionIn, Args&&... args)
        : nType{nTypeIn},
          nVersion{nVersionIn}
    {
        ::SerializeMany(*this, std::forward<Args>(args)...);
    }
    std::string str() const
    {
        return std::string{UCharCast(data()), UCharCast(data() + size())};
    }
    const_iterator begin() const                     { return vch.begin() + m_read_pos; }
    iterator begin()                                 { return vch.begin() + m_read_pos; }
    const_iterator end() const                       { return vch.end(); }
    iterator end()                                   { return vch.end(); }
    size_type size() const                           { return vch.size() - m_read_pos; }
    bool empty() const                               { return vch.size() == m_read_pos; }
    void resize(size_type n, value_type c = value_type{}) { vch.resize(n + m_read_pos, c); }
    void reserve(size_type n)                        { vch.reserve(n + m_read_pos); }
    const_reference operator[](size_type pos) const  { return vch[pos + m_read_pos]; }
    reference operator[](size_type pos)              { return vch[pos + m_read_pos]; }
    void clear()                                     { vch.clear(); m_read_pos = 0; }
    value_type* data()                               { return vch.data() + m_read_pos; }
    const value_type* data() const                   { return vch.data() + m_read_pos; }
    inline void Compact()
    {
        vch.erase(vch.begin(), vch.begin() + m_read_pos);
        m_read_pos = 0;
    }
    bool Rewind(std::optional<size_type> n = std::nullopt)
    {
        if (!n) {
            m_read_pos = 0;
            return true;
        }
        if (*n > m_read_pos)
            return false;
        m_read_pos -= *n;
        return true;
    }
    bool eof() const             { return size() == 0; }
    CDataStream* rdbuf()         { return this; }
    int in_avail() const         { return size(); }
    void SetType(int n)          { nType = n; }
    int GetType() const          { return nType; }
    void SetVersion(int n)       { nVersion = n; }
    int GetVersion() const       { return nVersion; }
    void read(Span<value_type> dst)
    {
        if (dst.size() == 0) return;
        auto next_read_pos{CheckedAdd(m_read_pos, dst.size())};
        if (!next_read_pos.has_value() || next_read_pos.value() > vch.size()) {
            throw std::ios_base::failure("CDataStream::read(): end of data");
        }
        memcpy(dst.data(), &vch[m_read_pos], dst.size());
        if (next_read_pos.value() == vch.size()) {
            m_read_pos = 0;
            vch.clear();
            return;
        }
        m_read_pos = next_read_pos.value();
    }
    void ignore(size_t num_ignore)
    {
        auto next_read_pos{CheckedAdd(m_read_pos, num_ignore)};
        if (!next_read_pos.has_value() || next_read_pos.value() > vch.size()) {
            throw std::ios_base::failure("CDataStream::ignore(): end of data");
        }
        if (next_read_pos.value() == vch.size()) {
            m_read_pos = 0;
            vch.clear();
            return;
        }
        m_read_pos = next_read_pos.value();
    }
    void write(Span<const value_type> src)
    {
        vch.insert(vch.end(), src.begin(), src.end());
    }
    template<typename Stream>
    void Serialize(Stream& s) const
    {
        if (!vch.empty())
            s.write(MakeByteSpan(vch));
    }
    template<typename T>
    CDataStream& operator<<(const T& obj)
    {
        ::Serialize(*this, obj);
        return (*this);
    }
    template<typename T>
    CDataStream& operator>>(T&& obj)
    {
        ::Unserialize(*this, obj);
        return (*this);
    }
    void Xor(const std::vector<unsigned char>& key)
    {
        if (key.size() == 0) {
            return;
        }
        for (size_type i = 0, j = 0; i != size(); i++) {
            vch[i] ^= std::byte{key[j++]};
            if (j == key.size())
                j = 0;
        }
    }
};
template <typename IStream>
class BitStreamReader
{
private:
    IStream& m_istream;
    uint8_t m_buffer{0};
    int m_offset{8};
public:
    explicit BitStreamReader(IStream& istream) : m_istream(istream) {}
    uint64_t Read(int nbits) {
        if (nbits < 0 || nbits > 64) {
            throw std::out_of_range("nbits must be between 0 and 64");
        }
        uint64_t data = 0;
        while (nbits > 0) {
            if (m_offset == 8) {
                m_istream >> m_buffer;
                m_offset = 0;
            }
            int bits = std::min(8 - m_offset, nbits);
            data <<= bits;
            data |= static_cast<uint8_t>(m_buffer << m_offset) >> (8 - bits);
            m_offset += bits;
            nbits -= bits;
        }
        return data;
    }
};
template <typename OStream>
class BitStreamWriter
{
private:
    OStream& m_ostream;
    uint8_t m_buffer{0};
    int m_offset{0};
public:
    explicit BitStreamWriter(OStream& ostream) : m_ostream(ostream) {}
    ~BitStreamWriter()
    {
        Flush();
    }
    void Write(uint64_t data, int nbits) {
        if (nbits < 0 || nbits > 64) {
            throw std::out_of_range("nbits must be between 0 and 64");
        }
        while (nbits > 0) {
            int bits = std::min(8 - m_offset, nbits);
            m_buffer |= (data << (64 - nbits)) >> (64 - 8 + m_offset);
            m_offset += bits;
            nbits -= bits;
            if (m_offset == 8) {
                Flush();
            }
        }
    }
    void Flush() {
        if (m_offset == 0) {
            return;
        }
        m_ostream << m_buffer;
        m_buffer = 0;
        m_offset = 0;
    }
};
class AutoFile
{
protected:
    FILE* file;
public:
    explicit AutoFile(FILE* filenew) : file{filenew} {}
    ~AutoFile()
    {
        fclose();
    }
    AutoFile(const AutoFile&) = delete;
    AutoFile& operator=(const AutoFile&) = delete;
    void fclose()
    {
        if (file) {
            ::fclose(file);
            file = nullptr;
        }
    }
    FILE* release()             { FILE* ret = file; file = nullptr; return ret; }
    FILE* Get() const           { return file; }
    bool IsNull() const         { return (file == nullptr); }
    void read(Span<std::byte> dst)
    {
        if (!file) throw std::ios_base::failure("AutoFile::read: file handle is nullptr");
        if (fread(dst.data(), 1, dst.size(), file) != dst.size()) {
            throw std::ios_base::failure(feof(file) ? "AutoFile::read: end of file" : "AutoFile::read: fread failed");
        }
    }
    void ignore(size_t nSize)
    {
        if (!file) throw std::ios_base::failure("AutoFile::ignore: file handle is nullptr");
        unsigned char data[4096];
        while (nSize > 0) {
            size_t nNow = std::min<size_t>(nSize, sizeof(data));
            if (fread(data, 1, nNow, file) != nNow)
                throw std::ios_base::failure(feof(file) ? "AutoFile::ignore: end of file" : "AutoFile::read: fread failed");
            nSize -= nNow;
        }
    }
    void write(Span<const std::byte> src)
    {
        if (!file) throw std::ios_base::failure("AutoFile::write: file handle is nullptr");
        if (fwrite(src.data(), 1, src.size(), file) != src.size()) {
            throw std::ios_base::failure("AutoFile::write: write failed");
        }
    }
    template <typename T>
    AutoFile& operator<<(const T& obj)
    {
        if (!file) throw std::ios_base::failure("AutoFile::operator<<: file handle is nullptr");
        ::Serialize(*this, obj);
        return *this;
    }
    template <typename T>
    AutoFile& operator>>(T&& obj)
    {
        if (!file) throw std::ios_base::failure("AutoFile::operator>>: file handle is nullptr");
        ::Unserialize(*this, obj);
        return *this;
    }
};
class CAutoFile : public AutoFile
{
private:
    const int nType;
    const int nVersion;
public:
    CAutoFile(FILE* filenew, int nTypeIn, int nVersionIn) : AutoFile{filenew}, nType(nTypeIn), nVersion(nVersionIn) {}
    int GetType() const          { return nType; }
    int GetVersion() const       { return nVersion; }
    template<typename T>
    CAutoFile& operator<<(const T& obj)
    {
        if (!file)
            throw std::ios_base::failure("CAutoFile::operator<<: file handle is nullptr");
        ::Serialize(*this, obj);
        return (*this);
    }
    template<typename T>
    CAutoFile& operator>>(T&& obj)
    {
        if (!file)
            throw std::ios_base::failure("CAutoFile::operator>>: file handle is nullptr");
        ::Unserialize(*this, obj);
        return (*this);
    }
};
class CBufferedFile
{
private:
    const int nType;
    const int nVersion;
    FILE *src;
    uint64_t nSrcPos;
    uint64_t m_read_pos;
    uint64_t nReadLimit;
    uint64_t nRewind;
    std::vector<std::byte> vchBuf;
protected:
    bool Fill() {
        unsigned int pos = nSrcPos % vchBuf.size();
        unsigned int readNow = vchBuf.size() - pos;
        unsigned int nAvail = vchBuf.size() - (nSrcPos - m_read_pos) - nRewind;
        if (nAvail < readNow)
            readNow = nAvail;
        if (readNow == 0)
            return false;
        size_t nBytes = fread((void*)&vchBuf[pos], 1, readNow, src);
        if (nBytes == 0) {
            throw std::ios_base::failure(feof(src) ? "CBufferedFile::Fill: end of file" : "CBufferedFile::Fill: fread failed");
        }
        nSrcPos += nBytes;
        return true;
    }
public:
    CBufferedFile(FILE* fileIn, uint64_t nBufSize, uint64_t nRewindIn, int nTypeIn, int nVersionIn)
        : nType(nTypeIn), nVersion(nVersionIn), nSrcPos(0), m_read_pos(0), nReadLimit(std::numeric_limits<uint64_t>::max()), nRewind(nRewindIn), vchBuf(nBufSize, std::byte{0})
    {
        if (nRewindIn >= nBufSize)
            throw std::ios_base::failure("Rewind limit must be less than buffer size");
        src = fileIn;
    }
    ~CBufferedFile()
    {
        fclose();
    }
    CBufferedFile(const CBufferedFile&) = delete;
    CBufferedFile& operator=(const CBufferedFile&) = delete;
    int GetVersion() const { return nVersion; }
    int GetType() const { return nType; }
    void fclose()
    {
        if (src) {
            ::fclose(src);
            src = nullptr;
        }
    }
    bool eof() const {
        return m_read_pos == nSrcPos && feof(src);
    }
    void read(Span<std::byte> dst)
    {
        if (dst.size() + m_read_pos > nReadLimit) {
            throw std::ios_base::failure("Read attempted past buffer limit");
        }
        while (dst.size() > 0) {
            if (m_read_pos == nSrcPos)
                Fill();
            unsigned int pos = m_read_pos % vchBuf.size();
            size_t nNow = dst.size();
            if (nNow + pos > vchBuf.size())
                nNow = vchBuf.size() - pos;
            if (nNow + m_read_pos > nSrcPos)
                nNow = nSrcPos - m_read_pos;
            memcpy(dst.data(), &vchBuf[pos], nNow);
            m_read_pos += nNow;
            dst = dst.subspan(nNow);
        }
    }
    uint64_t GetPos() const {
        return m_read_pos;
    }
    bool SetPos(uint64_t nPos) {
        size_t bufsize = vchBuf.size();
        if (nPos + bufsize < nSrcPos) {
            m_read_pos = nSrcPos - bufsize;
            return false;
        }
        if (nPos > nSrcPos) {
            m_read_pos = nSrcPos;
            return false;
        }
        m_read_pos = nPos;
        return true;
    }
    bool SetLimit(uint64_t nPos = std::numeric_limits<uint64_t>::max()) {
        if (nPos < m_read_pos)
            return false;
        nReadLimit = nPos;
        return true;
    }
    template<typename T>
    CBufferedFile& operator>>(T&& obj) {
        ::Unserialize(*this, obj);
        return (*this);
    }
    void FindByte(uint8_t ch)
    {
        while (true) {
            if (m_read_pos == nSrcPos)
                Fill();
            if (vchBuf[m_read_pos % vchBuf.size()] == std::byte{ch}) {
                break;
            }
            m_read_pos++;
        }
    }
};
#endif
