#include <chainparams.h>
#include <external_signer.h>
#include <wallet/external_signer_scriptpubkeyman.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
namespace wallet {
bool ExternalSignerScriptPubKeyMan::SetupDescriptor(std::unique_ptr<Descriptor> desc)
{
    LOCK(cs_desc_man);
    assert(m_storage.IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS));
    assert(m_storage.IsWalletFlagSet(WALLET_FLAG_EXTERNAL_SIGNER));
    int64_t creation_time = GetTime();
    WalletDescriptor w_desc(std::move(desc), creation_time, 0, 0, 0);
    m_wallet_descriptor = w_desc;
    WalletBatch batch(m_storage.GetDatabase());
    if (!batch.WriteDescriptor(GetID(), m_wallet_descriptor)) {
        throw std::runtime_error(std::string(__func__) + ": writing descriptor failed");
    }
    TopUp();
    m_storage.UnsetBlankWalletFlag(batch);
    return true;
}
ExternalSigner ExternalSignerScriptPubKeyMan::GetExternalSigner() {
    const std::string command = gArgs.GetArg("-signer", "");
    if (command == "") throw std::runtime_error(std::string(__func__) + ": restart lcrypd with -signer=<cmd>");
    std::vector<ExternalSigner> signers;
    ExternalSigner::Enumerate(command, signers, Params().NetworkIDString());
    if (signers.empty()) throw std::runtime_error(std::string(__func__) + ": No external signers found");
    if (signers.size() > 1) throw std::runtime_error(std::string(__func__) + ": More than one external signer found. Please connect only one at a time.");
    return signers[0];
}
bool ExternalSignerScriptPubKeyMan::DisplayAddress(const CScript scriptPubKey, const ExternalSigner &signer) const
{
    auto provider = GetSolvingProvider(scriptPubKey);
    auto descriptor = InferDescriptor(scriptPubKey, *provider);
    signer.DisplayAddress(descriptor->ToString());
    return true;
}
TransactionError ExternalSignerScriptPubKeyMan::FillPSBT(PartiallySignedTransaction& psbt, const PrecomputedTransactionData& txdata, int sighash_type, bool sign, bool bip32derivs, int* n_signed, bool finalize) const
{
    if (!sign) {
        return DescriptorScriptPubKeyMan::FillPSBT(psbt, txdata, sighash_type, false, bip32derivs, n_signed, finalize);
    }
    bool complete = true;
    for (const auto& input : psbt.inputs) {
        complete &= PSBTInputSigned(input);
    }
    if (complete) return TransactionError::OK;
    std::string strFailReason;
    if(!GetExternalSigner().SignTransaction(psbt, strFailReason)) {
        tfm::format(std::cerr, "Failed to sign: %s\n", strFailReason);
        return TransactionError::EXTERNAL_SIGNER_FAILED;
    }
    if (finalize) FinalizePSBT(psbt);
    return TransactionError::OK;
}
}
