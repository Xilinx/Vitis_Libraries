#!/usr/bin/env bash
# Generate A_<N>.txt under ../datas/ for block_cholesky_cfloat (Hermitian SPD, row-major text).
set -euo pipefail
TV_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${TV_DIR}"
PY="${TV_DIR}/generate_spd_matrix.py"
DATAS="${TV_DIR}/../datas"

usage() {
    echo "Usage: $0 [-n N]... | $0 --all-defaults"
    echo "  --all-defaults   Write ../datas/A_128.txt and A_256.txt (seed=42)."
    echo "  -n N             Matrix order (may repeat for multiple files)."
    echo "  -o FILE          Output path for the last -n (optional)."
    echo "  --seed S         RNG seed (default 42)."
    echo "  --numpy          Use NumPy if installed."
    echo "  --diag-shift X   Diagonal shift (default N for each -n)."
    echo ""
    echo "Examples:"
    echo "  $0 --all-defaults"
    echo "  $0 -n 256"
    echo "  $0 -n 512 -o /tmp/A_512.txt --seed 7"
}

ALL_DEFAULTS=0
SEED=42
USE_NUMPY=()
DIAG=()
ARGS=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help) usage; exit 0 ;;
        --all-defaults) ALL_DEFAULTS=1; shift ;;
        --numpy) USE_NUMPY=(--numpy); shift ;;
        --seed) SEED="$2"; shift 2 ;;
        --diag-shift) DIAG=(--diag-shift "$2"); shift 2 ;;
        *) ARGS+=("$1"); shift ;;
    esac
done

mkdir -p "${DATAS}"

if [[ "${ALL_DEFAULTS}" -eq 1 ]]; then
    python3 "${PY}" -n 128 --seed "${SEED}" -o "${DATAS}/A_128.txt" ${USE_NUMPY[@]:+"${USE_NUMPY[@]}"} ${DIAG[@]:+"${DIAG[@]}"}
    python3 "${PY}" -n 256 --seed "${SEED}" -o "${DATAS}/A_256.txt" ${USE_NUMPY[@]:+"${USE_NUMPY[@]}"} ${DIAG[@]:+"${DIAG[@]}"}
    exit 0
fi

# Passthrough: e.g. gen_tv.sh -n 256 -o ../datas/A_256.txt
if [[ ${#ARGS[@]} -eq 0 ]]; then
    usage
    exit 1
fi

python3 "${PY}" --seed "${SEED}" ${USE_NUMPY[@]:+"${USE_NUMPY[@]}"} ${DIAG[@]:+"${DIAG[@]}"} "${ARGS[@]}"
