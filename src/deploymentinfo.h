#ifndef LCRYP_DEPLOYMENTINFO_H
#define LCRYP_DEPLOYMENTINFO_H
#include <consensus/params.h>
#include <string>
struct VBDeploymentInfo {
    const char *name;
    bool gbt_force;
};
extern const VBDeploymentInfo VersionBitsDeploymentInfo[Consensus::MAX_VERSION_BITS_DEPLOYMENTS];
std::string DeploymentName(Consensus::BuriedDeployment dep);
inline std::string DeploymentName(Consensus::DeploymentPos pos)
{
    assert(Consensus::ValidDeployment(pos));
    return VersionBitsDeploymentInfo[pos].name;
}
#endif
