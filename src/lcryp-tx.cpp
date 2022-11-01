#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <clientversion.h>
#include <coins.h>
#include <compat/compat.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <core_io.h>
#include <key_io.h>
#include <fs.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <univalue.h>
#include <util/moneystr.h>
#include <util/rbf.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/system.h>
#include <util/translation.h>
#include <cstdio>
#include <functional>
#include <memory>
static bool fCreateBlank;
static std::map<std::string,UniValue> registers;
static const int CONTINUE_EXECUTION=-1;
const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;
static void SetupLcRypTxArgs(ArgsManager &argsman)
{
    SetupHelpOptions(argsman);
    argsman.AddArg("-version", "Print version and exit", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-create", "Create new, empty TX.", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-json", "Select JSON output", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-txid", "Output only the hex-encoded transaction id of the resultant transaction.", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    SetupChainParamsBaseOptions(argsman);
    argsman.AddArg("delin=N", "Delete input N from TX", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);
    argsman.AddArg("delout=N", "Delete output N from TX", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);
    argsman.AddArg("in=TXID:VOUT(:SEQUENCE_NUMBER)", "Add input to TX", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);
    argsman.AddArg("locktime=N", "Set TX lock time to N", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);
    argsman.AddArg("nversion=N", "Set TX version to N", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);
    argsman.AddArg("outaddr=VALUE:ADDRESS", "Add address-based output to TX", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);
    argsman.AddArg("outdata=[VALUE:]DATA", "Add data-based output to TX", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);
    argsman.AddArg("outmultisig=VALUE:REQUIRED:PUBKEYS:PUBKEY1:PUBKEY2:....[:FLAGS]", "Add Pay To n-of-m Multi-sig output to TX. n = REQUIRED, m = PUBKEYS. "
        "Optionally add the \"W\" flag to produce a pay-to-witness-script-hash output. "
        "Optionally add the \"S\" flag to wrap the output in a pay-to-script-hash.", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);
    argsman.AddArg("outpubkey=VALUE:PUBKEY[:FLAGS]", "Add pay-to-pubkey output to TX. "
        "Optionally add the \"W\" flag to produce a pay-to-witness-pubkey-hash output. "
        "Optionally add the \"S\" flag to wrap the output in a pay-to-script-hash.", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);
    argsman.AddArg("outscript=VALUE:SCRIPT[:FLAGS]", "Add raw script output to TX. "
        "Optionally add the \"W\" flag to produce a pay-to-witness-script-hash output. "
        "Optionally add the \"S\" flag to wrap the output in a pay-to-script-hash.", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);
    argsman.AddArg("replaceable(=N)", "Set RBF opt-in sequence number for input N (if not provided, opt-in all available inputs)", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);
    argsman.AddArg("sign=SIGHASH-FLAGS", "Add zero or more signatures to transaction. "
        "This command requires JSON registers:"
        "prevtxs=JSON object, "
        "privatekeys=JSON object. "
        "See signrawtransactionwithkey docs for format of sighash flags, JSON objects.", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);
    argsman.AddArg("load=NAME:FILENAME", "Load JSON file FILENAME into register NAME", ArgsManager::ALLOW_ANY, OptionsCategory::REGISTER_COMMANDS);
    argsman.AddArg("set=NAME:JSON-STRING", "Set register NAME to given JSON-STRING", ArgsManager::ALLOW_ANY, OptionsCategory::REGISTER_COMMANDS);
}
static int AppInitRawTx(int argc, char* argv[])
{
    SetupLcRypTxArgs(gArgs);
    std::string error;
    if (!gArgs.ParseParameters(argc, argv, error)) {
        tfm::format(std::cerr, "Error parsing command line arguments: %s\n", error);
        return EXIT_FAILURE;
    }
    try {
        SelectParams(gArgs.GetChainName());
    } catch (const std::exception& e) {
        tfm::format(std::cerr, "Error: %s\n", e.what());
        return EXIT_FAILURE;
    }
    fCreateBlank = gArgs.GetBoolArg("-create", false);
    if (argc < 2 || HelpRequested(gArgs) || gArgs.IsArgSet("-version")) {
        std::string strUsage = PACKAGE_NAME " lcryp-tx utility version " + FormatFullVersion() + "\n";
        if (gArgs.IsArgSet("-version")) {
            strUsage += FormatParagraph(LicenseInfo());
        } else {
            strUsage += "\n"
                "Usage:  lcryp-tx [options] <hex-tx> [commands]  Update hex-encoded lcryp transaction\n"
                "or:     lcryp-tx [options] -create [commands]   Create hex-encoded lcryp transaction\n"
                "\n";
            strUsage += gArgs.GetHelpMessage();
        }
        tfm::format(std::cout, "%s", strUsage);
        if (argc < 2) {
            tfm::format(std::cerr, "Error: too few parameters\n");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    return CONTINUE_EXECUTION;
}
static void RegisterSetJson(const std::string& key, const std::string& rawJson)
{
    UniValue val;
    if (!val.read(rawJson)) {
        std::string strErr = "Cannot parse JSON for key " + key;
        throw std::runtime_error(strErr);
    }
    registers[key] = val;
}
static void RegisterSet(const std::string& strInput)
{
    size_t pos = strInput.find(':');
    if ((pos == std::string::npos) ||
        (pos == 0) ||
        (pos == (strInput.size() - 1)))
        throw std::runtime_error("Register input requires NAME:VALUE");
    std::string key = strInput.substr(0, pos);
    std::string valStr = strInput.substr(pos + 1, std::string::npos);
    RegisterSetJson(key, valStr);
}
static void RegisterLoad(const std::string& strInput)
{
    size_t pos = strInput.find(':');
    if ((pos == std::string::npos) ||
        (pos == 0) ||
        (pos == (strInput.size() - 1)))
        throw std::runtime_error("Register load requires NAME:FILENAME");
    std::string key = strInput.substr(0, pos);
    std::string filename = strInput.substr(pos + 1, std::string::npos);
    FILE *f = fsbridge::fopen(filename.c_str(), "r");
    if (!f) {
        std::string strErr = "Cannot open file " + filename;
        throw std::runtime_error(strErr);
    }
    std::string valStr;
    while ((!feof(f)) && (!ferror(f))) {
        char buf[4096];
        int bread = fread(buf, 1, sizeof(buf), f);
        if (bread <= 0)
            break;
        valStr.insert(valStr.size(), buf, bread);
    }
    int error = ferror(f);
    fclose(f);
    if (error) {
        std::string strErr = "Error reading file " + filename;
        throw std::runtime_error(strErr);
    }
    RegisterSetJson(key, valStr);
}
static CAmount ExtractAndValidateValue(const std::string& strValue)
{
    if (std::optional<CAmount> parsed = ParseMoney(strValue)) {
        return parsed.value();
    } else {
        throw std::runtime_error("invalid TX output value");
    }
}
static void MutateTxVersion(CMutableTransaction& tx, const std::string& cmdVal)
{
    int64_t newVersion;
    if (!ParseInt64(cmdVal, &newVersion) || newVersion < 1 || newVersion > TX_MAX_STANDARD_VERSION) {
        throw std::runtime_error("Invalid TX version requested: '" + cmdVal + "'");
    }
    tx.nVersion = (int) newVersion;
}
static void MutateTxLocktime(CMutableTransaction& tx, const std::string& cmdVal)
{
    int64_t newLocktime;
    if (!ParseInt64(cmdVal, &newLocktime) || newLocktime < 0LL || newLocktime > 0xffffffffLL)
        throw std::runtime_error("Invalid TX locktime requested: '" + cmdVal + "'");
    tx.nLockTime = (unsigned int) newLocktime;
}
static void MutateTxRBFOptIn(CMutableTransaction& tx, const std::string& strInIdx)
{
    int64_t inIdx;
    if (!ParseInt64(strInIdx, &inIdx) || inIdx < 0 || inIdx >= static_cast<int64_t>(tx.vin.size())) {
        throw std::runtime_error("Invalid TX input index '" + strInIdx + "'");
    }
    int cnt = 0;
    for (CTxIn& txin : tx.vin) {
        if (strInIdx == "" || cnt == inIdx) {
            if (txin.nSequence > MAX_BIP125_RBF_SEQUENCE) {
                txin.nSequence = MAX_BIP125_RBF_SEQUENCE;
            }
        }
        ++cnt;
    }
}
template <typename T>
static T TrimAndParse(const std::string& int_str, const std::string& err)
{
    const auto parsed{ToIntegral<T>(TrimStringView(int_str))};
    if (!parsed.has_value()) {
        throw std::runtime_error(err + " '" + int_str + "'");
    }
    return parsed.value();
}
static void MutateTxAddInput(CMutableTransaction& tx, const std::string& strInput)
{
    std::vector<std::string> vStrInputParts = SplitString(strInput, ':');
    if (vStrInputParts.size()<2)
        throw std::runtime_error("TX input missing separator");
    uint256 txid;
    if (!ParseHashStr(vStrInputParts[0], txid)) {
        throw std::runtime_error("invalid TX input txid");
    }
    static const unsigned int minTxOutSz = 9;
    static const unsigned int maxVout = MAX_BLOCK_WEIGHT / (WITNESS_SCALE_FACTOR * minTxOutSz);
    const std::string& strVout = vStrInputParts[1];
    int64_t vout;
    if (!ParseInt64(strVout, &vout) || vout < 0 || vout > static_cast<int64_t>(maxVout))
        throw std::runtime_error("invalid TX input vout '" + strVout + "'");
    uint32_t nSequenceIn = CTxIn::SEQUENCE_FINAL;
    if (vStrInputParts.size() > 2) {
        nSequenceIn = TrimAndParse<uint32_t>(vStrInputParts.at(2), "invalid TX sequence id");
    }
    CTxIn txin(txid, vout, CScript(), nSequenceIn);
    tx.vin.push_back(txin);
}
static void MutateTxAddOutAddr(CMutableTransaction& tx, const std::string& strInput)
{
    std::vector<std::string> vStrInputParts = SplitString(strInput, ':');
    if (vStrInputParts.size() != 2)
        throw std::runtime_error("TX output missing or too many separators");
    CAmount value = ExtractAndValidateValue(vStrInputParts[0]);
    std::string strAddr = vStrInputParts[1];
    CTxDestination destination = DecodeDestination(strAddr);
    if (!IsValidDestination(destination)) {
        throw std::runtime_error("invalid TX output address");
    }
    CScript scriptPubKey = GetScriptForDestination(destination);
    CTxOut txout(value, scriptPubKey);
    tx.vout.push_back(txout);
}
static void MutateTxAddOutPubKey(CMutableTransaction& tx, const std::string& strInput)
{
    std::vector<std::string> vStrInputParts = SplitString(strInput, ':');
    if (vStrInputParts.size() < 2 || vStrInputParts.size() > 3)
        throw std::runtime_error("TX output missing or too many separators");
    CAmount value = ExtractAndValidateValue(vStrInputParts[0]);
    CPubKey pubkey(ParseHex(vStrInputParts[1]));
    if (!pubkey.IsFullyValid())
        throw std::runtime_error("invalid TX output pubkey");
    CScript scriptPubKey = GetScriptForRawPubKey(pubkey);
    bool bSegWit = false;
    bool bScriptHash = false;
    if (vStrInputParts.size() == 3) {
        std::string flags = vStrInputParts[2];
        bSegWit = (flags.find('W') != std::string::npos);
        bScriptHash = (flags.find('S') != std::string::npos);
    }
    if (bSegWit) {
        if (!pubkey.IsCompressed()) {
            throw std::runtime_error("Uncompressed pubkeys are not useable for SegWit outputs");
        }
        scriptPubKey = GetScriptForDestination(WitnessV0KeyHash(pubkey));
    }
    if (bScriptHash) {
        scriptPubKey = GetScriptForDestination(ScriptHash(scriptPubKey));
    }
    CTxOut txout(value, scriptPubKey);
    tx.vout.push_back(txout);
}
static void MutateTxAddOutMultiSig(CMutableTransaction& tx, const std::string& strInput)
{
    std::vector<std::string> vStrInputParts = SplitString(strInput, ':');
    if (vStrInputParts.size()<3)
        throw std::runtime_error("Not enough multisig parameters");
    CAmount value = ExtractAndValidateValue(vStrInputParts[0]);
    const uint32_t required{TrimAndParse<uint32_t>(vStrInputParts.at(1), "invalid multisig required number")};
    const uint32_t numkeys{TrimAndParse<uint32_t>(vStrInputParts.at(2), "invalid multisig total number")};
    if (vStrInputParts.size() < numkeys + 3)
        throw std::runtime_error("incorrect number of multisig pubkeys");
    if (required < 1 || required > MAX_PUBKEYS_PER_MULTISIG || numkeys < 1 || numkeys > MAX_PUBKEYS_PER_MULTISIG || numkeys < required)
        throw std::runtime_error("multisig parameter mismatch. Required " \
                            + ToString(required) + " of " + ToString(numkeys) + "signatures.");
    std::vector<CPubKey> pubkeys;
    for(int pos = 1; pos <= int(numkeys); pos++) {
        CPubKey pubkey(ParseHex(vStrInputParts[pos + 2]));
        if (!pubkey.IsFullyValid())
            throw std::runtime_error("invalid TX output pubkey");
        pubkeys.push_back(pubkey);
    }
    bool bSegWit = false;
    bool bScriptHash = false;
    if (vStrInputParts.size() == numkeys + 4) {
        std::string flags = vStrInputParts.back();
        bSegWit = (flags.find('W') != std::string::npos);
        bScriptHash = (flags.find('S') != std::string::npos);
    }
    else if (vStrInputParts.size() > numkeys + 4) {
        throw std::runtime_error("Too many parameters");
    }
    CScript scriptPubKey = GetScriptForMultisig(required, pubkeys);
    if (bSegWit) {
        for (const CPubKey& pubkey : pubkeys) {
            if (!pubkey.IsCompressed()) {
                throw std::runtime_error("Uncompressed pubkeys are not useable for SegWit outputs");
            }
        }
        scriptPubKey = GetScriptForDestination(WitnessV0ScriptHash(scriptPubKey));
    }
    if (bScriptHash) {
        if (scriptPubKey.size() > MAX_SCRIPT_ELEMENT_SIZE) {
            throw std::runtime_error(strprintf(
                        "redeemScript exceeds size limit: %d > %d", scriptPubKey.size(), MAX_SCRIPT_ELEMENT_SIZE));
        }
        scriptPubKey = GetScriptForDestination(ScriptHash(scriptPubKey));
    }
    CTxOut txout(value, scriptPubKey);
    tx.vout.push_back(txout);
}
static void MutateTxAddOutData(CMutableTransaction& tx, const std::string& strInput)
{
    CAmount value = 0;
    size_t pos = strInput.find(':');
    if (pos==0)
        throw std::runtime_error("TX output value not specified");
    if (pos == std::string::npos) {
        pos = 0;
    } else {
        value = ExtractAndValidateValue(strInput.substr(0, pos));
        ++pos;
    }
    const std::string strData{strInput.substr(pos, std::string::npos)};
    if (!IsHex(strData))
        throw std::runtime_error("invalid TX output data");
    std::vector<unsigned char> data = ParseHex(strData);
    CTxOut txout(value, CScript() << OP_RETURN << data);
    tx.vout.push_back(txout);
}
static void MutateTxAddOutScript(CMutableTransaction& tx, const std::string& strInput)
{
    std::vector<std::string> vStrInputParts = SplitString(strInput, ':');
    if (vStrInputParts.size() < 2)
        throw std::runtime_error("TX output missing separator");
    CAmount value = ExtractAndValidateValue(vStrInputParts[0]);
    std::string strScript = vStrInputParts[1];
    CScript scriptPubKey = ParseScript(strScript);
    bool bSegWit = false;
    bool bScriptHash = false;
    if (vStrInputParts.size() == 3) {
        std::string flags = vStrInputParts.back();
        bSegWit = (flags.find('W') != std::string::npos);
        bScriptHash = (flags.find('S') != std::string::npos);
    }
    if (scriptPubKey.size() > MAX_SCRIPT_SIZE) {
        throw std::runtime_error(strprintf(
                    "script exceeds size limit: %d > %d", scriptPubKey.size(), MAX_SCRIPT_SIZE));
    }
    if (bSegWit) {
        scriptPubKey = GetScriptForDestination(WitnessV0ScriptHash(scriptPubKey));
    }
    if (bScriptHash) {
        if (scriptPubKey.size() > MAX_SCRIPT_ELEMENT_SIZE) {
            throw std::runtime_error(strprintf(
                        "redeemScript exceeds size limit: %d > %d", scriptPubKey.size(), MAX_SCRIPT_ELEMENT_SIZE));
        }
        scriptPubKey = GetScriptForDestination(ScriptHash(scriptPubKey));
    }
    CTxOut txout(value, scriptPubKey);
    tx.vout.push_back(txout);
}
static void MutateTxDelInput(CMutableTransaction& tx, const std::string& strInIdx)
{
    int64_t inIdx;
    if (!ParseInt64(strInIdx, &inIdx) || inIdx < 0 || inIdx >= static_cast<int64_t>(tx.vin.size())) {
        throw std::runtime_error("Invalid TX input index '" + strInIdx + "'");
    }
    tx.vin.erase(tx.vin.begin() + inIdx);
}
static void MutateTxDelOutput(CMutableTransaction& tx, const std::string& strOutIdx)
{
    int64_t outIdx;
    if (!ParseInt64(strOutIdx, &outIdx) || outIdx < 0 || outIdx >= static_cast<int64_t>(tx.vout.size())) {
        throw std::runtime_error("Invalid TX output index '" + strOutIdx + "'");
    }
    tx.vout.erase(tx.vout.begin() + outIdx);
}
static const unsigned int N_SIGHASH_OPTS = 7;
static const struct {
    const char *flagStr;
    int flags;
} sighashOptions[N_SIGHASH_OPTS] = {
    {"DEFAULT", SIGHASH_DEFAULT},
    {"ALL", SIGHASH_ALL},
    {"NONE", SIGHASH_NONE},
    {"SINGLE", SIGHASH_SINGLE},
    {"ALL|ANYONECANPAY", SIGHASH_ALL|SIGHASH_ANYONECANPAY},
    {"NONE|ANYONECANPAY", SIGHASH_NONE|SIGHASH_ANYONECANPAY},
    {"SINGLE|ANYONECANPAY", SIGHASH_SINGLE|SIGHASH_ANYONECANPAY},
};
static bool findSighashFlags(int& flags, const std::string& flagStr)
{
    flags = 0;
    for (unsigned int i = 0; i < N_SIGHASH_OPTS; i++) {
        if (flagStr == sighashOptions[i].flagStr) {
            flags = sighashOptions[i].flags;
            return true;
        }
    }
    return false;
}
static CAmount AmountFromValue(const UniValue& value)
{
    if (!value.isNum() && !value.isStr())
        throw std::runtime_error("Amount is not a number or string");
    CAmount amount;
    if (!ParseFixedPoint(value.getValStr(), 8, &amount))
        throw std::runtime_error("Invalid amount");
    if (!MoneyRange(amount))
        throw std::runtime_error("Amount out of range");
    return amount;
}
static void MutateTxSign(CMutableTransaction& tx, const std::string& flagStr)
{
    int nHashType = SIGHASH_ALL;
    if (flagStr.size() > 0)
        if (!findSighashFlags(nHashType, flagStr))
            throw std::runtime_error("unknown sighash flag/sign option");
    CMutableTransaction mergedTx{tx};
    const CMutableTransaction txv{tx};
    CCoinsView viewDummy;
    CCoinsViewCache view(&viewDummy);
    if (!registers.count("privatekeys"))
        throw std::runtime_error("privatekeys register variable must be set.");
    FillableSigningProvider tempKeystore;
    UniValue keysObj = registers["privatekeys"];
    for (unsigned int kidx = 0; kidx < keysObj.size(); kidx++) {
        if (!keysObj[kidx].isStr())
            throw std::runtime_error("privatekey not a std::string");
        CKey key = DecodeSecret(keysObj[kidx].getValStr());
        if (!key.IsValid()) {
            throw std::runtime_error("privatekey not valid");
        }
        tempKeystore.AddKey(key);
    }
    if (!registers.count("prevtxs"))
        throw std::runtime_error("prevtxs register variable must be set.");
    UniValue prevtxsObj = registers["prevtxs"];
    {
        for (unsigned int previdx = 0; previdx < prevtxsObj.size(); previdx++) {
            const UniValue& prevOut = prevtxsObj[previdx];
            if (!prevOut.isObject())
                throw std::runtime_error("expected prevtxs internal object");
            std::map<std::string, UniValue::VType> types = {
                {"txid", UniValue::VSTR},
                {"vout", UniValue::VNUM},
                {"scriptPubKey", UniValue::VSTR},
            };
            if (!prevOut.checkObject(types))
                throw std::runtime_error("prevtxs internal object typecheck fail");
            uint256 txid;
            if (!ParseHashStr(prevOut["txid"].get_str(), txid)) {
                throw std::runtime_error("txid must be hexadecimal string (not '" + prevOut["txid"].get_str() + "')");
            }
            const int nOut = prevOut["vout"].getInt<int>();
            if (nOut < 0)
                throw std::runtime_error("vout cannot be negative");
            COutPoint out(txid, nOut);
            std::vector<unsigned char> pkData(ParseHexUV(prevOut["scriptPubKey"], "scriptPubKey"));
            CScript scriptPubKey(pkData.begin(), pkData.end());
            {
                const Coin& coin = view.AccessCoin(out);
                if (!coin.IsSpent() && coin.out.scriptPubKey != scriptPubKey) {
                    std::string err("Previous output scriptPubKey mismatch:\n");
                    err = err + ScriptToAsmStr(coin.out.scriptPubKey) + "\nvs:\n"+
                        ScriptToAsmStr(scriptPubKey);
                    throw std::runtime_error(err);
                }
                Coin newcoin;
                newcoin.out.scriptPubKey = scriptPubKey;
                newcoin.out.nValue = MAX_MONEY;
                if (prevOut.exists("amount")) {
                    newcoin.out.nValue = AmountFromValue(prevOut["amount"]);
                }
                newcoin.nHeight = 1;
                view.AddCoin(out, std::move(newcoin), true);
            }
            if ((scriptPubKey.IsPayToScriptHash() || scriptPubKey.IsPayToWitnessScriptHash()) &&
                prevOut.exists("redeemScript")) {
                UniValue v = prevOut["redeemScript"];
                std::vector<unsigned char> rsData(ParseHexUV(v, "redeemScript"));
                CScript redeemScript(rsData.begin(), rsData.end());
                tempKeystore.AddCScript(redeemScript);
            }
        }
    }
    const FillableSigningProvider& keystore = tempKeystore;
    bool fHashSingle = ((nHashType & ~SIGHASH_ANYONECANPAY) == SIGHASH_SINGLE);
    for (unsigned int i = 0; i < mergedTx.vin.size(); i++) {
        CTxIn& txin = mergedTx.vin[i];
        const Coin& coin = view.AccessCoin(txin.prevout);
        if (coin.IsSpent()) {
            continue;
        }
        const CScript& prevPubKey = coin.out.scriptPubKey;
        const CAmount& amount = coin.out.nValue;
        SignatureData sigdata = DataFromTransaction(mergedTx, i, coin.out);
        if (!fHashSingle || (i < mergedTx.vout.size()))
            ProduceSignature(keystore, MutableTransactionSignatureCreator(mergedTx, i, amount, nHashType), prevPubKey, sigdata);
        if (amount == MAX_MONEY && !sigdata.scriptWitness.IsNull()) {
            throw std::runtime_error(strprintf("Missing amount for CTxOut with scriptPubKey=%s", HexStr(prevPubKey)));
        }
        UpdateInput(txin, sigdata);
    }
    tx = mergedTx;
}
class Secp256k1Init
{
    ECCVerifyHandle globalVerifyHandle;
public:
    Secp256k1Init() {
        ECC_Start();
    }
    ~Secp256k1Init() {
        ECC_Stop();
    }
};
static void MutateTx(CMutableTransaction& tx, const std::string& command,
                     const std::string& commandVal)
{
    std::unique_ptr<Secp256k1Init> ecc;
    if (command == "nversion")
        MutateTxVersion(tx, commandVal);
    else if (command == "locktime")
        MutateTxLocktime(tx, commandVal);
    else if (command == "replaceable") {
        MutateTxRBFOptIn(tx, commandVal);
    }
    else if (command == "delin")
        MutateTxDelInput(tx, commandVal);
    else if (command == "in")
        MutateTxAddInput(tx, commandVal);
    else if (command == "delout")
        MutateTxDelOutput(tx, commandVal);
    else if (command == "outaddr")
        MutateTxAddOutAddr(tx, commandVal);
    else if (command == "outpubkey") {
        ecc.reset(new Secp256k1Init());
        MutateTxAddOutPubKey(tx, commandVal);
    } else if (command == "outmultisig") {
        ecc.reset(new Secp256k1Init());
        MutateTxAddOutMultiSig(tx, commandVal);
    } else if (command == "outscript")
        MutateTxAddOutScript(tx, commandVal);
    else if (command == "outdata")
        MutateTxAddOutData(tx, commandVal);
    else if (command == "sign") {
        ecc.reset(new Secp256k1Init());
        MutateTxSign(tx, commandVal);
    }
    else if (command == "load")
        RegisterLoad(commandVal);
    else if (command == "set")
        RegisterSet(commandVal);
    else
        throw std::runtime_error("unknown command");
}
static void OutputTxJSON(const CTransaction& tx)
{
    UniValue entry(UniValue::VOBJ);
    TxToUniv(tx,  uint256(), entry);
    std::string jsonOutput = entry.write(4);
    tfm::format(std::cout, "%s\n", jsonOutput);
}
static void OutputTxHash(const CTransaction& tx)
{
    std::string strHexHash = tx.GetHash().GetHex();
    tfm::format(std::cout, "%s\n", strHexHash);
}
static void OutputTxHex(const CTransaction& tx)
{
    std::string strHex = EncodeHexTx(tx);
    tfm::format(std::cout, "%s\n", strHex);
}
static void OutputTx(const CTransaction& tx)
{
    if (gArgs.GetBoolArg("-json", false))
        OutputTxJSON(tx);
    else if (gArgs.GetBoolArg("-txid", false))
        OutputTxHash(tx);
    else
        OutputTxHex(tx);
}
static std::string readStdin()
{
    char buf[4096];
    std::string ret;
    while (!feof(stdin)) {
        size_t bread = fread(buf, 1, sizeof(buf), stdin);
        ret.append(buf, bread);
        if (bread < sizeof(buf))
            break;
    }
    if (ferror(stdin))
        throw std::runtime_error("error reading stdin");
    return TrimString(ret);
}
static int CommandLineRawTx(int argc, char* argv[])
{
    std::string strPrint;
    int nRet = 0;
    try {
        while (argc > 1 && IsSwitchChar(argv[1][0]) &&
               (argv[1][1] != 0)) {
            argc--;
            argv++;
        }
        CMutableTransaction tx;
        int startArg;
        if (!fCreateBlank) {
            if (argc < 2)
                throw std::runtime_error("too few parameters");
            std::string strHexTx(argv[1]);
            if (strHexTx == "-")
                strHexTx = readStdin();
            if (!DecodeHexTx(tx, strHexTx, true))
                throw std::runtime_error("invalid transaction encoding");
            startArg = 2;
        } else
            startArg = 1;
        for (int i = startArg; i < argc; i++) {
            std::string arg = argv[i];
            std::string key, value;
            size_t eqpos = arg.find('=');
            if (eqpos == std::string::npos)
                key = arg;
            else {
                key = arg.substr(0, eqpos);
                value = arg.substr(eqpos + 1);
            }
            MutateTx(tx, key, value);
        }
        OutputTx(CTransaction(tx));
    }
    catch (const std::exception& e) {
        strPrint = std::string("error: ") + e.what();
        nRet = EXIT_FAILURE;
    }
    catch (...) {
        PrintExceptionContinue(nullptr, "CommandLineRawTx()");
        throw;
    }
    if (strPrint != "") {
        tfm::format(nRet == 0 ? std::cout : std::cerr, "%s\n", strPrint);
    }
    return nRet;
}
MAIN_FUNCTION
{
    SetupEnvironment();
    try {
        int ret = AppInitRawTx(argc, argv);
        if (ret != CONTINUE_EXECUTION)
            return ret;
    }
    catch (const std::exception& e) {
        PrintExceptionContinue(&e, "AppInitRawTx()");
        return EXIT_FAILURE;
    } catch (...) {
        PrintExceptionContinue(nullptr, "AppInitRawTx()");
        return EXIT_FAILURE;
    }
    int ret = EXIT_FAILURE;
    try {
        ret = CommandLineRawTx(argc, argv);
    }
    catch (const std::exception& e) {
        PrintExceptionContinue(&e, "CommandLineRawTx()");
    } catch (...) {
        PrintExceptionContinue(nullptr, "CommandLineRawTx()");
    }
    return ret;
}
