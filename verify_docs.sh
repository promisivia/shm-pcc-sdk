#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "verify_docs.sh is now part of the CXL-SDK open-source readiness harness."
exec "${repo_root}/tools/opensource-harness/run.sh" "$@"
