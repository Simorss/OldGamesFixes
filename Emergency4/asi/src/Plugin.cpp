#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <cstdio>
#include <cerrno>
#include <mutex>
#include <memory>
#include <array>
#include <cwchar>
#include "Engine.hpp"

static_assert(sizeof(void*)==4,"Build this plugin for x86.");
namespace {
constexpr const char* Version="2.0.0-beta.2";
HMODULE moduleHandle=nullptr;
std::once_flag initialized;
FILE* logFile=nullptr;
uint32_t gameBase=0;
em4::Layout layout;
int resolutionX=0,resolutionY=0;
int windowed=0;
float focal=160;
double focalFactor=1;
int forcedBaseFov=0;
bool active=false;
#ifdef EM4_TEST
HMODULE fixtureModule=nullptr;
#endif
using Setter=bool(__thiscall*)(void*,uint32_t);
std::array<Setter,5> originalSetters{};

void log(const char* message) { if(logFile){std::fprintf(logFile,"%s\n",message);std::fflush(logFile);}OutputDebugStringA(message); }
std::wstring modulePath(HMODULE module) {
    std::vector<wchar_t> buf(32768);
    DWORD n=GetModuleFileNameW(module,buf.data(),DWORD(buf.size()));
    em4::require(n>0 && n<buf.size(),"Cannot determine module path.");
    return std::wstring(buf.data(),n);
}
std::wstring sibling(const std::wstring& path,const wchar_t* extension) {
    size_t dot=path.find_last_of(L'.');em4::require(dot!=std::wstring::npos,"Invalid plugin filename.");
    return path.substr(0,dot)+extension;
}
int readOption(const wchar_t* key,int fallback,const std::wstring& ini) {
    wchar_t buf[128];const wchar_t* missing=L"<missing>";
    DWORD n=GetPrivateProfileStringW(L"MAIN",key,missing,buf,128,ini.c_str());
    if(!std::wcscmp(buf,missing))return fallback;
    em4::require(n<127,"INI option is too long.");
    wchar_t* end=nullptr;errno=0;long v=std::wcstol(buf,&end,10);
    while(end && (*end==L' ' || *end==L'\t'))end++;
    em4::require(!errno && end!=buf && *end==0 && v>=INT32_MIN && v<=INT32_MAX,"Invalid integer in the MAIN section of the INI.");
    return int(v);
}
em4::Bytes snapshot(HMODULE game) {
    auto* base=reinterpret_cast<uint8_t*>(game);
    auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(game);
    em4::require(dos->e_magic==IMAGE_DOS_SIGNATURE && dos->e_lfanew>0 && dos->e_lfanew<0x100000,"Invalid game image header.");
    auto* nt=reinterpret_cast<IMAGE_NT_HEADERS32*>(base+dos->e_lfanew);
    em4::require(nt->Signature==IMAGE_NT_SIGNATURE && nt->FileHeader.Machine==IMAGE_FILE_MACHINE_I386,"Not an x86 game image.");
    uint32_t size=nt->OptionalHeader.SizeOfImage;
    em4::require(size>=4096 && size<256*1024*1024,"Unsupported image size.");
    em4::Bytes result(size,0);
    for(uint32_t at=0;at<size;) {
        MEMORY_BASIC_INFORMATION info{};
        em4::require(VirtualQuery(base+at,&info,sizeof info)!=0,"Cannot query the game image.");
        uintptr_t end=reinterpret_cast<uintptr_t>(info.BaseAddress)+info.RegionSize;
        em4::require(end>reinterpret_cast<uintptr_t>(base+at),"Invalid image memory region.");
        uint32_t count=uint32_t(std::min<uintptr_t>(size-at,end-reinterpret_cast<uintptr_t>(base+at)));
        if(info.State==MEM_COMMIT && !(info.Protect&(PAGE_NOACCESS|PAGE_GUARD)))std::memcpy(result.data()+at,base+at,count);
        at+=count;
    }
    return result;
}

void enforce(void* self) {
    if(!active)return;
    uint32_t address=reinterpret_cast<uint32_t>(self);float value;
    if(address==layout.widthObject)value=float(resolutionX);
    else if(address==layout.heightObject)value=float(resolutionY);
    else if(address==layout.fullscreenObject)value=windowed?0.f:1.f;
    else if(address==layout.fovObject) {
        float raw;std::memcpy(&raw,reinterpret_cast<uint8_t*>(self)+0x14,4);
        if(forcedBaseFov)value=float(forcedBaseFov*(4.0/3.0)/(double(resolutionX)/resolutionY));
        else value=float((em4::positive(raw)?raw:float(layout.defaultFov))*focalFactor);
        focal=value;
    }
    else return;
    // Keep the engine's string representation intact. Its normal config writer
    // can save the user's original value; overrides exist only in numeric state.
    auto* obj=reinterpret_cast<uint8_t*>(self);
    std::memcpy(obj+0x14,&value,4);int integer=int(value);std::memcpy(obj+0x18,&integer,4);obj[0x1c]=value!=0;
}
template<size_t N> bool __fastcall setHook(void* self,void*,uint32_t arg) {
    bool result=originalSetters[N](self,arg);if(result)enforce(self);return result;
}
const std::array<void*,5> hooks={{reinterpret_cast<void*>(&setHook<0>),reinterpret_cast<void*>(&setHook<1>),
    reinterpret_cast<void*>(&setHook<2>),reinterpret_cast<void*>(&setHook<3>),reinterpret_cast<void*>(&setHook<4>)}};

struct Write { uint8_t* target;em4::Bytes before,after; };
class Transaction {
    std::vector<Write> writes;
    std::vector<std::pair<uint8_t*,DWORD>> pages;
    bool committed=false;
public:
    ~Transaction() { if(!pages.empty()){SYSTEM_INFO info{};GetSystemInfo(&info);restoreProtection(info.dwPageSize);} }
    void add(uint8_t* target,const uint8_t* before,const uint8_t* after,size_t n) {
        em4::require(n>0,"Empty memory patch.");
        for(const auto& w:writes)em4::require(target+n<=w.target || w.target+w.after.size()<=target,"Overlapping memory patches.");
        writes.push_back({target,em4::Bytes(before,before+n),em4::Bytes(after,after+n)});
    }
    void apply() {
        SYSTEM_INFO system{};GetSystemInfo(&system);size_t pageSize=system.dwPageSize;
        for(const auto& w:writes)em4::require(!std::memcmp(w.target,w.before.data(),w.before.size()),"Game code changed during initialization; no patches applied.");
        // Acquire all write permissions before changing any byte.
        for(const auto& w:writes) {
            uintptr_t first=reinterpret_cast<uintptr_t>(w.target)&~(pageSize-1);
            uintptr_t last=(reinterpret_cast<uintptr_t>(w.target)+w.after.size()-1)&~(pageSize-1);
            for(uintptr_t at=first;at<=last;at+=pageSize) {
                auto* page=reinterpret_cast<uint8_t*>(at);
                if(std::any_of(pages.begin(),pages.end(),[&](const auto& p){return p.first==page;}))continue;
                DWORD old=0;
                if(!VirtualProtect(page,pageSize,PAGE_EXECUTE_READWRITE,&old)){restoreProtection(pageSize);throw std::runtime_error("Cannot make the game patch pages writable.");}
                pages.push_back({page,old});
            }
        }
        for(const auto& w:writes)std::memcpy(w.target,w.after.data(),w.after.size());
        if(!FlushInstructionCache(GetCurrentProcess(),nullptr,0)) {
            for(const auto& w:writes)std::memcpy(w.target,w.before.data(),w.before.size());
            FlushInstructionCache(GetCurrentProcess(),nullptr,0);restoreProtection(pageSize);
            throw std::runtime_error("Instruction cache flush failed; memory patches rolled back.");
        }
        committed=true;restoreProtection(pageSize);
    }
    void restoreProtection(size_t pageSize) {
        bool ok=true;for(auto it=pages.rbegin();it!=pages.rend();++it){DWORD ignored;if(!VirtualProtect(it->first,pageSize,it->second,&ignored))ok=false;}
        pages.clear();if(!ok)log("Warning: Windows could not restore an original page protection.");
    }
    bool done() const {return committed;}
};

void initialize() {
    uint8_t* gates=nullptr;bool installed=false;
    try {
        std::wstring path=modulePath(moduleHandle),ini=sibling(path,L".ini");
        logFile=_wfopen(sibling(path,L".log").c_str(),L"wb");
        log((std::string("Emergency 4 Widescreen Fix ASI ")+Version).c_str());
        int enabled=readOption(L"Enabled",1,ini);
        em4::require(enabled==0 || enabled==1,"Enabled must be 0 or 1.");
        if(!enabled){log("Disabled by the INI. No game memory changed.");if(logFile){std::fclose(logFile);logFile=nullptr;}return;}
        resolutionX=readOption(L"ResX",0,ini);resolutionY=readOption(L"ResY",0,ini);
        int proportional=readOption(L"KeepUIAspectRatio",1,ini);
        em4::require(proportional==0 || proportional==1,"KeepUIAspectRatio must be 0 or 1.");
        windowed=readOption(L"Windowed",0,ini);
        em4::require(windowed==0 || windowed==1,"Windowed must be 0 or 1.");
        forcedBaseFov=readOption(L"BaseFOV",0,ini);
        em4::require(forcedBaseFov>=0 && forcedBaseFov<=9999,"BaseFOV must be 0..9999.");
        if(!resolutionX && !resolutionY) {
            POINT point{};GetCursorPos(&point);HMONITOR monitor=MonitorFromPoint(point,MONITOR_DEFAULTTOPRIMARY);
            MONITORINFOEXW info{};info.cbSize=sizeof info;
            em4::require(GetMonitorInfoW(monitor,&info)!=0,"Cannot detect the active display.");
            DEVMODEW mode{};mode.dmSize=sizeof mode;
            em4::require(EnumDisplaySettingsW(info.szDevice,ENUM_CURRENT_SETTINGS,&mode)!=0,"Cannot detect the desktop resolution.");
            resolutionX=int(mode.dmPelsWidth);resolutionY=int(mode.dmPelsHeight);
        }
        em4::require(resolutionX>=320 && resolutionX<=16384 && resolutionY>=200 && resolutionY<=16384,"Set both ResX and ResY, or set both to 0 for automatic resolution.");
        auto game=GetModuleHandleW(nullptr);
#ifdef EM4_TEST
        if(fixtureModule)game=fixtureModule;
#endif
        gameBase=reinterpret_cast<uint32_t>(game);
        em4::Image image(snapshot(game),gameBase);layout=em4::inspect(image);
        // Scale each freshly assigned config value exactly once. A recognized
        // older executable patch already used previousAspect, so recover its
        // original focal parameter through that aspect rather than assuming 4:3.
        focalFactor=double(layout.previousAspect)/(double(resolutionX)/resolutionY);
        em4::Bytes desired=em4::build(image,layout,resolutionX,resolutionY,proportional!=0);
        Transaction transaction;
        for(size_t at=0;at<desired.size();) {
            if(desired[at]==image.bytes[at]){at++;continue;}
            size_t start=at;while(at<desired.size() && desired[at]!=image.bytes[at])at++;
            transaction.add(reinterpret_cast<uint8_t*>(game)+start,image.bytes.data()+start,desired.data()+start,at-start);
        }
        // Each gateway copies exactly one six-byte FS load, then jumps back.
        // No partial instruction or PC-relative instruction is relocated.
        gates=static_cast<uint8_t*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
        em4::require(gates!=nullptr,"Cannot allocate CVar gateways.");
        for(size_t n=0;n<5;n++) {
            uint32_t at=layout.setters[n];
            em4::require(em4::equal(image.bytes,at,{0x64,0xa1,0,0,0,0}),"Unsupported CVar hook instruction.");
            em4::Bytes gateway(11);std::copy_n(image.bytes.begin()+at,6,gateway.begin());
            em4::branch(gateway,6,reinterpret_cast<uint32_t>(gates+n*16),gameBase+at+6,0xe9);
            std::memcpy(gates+n*16,gateway.data(),gateway.size());
            originalSetters[n]=reinterpret_cast<Setter>(gates+n*16);
            em4::Bytes jump(6,0x90);em4::branch(jump,0,gameBase+at,reinterpret_cast<uint32_t>(hooks[n]),0xe9);
            transaction.add(reinterpret_cast<uint8_t*>(game)+at,image.bytes.data()+at,jump.data(),6);
        }
        DWORD ignored=0;
        em4::require(VirtualProtect(gates,4096,PAGE_EXECUTE_READ,&ignored)!=0,"Cannot protect CVar gateways.");
        em4::require(FlushInstructionCache(GetCurrentProcess(),gates,4096)!=0,"Cannot flush CVar gateways.");
        // Pin the plugin before placing any permanent branch to its code.
        HMODULE pinned=nullptr;
        em4::require(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&initialize),&pinned)!=0,"Cannot pin the plugin module.");
        transaction.apply();installed=transaction.done();active=true;
        // Handles late initialization and constructor defaults; future assignments
        // are enforced by the typed setter hooks, without scaling a value twice.
        for(auto object:{layout.widthObject,layout.heightObject,layout.fovObject,layout.fullscreenObject})enforce(reinterpret_cast<void*>(object));
        char summary[256];std::snprintf(summary,sizeof summary,"Applied: %dx%d; windowed=%d; proportional menu/HUD=%d; FOV source=%s; 5 CVar hooks.",resolutionX,resolutionY,windowed,proportional,forcedBaseFov?"INI BaseFOV":"game configuration");log(summary);
        if(std::abs(layout.previousAspect-4.f/3.f)>0.00001f)log("A previous executable patch was normalized in memory. Restore the previous on-disk patch for a clean migration.");
        log("The plugin does not write the game executable or configuration. Restart the game after editing the INI.");
    } catch(const std::exception& e) {
        log((std::string("Initialization stopped: ")+e.what()).c_str());
        if(!installed && gates)VirtualFree(gates,0,MEM_RELEASE);
    }
    if(logFile){std::fclose(logFile);logFile=nullptr;}
}
} // namespace

extern "C" __declspec(dllexport) void InitializeASI() { std::call_once(initialized,initialize); }
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH){moduleHandle=module;DisableThreadLibraryCalls(module);}
    return TRUE;
}
