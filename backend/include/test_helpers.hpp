
#include "ZX_Graph.hpp"
#include <filesystem>
#include <fstream>
#include <filesystem>

inline bool compareTensors(ZXGraph zx1, ZXGraph zx2) {
    std::string file1 = "ComparisonZX_1.json";
    std::string file2 = "ComparisonZX_2.json";

    std::ofstream(file1) << zx1.toPyZXJson().dump(4);
    std::ofstream(file2) << zx2.toPyZXJson().dump(4);

    std::string pythonCommand = "python_venv/bin/python test/compare_tensors.py " + file1 + " " + file2;
    int result = system(pythonCommand.c_str());

    std::filesystem::remove(file1);
    std::filesystem::remove(file2);

    return result == 0;
}
