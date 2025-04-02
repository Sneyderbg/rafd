import sys

from afd import AFD
import definitions
from visualizer import run
from random import randint

USAGE = """
Usage:
    <program> cli <num_automata> - run automata and type through console
    <program> vis <num_automata> - run visualizer
    ** num_automata accept 1 - original automata or 2 - modified automata
"""


def run_tests():
    automata = AFD(
        definitions.modified["sigma"],
        definitions.modified["Q"],
        definitions.modified["F"],
        definitions.modified["delta"],
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
        assert not succ, f"failed a bad_test: #{i} -> {test_bad} -> {succ}, {err}"
    print("all good")


def run_cli(num_automata: int):

    if num_automata == 0:
        automata = AFD(
            definitions.original["sigma"],
            definitions.original["Q"],
            definitions.original["F"],
            definitions.original["delta"],
        )
    elif num_automata == 1:
        automata = AFD(
            definitions.modified["sigma"],
            definitions.modified["Q"],
            definitions.modified["F"],
            definitions.modified["delta"],
        )
    else:
        print(f'the argum = {num_automata }'," isn't a correct automata")
        exit(1)


    print("Type a string to evaluate with the automata")
    input_str = input("string: ")
    while input_str != "exit":
        succ, err = automata.feed(input_str)
        if err:
            print(f"Error: {err}")
        else:
            print(f'the string is {"ACCEPTED" if succ else "REJECTED"}')
        input_str = input("string: ")

def run_vis(num_automata: int):

    if num_automata == 0:
        run(definitions.original)
    elif num_automata == 1:
        run(definitions.modified)
    else:
        print(f'the argum = {num_automata}', " isn't a correct automata")
        exit(1)



# main
if __name__ == "__main__":
    if len(sys.argv) == 2 or len(sys.argv) == 3:
        if len(sys.argv) == 3:
            num_automata = int(sys.argv[2])

        if sys.argv[1] == "cli":
            run_cli(num_automata)
        elif sys.argv[1] == "vis":
            run_vis(num_automata)
        elif sys.argv[1] == "test":
            run_tests()
        else:
            print(USAGE)

    else:
        print(USAGE)
