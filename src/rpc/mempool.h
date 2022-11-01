#ifndef LCRYP_RPC_MEMPOOL_H
#define LCRYP_RPC_MEMPOOL_H
class CTxMemPool;
class UniValue;
UniValue MempoolInfoToJSON(const CTxMemPool& pool);
UniValue MempoolToJSON(const CTxMemPool& pool, bool verbose = false, bool include_mempool_sequence = false);
#endif
