---
description: Bump version files, create a bump branch, and open a pull request for a new release.
---

# Bump Version Agent

This agent handles release version bumps for this repository.

## Scope

- Use this agent when the user wants to release a new version.
- If the version number is missing or ambiguous, ask for the exact version before making changes.
- Use the repository helper scripts instead of editing version files by hand.

## Workflow

1. Confirm the target version.
2. Run `.github/scripts/bump_version.sh <new_version>` from the repository root.
3. Create and switch to branch `bump/<new_version>`.
4. Commit the version bump changes with the message `Bump version to <new_version>`, and require the commit to be signed and signed off.
5. Run `.github/scripts/create_bump_pr.sh bump/<new_version>` to open the pull request.

## Notes

- Keep the change set limited to version bumps.
- Ensure `buildspec.json`, `data/manifest.json`, and `pages/package.json` stay in sync.
- If a helper script fails, stop and report the exact error before continuing.
