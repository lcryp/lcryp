#ifndef LCRYP_INDEX_BASE_H
#define LCRYP_INDEX_BASE_H
#include <dbwrapper.h>
#include <interfaces/chain.h>
#include <threadinterrupt.h>
#include <validationinterface.h>
#include <string>
class CBlock;
class CBlockIndex;
class Chainstate;
namespace interfaces {
class Chain;
}
struct IndexSummary {
    std::string name;
    bool synced{false};
    int best_block_height{0};
};
class BaseIndex : public CValidationInterface
{
protected:
    class DB : public CDBWrapper
    {
    public:
        DB(const fs::path& path, size_t n_cache_size,
           bool f_memory = false, bool f_wipe = false, bool f_obfuscate = false);
        bool ReadBestBlock(CBlockLocator& locator) const;
        void WriteBestBlock(CDBBatch& batch, const CBlockLocator& locator);
    };
private:
    std::atomic<bool> m_synced{false};
    std::atomic<const CBlockIndex*> m_best_block_index{nullptr};
    std::thread m_thread_sync;
    CThreadInterrupt m_interrupt;
    bool Init();
    void ThreadSync();
    bool Commit();
    bool Rewind(const CBlockIndex* current_tip, const CBlockIndex* new_tip);
    virtual bool AllowPrune() const = 0;
protected:
    std::unique_ptr<interfaces::Chain> m_chain;
    Chainstate* m_chainstate{nullptr};
    const std::string m_name;
    void BlockConnected(const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex) override;
    void ChainStateFlushed(const CBlockLocator& locator) override;
    [[nodiscard]] virtual bool CustomInit(const std::optional<interfaces::BlockKey>& block) { return true; }
    [[nodiscard]] virtual bool CustomAppend(const interfaces::BlockInfo& block) { return true; }
    virtual bool CustomCommit(CDBBatch& batch) { return true; }
    [[nodiscard]] virtual bool CustomRewind(const interfaces::BlockKey& current_tip, const interfaces::BlockKey& new_tip) { return true; }
    virtual DB& GetDB() const = 0;
    const std::string& GetName() const LIFETIMEBOUND { return m_name; }
    void SetBestBlockIndex(const CBlockIndex* block);
public:
    BaseIndex(std::unique_ptr<interfaces::Chain> chain, std::string name);
    virtual ~BaseIndex();
    bool BlockUntilSyncedToCurrentChain() const LOCKS_EXCLUDED(::cs_main);
    void Interrupt();
    [[nodiscard]] bool Start();
    void Stop();
    IndexSummary GetSummary() const;
};
#endif
