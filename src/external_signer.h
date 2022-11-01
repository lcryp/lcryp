#ifndef LCRYP_EXTERNAL_SIGNER_H
#define LCRYP_EXTERNAL_SIGNER_H
#include <univalue.h>
#include <util/system.h>
#include <string>
#include <vector>
struct PartiallySignedTransaction;
class ExternalSigner
{
private:
    std::string m_command;
    std::string m_chain;
    const std::string NetworkArg() const;
public:
    ExternalSigner(const std::string& command, const std::string chain, const std::string& fingerprint, const std::string name);
    std::string m_fingerprint;
    std::string m_name;
    static bool Enumerate(const std::string& command, std::vector<ExternalSigner>& signers, const std::string chain);
    UniValue DisplayAddress(const std::string& descriptor) const;
    UniValue GetDescriptors(const int account);
    bool SignTransaction(PartiallySignedTransaction& psbt, std::string& error);
};
#endif
