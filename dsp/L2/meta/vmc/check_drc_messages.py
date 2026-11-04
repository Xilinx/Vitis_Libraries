# Copyright (C) 2026, Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#!/usr/bin/env python3
"""Static checker for DRC (error) messages in L2/meta validator modules.

For each <block>.py in the target directory, this script:

  1. Extracts the block's Keys from top-level `def validate_<KEY>(args)` and
     `def update_<KEY>(args)` functions (only signatures that take a single
     `args` parameter -- i.e. framework entry points) and from `args["..."]`
     subscript accesses.

  2. Walks every `isError(...)` / `com.isError(...)` call and for each message:

     (a) unknown_key -- reports any UPPER_CASE_WITH_UNDERSCORE token in the
         literal text of the message that is NOT one of the block's Keys
         (typical bug: `TP_DIM` instead of `TP_DIM_A`, `NUM_FRAMES` instead
         of `TP_NUM_FRAMES`).

     (b) undefined_interpolation -- for every `{expr}` placeholder in an
         f-string, verifies that every Name referenced by the expression is
         defined in the enclosing function scope (parameters + locals) or in
         the module scope. Module scope is computed recursively: names
         pulled in via `from X import *` ARE resolved by parsing the target
         module (cycle-safe, cached), so helpers like `k_mem_module_size`
         defined in `aie_common.py` and imported via `from aie_common import *`
         are correctly recognized.

     (c) possibly_truncated_key (--heuristic) -- reports bare uppercase
         tokens (no underscores) that are a suffix of a block Key, e.g.
         "SSR" where "TP_SSR" is the actual Key. Useful for catching
         "... SSR ({TP_SSR})"-style inconsistencies.

Well-known shared-helper modules (`aie_common.py`, `aie_common_fir.py`,
`aie_common_fir_updaters.py`, `fir_utils.py`) are audited for
`undefined_interpolation` only; the key-presence checks are suppressed for
them because they operate on their callers' Keys and would be pure noise.

Usage:
    python3 check_drc_messages.py [DIR] [--heuristic] [--json]
                                  [--include-subdirs] [--file FILE]
                                  [--min-keys N] [--utility NAME]

DIR defaults to the parent directory of this script (i.e. L2/meta).
Exit code is 0 when no errors are found (warnings do not fail the run).
"""

import argparse
import ast
import builtins
import json
import os
import re
import sys
from pathlib import Path

KEY_TOKEN = re.compile(r"\b([A-Z][A-Z0-9]*(?:_[A-Z0-9]+)+)\b")
BARE_UPPER = re.compile(r"\b([A-Z]{2,})\b")
BUILTIN_NAMES = set(dir(builtins))


class Issue(object):
    __slots__ = ("file", "line", "function", "severity", "kind",
                 "message_snippet", "detail")

    def __init__(self, file, line, function, severity, kind,
                 message_snippet, detail):
        self.file = file
        self.line = line
        self.function = function
        self.severity = severity
        self.kind = kind
        self.message_snippet = message_snippet
        self.detail = detail

    def as_dict(self):
        return {s: getattr(self, s) for s in self.__slots__}


def _str_value(node):
    """Return the string value if node is a string literal, else None.
    Handles both ast.Constant (py>=3.8) and legacy ast.Str (py<3.8)."""
    if isinstance(node, ast.Constant) and isinstance(node.value, str):
        return node.value
    if hasattr(ast, "Str") and isinstance(node, ast.Str):
        return node.s
    return None


def _subscript_key(node):
    """Return the string subscript key if node is `something["KEY"]`, else None.
    Handles the ast.Index wrapper used by Python <3.9."""
    if not isinstance(node, ast.Subscript):
        return None
    slc = node.slice
    if hasattr(ast, "Index") and isinstance(slc, ast.Index):
        slc = slc.value
    return _str_value(slc)


def _collect_targets(target, names):
    if isinstance(target, ast.Name):
        names.add(target.id)
    elif isinstance(target, (ast.Tuple, ast.List)):
        for el in target.elts:
            _collect_targets(el, names)
    elif isinstance(target, ast.Starred):
        _collect_targets(target.value, names)


def _is_block_entry_point(fn):
    """True if `fn` matches the metadata-framework entry signature
    `validate_X(args)` / `update_X(args)` -- a single positional `args`."""
    a = fn.args
    all_pos = list(a.args)
    if hasattr(a, "posonlyargs"):
        all_pos = list(a.posonlyargs) + all_pos
    return (len(all_pos) == 1 and all_pos[0].arg == "args"
            and not a.kwonlyargs and not a.vararg and not a.kwarg)


def extract_block_keys(tree):
    """Block Keys = parameters this file exposes as framework entry points.

    Only `def validate_<KEY>(args)` / `def update_<KEY>(args)` functions whose
    signature takes a single `args` parameter count. Helpers like
    `validate_LUT_len(lut_list, len_lut)` are intentionally excluded because
    they are not block-level entry points.

    Also unions in any `args["KEY"]` subscript accesses seen in the module.
    """
    keys = set()
    for node in tree.body:
        if isinstance(node, ast.FunctionDef):
            for prefix in ("validate_", "update_"):
                if (node.name.startswith(prefix)
                        and len(node.name) > len(prefix)
                        and node.name[len(prefix)].isupper()
                        and _is_block_entry_point(node)):
                    keys.add(node.name[len(prefix):])
    for node in ast.walk(tree):
        if isinstance(node, ast.Subscript) and isinstance(node.value, ast.Name) and node.value.id == "args":
            key = _subscript_key(node)
            if key is not None:
                keys.add(key)
    return keys


# Module-level name resolution cache keyed by resolved filesystem path.
# Avoids re-parsing shared modules like `aie_common.py` for every block.
_MODULE_NAMES_CACHE = {}


def _resolve_wildcard_target(module_name, search_dirs):
    """Best-effort resolve `from <module_name> import *` to a .py file path.

    Searches each directory in `search_dirs`, trying both a flat
    `<dir>/<module>.py` and a dotted `<dir>/<parts>/last.py` layout.
    Returns the first existing Path, or None.
    """
    if not module_name:
        return None
    parts = module_name.split(".")
    for d in search_dirs:
        cand = d / (module_name + ".py")
        if cand.is_file():
            return cand
        cand = d.joinpath(*parts[:-1], parts[-1] + ".py")
        if cand.is_file():
            return cand
    return None


def module_level_names(tree, file_path=None, _seen=None):
    """Names defined at the top level of this module, INCLUDING those pulled
    in through `from X import *` wildcard imports (resolved recursively
    relative to `file_path`'s directory).

    Cycle-safe via `_seen`; results cached per resolved path so that
    commonly-imported modules are parsed only once across an audit run.
    """
    if _seen is None:
        _seen = set()

    cache_key = None
    if file_path is not None:
        cache_key = Path(file_path).resolve()
        if cache_key in _seen:
            return set()
        _seen.add(cache_key)
        cached = _MODULE_NAMES_CACHE.get(cache_key)
        if cached is not None:
            return set(cached)

    names = set()
    search_dirs = []
    if cache_key is not None:
        search_dirs.append(cache_key.parent)

    for node in tree.body:
        if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef)):
            names.add(node.name)
        elif isinstance(node, ast.Assign):
            for t in node.targets:
                _collect_targets(t, names)
        elif isinstance(node, ast.AnnAssign) and isinstance(node.target, ast.Name):
            names.add(node.target.id)
        elif isinstance(node, ast.Import):
            for alias in node.names:
                names.add(alias.asname or alias.name.split(".")[0])
        elif isinstance(node, ast.ImportFrom):
            for alias in node.names:
                if alias.name == "*":
                    target = _resolve_wildcard_target(node.module, search_dirs)
                    if target is None:
                        continue
                    try:
                        sub_src = target.read_text(encoding="utf-8", errors="replace")
                        sub_tree = ast.parse(sub_src, filename=str(target))
                    except (OSError, SyntaxError):
                        continue
                    names |= module_level_names(sub_tree, target, _seen)
                else:
                    names.add(alias.asname or alias.name)

    if cache_key is not None:
        _MODULE_NAMES_CACHE[cache_key] = frozenset(names)
    return names


def fn_scope(fn):
    """Names visible inside a function: parameters + assigned locals."""
    names = set()
    a = fn.args
    all_args = list(a.args) + list(a.kwonlyargs)
    if hasattr(a, "posonlyargs"):
        all_args += list(a.posonlyargs)
    for arg in all_args:
        names.add(arg.arg)
    if a.vararg:
        names.add(a.vararg.arg)
    if a.kwarg:
        names.add(a.kwarg.arg)
    for node in ast.walk(fn):
        if isinstance(node, ast.Assign):
            for t in node.targets:
                _collect_targets(t, names)
        elif isinstance(node, ast.AnnAssign) and isinstance(node.target, ast.Name):
            names.add(node.target.id)
        elif isinstance(node, ast.AugAssign) and isinstance(node.target, ast.Name):
            names.add(node.target.id)
        elif isinstance(node, ast.For):
            _collect_targets(node.target, names)
        elif isinstance(node, ast.With):
            for item in node.items:
                if item.optional_vars is not None:
                    _collect_targets(item.optional_vars, names)
        elif isinstance(node, ast.comprehension):
            _collect_targets(node.target, names)
    return names


def extract_message(node):
    """Reduce a message expression to literal text + list of interpolated exprs.

    Returns {'literal': str, 'interps': [ast.expr]} or None when the message
    is not statically analyzable (e.g. .format() / + with a Call / etc).
    """
    lit_parts = []
    interps = []

    def walk(n):
        if isinstance(n, ast.JoinedStr):
            for v in n.values:
                if not walk(v):
                    return False
            return True
        if isinstance(n, ast.FormattedValue):
            lit_parts.append("{}")
            interps.append(n.value)
            return True
        s = _str_value(n)
        if s is not None:
            lit_parts.append(s)
            return True
        if isinstance(n, ast.BinOp) and isinstance(n.op, ast.Add):
            return walk(n.left) and walk(n.right)
        return False

    if not walk(node):
        return None
    return {"literal": "".join(lit_parts), "interps": interps}


def find_isError_calls(tree):
    """Yield (enclosing_FunctionDef_or_None, Call_node, first_arg_node)."""
    results = []
    stack = []

    class V(ast.NodeVisitor):
        def visit_FunctionDef(self, n):
            stack.append(n)
            self.generic_visit(n)
            stack.pop()

        visit_AsyncFunctionDef = visit_FunctionDef

        def visit_Call(self, n):
            f = n.func
            hit = False
            if isinstance(f, ast.Name) and f.id == "isError":
                hit = True
            elif (isinstance(f, ast.Attribute)
                  and f.attr == "isError"
                  and isinstance(f.value, ast.Name)
                  and f.value.id == "com"):
                hit = True
            if hit and n.args:
                enclosing = stack[-1] if stack else None
                results.append((enclosing, n, n.args[0]))
            self.generic_visit(n)

    V().visit(tree)
    return results


def _nearest(tok, keys):
    """Closest block key to `tok` by substring / length heuristic, or None."""
    candidates = [k for k in keys if tok in k or k in tok]
    if not candidates:
        return None
    candidates.sort(key=lambda k: (abs(len(k) - len(tok)), k))
    return candidates[0]


def audit_file(path, heuristic, min_keys=3, utility_names=()):
    src = path.read_text(encoding="utf-8", errors="replace")
    try:
        tree = ast.parse(src, filename=str(path))
    except SyntaxError as e:
        return [Issue(str(path), e.lineno or 0, "<parse>",
                      "error", "parse_error", "", str(e))]

    block_keys = extract_block_keys(tree)
    module_names = module_level_names(tree, path)
    # Files marked as utility (either explicitly via --utility or implicitly
    # by having fewer than `min_keys` block entry points) are audited for
    # undefined_interpolation only -- running unknown_key / truncated_key on
    # shared helpers that operate on the caller's Keys would be pure noise.
    is_utility = (path.name in utility_names
                  or path.resolve().name in utility_names
                  or len(block_keys) < min_keys)
    has_block_keys = bool(block_keys) and not is_utility

    issues = []
    for enclosing, call_node, msg_node in find_isError_calls(tree):
        fn_name = enclosing.name if enclosing else "<module>"
        scope = fn_scope(enclosing) if enclosing else set()
        parsed = extract_message(msg_node)
        if parsed is None:
            continue

        literal = parsed["literal"]
        interps = parsed["interps"]
        snippet = literal if len(literal) < 160 else literal[:157] + "..."

        if has_block_keys:
            for tok in sorted(set(KEY_TOKEN.findall(literal))):
                if tok in block_keys or tok in module_names:
                    continue
                suggestion = _nearest(tok, block_keys)
                sug = " (did you mean %s?)" % suggestion if suggestion else ""
                issues.append(Issue(
                    str(path), call_node.lineno, fn_name, "error",
                    "unknown_key", snippet,
                    "Token '%s' in DRC message is not a Key of this block%s." % (tok, sug),
                ))

        for expr in interps:
            for n in ast.walk(expr):
                if isinstance(n, ast.Name):
                    if (n.id in scope or n.id in module_names
                            or n.id in BUILTIN_NAMES):
                        continue
                    issues.append(Issue(
                        str(path), call_node.lineno, fn_name, "error",
                        "undefined_interpolation", snippet,
                        "Interpolated name '%s' in DRC message is not defined "
                        "in function scope or module scope." % n.id,
                    ))

        if heuristic and has_block_keys:
            bare = {t for t in BARE_UPPER.findall(literal)
                    if "_" not in t and len(t) >= 3}
            for tok in sorted(bare):
                if tok in block_keys or tok in module_names or tok in BUILTIN_NAMES:
                    continue
                matches = [k for k in block_keys if k.endswith("_" + tok) or k == tok]
                if matches:
                    # Prefer the shortest / most direct match
                    # (TP_SSR over TP_USE_COEFF_RELOAD_NON_SSR).
                    matches.sort(key=lambda k: (len(k), k))
                    issues.append(Issue(
                        str(path), call_node.lineno, fn_name, "warning",
                        "possibly_truncated_key", snippet,
                        "Bare token '%s' may be a truncated reference to "
                        "block Key '%s'." % (tok, matches[0]),
                    ))

    return issues


def main(argv=None):
    p = argparse.ArgumentParser(
        description="Static checker for DRC error messages in L2/meta validators."
    )
    p.add_argument(
        "directory", nargs="?",
        default=str(Path(__file__).resolve().parent.parent),
        help="Directory containing the L2/meta/*.py files (default: parent of this script).",
    )
    p.add_argument("--heuristic", action="store_true",
                   help="Also flag bare uppercase tokens that may be truncated keys "
                        "(e.g. 'SSR' vs 'TP_SSR').")
    p.add_argument("--json", dest="as_json", action="store_true",
                   help="Emit machine-readable JSON instead of text.")
    p.add_argument("--include-subdirs", action="store_true",
                   help="Recurse into subdirectories.")
    p.add_argument("--file", action="append", default=[],
                   help="Audit only the given file(s); may be repeated.")
    p.add_argument("--min-keys", type=int, default=3,
                   help="Treat files with fewer than this many block entry "
                        "points as utility modules and skip the 'unknown_key' "
                        "and 'possibly_truncated_key' checks on them "
                        "(default: 3). 'undefined_interpolation' always runs.")
    p.add_argument("--utility", action="append", default=[
                       "aie_common.py", "aie_common_fir.py",
                       "aie_common_fir_updaters.py", "fir_utils.py",
                   ],
                   help="Basename of a shared-helper module to exclude from "
                        "the unknown-key checks. May be repeated. Defaults "
                        "cover the well-known shared modules; pass the flag "
                        "to add more.")
    args = p.parse_args(argv)

    if args.file:
        files = [Path(f).resolve() for f in args.file]
    else:
        root = Path(args.directory).resolve()
        if not root.is_dir():
            print("error: %s is not a directory" % root, file=sys.stderr)
            return 2
        pattern = "**/*.py" if args.include_subdirs else "*.py"
        files = sorted(root.glob(pattern))

    me = Path(__file__).resolve()
    files = [f for f in files if f.resolve() != me]

    utility_names = set(args.utility)
    all_issues = []
    for f in files:
        all_issues.extend(audit_file(f, heuristic=args.heuristic,
                                     min_keys=args.min_keys,
                                     utility_names=utility_names))

    if args.as_json:
        json.dump([i.as_dict() for i in all_issues], sys.stdout, indent=2)
        sys.stdout.write("\n")
        return 1 if any(i.severity == "error" for i in all_issues) else 0

    by_file = {}
    for i in all_issues:
        by_file.setdefault(i.file, []).append(i)

    total_errors = sum(1 for i in all_issues if i.severity == "error")
    total_warnings = sum(1 for i in all_issues if i.severity == "warning")

    if not all_issues:
        print("No DRC message issues found across %d file(s)." % len(files))
        return 0

    for f, items in sorted(by_file.items()):
        print("\n=== %s ===" % f)
        for i in items:
            print("  [%-7s] %-24s line %5d  in %s"
                  % (i.severity, i.kind, i.line, i.function))
            print("             detail : %s" % i.detail)
            if i.message_snippet:
                print("             message: %r" % i.message_snippet)

    print("\nSummary: %d error(s), %d warning(s) across %d file(s) / %d scanned."
          % (total_errors, total_warnings, len(by_file), len(files)))
    return 1 if total_errors else 0


if __name__ == "__main__":
    sys.exit(main())
