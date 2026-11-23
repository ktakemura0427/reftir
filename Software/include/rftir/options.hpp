#pragma once

#include "rftir/dbscan.hpp"
#include "rftir/lut_reconstructor.hpp"

#include <string>

namespace rftir {

struct Options {
    std::string input_dir;
    std::string output_dir;  // empty: <input>_recon next to the input folder
    std::string lut_path;    // empty: search for the default LUT
    std::string suffix;      // process only images whose stem ends with this
    ReconstructionParams recon;
    DbscanParams dbscan;
};

enum class ParseStatus { kOk, kHelp, kError };

// Parses and validates command-line options; prints the reason on error.
ParseStatus parseOptions(int argc, const char *argv[], Options &options);

void printUsage();

}  // namespace rftir
