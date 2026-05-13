# Checker Template

Run this checklist after the writing pass. Treat it as a separate role even if the same agent performs both passes.

## Required Checks

- Structure: Is the page placed in the correct module bucket?
- Navigation: Did the relevant `index.md` or `toctree` get updated?
- Facts: Do commands, paths, headers, modules, types, and functions exist in the repo?
- Audience: Does the page clearly say who it is for?
- Procedure: Are prerequisites, steps, and verification points complete?
- Constraints: Are limits, failure cases, and non-applicable cases stated?
- Examples: Are examples real or explicitly labeled as pseudocode?
- Consistency: Does terminology match nearby docs?
- Cross-links: Are upstream and related pages linked where needed?
- Generated content: Was any auto-generated API section edited by hand?
- Language sync: If only Chinese changed, is the English sync status stated?

## Decision Labels

Use exactly one result:

- `PASS`
- `PASS WITH FOLLOW-UPS`
- `BLOCKED`

## Output Format

```md
## Checker
- Result:
- Scope checked:
- Findings:
- Missing follow-ups:
```

When the task is explicitly a review, list findings first and keep summaries short.
