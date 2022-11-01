#ifndef LCRYP_KERNEL_MEMPOOL_PERSIST_H
#define LCRYP_KERNEL_MEMPOOL_PERSIST_H
#include <fs.h>
class Chainstate;
class CTxMemPool;
namespace kernel {
bool DumpMempool(const CTxMemPool& pool, const fs::path& dump_path,
                 fsbridge::FopenFn mockable_fopen_function = fsbridge::fopen,
                 bool skip_file_commit = false);
bool LoadMempool(CTxMemPool& pool, const fs::path& load_path,
                 Chainstate& active_chainstate,
                 fsbridge::FopenFn mockable_fopen_function = fsbridge::fopen);
}
#endif
