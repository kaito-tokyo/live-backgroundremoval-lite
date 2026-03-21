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

1. **Target Version**: If you know the target version, you can start this workflow to help the user bump the version of this product. When you are not sure about the target version but have to start the workflow, you MUST ask the user for the exact target version number to determine the target version.
2. **Clean Working Tree**: Ensure the git working tree is clean before making any changes. If not, you MUST stop working and ask user for actions. You MUST NOT make any changes if the working tree is not clean.
3. **Switch to Local Bump Branch**: The new branch name MUST be `bump/<new_version>`, this `bump/` prefix is essential to complete the whole release process. You MUST switch to the branch `bump/<new_version>` before making any changes.
4. **Bump Version Files**: Run `.github/scripts/bump_version.sh <new_version>` once you confirm the wrorking tree is clean. This script will update and sync all the version files automatically. Calling this script without the new version will print actual versions numbers written in each version file.
5. **Commit Changes**: The prior step SHOULD modify `buildspec.json`, `data/manifest.json`, `pages/package.json`, and `pages/package-lock.json`. Ensure the diff only contains these four files. Commit them with the message ``Bump version to <new_version>``. The commit MUST be signed and signed off (-s and -S option for git command).
6. **Push Branch to Remote**: Push the branch `bump/<new_version>` to the remote. You MUST set tracking remote branch for the local branch `bump/<new_version>`.
7. **Open Pull Request**: Run `.github/scripts/create_bump_pr.sh bump/<new_version>` to open a pull request for the version bump. This script will automatically open the Pull Request page. Now the user can enable Auto-merge, and wait for the next workflow to start.

## Notes

- Keep the change set limited to version bumps.
- Ensure `buildspec.json`, `data/manifest.json`, `pages/package.json`, and `pages/package-lock.json` stay in sync.
- If a helper script fails, stop and report the exact error before continuing.
