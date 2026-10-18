#pragma once
// Emergency 4 / Vision 7.1 x86 engine recognition. Offsets are discovered.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace em4 {
using Bytes = std::vector<uint8_t>;
inline void require(bool ok, const std::string& message) { if (!ok) throw std::runtime_error(message); }
template<class T> T read(const Bytes& b, size_t at) {
    require(at <= b.size() && sizeof(T) <= b.size()-at, "Invalid image range.");
    T result; std::memcpy(&result, b.data()+at, sizeof result); return result;
}
template<class T> void put(Bytes& b, size_t at, T value) {
    require(at <= b.size() && sizeof(T) <= b.size()-at, "Invalid patch range.");
    std::memcpy(b.data()+at, &value, sizeof value);
}
inline bool equal(const Bytes& b, size_t at, std::initializer_list<uint8_t> data) {
    return at <= b.size() && data.size() <= b.size()-at && std::equal(data.begin(),data.end(),b.begin()+at);
}
inline bool padding(const Bytes& b, size_t at, size_t n) {
    return at <= b.size() && n <= b.size()-at && std::all_of(b.begin()+at,b.begin()+at+n,[](uint8_t v){return v==0xcc;});
}
inline bool positive(float x) { return std::isfinite(x) && x>0; }
inline uint32_t one(std::vector<uint32_t> hits, const char* name) {
    std::sort(hits.begin(),hits.end()); hits.erase(std::unique(hits.begin(),hits.end()),hits.end());
    require(hits.size()==1, std::string("Engine recognition failed: ")+name+" has "+std::to_string(hits.size())+" matches.");
    return hits[0];
}
struct Section { uint32_t rva, size, flags; };
struct Image {
    Bytes bytes;
    uint32_t base;
    std::vector<Section> sections;
    std::map<uint32_t,std::string> imports;
    explicit Image(Bytes data, uint32_t address) : bytes(std::move(data)), base(address) {
        require(u16(0)==0x5a4d, "Not a PE image.");
        uint32_t pe=u32(0x3c);
        require(u32(pe)==0x4550 && u16(pe+4)==0x14c, "The plugin requires a 32-bit x86 PE image.");
        uint32_t optional=pe+24, count=u16(pe+6), optSize=u16(pe+20);
        require(count>0 && count<=96 && optSize>=112 && u16(optional)==0x10b,"Unsupported PE header.");
        require(u32(optional+56)==bytes.size(),"Mapped image size mismatch.");
        for(uint32_t n=0;n<count;n++) {
            uint32_t at=optional+optSize+40*n;
            Section s{u32(at+12),std::max(u32(at+8),u32(at+16)),u32(at+36)};
            require(s.rva<=bytes.size() && s.size<=bytes.size()-s.rva,"Invalid mapped section.");
            sections.push_back(s);
        }
        uint32_t desc=u32(optional+104);
        require(desc!=0,"No engine imports found.");
        for(int n=0;n<512;n++,desc+=20) {
            uint32_t names=u32(desc), dll=u32(desc+12), first=u32(desc+16);
            if(!names && !dll && !first) break;
            require(names!=0,"An import name table is required for engine recognition.");
            std::string library=z(dll);
            for(uint32_t j=0;j<8192;j++) {
                uint32_t entry=u32(names+j*4); if(!entry) break;
                if(!(entry&0x80000000)) imports[addressOf(first+j*4)]=library+"!"+z(entry+2);
            }
        }
    }
    uint16_t u16(uint32_t rva) const { return read<uint16_t>(bytes,rva); }
    uint32_t u32(uint32_t rva) const { return read<uint32_t>(bytes,rva); }
    float f32(uint32_t rva) const { return read<float>(bytes,rva); }
    uint32_t offset(uint32_t va) const {
        require(va>=base && uint64_t(va)-base<bytes.size(),"Engine pointer is outside the main module.");
        return va-base;
    }
    uint32_t addressOf(uint32_t rva) const {
        require(rva<bytes.size() && uint64_t(base)+rva<=UINT32_MAX,"Invalid engine address.");
        return base+rva;
    }
    bool executable(uint32_t rva, uint32_t size=1) const {
        for(const auto& s:sections) if((s.flags&0x20000000) && rva>=s.rva && uint64_t(rva)+size<=uint64_t(s.rva)+s.size) return true;
        return false;
    }
    bool writable(uint32_t rva, uint32_t size=1) const {
        for(const auto& s:sections) if((s.flags&0x80000000) && rva>=s.rva && uint64_t(rva)+size<=uint64_t(s.rva)+s.size) return true;
        return false;
    }
    std::string z(uint32_t at) const {
        require(at<bytes.size(),"Invalid image string.");
        uint32_t end=at;
        while(end<bytes.size() && end-at<512 && bytes[end]) end++;
        require(end<bytes.size() && end-at<512,"Unterminated image string.");
        return std::string(reinterpret_cast<const char*>(bytes.data()+at),end-at);
    }
    std::vector<uint32_t> find(const std::string& signature) const {
        std::istringstream in(signature); std::string item; std::vector<int> sig;
        while(in>>item) sig.push_back(item=="??"?-1:std::stoi(item,nullptr,16));
        require(!sig.empty(),"Empty signature."); std::vector<uint32_t> hits;
        for(const auto& s:sections) {
            if(!(s.flags&0x20000000) || s.size<sig.size()) continue;
            for(uint32_t at=s.rva;at<=s.rva+s.size-sig.size();at++) {
                if(sig[0]>=0 && bytes[at]!=sig[0]) continue;
                size_t k=1; while(k<sig.size() && (sig[k]<0 || bytes[at+k]==sig[k])) k++;
                if(k==sig.size()) hits.push_back(at);
            }
        }
        return hits;
    }
    uint32_t branch(uint32_t at) const {
        require(equal(bytes,at,{0xe8}) || equal(bytes,at,{0xe9}),"Expected an x86 branch.");
        return addressOf(at)+5+u32(at+1);
    }
    bool importCall(uint32_t at, const char* name) const {
        try {
            uint32_t target=offset(branch(at));
            if(!equal(bytes,target,{0xff,0x25})) return false;
            auto i=imports.find(u32(target+2));
            return i!=imports.end() && i->second.find(name)!=std::string::npos;
        } catch(const std::exception&) { return false; }
    }
};
struct Layout {
    Bytes baseline;
    uint32_t aspect, forwardX, forwardY, inverseX, inverseY;
    uint32_t width, height, reciprocalX, reciprocalY, mouseW, mouseH, clipW, clipH, rootCall, rootInitializer;
    uint32_t globalW,globalH,fovObject, widthObject,heightObject;
    int defaultFov;
    float previousAspect;
    std::vector<uint32_t> verticalRefs, setters;
};
inline void branch(Bytes& b, uint32_t at, uint32_t base, uint32_t target, uint8_t op) {
    put<uint8_t>(b,at,op); put<uint32_t>(b,at+1,target-(base+at+5));
}
inline bool pointerUse(const Bytes& b,uint32_t at) {
    return (at>=2 && equal(b,at-2,{0xd8,0x0d})) || (at>=4 && b[at-4]==0xf3 && b[at-3]==0x0f &&
        (b[at-2]==0x10 || b[at-2]==0x59) && (b[at-1]&0xc7)==5);
}
inline uint32_t initializer(const Image& p, uint32_t at) {
    uint32_t target=p.branch(at);
    for(int n=0;n<3;n++) {
        uint32_t off=p.offset(target);
        if(equal(p.bytes,off,{0xc7,0x44,0x24,4}) || equal(p.bytes,off,{0xc7,0x44,0x24,8})) {target=p.branch(off+8);continue;}
        require(equal(p.bytes,off,{0x83,0xc1,0x30,0xe9}),"Unknown root GUI initializer.");
        return target;
    }
    throw std::runtime_error("Unknown root GUI trampoline.");
}
inline uint32_t cvarConstructor(const Image& p,const char* name,uint32_t valueAddress) {
    std::vector<uint32_t> hits;
    for(uint32_t at:p.find("68 ?? ?? ?? ?? 6A 02 68 ?? ?? ?? ?? 68 ?? ?? ?? ?? B9 ?? ?? ?? ?? E8 ?? ?? ?? ??")) {
        try {
            if(p.z(p.offset(p.u32(at+13)))==name && p.u32(at+18)+0x14==valueAddress) hits.push_back(at);
        } catch(const std::exception&) {}
    }
    return one(hits,name);
}
inline Layout inspect(const Image& p) {
    const auto& b=p.bytes; Layout l{};
    std::vector<uint32_t> matches;
    for(auto at:p.find("A1 ?? ?? ?? ?? 68 ?? ?? ?? ?? 8D 4C 24 ?? 51 8B 0D ?? ?? ?? ?? 8D 54 24 ?? 52 50 E8 ?? ?? ?? ?? 8B 4C 24 ?? 8B 54 24 ?? 51 52 E8"))
        if(p.importCall(at+27,"PerspectiveToAngles@VisRenderer_cl")) matches.push_back(at);
    uint32_t camera=one(matches,"main camera projection"); l.aspect=camera+6; l.previousAspect=p.f32(l.aspect);
    require(positive(l.previousAspect),"Invalid camera aspect.");
    uint32_t root=one(p.find("68 ?? ?? ?? ?? 68 ?? ?? ?? ?? 53 53 8B CF E8 ?? ?? ?? ?? 53 53 8B CD E8 ?? ?? ?? ?? 68 00 00 40 44 68 00 00 80 44 53 53 8B CD E8"),"root GUI constructor");
    l.clipH=root+1;l.clipW=root+6;l.rootCall=root+23;l.rootInitializer=initializer(p,l.rootCall);
    uint32_t setter=p.offset(p.branch(p.offset(l.rootInitializer)+3));
    auto fw=p.find("D9 05 ?? ?? ?? ?? D8 4C 24 04 D8 0D ?? ?? ?? ?? C3");
    auto inv=p.find("D9 44 24 04 D8 35 ?? ?? ?? ?? D8 0D ?? ?? ?? ?? C3");
    matches.clear();
    for(auto x:fw) for(auto y:fw) if(y>x && y-x<128) {
        std::vector<uint32_t> ix,iy;
        for(auto i:inv) if(i>y && i-y<128 && p.u32(i+6)==p.u32(x+2)) ix.push_back(i);
        if(ix.size()!=1) continue;
        for(auto i:inv) if(i>ix[0] && i-ix[0]<128 && p.u32(i+6)==p.u32(y+2)) iy.push_back(i);
        if(iy.size()==1 && setter>iy[0] && setter-iy[0]<0x10000) matches.push_back(x);
    }
    l.forwardX=one(matches,"GUI coordinate family");matches.clear();
    for(auto i:fw) if(i>l.forwardX && i-l.forwardX<128) matches.push_back(i);
    l.forwardY=one(matches,"vertical forward conversion");matches.clear();
    l.globalW=p.u32(l.forwardX+2);l.globalH=p.u32(l.forwardY+2);
    for(auto i:inv) if(i>l.forwardY && i-l.forwardY<128 && p.u32(i+6)==l.globalW) matches.push_back(i);
    l.inverseX=one(matches,"horizontal inverse conversion");matches.clear();
    for(auto i:inv) if(i>l.inverseX && i-l.inverseX<128 && p.u32(i+6)==l.globalH) matches.push_back(i);
    l.inverseY=one(matches,"vertical inverse conversion");
    l.width=p.offset(p.u32(l.inverseX+12));l.height=p.offset(p.u32(l.inverseY+12));
    l.reciprocalX=p.offset(p.u32(l.forwardX+12));
    uint32_t currentY=p.u32(l.forwardY+12); bool oldPrivateY=p.offset(currentY)==l.inverseX+17;
    if(!oldPrivateY) l.reciprocalY=p.offset(currentY);
    else {
        matches.clear();
        for(const auto& s:p.sections) if(!(s.flags&0x20000000))
            for(uint32_t at=s.rva;at+4<=s.rva+s.size;at+=4) if(p.f32(at)==1.f/768.f) matches.push_back(at);
        l.reciprocalY=one(matches,"original vertical reciprocal");
    }
    require(positive(p.f32(l.width)) && positive(p.f32(l.height)) &&
        std::abs(p.f32(l.width)*p.f32(l.reciprocalX)-1)<0.001 &&
        std::abs(p.f32(l.height)*p.f32(p.offset(currentY))-1)<0.001,"Inconsistent GUI scales.");
    matches.clear();
    for(auto at:p.find("F3 0F 2A 04 24 F3 0F 59 05 ?? ?? ?? ?? F3 0F 5E 05 ?? ?? ?? ?? 8B 4C 24 0C 8B 54 24 10 F3 0F 11 01 F3 0F 2A 44 24 04 F3 0F 59 05 ?? ?? ?? ?? F3 0F 5E 05 ?? ?? ?? ?? F3 0F 11 02 83 C4 08 C2 08 00"))
        if(p.u32(at+17)==l.globalW && p.u32(at+51)==l.globalH) matches.push_back(at);
    uint32_t mouse=one(matches,"GUI mouse coordinate mapping");
    l.mouseW=p.offset(p.u32(mouse+9));l.mouseH=p.offset(p.u32(mouse+43));
    require(std::abs(p.f32(l.mouseW)-p.f32(l.width))<0.01 && std::abs(p.f32(l.mouseH)-p.f32(l.height))<0.01,"Mouse and drawing dimensions disagree.");
    for(auto pair:std::vector<std::pair<uint32_t,uint8_t>>{{l.forwardX+17,4},{l.forwardY+17,8}})
        require(padding(b,pair.first,15) || (equal(b,pair.first,{0xc7,0x44,0x24,pair.second}) && b.at(pair.first+8)==0xe9 && padding(b,pair.first+13,2)),"GUI alignment space is occupied.");
    require(padding(b,l.inverseX+17,15) || (oldPrivateY && padding(b,l.inverseX+21,11)),"GUI reciprocal space is occupied.");
    for(uint32_t at=l.forwardX;at+4<setter;at++) if(p.u32(at)==currentY) {
        require(pointerUse(b,at),"Unknown vertical GUI pointer instruction.");l.verticalRefs.push_back(at);
    }
    require(l.verticalRefs.size()>=2,"Missing vertical GUI references.");
    l.baseline=b;
    put<float>(l.baseline,l.aspect,4.f/3.f);
    for(auto at:{l.width,l.mouseW,l.clipW})put<float>(l.baseline,at,1024);
    for(auto at:{l.height,l.mouseH,l.clipH})put<float>(l.baseline,at,768);
    put<float>(l.baseline,l.reciprocalX,1.f/1024.f);put<float>(l.baseline,l.reciprocalY,1.f/768.f);
    for(auto at:l.verticalRefs)put<uint32_t>(l.baseline,at,p.addressOf(l.reciprocalY));
    for(auto at:{l.forwardX+17,l.forwardY+17,l.inverseX+17})std::fill_n(l.baseline.begin()+at,15,0xcc);
    branch(l.baseline,l.rootCall,p.base,l.rootInitializer,0xe8);

    uint32_t cx=cvarConstructor(p,"r_xres",l.globalW),cy=cvarConstructor(p,"r_yres",l.globalH);
    uint32_t cf=cvarConstructor(p,"r_fov",p.u32(camera+1));
    require(p.branch(cx+22)==p.branch(cy+22) && p.branch(cx+22)==p.branch(cf+22),"CVar constructors disagree.");
    l.widthObject=p.u32(cx+18);l.heightObject=p.u32(cy+18);l.fovObject=p.u32(cf+18);
    l.defaultFov=static_cast<int>(p.u32(cf+8));
    require(l.defaultFov>0 && l.defaultFov<10000,"Invalid default focal parameter.");
    for(auto object:{l.widthObject,l.heightObject,l.fovObject})require(p.writable(p.offset(object),0x70),"CVar object is outside writable game data.");
    // The integer constructor must call the recognized setter. All five typed
    // setters have a complete six-byte, position-independent SEH first instruction.
    uint32_t ctor=p.offset(p.branch(cx+22)); matches.clear();
    for(uint32_t at=ctor;at<ctor+256;at++) if(equal(b,at,{0xe8})) {
        try {
            uint32_t target=p.offset(p.branch(at));
            if(equal(b,target,{0x64,0xa1,0,0,0,0,0x6a,0xff,0x68}) && equal(b,target+21,{0x83,0xec,0x60})) matches.push_back(target);
        } catch(const std::exception&) {}
    }
    uint32_t integerSetter=one(matches,"integer CVar setter");
    int shortFrames=0,longFrames=0,floatFrames=0;
    for(auto at:p.find("64 A1 00 00 00 00 6A FF 68 ?? ?? ?? ?? 50 64 89 25 00 00 00 00")) {
        if(at+0x1000<integerSetter || at>integerSetter+0x1000)continue;
        if(equal(b,at+21,{0x83,0xec,0x60}))longFrames++;
        else if(equal(b,at+21,{0x83,0xec,0x20}))shortFrames++;
        else if(equal(b,at+21,{0x81,0xec,0xa0,0,0,0}))floatFrames++;
        else continue;
        uint32_t body=at+(equal(b,at+21,{0x81})?27:24);
        require(equal(b,body,{0x56,0x8b,0xf1,0xf6,0x46,6,1}),"Unknown CVar setter body.");
        l.setters.push_back(at);
    }
    require(shortFrames==2 && longFrames==2 && floatFrames==1,"Incomplete CVar setter family.");
    return l;
}
inline Bytes build(const Image& p,const Layout& l,int width,int height,bool proportional) {
    require(width>=320 && width<=16384 && height>=200 && height<=16384,"Resolution is outside supported input limits.");
    Bytes b=l.baseline; put<float>(b,l.aspect,float(width)/height);
    if(!proportional)return b;
    double scale=std::min(width/1024.0,height/768.0),cw=width/scale,ch=height/scale;
    for(auto at:{l.width,l.mouseW,l.clipW})put<float>(b,at,float(cw));
    for(auto at:{l.height,l.mouseH,l.clipH})put<float>(b,at,float(ch));
    put<float>(b,l.reciprocalX,float(1.0/cw));
    uint32_t sx=l.forwardX+17,sy=l.forwardY+17,reciprocal=l.inverseX+17;
    put<float>(b,reciprocal,float(1.0/ch));
    for(auto at:l.verticalRefs)put<uint32_t>(b,at,p.addressOf(reciprocal));
    branch(b,l.rootCall,p.base,p.addressOf(sx),0xe8);
    for(auto pair:std::vector<std::pair<uint32_t,uint8_t>>{{sx,4},{sy,8}}) {
        uint32_t at=pair.first;put<uint32_t>(b,at,0x002444c7u|(uint32_t(pair.second)<<24));
        put<float>(b,at+4,float(((at==sx?cw:ch)-(at==sx?1024:768))/2));
        branch(b,at+8,p.base,at==sx?p.addressOf(sy):l.rootInitializer,0xe9);
    }
    return b;
}
} // namespace em4
