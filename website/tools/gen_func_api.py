#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, List, Tuple


AUTO_MARKER = "<!-- AUTO-GENERATED BELOW. DO NOT EDIT BY HAND. -->"


def strip_comments(text: str) -> str:
    # Remove /* ... */ comments first (including multiline)
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    # Remove // comments
    text = re.sub(r"//.*?$", "", text, flags=re.M)
    return text


def iter_headers(include_root: Path) -> Iterable[Path]:
    for path in sorted(include_root.rglob("*.h")):
        yield path


@dataclass
class Candidate:
    signature: str
    conditions: Tuple[str, ...]


_re_ws = re.compile(r"\s+")


def normalize_ws(s: str) -> str:
    return _re_ws.sub(" ", s).strip()


def clean_signature(stmt: str) -> str:
    s = normalize_ws(stmt)
    # Drop leading scope braces that can appear after "};"
    while s.startswith("}"):
        s = s[1:].lstrip()
    return s


def looks_like_func_decl(stmt: str) -> bool:
    s = clean_signature(stmt)
    if not s or s.startswith("#"):
        return False
    # Avoid parsing artifacts from macro bodies / inline definitions.
    if "{" in s or "}" in s or "\\" in s:
        return False
    # Exclude common non-declaration statements / typedefs
    if s.startswith(("typedef ", "using ")):
        return False
    # Exclude type declarations (including template type declarations that contain parentheses).
    if re.search(r"\b(class|struct|enum)\b", s):
        return False
    if "(" not in s or ")" not in s:
        return False
    # Exclude control statements (should not appear at file scope, but be safe)
    if re.match(r"^(if|for|while|switch)\b", s):
        return False
    # Exclude function pointer declarations
    if "(*" in s:
        return False
    # Exclude static_assert
    if s.startswith("static_assert"):
        return False
    # Exclude obvious macro invocations like FOO_BAR(...);
    if re.match(r"^[A-Z_][A-Z0-9_]*\(", s):
        return False
    return True


def parse_condition_line(line: str) -> str | None:
    line = line.strip()
    if not line.startswith("#"):
        return None
    if line.startswith("#ifdef "):
        return line[len("#ifdef ") :].strip()
    if line.startswith("#ifndef "):
        return "!" + line[len("#ifndef ") :].strip()
    if line.startswith("#if "):
        expr = line[len("#if ") :].strip()
        return expr
    if line.startswith("#elif "):
        expr = line[len("#elif ") :].strip()
        return expr
    if line.startswith("#else"):
        return "else"
    if line.startswith("#endif"):
        return "endif"
    return None


def extract_free_function_decls(header_path: Path) -> List[Candidate]:
    raw = header_path.read_text(errors="ignore")

    # Track preprocessor conditions (best-effort)
    cond_stack: List[str] = []

    # Remove comments, but keep line structure for #if scanning
    no_comments = strip_comments(raw)
    lines = no_comments.splitlines(keepends=True)

    # Scope tracking: allow namespace scopes, exclude class/struct/enum and function bodies
    scope_stack: List[str] = []
    recent = ""

    stmt_buf: List[str] = []
    results: List[Candidate] = []

    def current_conditions() -> Tuple[str, ...]:
        # Filter out bookkeeping tokens
        return tuple(c for c in cond_stack if c not in ("else", "endif"))

    file_started = False
    for line in lines:
        cond = parse_condition_line(line)
        if cond is not None:
            # Heuristic: ignore a top-of-file header guard "#ifndef FOO_HH" style
            if not file_started and (line.lstrip().startswith("#ifndef ") or line.lstrip().startswith("#ifdef ")):
                # do not push header guard macros to the condition stack
                pass
            elif cond == "endif":
                if cond_stack:
                    cond_stack.pop()
            elif cond == "else":
                # keep marker to indicate we are in the else branch
                cond_stack.append("else")
            else:
                # #if/#ifdef/#ifndef/#elif (best effort: just push expression)
                cond_stack.append(cond)
            # Do not include preprocessor directive text into statement buffer
            continue
        # Skip other preprocessor directives (e.g., #define/#include/#pragma) so they don't
        # merge into the next statement.
        if line.lstrip().startswith("#"):
            continue

        if not file_started and line.strip():
            file_started = True

        i = 0
        while i < len(line):
            ch = line[i]
            stmt_buf.append(ch)
            recent = (recent + ch)[-200:]

            if ch == "{":
                # classify scope start
                before = recent[:-1]
                if re.search(r"\b(namespace)\b[^;{]*$", before):
                    scope_stack.append("namespace")
                elif re.search(r"\b(class|struct|enum)\b[^;{]*$", before):
                    scope_stack.append("type")
                else:
                    scope_stack.append("other")
            elif ch == "}":
                if scope_stack:
                    scope_stack.pop()
            elif ch == ";":
                stmt = "".join(stmt_buf).strip()
                stmt_buf.clear()

                # Only keep candidates that are not inside type/function-body scopes.
                if "type" in scope_stack or "other" in scope_stack:
                    i += 1
                    continue

                if looks_like_func_decl(stmt):
                    sig = clean_signature(stmt)
                    results.append(Candidate(signature=sig, conditions=current_conditions()))
            i += 1

    return results


def render_md(grouped: List[Tuple[str, List[Candidate]]]) -> str:
    out: List[str] = []
    for header_rel, decls in grouped:
        out.append(f"## `{header_rel}`")
        out.append("")
        if not decls:
            out.append("_（未发现自由函数声明）_")
            out.append("")
            continue

        # Group by conditions to keep noise low
        for cand in decls:
            cond = ""
            if cand.conditions:
                cond = "  *(条件: " + " && ".join(f"({c})" for c in cand.conditions) + ")*"
            out.append(f"- `{cand.signature}`{cond}")
        out.append("")
    return "\n".join(out).rstrip() + "\n"


def splice_into_template(template_text: str, generated_md: str) -> str:
    if AUTO_MARKER not in template_text:
        raise SystemExit(f"Template missing marker: {AUTO_MARKER}")
    prefix, _marker, _rest = template_text.partition(AUTO_MARKER)
    return prefix + AUTO_MARKER + "\n\n" + generated_md


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo-root", required=True, help="Path to repo root")
    ap.add_argument("--out", required=True, help="Output markdown path")
    args = ap.parse_args()

    repo_root = Path(args.repo_root).resolve()
    include_root = repo_root / "shm-lib" / "include"
    out_path = Path(args.out).resolve()

    if not include_root.is_dir():
        raise SystemExit(f"Include root not found: {include_root}")

    grouped: List[Tuple[str, List[Candidate]]] = []
    for header in iter_headers(include_root):
        rel = header.relative_to(repo_root).as_posix()
        decls = extract_free_function_decls(header)
        grouped.append((rel, decls))

    generated = render_md(grouped)

    # Keep the hand-written header section in the existing file, replace only the generated block.
    template = out_path.read_text() if out_path.exists() else (AUTO_MARKER + "\n")
    final = splice_into_template(template, generated)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(final)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
