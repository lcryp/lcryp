#ifndef LCRYP_SCRIPT_SIGN_H
#define LCRYP_SCRIPT_SIGN_H
#include <attributes.h>
#include <coins.h>
#include <hash.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <script/keyorigin.h>
#include <script/standard.h>
class CKey;
class CKeyID;
class CScript;
class CTransaction;
class SigningProvider;
struct bilingual_str;
struct CMutableTransaction;
class BaseSignatureCreator {
public:
    virtual ~BaseSignatureCreator() {}
    virtual const BaseSignatureChecker& Checker() const =0;
    virtual bool CreateSig(const SigningProvider& provider, std::vector<unsigned char>& vchSig, const CKeyID& keyid, const CScript& scriptCode, SigVersion sigversion) const =0;
    virtual bool CreateSchnorrSig(const SigningProvider& provider, std::vector<unsigned char>& sig, const XOnlyPubKey& pubkey, const uint256* leaf_hash, const uint256* merkle_root, SigVersion sigversion) const =0;
};
class MutableTransactionSignatureCreator : public BaseSignatureCreator
{
    const CMutableTransaction& m_txto;
    unsigned int nIn;
    int nHashType;
    CAmount amount;
    const MutableTransactionSignatureChecker checker;
    const PrecomputedTransactionData* m_txdata;
public:
    MutableTransactionSignatureCreator(const CMutableTransaction& tx LIFETIMEBOUND, unsigned int input_idx, const CAmount& amount, int hash_type);
    MutableTransactionSignatureCreator(const CMutableTransaction& tx LIFETIMEBOUND, unsigned int input_idx, const CAmount& amount, const PrecomputedTransactionData* txdata, int hash_type);
    const BaseSignatureChecker& Checker() const override { return checker; }
    bool CreateSig(const SigningProvider& provider, std::vector<unsigned char>& vchSig, const CKeyID& keyid, const CScript& scriptCode, SigVersion sigversion) const override;
    bool CreateSchnorrSig(const SigningProvider& provider, std::vector<unsigned char>& sig, const XOnlyPubKey& pubkey, const uint256* leaf_hash, const uint256* merkle_root, SigVersion sigversion) const override;
};
extern const BaseSignatureChecker& DUMMY_CHECKER;
extern const BaseSignatureCreator& DUMMY_SIGNATURE_CREATOR;
extern const BaseSignatureCreator& DUMMY_MAXIMUM_SIGNATURE_CREATOR;
typedef std::pair<CPubKey, std::vector<unsigned char>> SigPair;
struct SignatureData {
    bool complete = false;
    bool witness = false;
    CScript scriptSig;
    CScript redeem_script;
    CScript witness_script;
    CScriptWitness scriptWitness;
    TaprootSpendData tr_spenddata;
    std::optional<TaprootBuilder> tr_builder;
    std::map<CKeyID, SigPair> signatures;
    std::map<CKeyID, std::pair<CPubKey, KeyOriginInfo>> misc_pubkeys;
    std::vector<unsigned char> taproot_key_path_sig;
    std::map<std::pair<XOnlyPubKey, uint256>, std::vector<unsigned char>> taproot_script_sigs;
    std::map<XOnlyPubKey, std::pair<std::set<uint256>, KeyOriginInfo>> taproot_misc_pubkeys;
    std::vector<CKeyID> missing_pubkeys;
    std::vector<CKeyID> missing_sigs;
    uint160 missing_redeem_script;
    uint256 missing_witness_script;
    SignatureData() {}
    explicit SignatureData(const CScript& script) : scriptSig(script) {}
    void MergeSignatureData(SignatureData sigdata);
};
bool ProduceSignature(const SigningProvider& provider, const BaseSignatureCreator& creator, const CScript& scriptPubKey, SignatureData& sigdata);
bool SignSignature(const SigningProvider &provider, const CScript& fromPubKey, CMutableTransaction& txTo, unsigned int nIn, const CAmount& amount, int nHashType);
bool SignSignature(const SigningProvider &provider, const CTransaction& txFrom, CMutableTransaction& txTo, unsigned int nIn, int nHashType);
SignatureData DataFromTransaction(const CMutableTransaction& tx, unsigned int nIn, const CTxOut& txout);
void UpdateInput(CTxIn& input, const SignatureData& data);
bool IsSegWitOutput(const SigningProvider& provider, const CScript& script);
bool SignTransaction(CMutableTransaction& mtx, const SigningProvider* provider, const std::map<COutPoint, Coin>& coins, int sighash, std::map<int, bilingual_str>& input_errors);
#endif
