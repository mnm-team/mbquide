import pyzx as zx
import json
import sys
import numpy as np
from itertools import permutations

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

    g1.set_inputs(JSON1["inputs"])
    g1.set_outputs(JSON1["outputs"])
    g2.set_inputs(JSON2["inputs"])
    g2.set_outputs(JSON2["outputs"])

    areEqual = zx.compare_tensors(g1, g2)
    if areEqual:
        sys.exit(0)

    t1 = zx.tensorfy(g1)
    t2 = zx.tensorfy(g2)

    n_axes = t1.ndim  # e.g. 4 for a 2-qubit map (2 inputs + 2 outputs)
    found_perm = None

    for perm in permutations(range(n_axes)):
        t2_perm = np.transpose(t2, perm)
        # compare up to global scalar (same logic as pyzx)
        if np.allclose(t1, t2_perm):
            found_perm = perm
            break
        # also try scalar-invariant comparison
        epsilon = 1e-14
        for i, a in enumerate(t1.flat):
            if abs(a) > epsilon:
                if abs(t2_perm.flat[i]) < epsilon:
                    break
                if np.allclose(t1 / a, t2_perm / t2_perm.flat[i]):
                    found_perm = perm
                break

    if found_perm is not None:
        print(f"Tensors are equal up to axis permutation: {found_perm}")
        sys.exit(0)
    else:
        print("UNEQUAL TENSORS (no permutation matches):")
        print("\n------\nTENSOR 1:")
        print(t1)
        print("\n------\nTENSOR 2:")
        print(t2)
        sys.exit(1)