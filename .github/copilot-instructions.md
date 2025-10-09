# Development Guideline for OBS Background Removal Lite Plugin

- Develop this project using C++17.
- For empty argument lists, use `()` instead of `(void)`, except within regions marked by `extern "C"`.
- After modifying C or C++ files, format them with `clang-format-19`.
- After modifying CMake files, format them with `gersemi`.
- The OBS team maintains CMake and GitHub Actions files. Do not modify these, except for workflow files starting with `my-`.
- The default branch is `main`.
- Ensure each file ends with a single empty newline. Builds will fail if this rule is not followed.

## Building and Running Tests on macOS

1. Run `cmake --preset macos-testing`.
2. Run `cmake --build --preset macos-testing`.
3. Run `ctest --preset macos-testing --rerun-failed --output-on-failure`.

## Testing the Plugin with OBS

1. Run `cmake --preset macos-testing` only if CMake-related changes were made.
2. Run:
   ```
   cmake --build --preset macos-testing && \
   rm -rf ~/Library/Application\ Support/obs-studio/plugins/live-backgroundremoval-lite.plugin && \
   cp -r ./build_macos/RelWithDebInfo/live-backgroundremoval-lite.plugin ~/Library/Application\ Support/obs-studio/plugins
   ```

## Release Automation

To initiate a new release, the user will instruct Gemini to start the process (e.g., "リリースを開始して" or "リリースしたい"). Gemini will then perform the following steps:

1.  **Specify New Version**:
    * **ACTION**: Display the current version.
    * **ACTION**: Prompt the user to provide the new version number (e.g., `1.0.0`, `1.0.0-beta1`).
    * **CONSTRAINT**: The version must follow Semantic Versioning (e.g., `MAJOR.MINOR.PATCH`).

2.  **Prepare & Update `buildspec.json`**:
    * **ACTION**: Confirm the current branch is `main` and synchronized with the remote.
    * **ACTION**: Create a new branch (`bump-X.Y.Z`).
    * **ACTION**: Update the `version` field in `buildspec.json` using `jq`, e.g.:
      ```
      jq '.version = "<new_version>"' buildspec.json > buildspec.json.tmp && mv buildspec.json.tmp buildspec.json
      ```

3.  **Create & Merge Pull Request (PR)**:
    * **ACTION**: Create a PR for the version update.
    * **ACTION**: Provide the URL of the created PR.
    * **ACTION**: Instruct the user to merge this PR.
    * **PAUSE**: Wait for user confirmation of PR merge.

4.  **Push Git Tag**:
    * **TRIGGER**: User instructs Gemini to push the Git tag after PR merge confirmation.
    * **ACTION**: Switch to the `main` branch.
    * **ACTION**: Synchronize with the remote.
    * **ACTION**: Verify the `buildspec.json` version.
    * **ACTION**: Push the Git tag.
    * **CONSTRAINT**: The tag must be `X.Y.Z` (no 'v' prefix).
    * **RESULT**: Pushing the tag triggers the automated release workflow.

5.  **Finalize Release**:
    * **ACTION**: Provide the releases URL.
    * **INSTRUCTION**: User completes the release on GitHub.

6.  **Update Arch Linux and Flatpak Package Manifests**:
    * **ACTION**: Run `unsupported/update-packages.sh <new_version>` to update both Arch Linux and Flatpak manifests automatically.
    * **ACTION**: Review the changes made by the script.

7.  **Create Pull Request for Manifest Updates**:
    * **ACTION**: Create a new branch named `unsupported/bump-<new_version>`.
    * **ACTION**: Commit the changes with the message `chore: Update PKGBUILD and Flatpak for <new_version>`.
    * **ACTION**: Create a single pull request for these changes.

