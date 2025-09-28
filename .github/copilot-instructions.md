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
   rm -rf ~/Library/Application\ Support/obs-studio/plugins/backgroundremoval-lite.plugin && \
   cp -r ./build_macos/RelWithDebInfo/backgroundremoval-lite.plugin ~/Library/Application\ Support/obs-studio/plugins
   ```

## Release Automation with Gemini

To initiate a new release, the user will instruct Gemini to start the process (e.g., "リリースを開始して" or "リリースしたい"). Gemini will then perform the following steps:

1.  **Specify New Version**:
    * **ACTION**: Display the current version.
    * **ACTION**: Prompt the user to provide the new version number (e.g., `1.0.0`, `1.0.0-beta1`).
    * **CONSTRAINT**: The version must follow Semantic Versioning (e.g., `MAJOR.MINOR.PATCH`).

2.  **Prepare & Update `buildspec.json`**:
    * **ACTION**: Confirm the current branch is `main` and synchronized with the remote.
    * **ACTION**: Create a new branch (`bump-X.Y.Z`).
    * **ACTION**: Update the `version` field in `buildspec.json`.

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

6.  **Update Arch Linux Package Manifest**:
    * **ACTION**: Match the `pkgver` field in `unsupported/arch/obs-backgroundremoval-lite/PKGBUILD` with the version in `buildspec.json`.
    * **ACTION**: Download the source tar.gz for that version from GitHub and calculate its SHA-256 checksum.
    * **ACTION**: Replace the `sha256sums` field in `unsupported/arch/obs-backgroundremoval-lite/PKGBUILD` with the newly calculated SHA-256 checksum.

7.  **Update Flatpak Package Manifest**:
    * **ACTION**: Add a new `<release>` element to `unsupported/flatpak/com.obsproject.Studio.Plugin.BackgroundRemovalLite.metainfo.xml`.
    * **ACTION**: The new release element should have the `version` and `date` attributes set to the new version and current date.
    * **ACTION**: The description inside the release element should be a summary of the release notes from GitHub Releases.
        You can get the body of release note by running `gh release view <tag>`.
    * **ACTION**: Update the `tag` field for the `backgroundremoval-lite` module in `unsupported/flatpak/com.obsproject.Studio.Plugin.BackgroundRemovalLite.yaml` to the new version.
    * **ACTION**: Get the commit hash for the new tag by running `git rev-list -n 1 <new_version_tag>`.
    * **ACTION**: Update the `commit` field for the `backgroundremoval-lite` module in `unsupported/flatpak/com.obsproject.Studio.Plugin.BackgroundRemovalLite.yaml` to the new commit hash.

8.  **Create Pull Request for Manifest Updates**:
    * **ACTION**: Commit the changes for both files and create a single pull request.

