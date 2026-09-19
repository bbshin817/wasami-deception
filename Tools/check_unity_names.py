"""Lists the names declared at the top of an anonymous namespace in more than one .cpp of the game module.

UBT's unity build puts many .cpp files into one translation unit, where two files' anonymous namespaces are one: the same
constant or helper in both no longer compiles. Which files share a unit changes as files are added (and files being
changed in git are compiled apart, so a clash may only show after the commit). Run this before committing new C++.

    python Tools/check_unity_names.py        exit 0 when no name clashes, 1 otherwise

A function name found in several files with different parameter lists is taken as overloads and not reported.
"""
import collections
import glob
import os
import re
import sys

MODULE = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "Source", "wasami_deception")
DECLARATION = re.compile(
    r"\t(?:static\s+)?(?:constexpr\s+|const\s+|inline\s+)*[\w:<>,\*& ]+?[\s\*&]+(\w+)\s*(\[[^\]]*\])?\s*(=|\(|\{|;)")
NOT_NAMES = {"if", "return", "for", "while", "using", "else"}


def declarations(path):
    """(name, parameters or None) for each top-level line of the file's anonymous namespaces."""
    lines = open(path, encoding="utf-8", errors="replace").read().split("\n")
    found = []
    index = 0
    while index < len(lines):
        if lines[index].strip() == "namespace" and index + 1 < len(lines) and lines[index + 1] == "{":
            inner = index + 2
            while inner < len(lines) and lines[inner] != "}":
                line = lines[inner]
                if line.startswith("\t") and not line.startswith("\t\t") and not line.startswith("\t//"):
                    match = DECLARATION.match(line)
                    if match and match.group(1) not in NOT_NAMES:
                        # A function (a parenthesis on a line that is not an initialised constant) keeps its parameters.
                        is_function = match.group(3) == "(" and not re.match(r"\t(?:static\s+)?(?:constexpr|const)\b", line)
                        found.append((match.group(1), line[match.end():].strip() if is_function else None))
                inner += 1
            index = inner
        index += 1
    return found


def main():
    seen = collections.defaultdict(list)
    for path in glob.glob(os.path.join(MODULE, "**", "*.cpp"), recursive=True):
        for name, parameters in declarations(path):
            seen[name].append((os.path.basename(path), parameters))
    clashes = 0
    for name, where in sorted(seen.items()):
        files = sorted({file for file, _ in where})
        if len(files) < 2:
            continue
        signatures = [parameters for _, parameters in where]
        if all(signature is not None for signature in signatures) and len(set(signatures)) == len(signatures):
            continue
        clashes += 1
        print(f"{name}: {', '.join(files)}")
    print(f"{clashes} clash(es)" if clashes else "no clashes")
    return 1 if clashes else 0


if __name__ == "__main__":
    sys.exit(main())
