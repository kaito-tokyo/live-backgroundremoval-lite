---
layout: ../layouts/MarkdownLayout.astro
pathname: site-verification
lang: en
title: Live Background Removal Lite – Site Verification Guide
description: "Learn how to verify the integrity and authenticity of the Live Background Removal Lite documentation website using provenance and attestation."
---

# Site Verification Guide

This guide explains how to verify the integrity and authenticity of the Live Background Removal Lite documentation website using cryptographic provenance and attestation.

---

## What is Site Verification?

Site verification for this project refers to the process of cryptographically verifying that the deployed website has not been tampered with and originates from the official GitHub repository. This is accomplished through:

1. **Build Provenance**: A cryptographic record of what was built and how it was built
2. **Attestation**: A signed statement from GitHub Actions verifying the provenance
3. **Integrity Verification**: SHA384 checksums of all deployed files

This ensures that the website you're viewing is the authentic, unmodified version built from our official source code.

---

## Why is This Important?

Website verification provides several security guarantees:

- **Supply Chain Security**: Ensures the website hasn't been compromised during the build or deployment process
- **Authenticity**: Confirms the website was built from the official GitHub repository
- **Integrity**: Verifies that no files have been modified after deployment
- **Transparency**: Provides a verifiable audit trail of the build process

---

## How It Works

### Build Process

When the documentation is deployed, the following security measures are automatically applied:

1. **Subresource Integrity (SRI)**: All JavaScript and CSS files are hashed with SHA384
2. **Content Security Policy (CSP)**: Script hashes are generated and added to the CSP
3. **Provenance Generation**: A complete manifest of all files with their SHA384 hashes is created
4. **Attestation**: GitHub Actions signs the provenance using Sigstore

### Deployed Files

Two files are deployed to the website root:

- **`provenance.json`**: Contains the list of all files with their SHA384 hashes and build metadata
- **`provenance.attestation.jsonl`**: The cryptographic attestation bundle from GitHub Actions

These files can be accessed at:

- https://kaito-tokyo.github.io/live-backgroundremoval-lite/provenance.json
- https://kaito-tokyo.github.io/live-backgroundremoval-lite/provenance.attestation.jsonl

---

## Automated Security Checks

A daily security check workflow (`daily-website-security-check.yaml`) automatically verifies the website's integrity:

### What the Daily Check Does

1. **Downloads provenance files** from the live website
2. **Verifies attestation** using GitHub's `gh attestation verify` command
3. **Downloads all website files** listed in the provenance
4. **Verifies checksums** of all downloaded files against the provenance

This ensures that the deployed website matches what was built and hasn't been tampered with.

### Viewing Security Check Status

You can view the status of the daily security check:

- **Badge**: Displayed on the homepage and in the footer
- **Workflow runs**: https://github.com/kaito-tokyo/live-backgroundremoval-lite/actions/workflows/daily-website-security-check.yaml

---

## Manual Verification

You can manually verify the website's integrity using the GitHub CLI.

### Prerequisites

- [GitHub CLI (`gh`)](https://cli.github.com/) installed
- `curl` and `jq` installed

### Step-by-Step Verification

#### 1. Download Provenance Files

```bash
curl -fsSL -O "https://kaito-tokyo.github.io/live-backgroundremoval-lite/provenance.json"
curl -fsSL -O "https://kaito-tokyo.github.io/live-backgroundremoval-lite/provenance.attestation.jsonl"
```

#### 2. Verify Attestation

Verify that the provenance was signed by GitHub Actions:

```bash
gh attestation verify provenance.json \
  --repo kaito-tokyo/live-backgroundremoval-lite \
  --bundle provenance.attestation.jsonl
```

**Expected output**: A message confirming the attestation is valid and signed by GitHub Actions.

#### 3. Download Website Files

Download all files listed in the provenance:

```bash
mkdir -p site
jq -r --arg base "https://kaito-tokyo.github.io/live-backgroundremoval-lite/" '
  .subject[] |
  .name as $url |
  ($url | sub($base; "")) as $path |
  "url = \"\($url)\"",
  "output = \"site/\($path)\"",
  "create-dirs"
' provenance.json | curl -fsS -K -
```

#### 4. Generate Checksums

Extract the expected checksums from the provenance:

```bash
jq -r --arg base "https://kaito-tokyo.github.io/live-backgroundremoval-lite/" '
  .subject[] |
  (.name | sub($base; "")) as $path |
  "\(.digest.sha384)  site/\($path)"
' provenance.json > SHA384SUMS.txt
```

#### 5. Verify Checksums

Verify that all downloaded files match their expected checksums:

```bash
sha384sum --check --strict SHA384SUMS.txt
```

**Expected output**: Each file should show "OK" indicating the checksum matches.

### Complete Verification Script

Here's a complete script that performs all verification steps:

```bash
#!/bin/bash
set -euo pipefail

echo "==> Downloading provenance files..."
curl -fsSL -O "https://kaito-tokyo.github.io/live-backgroundremoval-lite/provenance.json"
curl -fsSL -O "https://kaito-tokyo.github.io/live-backgroundremoval-lite/provenance.attestation.jsonl"

echo "==> Verifying attestation..."
gh attestation verify provenance.json \
  --repo kaito-tokyo/live-backgroundremoval-lite \
  --bundle provenance.attestation.jsonl

echo "==> Downloading website files..."
rm -rf site
mkdir -p site
jq -r --arg base "https://kaito-tokyo.github.io/live-backgroundremoval-lite/" '
  .subject[] |
  .name as $url |
  ($url | sub($base; "")) as $path |
  "url = \"\($url)\"",
  "output = \"site/\($path)\"",
  "create-dirs"
' provenance.json | curl -fsS -K -

echo "==> Generating checksums..."
jq -r --arg base "https://kaito-tokyo.github.io/live-backgroundremoval-lite/" '
  .subject[] |
  (.name | sub($base; "")) as $path |
  "\(.digest.sha384)  site/\($path)"
' provenance.json > SHA384SUMS.txt

echo "==> Verifying checksums..."
sha384sum --check --strict SHA384SUMS.txt

echo "==> ✅ Verification successful! The website is authentic and unmodified."
```

---

## Understanding the Provenance File

The `provenance.json` file contains detailed information about the build:

### Structure

```json
{
  "_type": "https://in-toto.io/Statement/v1",
  "subject": [
    {
      "name": "https://kaito-tokyo.github.io/live-backgroundremoval-lite/index.html",
      "digest": {
        "sha384": "abc123..."
      },
      "size": 12345
    }
  ],
  "predicateType": "https://slsa.dev/provenance/v1",
  "predicate": {
    "buildDefinition": {
      "buildType": "https://actions.github.io/buildtypes/workflow/v1",
      "externalParameters": {
        "source": {
          "uri": "https://github.com/kaito-tokyo/live-backgroundremoval-lite",
          "digest": { "sha1": "commit-sha" }
        },
        "workflow": {
          "ref": "workflow-ref",
          "runId": "run-id"
        }
      },
      "startedOn": "2024-01-01T00:00:00Z"
    },
    "runDetails": {
      "builder": {
        "id": "https://github.com/actions/runner/github-hosted"
      }
    }
  }
}
```

### Key Fields

- **`subject`**: Array of all files with their URLs, SHA384 hashes, and sizes
- **`source.uri`**: The GitHub repository URL
- **`source.digest.sha1`**: The commit SHA that was built
- **`workflow.ref`**: The workflow file that performed the build
- **`workflow.runId`**: The specific GitHub Actions run ID
- **`startedOn`**: Build timestamp

---

## Troubleshooting

### Attestation Verification Fails

**Problem**: `gh attestation verify` command fails.

**Solutions**:

1. Ensure you have the latest version of GitHub CLI: `gh version`
2. Update GitHub CLI if needed: `brew upgrade gh` (macOS) or download from https://cli.github.com
3. Check that you're using the correct repository name: `kaito-tokyo/live-backgroundremoval-lite`
4. Verify the attestation bundle file is not corrupted: `jq . provenance.attestation.jsonl`

### Checksum Verification Fails

**Problem**: `sha384sum --check` reports mismatches.

**Solutions**:

1. **Cache Issue**: Your browser or CDN may have cached an old version. Wait a few minutes and try again.
2. **Download Error**: Re-download the files using the curl command.
3. **Deployment in Progress**: A new deployment might be in progress. Check the [Actions](https://github.com/kaito-tokyo/live-backgroundremoval-lite/actions) page.

### Cannot Find Provenance Files

**Problem**: 404 error when downloading provenance files.

**Solutions**:

1. Check the GitHub Actions deployment workflow has completed successfully
2. Verify the website is accessible: https://kaito-tokyo.github.io/live-backgroundremoval-lite/
3. Try accessing the provenance file directly in your browser

---

## Technical Details

### SLSA Provenance

This project uses [SLSA (Supply chain Levels for Software Artifacts)](https://slsa.dev/) provenance format, which is an industry-standard framework for supply chain security.

### Sigstore Attestation

GitHub Actions uses [Sigstore](https://www.sigstore.dev/) to sign the provenance. Sigstore provides:

- **Transparency**: All signatures are logged in a public transparency log
- **Ephemeral Keys**: No long-lived signing keys to manage
- **Certificate-based**: Uses OIDC tokens and X.509 certificates

### SHA384 Hashing

All files are hashed using SHA384 (SHA-2 family, 384 bits), which provides:

- **Strong Security**: Cryptographically secure hash function
- **Collision Resistance**: Practically impossible to find two files with the same hash
- **SRI Compatibility**: Supported by modern browsers for Subresource Integrity

---

## Additional Security Measures

Beyond provenance verification, the website implements additional security measures:

### Subresource Integrity (SRI)

All JavaScript and CSS files include `integrity` attributes with SHA384 hashes. Browsers automatically verify these before executing scripts or applying styles.

### Content Security Policy (CSP)

A strict Content Security Policy is enforced, including:

- **Script restrictions**: Only scripts with specific hashes or from trusted CDNs can execute
- **No inline scripts**: All JavaScript must be in external files with SRI
- **Trusted Types**: Protection against DOM-based XSS attacks

### HTTPS Only

The website is served exclusively over HTTPS with HSTS (HTTP Strict Transport Security) enabled by GitHub Pages.

---

## Related Resources

- **Daily Security Check Workflow**: [`.github/workflows/daily-website-security-check.yaml`](https://github.com/kaito-tokyo/live-backgroundremoval-lite/blob/main/.github/workflows/daily-website-security-check.yaml)
- **Deploy Workflow**: [`.github/workflows/deploy-docs.yml`](https://github.com/kaito-tokyo/live-backgroundremoval-lite/blob/main/.github/workflows/deploy-docs.yml)
- **Provenance Generation Script**: [`docs/scripts/generate-provenance.mjs`](https://github.com/kaito-tokyo/live-backgroundremoval-lite/blob/main/docs/scripts/generate-provenance.mjs)
- **SLSA Documentation**: https://slsa.dev/
- **Sigstore Documentation**: https://www.sigstore.dev/
- **GitHub Attestations**: https://docs.github.com/en/actions/security-guides/using-artifact-attestations-to-establish-provenance-for-builds

---

## Summary

The Live Background Removal Lite documentation website implements comprehensive supply chain security through:

1. **Build provenance** with complete file manifests and SHA384 hashes
2. **Cryptographic attestation** signed by GitHub Actions via Sigstore
3. **Daily automated verification** to detect tampering
4. **Manual verification** capability using standard tools

This ensures that users can trust the documentation website is authentic and hasn't been compromised.

To verify the website yourself, simply run:

```bash
gh attestation verify \
  <(curl -fsSL https://kaito-tokyo.github.io/live-backgroundremoval-lite/provenance.json) \
  --repo kaito-tokyo/live-backgroundremoval-lite \
  --bundle <(curl -fsSL https://kaito-tokyo.github.io/live-backgroundremoval-lite/provenance.attestation.jsonl)
```

For questions or issues related to site verification, please open an issue on the [GitHub repository](https://github.com/kaito-tokyo/live-backgroundremoval-lite/issues).
