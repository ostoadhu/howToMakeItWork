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
        state.setdefault("parent", None)
        state.setdefault("initial", None)
    for name, state in states.items():
        parent = state["parent"]
        if parent is not None and parent not in states:
            raise SpecError(f"state '{name}': unknown parent '{parent}'")
        seen = {name}
        while parent is not None:
            if parent in seen:
                raise SpecError(f"state '{name}': parent cycle")
            seen.add(parent)
            parent = states[parent]["parent"]
    for name, state in states.items():
        children = [n for n, s in states.items() if s["parent"] == name]
        if children and state["kind"] != "normal":
            raise SpecError(f"state '{name}': only normal states can contain sub-states")
        if children and state["initial"] not in children:
            raise SpecError(f"composite state '{name}': 'initial' must be one of {children}")
        if not children and state["initial"] is not None:
            raise SpecError(f"state '{name}': 'initial' set but it has no sub-states")
    if states[initial]["parent"] is not None:
        raise SpecError(f"initial state '{initial}' must be a top-level state")
    for i, t in enumerate(transitions, 1):
        for key in ("from", "to"):
            if t.get(key) != PSEUDO and t.get(key) not in states:
                raise SpecError(f"transition {i}: unknown state '{t.get(key)}' in '{key}'")
        if t["from"] == PSEUDO:
            raise SpecError(f"transition {i}: '[*]' can only be a target")
        to_parent = states[t["to"]]["parent"] if t["to"] != PSEUDO else states[t["from"]]["parent"]
        if states[t["from"]]["parent"] != to_parent:
            raise SpecError(f"transition {i}: '{t['from']}' and '{t['to']}' are not in the same "
                            "composite state; connect the composite states instead")
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


def state_label(name: str, s: dict) -> str:
    label = name
    if s["use_cases"]:
        label += " (" + ", ".join(s["use_cases"]) + ")"
    if not s["defined"]:
        label += " - not defined yet"
    return label


def render_scope(spec: dict, parent: str | None, indent: str) -> list[str]:
    """Renders the states and transitions directly inside `parent` (None = top level)."""
    states, transitions = spec["states"], spec["transitions"]
    lines = []
    initial = spec["initial"] if parent is None else states[parent]["initial"]
    lines.append(f"{indent}[*] --> {initial}")
    for name, s in states.items():
        if s["parent"] != parent:
            continue
        if s["kind"] == "choice":
            lines.append(f"{indent}state {name} <<choice>>")
            continue
        if any(c["parent"] == name for c in states.values()):
            # Composite: old Mermaid versions cannot combine `state "label" as X`
            # with a `{ }` body, so the label/description go into a note.
            lines.append(f"{indent}state {name} {{")
            lines += render_scope(spec, name, indent + "    ")
            lines.append(f"{indent}}}")
            note = mermaid_text(state_label(name, s) + (f" - {s['description']}" if s["description"] else ""))
            lines.append(f"{indent}note right of {name} : {note}")
            continue
        lines.append(f"{indent}state \"{mermaid_text(state_label(name, s))}\" as {name}")
        if s["description"]:
            lines.append(f"{indent}{name} : {mermaid_text(s['description'])}")
    for t in transitions:
        scope = states[t["from"]]["parent"]
        if scope != parent:
            continue
        event = f" : {mermaid_text(t['event'])}" if t["event"] else ""
        lines.append(f"{indent}{t['from']} --> {t['to']}{event}")
    return lines


def state_path(states: dict, name: str) -> str:
    parts = [name]
    while states[parts[-1]]["parent"] is not None:
        parts.append(states[parts[-1]]["parent"])
    return " / ".join(reversed(parts))


def render(spec: dict) -> str:
    states, transitions = spec["states"], spec["transitions"]

    lines = ["```mermaid", "stateDiagram-v2"]
    lines += render_scope(spec, None, "    ")
    # No classDef/class styling: older Mermaid versions (e.g. 8.x/9.1) cannot render it.
    lines.append("```")

    lines += ["", "| State | Use cases | Description |", "|---|---|---|"]
    for name, s in states.items():
        desc = s["description"] or ("decision" if s["kind"] == "choice" else "")
        if s["initial"]:
            desc += f" (composite, starts in {s['initial']})"
        if not s["defined"]:
            desc += " *(not defined yet)*"
        ucs = ", ".join(s["use_cases"]) or "–"
        lines.append(f"| {state_path(states, name)} | {ucs} | {desc.strip()} |")

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
