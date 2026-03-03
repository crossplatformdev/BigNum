#!/usr/bin/env python3
"""
split_bucket_batches.py – Generate a GitHub Actions batch matrix for the
power-range-prime-sweep workflow.

For each selected bucket, enumerates all prime exponents and splits them into
contiguous batches of at most BATCH_SIZE (default: 1000) primes.  Outputs a
JSON array of batch descriptor objects suitable for use as a GitHub Actions
``matrix.include`` value.

Usage:
    python3 scripts/split_bucket_batches.py [options]

Options:
    --bucket-start N   First bucket (1-64, default: 1)
    --bucket-end   N   Last  bucket (1-64, default: 64)
    --batch-size   N   Max primes per batch (default: 1000)
    --dry-run          Print human-readable plan instead of JSON
    --output       FILE  Write JSON to FILE instead of stdout

Worker name format (deterministic, stable):
    bucket-{N:02d}-batch-{start_ordinal:04d}-{end_ordinal:04d}-exp-{pmin}-{pmax}

Example:
    bucket-17-batch-0001-1000-exp-65537-65867
"""

import argparse
import json
import os
import sys

# Import helpers from the sibling script (same scripts/ directory).
_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, _HERE)
from generate_bucket_primes import bucket_range, enumerate_bucket_primes  # noqa: E402

BATCH_SIZE_DEFAULT = 1000
GITHUB_MATRIX_MAX = 256


def make_worker_name(
    bucket_n: int,
    start_ordinal: int,    # 1-based ordinal of first prime in this batch
    end_ordinal: int,      # 1-based ordinal of last prime in this batch
    batch_min_exponent: int,
    batch_max_exponent: int,
) -> str:
    """Return a deterministic, stable worker name for one batch.

    Format:
        bucket-{N:02d}-batch-{start_ordinal:04d}-{end_ordinal:04d}-exp-{pmin}-{pmax}

    Example:
        bucket-17-batch-0001-1000-exp-65537-65867
    """
    return (
        f"bucket-{bucket_n:02d}"
        f"-batch-{start_ordinal:04d}-{end_ordinal:04d}"
        f"-exp-{batch_min_exponent}-{batch_max_exponent}"
    )


def split_bucket_into_batches(
    bucket_n: int,
    batch_size: int = BATCH_SIZE_DEFAULT,
) -> list[dict]:
    """Return a list of batch descriptor dicts for one bucket.

    Each descriptor contains all fields needed by a GitHub Actions matrix
    entry and by the worker step.
    """
    lo, hi = bucket_range(bucket_n)
    primes = enumerate_bucket_primes(bucket_n)

    if not primes:
        return []

    total = len(primes)
    chunks = [primes[i : i + batch_size] for i in range(0, total, batch_size)]
    batch_count = len(chunks)

    batches: list[dict] = []
    cumulative = 0
    for idx, chunk in enumerate(chunks):
        start_idx = cumulative              # 0-based, inclusive
        end_idx = cumulative + len(chunk) - 1  # 0-based, inclusive
        start_ordinal = start_idx + 1       # 1-based
        end_ordinal = end_idx + 1           # 1-based
        batch_min_exp = chunk[0]
        batch_max_exp = chunk[-1]

        batches.append(
            {
                "bucket_n": bucket_n,
                "bucket_min": lo,
                "bucket_max": hi,
                "batch_index": idx,
                "batch_count": batch_count,
                "batch_prime_start_index": start_idx,
                "batch_prime_end_index": end_idx,
                "batch_min_exponent": batch_min_exp,
                "batch_max_exponent": batch_max_exp,
                "batch_size": len(chunk),
                "worker_name": make_worker_name(
                    bucket_n,
                    start_ordinal,
                    end_ordinal,
                    batch_min_exp,
                    batch_max_exp,
                ),
            }
        )
        cumulative += len(chunk)

    return batches


def generate_batch_matrix(
    bucket_start: int,
    bucket_end: int,
    batch_size: int = BATCH_SIZE_DEFAULT,
) -> list[dict]:
    """Return the full batch matrix across all selected buckets."""
    matrix: list[dict] = []
    for n in range(bucket_start, bucket_end + 1):
        matrix.extend(split_bucket_into_batches(n, batch_size))
    return matrix


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--bucket-start", type=int, default=1,
                        metavar="N", help="First bucket (1-64, default: 1)")
    parser.add_argument("--bucket-end", type=int, default=64,
                        metavar="N", help="Last bucket (1-64, default: 64)")
    parser.add_argument("--batch-size", type=int, default=BATCH_SIZE_DEFAULT,
                        metavar="N", help=f"Max primes per batch (default: {BATCH_SIZE_DEFAULT})")
    parser.add_argument("--dry-run", action="store_true",
                        help="Print human-readable plan instead of JSON")
    parser.add_argument("--output", type=str, default=None,
                        metavar="FILE", help="Write JSON to FILE instead of stdout")
    args = parser.parse_args()

    if args.bucket_start < 1 or args.bucket_start > 64:
        parser.error(f"--bucket-start must be in [1, 64], got {args.bucket_start}")
    if args.bucket_end < 1 or args.bucket_end > 64:
        parser.error(f"--bucket-end must be in [1, 64], got {args.bucket_end}")
    if args.bucket_start > args.bucket_end:
        parser.error(f"--bucket-start ({args.bucket_start}) must be <= --bucket-end ({args.bucket_end})")
    if args.batch_size < 1:
        parser.error(f"--batch-size must be >= 1, got {args.batch_size}")

    matrix = generate_batch_matrix(args.bucket_start, args.bucket_end, args.batch_size)

    if args.dry_run:
        print(f"bucket_start  : {args.bucket_start}")
        print(f"bucket_end    : {args.bucket_end}")
        print(f"batch_size    : {args.batch_size}")
        print(f"total batches : {len(matrix)}")
        print()
        for entry in matrix:
            print(
                f"  [{entry['batch_index']:3d}/{entry['batch_count']:3d}]"
                f"  bucket={entry['bucket_n']:2d}"
                f"  primes={entry['batch_prime_start_index']:6d}-{entry['batch_prime_end_index']:6d}"
                f"  exp={entry['batch_min_exponent']}-{entry['batch_max_exponent']}"
                f"  name={entry['worker_name']}"
            )
        if len(matrix) > GITHUB_MATRIX_MAX:
            print(
                f"\nWARNING: {len(matrix)} batches exceeds the GitHub Actions matrix limit"
                f" of {GITHUB_MATRIX_MAX}.  Narrow the bucket range or increase --batch-size.",
                file=sys.stderr,
            )
        return

    if len(matrix) > GITHUB_MATRIX_MAX:
        print(
            f"ERROR: {len(matrix)} batches exceeds the GitHub Actions matrix limit"
            f" of {GITHUB_MATRIX_MAX}. Narrow the bucket range or increase --batch-size.",
            file=sys.stderr,
        )
        sys.exit(1)

    json_text = json.dumps(matrix)

    if args.output:
        with open(args.output, "w") as fh:
            fh.write(json_text + "\n")
        print(f"Wrote {len(matrix)} batch entries to {args.output}", file=sys.stderr)
    else:
        print(json_text)


if __name__ == "__main__":
    main()
