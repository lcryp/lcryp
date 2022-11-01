#ifndef LCRYP_WALLET_EXTERNAL_SIGNER_SCRIPTPUBKEYMAN_H
#define LCRYP_WALLET_EXTERNAL_SIGNER_SCRIPTPUBKEYMAN_H
#include <wallet/scriptpubkeyman.h>
#include <memory>
namespace wallet {
class ExternalSignerScriptPubKeyMan : public DescriptorScriptPubKeyMan
{
  public:
  ExternalSignerScriptPubKeyMan(WalletStorage& storage, WalletDescriptor& descriptor)
      :   DescriptorScriptPubKeyMan(storage, descriptor)
      {}
  ExternalSignerScriptPubKeyMan(WalletStorage& storage)
      :   DescriptorScriptPubKeyMan(storage)
      {}
  bool SetupDescriptor(std::unique_ptr<Descriptor>desc);
  static ExternalSigner GetExternalSigner();
  bool DisplayAddress(const CScript scriptPubKey, const ExternalSigner &signer) const;
  TransactionError FillPSBT(PartiallySignedTransaction& psbt, const PrecomputedTransactionData& txdata, int sighash_type = 1  , bool sign = true, bool bip32derivs = false, int* n_signed = nullptr, bool finalize = true) const override;
};
}
#endif
