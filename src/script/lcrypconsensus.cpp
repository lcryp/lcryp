#include <script/lcrypconsensus.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <version.h>
namespace {
class TxInputStream
{
public:
    TxInputStream(int nVersionIn, const unsigned char *txTo, size_t txToLen) :
    m_version(nVersionIn),
    m_data(txTo),
    m_remaining(txToLen)
    {}
    void read(Span<std::byte> dst)
    {
        if (dst.size() > m_remaining) {
            throw std::ios_base::failure(std::string(__func__) + ": end of data");
        }
        if (dst.data() == nullptr) {
            throw std::ios_base::failure(std::string(__func__) + ": bad destination buffer");
        }
        if (m_data == nullptr) {
            throw std::ios_base::failure(std::string(__func__) + ": bad source buffer");
        }
        memcpy(dst.data(), m_data, dst.size());
        m_remaining -= dst.size();
        m_data += dst.size();
    }
    template<typename T>
    TxInputStream& operator>>(T&& obj)
    {
        ::Unserialize(*this, obj);
        return *this;
    }
    int GetVersion() const { return m_version; }
private:
    const int m_version;
    const unsigned char* m_data;
    size_t m_remaining;
};
inline int set_error(lcrypconsensus_error* ret, lcrypconsensus_error serror)
{
    if (ret)
        *ret = serror;
    return 0;
}
struct ECCryptoClosure
{
    ECCVerifyHandle handle;
};
ECCryptoClosure instance_of_eccryptoclosure;
}
static bool verify_flags(unsigned int flags)
{
    return (flags & ~(lcrypconsensus_SCRIPT_FLAGS_VERIFY_ALL)) == 0;
}
static int verify_script(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen, CAmount amount,
                                    const unsigned char *txTo        , unsigned int txToLen,
                                    unsigned int nIn, unsigned int flags, lcrypconsensus_error* err)
{
    if (!verify_flags(flags)) {
        return set_error(err, lcrypconsensus_ERR_INVALID_FLAGS);
    }
    try {
        TxInputStream stream(PROTOCOL_VERSION, txTo, txToLen);
        CTransaction tx(deserialize, stream);
        if (nIn >= tx.vin.size())
            return set_error(err, lcrypconsensus_ERR_TX_INDEX);
        if (GetSerializeSize(tx, PROTOCOL_VERSION) != txToLen)
            return set_error(err, lcrypconsensus_ERR_TX_SIZE_MISMATCH);
        set_error(err, lcrypconsensus_ERR_OK);
        PrecomputedTransactionData txdata(tx);
        return VerifyScript(tx.vin[nIn].scriptSig, CScript(scriptPubKey, scriptPubKey + scriptPubKeyLen), &tx.vin[nIn].scriptWitness, flags, TransactionSignatureChecker(&tx, nIn, amount, txdata, MissingDataBehavior::FAIL), nullptr);
    } catch (const std::exception&) {
        return set_error(err, lcrypconsensus_ERR_TX_DESERIALIZE);
    }
}
int lcrypconsensus_verify_script_with_amount(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen, int64_t amount,
                                    const unsigned char *txTo        , unsigned int txToLen,
                                    unsigned int nIn, unsigned int flags, lcrypconsensus_error* err)
{
    CAmount am(amount);
    return ::verify_script(scriptPubKey, scriptPubKeyLen, am, txTo, txToLen, nIn, flags, err);
}
int lcrypconsensus_verify_script(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen,
                                   const unsigned char *txTo        , unsigned int txToLen,
                                   unsigned int nIn, unsigned int flags, lcrypconsensus_error* err)
{
    if (flags & lcrypconsensus_SCRIPT_FLAGS_VERIFY_WITNESS) {
        return set_error(err, lcrypconsensus_ERR_AMOUNT_REQUIRED);
    }
    CAmount am(0);
    return ::verify_script(scriptPubKey, scriptPubKeyLen, am, txTo, txToLen, nIn, flags, err);
}
unsigned int lcrypconsensus_version()
{
    return LCRYPCONSENSUS_API_VER;
}
