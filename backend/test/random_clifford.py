import pyzx as zx
import argparse

def maybe_normalize(p_t, p_s, p_hsh, p_cnot, tol=1e-6):
    ps = [p_t, p_s, p_hsh, p_cnot]

    if any(p is None for p in ps):
        return p_t, p_s, p_hsh, p_cnot

    s = sum(ps)

    if abs(s - 1.0) < tol:
        return tuple(p / s for p in ps)

    # If clearly wrong, fail loudly (better than silent bias)
    raise ValueError(f"Probabilities must sum to 1 (got {s})")


def main():
    parser = argparse.ArgumentParser(description="Generate random Clifford+T circuits")

    parser.add_argument("n_qubits", type=int, help="Number of qubits")
    parser.add_argument("depth", type=int, help="Circuit depth")

    # Optional probabilities (default = None → handled by pyzx)
    parser.add_argument("--p_t", type=float, default=None, help="Probability of T gate")
    parser.add_argument("--p_s", type=float, default=None, help="Probability of S gate")
    parser.add_argument("--p_hsh", type=float, default=None, help="Probability of HSH gate")
    parser.add_argument("--p_cnot", type=float, default=None, help="Probability of CNOT gate")

    args = parser.parse_args()

    p_t, p_s, p_hsh, p_cnot = maybe_normalize(
        args.p_t, args.p_s, args.p_hsh, args.p_cnot
    )


    g = zx.generate.cliffordT(
        qubits=args.n_qubits,
        depth=args.depth,
        p_t=args.p_t,
        p_s=args.p_s,
        p_hsh=args.p_hsh,
        p_cnot=args.p_cnot
    )

    circ = zx.Circuit.from_graph(g).to_qasm()
    print(circ)


if __name__ == "__main__":
    main()