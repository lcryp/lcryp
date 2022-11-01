#ifndef LCRYP_PSBT_H
#define LCRYP_PSBT_H
#include <node/transaction.h>
#include <policy/feerate.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/keyorigin.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <span.h>
#include <streams.h>
#include <optional>
static constexpr uint8_t PSBT_MAGIC_BYTES[5] = {'p', 's', 'b', 't', 0xff};
static constexpr uint8_t PSBT_GLOBAL_UNSIGNED_TX = 0x00;
static constexpr uint8_t PSBT_GLOBAL_XPUB = 0x01;
static constexpr uint8_t PSBT_GLOBAL_VERSION = 0xFB;
static constexpr uint8_t PSBT_GLOBAL_PROPRIETARY = 0xFC;
static constexpr uint8_t PSBT_IN_NON_WITNESS_UTXO = 0x00;
static constexpr uint8_t PSBT_IN_WITNESS_UTXO = 0x01;
static constexpr uint8_t PSBT_IN_PARTIAL_SIG = 0x02;
static constexpr uint8_t PSBT_IN_SIGHASH = 0x03;
static constexpr uint8_t PSBT_IN_REDEEMSCRIPT = 0x04;
static constexpr uint8_t PSBT_IN_WITNESSSCRIPT = 0x05;
static constexpr uint8_t PSBT_IN_BIP32_DERIVATION = 0x06;
static constexpr uint8_t PSBT_IN_SCRIPTSIG = 0x07;
static constexpr uint8_t PSBT_IN_SCRIPTWITNESS = 0x08;
static constexpr uint8_t PSBT_IN_RIPEMD160 = 0x0A;
static constexpr uint8_t PSBT_IN_SHA256 = 0x0B;
static constexpr uint8_t PSBT_IN_HASH160 = 0x0C;
static constexpr uint8_t PSBT_IN_HASH256 = 0x0D;
static constexpr uint8_t PSBT_IN_TAP_KEY_SIG = 0x13;
static constexpr uint8_t PSBT_IN_TAP_SCRIPT_SIG = 0x14;
static constexpr uint8_t PSBT_IN_TAP_LEAF_SCRIPT = 0x15;
static constexpr uint8_t PSBT_IN_TAP_BIP32_DERIVATION = 0x16;
static constexpr uint8_t PSBT_IN_TAP_INTERNAL_KEY = 0x17;
static constexpr uint8_t PSBT_IN_TAP_MERKLE_ROOT = 0x18;
static constexpr uint8_t PSBT_IN_PROPRIETARY = 0xFC;
static constexpr uint8_t PSBT_OUT_REDEEMSCRIPT = 0x00;
static constexpr uint8_t PSBT_OUT_WITNESSSCRIPT = 0x01;
static constexpr uint8_t PSBT_OUT_BIP32_DERIVATION = 0x02;
static constexpr uint8_t PSBT_OUT_TAP_INTERNAL_KEY = 0x05;
static constexpr uint8_t PSBT_OUT_TAP_TREE = 0x06;
static constexpr uint8_t PSBT_OUT_TAP_BIP32_DERIVATION = 0x07;
static constexpr uint8_t PSBT_OUT_PROPRIETARY = 0xFC;
static constexpr uint8_t PSBT_SEPARATOR = 0x00;
const std::streamsize MAX_FILE_SIZE_PSBT = 100000000;
static constexpr uint32_t PSBT_HIGHEST_VERSION = 0;
struct PSBTProprietary
{
    uint64_t subtype;
    std::vector<unsigned char> identifier;
    std::vector<unsigned char> key;
    std::vector<unsigned char> value;
    bool operator<(const PSBTProprietary &b) const {
        return key < b.key;
    }
    bool operator==(const PSBTProprietary &b) const {
        return key == b.key;
    }
};
template<typename Stream, typename... X>
void SerializeToVector(Stream& s, const X&... args)
{
    WriteCompactSize(s, GetSerializeSizeMany(s.GetVersion(), args...));
    SerializeMany(s, args...);
}
template<typename Stream, typename... X>
void UnserializeFromVector(Stream& s, X&... args)
{
    size_t expected_size = ReadCompactSize(s);
    size_t remaining_before = s.size();
    UnserializeMany(s, args...);
    size_t remaining_after = s.size();
    if (remaining_after + expected_size != remaining_before) {
        throw std::ios_base::failure("Size of value was not the stated size");
    }
}
template<typename Stream>
KeyOriginInfo DeserializeKeyOrigin(Stream& s, uint64_t length)
{
    if (length % 4 || length == 0) {
        throw std::ios_base::failure("Invalid length for HD key path");
    }
    KeyOriginInfo hd_keypath;
    s >> hd_keypath.fingerprint;
    for (unsigned int i = 4; i < length; i += sizeof(uint32_t)) {
        uint32_t index;
        s >> index;
        hd_keypath.path.push_back(index);
    }
    return hd_keypath;
}
template<typename Stream>
void DeserializeHDKeypath(Stream& s, KeyOriginInfo& hd_keypath)
{
    hd_keypath = DeserializeKeyOrigin(s, ReadCompactSize(s));
}
template<typename Stream>
void DeserializeHDKeypaths(Stream& s, const std::vector<unsigned char>& key, std::map<CPubKey, KeyOriginInfo>& hd_keypaths)
{
    if (key.size() != CPubKey::SIZE + 1 && key.size() != CPubKey::COMPRESSED_SIZE + 1) {
        throw std::ios_base::failure("Size of key was not the expected size for the type BIP32 keypath");
    }
    CPubKey pubkey(key.begin() + 1, key.end());
    if (!pubkey.IsFullyValid()) {
       throw std::ios_base::failure("Invalid pubkey");
    }
    if (hd_keypaths.count(pubkey) > 0) {
        throw std::ios_base::failure("Duplicate Key, pubkey derivation path already provided");
    }
    KeyOriginInfo keypath;
    DeserializeHDKeypath(s, keypath);
    hd_keypaths.emplace(pubkey, std::move(keypath));
}
template<typename Stream>
void SerializeKeyOrigin(Stream& s, KeyOriginInfo hd_keypath)
{
    s << hd_keypath.fingerprint;
    for (const auto& path : hd_keypath.path) {
        s << path;
    }
}
template<typename Stream>
void SerializeHDKeypath(Stream& s, KeyOriginInfo hd_keypath)
{
    WriteCompactSize(s, (hd_keypath.path.size() + 1) * sizeof(uint32_t));
    SerializeKeyOrigin(s, hd_keypath);
}
template<typename Stream>
void SerializeHDKeypaths(Stream& s, const std::map<CPubKey, KeyOriginInfo>& hd_keypaths, CompactSizeWriter type)
{
    for (auto keypath_pair : hd_keypaths) {
        if (!keypath_pair.first.IsValid()) {
            throw std::ios_base::failure("Invalid CPubKey being serialized");
        }
        SerializeToVector(s, type, Span{keypath_pair.first});
        SerializeHDKeypath(s, keypath_pair.second);
    }
}
struct PSBTInput
{
    CTransactionRef non_witness_utxo;
    CTxOut witness_utxo;
    CScript redeem_script;
    CScript witness_script;
    CScript final_script_sig;
    CScriptWitness final_script_witness;
    std::map<CPubKey, KeyOriginInfo> hd_keypaths;
    std::map<CKeyID, SigPair> partial_sigs;
    std::map<uint160, std::vector<unsigned char>> ripemd160_preimages;
    std::map<uint256, std::vector<unsigned char>> sha256_preimages;
    std::map<uint160, std::vector<unsigned char>> hash160_preimages;
    std::map<uint256, std::vector<unsigned char>> hash256_preimages;
    std::vector<unsigned char> m_tap_key_sig;
    std::map<std::pair<XOnlyPubKey, uint256>, std::vector<unsigned char>> m_tap_script_sigs;
    std::map<std::pair<CScript, int>, std::set<std::vector<unsigned char>, ShortestVectorFirstComparator>> m_tap_scripts;
    std::map<XOnlyPubKey, std::pair<std::set<uint256>, KeyOriginInfo>> m_tap_bip32_paths;
    XOnlyPubKey m_tap_internal_key;
    uint256 m_tap_merkle_root;
    std::map<std::vector<unsigned char>, std::vector<unsigned char>> unknown;
    std::set<PSBTProprietary> m_proprietary;
    std::optional<int> sighash_type;
    bool IsNull() const;
    void FillSignatureData(SignatureData& sigdata) const;
    void FromSignatureData(const SignatureData& sigdata);
    void Merge(const PSBTInput& input);
    PSBTInput() {}
    template <typename Stream>
    inline void Serialize(Stream& s) const {
        if (non_witness_utxo) {
            SerializeToVector(s, CompactSizeWriter(PSBT_IN_NON_WITNESS_UTXO));
            OverrideStream<Stream> os(&s, s.GetType(), s.GetVersion() | SERIALIZE_TRANSACTION_NO_WITNESS);
            SerializeToVector(os, non_witness_utxo);
        }
        if (!witness_utxo.IsNull()) {
            SerializeToVector(s, CompactSizeWriter(PSBT_IN_WITNESS_UTXO));
            SerializeToVector(s, witness_utxo);
        }
        if (final_script_sig.empty() && final_script_witness.IsNull()) {
            for (auto sig_pair : partial_sigs) {
                SerializeToVector(s, CompactSizeWriter(PSBT_IN_PARTIAL_SIG), Span{sig_pair.second.first});
                s << sig_pair.second.second;
            }
            if (sighash_type != std::nullopt) {
                SerializeToVector(s, CompactSizeWriter(PSBT_IN_SIGHASH));
                SerializeToVector(s, *sighash_type);
            }
            if (!redeem_script.empty()) {
                SerializeToVector(s, CompactSizeWriter(PSBT_IN_REDEEMSCRIPT));
                s << redeem_script;
            }
            if (!witness_script.empty()) {
                SerializeToVector(s, CompactSizeWriter(PSBT_IN_WITNESSSCRIPT));
                s << witness_script;
            }
            SerializeHDKeypaths(s, hd_keypaths, CompactSizeWriter(PSBT_IN_BIP32_DERIVATION));
            for (const auto& [hash, preimage] : ripemd160_preimages) {
                SerializeToVector(s, CompactSizeWriter(PSBT_IN_RIPEMD160), Span{hash});
                s << preimage;
            }
            for (const auto& [hash, preimage] : sha256_preimages) {
                SerializeToVector(s, CompactSizeWriter(PSBT_IN_SHA256), Span{hash});
                s << preimage;
            }
            for (const auto& [hash, preimage] : hash160_preimages) {
                SerializeToVector(s, CompactSizeWriter(PSBT_IN_HASH160), Span{hash});
                s << preimage;
            }
            for (const auto& [hash, preimage] : hash256_preimages) {
                SerializeToVector(s, CompactSizeWriter(PSBT_IN_HASH256), Span{hash});
                s << preimage;
            }
            if (!m_tap_key_sig.empty()) {
                SerializeToVector(s, PSBT_IN_TAP_KEY_SIG);
                s << m_tap_key_sig;
            }
            for (const auto& [pubkey_leaf, sig] : m_tap_script_sigs) {
                const auto& [xonly, leaf_hash] = pubkey_leaf;
                SerializeToVector(s, PSBT_IN_TAP_SCRIPT_SIG, xonly, leaf_hash);
                s << sig;
            }
            for (const auto& [leaf, control_blocks] : m_tap_scripts) {
                const auto& [script, leaf_ver] = leaf;
                for (const auto& control_block : control_blocks) {
                    SerializeToVector(s, PSBT_IN_TAP_LEAF_SCRIPT, Span{control_block});
                    std::vector<unsigned char> value_v(script.begin(), script.end());
                    value_v.push_back((uint8_t)leaf_ver);
                    s << value_v;
                }
            }
            for (const auto& [xonly, leaf_origin] : m_tap_bip32_paths) {
                const auto& [leaf_hashes, origin] = leaf_origin;
                SerializeToVector(s, PSBT_IN_TAP_BIP32_DERIVATION, xonly);
                std::vector<unsigned char> value;
                CVectorWriter s_value(s.GetType(), s.GetVersion(), value, 0);
                s_value << leaf_hashes;
                SerializeKeyOrigin(s_value, origin);
                s << value;
            }
            if (!m_tap_internal_key.IsNull()) {
                SerializeToVector(s, PSBT_IN_TAP_INTERNAL_KEY);
                s << ToByteVector(m_tap_internal_key);
            }
            if (!m_tap_merkle_root.IsNull()) {
                SerializeToVector(s, PSBT_IN_TAP_MERKLE_ROOT);
                SerializeToVector(s, m_tap_merkle_root);
            }
        }
        if (!final_script_sig.empty()) {
            SerializeToVector(s, CompactSizeWriter(PSBT_IN_SCRIPTSIG));
            s << final_script_sig;
        }
        if (!final_script_witness.IsNull()) {
            SerializeToVector(s, CompactSizeWriter(PSBT_IN_SCRIPTWITNESS));
            SerializeToVector(s, final_script_witness.stack);
        }
        for (const auto& entry : m_proprietary) {
            s << entry.key;
            s << entry.value;
        }
        for (auto& entry : unknown) {
            s << entry.first;
            s << entry.second;
        }
        s << PSBT_SEPARATOR;
    }
    template <typename Stream>
    inline void Unserialize(Stream& s) {
        std::set<std::vector<unsigned char>> key_lookup;
        bool found_sep = false;
        while(!s.empty()) {
            std::vector<unsigned char> key;
            s >> key;
            if (key.empty()) {
                found_sep = true;
                break;
            }
            SpanReader skey(s.GetType(), s.GetVersion(), key);
            uint64_t type = ReadCompactSize(skey);
            switch(type) {
                case PSBT_IN_NON_WITNESS_UTXO:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input non-witness utxo already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Non-witness utxo key is more than one byte type");
                    }
                    OverrideStream<Stream> os(&s, s.GetType(), s.GetVersion() & ~SERIALIZE_TRANSACTION_NO_WITNESS);
                    UnserializeFromVector(os, non_witness_utxo);
                    break;
                }
                case PSBT_IN_WITNESS_UTXO:
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input witness utxo already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Witness utxo key is more than one byte type");
                    }
                    UnserializeFromVector(s, witness_utxo);
                    break;
                case PSBT_IN_PARTIAL_SIG:
                {
                    if (key.size() != CPubKey::SIZE + 1 && key.size() != CPubKey::COMPRESSED_SIZE + 1) {
                        throw std::ios_base::failure("Size of key was not the expected size for the type partial signature pubkey");
                    }
                    CPubKey pubkey(key.begin() + 1, key.end());
                    if (!pubkey.IsFullyValid()) {
                       throw std::ios_base::failure("Invalid pubkey");
                    }
                    if (partial_sigs.count(pubkey.GetID()) > 0) {
                        throw std::ios_base::failure("Duplicate Key, input partial signature for pubkey already provided");
                    }
                    std::vector<unsigned char> sig;
                    s >> sig;
                    partial_sigs.emplace(pubkey.GetID(), SigPair(pubkey, std::move(sig)));
                    break;
                }
                case PSBT_IN_SIGHASH:
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input sighash type already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Sighash type key is more than one byte type");
                    }
                    int sighash;
                    UnserializeFromVector(s, sighash);
                    sighash_type = sighash;
                    break;
                case PSBT_IN_REDEEMSCRIPT:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input redeemScript already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Input redeemScript key is more than one byte type");
                    }
                    s >> redeem_script;
                    break;
                }
                case PSBT_IN_WITNESSSCRIPT:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input witnessScript already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Input witnessScript key is more than one byte type");
                    }
                    s >> witness_script;
                    break;
                }
                case PSBT_IN_BIP32_DERIVATION:
                {
                    DeserializeHDKeypaths(s, key, hd_keypaths);
                    break;
                }
                case PSBT_IN_SCRIPTSIG:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input final scriptSig already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Final scriptSig key is more than one byte type");
                    }
                    s >> final_script_sig;
                    break;
                }
                case PSBT_IN_SCRIPTWITNESS:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input final scriptWitness already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Final scriptWitness key is more than one byte type");
                    }
                    UnserializeFromVector(s, final_script_witness.stack);
                    break;
                }
                case PSBT_IN_RIPEMD160:
                {
                    if (key.size() != CRIPEMD160::OUTPUT_SIZE + 1) {
                        throw std::ios_base::failure("Size of key was not the expected size for the type ripemd160 preimage");
                    }
                    std::vector<unsigned char> hash_vec(key.begin() + 1, key.end());
                    uint160 hash(hash_vec);
                    if (ripemd160_preimages.count(hash) > 0) {
                        throw std::ios_base::failure("Duplicate Key, input ripemd160 preimage already provided");
                    }
                    std::vector<unsigned char> preimage;
                    s >> preimage;
                    ripemd160_preimages.emplace(hash, std::move(preimage));
                    break;
                }
                case PSBT_IN_SHA256:
                {
                    if (key.size() != CSHA256::OUTPUT_SIZE + 1) {
                        throw std::ios_base::failure("Size of key was not the expected size for the type sha256 preimage");
                    }
                    std::vector<unsigned char> hash_vec(key.begin() + 1, key.end());
                    uint256 hash(hash_vec);
                    if (sha256_preimages.count(hash) > 0) {
                        throw std::ios_base::failure("Duplicate Key, input sha256 preimage already provided");
                    }
                    std::vector<unsigned char> preimage;
                    s >> preimage;
                    sha256_preimages.emplace(hash, std::move(preimage));
                    break;
                }
                case PSBT_IN_HASH160:
                {
                    if (key.size() != CHash160::OUTPUT_SIZE + 1) {
                        throw std::ios_base::failure("Size of key was not the expected size for the type hash160 preimage");
                    }
                    std::vector<unsigned char> hash_vec(key.begin() + 1, key.end());
                    uint160 hash(hash_vec);
                    if (hash160_preimages.count(hash) > 0) {
                        throw std::ios_base::failure("Duplicate Key, input hash160 preimage already provided");
                    }
                    std::vector<unsigned char> preimage;
                    s >> preimage;
                    hash160_preimages.emplace(hash, std::move(preimage));
                    break;
                }
                case PSBT_IN_HASH256:
                {
                    if (key.size() != CHash256::OUTPUT_SIZE + 1) {
                        throw std::ios_base::failure("Size of key was not the expected size for the type hash256 preimage");
                    }
                    std::vector<unsigned char> hash_vec(key.begin() + 1, key.end());
                    uint256 hash(hash_vec);
                    if (hash256_preimages.count(hash) > 0) {
                        throw std::ios_base::failure("Duplicate Key, input hash256 preimage already provided");
                    }
                    std::vector<unsigned char> preimage;
                    s >> preimage;
                    hash256_preimages.emplace(hash, std::move(preimage));
                    break;
                }
                case PSBT_IN_TAP_KEY_SIG:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input Taproot key signature already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Input Taproot key signature key is more than one byte type");
                    }
                    s >> m_tap_key_sig;
                    if (m_tap_key_sig.size() < 64) {
                        throw std::ios_base::failure("Input Taproot key path signature is shorter than 64 bytes");
                    } else if (m_tap_key_sig.size() > 65) {
                        throw std::ios_base::failure("Input Taproot key path signature is longer than 65 bytes");
                    }
                    break;
                }
                case PSBT_IN_TAP_SCRIPT_SIG:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input Taproot script signature already provided");
                    } else if (key.size() != 65) {
                        throw std::ios_base::failure("Input Taproot script signature key is not 65 bytes");
                    }
                    SpanReader s_key(s.GetType(), s.GetVersion(), Span{key}.subspan(1));
                    XOnlyPubKey xonly;
                    uint256 hash;
                    s_key >> xonly;
                    s_key >> hash;
                    std::vector<unsigned char> sig;
                    s >> sig;
                    if (sig.size() < 64) {
                        throw std::ios_base::failure("Input Taproot script path signature is shorter than 64 bytes");
                    } else if (sig.size() > 65) {
                        throw std::ios_base::failure("Input Taproot script path signature is longer than 65 bytes");
                    }
                    m_tap_script_sigs.emplace(std::make_pair(xonly, hash), sig);
                    break;
                }
                case PSBT_IN_TAP_LEAF_SCRIPT:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input Taproot leaf script already provided");
                    } else if (key.size() < 34) {
                        throw std::ios_base::failure("Taproot leaf script key is not at least 34 bytes");
                    } else if ((key.size() - 2) % 32 != 0) {
                        throw std::ios_base::failure("Input Taproot leaf script key's control block size is not valid");
                    }
                    std::vector<unsigned char> script_v;
                    s >> script_v;
                    if (script_v.empty()) {
                        throw std::ios_base::failure("Input Taproot leaf script must be at least 1 byte");
                    }
                    uint8_t leaf_ver = script_v.back();
                    script_v.pop_back();
                    const auto leaf_script = std::make_pair(CScript(script_v.begin(), script_v.end()), (int)leaf_ver);
                    m_tap_scripts[leaf_script].insert(std::vector<unsigned char>(key.begin() + 1, key.end()));
                    break;
                }
                case PSBT_IN_TAP_BIP32_DERIVATION:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input Taproot BIP32 keypath already provided");
                    } else if (key.size() != 33) {
                        throw std::ios_base::failure("Input Taproot BIP32 keypath key is not at 33 bytes");
                    }
                    SpanReader s_key(s.GetType(), s.GetVersion(), Span{key}.subspan(1));
                    XOnlyPubKey xonly;
                    s_key >> xonly;
                    std::set<uint256> leaf_hashes;
                    uint64_t value_len = ReadCompactSize(s);
                    size_t before_hashes = s.size();
                    s >> leaf_hashes;
                    size_t after_hashes = s.size();
                    size_t hashes_len = before_hashes - after_hashes;
                    if (hashes_len > value_len) {
                        throw std::ios_base::failure("Input Taproot BIP32 keypath has an invalid length");
                    }
                    size_t origin_len = value_len - hashes_len;
                    m_tap_bip32_paths.emplace(xonly, std::make_pair(leaf_hashes, DeserializeKeyOrigin(s, origin_len)));
                    break;
                }
                case PSBT_IN_TAP_INTERNAL_KEY:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input Taproot internal key already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Input Taproot internal key key is more than one byte type");
                    }
                    UnserializeFromVector(s, m_tap_internal_key);
                    break;
                }
                case PSBT_IN_TAP_MERKLE_ROOT:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input Taproot merkle root already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Input Taproot merkle root key is more than one byte type");
                    }
                    UnserializeFromVector(s, m_tap_merkle_root);
                    break;
                }
                case PSBT_IN_PROPRIETARY:
                {
                    PSBTProprietary this_prop;
                    skey >> this_prop.identifier;
                    this_prop.subtype = ReadCompactSize(skey);
                    this_prop.key = key;
                    if (m_proprietary.count(this_prop) > 0) {
                        throw std::ios_base::failure("Duplicate Key, proprietary key already found");
                    }
                    s >> this_prop.value;
                    m_proprietary.insert(this_prop);
                    break;
                }
                default:
                    if (unknown.count(key) > 0) {
                        throw std::ios_base::failure("Duplicate Key, key for unknown value already provided");
                    }
                    std::vector<unsigned char> val_bytes;
                    s >> val_bytes;
                    unknown.emplace(std::move(key), std::move(val_bytes));
                    break;
            }
        }
        if (!found_sep) {
            throw std::ios_base::failure("Separator is missing at the end of an input map");
        }
    }
    template <typename Stream>
    PSBTInput(deserialize_type, Stream& s) {
        Unserialize(s);
    }
};
struct PSBTOutput
{
    CScript redeem_script;
    CScript witness_script;
    std::map<CPubKey, KeyOriginInfo> hd_keypaths;
    XOnlyPubKey m_tap_internal_key;
    std::optional<TaprootBuilder> m_tap_tree;
    std::map<XOnlyPubKey, std::pair<std::set<uint256>, KeyOriginInfo>> m_tap_bip32_paths;
    std::map<std::vector<unsigned char>, std::vector<unsigned char>> unknown;
    std::set<PSBTProprietary> m_proprietary;
    bool IsNull() const;
    void FillSignatureData(SignatureData& sigdata) const;
    void FromSignatureData(const SignatureData& sigdata);
    void Merge(const PSBTOutput& output);
    PSBTOutput() {}
    template <typename Stream>
    inline void Serialize(Stream& s) const {
        if (!redeem_script.empty()) {
            SerializeToVector(s, CompactSizeWriter(PSBT_OUT_REDEEMSCRIPT));
            s << redeem_script;
        }
        if (!witness_script.empty()) {
            SerializeToVector(s, CompactSizeWriter(PSBT_OUT_WITNESSSCRIPT));
            s << witness_script;
        }
        SerializeHDKeypaths(s, hd_keypaths, CompactSizeWriter(PSBT_OUT_BIP32_DERIVATION));
        for (const auto& entry : m_proprietary) {
            s << entry.key;
            s << entry.value;
        }
        if (!m_tap_internal_key.IsNull()) {
            SerializeToVector(s, PSBT_OUT_TAP_INTERNAL_KEY);
            s << ToByteVector(m_tap_internal_key);
        }
        if (m_tap_tree.has_value()) {
            SerializeToVector(s, PSBT_OUT_TAP_TREE);
            std::vector<unsigned char> value;
            CVectorWriter s_value(s.GetType(), s.GetVersion(), value, 0);
            const auto& tuples = m_tap_tree->GetTreeTuples();
            for (const auto& tuple : tuples) {
                uint8_t depth = std::get<0>(tuple);
                uint8_t leaf_ver = std::get<1>(tuple);
                CScript script = std::get<2>(tuple);
                s_value << depth;
                s_value << leaf_ver;
                s_value << script;
            }
            s << value;
        }
        for (const auto& [xonly, leaf] : m_tap_bip32_paths) {
            const auto& [leaf_hashes, origin] = leaf;
            SerializeToVector(s, PSBT_OUT_TAP_BIP32_DERIVATION, xonly);
            std::vector<unsigned char> value;
            CVectorWriter s_value(s.GetType(), s.GetVersion(), value, 0);
            s_value << leaf_hashes;
            SerializeKeyOrigin(s_value, origin);
            s << value;
        }
        for (auto& entry : unknown) {
            s << entry.first;
            s << entry.second;
        }
        s << PSBT_SEPARATOR;
    }
    template <typename Stream>
    inline void Unserialize(Stream& s) {
        std::set<std::vector<unsigned char>> key_lookup;
        bool found_sep = false;
        while(!s.empty()) {
            std::vector<unsigned char> key;
            s >> key;
            if (key.empty()) {
                found_sep = true;
                break;
            }
            SpanReader skey(s.GetType(), s.GetVersion(), key);
            uint64_t type = ReadCompactSize(skey);
            switch(type) {
                case PSBT_OUT_REDEEMSCRIPT:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, output redeemScript already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Output redeemScript key is more than one byte type");
                    }
                    s >> redeem_script;
                    break;
                }
                case PSBT_OUT_WITNESSSCRIPT:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, output witnessScript already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Output witnessScript key is more than one byte type");
                    }
                    s >> witness_script;
                    break;
                }
                case PSBT_OUT_BIP32_DERIVATION:
                {
                    DeserializeHDKeypaths(s, key, hd_keypaths);
                    break;
                }
                case PSBT_OUT_TAP_INTERNAL_KEY:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, output Taproot internal key already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Output Taproot internal key key is more than one byte type");
                    }
                    UnserializeFromVector(s, m_tap_internal_key);
                    break;
                }
                case PSBT_OUT_TAP_TREE:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, output Taproot tree already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Output Taproot tree key is more than one byte type");
                    }
                    m_tap_tree.emplace();
                    std::vector<unsigned char> tree_v;
                    s >> tree_v;
                    SpanReader s_tree(s.GetType(), s.GetVersion(), tree_v);
                    while (!s_tree.empty()) {
                        uint8_t depth;
                        uint8_t leaf_ver;
                        CScript script;
                        s_tree >> depth;
                        s_tree >> leaf_ver;
                        s_tree >> script;
                        if (depth > TAPROOT_CONTROL_MAX_NODE_COUNT) {
                            throw std::ios_base::failure("Output Taproot tree has as leaf greater than Taproot maximum depth");
                        }
                        if ((leaf_ver & ~TAPROOT_LEAF_MASK) != 0) {
                            throw std::ios_base::failure("Output Taproot tree has a leaf with an invalid leaf version");
                        }
                        m_tap_tree->Add((int)depth, script, (int)leaf_ver, true  );
                    }
                    if (!m_tap_tree->IsComplete()) {
                        throw std::ios_base::failure("Output Taproot tree is malformed");
                    }
                    break;
                }
                case PSBT_OUT_TAP_BIP32_DERIVATION:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, output Taproot BIP32 keypath already provided");
                    } else if (key.size() != 33) {
                        throw std::ios_base::failure("Output Taproot BIP32 keypath key is not at 33 bytes");
                    }
                    XOnlyPubKey xonly(uint256({key.begin() + 1, key.begin() + 33}));
                    std::set<uint256> leaf_hashes;
                    uint64_t value_len = ReadCompactSize(s);
                    size_t before_hashes = s.size();
                    s >> leaf_hashes;
                    size_t after_hashes = s.size();
                    size_t hashes_len = before_hashes - after_hashes;
                    if (hashes_len > value_len) {
                        throw std::ios_base::failure("Output Taproot BIP32 keypath has an invalid length");
                    }
                    size_t origin_len = value_len - hashes_len;
                    m_tap_bip32_paths.emplace(xonly, std::make_pair(leaf_hashes, DeserializeKeyOrigin(s, origin_len)));
                    break;
                }
                case PSBT_OUT_PROPRIETARY:
                {
                    PSBTProprietary this_prop;
                    skey >> this_prop.identifier;
                    this_prop.subtype = ReadCompactSize(skey);
                    this_prop.key = key;
                    if (m_proprietary.count(this_prop) > 0) {
                        throw std::ios_base::failure("Duplicate Key, proprietary key already found");
                    }
                    s >> this_prop.value;
                    m_proprietary.insert(this_prop);
                    break;
                }
                default: {
                    if (unknown.count(key) > 0) {
                        throw std::ios_base::failure("Duplicate Key, key for unknown value already provided");
                    }
                    std::vector<unsigned char> val_bytes;
                    s >> val_bytes;
                    unknown.emplace(std::move(key), std::move(val_bytes));
                    break;
                }
            }
        }
        if (m_tap_tree.has_value() && m_tap_tree->IsComplete() && m_tap_internal_key.IsFullyValid()) {
            m_tap_tree->Finalize(m_tap_internal_key);
        }
        if (!found_sep) {
            throw std::ios_base::failure("Separator is missing at the end of an output map");
        }
    }
    template <typename Stream>
    PSBTOutput(deserialize_type, Stream& s) {
        Unserialize(s);
    }
};
struct PartiallySignedTransaction
{
    std::optional<CMutableTransaction> tx;
    std::map<KeyOriginInfo, std::set<CExtPubKey>> m_xpubs;
    std::vector<PSBTInput> inputs;
    std::vector<PSBTOutput> outputs;
    std::map<std::vector<unsigned char>, std::vector<unsigned char>> unknown;
    std::optional<uint32_t> m_version;
    std::set<PSBTProprietary> m_proprietary;
    bool IsNull() const;
    uint32_t GetVersion() const;
    [[nodiscard]] bool Merge(const PartiallySignedTransaction& psbt);
    bool AddInput(const CTxIn& txin, PSBTInput& psbtin);
    bool AddOutput(const CTxOut& txout, const PSBTOutput& psbtout);
    PartiallySignedTransaction() {}
    explicit PartiallySignedTransaction(const CMutableTransaction& tx);
    bool GetInputUTXO(CTxOut& utxo, int input_index) const;
    template <typename Stream>
    inline void Serialize(Stream& s) const {
        s << PSBT_MAGIC_BYTES;
        SerializeToVector(s, CompactSizeWriter(PSBT_GLOBAL_UNSIGNED_TX));
        OverrideStream<Stream> os(&s, s.GetType(), s.GetVersion() | SERIALIZE_TRANSACTION_NO_WITNESS);
        SerializeToVector(os, *tx);
        for (const auto& xpub_pair : m_xpubs) {
            for (const auto& xpub : xpub_pair.second) {
                unsigned char ser_xpub[BIP32_EXTKEY_WITH_VERSION_SIZE];
                xpub.EncodeWithVersion(ser_xpub);
                SerializeToVector(s, PSBT_GLOBAL_XPUB, ser_xpub);
                SerializeHDKeypath(s, xpub_pair.first);
            }
        }
        if (GetVersion() > 0) {
            SerializeToVector(s, CompactSizeWriter(PSBT_GLOBAL_VERSION));
            SerializeToVector(s, *m_version);
        }
        for (const auto& entry : m_proprietary) {
            s << entry.key;
            s << entry.value;
        }
        for (auto& entry : unknown) {
            s << entry.first;
            s << entry.second;
        }
        s << PSBT_SEPARATOR;
        for (const PSBTInput& input : inputs) {
            s << input;
        }
        for (const PSBTOutput& output : outputs) {
            s << output;
        }
    }
    template <typename Stream>
    inline void Unserialize(Stream& s) {
        uint8_t magic[5];
        s >> magic;
        if (!std::equal(magic, magic + 5, PSBT_MAGIC_BYTES)) {
            throw std::ios_base::failure("Invalid PSBT magic bytes");
        }
        std::set<std::vector<unsigned char>> key_lookup;
        std::set<CExtPubKey> global_xpubs;
        bool found_sep = false;
        while(!s.empty()) {
            std::vector<unsigned char> key;
            s >> key;
            if (key.empty()) {
                found_sep = true;
                break;
            }
            SpanReader skey(s.GetType(), s.GetVersion(), key);
            uint64_t type = ReadCompactSize(skey);
            switch(type) {
                case PSBT_GLOBAL_UNSIGNED_TX:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, unsigned tx already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Global unsigned tx key is more than one byte type");
                    }
                    CMutableTransaction mtx;
                    OverrideStream<Stream> os(&s, s.GetType(), s.GetVersion() | SERIALIZE_TRANSACTION_NO_WITNESS);
                    UnserializeFromVector(os, mtx);
                    tx = std::move(mtx);
                    for (const CTxIn& txin : tx->vin) {
                        if (!txin.scriptSig.empty() || !txin.scriptWitness.IsNull()) {
                            throw std::ios_base::failure("Unsigned tx does not have empty scriptSigs and scriptWitnesses.");
                        }
                    }
                    break;
                }
                case PSBT_GLOBAL_XPUB:
                {
                    if (key.size() != BIP32_EXTKEY_WITH_VERSION_SIZE + 1) {
                        throw std::ios_base::failure("Size of key was not the expected size for the type global xpub");
                    }
                    CExtPubKey xpub;
                    xpub.DecodeWithVersion(&key.data()[1]);
                    if (!xpub.pubkey.IsFullyValid()) {
                       throw std::ios_base::failure("Invalid pubkey");
                    }
                    if (global_xpubs.count(xpub) > 0) {
                       throw std::ios_base::failure("Duplicate key, global xpub already provided");
                    }
                    global_xpubs.insert(xpub);
                    KeyOriginInfo keypath;
                    DeserializeHDKeypath(s, keypath);
                    if (m_xpubs.count(keypath) == 0) {
                        m_xpubs[keypath] = {xpub};
                    } else {
                        m_xpubs[keypath].insert(xpub);
                    }
                    break;
                }
                case PSBT_GLOBAL_VERSION:
                {
                    if (m_version) {
                        throw std::ios_base::failure("Duplicate Key, version already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Global version key is more than one byte type");
                    }
                    uint32_t v;
                    UnserializeFromVector(s, v);
                    m_version = v;
                    if (*m_version > PSBT_HIGHEST_VERSION) {
                        throw std::ios_base::failure("Unsupported version number");
                    }
                    break;
                }
                case PSBT_GLOBAL_PROPRIETARY:
                {
                    PSBTProprietary this_prop;
                    skey >> this_prop.identifier;
                    this_prop.subtype = ReadCompactSize(skey);
                    this_prop.key = key;
                    if (m_proprietary.count(this_prop) > 0) {
                        throw std::ios_base::failure("Duplicate Key, proprietary key already found");
                    }
                    s >> this_prop.value;
                    m_proprietary.insert(this_prop);
                    break;
                }
                default: {
                    if (unknown.count(key) > 0) {
                        throw std::ios_base::failure("Duplicate Key, key for unknown value already provided");
                    }
                    std::vector<unsigned char> val_bytes;
                    s >> val_bytes;
                    unknown.emplace(std::move(key), std::move(val_bytes));
                }
            }
        }
        if (!found_sep) {
            throw std::ios_base::failure("Separator is missing at the end of the global map");
        }
        if (!tx) {
            throw std::ios_base::failure("No unsigned transcation was provided");
        }
        unsigned int i = 0;
        while (!s.empty() && i < tx->vin.size()) {
            PSBTInput input;
            s >> input;
            inputs.push_back(input);
            if (input.non_witness_utxo && input.non_witness_utxo->GetHash() != tx->vin[i].prevout.hash) {
                throw std::ios_base::failure("Non-witness UTXO does not match outpoint hash");
            }
            ++i;
        }
        if (inputs.size() != tx->vin.size()) {
            throw std::ios_base::failure("Inputs provided does not match the number of inputs in transaction.");
        }
        i = 0;
        while (!s.empty() && i < tx->vout.size()) {
            PSBTOutput output;
            s >> output;
            outputs.push_back(output);
            ++i;
        }
        if (outputs.size() != tx->vout.size()) {
            throw std::ios_base::failure("Outputs provided does not match the number of outputs in transaction.");
        }
    }
    template <typename Stream>
    PartiallySignedTransaction(deserialize_type, Stream& s) {
        Unserialize(s);
    }
};
enum class PSBTRole {
    CREATOR,
    UPDATER,
    SIGNER,
    FINALIZER,
    EXTRACTOR
};
std::string PSBTRoleName(PSBTRole role);
PrecomputedTransactionData PrecomputePSBTData(const PartiallySignedTransaction& psbt);
bool PSBTInputSigned(const PSBTInput& input);
bool SignPSBTInput(const SigningProvider& provider, PartiallySignedTransaction& psbt, int index, const PrecomputedTransactionData* txdata, int sighash = SIGHASH_ALL, SignatureData* out_sigdata = nullptr, bool finalize = true);
size_t CountPSBTUnsignedInputs(const PartiallySignedTransaction& psbt);
void UpdatePSBTOutput(const SigningProvider& provider, PartiallySignedTransaction& psbt, int index);
bool FinalizePSBT(PartiallySignedTransaction& psbtx);
bool FinalizeAndExtractPSBT(PartiallySignedTransaction& psbtx, CMutableTransaction& result);
[[nodiscard]] TransactionError CombinePSBTs(PartiallySignedTransaction& out, const std::vector<PartiallySignedTransaction>& psbtxs);
[[nodiscard]] bool DecodeBase64PSBT(PartiallySignedTransaction& decoded_psbt, const std::string& base64_psbt, std::string& error);
[[nodiscard]] bool DecodeRawPSBT(PartiallySignedTransaction& decoded_psbt, Span<const std::byte> raw_psbt, std::string& error);
#endif
