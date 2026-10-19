#include "glrc_hmbr.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace ECProject
{

bool glrc_hmbr_build_shard_split(int global_shard_count, int block_size,
                                 int helper_count, int failed_count,
                                 double rp_bandwidth_mbps, double conv_bandwidth_mbps,
                                 const std::string &rp_shard_config,
                                 GlrcHmbrShardSplit &out, std::string &error_message)
{
  out = GlrcHmbrShardSplit{};
  error_message.clear();
  if (global_shard_count < 2)
  {
    error_message = "HMBR-LRC requires GlrcShardCount >= 2";
    return false;
  }
  if (block_size < 1 || block_size % global_shard_count != 0)
  {
    error_message = "HMBR-LRC requires BlockSize divisible by GlrcShardCount";
    return false;
  }
  if (helper_count < 1 || failed_count < 1 ||
      rp_bandwidth_mbps <= 0.0 || conv_bandwidth_mbps <= 0.0)
  {
    error_message = "HMBR-LRC requires positive helper, failure, and bandwidth values";
    return false;
  }

  out.global_shard_count = global_shard_count;
  out.stripe_byte_len = block_size / global_shard_count;
  out.rp_shard_begin = 0;

  const bool automatic =
      rp_shard_config.empty() || rp_shard_config == "auto" ||
      rp_shard_config == "AUTO";
  if (automatic)
  {
    const double rp_coef =
        static_cast<double>(failed_count) / rp_bandwidth_mbps;
    const double conv_coef =
        static_cast<double>(helper_count + failed_count - 1) / conv_bandwidth_mbps;
    out.rp_fraction = conv_coef / (rp_coef + conv_coef);
    out.rp_continuous_shards = out.rp_fraction * global_shard_count;
    out.rp_shard_count =
        static_cast<int>(std::lround(out.rp_continuous_shards));
  }
  else
  {
    try
    {
      size_t used = 0;
      out.rp_shard_count = std::stoi(rp_shard_config, &used);
      if (used != rp_shard_config.size())
        throw std::invalid_argument("trailing characters");
    }
    catch (const std::exception &)
    {
      error_message = "GlrcHmbrP must be auto or an integer";
      return false;
    }
    if (out.rp_shard_count < 1 ||
        out.rp_shard_count >= global_shard_count)
    {
      error_message =
          "fixed GlrcHmbrP must be in [1, GlrcShardCount-1]";
      return false;
    }
    out.rp_fraction =
        static_cast<double>(out.rp_shard_count) / global_shard_count;
    out.rp_continuous_shards = static_cast<double>(out.rp_shard_count);
  }

  if (automatic)
    out.rp_shard_count =
        std::max(1, std::min(global_shard_count - 1, out.rp_shard_count));
  out.rp_fraction =
      static_cast<double>(out.rp_shard_count) / global_shard_count;
  out.conv_shard_begin = out.rp_shard_count;
  out.conv_shard_count = global_shard_count - out.rp_shard_count;
  const double block_megabytes =
      static_cast<double>(block_size) / (1024.0 * 1024.0);
  out.rp_estimated_time =
      out.rp_fraction * failed_count * block_megabytes /
      rp_bandwidth_mbps;
  out.conv_estimated_time =
      (1.0 - out.rp_fraction) * (helper_count + failed_count - 1) /
      conv_bandwidth_mbps * block_megabytes;
  return true;
}

} // namespace ECProject
