#!/usr/bin/env bash

# Synchronize the prepared repository from node0 to every real-system host.
# Build once on node0 before running this script; binaries and generated
# topology files are copied verbatim so every process uses the same revision.

set -euo pipefail
source "$(cd "$(dirname "$0")" && pwd)/common.sh"
load_hosts

[[ -x "$ROOT/project/cmake/build/run_coordinator" ]] || {
  echo "missing build artifacts; run 'bash compile.sh' on node0 first" >&2
  exit 1
}

# Skip only the machine running this script. The coordinator is often a
# different host; skipping it by role leaves /root/MLRC missing there.
local_ips=" $(hostname -I 2>/dev/null || true) "
is_local_host() {
  local host="$1"
  [[ "$host" == "localhost" || "$host" == "127.0.0.1" || "$host" == "$(hostname -s 2>/dev/null || true)" ]] && return 0
  [[ "$local_ips" == *" $host "* ]]
}

for host in "${ALL_HOSTS[@]}"; do
  if is_local_host "$host"; then
    echo "Skipping $host (local tree)."
    continue
  fi
  echo "Deploying to $host..."
  deployed=0
  for attempt in 1 2 3 4 5; do
    if rsync -az --delete \
      -e "ssh -i ${SSH_KEY} -o BatchMode=yes -o ConnectTimeout=10 -o StrictHostKeyChecking=accept-new -o IdentitiesOnly=yes" \
      --exclude='.git/' --exclude='storage/' --exclude='logs/' \
      "$ROOT/" "${SSH_USER}@${host}:${REMOTE_ROOT}/"; then
      deployed=1
      break
    fi
    echo "deploy to $host failed (attempt ${attempt}/5), retrying..." >&2
    sleep $((attempt * 2))
  done
  if ((deployed == 0)); then
    echo "deploy to $host failed after 5 attempts" >&2
    exit 1
  fi
done

echo "Deployment complete."
