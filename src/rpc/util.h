#ifndef LCRYP_RPC_UTIL_H
#define LCRYP_RPC_UTIL_H
#include <node/transaction.h>
#include <outputtype.h>
#include <protocol.h>
#include <pubkey.h>
#include <rpc/protocol.h>
#include <rpc/request.h>
#include <script/script.h>
#include <script/sign.h>
#include <script/standard.h>
#include <univalue.h>
#include <util/check.h>
#include <string>
#include <variant>
#include <vector>
static constexpr bool DEFAULT_RPC_DOC_CHECK{
#ifdef RPC_DOC_CHECK
    true
#else
    false
#endif
};
extern const std::string UNIX_EPOCH_TIME;
extern const std::string EXAMPLE_ADDRESS[2];
class FillableSigningProvider;
class CPubKey;
class CScript;
struct Sections;
std::string GetAllOutputTypes();
struct UniValueType {
    UniValueType(UniValue::VType _type) : typeAny(false), type(_type) {}
    UniValueType() : typeAny(true) {}
    bool typeAny;
    UniValue::VType type;
};
void RPCTypeCheck(const UniValue& params,
                  const std::list<UniValueType>& typesExpected, bool fAllowNull=false);
void RPCTypeCheckArgument(const UniValue& value, const UniValueType& typeExpected);
void RPCTypeCheckObj(const UniValue& o,
    const std::map<std::string, UniValueType>& typesExpected,
    bool fAllowNull = false,
    bool fStrict = false);
uint256 ParseHashV(const UniValue& v, std::string strName);
uint256 ParseHashO(const UniValue& o, std::string strKey);
std::vector<unsigned char> ParseHexV(const UniValue& v, std::string strName);
std::vector<unsigned char> ParseHexO(const UniValue& o, std::string strKey);
CAmount AmountFromValue(const UniValue& value, int decimals = 8);
using RPCArgList = std::vector<std::pair<std::string, UniValue>>;
std::string HelpExampleCli(const std::string& methodname, const std::string& args);
std::string HelpExampleCliNamed(const std::string& methodname, const RPCArgList& args);
std::string HelpExampleRpc(const std::string& methodname, const std::string& args);
std::string HelpExampleRpcNamed(const std::string& methodname, const RPCArgList& args);
CPubKey HexToPubKey(const std::string& hex_in);
CPubKey AddrToPubKey(const FillableSigningProvider& keystore, const std::string& addr_in);
CTxDestination AddAndGetMultisigDestination(const int required, const std::vector<CPubKey>& pubkeys, OutputType type, FillableSigningProvider& keystore, CScript& script_out);
UniValue DescribeAddress(const CTxDestination& dest);
unsigned int ParseConfirmTarget(const UniValue& value, unsigned int max_target);
RPCErrorCode RPCErrorFromTransactionError(TransactionError terr);
UniValue JSONRPCTransactionError(TransactionError terr, const std::string& err_string = "");
std::pair<int64_t, int64_t> ParseDescriptorRange(const UniValue& value);
std::vector<CScript> EvalDescriptorStringOrObject(const UniValue& scanobject, FlatSigningProvider& provider);
UniValue GetServicesNames(ServiceFlags services);
enum class OuterType {
    ARR,
    OBJ,
    NONE,
};
struct RPCArg {
    enum class Type {
        OBJ,
        ARR,
        STR,
        NUM,
        BOOL,
        OBJ_USER_KEYS,
        AMOUNT,
        STR_HEX,
        RANGE,
    };
    enum class Optional {
        NO,
        OMITTED_NAMED_ARG,
        OMITTED,
    };
    using DefaultHint = std::string;
    using Default = UniValue;
    using Fallback = std::variant<Optional,   DefaultHint,   Default>;
    const std::string m_names;
    const Type m_type;
    const bool m_hidden;
    const std::vector<RPCArg> m_inner;
    const Fallback m_fallback;
    const std::string m_description;
    const std::string m_oneline_description;
    const std::vector<std::string> m_type_str;
    RPCArg(
        const std::string name,
        const Type type,
        const Fallback fallback,
        const std::string description,
        const std::string oneline_description = "",
        const std::vector<std::string> type_str = {},
        const bool hidden = false)
        : m_names{std::move(name)},
          m_type{std::move(type)},
          m_hidden{hidden},
          m_fallback{std::move(fallback)},
          m_description{std::move(description)},
          m_oneline_description{std::move(oneline_description)},
          m_type_str{std::move(type_str)}
    {
        CHECK_NONFATAL(type != Type::ARR && type != Type::OBJ && type != Type::OBJ_USER_KEYS);
    }
    RPCArg(
        const std::string name,
        const Type type,
        const Fallback fallback,
        const std::string description,
        const std::vector<RPCArg> inner,
        const std::string oneline_description = "",
        const std::vector<std::string> type_str = {})
        : m_names{std::move(name)},
          m_type{std::move(type)},
          m_hidden{false},
          m_inner{std::move(inner)},
          m_fallback{std::move(fallback)},
          m_description{std::move(description)},
          m_oneline_description{std::move(oneline_description)},
          m_type_str{std::move(type_str)}
    {
        CHECK_NONFATAL(type == Type::ARR || type == Type::OBJ || type == Type::OBJ_USER_KEYS);
    }
    bool IsOptional() const;
    std::string GetFirstName() const;
    std::string GetName() const;
    std::string ToString(bool oneline) const;
    std::string ToStringObj(bool oneline) const;
    std::string ToDescriptionString() const;
};
struct RPCResult {
    enum class Type {
        OBJ,
        ARR,
        STR,
        NUM,
        BOOL,
        NONE,
        ANY,
        STR_AMOUNT,
        STR_HEX,
        OBJ_DYN,
        ARR_FIXED,
        NUM_TIME,
        ELISION,
    };
    const Type m_type;
    const std::string m_key_name;
    const std::vector<RPCResult> m_inner;
    const bool m_optional;
    const bool m_skip_type_check;
    const std::string m_description;
    const std::string m_cond;
    RPCResult(
        const std::string cond,
        const Type type,
        const std::string m_key_name,
        const bool optional,
        const std::string description,
        const std::vector<RPCResult> inner = {})
        : m_type{std::move(type)},
          m_key_name{std::move(m_key_name)},
          m_inner{std::move(inner)},
          m_optional{optional},
          m_skip_type_check{false},
          m_description{std::move(description)},
          m_cond{std::move(cond)}
    {
        CHECK_NONFATAL(!m_cond.empty());
        CheckInnerDoc();
    }
    RPCResult(
        const std::string cond,
        const Type type,
        const std::string m_key_name,
        const std::string description,
        const std::vector<RPCResult> inner = {})
        : RPCResult{cond, type, m_key_name, false, description, inner} {}
    RPCResult(
        const Type type,
        const std::string m_key_name,
        const bool optional,
        const std::string description,
        const std::vector<RPCResult> inner = {},
        bool skip_type_check = false)
        : m_type{std::move(type)},
          m_key_name{std::move(m_key_name)},
          m_inner{std::move(inner)},
          m_optional{optional},
          m_skip_type_check{skip_type_check},
          m_description{std::move(description)},
          m_cond{}
    {
        CheckInnerDoc();
    }
    RPCResult(
        const Type type,
        const std::string m_key_name,
        const std::string description,
        const std::vector<RPCResult> inner = {},
        bool skip_type_check = false)
        : RPCResult{type, m_key_name, false, description, inner, skip_type_check} {}
    void ToSections(Sections& sections, OuterType outer_type = OuterType::NONE, const int current_indent = 0) const;
    std::string ToStringObj() const;
    std::string ToDescriptionString() const;
    bool MatchesType(const UniValue& result) const;
private:
    void CheckInnerDoc() const;
};
struct RPCResults {
    const std::vector<RPCResult> m_results;
    RPCResults(RPCResult result)
        : m_results{{result}}
    {
    }
    RPCResults(std::initializer_list<RPCResult> results)
        : m_results{results}
    {
    }
    std::string ToDescriptionString() const;
};
struct RPCExamples {
    const std::string m_examples;
    explicit RPCExamples(
        std::string examples)
        : m_examples(std::move(examples))
    {
    }
    std::string ToDescriptionString() const;
};
class RPCHelpMan
{
public:
    RPCHelpMan(std::string name, std::string description, std::vector<RPCArg> args, RPCResults results, RPCExamples examples);
    using RPCMethodImpl = std::function<UniValue(const RPCHelpMan&, const JSONRPCRequest&)>;
    RPCHelpMan(std::string name, std::string description, std::vector<RPCArg> args, RPCResults results, RPCExamples examples, RPCMethodImpl fun);
    UniValue HandleRequest(const JSONRPCRequest& request) const;
    std::string ToString() const;
    UniValue GetArgMap() const;
    bool IsValidNumArgs(size_t num_args) const;
    std::vector<std::string> GetArgNames() const;
    const std::string m_name;
private:
    const RPCMethodImpl m_fun;
    const std::string m_description;
    const std::vector<RPCArg> m_args;
    const RPCResults m_results;
    const RPCExamples m_examples;
};
#endif
