#include <iostream>
#include <fstream>

#include <boost/iostreams/device/mapped_file.hpp>

#include "repair.hpp"

int main(int argc, char** argv)
{
    std::vector<std::string> args(argv + 1, argv + argc);
    std::string filename;
    bool preserve_zeros = false;

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "-z") {
            preserve_zeros = true;
        } else {
            filename = args[i];
        }
    }
    
    if (!filename.size()) std::terminate();

    boost::iostreams::mapped_file_source m(filename);

    std::vector<repair::code_type> C;
    std::vector<std::string> D;
    repair::approximate_repair(std::make_pair(m.data(), m.data() + m.size()), C, D, preserve_zeros);
    
    std::ofstream Df((filename + ".D").c_str(), std::ios::binary);
    std::ofstream Cf((filename + ".C").c_str(), std::ios::binary);
    for (size_t i = 0; i < D.size(); ++i) {
        uint32_t l = uint32_t(D[i].size());
        Df.write(reinterpret_cast<const char*>(&l), 4);
        Df.write(&(D[i])[0], l);
    }

    Cf.write(reinterpret_cast<const char*>(&C[0]), C.size() * sizeof(repair::code_type));
}


