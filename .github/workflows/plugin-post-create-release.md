---
on:
  workflow_call:
    inputs:
      ref:
        description: 'The git ref of the release, e.g. "refs/tags/v1.2.3".'
        required: true
        type: string

safe-outputs:
  create-issue:
    title-prefix: "[Plugin CI] "
    labels: [todo]
    assignees: [umireon]
    expires: false

engine:
  id: copilot
  model: gpt-5-mini
---

# Postprocess After Creating a Draft Release

- **Trigger**: This workflow is triggered after the plugin-create-release workflow creates a draft release for the current git ref successfully.
- **Goal**: Notify the maintainer (@umireon) that a new draft release has been created by creating an issue with a link to the draft release. Additionally, suggest a one-line summary of the release based on contents.
