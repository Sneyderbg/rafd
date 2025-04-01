import sys

from afd import AFD
import definitions
from visualizer import run
from random import randint

USAGE = """
Usage:
    <program>     - run automata an type throught console
    <program> vis - run visualizer
"""

# main
if __name__ == "__main__":
    if len(sys.argv) == 1:
        automata = AFD(
            definitions.original["sigma"],
            definitions.original["Q"],
            definitions.original["F"],
            definitions.original["delta"],
        )

        print("Type a string to evaluate with the automata")
        input_str = input("string: ")
        while input_str != "exit":
            succ, err = automata.feed(input_str)
            if err:
                print(f"Error: {err}")
            else:
                print(f'the string is {"ACCEPTED" if succ else "REJECTED"}')
            input_str = input("string: ")
    elif len(sys.argv) == 2:
        if sys.argv[1] == "vis":
            run()
        elif sys.argv[1] == "test":
            automata = AFD(
                definitions.original["sigma"],
                definitions.original["Q"],
                definitions.original["F"],
                definitions.original["delta"],
            )

            def rand_char():
                return automata.sigma[randint(0, 1)]

            for i in range(100):
                test_good = rand_char() * randint(0, 100) + "1001"  # (1+0)*(1001)
                succ, err = automata.feed(test_good)
                assert (
                    succ and err is None
                ), f"failed a good_test: #{i} -> {test_good} -> {succ}"

            for i in range(100):
                test_bad = (
                    rand_char() * randint(0, 50)
                    + "1001" * randint(0, 5)
                    + rand_char() * randint(1, 50)  # (1+0)*(1001)(1+0)^+
                )
                succ, err = automata.feed(test_bad)
                assert (
                    not succ
                ), f"failed a bad_test: #{i} -> {test_bad} -> {succ}, {err}"
            print("all good")

    else:
        print(USAGE)
