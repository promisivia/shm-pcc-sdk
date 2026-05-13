# Module Rules

Use this file after classifying the task into one primary module.

## Shared Rules

- Start with what the reader can do after reading the page.
- Use real repository paths, commands, function names, types, and modules.
- State prerequisites, limits, and non-happy-path behavior.
- Keep one page focused on one main job.
- Add new pages to the correct `index.md` `toctree`.
- Update nearby entry pages when page scope changes.
- Avoid meta-writing such as "this section will introduce".
- Prefer code-backed references over copied blocks.

## `content/`

Use for overview, architecture, repo layout, component relationships, and reading order.

Include:

1. What question the page answers
2. Core concepts or system shape
3. Module relationships
4. Code or directory entry points
5. Related reading

Avoid:

- Turning the page into an API listing
- Long procedural step-by-step sections
- Empty high-level claims without repo anchors

## `components/`

Use for component usage pages such as data structures, RPC/comms, or apps.

Include:

1. What problem the component solves
2. Preconditions and environment assumptions
3. Key files and entry points
4. Minimal usage flow
5. Important structures or interfaces
6. Constraints, pitfalls, or common failures

Avoid:

- Concept-only descriptions with no usage path
- Commands or paths that cannot be reproduced from the repo
- Omitting boundaries and failure cases

## `guides/`

Use for developer workflows, extension guides, debugging flows, or topical procedures.

Include:

1. Goal and intended reader
2. Required knowledge or environment
3. Ordered steps
4. Verification for each step
5. Common errors and fixes
6. Follow-up links

Avoid:

- Essay-style explanations without procedure
- Skipping validation checkpoints
- Explaining principles without concrete actions

## `api/`

Use for interface references, header-based grouping, and module interface guides.

Include:

1. Module boundary and when to use it
2. Grouping by header, submodule, or responsibility
3. Key types and functions
4. Calling constraints such as memory, lifetime, or threading semantics
5. Related examples or upstream pages

Special rules:

- Treat `website/zh/docs/api/func_api.md` below the auto-generated marker as read-only.
- Put hand-written interpretation above that marker only.
- Add context that helps the reader understand when and how to use the generated APIs instead of repeating generated signatures.

## Index Pages

Always inspect these when adding or reshaping pages:

- `website/zh/docs/index.md`
- `website/zh/docs/content/index.md`
- `website/zh/docs/components/index.md`
- `website/zh/docs/guides/index.md`
- `website/zh/docs/api/index.md`

Each index page must explain what its subtree is for and link to every page that should be discoverable.
