
#include "ZX_Graph.hpp"
#include <filesystem>
#include <fstream>
#include <filesystem>
#include <string>
#include <optional>

inline bool compareTensors(ZXGraph zx1, ZXGraph zx2) {
    std::string file1 = "ComparisonZX_1.json";
    std::string file2 = "ComparisonZX_2.json";

    std::ofstream(file1) << zx1.toPyZXJson().dump(4);
    std::ofstream(file2) << zx2.toPyZXJson().dump(4);

    std::string pythonCommand = "python_venv/bin/python backend/test/compare_tensors.py " + file1 + " " + file2;
    int result = system(pythonCommand.c_str());

    std::filesystem::remove(file1);
    std::filesystem::remove(file2);

    return result == 0;
}



inline std::string randomClifford(
    int num_qubits,
    int depth,
    std::optional<double> p_t = std::nullopt,
    std::optional<double> p_s = std::nullopt,
    std::optional<double> p_hsh = std::nullopt,
    std::optional<double> p_cnot = std::nullopt
) {
    std::string command =
        "python_venv/bin/python backend/test/random_clifford.py " +
        std::to_string(num_qubits) + " " +
        std::to_string(depth);

    if (p_t)    command += " --p_t " + std::to_string(*p_t);
    if (p_s)    command += " --p_s " + std::to_string(*p_s);
    if (p_hsh)  command += " --p_hsh " + std::to_string(*p_hsh);
    if (p_cnot) command += " --p_cnot " + std::to_string(*p_cnot);

    std::array<char, 256> buffer{};
    std::string result;

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << "Error: popen() failed\n";
        return "";  // graceful fallback
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
        result += buffer.data();

    int rc = pclose(pipe);
    if (rc != 0) {
        std::cerr << "Error: Python script failed with code " << rc << "\n";
        return "";  // or return partial result if you prefer
    }

    return result;
}
