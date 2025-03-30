from typing import Tuple


class AFD:
    def __init__(
        self,
        sigma: list[str],
        Q: list[str],
        F: list[str],
        delta: dict[str, dict[str, str]],
    ) -> None:
        self.sigma = sigma
        self.Q = Q
        self.n_nodes = len(Q)
        self.F = F
        self.delta = delta
        """δ[q][a]"""

        self.indexes = {name: idx for idx, name in enumerate(self.Q)}
        self.reset()

    def feed_one(self, c: str) -> Tuple[bool, str | None]:
        if not self.is_valid_input(c):
            return False, f"'{c}' is not in sigma"
        else:
            self.current_state = self.delta[self.current_state][c]
            return self.current_state in self.F, None

    def feed(self, cad: str, skip_errors=False) -> Tuple[bool, str | None]:
        self.reset()
        succ = self.is_current_success()
        for c in cad:
            succ, err = self.feed_one(c)
            if err is not None and not skip_errors:
                return False, err
        return succ, None

    def reset(self):
        self.current_state = self.Q[0]

    def is_current_success(self):
        return self.current_state in self.F

    def is_valid_input(self, c):
        return c in self.sigma
