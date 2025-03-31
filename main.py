import sys
from afd import AFD
import afd1
from visualizer import run

# main
if __name__ == "__main__":
    if len(sys.argv) == 2 and sys.argv[1] == "vis":
        run()
    else:
        automata = AFD(
            afd1.original["sigma"],
            afd1.original["Q"],
            afd1.original["F"],
            afd1.original["delta"],
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
