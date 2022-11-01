#ifndef LCRYP_NODE_COIN_H
#define LCRYP_NODE_COIN_H
#include <map>
class COutPoint;
class Coin;
namespace node {
struct NodeContext;
void FindCoins(const node::NodeContext& node, std::map<COutPoint, Coin>& coins);
}
#endif
