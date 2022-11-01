#ifndef LCRYP_FLATFILE_H
#define LCRYP_FLATFILE_H
#include <string>
#include <fs.h>
#include <serialize.h>
struct FlatFilePos
{
    int nFile;
    unsigned int nPos;
    SERIALIZE_METHODS(FlatFilePos, obj) { READWRITE(VARINT_MODE(obj.nFile, VarIntMode::NONNEGATIVE_SIGNED), VARINT(obj.nPos)); }
    FlatFilePos() : nFile(-1), nPos(0) {}
    FlatFilePos(int nFileIn, unsigned int nPosIn) :
        nFile(nFileIn),
        nPos(nPosIn)
    {}
    friend bool operator==(const FlatFilePos &a, const FlatFilePos &b) {
        return (a.nFile == b.nFile && a.nPos == b.nPos);
    }
    friend bool operator!=(const FlatFilePos &a, const FlatFilePos &b) {
        return !(a == b);
    }
    void SetNull() { nFile = -1; nPos = 0; }
    bool IsNull() const { return (nFile == -1); }
    std::string ToString() const;
};
class FlatFileSeq
{
private:
    const fs::path m_dir;
    const char* const m_prefix;
    const size_t m_chunk_size;
public:
    FlatFileSeq(fs::path dir, const char* prefix, size_t chunk_size);
    fs::path FileName(const FlatFilePos& pos) const;
    FILE* Open(const FlatFilePos& pos, bool read_only = false);
    size_t Allocate(const FlatFilePos& pos, size_t add_size, bool& out_of_space);
    bool Flush(const FlatFilePos& pos, bool finalize = false);
};
#endif
