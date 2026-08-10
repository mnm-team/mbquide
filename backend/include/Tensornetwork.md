# TensorNetwork Simulator: Current State & SVD Improvement Guide

## Current Implementation: Block-Based Tensor Network

### How It Works Now

**Data Structure:**
- `blocks`: Map of independent dense amplitude vectors (one per entangled cluster)
- `posToLoc`: Tracks which global qubit position lives in which block and at which local bit
- Each qubit starts in its own 1-qubit block (isolated, memory efficient)

**Example with 4 qubits:**
```
Initial state (unentangled):
  Qubit 0 → Block 0: [1, 0]ᵀ (|0⟩)
  Qubit 1 → Block 1: [1, 0]ᵀ (|0⟩)
  Qubit 2 → Block 2: [1, 0]ᵀ (|0⟩)
  Qubit 3 → Block 3: [1, 0]ᵀ (|0⟩)
  Total storage: 4 blocks × 2 amps = 8 values

After CZ(0, 1):
  Block 0: [1, 0, 0, 0]ᵀ (qubits 0,1 entangled, 2^2 = 4 amps)
  Block 1: [1, 0]ᵀ (qubit 2)
  Block 2: [1, 0]ᵀ (qubit 3)
  Total storage: 4 + 2 + 2 = 8 values

After CZ(1, 2):
  Block 0: [1, 0, 0, 0, 0, 0, 0, 0]ᵀ (qubits 0,1,2 all entangled, 2^3 = 8 amps)
  Block 1: [1, 0]ᵀ (qubit 3)
  Total storage: 8 + 2 = 10 values
```

**Key Properties:**
- ✅ **Memory efficient for sparse entanglement**: Unentangled qubits cost O(1) each
- ✅ **Fast for local operations**: Single-qubit gates apply within blocks only
- ❌ **Blocks grow exponentially**: A chain of n CZ gates creates a 2^n-sized block
- ❌ **No compression**: Once qubits are entangled, all amplitudes are stored exactly
- ❌ **Not MPS-like**: No bond truncation, no sequential structure

**Memory Cost Example:**
- 100 qubits in one fully entangled cluster: 2^100 ≈ 10³⁰ amplitudes (impossible!)
- Same 100 qubits with SVD truncation at χ=256: ~100 × 256 × 2 ≈ 50k values (tractable!)

---

## Proposed SVD Decomposition Strategy

### The Idea: Break Blocks into MPS Chains

When a block gets large (or when requested), decompose it via SVD into a **Matrix Product State (MPS) chain** with truncated bond dimensions.

**Example: Block with 4 qubits becomes MPS chain**
```
Dense block: [1, 0, 0, 0, 0, 0, 0, 0, ...]ᵀ  (size 2^4 = 16)

SVD decomposition (left-canonical MPS):
  A[0]: (2, χ₀) matrix             — physical index 2, bond out χ₀
  A[1]: (χ₀, 2, χ₁) 3-tensor       — bond in χ₀, physical 2, bond out χ₁
  A[2]: (χ₁, 2, χ₂) 3-tensor       — bond in χ₁, physical 2, bond out χ₂
  A[3]: (χ₂, 2) matrix             — bond in χ₂, physical index 2

With truncation χ_max = 8:
  Storage: 2×8 + 8×2×8 + 8×2×8 + 8×2 = 16 + 128 + 128 + 16 = 288 values
  vs. dense: 2^4 = 16 values (small gains for 4 qubits, huge for 20+)
  
With 20 qubits and χ=256:
  Storage: 20 × 256 × 2 ≈ 10k values
  vs. dense: 2^20 ≈ 1M values (100× compression!)
```

---

## Implementation Strategy for Claude Code

### Phase 1: Create SVD Decomposer (New File)

**File:** `svd_mps_decomposer.hpp`

**Key Functions:**
```cpp
// Decompose a dense amplitude vector into MPS chain
struct MPSChain {
    std::vector<MatrixXcd> tensors;      // A[0], A[1], ..., A[n-1]
    std::vector<int> bondDims;           // χ₀, χ₁, ..., χₙ₋₂
    std::vector<double> truncErrors;     // error at each bond
};

MPSChain vectorToMPS(
    const VectorXcd& amp,
    int numQubits,
    int maxBondDim,          // e.g., 256
    double relTol            // e.g., 1e-12
);

// Reconstruct full vector from MPS (for comparison/readout)
VectorXcd mpsToVector(const MPSChain& mps);

// Compute fidelity loss from truncation
double computeTruncationFidelity(const MPSChain& mps);
```

**Algorithm (Left-Canonical SVD):**
1. Reshape amplitude vector [2^n] → [2 × 2^(n-1)] matrix
2. SVD decompose: U × Σ × V†
3. Keep top χ singular values (truncate)
4. Store U as A[0], set up next: Σ × V† reshaped as [2χ × 2^(n-2)]
5. Repeat for each site

---

### Phase 2: Extend Block Structure (Modify TensorNetworkSimulator)

**Add to Block struct:**
```cpp
struct Block {
    VectorXcd amp;              // Dense amplitudes (current)
    std::optional<MPSChain> mps; // MPS representation (if decomposed)
    int numQubits = 0;
    bool isMPS = false;         // Flag: use MPS or dense?
};
```

**Add methods to TensorNetworkSimulator:**
```cpp
// On-demand decomposition: convert large block to MPS
void decomposeBlockToMPS(int blockId, int maxBondDim=256, double relTol=1e-10);

// For gates: check if operand is MPS; if so, convert back to dense first
// (or add MPS-aware gate application later)
void ensureDenseBlock(int blockId);

// Memory profiling
long long getMemoryUsage() const;  // Count both dense + MPS storage

// Statistics
struct CompressionStats {
    int numBlocks;
    int numMPSBlocks;
    long long denseStorage;
    long long mpsStorage;
    double totalCompressionRatio;
};
CompressionStats getCompressionStats() const;
```

---

### Phase 3: Add MPS-Aware Operations (Future)

**Optional, more advanced:**

```cpp
// Apply single-qubit gate directly to MPS without expanding to dense
void applyGateToMPS(int blockId, int qubit, const Matrix2cd& gate);

// Contract two MPS blocks (for two-qubit gates)
void mergeMPSBlocks(int blockId1, int blockId2);

// Measurement with MPS
int measureMPS(int blockId, int qubit, bool traceOut);
```

For Phase 1, gates on MPS blocks will convert back to dense (simpler, correct, not optimal).

---

## Usage Workflow

### Simple Use Case: Auto-Decompose Large Blocks

```cpp
TensorNetworkSimulator sim(20);

// ... apply gates, build up entanglement ...

// When block gets too large, decompose it
if (sim.getStoredAmplitudeCount() > 1e6) {
    sim.decomposeAllLargeBlocks(maxBondDim=256, relTol=1e-12);
}

// Check compression
auto stats = sim.getCompressionStats();
std::cout << "Compression ratio: " << stats.totalCompressionRatio << "x\n";

// Continue using simulator normally
// (Gates automatically handle dense/MPS as needed)
```

### Advanced: Manual Control

```cpp
// Measure fidelity of MPS approximation
auto mps = sim.getBlockMPS(blockId);
double fidelity = sim.computeBlockFidelity(blockId);
std::cout << "Fidelity loss: " << (1.0 - fidelity) << "\n";

// Get full state (automatically contracts MPS if needed)
auto psi = sim.get_statevector();  // Returns dense vector
```

---

## Benefits

| Metric | Current | With SVD MPS |
|--------|---------|--------------|
| **100 qubits, full cluster** | 2^100 (impossible) | χ=256: ~50k values (works!) |
| **10 qubits, χ=64** | 1024 values | ~1300 values (1.3× overhead) |
| **Theoretical limit** | Exponential in cluster size | Exponential in χ only |
| **Compression tuning** | None | Adjustable: χ and relTol |
| **Gate speed** | Exponential in block size | Exponential in χ (if native MPS ops) |

---

## Implementation Steps for Claude Code

1. **Create `svd_mps_decomposer.hpp`**
   - Implement `vectorToMPS()` with proper tensor reshaping
   - Implement `mpsToVector()` for reconstruction
   - Add fidelity computation

2. **Modify `TensorNetworkSimulator`**
   - Add `std::optional<MPSChain> mps` to Block
   - Add `decomposeBlockToMPS()` method
   - Add `ensureDenseBlock()` helper (gates use this)
   - Add `getMemoryUsage()` and `getCompressionStats()`

3. **Test & Validate**
   - Verify `vectorToMPS() → mpsToVector()` recovers original state
   - Compare fidelity vs. truncation parameters
   - Benchmark: measure time/space trade-offs

4. **(Optional, Phase 2) Add MPS-aware operations**
   - Direct gate application to MPS tensors
   - MPS contraction for two-qubit gates
   - Measurement with MPS

---

## Key Files to Create/Modify

```
├── svd_mps_decomposer.hpp          [NEW] SVD decomposition, MPS chain
├── tensor_network_simulator.hpp    [MODIFY] Add MPS support to Block
└── test_svd_decomposition.cpp      [NEW] Unit tests & benchmarks
```

---

## Why This Works

- **Preserves exactness** (for small blocks, no approximation)
- **Adds approximation capability** (tuneable via χ and relTol)
- **Backward compatible** (dense blocks still work, no breaking changes)
- **Composable** (some blocks MPS, others dense, others hybrid)
- **Progressively improvable** (Phase 1: decompose + convert-back-to-dense gates, Phase 2: native MPS operations)