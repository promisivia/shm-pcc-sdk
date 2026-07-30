#!/usr/bin/env python3
"""Dependency-light open-source readiness checks for CXL-SDK."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
from typing import Iterable
from urllib.parse import unquote


ROOT = Path(__file__).resolve().parents[2]


class Harness:
    def __init__(self) -> None:
        self.results: list[dict[str, str]] = []

    def add(self, status: str, name: str, detail: str = "") -> None:
        self.results.append({"status": status, "name": name, "detail": detail})
        symbol = {"PASS": "✓", "WARN": "!", "FAIL": "✗"}[status]
        print(f"{symbol} {status:<4} {name}")
        if detail:
            for line in detail.splitlines():
                print(f"         {line}")

    def pass_(self, name: str, detail: str = "") -> None:
        self.add("PASS", name, detail)

    def warn(self, name: str, detail: str = "") -> None:
        self.add("WARN", name, detail)

    def fail(self, name: str, detail: str = "") -> None:
        self.add("FAIL", name, detail)

    def summary(self) -> tuple[int, int, int]:
        counts = tuple(
            sum(result["status"] == status for result in self.results)
            for status in ("PASS", "WARN", "FAIL")
        )
        print("\nSummary: " + ", ".join(
            f"{label.lower()}={count}"
            for label, count in zip(("PASS", "WARN", "FAIL"), counts)
        ))
        return counts


def run(*args: str, cwd: Path = ROOT, timeout: int = 120) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        args,
        cwd=cwd,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=timeout,
        check=False,
    )


def candidate_files() -> list[Path]:
    result = run(
        "git", "ls-files", "--cached", "--others", "--exclude-standard", "-z"
    )
    if result.returncode != 0:
        raise RuntimeError(result.stdout.strip() or "git ls-files failed")
    return [ROOT / value for value in result.stdout.split("\0") if value]


def text_content(path: Path) -> str | None:
    try:
        data = path.read_bytes()
    except OSError:
        return None
    if b"\0" in data[:8192]:
        return None
    try:
        return data.decode("utf-8")
    except UnicodeDecodeError:
        return None


def relative(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def check_required_files(harness: Harness) -> None:
    required = [
        "README.md",
        "LICENSE",
        "CHANGELOG.md",
        "CONTRIBUTING.md",
        "CODE_OF_CONDUCT.md",
        "SECURITY.md",
        "SUPPORT.md",
        "THIRD_PARTY_NOTICES.md",
        "OPEN_SOURCE_READINESS.md",
        ".gitignore",
    ]
    missing = [name for name in required if not (ROOT / name).is_file()]
    if missing:
        harness.fail("community and release files", "missing: " + ", ".join(missing))
    else:
        harness.pass_("community and release files", f"{len(required)} required files present")


def check_repository_layout(harness: Harness) -> None:
    required = ["shm-lib", "ds", "malloc", "tests", "demos", "website"]
    missing = [name for name in required if not (ROOT / name).is_dir()]
    if missing:
        harness.fail("repository layout", "missing: " + ", ".join(missing))
    else:
        harness.pass_("repository layout")


def check_submodule_metadata(harness: Harness) -> None:
    result = run("git", "ls-files", "-s")
    gitlinks = [
        line.split("\t", 1)[1]
        for line in result.stdout.splitlines()
        if line.startswith("160000 ") and "\t" in line
    ]
    metadata = (ROOT / ".gitmodules").is_file()
    if metadata and not gitlinks:
        harness.fail("submodule metadata", ".gitmodules exists but the tree has no gitlinks")
    elif gitlinks and not metadata:
        harness.fail("submodule metadata", "gitlinks exist but .gitmodules is missing")
    else:
        detail = f"{len(gitlinks)} configured submodules" if gitlinks else "no submodules"
        harness.pass_("submodule metadata", detail)


def check_tracked_artifacts(harness: Harness, files: Iterable[Path]) -> None:
    artifact_patterns = [
        re.compile(r"(?:^|/)__pycache__(?:/|$)"),
        re.compile(r"\.(?:o|a|so|dylib|dll|exe|pyc)$", re.IGNORECASE),
        re.compile(r"\.tar(?:\.[0-9]+|[0-9]+)?$", re.IGNORECASE),
        re.compile(r"(?:^|/)compile_commands\.json$"),
    ]
    found: list[str] = []
    lfs_pointers: list[str] = []
    elf_files: list[str] = []
    large_files: list[str] = []
    for path in files:
        if not path.is_file():
            continue
        name = relative(path)
        if any(pattern.search(name) for pattern in artifact_patterns):
            found.append(name)
        try:
            header = path.read_bytes()[:160]
            size = path.stat().st_size
        except OSError:
            continue
        if header.startswith(b"\x7fELF"):
            elf_files.append(name)
        if header.startswith(b"version https://git-lfs.github.com/spec/v1"):
            lfs_pointers.append(name)
        if size >= 10 * 1024 * 1024:
            large_files.append(f"{size / 1024 / 1024:.1f} MiB  {name}")

    combined = sorted(set(found + elf_files + lfs_pointers))
    if combined:
        harness.fail("generated and packaged artifacts", "\n".join(combined))
    else:
        harness.pass_("generated and packaged artifacts")
    if large_files:
        harness.warn("large tracked source or workload files", "\n".join(sorted(large_files)))
    else:
        harness.pass_("large tracked source or workload files")


MARKDOWN_LINK = re.compile(r"!?\[[^\]]+\]\(([^)]+)\)")


def documentation_files(files: Iterable[Path]) -> list[Path]:
    selected: list[Path] = []
    root_names = {
        "README.md", "CHANGELOG.md", "CONTRIBUTING.md", "CODE_OF_CONDUCT.md",
        "SECURITY.md", "SUPPORT.md", "THIRD_PARTY_NOTICES.md",
        "OPEN_SOURCE_READINESS.md",
    }
    for path in files:
        name = relative(path)
        if name in root_names or name.startswith("website/zh/docs/") or name.startswith("website/en/"):
            if path.suffix.lower() == ".md" and path.is_file():
                selected.append(path)
    return selected


def normalize_markdown_target(raw: str) -> str:
    target = raw.strip()
    if target.startswith("<") and target.endswith(">"):
        target = target[1:-1]
    if " " in target and not target.startswith(("http://", "https://")):
        target = target.split(" ", 1)[0]
    return unquote(target.split("#", 1)[0])


def check_markdown_links(harness: Harness, docs: Iterable[Path]) -> None:
    missing: list[str] = []
    for path in docs:
        content = text_content(path)
        if content is None:
            continue
        for match in MARKDOWN_LINK.finditer(content):
            target = normalize_markdown_target(match.group(1))
            if not target or target.startswith(("#", "http://", "https://", "mailto:")):
                continue
            if "{{" in target or "{%" in target:
                continue
            resolved = (path.parent / target).resolve()
            try:
                resolved.relative_to(ROOT)
            except ValueError:
                missing.append(f"{relative(path)} -> {target} (outside repository)")
                continue
            if not resolved.exists():
                missing.append(f"{relative(path)} -> {target}")
    if missing:
        harness.fail("local Markdown links", "\n".join(sorted(set(missing))))
    else:
        harness.pass_("local Markdown links")


def check_documentation_placeholders(harness: Harness, docs: Iterable[Path]) -> None:
    patterns = {
        "old project name": re.compile(r"SHM-PCC-SDK"),
        "placeholder repository": re.compile(r"github\.com/your-org/"),
        "unfinished placeholder": re.compile(r"\[Add [^\]]+ here\]", re.IGNORECASE),
    }
    findings: list[str] = []
    for path in docs:
        content = text_content(path)
        if content is None:
            continue
        for label, pattern in patterns.items():
            for match in pattern.finditer(content):
                line = content.count("\n", 0, match.start()) + 1
                findings.append(f"{relative(path)}:{line}: {label}")
    if findings:
        harness.fail("documentation placeholders and branding", "\n".join(findings))
    else:
        harness.pass_("documentation placeholders and branding")


def check_secrets(harness: Harness, files: Iterable[Path]) -> None:
    patterns = [
        re.compile(r"-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----"),
        re.compile(r"\bghp_[A-Za-z0-9]{20,}\b"),
        re.compile(r"\bgithub_pat_[A-Za-z0-9_]{20,}\b"),
        re.compile(r"\bAKIA[0-9A-Z]{16}\b"),
        re.compile(r"https?://[^/@\s:]+:[^/@\s]+@"),
    ]
    findings: list[str] = []
    for path in files:
        if not path.is_file() or path.stat().st_size > 5 * 1024 * 1024:
            continue
        content = text_content(path)
        if content is None:
            continue
        for pattern in patterns:
            match = pattern.search(content)
            if match:
                line = content.count("\n", 0, match.start()) + 1
                findings.append(f"{relative(path)}:{line}")
                break
    if findings:
        harness.fail("conservative secret scan", "\n".join(findings))
    else:
        harness.pass_("conservative secret scan")


def check_shell_syntax(harness: Harness) -> None:
    scripts = [
        "verify_docs.sh",
        "demos/build.sh",
        "website/serve.sh",
        "tests/YCSB-C/build.sh",
        "tests/YCSB-C/run_shm_ds.sh",
        "malloc/lsmalloc/build.sh",
        "tests/correctness/build_and_test.sh",
        "tools/opensource-harness/run.sh",
    ]
    failures: list[str] = []
    for name in scripts:
        path = ROOT / name
        if not path.is_file():
            continue
        result = run("bash", "-n", str(path))
        if result.returncode != 0:
            failures.append(f"{name}: {result.stdout.strip()}")
    if failures:
        harness.fail("maintained shell script syntax", "\n".join(failures))
    else:
        harness.pass_("maintained shell script syntax")


def check_diff_whitespace(harness: Harness) -> None:
    result = run("git", "diff", "--check")
    if result.returncode:
        harness.fail("Git whitespace errors", result.stdout.strip())
    else:
        harness.pass_("Git whitespace errors")


def last_output(output: str, lines: int = 30) -> str:
    return "\n".join(output.rstrip().splitlines()[-lines:])


def check_core_build(harness: Harness) -> None:
    with tempfile.TemporaryDirectory(prefix="cxl-sdk-harness-") as directory:
        build = Path(directory) / "shm-lib"
        configure = run("cmake", "-S", "shm-lib", "-B", str(build), timeout=180)
        if configure.returncode:
            harness.fail("core CMake configure", last_output(configure.stdout))
            return
        harness.pass_("core CMake configure")
        build_result = run("cmake", "--build", str(build), "--parallel", timeout=600)
        if build_result.returncode:
            harness.fail("core library build", last_output(build_result.stdout))
        else:
            harness.pass_("core library build")


def check_demos_build(harness: Harness) -> None:
    with tempfile.TemporaryDirectory(prefix="cxl-sdk-demos-") as directory:
        configure = run("cmake", "-S", "demos", "-B", directory, timeout=180)
        if configure.returncode:
            harness.fail("demo CMake configure", last_output(configure.stdout))
            return
        harness.pass_("demo CMake configure")
        build_result = run("cmake", "--build", directory, "--parallel", timeout=600)
        if build_result.returncode:
            harness.fail("demo build", last_output(build_result.stdout))
        else:
            harness.pass_("demo build")


def check_paper_index_build(harness: Harness) -> None:
    with tempfile.TemporaryDirectory(prefix="cxl-sdk-ycsb-") as directory:
        configure = run(
            "cmake", "-S", "tests/YCSB-C", "-B", directory,
            "-DVARIANT=nocc", timeout=240,
        )
        if configure.returncode:
            harness.fail("paper index adapters configure", last_output(configure.stdout))
            return
        harness.pass_("paper index adapters configure", "YCSB-C NO_CC variant")
        build_result = run("cmake", "--build", directory, "--parallel", timeout=900)
        if build_result.returncode:
            harness.fail("paper index adapters build", last_output(build_result.stdout))
        else:
            harness.pass_("paper index adapters build", "BTreeOLC, ART, Masstree, BwTree, CLHT, HOT, and ClevelHash")


def check_g2_stress(harness: Harness) -> None:
    source_dir = ROOT / "tests/correctness/help_update_verify"
    with tempfile.TemporaryDirectory(prefix="cxl-sdk-g2-") as directory:
        binary = Path(directory) / "test_linearizability"
        compile_result = run(
            "c++", "-std=c++17", "-Wall", "-Wextra", "-O2", "-pthread",
            str(source_dir / "test_linearizability.cc"), "-o", str(binary),
            cwd=source_dir, timeout=180,
        )
        if compile_result.returncode:
            harness.fail("G2 replicated-pointer stress build", last_output(compile_result.stdout))
            return
        harness.pass_("G2 replicated-pointer stress build")
        test_result = run(str(binary), cwd=source_dir, timeout=60)
        if test_result.returncode:
            harness.fail("G2 replicated-pointer stress run", last_output(test_result.stdout, 50))
        else:
            harness.pass_("G2 replicated-pointer stress run", last_output(test_result.stdout, 8))


def check_documentation_build(harness: Harness) -> None:
    if shutil.which("sphinx-build") is None:
        harness.warn("Sphinx documentation build", "sphinx-build not installed; install website/requirements.txt")
        return
    with tempfile.TemporaryDirectory(prefix="cxl-sdk-docs-") as directory:
        result = run(
            "sphinx-build", "-n", "-W", "-b", "html",
            "website/zh/docs", directory, timeout=300,
        )
        if result.returncode:
            harness.fail("Sphinx documentation build", last_output(result.stdout, 50))
        else:
            harness.pass_("Sphinx documentation build")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--full", action="store_true", help="also build the core library and documentation")
    parser.add_argument("--report", type=Path, help="write a JSON report to this path")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    os.chdir(ROOT)
    harness = Harness()
    try:
        files = candidate_files()
    except RuntimeError as error:
        print(f"fatal: {error}", file=sys.stderr)
        return 2
    docs = documentation_files(files)

    print(f"CXL-SDK open-source readiness harness ({'full' if args.full else 'quick'})\n")
    check_required_files(harness)
    check_repository_layout(harness)
    check_submodule_metadata(harness)
    check_tracked_artifacts(harness, files)
    check_markdown_links(harness, docs)
    check_documentation_placeholders(harness, docs)
    check_secrets(harness, files)
    check_shell_syntax(harness)
    check_diff_whitespace(harness)
    if args.full:
        check_core_build(harness)
        check_demos_build(harness)
        check_paper_index_build(harness)
        check_g2_stress(harness)
        check_documentation_build(harness)

    passed, warnings, failures = harness.summary()
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(
            json.dumps(
                {"mode": "full" if args.full else "quick", "pass": passed,
                 "warn": warnings, "fail": failures, "results": harness.results},
                indent=2,
            ) + "\n",
            encoding="utf-8",
        )
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
