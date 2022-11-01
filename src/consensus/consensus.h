#ifndef LCRYP_CONSENSUS_CONSENSUS_H
#define LCRYP_CONSENSUS_CONSENSUS_H
#include <cstdlib>
#include <stdint.h>
static const unsigned int MAX_BLOCK_SERIALIZED_SIZE = 4000000;
static const unsigned int MAX_BLOCK_WEIGHT = 4000000;
static const int64_t MAX_BLOCK_SIGOPS_COST = 80000;
static const int COINBASE_MATURITY = 100;
static const int WITNESS_SCALE_FACTOR = 4;
static const size_t MIN_TRANSACTION_WEIGHT = WITNESS_SCALE_FACTOR * 60;
static const size_t MIN_SERIALIZABLE_TRANSACTION_WEIGHT = WITNESS_SCALE_FACTOR * 10;
static constexpr unsigned int LOCKTIME_VERIFY_SEQUENCE = (1 << 0);
#endif
