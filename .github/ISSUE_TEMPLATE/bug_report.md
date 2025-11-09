name: Bug Report
description: Report a bug or crash
labels: bug
assignees: umireon
body:
  - type: markdown
    id: md_welcome
    attributes:
      value: |
        ## 🐞 Bug Report for Live Background Removal Lite

        Please fill out **all required fields**. Incomplete reports may be closed without investigation.
        - Attach an OBS log file (see below).
        - For crashes, attach a crash log if available.
        - Check existing issues before submitting a new one.

  - type: dropdown
    id: os_info
    attributes:
      label: Operating System
      description: Which operating system are you using?
      options:
        - Windows 11
        - Windows 10
        - macOS 15
        - macOS 14
        - macOS 13
        - macOS 12
        - macOS 11
        - Ubuntu 24.04
        - Arch Linux
        - Other (please specify below)
    validations:
      required: true

  - type: input
    id: os_info_other
    attributes:
      label: Other OS (if not listed above)
      description: Specify your OS if you selected "Other".
      placeholder: "e.g., Fedora 39, FreeBSD 13.2"
    validations:
      required: false

  - type: input
    id: plugin_version
    attributes:
      label: Plugin Version
      description: What version of Live Background Removal Lite are you using?
      placeholder: "e.g., 1.0.0, 1.0.0-beta1, git-abcdef"
    validations:
      required: true

  - type: dropdown
    id: obs_version
    attributes:
      label: OBS Studio Version
      description: Which version of OBS Studio are you using?
      options:
        - 32.0.2
        - 31.1.2
        - Git (please specify below)
        - Other (please specify below)
    validations:
      required: true

  - type: input
    id: obs_version_other
    attributes:
      label: OBS Studio Version (if "Git" or "Other")
      description: Specify your OBS Studio version if you selected "Git" or "Other".
      placeholder: "e.g., 33.0.0-beta1, git-abcdef"
    validations:
      required: false

  - type: input
    id: obs_log_url
    attributes:
      label: OBS Studio Log URL
      description: |
        Please provide the obsproject.com URL to the OBS log file where this issue occurred.
        **Bug reports without a log file will be closed.**
        (Help menu > Log Files > Upload Current/Previous Log File)
      placeholder: "https://obsproject.com/logs/xxxxxxx"
    validations:
      required: true

  - type: input
    id: obs_crash_log_url
    attributes:
      label: OBS Studio Crash Log URL (if applicable)
      description: If this is a crash report, provide the obsproject.com URL to the crash log file.
      placeholder: "https://obsproject.com/logs/xxxxxxx"
    validations:
      required: false

  - type: textarea
    id: expected_behavior
    attributes:
      label: Expected Behavior
      description: What did you expect to happen?
      placeholder: "Describe what should have happened."
    validations:
      required: true

  - type: textarea
    id: current_behavior
    attributes:
      label: Actual Behavior
      description: What actually happened?
      placeholder: "Describe what actually happened, including error messages if any."
    validations:
      required: true

  - type: textarea
    id: steps_to_reproduce
    attributes:
      label: Steps to Reproduce
      description: How can we reproduce this bug? Please provide step-by-step instructions.
      placeholder: |
        1. ...
        2. ...
        3. ...
    validations:
      required: true

  - type: textarea
    id: additional_notes
    attributes:
      label: Additional Notes / Screenshots
      description: Add any other context, screenshots, or information that might help us understand the issue.
    validations:
      required: false

  - type: checkboxes
    id: confirm_log
    attributes:
      label: Confirmation
      description: Please confirm the following before submitting.
      options:
        - label: I have attached the required OBS log file(s).
          required: true