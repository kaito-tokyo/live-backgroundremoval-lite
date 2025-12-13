# Contributing to Live Background Removal Lite

Thank you for your interest in contributing to Live Background Removal Lite! We welcome contributions from the community and appreciate your efforts to help improve this project.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Ways to Contribute](#ways-to-contribute)
- [Reporting Bugs](#reporting-bugs)
- [Suggesting Features](#suggesting-features)
- [Development Setup](#development-setup)
- [Development Guidelines](#development-guidelines)
- [Building and Testing](#building-and-testing)
- [Submitting Changes](#submitting-changes)
- [Style Guide](#style-guide)
- [Community and Support](#community-and-support)

## Code of Conduct

This project and everyone participating in it is expected to maintain a professional and respectful environment. Please be polite and respectful in all communications. Remember that developers are human too, with limited time and resources.

## Ways to Contribute

There are many ways you can contribute to Live Background Removal Lite:

- **Report Bugs**: Help us identify and fix issues
- **Suggest Features**: Share ideas for new functionality or improvements
- **Submit Code**: Contribute bug fixes, features, or improvements
- **Improve Documentation**: Help clarify or expand our documentation
- **Answer Questions**: Help other users in discussions and issues
- **Package Maintenance**: Help maintain packages for different distributions (especially AUR, Flatpak)

## Reporting Bugs

Before reporting a bug, please:

1. Check the [existing issues](https://github.com/kaito-tokyo/live-backgroundremoval-lite/issues) to see if it has already been reported
2. Use the latest version of the plugin to see if the issue has been fixed
3. Gather the required information (OBS log files, system information, etc.)

When reporting a bug, use the [bug report template](https://github.com/kaito-tokyo/live-backgroundremoval-lite/issues/new?template=bug_report.yml) and provide:

- Operating system and version
- Plugin version
- OBS Studio version
- OBS Studio log URL (required)
- OBS Studio crash log URL (if applicable)
- Expected vs. actual behavior
- Steps to reproduce
- Any additional context or screenshots

**Note**: Bug reports without an OBS log file will be closed.

## Suggesting Features

We welcome feature suggestions! Before submitting:

1. Check [existing feature requests](https://github.com/kaito-tokyo/live-backgroundremoval-lite/issues?q=is%3Aissue+label%3Aenhancement) to avoid duplicates
2. Consider the project's philosophy of being "Zero-Configuration" and crash-resistant
3. Think about technical feasibility

Use the [feature request template](https://github.com/kaito-tokyo/live-backgroundremoval-lite/issues/new?template=feature_request.yml) and include:

- Description of the problem (if applicable)
- Proposed solution
- Technical justification/feasibility
- Additional context

## Development Setup

### Prerequisites

- **C++17 compiler**: clang-format-19 for formatting
- **CMake**: Version 3.28 or higher
- **Development Tools**:
  - clang-format-19 (for C/C++ formatting)
  - gersemi (for CMake formatting)
- **Dependencies**: Managed via vcpkg (see `vcpkg.json`)

### Getting Started

1. **Clone the repository**:
   ```bash
   git clone https://github.com/kaito-tokyo/live-backgroundremoval-lite.git
   cd live-backgroundremoval-lite
   ```

2. **Read the development guidelines**:
   Review [GEMINI.md](GEMINI.md) for detailed development instructions specific to your platform.

## Development Guidelines

### General Rules

- Develop this project using **C++17**
- Use `()` instead of `(void)` for empty argument lists, except within regions marked by `extern "C"`
- Member variables must be suffixed with an underscore (`_`)
- Each file must end with a single empty newline (builds will fail otherwise)
- The default branch is `main`

### Formatting

After modifying any files, format them using:

- **C/C++ files**: `clang-format-19`
- **CMake files**: `gersemi`

### What NOT to Modify

The OBS team maintains CMake and GitHub Actions files. **Do not modify these**, except for workflow files starting with `my-`.

## Building and Testing

### macOS

1. **Configure the build**:
   ```bash
   cmake --preset macos
   ```

2. **Build the project**:
   ```bash
   cmake --build --preset macos
   ```

3. **Run tests**:
   ```bash
   ctest --preset macos --rerun-failed --output-on-failure
   ```

### Testing with OBS on macOS

1. Run `cmake --preset macos` only if CMake-related changes were made.

2. Build and install to OBS:
   ```bash
   cmake --build --preset macos && \
   rm -rf ~/Library/Application\ Support/obs-studio/plugins/live-backgroundremoval-lite.plugin && \
   cp -r ./build_macos/RelWithDebInfo/live-backgroundremoval-lite.plugin ~/Library/Application\ Support/obs-studio/plugins
   ```

### Other Platforms

For Windows and Linux build instructions, please refer to the GitHub Actions workflows in `.github/workflows/`.

## Submitting Changes

### Pull Request Process

1. **Fork the repository** and create a new branch from `main`:
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Make your changes**:
   - Follow the [development guidelines](#development-guidelines)
   - Write or update tests as needed
   - Format your code

3. **Test your changes**:
   - Ensure all tests pass
   - Test the plugin with OBS Studio manually

4. **Commit your changes**:
   - Write clear, concise commit messages
   - Reference any related issues (e.g., "Fixes #123")

5. **Push to your fork** and create a pull request:
   - Provide a clear description of your changes
   - Explain why the changes are necessary
   - Include any relevant issue numbers

6. **Respond to feedback**:
   - The maintainer (@umireon) reviews issues and PRs about once a week
   - Be patient and responsive to review comments
   - Make requested changes promptly

### Pull Request Guidelines

- Keep changes focused and minimal
- One feature/fix per pull request
- Update documentation if needed
- Ensure CI checks pass
- Be respectful and professional in all communications

## Style Guide

### C++ Code Style

- Follow **Modern C++ practices** (RAII, smart pointers)
- Use descriptive variable names
- Member variables must end with an underscore (`_`)
- Empty argument lists use `()` not `(void)` (except in `extern "C"` blocks)
- Format code with clang-format-19

### CMake Style

- Follow the existing patterns in CMakeLists.txt files
- Format with gersemi after modifications

### Documentation

- Use clear, concise language
- Include code examples where helpful
- Update relevant documentation when changing functionality

## Community and Support

### Getting Help

- **Issues**: For bug reports and feature requests, use [GitHub Issues](https://github.com/kaito-tokyo/live-backgroundremoval-lite/issues)
- **Discussions**: For questions and general discussion, use [GitHub Discussions](https://github.com/kaito-tokyo/live-backgroundremoval-lite/discussions)
- **Security**: For security vulnerabilities, see [SECURITY.md](SECURITY.md)

### Response Times

- The main developer (@umireon) is quite busy and checks issues about once a week
- Be patient—a human will always respond to your issue
- If you don't hear back for a while, feel free to ping `@umireon`

### Contributing to Unsupported Platforms

We welcome community contributions for platforms not officially supported:

- **Arch Linux**: Help maintain the AUR package using the resources in `unsupported/arch/`
- **Flatpak**: Help maintain Flatpak packages using the resources in `unsupported/flatpak/`
- **Other Distributions**: Create and maintain packages for your distribution

These community-maintained packages are valuable contributions that help users on these platforms.

## License

By contributing to Live Background Removal Lite, you agree that your contributions will be licensed under the project's dual license:

- The plugin as a whole: [GPL-3.0-or-later](LICENSE.GPL-3.0-or-later)
- Reusable components: [MIT License](LICENSE.MIT)

See the [LICENSE](LICENSE) file for detailed information.

---

Thank you for contributing to Live Background Removal Lite! Your efforts help make this plugin better for everyone.
