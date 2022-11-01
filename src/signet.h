#ifndef LCRYP_SIGNET_H
#define LCRYP_SIGNET_H
#include <consensus/params.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <optional>
bool CheckSignetBlockSolution(const CBlock& block, const Consensus::Params& consensusParams);
class SignetTxs {
    template<class T1, class T2>
    SignetTxs(const T1& to_spend, const T2& to_sign) : m_to_spend{to_spend}, m_to_sign{to_sign} { }
public:
    static std::optional<SignetTxs> Create(const CBlock& block, const CScript& challenge);
    const CTransaction m_to_spend;
    const CTransaction m_to_sign;
};
#endif
