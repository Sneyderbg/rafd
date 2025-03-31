from typing import Required, TypedDict


class AFDModelDef(TypedDict):
    sigma: Required[list[str]]
    Q: Required[list[str]]
    F: Required[list[str]]
    delta: Required[dict[str, dict[str, str]]]


# ---------------------------------
# ---------- Definitions ----------
# ---------------------------------

original: AFDModelDef = {
    # Σ
    "sigma": ["0", "1"],
    "Q": ["q0", "q1", "q2", "q3"],
    "F": ["q3"],
    # q -> state, a -> input
    # δ : δ[q][a]
    "delta": {
        "q0": {"0": "q2", "1": "q1"},
        "q1": {"0": "q3", "1": "q0"},
        "q2": {"0": "q0", "1": "q3"},
        "q3": {"0": "q2", "1": "q1"},
    },
}

two_nodes: AFDModelDef = {
    "sigma": ["0", "1"],
    "Q": ["q0", "q1"],
    "F": ["q1"],
    "delta": {
        "q0": {"0": "q0", "1": "q1"},
        "q1": {"0": "q1", "1": "q1"},
    },
}
