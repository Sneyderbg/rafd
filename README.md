# Description
This is a simple visualizer and simulator for *Deterministic Finite Automatas*. You can create multiple definitions and load them in the visualizer (like the examples), and then just feed whatever string you want to test against the automata.

It actually creates a pretty graph with its nodes arranged by physics forces. The next definition produces the image below:

```python
# Σ
"sigma": ["0", "1"],
"Q": ["q0", "q1", "q2", "q3", "q4"],
"F": ["q4"],
# q -> state, a -> input
# δ : δ[q][a]
"delta": {
    "q0": {"0": "q0", "1": "q1"},
    "q1": {"0": "q2", "1": "q1"},
    "q2": {"0": "q3", "1": "q1"},
    "q3": {"0": "q0", "1": "q4"},
    "q4": {"0": "q2", "1": "q1"},
},
```
![live mode image](/screenshots/live_mode.png)
