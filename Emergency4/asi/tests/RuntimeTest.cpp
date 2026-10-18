// A local harness maps a private fixture; it never starts the game.
// EM4_TEST changes only the source of the main module handle.
#define EM4_TEST
#include "../src/Plugin.cpp"
#include <fstream>
#include <iostream>
#include <iterator>

int main(int argc,char** argv) {
    try {
        em4::require(argc==4,"Usage: RuntimeTest mapped.bin base success|reject|disabled");
        std::ifstream file(argv[1],std::ios::binary);em4::require(bool(file),"Cannot read fixture.");
        uint32_t preferred=uint32_t(std::stoul(argv[2],nullptr,0));
        file.seekg(0,std::ios::end);size_t fixtureSize=size_t(file.tellg());file.seekg(0);
        auto* mapped=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(preferred),fixtureSize,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
        em4::require(mapped && reinterpret_cast<uint32_t>(mapped)==preferred,"Cannot map private fixture at the requested base.");
        em4::Bytes before((std::istreambuf_iterator<char>(file)),{});
        std::memcpy(mapped,before.data(),before.size());fixtureModule=reinterpret_cast<HMODULE>(mapped);
        moduleHandle=GetModuleHandleW(nullptr);InitializeASI();
        std::string expected=argv[3];
        if(expected!="success") {
            em4::require(!active,"A rejected/disabled plugin became active.");
            em4::require(!std::memcmp(mapped,before.data(),before.size()),"Rejected/disabled initialization changed memory.");
            std::cout<<"PASS: "<<expected<<" initialization left all fixture bytes unchanged.\n";return 0;
        }
        em4::require(active,"Plugin did not initialize; inspect RuntimeTest.log.");
        auto image=em4::Image(before,preferred);auto found=em4::inspect(image);
        auto wanted=em4::build(image,found,resolutionX,resolutionY,true);
        // GUI/camera bytes must match the separately generated patch plan.
        for(size_t i=0;i<wanted.size();i++)if(wanted[i]!=before[i])
            em4::require(mapped[i]==wanted[i],"Applied game patch differs from the plan.");
        for(auto obj:{layout.widthObject,layout.heightObject,layout.fovObject,layout.fullscreenObject}) {
            float wantedValue=obj==layout.fullscreenObject?float(!windowed):(obj==layout.widthObject?float(resolutionX):(obj==layout.heightObject?float(resolutionY):focal));
            float got;std::memcpy(&got,reinterpret_cast<void*>(obj+0x14),4);
            em4::require(got==wantedValue,"Initial CVar override is wrong.");
        }
        // Replace only the original methods' continuations with a small test body.
        // The real installed detours, wrappers and copied FS-load gateways execute.
        // The body sets a dummy int/float/bool, returns true and cleans one argument.
        const uint8_t body[]={0x8b,0x44,0x24,4,0x89,0x41,0x18,0xf3,0x0f,0x2a,0xc0,
            0xf3,0x0f,0x11,0x41,0x14,0xc6,0x41,0x1c,1,0xb0,1,0xc2,4,0};
        for(uint32_t entry:layout.setters)std::memcpy(mapped+entry+6,body,sizeof body);
        FlushInstructionCache(GetCurrentProcess(),nullptr,0);
        uint8_t unrelated[0x70]{};
        for(uint32_t entry:layout.setters) {
            Setter method=reinterpret_cast<Setter>(mapped+entry);
            for(int repeat=0;repeat<3;repeat++) {
                for(auto obj:{layout.widthObject,layout.heightObject,layout.fovObject,layout.fullscreenObject}) {
                    std::memset(reinterpret_cast<void*>(obj+0x20),0x5a,0x50);
                    em4::require(method(reinterpret_cast<void*>(obj),123),"Setter return value was not preserved.");
                    float expectedValue=obj==layout.fullscreenObject?float(!windowed):(obj==layout.widthObject?float(resolutionX):(obj==layout.heightObject?float(resolutionY):
                        (forcedBaseFov?float(forcedBaseFov*(4.0/3.0)/(double(resolutionX)/resolutionY)):float(123*focalFactor))));
                    float got;std::memcpy(&got,reinterpret_cast<void*>(obj+0x14),4);
                    em4::require(got==expectedValue,"Typed setter did not enforce resolution/FOV.");
                    int integer;std::memcpy(&integer,reinterpret_cast<void*>(obj+0x18),4);
                    em4::require(integer==int(expectedValue),"Integer CVar state does not match its float.");
                    em4::require(*reinterpret_cast<uint8_t*>(obj+0x1c)==(expectedValue!=0),"Boolean CVar state does not match its float.");
                    for(int j=0x20;j<0x70;j++)em4::require(*reinterpret_cast<uint8_t*>(obj+j)==0x5a,"A string representation was overwritten.");
                }
                em4::require(method(unrelated,123),"An unrelated setter return changed.");
                float got;std::memcpy(&got,unrelated+0x14,4);em4::require(got==123,"An unrelated CVar was overridden.");
            }
        }
        auto saved=em4::Bytes(mapped,mapped+before.size());InitializeASI();
        em4::require(!std::memcmp(mapped,saved.data(),saved.size()),"InitializeASI ran twice.");
        std::cout<<"PASS: native initialization, camera/UI patches, all 5 x86 detours, CVar consistency, unrelated variables, repeat protection.\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
