<!--
SPDX-FileCopyrightText: 2026 Kaito Udagawa <umireon@kaito.tokyo>

SPDX-License-Identifier: CC0-1.0
-->

# AI-Generated Content History Audit

Audit date: 2026-04-14

## Scope and method

This report reviews every currently tracked file in the repository with rename-aware git history.

- Primary evidence threshold: direct provenance from Copilot-authored commits.
- Secondary review: AI-related commit subjects and AI-documentation paths were checked for extra candidates, but keyword-only matches were rejected when they described tooling or documentation rather than generated repository content.
- Translation files remain in scope as policy-allowed exceptions because the repository policy explicitly allows GenAI usage for translations.

The audit covered 339 tracked files.

| Category | Count |
| --- | ---: |
| First-party files reviewed | 302 |
| Translation files reviewed as policy-allowed | 10 |
| Generated artifacts excluded | 13 |
| Vendor or submodule files excluded | 14 |
| First-party files with confirmed Copilot-authored history | 12 |
| Translation files with confirmed Copilot-authored history | 3 |
| Files with Copilot-attributed lines still present at HEAD | 8 |

## Confirmed findings

The following current files have direct history evidence from Copilot-authored commits.

| Path | Scope | Evidence commits | HEAD retention | Assessment |
| --- | --- | --- | --- | --- |
| `.gitignore` | first-party | `c5139f8` Make first run dialog modeless | Not retained at HEAD | Historical Copilot edit only |
| `CONTRIBUTING.md` | first-party | `fd6ae23` Add CONTRIBUTING.md | Not retained at HEAD | Historical Copilot edit only |
| `README.md` | first-party | `17f8bff` Fix broken releases page link; `ea58c22` Add comprehensive dual licensing documentation; `25a011b` Add binary verification documentation; `eaae450` Add Discord Server link | Retained at HEAD (42 Copilot-attributed lines) | Confirmed Copilot-authored content remains in the current file |
| `buildspec.json` | first-party | `5185bcd` Release 1.0.2 | Not retained at HEAD | Historical Copilot edit only |
| `data/locale/en-US.ini` | translation, policy-allowed | `e10d14d` Implement Center Frame | Retained at HEAD (2 Copilot-attributed lines) | Policy-allowed translation history remains in the current file |
| `data/locale/ja-JP.ini` | translation, policy-allowed | `e10d14d` Implement Center Frame | Retained at HEAD (1 Copilot-attributed line) | Policy-allowed translation history remains in the current file |
| `data/locale/ko-KR.ini` | translation, policy-allowed | `e10d14d` Implement Center Frame | Retained at HEAD (1 Copilot-attributed line) | Policy-allowed translation history remains in the current file |
| `pages/src/pages/faq.md` | first-party | `eaae450` Add Discord Server link to README and documentation site | Not retained at HEAD | Historical Copilot edit only |
| `pages/src/pages/index.astro` | first-party | `eaae450` Add Discord Server link to README and documentation site | Not retained at HEAD | Historical Copilot edit only |
| `src/LiveBackgroundRemovalLite/MainFilter/MainFilterContext.cpp` | first-party | `e10d14d` Implement Center Frame | Retained at HEAD (6 Copilot-attributed lines) | Confirmed Copilot-authored code remains in the current file |
| `src/LiveBackgroundRemovalLite/MainFilter/PluginProperty.hpp` | first-party | `e10d14d` Implement Center Frame | Retained at HEAD (2 Copilot-attributed lines) | Confirmed Copilot-authored code remains in the current file |
| `src/LiveBackgroundRemovalLite/MainFilter/RenderingContext.cpp` | first-party | `e10d14d` Implement Center Frame | Retained at HEAD (3 Copilot-attributed lines) | Confirmed Copilot-authored code remains in the current file |
| `src/LiveBackgroundRemovalLite/MainFilter/RenderingContext.hpp` | first-party | `e10d14d` Implement Center Frame | Retained at HEAD (6 Copilot-attributed lines) | Confirmed Copilot-authored code remains in the current file |
| `src/LiveBackgroundRemovalLite/MainFilter/TroubleshootDialog.hpp` | first-party | `e10d14d` Implement Center Frame | Not retained at HEAD | Historical Copilot edit only |
| `src/LiveBackgroundRemovalLite/StartupUI/FirstRunDialog.cpp` | first-party | `c5139f8` Make first run dialog modeless | Not retained at HEAD | Historical Copilot edit only |

## Cleanup commit evidence

Commit `288f755` (`Delete GenAI-related files`) is an explicit remediation signal in history.

- `README.md`: removed AI-agent-oriented documentation sections.
- `src/LiveBackgroundRemovalLite/MainFilter/MainFilterContext.cpp`: removed the `enableCenterFrame` default and property wiring.
- `src/LiveBackgroundRemovalLite/MainFilter/RenderingContext.cpp`: removed the `enableCenterFrame` rendering path and related logging.

Those three files already have direct Copilot-authored history, so this cleanup commit is supporting context rather than a separate finding class.

## Rejected heuristic hits

The heuristic pass did not add any extra confirmed file findings beyond the Copilot-authored commits listed above.

Reviewed and rejected as false positives:

- `pages/public/llms.txt`
- `Meta llms` commit subjects
- `GEMINI.md`-related history
- `copilot-instructions.md`-related history

These items document AI tooling or repository instructions, but they do not prove generated repository content by themselves.

## Remaining reviewed files

After removing the confirmed findings above:

- 290 first-party files were reviewed with no direct GenAI provenance found.
- 7 translation files were reviewed with no direct GenAI provenance found.
- 13 generated artifacts were excluded from manual findings.
- 14 vendor or submodule files were excluded as third-party content.
