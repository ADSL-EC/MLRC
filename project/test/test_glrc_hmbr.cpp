#include "glrc_hmbr.h"
#include "glrc_repair_ilp.h"
#include "unilrc_encoder.h"

#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace
{

void test_shard_split()
{
  ECProject::GlrcHmbrShardSplit split;
  std::string error;
  assert(ECProject::glrc_hmbr_build_shard_split(
      6, 192, 4, 2, 125.0, 125.0, "2", split, error));
  assert(split.rp_shard_begin == 0);
  assert(split.rp_shard_count == 2);
  assert(split.conv_shard_begin == 2);
  assert(split.conv_shard_count == 4);
  assert(split.rp_byte_offset() == 0);
  assert(split.rp_byte_size() == 64);
  assert(split.conv_byte_offset() == 64);
  assert(split.conv_byte_size() == 128);
  assert(split.rp_byte_size() + split.conv_byte_size() == 192);

  assert(!ECProject::glrc_hmbr_build_shard_split(
      1, 192, 4, 2, 125.0, 125.0, "auto", split, error));
  assert(!error.empty());
  assert(!ECProject::glrc_hmbr_build_shard_split(
      6, 193, 4, 2, 125.0, 125.0, "auto", split, error));

  assert(ECProject::glrc_hmbr_build_shard_split(
      12, 192, 8, 2, 125.0, 125.0, "auto", split, error));
  assert(split.rp_shard_count == 10);
  assert(split.conv_shard_count == 2);
  assert(std::abs(split.rp_continuous_shards - 108.0 / 11.0) < 1e-12);
  assert(split.rp_fraction == 5.0 / 6.0);
}

void test_range_decode_matches_full_decode()
{
  constexpr int k = 4;
  constexpr int r = 2;
  constexpr int z = 2;
  constexpr int n = k + r + z;
  constexpr int block_size = 192;

  std::vector<std::vector<unsigned char>> blocks(
      n, std::vector<unsigned char>(block_size, 0));
  for (int d = 0; d < k; d++)
    for (int i = 0; i < block_size; i++)
      blocks[d][i] = static_cast<unsigned char>((d * 53 + i * 17 + 11) & 0xff);

  std::vector<unsigned char> matrix(n * k, 0);
  ECProject::gen_glrc_matrix(matrix.data(), k, r, z);
  for (int row = k; row < n; row++)
    for (int i = 0; i < block_size; i++)
      for (int d = 0; d < k; d++)
        blocks[row][i] ^= ECProject::gf_mul(matrix[row * k + d], blocks[d][i]);

  const std::vector<int> failed_ids{0, 2};
  ECProject::GlrcIlpRepairPlan plan;
  assert(ECProject::glrc_solve_repair_plan(
      k, r, z, failed_ids, "global-first", plan));
  assert(plan.selected_equation_indices.size() == failed_ids.size());
  for (int eq_idx : plan.selected_equation_indices)
    assert(eq_idx >= z);

  std::vector<unsigned char *> helper_ptrs;
  for (int helper_id : plan.helper_block_ids)
    helper_ptrs.push_back(blocks[helper_id].data());

  std::vector<unsigned char *> full_recovered;
  assert(ECProject::decode_glrc_ilp(
      k, r, z, block_size, plan.helper_block_ids, helper_ptrs.data(),
      failed_ids, plan.selected_equation_indices, full_recovered));

  ECProject::GlrcHmbrShardSplit split;
  std::string error;
  assert(ECProject::glrc_hmbr_build_shard_split(
      6, block_size, plan.helper_block_count,
      static_cast<int>(failed_ids.size()), 125.0, 125.0, "2", split, error));
  std::vector<unsigned char *> conv_recovered;
  assert(ECProject::decode_glrc_ilp_range(
      k, r, z, block_size, plan.helper_block_ids, helper_ptrs.data(),
      failed_ids, plan.selected_equation_indices, split.conv_byte_offset(),
      split.conv_byte_size(), conv_recovered));

  for (size_t target = 0; target < failed_ids.size(); target++)
  {
    assert(std::memcmp(
               full_recovered[target] + split.conv_byte_offset(),
               conv_recovered[target] + split.conv_byte_offset(),
               static_cast<size_t>(split.conv_byte_size())) == 0);
    assert(std::memcmp(full_recovered[target], blocks[failed_ids[target]].data(),
                       block_size) == 0);
    delete[] full_recovered[target];
    delete[] conv_recovered[target];
  }
}

} // namespace

int main()
{
  test_shard_split();
  test_range_decode_matches_full_decode();
  std::cout << "HMBR-LRC tests passed" << std::endl;
  return 0;
}
