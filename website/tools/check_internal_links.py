#!/usr/bin/env python3
"""Check generated static-site links without requiring third-party packages."""

from __future__ import annotations

import argparse
import posixpath
from html.parser import HTMLParser
from pathlib import Path, PurePosixPath
from urllib.parse import unquote, urlsplit


class PageParser(HTMLParser):
    def __init__(self) -> None:
        super().__init__()
        self.references: list[str] = []
        self.anchors: set[str] = set()

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        values = dict(attrs)
        if values.get("id"):
            self.anchors.add(values["id"] or "")
        attribute = "src" if tag in {"script", "img", "source"} else "href"
        if tag in {"a", "link", "script", "img", "source"} and values.get(attribute):
            self.references.append(values[attribute] or "")


def parse_page(path: Path) -> PageParser:
    parser = PageParser()
    parser.feed(path.read_text(encoding="utf-8"))
    return parser


def resolve_target(page: PurePosixPath, reference: str, base_path: str) -> tuple[PurePosixPath, str] | str | None:
    parsed = urlsplit(reference)
    if parsed.scheme or parsed.netloc or reference.startswith("//"):
        return None

    path = unquote(parsed.path)
    directory_target = path.endswith("/")
    if path.startswith("/"):
        if not path.startswith(base_path):
            return f"escapes deployment base {base_path}: {reference}"
        path = path[len(base_path) :]
    else:
        path = posixpath.normpath(str(page.parent / path)) if path else str(page)

    if path in {"", "."}:
        path = "index.html"
    if directory_target:
        path = posixpath.join(path, "index.html")

    return PurePosixPath(path), unquote(parsed.fragment)


def main() -> int:
    argument_parser = argparse.ArgumentParser()
    argument_parser.add_argument("site_dir", type=Path)
    argument_parser.add_argument("--base-path", default="/")
    args = argument_parser.parse_args()

    site_dir = args.site_dir.resolve()
    base_path = "/" + args.base_path.strip("/") + "/" if args.base_path != "/" else "/"
    pages = {PurePosixPath(path.relative_to(site_dir).as_posix()): parse_page(path) for path in site_dir.rglob("*.html")}
    errors: list[str] = []

    for page, parsed_page in sorted(pages.items(), key=lambda item: str(item[0])):
        for reference in parsed_page.references:
            resolved = resolve_target(page, reference, base_path)
            if resolved is None:
                continue
            if isinstance(resolved, str):
                errors.append(f"{page}: {resolved}")
                continue

            target, fragment = resolved
            target_path = site_dir / target
            if not target_path.is_file():
                errors.append(f"{page}: missing target {reference} -> {target}")
                continue
            if fragment and target.suffix == ".html":
                target_page = pages.get(target)
                if target_page is not None and fragment not in target_page.anchors:
                    errors.append(f"{page}: missing anchor {reference}")

    if errors:
        print("Internal link check failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print(f"Checked {len(pages)} HTML pages: all internal links and anchors are valid.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
