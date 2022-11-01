#include <iostream>
#include <assert.h>
#include <chainparams.h>
#include <chainparamsseeds.h>
#include <consensus/merkle.h>
#include <deploymentinfo.h>
#include <hash.h>
#include <script/interpreter.h>
#include <util/string.h>
#include <util/system.h>
#include <stdlib.h>
#include <fstream>
#include <pow.h>
#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>

bool flag_param = false;
std::string is_show = "false";
uint32_t min_nonce = 0;
uint32_t max_nonce = 4294967295;
std::string FILE_NAME = "generate_genesis";

#define POW_LIMIT "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"

#define MAIN_GENESIS_BLOCK_BIT 0x207fffff

#define MAIN_GENESIS_BLOCK_NONCE 0
#define STR_PUBLIC_KEY "044f49c91ad302c63a03edcba9485ce4f3f2230b0a4393250b7de5781e6c694fd4547c12576adb8d38095fde59ccce2294c26d0d1981e586ac67976bdf9736f625"
#define STR_GENESIS_TIMESTAMP_START "The Times ts-1667196479 It is a new coin for free circulation without banks"
#define MAIN_GENESIS_BLOCK_TIME 1667196479
#define STR_GENESIS_BLOCK_VERSION 1
#define STR_GENESIS_BLOCK_AMOUNT 50

static void showMessageBox(std::string stringText)
{
    std::wstring stemp = std::wstring(stringText.begin(), stringText.end());
    LPCWSTR lpText = stemp.c_str();
    MessageBox(NULL, lpText, L"showMessageBox", 0);
}

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;
    CBlock genesis;
    genesis.nTime = nTime;
    genesis.nBits = nBits;
    genesis.nNonce = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = STR_GENESIS_TIMESTAMP_START;
    const CScript genesisOutputScript = CScript() << ParseHex(STR_PUBLIC_KEY) << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

static void print_ok(const CBlock& block)
{
    std::cout   << "{\"nNonce\":"
                << block.nNonce
                << ",\"nTime\":"
                << block.nTime
                << ",\"pszTimestamp\":\""
                << STR_GENESIS_TIMESTAMP_START
                << "\",\"PublicKey\":\""
                << STR_PUBLIC_KEY
                << "\",\"hashGenesisBlock\":\""
                << block.GetHash().ToString()
                << "\",\"hashMerkleRoot\":\""
                << block.hashMerkleRoot.ToString() << "\"}";
    std::cout   << std::endl
                << "ok" << std::endl;
    std::ofstream out;
    std::string file = FILE_NAME;
    out.open(file);
    if (out.is_open()) {
        out << "{\"nNonce\":"
            << block.nNonce
            << ",\"nTime\":"
            << block.nTime
            << ",\"pszTimestamp\":\""
            << STR_GENESIS_TIMESTAMP_START
            << "\",\"PublicKey\":\""
            << STR_PUBLIC_KEY
            << "\",\"hashGenesisBlock\":\""
            << block.GetHash().ToString() << "\",\"hashMerkleRoot\":\""
            << block.hashMerkleRoot.ToString() << "\"}";
    }
    out.close();
    if (is_show == "true") showMessageBox(block.ToString());
}

static bool test_final()
{
    std::ifstream infile(FILE_NAME);
    return infile.good();
}

static bool print_progress(const CBlock& block, uint64_t iter)
{
    if ((block.nNonce / 1000000) * 1000000 == block.nNonce)
    {
        double one_pr = (double)(max_nonce - min_nonce) / (double)100;
        double progress = 100;
        if ((double)one_pr!=0)
            progress = (double)iter / (double)one_pr;
        std::cout   << "Nonce "
                    << block.nNonce
                    << " nTime "
                    << block.nTime
                    << "  (" << progress << "%)"
                    << std::endl;
        return test_final();
    }
    return false;
}
static void GenerateGenesisBlock(Consensus::Params& consensus)
{
    uint32_t nNonce = MAIN_GENESIS_BLOCK_NONCE;
    if (flag_param) {
        nNonce = min_nonce;
    }
    static CBlock genesis;
    genesis = CreateGenesisBlock(MAIN_GENESIS_BLOCK_TIME, nNonce, MAIN_GENESIS_BLOCK_BIT, STR_GENESIS_BLOCK_VERSION, STR_GENESIS_BLOCK_AMOUNT * COIN);
    consensus.hashGenesisBlock = uint256S("0x");
    if (genesis.GetHash() != consensus.hashGenesisBlock) {
        arith_uint256 hashTarget = arith_uint256().SetCompact(genesis.nBits);
        uint256 hash;
        uint64_t iter = 0;
        while (UintToArith256(genesis.GetHash()) > hashTarget)
        {
            if (genesis.nNonce == max_nonce)
            {
                genesis.nNonce = min_nonce;
                ++genesis.nTime;
            }
            else
                ++genesis.nNonce;
            if (print_progress(genesis,++iter))
                return;
        }
    }
    if (CheckProofOfWork(genesis.GetHash(), genesis.nBits, consensus))
        print_ok(genesis);
}

int main(int argc, char* argv[])
{
    flag_param = false;
    if (argc >= 3) {
        min_nonce = std::atoll(argv[1]);
        max_nonce = std::atoll(argv[2]);
        if (min_nonce > max_nonce)
            max_nonce = min_nonce;
        std::cout << "min=" << min_nonce << std::endl;
        std::cout << "max=" << max_nonce << std::endl;
        flag_param = true;
    }
    if (argc >= 4) {
        is_show = argv[3];
        std::cout << "show=" << is_show << std::endl;
    }
    if (argc >= 5) {
        FILE_NAME = argv[4];
        std::cout << "file_name=" << FILE_NAME << std::endl;
    }
    if (!test_final())
    {
        Consensus::Params consensus;
        consensus.powLimit = uint256S(POW_LIMIT);
        consensus.fPowAllowMinDifficultyBlocks = true;
        GenerateGenesisBlock(consensus);
    }
}
