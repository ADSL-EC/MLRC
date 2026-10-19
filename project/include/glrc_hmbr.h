#ifndef GLRC_HMBR_H
#define GLRC_HMBR_H

#include <string>

namespace ECProject
{

struct GlrcHmbrShardSplit
{
  int global_shard_count = 0;
  int stripe_byte_len = 0;
  int rp_shard_begin = 0;
  int rp_shard_count = 0;
  int conv_shard_begin = 0;
  int conv_shard_count = 0;
  double rp_fraction = 0.0;
  double rp_continuous_shards = 0.0;
  double rp_estimated_time = 0.0;
  double conv_estimated_time = 0.0;

  int rp_byte_offset() const { return rp_shard_begin * stripe_byte_len; }
  int rp_byte_size() const { return rp_shard_count * stripe_byte_len; }
  int conv_byte_offset() const { return conv_shard_begin * stripe_byte_len; }
  int conv_byte_size() const { return conv_shard_count * stripe_byte_len; }
};

/**
 * HMBR balances RP and conventional recovery:
 *   T_RP(t)   = t*f/B_rp
 *   T_CONV(t) = (1-t)*(h+f-1)/B_conv
 * The RP model assumes independent ingress and egress bandwidth budgets.
 * Here h is the selected LRC helper-union size. "auto" solves the continuous
 * intersection and rounds it to shards; an integer selects a fixed RP prefix.
 */
bool glrc_hmbr_build_shard_split(int global_shard_count, int block_size,
                                 int helper_count, int failed_count,
                                 double rp_bandwidth_mbps, double conv_bandwidth_mbps,
                                 const std::string &rp_shard_config,
                                 GlrcHmbrShardSplit &out, std::string &error_message);

} // namespace ECProject

#endif
