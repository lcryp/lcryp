#include <deploymentstatus.h>
#include <consensus/params.h>
#include <versionbits.h>
#include <type_traits>
static_assert(ValidDeployment(Consensus::DEPLOYMENT_TESTDUMMY), "sanity check of DeploymentPos failed (TESTDUMMY not valid)");
static_assert(!ValidDeployment(Consensus::MAX_VERSION_BITS_DEPLOYMENTS), "sanity check of DeploymentPos failed (MAX value considered valid)");
static_assert(!ValidDeployment(static_cast<Consensus::BuriedDeployment>(Consensus::DEPLOYMENT_TESTDUMMY)), "sanity check of BuriedDeployment failed (overlaps with DeploymentPos)");
template<typename T, T x>
static constexpr bool is_minimum()
{
    using U = typename std::underlying_type<T>::type;
    return x == std::numeric_limits<U>::min();
}
static_assert(is_minimum<Consensus::BuriedDeployment, Consensus::DEPLOYMENT_HEIGHTINCB>(), "heightincb is not minimum value for BuriedDeployment");
static_assert(is_minimum<Consensus::DeploymentPos, Consensus::DEPLOYMENT_TESTDUMMY>(), "testdummy is not minimum value for DeploymentPos");
