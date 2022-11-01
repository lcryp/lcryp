#include <deploymentinfo.h>
#include <consensus/params.h>
const struct VBDeploymentInfo VersionBitsDeploymentInfo[Consensus::MAX_VERSION_BITS_DEPLOYMENTS] = {
    {
          "testdummy",
          true,
    },
    {
          "taproot",
          true,
    },
};
std::string DeploymentName(Consensus::BuriedDeployment dep)
{
    assert(ValidDeployment(dep));
    switch (dep) {
    case Consensus::DEPLOYMENT_HEIGHTINCB:
        return "bip34";
    case Consensus::DEPLOYMENT_CLTV:
        return "bip65";
    case Consensus::DEPLOYMENT_DERSIG:
        return "bip66";
    case Consensus::DEPLOYMENT_CSV:
        return "csv";
    case Consensus::DEPLOYMENT_SEGWIT:
        return "segwit";
    }
    return "";
}
