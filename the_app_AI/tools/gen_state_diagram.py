#!/usr/bin/env python3
"""Generates the state machine diagram of the_app_AI.

Reads docs/state_machine.yaml (the single source of states and transitions)
and writes a Mermaid diagram plus transition table into the generated
section of docs/use_cases.md. GitHub renders the Mermaid block as a picture.

    python the_app_AI/tools/gen_state_diagram.py           # regenerate
    python the_app_AI/tools/gen_state_diagram.py --check   # fail if out of date

Requires PyYAML (pip install pyyaml / apt install python3-yaml).
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

try:
    import yaml
except ImportError:
    sys.exit("error: PyYAML is required (pip install pyyaml / apt install python3-yaml)")

APP_DIR = Path(__file__).resolve().parent.parent
SPEC = APP_DIR / "docs" / "state_machine.yaml"
DOC = APP_DIR / "docs" / "use_cases.md"

BEGIN = "<!-- BEGIN GENERATED STATE MACHINE -->"
END = "<!-- END GENERATED STATE MACHINE -->"
PSEUDO = "[*]"
KINDS = {"normal", "choice", "error"}


class SpecError(Exception):
    pass


def load_spec(path: Path) -> dict:
    spec = yaml.safe_load(path.read_text(encoding="utf-8")) or {}
    states = spec.get("states") or {}
    transitions = spec.get("transitions") or []
    initial = spec.get("initial")

    if not states:
        raise SpecError("no states defined")
    if initial not in states:
        raise SpecError(f"initial state '{initial}' is not defined")
    for name in list(states):
        if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
            raise SpecError(f"invalid state name '{name}' (letters, digits, _ only)")
        state = states[name] = states[name] or {}
        kind = state.setdefault("kind", "normal")
        if kind not in KINDS:
            raise SpecError(f"state '{name}': unknown kind '{kind}'")
        state.setdefault("use_cases", [])
        state.setdefault("description", "")
        state.setdefault("defined", True)
    for i, t in enumerate(transitions, 1):
        for key in ("from", "to"):
            if t.get(key) != PSEUDO and t.get(key) not in states:
                raise SpecError(f"transition {i}: unknown state '{t.get(key)}' in '{key}'")
        t.setdefault("event", "")
    return {"initial": initial, "states": states, "transitions": transitions}


def check_use_cases(spec: dict, doc_text: str) -> None:
    documented = set(re.findall(r"^## (UseCase_\d+):", doc_text, flags=re.M))
    referenced = {uc for s in spec["states"].values() for uc in s["use_cases"]}
    unknown = sorted(referenced - documented)
    if unknown:
        raise SpecError(f"states reference use cases not in use_cases.md: {', '.join(unknown)}")
    missing = sorted(documented - referenced, key=lambda u: int(u.split("_")[1]))
    if missing:
        raise SpecError(
            f"use cases not covered by any state in state_machine.yaml: {', '.join(missing)}"
        )


def mermaid_text(text: str) -> str:
    # ':' and ';' are separators in Mermaid state diagrams.
    return re.sub(r"\s+", " ", str(text)).replace(":", " -").replace(";", ",").strip()


def render(spec: dict) -> str:
    states, transitions = spec["states"], spec["transitions"]

    lines = ["```mermaid", "stateDiagram-v2", f"    [*] --> {spec['initial']}"]
    for name, s in states.items():
        if s["kind"] == "choice":
            lines.append(f"    state {name} <<choice>>")
            continue
        label = name
        if s["use_cases"]:
            label += " (" + ", ".join(s["use_cases"]) + ")"
        lines.append(f"    state \"{mermaid_text(label)}\" as {name}")
        if s["description"]:
            lines.append(f"    {name} : {mermaid_text(s['description'])}")
    for t in transitions:
        event = f" : {mermaid_text(t['event'])}" if t["event"] else ""
        lines.append(f"    {t['from']} --> {t['to']}{event}")
    undefined = [n for n, s in states.items() if not s["defined"]]
    errors = [n for n, s in states.items() if s["kind"] == "error"]
    if undefined:
        lines.append("    classDef undefined stroke-dasharray: 5 5")
        lines.append(f"    class {','.join(undefined)} undefined")
    if errors:
        lines.append("    classDef error stroke:#c00,color:#c00")
        lines.append(f"    class {','.join(errors)} error")
    lines.append("```")

    lines += ["", "| State | Use cases | Description |", "|---|---|---|"]
    for name, s in states.items():
        desc = s["description"] or ("decision" if s["kind"] == "choice" else "")
        if not s["defined"]:
            desc += " *(not defined yet)*"
        ucs = ", ".join(s["use_cases"]) or "–"
        lines.append(f"| {name} | {ucs} | {desc.strip()} |")

    lines += ["", "| From | Event / guard | To |", "|---|---|---|"]
    for t in transitions:
        lines.append(f"| {t['from']} | {t['event']} | {t['to']} |")

    header = [
        BEGIN,
        "<!-- Generated from docs/state_machine.yaml by tools/gen_state_diagram.py."
        " Do not edit by hand. -->",
        "",
    ]
    return "\n".join(header + lines + ["", END])


def regenerate(doc_text: str, section: str) -> str:
    pattern = re.compile(re.escape(BEGIN) + r".*?" + re.escape(END), flags=re.S)
    if not pattern.search(doc_text):
        raise SpecError(f"markers {BEGIN} / {END} not found in {DOC.name}")
    return pattern.sub(lambda _: section, doc_text, count=1)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true",
                        help="do not write; exit 1 if use_cases.md is out of date")
    args = parser.parse_args()

    raw = DOC.read_bytes().decode("utf-8")
    newline = "\r\n" if "\r\n" in raw else "\n"
    doc_text = raw.replace("\r\n", "\n")

    try:
        spec = load_spec(SPEC)
        check_use_cases(spec, doc_text)
        updated = regenerate(doc_text, render(spec))
    except (SpecError, yaml.YAMLError) as e:
        print(f"error: {e}", file=sys.stderr)
        return 2

    if updated == doc_text:
        print("state machine diagram is up to date")
        return 0
    if args.check:
        print("error: state machine diagram in docs/use_cases.md is out of date; run "
              "python the_app_AI/tools/gen_state_diagram.py", file=sys.stderr)
        return 1
    DOC.write_bytes(updated.replace("\n", newline).encode("utf-8"))
    print(f"regenerated state machine diagram in {DOC.relative_to(APP_DIR.parent)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
