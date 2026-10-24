#define _WIN32_WINNT 0x0601
#define WINVER 0x0601
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <wincrypt.h>
#include <d3d9.h>
#include <algorithm>
#include <atomic>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <iterator>
#include <mutex>
#include <string>
#include <vector>
#include "FovMath.h"
#include "InputMath.h"

static_assert(sizeof(void*) == 4, "Build this ASI for x86.");
namespace {
HMODULE self;
std::once_flag once;
std::wstring iniPath, logPath;
double baseFov=70.0, step=1.0;
bool horPlus=true, persist=true;
double viewmodelFov=70.0,viewmodelStep=1.0;
bool viewmodelEnabled=true,viewmodelHorPlus=true,viewmodelPersist=true;
bool mouseEnabled=true,noAcceleration=true,noSmoothing=true;
int osdMs=2200;
enum class Setting {Camera,Viewmodel};
std::atomic<Setting> osdSetting{Setting::Camera};
struct Hotkeys {int decrease,increase;regiment::RepeatKey lower,upper;};
Hotkeys hotkeys[2]={{VK_F7,VK_F8,{},{}},{VK_F11,VK_F12,{},{}}};
std::atomic<float> currentFov{70.0f};
std::atomic<float> currentViewmodelFov{70.0f};
std::atomic<DWORD> lastViewmodel{0},lastInput{0};
std::atomic<DWORD> lastCamera{0}, osdUntil{0};
std::atomic<int> outputWidth{0},outputHeight{0};
std::atomic<bool> resolutionOsdPending{false};
DWORD saveAt=0;
unsigned dirtySettings=0;
HWND gameWindow=nullptr;

void Log(const char* format,...) {
    FILE* f=_wfopen(logPath.c_str(),L"a"); if(!f) return;
    std::fprintf(f,"[%lu] ",GetTickCount()); va_list args;va_start(args,format);
    std::vfprintf(f,format,args);va_end(args);std::fputc('\n',f);std::fclose(f);
}
double Number(const wchar_t* section,const wchar_t* key,double fallback,double low,double high) {
    wchar_t text[80], def[80];std::swprintf(def,80,L"%.3f",fallback);
    GetPrivateProfileStringW(section,key,def,text,80,iniPath.c_str());
    wchar_t* end=nullptr;double value=std::wcstod(text,&end);
    if(end==text || *end || !std::isfinite(value))return fallback;
    return std::clamp(value,low,high);
}
int Key(const wchar_t* name,int fallback) {
    wchar_t text[80];GetPrivateProfileStringW(L"Keys",name,L"",text,80,iniPath.c_str());
    if(!text[0])return fallback;
    if(!_wcsicmp(text,L"Comma") || !wcscmp(text,L","))return VK_OEM_COMMA;
    if(!_wcsicmp(text,L"Period") || !wcscmp(text,L"."))return VK_OEM_PERIOD;
    if((text[0]==L'F'||text[0]==L'f') && text[1]){
        wchar_t* end=nullptr;long n=std::wcstol(text+1,&end,10);
        if(!*end && n>=1 && n<=24)return VK_F1+static_cast<int>(n)-1;
    }
    wchar_t* end=nullptr;long n=std::wcstol(text,&end,0);
    if(end!=text && !*end && n>=1 && n<=254)return static_cast<int>(n);
    Log("Invalid key setting, using default: %d",fallback);return fallback;
}
void Save() {
    struct Item {unsigned bit;bool enabled;const wchar_t* section;const wchar_t* key;double value;};
    Item items[]={{1,persist,L"FOV",L"BaseFOV",baseFov},{2,viewmodelPersist,L"Viewmodel",L"BaseFOV",viewmodelFov}};
    for(auto& item:items){
        if(!(dirtySettings&item.bit))continue;
        if(!item.enabled){dirtySettings&=~item.bit;continue;}
        wchar_t value[48];std::swprintf(value,48,L"%.6f",item.value);
        if(WritePrivateProfileStringW(item.section,item.key,value,iniPath.c_str()))dirtySettings&=~item.bit;
        else Log("Could not save setting %u (Windows error %lu)",item.bit,GetLastError());
    }
}
bool Readable(const void* address,size_t length) {
    if(!address || !length)return false;
    MEMORY_BASIC_INFORMATION m{};
    if(!VirtualQuery(address,&m,sizeof(m)) || m.State!=MEM_COMMIT ||
       (m.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
    auto start=reinterpret_cast<uintptr_t>(address), end=start+length;
    return end>=start && end<=reinterpret_cast<uintptr_t>(m.BaseAddress)+m.RegionSize;
}
bool Replace(void* target,const void* data,size_t n) {
    DWORD old=0;if(!VirtualProtect(target,n,PAGE_EXECUTE_READWRITE,&old))return false;
    std::memcpy(target,data,n);FlushInstructionCache(GetCurrentProcess(),target,n);
    DWORD ignored=0;VirtualProtect(target,n,old,&ignored);return true;
}
void* Slot(void** table,size_t index,void* hook) {
    void* previous=table[index];if(previous==hook)return nullptr;
    return Replace(&table[index],&hook,sizeof(hook))?previous:nullptr;
}
bool SupportedExe() {
    wchar_t path[MAX_PATH];if(!GetModuleFileNameW(nullptr,path,MAX_PATH))return false;
    HCRYPTPROV provider=0;HCRYPTHASH hash=0;
    if(!CryptAcquireContextW(&provider,nullptr,nullptr,PROV_RSA_AES,CRYPT_VERIFYCONTEXT))return false;
    if(!CryptCreateHash(provider,CALG_SHA_256,0,0,&hash)){CryptReleaseContext(provider,0);return false;}
    FILE* f=_wfopen(path,L"rb");bool ok=f!=nullptr;BYTE buf[65536];
    if(f){size_t count;while((count=fread(buf,1,sizeof(buf),f))!=0)if(!CryptHashData(hash,buf,static_cast<DWORD>(count),0)){ok=false;break;}if(ferror(f))ok=false;fclose(f);}
    BYTE digest[32];DWORD length=32;ok=ok && CryptGetHashParam(hash,HP_HASHVAL,digest,&length,0);
    CryptDestroyHash(hash);CryptReleaseContext(provider,0);
    char hex[65]{};if(ok)for(int i=0;i<32;++i)std::snprintf(hex+i*2,3,"%02x",digest[i]);
    Log("Executable SHA256: %s",ok?hex:"unavailable");
    return ok && !strcmp(hex,"f7787e7c1aa58416bc3222259cbeac02f7431794f1c3678c28eba14404a90c66");
}

struct Vec {float x,y,z;};struct Rot {int pitch,yaw,roll;};
using CameraCtor=void* (__thiscall*)(void*,void*,void*,void*,Vec,Rot,float);
CameraCtor originalCamera[2]{};
struct Trampoline {BYTE* target=nullptr;BYTE saved[8]{};BYTE* code=nullptr;bool installed=false;};
Trampoline cameras[2];
// The two player-scene constructors have the same ABI. Their first eight bytes
// are two complete MOV instructions, without relative or absolute addresses.
bool PrepareCamera(size_t index,BYTE* address) {
    constexpr BYTE expected[8]={0x8b,0x44,0x24,0x28,0x8b,0x54,0x24,0x1c};
    if(!Readable(address,8) || memcmp(address,expected,8))return false;
    auto& t=cameras[index];t.target=address;memcpy(t.saved,address,8);
    t.code=static_cast<BYTE*>(VirtualAlloc(nullptr,16,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!t.code)return false;
    memcpy(t.code,t.saved,8);t.code[8]=0xe9;
    *reinterpret_cast<int32_t*>(t.code+9)=static_cast<int32_t>((address+8)-(t.code+13));
    DWORD old=0;if(!VirtualProtect(t.code,16,PAGE_EXECUTE_READ,&old))return false;
    FlushInstructionCache(GetCurrentProcess(),t.code,16);
    originalCamera[index]=reinterpret_cast<CameraCtor>(t.code);return true;
}
bool TargetAspect(void* renderTarget,double& aspect,int& w,int& h) {
    if(!Readable(renderTarget,4))return false;
    void** vtable=*static_cast<void***>(renderTarget);
    if(!Readable(vtable,11*sizeof(void*)))return false;
    // FRenderTarget::GetWidth/GetHeight, verified against UpdateMatrices.
    using Dimension=int (__thiscall*)(void*);
    w=reinterpret_cast<Dimension>(vtable[9])(renderTarget);
    h=reinterpret_cast<Dimension>(vtable[10])(renderTarget);
    if(w<64 || h<64 || w>32768 || h>32768)return false;
    aspect=static_cast<double>(w)/h;return true;
}
void ObserveOutputSize(int width,int height){
    if(width<64||height<64||width>32768||height>32768)return;
    int oldWidth=outputWidth.exchange(width),oldHeight=outputHeight.exchange(height);
    if(oldWidth&&oldHeight&&(width!=oldWidth||height!=oldHeight)){
        resolutionOsdPending=true;
        Log("Output resolution changed: %dx%d -> %dx%d; waiting for updated player camera",oldWidth,oldHeight,width,height);
    }
}
void ObserveResolution(IDirect3DDevice9* device){
    // Back-buffer dimensions remain authoritative during off-screen/LOD passes.
    // No surface is retained across frames or device Reset.
    IDirect3DSurface9* buffer=nullptr;
    if(FAILED(device->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&buffer)))return;
    D3DSURFACE_DESC desc{};HRESULT result=buffer->GetDesc(&desc);buffer->Release();
    if(SUCCEEDED(result))ObserveOutputSize(static_cast<int>(desc.Width),static_cast<int>(desc.Height));
}
float CameraFov(void* renderTarget,float incoming) {
    double aspect=0;int w=0,h=0;if(!TargetAspect(renderTarget,aspect,w,h))return incoming;
    float result=regiment::CorrectFov(incoming,aspect,baseFov,horPlus);
    // Still correct secondary scenes, but never use them to announce a display
    // change or overwrite the displayed FOV with an off-screen projection.
    int width=outputWidth.load(),height=outputHeight.load();
    if((width&&height&&(w!=width||h!=height))||!std::isfinite(incoming)||incoming<=1||incoming>=179)return result;
    DWORD now=GetTickCount();
    bool first=lastCamera.load(std::memory_order_relaxed)==0;
    currentFov.store(result,std::memory_order_relaxed);
    lastCamera.store(now,std::memory_order_relaxed);
    bool resized=resolutionOsdPending.exchange(false);
    if(first||resized){
        osdSetting=Setting::Camera;osdUntil=now+osdMs;
        Log("%s: %dx%d, incoming %.3f, horizontal %.3f",first?"Player camera active":"Resolution FOV updated",w,h,incoming,result);
    }
    return result;
}
void CorrectScene(void* object,void* target,float incoming) {
    // Rebuild after the original constructor. Some player scenes construct a
    // separate LOD scene internally; changing their input FOV would risk applying
    // the correction twice to that nested scene.
    if(!Readable(object,0x1e8))return;
    auto table=*static_cast<void***>(object);
    if(!Readable(table,14*sizeof(void*)))return;
    float* fov=reinterpret_cast<float*>(static_cast<BYTE*>(object)+0x1e4);
    if(!std::isfinite(*fov) || std::fabs(*fov-incoming)>.01f)return;
    *fov=CameraFov(target,incoming);
    using UpdateMatrices=void (__thiscall*)(void*);
    reinterpret_cast<UpdateMatrices>(table[13])(object);
}
void* __fastcall Camera0(void* object,void*,void* viewport,void* target,void* actor,Vec pos,Rot rot,float fov){auto result=originalCamera[0](object,viewport,target,actor,pos,rot,fov);CorrectScene(object,target,fov);return result;}
void* __fastcall Camera1(void* object,void*,void* viewport,void* target,void* actor,Vec pos,Rot rot,float fov){auto result=originalCamera[1](object,viewport,target,actor,pos,rot,fov);CorrectScene(object,target,fov);return result;}
bool InstallCamera(size_t index,void* hook) {
    BYTE patch[8]={0xe9,0,0,0,0,0x90,0x90,0x90};auto& t=cameras[index];
    *reinterpret_cast<int32_t*>(patch+1)=static_cast<int32_t>(reinterpret_cast<BYTE*>(hook)-(t.target+5));
    t.installed=Replace(t.target,patch,8);return t.installed;
}

struct SmallHook {
    BYTE* target=nullptr;BYTE saved[16]{};BYTE* code=nullptr;size_t length=0;bool installed=false;
    bool Prepare(BYTE* address,const BYTE* expected,size_t count){
        if(count<5||count>sizeof(saved)||!Readable(address,count)||memcmp(address,expected,count))return false;
        target=address;length=count;memcpy(saved,address,count);
        code=static_cast<BYTE*>(VirtualAlloc(nullptr,count+5,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return false;
        memcpy(code,saved,count);code[count]=0xe9;
        *reinterpret_cast<int32_t*>(code+count+1)=static_cast<int32_t>((address+count)-(code+count+5));
        DWORD old=0;if(!VirtualProtect(code,count+5,PAGE_EXECUTE_READ,&old))return false;
        FlushInstructionCache(GetCurrentProcess(),code,count+5);return true;
    }
    bool Install(void* hook){
        BYTE patch[16];memset(patch,0x90,length);patch[0]=0xe9;
        *reinterpret_cast<int32_t*>(patch+1)=static_cast<int32_t>(reinterpret_cast<BYTE*>(hook)-(target+5));
        installed=Replace(target,patch,length);return installed;
    }
    void Restore(){if(installed && Replace(target,saved,length))installed=false;}
};
SmallHook viewmodelHook,inputHook;
using NameEntryFn=void* (__cdecl*)(int);
using NameTextFn=const wchar_t* (__thiscall*)(void*);
NameEntryFn nameEntry=nullptr;NameTextFn nameText=nullptr;
bool HasNamedClass(void* object,const wchar_t* wanted){
    if(!nameEntry||!nameText||!Readable(object,0x28))return false;
    void* cls=*reinterpret_cast<void**>(static_cast<BYTE*>(object)+0x24);
    for(int depth=0;depth<64&&cls;++depth){
        if(!Readable(cls,0x2c))return false;
        int id=*reinterpret_cast<int*>(static_cast<BYTE*>(cls)+0x20);
        if(id<0||id>0x1000000)return false;
        void* entry=nameEntry(id);if(!Readable(entry,12))return false;
        const wchar_t* text=nameText(entry);
        if(Readable(text,2)&&!wcscmp(text,wanted))return true;
        void* parent=*reinterpret_cast<void**>(static_cast<BYTE*>(cls)+0x28);
        if(parent==cls)return false;
        cls=parent;
    }
    return false;
}
using DrawActorFn=void (__thiscall*)(void*,void*,int,int,float);
using ProcessInternalFn=void (__thiscall*)(void*,void*,void*);
DrawActorFn originalDrawActor=nullptr;ProcessInternalFn originalProcessInternal=nullptr;
const int* playerInputName=nullptr;
float ViewmodelFov(void* canvas,void* actor,int wireframe,int clearZ,float incoming){
    // The game deliberately submits FOV=0 to update hidden torch/laser effects
    // in scoped view. Preserve those submissions exactly.
    if(!viewmodelEnabled||wireframe||!clearZ||!std::isfinite(incoming)||incoming<=1||incoming>=179||!HasNamedClass(actor,L"SASWeapon")||!Readable(canvas,0x1e0))return incoming;
    void* viewport=*reinterpret_cast<void**>(static_cast<BYTE*>(canvas)+0x1dc);
    if(!Readable(viewport,0x90))return incoming;
    // UViewport +0x148 is FRenderInterface*, not FRenderTarget*. Calling its
    // slots as GetWidth/GetHeight has a different ABI and can crash. The
    // original DrawActor reads SizeX/SizeY here, before its temporary 512x512
    // off-screen pass, so read those same dimensions without any virtual call.
    int w=*reinterpret_cast<int*>(static_cast<BYTE*>(viewport)+0x88);
    int h=*reinterpret_cast<int*>(static_cast<BYTE*>(viewport)+0x8c);
    if(w<64||h<64||w>32768||h>32768)return incoming;
    double aspect=static_cast<double>(w)/h;
    float result=regiment::CorrectViewmodelFov(incoming,aspect,viewmodelFov,viewmodelHorPlus);
    if(!lastViewmodel.load())Log("Viewmodel active: %dx%d, incoming %.3f, horizontal %.3f",w,h,incoming,result);
    currentViewmodelFov=result;lastViewmodel=GetTickCount();return result;
}
void __fastcall DrawActor(void* canvas,void*,void* actor,int wireframe,int clearZ,float fov){originalDrawActor(canvas,actor,wireframe,clearZ,ViewmodelFov(canvas,actor,wireframe,clearZ,fov));}
struct MouseOverride {
    BYTE* input;BYTE mode;float strength,accel;
    explicit MouseOverride(void* object):input(static_cast<BYTE*>(object)),mode(input[0x2c]),
        strength(*reinterpret_cast<float*>(input+0x30)),accel(*reinterpret_cast<float*>(input+0x3c)){
        if(noSmoothing){input[0x2c]=0;*reinterpret_cast<float*>(input+0x30)=0;}
        if(noAcceleration)*reinterpret_cast<float*>(input+0x3c)=0;
    }
    // Sensitivity belongs entirely to the game/menu: never read, write, or
    // restore that field here, including when the menu changes it in-event.
    ~MouseOverride(){input[0x2c]=mode;*reinterpret_cast<float*>(input+0x30)=strength;*reinterpret_cast<float*>(input+0x3c)=accel;}
};
void __fastcall ProcessInternal(void* object,void*,void* frame,void* result){
    // PlayerTick calls PlayerInput through UnrealScript. FFrame::Node is at +4;
    // frame and node are valid by the interpreter's native calling contract.
    void* node=frame?*reinterpret_cast<void**>(static_cast<BYTE*>(frame)+4):nullptr;
    bool inputEvent=node&&playerInputName&&*playerInputName&&
        *reinterpret_cast<int*>(static_cast<BYTE*>(node)+0x20)==*playerInputName;
    if(mouseEnabled && inputEvent && Readable(object,0x80) && HasNamedClass(object,L"PlayerInput")){
        MouseOverride settings(object);
        if(!lastInput.load())Log("Mouse filters active: acceleration disabled=%d, smoothing disabled=%d; sensitivity controlled by game menu",noAcceleration,noSmoothing);
        lastInput=GetTickCount();originalProcessInternal(object,frame,result);
    } else originalProcessInternal(object,frame,result);
}
void ChangeSetting(Setting setting,int direction,DWORD now){
    unsigned bit=1u<<static_cast<unsigned>(setting);
    double* value=setting==Setting::Camera?&baseFov:&viewmodelFov;
    double before=*value;
    if(setting==Setting::Camera)*value=regiment::ClampBase(*value+direction*step);
    else *value=regiment::ClampViewmodel(*value+direction*viewmodelStep);
    osdSetting=setting;osdUntil=now+osdMs;
    if(*value!=before){dirtySettings|=bit;saveAt=now+400;Log("Setting %u changed: %.6f",bit,*value);}
}
void Keys() {
    DWORD now=GetTickCount(),pid=0;HWND foreground=GetForegroundWindow();GetWindowThreadProcessId(foreground,&pid);
    bool active=pid==GetCurrentProcessId() && (!gameWindow || foreground==gameWindow || IsChild(gameWindow,foreground));
    bool modifiers=(GetAsyncKeyState(VK_CONTROL)|GetAsyncKeyState(VK_MENU)|GetAsyncKeyState(VK_SHIFT))&0x8000;
    bool any=false;
    for(int i=0;i<2;++i){
        auto& pair=hotkeys[i];bool enabled=i==0||viewmodelEnabled;
        bool down=active&&!modifiers&&enabled&&(GetAsyncKeyState(pair.decrease)&0x8000);
        bool up=active&&!modifiers&&enabled&&(GetAsyncKeyState(pair.increase)&0x8000);
        any=any||down||up;
        bool lower=pair.lower.Poll(down&&!up,now),upper=pair.upper.Poll(up&&!down,now);
        if(lastCamera.load()&&now-lastCamera.load()<1000 && (lower!=upper))ChangeSetting(static_cast<Setting>(i),upper?1:-1,now);
    }
    if(dirtySettings&&(static_cast<LONG>(now-saveAt)>=0||!any))Save();
}

// A tiny built-in 5x7 font keeps the OSD independent of D3DX, fonts and textures.
const BYTE* Glyph(char c) {
    static const BYTE glyphs[][7]={
        {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
        {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
        {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
        {14,17,17,15,1,1,14},
        {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
        {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
        {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
        {7,2,2,2,2,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
        {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
        {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
        {0,4,4,0,4,4,0},{0,0,0,0,0,6,6},{0,0,0,0,0,0,0}};
    int i=c>='0'&&c<='9'?c-'0':c>='A'&&c<='Z'?10+c-'A':c==':'?36:c=='.'?37:38;return glyphs[i];
}
struct Vertex{float x,y,z,rhw;DWORD color;};
void Rectangle(std::vector<Vertex>& v,float x,float y,float w,float h,DWORD color){
    Vertex a{x-.5f,y-.5f,0,1,color},b{x+w-.5f,y-.5f,0,1,color},c{x+w-.5f,y+h-.5f,0,1,color},d{x-.5f,y+h-.5f,0,1,color};v.insert(v.end(),{a,b,c,a,c,d});
}
void OsdText(char* text,size_t length){
    auto setting=osdSetting.load();
    if(setting==Setting::Viewmodel){
        if(lastViewmodel.load()&&GetTickCount()-lastViewmodel.load()<1000)std::snprintf(text,length,"VIEWMODEL FOV: %.1f",currentViewmodelFov.load());
        else std::snprintf(text,length,"VIEWMODEL BASE: %.1f",viewmodelFov);
    } else std::snprintf(text,length,"FOV: %.1f",currentFov.load());
}
void DrawOsd(IDirect3DDevice9* device) {
    if(!osdMs || static_cast<LONG>(GetTickCount()-osdUntil.load())>=0 || GetTickCount()-lastCamera.load()>1000)return;
    D3DVIEWPORT9 viewport{};if(FAILED(device->GetViewport(&viewport)))return;
    char text[64];OsdText(text,sizeof(text));
    float scale=std::clamp(viewport.Height/480.0f,1.0f,3.0f), pad=4*scale;
    float width=static_cast<float>(strlen(text))*6*scale;
    float x=viewport.X+(viewport.Width-width)/2, y=viewport.Y+12*scale;
    std::vector<Vertex> vertices;vertices.reserve(9000);
    Rectangle(vertices,x-pad,y-pad,width+2*pad,7*scale+2*pad,0xb0000000);
    for(size_t i=0;text[i];++i){auto glyph=Glyph(text[i]);for(int row=0;row<7;++row)for(int col=0;col<5;++col)if(glyph[row]&(1<<(4-col)))Rectangle(vertices,x+i*6*scale+col*scale,y+row*scale,scale,scale,0xfff0f0f0);}
    IDirect3DStateBlock9* state=nullptr;
    if(FAILED(device->CreateStateBlock(D3DSBT_ALL,&state)))return;
    if(FAILED(state->Capture())){state->Release();return;}
    device->SetVertexShader(nullptr);device->SetPixelShader(nullptr);
    device->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);device->SetTexture(0,nullptr);
    device->SetRenderState(D3DRS_ZENABLE,FALSE);device->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);device->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);device->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    device->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,FALSE);
    device->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
    device->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);device->SetRenderState(D3DRS_LIGHTING,FALSE);
    device->SetRenderState(D3DRS_FOGENABLE,FALSE);device->SetRenderState(D3DRS_STENCILENABLE,FALSE);
    device->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);device->SetRenderState(D3DRS_COLORWRITEENABLE,15);
    device->SetRenderState(D3DRS_SRGBWRITEENABLE,FALSE);device->SetRenderState(D3DRS_CLIPPLANEENABLE,0);
    device->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);device->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);
    device->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);device->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);
    device->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
    device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,static_cast<UINT>(vertices.size()/3),vertices.data(),sizeof(Vertex));
    state->Apply();state->Release();
}

using EndSceneFn=HRESULT(WINAPI*)(IDirect3DDevice9*);
using CreateDeviceFn=HRESULT(WINAPI*)(IDirect3D9*,UINT,D3DDEVTYPE,HWND,DWORD,D3DPRESENT_PARAMETERS*,IDirect3DDevice9**);
using Create9Fn=IDirect3D9*(WINAPI*)(UINT);
EndSceneFn originalEndScene=nullptr;CreateDeviceFn originalCreateDevice=nullptr;Create9Fn originalCreate9=nullptr;
HRESULT WINAPI EndScene(IDirect3DDevice9* device){ObserveResolution(device);Keys();DrawOsd(device);return originalEndScene(device);}
HRESULT WINAPI CreateDevice(IDirect3D9* d3d,UINT adapter,D3DDEVTYPE type,HWND window,DWORD flags,D3DPRESENT_PARAMETERS* params,IDirect3DDevice9** out){
    HRESULT result=originalCreateDevice(d3d,adapter,type,window,flags,params,out);
    if(SUCCEEDED(result)&&out&&*out){gameWindow=window;void** table=*reinterpret_cast<void***>(*out);
        if(table[42]!=reinterpret_cast<void*>(&EndScene)){
            auto previous=Slot(table,42,reinterpret_cast<void*>(&EndScene));
            if(previous)originalEndScene=reinterpret_cast<EndSceneFn>(previous);
            Log("Direct3D9 OSD %s",previous?"hooked":"hook failed");
        }
    }return result;
}
IDirect3D9* WINAPI Create9(UINT sdk){
    IDirect3D9* d3d=originalCreate9(sdk);if(d3d){void** table=*reinterpret_cast<void***>(d3d);
        if(table[16]!=reinterpret_cast<void*>(&CreateDevice)){
            auto previous=Slot(table,16,reinterpret_cast<void*>(&CreateDevice));
            if(previous)originalCreateDevice=reinterpret_cast<CreateDeviceFn>(previous);
        }
    }return d3d;
}
void** FindImport(HMODULE module,const char* dll,const char* name){
    auto image=reinterpret_cast<BYTE*>(module);auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(image);
    auto nt=reinterpret_cast<IMAGE_NT_HEADERS*>(image+dos->e_lfanew);
    auto rva=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;if(!rva)return nullptr;
    auto desc=reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(image+rva);
    for(;desc->Name;++desc){if(_stricmp(reinterpret_cast<char*>(image+desc->Name),dll)||!desc->OriginalFirstThunk)continue;
        auto names=reinterpret_cast<IMAGE_THUNK_DATA*>(image+desc->OriginalFirstThunk);auto funcs=reinterpret_cast<IMAGE_THUNK_DATA*>(image+desc->FirstThunk);
        for(;names->u1.AddressOfData;++names,++funcs){if(IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal))continue;
            auto entry=reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(image+names->u1.AddressOfData);
            if(!strcmp(reinterpret_cast<char*>(entry->Name),name))return reinterpret_cast<void**>(&funcs->u1.Function);
        }
    }return nullptr;
}
#include "Settings.inl"
void Init(){
    wchar_t path[MAX_PATH];GetModuleFileNameW(self,path,MAX_PATH);std::wstring stem=path;stem.resize(stem.find_last_of(L'.'));
    iniPath=stem+L".ini";logPath=stem+L".log";
    Log("The Regiment FOV Fix 1.0, x86");
    if(!Flag(L"Main",L"Enabled",true)){
        Log("Disabled by [Main] Enabled=0. No hooks installed; FOV, mouse, hotkeys and OSD inactive.");return;
    }
    if(!SupportedExe()){Log("UNSUPPORTED executable. No camera or Direct3D hooks installed.");return;}
    LoadSettings();
    void** iat=FindImport(GetModuleHandleW(nullptr),"d3d9.dll","Direct3DCreate9");
    auto image=reinterpret_cast<BYTE*>(GetModuleHandleW(nullptr));
    const BYTE drawPrologue[5]={0x55,0x8b,0xec,0x6a,0xff};
    if(!iat || !PrepareCamera(0,image+0x6fcf0) || !PrepareCamera(1,image+0x1f9160) ||
        !viewmodelHook.Prepare(image+0x15ebf0,drawPrologue,5) || !inputHook.Prepare(image+0x18bb90,drawPrologue,5)){
        Log("Required import or camera signature missing. Initialization stopped without patching.");return;}
    nameEntry=reinterpret_cast<NameEntryFn>(image+0x38a40);nameText=reinterpret_cast<NameTextFn>(image+0x388b0);
    playerInputName=reinterpret_cast<const int*>(image+0x9f3b0c);
    originalDrawActor=reinterpret_cast<DrawActorFn>(viewmodelHook.code);originalProcessInternal=reinterpret_cast<ProcessInternalFn>(inputHook.code);
    originalCreate9=reinterpret_cast<Create9Fn>(*iat);
    void* createHook=reinterpret_cast<void*>(&Create9);
    if(!Replace(iat,&createHook,sizeof(createHook))){Log("Could not install Direct3D9 import hook.");return;}
    if(!InstallCamera(0,reinterpret_cast<void*>(&Camera0)) || !InstallCamera(1,reinterpret_cast<void*>(&Camera1)) ||
       !viewmodelHook.Install(reinterpret_cast<void*>(&DrawActor)) || !inputHook.Install(reinterpret_cast<void*>(&ProcessInternal))){
        for(auto& camera:cameras)if(camera.installed)Replace(camera.target,camera.saved,8);
        viewmodelHook.Restore();inputHook.Restore();
        void* original=reinterpret_cast<void*>(originalCreate9);Replace(iat,&original,sizeof(original));
        Log("Hook installation failed; original entry points restored.");return;
    }
    Log("Ready. Camera BaseFOV=%.3f HorPlus=%d; viewmodel BaseFOV=%.3f HorPlus=%d; mouse sensitivity controlled by game menu",baseFov,horPlus,viewmodelFov,viewmodelHorPlus);
    Log("Hotkeys: camera 0x%02X/0x%02X, viewmodel 0x%02X/0x%02X",hotkeys[0].decrease,hotkeys[0].increase,hotkeys[1].decrease,hotkeys[1].increase);
}
}
extern "C" __declspec(dllexport) void InitializeASI(){std::call_once(once,Init);}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){self=instance;DisableThreadLibraryCalls(instance);}return TRUE;}
