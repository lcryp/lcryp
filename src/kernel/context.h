#ifndef LCRYP_KERNEL_CONTEXT_H
#define LCRYP_KERNEL_CONTEXT_H
#include <memory>
class ECCVerifyHandle;
namespace kernel {
struct Context {
    std::unique_ptr<ECCVerifyHandle> ecc_verify_handle;
    Context();
    ~Context();
};
}
#endif
