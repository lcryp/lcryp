#include <kernel/context.h>
#include <crypto/sha256.h>
#include <key.h>
#include <logging.h>
#include <pubkey.h>
#include <random.h>
#include <string>
namespace kernel {
Context::Context()
{
    std::string sha256_algo = SHA256AutoDetect();
    LogPrintf("Using the '%s' SHA256 implementation\n", sha256_algo);
    RandomInit();
    ECC_Start();
    ecc_verify_handle.reset(new ECCVerifyHandle());
}
Context::~Context()
{
    ecc_verify_handle.reset();
    ECC_Stop();
}
}
