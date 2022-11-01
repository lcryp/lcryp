#include <wallet/coincontrol.h>
#include <util/system.h>
namespace wallet {
CCoinControl::CCoinControl()
{
    m_avoid_partial_spends = gArgs.GetBoolArg("-avoidpartialspends", DEFAULT_AVOIDPARTIALSPENDS);
}
}
