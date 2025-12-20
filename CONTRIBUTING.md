# Contributing to Live Background Removal Lite

Thank you for your interest in contributing to Live Background Removal Lite! We welcome contributions from the community and appreciate your efforts to make this plugin better.

## Code of Conduct

By participating in this project, you agree to maintain a respectful and collaborative environment. Please be professional and considerate in all interactions.

## How to Contribute

### Reporting Bugs

If you find a bug, please report it on our [GitHub Issues page](https://github.com/kaito-tokyo/live-backgroundremoval-lite/issues) using the bug report template. When reporting bugs, please include:

- A clear description of the issue
- Steps to reproduce the problem
- Expected vs. actual behavior
- Your operating system and OBS Studio version
- Plugin version
- Any relevant logs or screenshots

### Requesting Features

Feature requests are welcome! Please use the feature request template on our [GitHub Issues page](https://github.com/kaito-tokyo/live-backgroundremoval-lite/issues). Describe:

- The problem you're trying to solve
- Your proposed solution
- Why this feature would benefit other users
- Any alternative solutions you've considered

### Security Vulnerabilities

If you discover a security vulnerability, please follow the instructions in our [SECURITY.md](SECURITY.md) file. **Do not** report security issues through public GitHub issues.

## Development Setup

### Prerequisites

- **C++ Compiler**: C++17 compatible compiler
- **CMake**: Version 3.28 or later
- **clang-format-19**: For code formatting
- **gersemi**: For CMake file formatting
- **Platform-specific dependencies**:
  - macOS: Xcode Command Line Tools
  - Windows: Visual Studio 2019 or later
  - Linux: GCC/Clang and development libraries

### Building on macOS

1. Clone the repository:
   ```bash
   git clone https://github.com/kaito-tokyo/live-backgroundremoval-lite.git
   cd live-backgroundremoval-lite
   ```

2. Configure the project:
   ```bash
   cmake --preset macos
   ```

3. Build the project:
   ```bash
   cmake --build --preset macos
   ```

4. Run tests:
   ```bash
   ctest --preset macos --verbose
   ```

> **Note**: The `macos` preset includes testing by default. For production builds without tests, use the `macos-ci` preset.

### Testing the Plugin with OBS

After building, you can test the plugin with OBS Studio:

1. Build the project (if not already done):
   ```bash
   cmake --preset macos
   cmake --build --preset macos
   ```

2. Install to your OBS plugins directory:
   ```bash
   cmake --install build_macos --config RelWithDebInfo --prefix "$HOME/Library/Application Support/obs-studio/plugins"
   ```

3. Launch OBS Studio and test your changes.

> **Note**: You only need to run `cmake --preset macos` again if you've made CMake-related changes. Otherwise, just rebuild with `cmake --build --preset macos`.

### Building on Other Platforms

- **Windows**: Use Visual Studio or the Windows CMake preset
- **Linux**: Follow similar steps to macOS, adjusting paths as needed

## Coding Standards

### General Guidelines

- **Language**: C++17
- **Empty argument lists**: Use `()` instead of `(void)`, except within regions marked by `extern "C"`
- **Member variables**: Must be suffixed with an underscore (`_`)
- **File endings**: Each file must end with a single empty newline (builds will fail otherwise)

### Code Formatting

- **C/C++ files**: Format with `clang-format-19` after any modification
- **CMake files**: Format with `gersemi` after any modification
- **Configuration**: Formatting rules are defined in `.clang-format` (C/C++) and `.gersemirc` (CMake); general editor settings (indentation, line endings, etc.) are defined in `.editorconfig`

To format your code:

```bash
# Format C/C++ files
clang-format-19 -i path/to/your/file.cpp

# Format CMake files
gersemi -i path/to/CMakeLists.txt
```

### Code Style

- Follow the existing code style in the project
- Use meaningful variable and function names
- Keep functions focused and concise
- Add comments only when necessary to explain complex logic
- Use Modern C++ practices (RAII, smart pointers) for memory management

## Pull Request Process

### Before Submitting

1. **Test your changes**: Ensure all tests pass on your platform
2. **Format your code**: Run `clang-format-19` and `gersemi` on modified files
3. **Check for lint errors**: Verify your code passes any linting checks
4. **Update documentation**: If your changes affect user-facing features, update relevant documentation

### Submitting a Pull Request

1. Fork the repository and create a new branch from `main`:
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. Make your changes following the coding standards

3. Commit your changes with clear, descriptive commit messages:
   ```bash
   git commit -m "Add feature: brief description"
   ```

4. Push to your fork:
   ```bash
   git push origin feature/your-feature-name
   ```

5. Open a Pull Request against the `main` branch

### Pull Request Guidelines

- **Title**: Use a clear, descriptive title
- **Description**: Explain what changes you made and why
- **Link issues**: Reference any related issues (e.g., "Fixes #123")
- **Keep it focused**: Each PR should address a single concern
- **Be responsive**: Address feedback and questions promptly

### Review Process

- The maintainer (@umireon) reviews issues and pull requests regularly, typically about once per week, though response times may vary depending on availability
- Be patient and respectful during the review process
- If you don't receive a response within a reasonable time, feel free to ping `@umireon`

## Testing

All code changes should be tested before submission:

- Run the test suite on your platform
- Manually test the plugin with OBS Studio
- Verify your changes don't break existing functionality
- Test edge cases when applicable

## Documentation

When contributing, please ensure that:

- Code is self-documenting where possible
- Complex algorithms or business logic include explanatory comments
- User-facing changes are reflected in README.md or other documentation
- API changes are documented

## Release Process

The release process is handled by the maintainer and is documented in [.github/copilot-instructions.md](.github/copilot-instructions.md). Contributors do not need to manage releases, but you can review the process if you're interested in understanding how new versions are published.

## Platform-Specific Contributions

### Arch Linux

For Arch Linux packaging, refer to [`unsupported/arch/`](unsupported/arch). Community contributions for maintaining the PKGBUILD are welcome.

### Flatpak

For Flatpak packaging, refer to [`unsupported/flatpak/`](unsupported/flatpak). We welcome contributions from community members willing to maintain Flatpak packages.

## Community and Support

- **Issues**: [GitHub Issues](https://github.com/kaito-tokyo/live-backgroundremoval-lite/issues)
- **Maintainer**: Kaito Udagawa (@umireon)
- **Email**: umireon@kaito.tokyo
- **Website**: [https://kaito-tokyo.github.io/live-backgroundremoval-lite/](https://kaito-tokyo.github.io/live-backgroundremoval-lite/)

## License

By contributing to Live Background Removal Lite, you agree that your contributions will be licensed under the project's dual license:

- The plugin as a whole: [GPL-3.0-or-later](LICENSE.GPL-3.0-or-later)
- Reusable components: [MIT License](LICENSE.MIT)

See the [LICENSE](LICENSE) file for detailed information.

## Questions?

If you have questions about contributing, feel free to:

- Open an issue
- Reach out to the maintainer via email
- Check existing issues and documentation for answers

Thank you for contributing to Live Background Removal Lite! Your efforts help make this plugin better for everyone.
