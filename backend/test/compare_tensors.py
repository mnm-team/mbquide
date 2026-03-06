import pyzx as zx
import json
import sys
import numpy as np

def usage():
    print("Usage: compare_tensor.py [file1] [file2]")
    sys.exit(1)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        usage()

    with open(sys.argv[1], "r") as f:
        JSON1 = json.load(f)

    with open(sys.argv[2], "r") as f:
        JSON2 = json.load(f)

    g1 = zx.Graph.from_json(JSON1)
    g2 = zx.Graph.from_json(JSON2)

    # For some reason PyZX doesn't read the inputs and outputs from json
    g1.set_inputs(JSON1["inputs"])
    g1.set_outputs(JSON1["outputs"])
    g2.set_inputs(JSON2["inputs"])
    g2.set_outputs(JSON2["outputs"])

    areEqual = zx.compare_tensors(g1, g2)

    if areEqual:
        sys.exit(0)

    if not isinstance(g1, np.ndarray):
        g1 = zx.tensorfy(g1)
    if not isinstance(g2, np.ndarray):
        g2 = zx.tensorfy(g2)

    print("UNEQUAL TENSORS:")
    print("\n------\nTENSOR 1:")
    print(g1)
    print("\n------\nTENSOR 2:")
    print(g2)
    print("----------------------------------")
    
    
    sys.exit(1)
