#ifndef LCRYP_NODE_PSBT_H
#define LCRYP_NODE_PSBT_H
#include <psbt.h>
#include <optional>
namespace node {
struct PSBTInputAnalysis {
    bool has_utxo;
    bool is_final;
    PSBTRole next;
    std::vector<CKeyID> missing_pubkeys;
    std::vector<CKeyID> missing_sigs;
    uint160 missing_redeem_script;
    uint256 missing_witness_script;
};
struct PSBTAnalysis {
    std::optional<size_t> estimated_vsize;
    std::optional<CFeeRate> estimated_feerate;
    std::optional<CAmount> fee;
    std::vector<PSBTInputAnalysis> inputs;
    PSBTRole next;
    std::string error;
    void SetInvalid(std::string err_msg)
    {
        estimated_vsize = std::nullopt;
        estimated_feerate = std::nullopt;
        fee = std::nullopt;
        inputs.clear();
        next = PSBTRole::CREATOR;
        error = err_msg;
    }
};
PSBTAnalysis AnalyzePSBT(PartiallySignedTransaction psbtx);
}
#endif
