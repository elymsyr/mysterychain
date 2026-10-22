"""Minimal KiCad S-expression reader/writer (lists of str / nested lists)."""
import re

_TOKEN = re.compile(r'\(|\)|"(?:\\.|[^"\\])*"|[^\s()]+')


class Q(str):
    """A string that must be written quoted."""


def loads(text):
    stack = [[]]
    for t in _TOKEN.findall(text):
        if t == "(":
            stack.append([])
        elif t == ")":
            done = stack.pop()
            stack[-1].append(done)
        elif t[0] == '"':
            stack[-1].append(Q(t[1:-1].replace('\\"', '"').replace("\\\\", "\\")))
        else:
            stack[-1].append(t)
    return stack[0][0]


def _atom(x):
    if isinstance(x, Q):
        return '"' + x.replace("\\", "\\\\").replace('"', '\\"') + '"'
    if isinstance(x, float):
        return f"{x:.4f}".rstrip("0").rstrip(".") if x != int(x) else str(int(x))
    return str(x)


def dumps(node, indent=0):
    """Nodes holding only atoms go on one line; others break per child."""
    if not isinstance(node, list):
        return _atom(node)
    if all(not isinstance(c, list) for c in node) or (node and node[0] in ("at", "xy", "pts", "effects", "font", "stroke", "uuid")):
        return "(" + " ".join(dumps(c) for c in node) + ")"
    pad = "\t" * (indent + 1)
    head = [c for c in node if not isinstance(c, list)]
    body = [c for c in node if isinstance(c, list)]
    return ("(" + " ".join(_atom(c) for c in head) + "".join("\n" + pad + dumps(c, indent + 1) for c in body)
            + "\n" + "\t" * indent + ")")


def find(node, key):
    """Direct children of node whose head is key."""
    return [c for c in node if isinstance(c, list) and c and c[0] == key]


if __name__ == "__main__":
    s = '(a "b c" (d 1.5 (e "x\\"y")) (at 0 0 90))'
    t = loads(s)
    assert t == ["a", "b c", ["d", "1.5", ["e", 'x"y']], ["at", "0", "0", "90"]], t
    assert isinstance(t[1], Q)
    assert loads(dumps(t)) == t
    print("sexp ok")
