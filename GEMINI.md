# Development Guideline of the OBS plugin

- This project must be developed in C++17.
- Use `()` instead of `(void)` for empty argument lists except for part surrounded by `extern "C"`.
- Implementation related to unique_ptr should be placed under bridge.hpp.
- C and C++ files must be formatted using clang-format-19 after any modification.
- CMake files must be formatted using gersemi after any modification.
- OBS team maintains the CMake and GitHub Actions so we don't need to improve these parts. However, you can modify workflows start with `my-`.
- The default branch of this project is `main`.
- There must be a empty newline at the end of the file. The build will fail if this rule is not followed.

## How to build and run tests on macOS

1. Run `cmake --preset macos-testing`.
2. Run `cmake --build --preset macos-testing`.
3. Run `ctest --preset macos-testing --rerun-failed --output-on-failure`.

## How to test the effect shader (DrawingEffect_shader) on macOS
1. Run `( cd tests/shader && cmake --preset macos-testing )`.
2. Run ``( cd tests/shader && cmake --build --preset macos-testing )`.
3. Run `( cd tests/shader && ctest --preset macos-testing )`.

## How to test plugin with OBS

1. Run `cmake --preset macos-testing` only when CMake-related changes are made.
2. Run `cmake --build --preset macos-testing && rm -rf ~/"Library/Application Support/obs-studio/plugins/obs-backgroundremoval-lite.plugin" && cp -r ./build_macos/RelWithDebInfo/obs-backgroundremoval-lite.plugin ~/"Library/Application Support/obs-studio/plugins"`.

## Release Automation with Gemini

To initiate a new release, the user will instruct Gemini to start the process (e.g., "リリースを開始して"). Gemini will then perform the following steps:

1.  **Specify New Version**:
    *   **ACTION**: Display the current version.
    *   **ACTION**: Prompt the user to provide the new version number (e.g., `1.0.0`, `1.0.0-beta1`).
    *   **CONSTRAINT**: The version must follow Semantic Versioning (e.g., `MAJOR.MINOR.PATCH`).

2.  **Prepare & Update `buildspec.json`**:
    *   **ACTION**: Confirm the current branch is `main` and synchronized with the remote.
    *   **ACTION**: Create a new branch (`bump-X.Y.Z`).
    *   **ACTION**: Update the `version` field in `buildspec.json`.

3.  **Create & Merge Pull Request (PR)**:
    *   **ACTION**: Create a PR for the version update.
    *   **ACTION**: Provide the URL of the created PR.
    *   **ACTION**: Instruct the user to merge this PR.
    *   **PAUSE**: Wait for user confirmation of PR merge.

4.  **Push Git Tag**:
    *   **TRIGGER**: User instructs Gemini to push the Git tag after PR merge confirmation.
    *   **ACTION**: Switch to the `main` branch.
    *   **ACTION**: Synchronize with the remote.
    *   **ACTION**: Verify the `buildspec.json` version.
    *   **ACTION**: Push the Git tag.
    *   **CONSTRAINT**: The tag must be `X.Y.Z` (no 'v' prefix).
    *   **RESULT**: Pushing the tag triggers the automated release workflow.

5.  **Finalize Release**:
    *   **ACTION**: Provide the releases URL.
    *   **INSTRUCTION**: User completes the release on GitHub.

**Example Interaction:**

User: `リリースを開始して`
Gemini: `はい、新しいリリースを開始します。現在のバージョンは X.Y.Z です。buildspec.json に記載するリリースバージョンを指定してください (例: 1.0.0, 1.0.0-beta1)。`
User: `1.2.3`
Gemini: `buildspec.json のバージョンを 1.2.3 に更新します。よろしいですか？`
User: `はい`
Gemini: `mainブランチにいることを確認し、ブランチ 'bump-1.2.3' を作成し、buildspec.json を更新しました。プルリクエストを作成します。`
Gemini: `プルリクエストのURL: https://github.com/kaito-tokyo/obs-showdraw/pull/123`
Gemini: `このプルリクエストをマージし、完了したら「マージしたのでタグをプッシュして」と私に報告してください。`
User: `マージしたのでタグをプッシュして`
Gemini: `バージョン 1.2.3 の Git タグをプッシュします。よろしいですか？`
User: `はい`
Gemini: `mainブランチに切り替え、リモートと同期し、buildspec.jsonのバージョンを確認しました。Git タグ '1.2.3' をプッシュしました。GitHub Actions でリリースワークフローが開始されます。`
Gemini: `GitHub リリースのURL: https://github.com/kaito-tokyo/obs-showdraw/releases`
Gemini: `GitHub上でリリースを完了してください。`

# Development Guideline for Astro Pages

- **Purpose**: The Astro project in the `docs/` directory builds the official website for this OBS plugin.
- **Hosting**: The site is hosted on GitHub Pages at `obs-showdraw.kaito.tokyo`.
- **Source Directory**: All source code for the Astro site is located in the `docs` directory.
- **Key URLs**:
    - `https://obs-showdraw.kaito.tokyo/`: Main page with a multilingual plugin description.
    - `https://obs-showdraw.kaito.tokyo/metadata/`: Hosts metadata for the OBS plugin itself.
- **Formatting**: All files within the `docs/` directory must be formatted using Prettier.
- **Formatting Command**: To format the files, run `npm run format` from within the `docs/` directory.
- **Dependencies**: Dependencies are managed by `npm` and defined in `docs/package.json`.

# 課題トリアージ方針

- パッケージングの方針はOBS公式の方針に倣います。Windows向けにはzipのみ提供します。Mac向けにはpkgのみ提供します。Ubuntu向けにはdebのみ提供します。それ以外の形式を提供することはありません。
- Ubuntu以外のLinuxユーザーには自分でプラグインをビルドすることを求めます。ビルドの方法に関してはCONTRIBUTING.mdを参照してください。
- FlatpakやAURをサポートする予定は今のところありません。
