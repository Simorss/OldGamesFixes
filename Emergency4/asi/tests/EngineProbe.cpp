#include "../src/Engine.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
int main(int argc,char** argv) {
    try {
        em4::require(argc==8,"Usage: EngineProbe mapped.bin base width height proportional output.bin layout.json");
        std::ifstream in(argv[1],std::ios::binary);em4::require(bool(in),"Cannot read fixture.");
        em4::Bytes bytes((std::istreambuf_iterator<char>(in)),{});
        em4::Image image(std::move(bytes),uint32_t(std::stoul(argv[2],nullptr,0)));
        auto l=em4::inspect(image);auto result=em4::build(image,l,std::stoi(argv[3]),std::stoi(argv[4]),std::stoi(argv[5])!=0);
        std::ofstream out(argv[6],std::ios::binary);out.write(reinterpret_cast<const char*>(result.data()),result.size());
        std::ofstream json(argv[7]);
        json<<"{\"aspect\":"<<l.aspect<<",\"forwardX\":"<<l.forwardX<<",\"forwardY\":"<<l.forwardY
            <<",\"inverseX\":"<<l.inverseX<<",\"inverseY\":"<<l.inverseY<<",\"rootCall\":"<<l.rootCall
            <<",\"rootInitializer\":"<<l.rootInitializer<<",\"globalW\":"<<l.globalW<<",\"globalH\":"<<l.globalH
            <<",\"fovObject\":"<<l.fovObject<<",\"widthObject\":"<<l.widthObject<<",\"heightObject\":"<<l.heightObject
            <<",\"defaultFov\":"<<l.defaultFov<<",\"setters\":[";
        for(size_t n=0;n<l.setters.size();n++)json<<(n?",":"")<<l.setters[n];
        json<<"]}";em4::require(bool(out)&&bool(json),"Cannot write probe results.");
        return 0;
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
