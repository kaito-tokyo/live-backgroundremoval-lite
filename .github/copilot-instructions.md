# Development Guideline for Live Background Removal Lite Plugin

- Develop this project using C++20.
- For empty argument lists, use `()` instead of `(void)`, except within regions marked by `extern "C"`.
- After modifying C or C++ files, format them with `clang-format-19`.
- After modifying CMake files, format them with `gersemi`.
- The default branch is `main`.
- Ensure each file ends with a single empty newline. Builds will fail if this rule is not followed.
- Member variables must be suffixed with an underscore (`_`).

## Building and Running Tests on macOS

1. Run `cmake --preset macos`.
2. Run `cmake --build --preset macos`.
3. Run `ctest --preset macos --rerun-failed --output-on-failure`.

## Testing the Plugin with OBS on Mac

1. Run `cmake --preset macos` only if CMake-related changes were made.
2. Run:
   ```
   cmake --build --preset macos && \
   rm -rf ~/Library/Application\ Support/obs-studio/plugins/live-backgroundremoval-lite.plugin && \
   cp -r ./build_macos/RelWithDebInfo/live-backgroundremoval-lite.plugin ~/Library/Application\ Support/obs-studio/plugins
   ```

## Release Automation

To initiate a new release, the user will instruct Gemini to start the process (e.g., "リリースを開始して" or "リリースしたい"). Gemini will then perform the following steps:

1.  **Specify New Version**:
    - **ACTION**: Display the current version.
    - **ACTION**: Prompt the user to provide the new version number (e.g., `1.0.0`, `1.0.0-beta1`).
    - **CONSTRAINT**: The version must follow Semantic Versioning (e.g., `MAJOR.MINOR.PATCH`).

2.  **Prepare & Update `buildspec.json`**:
    - **ACTION**: Confirm the current branch is `main` and synchronized with the remote.
    - **ACTION**: Create a new branch (`bump-X.Y.Z`).
    - **ACTION**: Update the `version` field in `buildspec.json` using `jq`, e.g.:
      ```
      jq '.version = "<new_version>"' buildspec.json > buildspec.json.tmp && mv buildspec.json.tmp buildspec.json
      ```
      Do not change by yourself; use `jq` to avoid formatting issues.

3.  **Create & Merge Pull Request (PR)**:
    - **ACTION**: Create a PR for the version update.
    - **ACTION**: Provide the URL of the created PR.
    - **ACTION**: Instruct the user to merge this PR.
    - **PAUSE**: Wait for user confirmation of PR merge.

4.  **Push Git Tag**:
    - **TRIGGER**: User instructs Gemini to push the Git tag after PR merge confirmation.
    - **ACTION**: Switch to the `main` branch.
    - **ACTION**: Synchronize with the remote.
    - **ACTION**: Verify the `buildspec.json` version.
    - **ACTION**: Push the Git tag.
    - **CONSTRAINT**: The tag must be `X.Y.Z` (no 'v' prefix).
    - **RESULT**: Pushing the tag triggers the automated release workflow.

5.  **Finalize Release**:
    - **ACTION**: Provide the releases URL.
    - **INSTRUCTION**: User completes the release on GitHub.

6.  **Update Arch Linux and Flatpak Package Manifests**:
    - **ACTION**: Run `~/Documents/GitHub/live-plugins-hub/scripts/update-live-backgroundremoval-lite-arch.sh` to update the Arch Linux manifest automatically.
    - **ACTION**: Run `~/Documents/GitHub/live-plugins-hub/scripts/update-live-backgroundremoval-lite-flatpak.sh` to update the Flatpak manifest automatically.
    - **ACTION**: Review the changes made by the script.
