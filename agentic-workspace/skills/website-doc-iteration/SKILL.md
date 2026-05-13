---
name: website-doc-iteration
description: Sustainably update and review documentation under `website/` for this repository. Use when Codex needs to add, revise, reorganize, or audit website docs; assign a checker; follow module-specific rules for `content/`, `components/`, `guides/`, or `api/`; or keep navigation, cross-links, and generated API pages consistent with code.
---

# Website Doc Iteration

Maintain `website/` as a code-backed documentation system, not a loose note collection.

Work from the current repo layout:

- Main source tree: `website/zh/docs/`
- Module buckets: `content/`, `components/`, `guides/`, `api/`
- Site-level constraints: `website/AGENT.md`

Read [references/module-rules.md](references/module-rules.md) before writing or reviewing pages. Read [references/checker-template.md](references/checker-template.md) when producing a review result or assigning a checker checklist.

## Execute The Workflow

1. Classify the request into exactly one primary bucket: `content`, `components`, `guides`, or `api`.
2. Inspect the relevant code, existing docs, and the nearest `index.md` before editing.
3. Decide whether to revise an existing page or create a new page.
4. Write to the module-specific rules in `references/module-rules.md`.
5. Update `toctree`, index pages, and cross-links so the page is discoverable.
6. Run the checker pass using `references/checker-template.md`.
7. Report the result as `PASS`, `PASS WITH FOLLOW-UPS`, or `BLOCKED`.

Do not skip steps 2, 5, or 6.

## Apply Role Separation

Use two roles on every non-trivial doc task:

- `Writer`: inspect code, draft or revise pages, and update navigation.
- `Checker`: verify facts, placement, entry points, constraints, and generated-content boundaries.

If only one agent is available, finish the writing pass first, then perform a distinct checker pass and explicitly label it as such.

## Keep The Docs Code-Backed

- Prefer real paths, real commands, real type names, and real interfaces.
- Prefer `literalinclude` or direct references to repo files instead of copied code.
- State audience, prerequisites, limits, and failure modes.
- Keep each page focused on one main problem.
- Update nearby summary pages when a page's role changes.

## Handle Special Cases

- For `api/func_api.md`, treat content below the auto-generated marker as read-only.
- For multilingual changes, state whether English pages are synced, intentionally deferred, or now stale.
- When the request is a doc review, make findings the primary output. List concrete issues before summaries.

## Return A Standard Iteration Record

Include a short record in the final response or work log:

```md
## 文档迭代记录
- 范围：
- 读者：
- 代码依据：
- 更新页面：
- 主要新增信息：
- Checker 结论：
- 待补项：
```

Keep the body concise. Put detailed rules and checklists in `references/`.
