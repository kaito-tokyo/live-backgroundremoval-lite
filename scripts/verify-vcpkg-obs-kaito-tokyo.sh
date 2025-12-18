#!/bin/bash
set -euo pipefail

# Configuration
ID="vcpkg-obs-kaito-tokyo"
REPO="kaito-tokyo/vcpkg-obs-kaito-tokyo"
OBS_BASE_URL="https://vcpkg-obs.kaito.tokyo"

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <status_file1> [<status_file2> ...]" >&2
  exit 1
fi

# Determine null device path based on OS for curl config
# Windows (Git Bash) needs "NUL", others use "/dev/null"
case "$OSTYPE" in
  msys*|cygwin*) NULL_PATH="NUL" ;;
  *)             NULL_PATH="/dev/null" ;;
esac

# Convert status file paths to absolute paths
STATUS_FILES=()
for f in "$@"; do
  if [[ -f "$f" ]]; then
    if command -v realpath >/dev/null 2>&1; then
      STATUS_FILES+=("$(realpath "$f")")
    else
      STATUS_FILES+=("$(cd "$(dirname "$f")" && pwd)/$(basename "$f")")
    fi
  else
    echo "Error: Status file not found: $f" >&2
    exit 1
  fi
done

# Create workspace
WORK_DIR="sigstore-$ID"
mkdir -p "$WORK_DIR"
pushd "$WORK_DIR" > /dev/null

echo "Fetching attestation bundle..." >&2
rm -f ./*.jsonl
curl -fsSL "https://readwrite.vcpkg-obs.kaito.tokyo/sigstore/curl" | curl -s -Z -K -
cat ./*.jsonl > bundle.jsonl

echo "Analyzing status files..." >&2

# --- Step 1: Extract candidates from status files ---
{
  for status_file in "${STATUS_FILES[@]}"; do
    echo "   Processing: $status_file" >&2

    tr -d '\r' < "$status_file" | awk -v RS="" -F"\n" '{
      pkg=""; ver=""; abi=""
      for(i=1; i<=NF; i++) {
        if ($i ~ /^Package:/) { split($i, a, ":"); pkg = a[2]; gsub(/^[ \t]+|[ \t]+$/, "", pkg); }
        if ($i ~ /^Version:/) { split($i, a, ":"); ver = a[2]; gsub(/^[ \t]+|[ \t]+$/, "", ver); }
        if ($i ~ /^Abi:/)     { split($i, a, ":"); abi = a[2]; gsub(/^[ \t]+|[ \t]+$/, "", abi); }
      }
      if (pkg != "" && ver != "" && abi != "") {
        print pkg, ver, abi
      }
    }'
  done
} > candidates.txt

candidate_count=$(wc -l < candidates.txt | tr -d ' ')
echo "Found $candidate_count packages in status files." >&2

if [[ "$candidate_count" -eq 0 ]]; then
  echo "No packages found to check." >&2
  popd > /dev/null
  exit 0
fi

# --- Step 2: Check existence via HEAD requests ---
echo "Checking existence on server (HEAD requests)..." >&2

while read -r pkg ver abi; do
  url="${OBS_BASE_URL}/${pkg}/${ver}/${abi}"
  printf 'url = "%s"\n' "$url"
  printf 'output = %s\n' "$NULL_PATH"
  printf 'write-out = "%%{http_code} %%{url_effective}\\n"\n'
done < candidates.txt > curl_head_config.txt

curl -s -I -Z -K curl_head_config.txt > existence_results.txt

# --- Step 3: Generate download config ---
echo "Generating download list..." >&2

awk '
$1 == "200" {
  url = $2
  n = split(url, parts, "/")
  abi = parts[n]

  print "url = \"" url "\""
  print "output = \"downloads/" abi "\""
}' existence_results.txt > curl_download_config.txt

download_count=$(grep -c "^url =" curl_download_config.txt || true)
echo "$download_count packages exist on remote server." >&2

# --- Step 4: Download ---
mkdir -p downloads
echo "Downloading artifacts..." >&2
if [ -s curl_download_config.txt ]; then
  curl -f -s -Z -K curl_download_config.txt
else
  echo "No artifacts to download." >&2
fi

# --- Step 5: Verify (Sequential) ---
echo "Verifying attestations..." >&2
verified_count=0
failed_count=0

if [ -d "downloads" ]; then
  shopt -s nullglob
  for artifact in downloads/*; do
    filename=$(basename "$artifact")

    if gh attestation verify "$artifact" --repo "$REPO" --bundle "bundle.jsonl" >/dev/null 2>&1; then
      echo "Verified: $filename" >&2
      verified_count=$((verified_count + 1))
    else
      echo "FAILED: $filename" >&2
      failed_count=$((failed_count + 1))
    fi
  done
  shopt -u nullglob
fi

popd > /dev/null

# Calculate Skipped
skipped_count=$((candidate_count - download_count))

echo "----------------------------------------" >&2
echo "Result: Success: $verified_count, Failed: $failed_count, Skipped: $skipped_count" >&2
echo "Debug files are preserved in: ./$WORK_DIR" >&2

if [[ $failed_count -gt 0 ]]; then
  exit 1
fi
