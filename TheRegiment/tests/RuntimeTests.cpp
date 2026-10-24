#include "../src/Plugin.cpp"
#include <cstdlib>
#include <limits>
static int checks=0;
void Check(bool condition,const char* message){++checks;if(!condition){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
void Near(double a,double b,const char* message){Check(std::abs(a-b)<0.0002,message);}
int targetWidth=3440,targetHeight=1440;
int __fastcall Width(void*,void*){return targetWidth;}
int __fastcall Height(void*,void*){return targetHeight;}
int updates=0;float matrixFov=0;
void __fastcall Matrices(void* scene,void*){++updates;matrixFov=*reinterpret_cast<float*>(static_cast<BYTE*>(scene)+0x1e4);}
float recordedIncoming=0;
void TestMath(){
    using namespace regiment;
    Near(CorrectFov(70,4.0/3,70,true),70,"4:3 unchanged");
    for(double aspect:{4.0/3,16.0/9,16.0/10,3440.0/1440,32.0/9}){
        double result=CorrectFov(70,aspect,70,true);
        double vfov=360/Pi*atan(tan(result*Pi/360)/aspect);
        double expected=360/Pi*atan(tan(70*Pi/360)/ReferenceAspect);
        Near(vfov,expected,"constant vertical FOV across aspect ratios");
        double zoom=CorrectFov(35,aspect,70,true);
        Near(tan(zoom*Pi/360)/tan(result*Pi/360),tan(35*Pi/360)/tan(70*Pi/360),"zoom multiplier preserved");
    }
    Near(CorrectFov(70,3440.0/1440,80,false),80,"manual FOV with HorPlus disabled");
    Near(CorrectFov(70,4.0/3,80,true),80,"manual 4:3 base");
    Near(CorrectFov(70,0,70,true),70,"reject invalid aspect");
    Near(CorrectFov(0,2.4,70,true),0,"reject invalid input FOV");
    Check(std::isnan(CorrectFov(std::numeric_limits<float>::quiet_NaN(),2.4,70,true)),"NaN input untouched");
    Near(ClampBase(0),30,"lower limit");Near(ClampBase(200),110,"upper limit");
    std::printf("Hor+ at 3440x1440: %.4f degrees\n",CorrectFov(70,3440.0/1440,70,true));
}
void TestHooks(){
    void* table[14]{};table[13]=reinterpret_cast<void*>(&Matrices);
    void* targetTable[11]{};targetTable[9]=reinterpret_cast<void*>(&Width);targetTable[10]=reinterpret_cast<void*>(&Height);
    void** target=targetTable;
    alignas(4) BYTE scene[0x1e8]{};*reinterpret_cast<void***>(scene)=table;
    for(int index=0;index<2;++index){
        BYTE* code=static_cast<BYTE*>(VirtualAlloc(nullptr,64,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
        // MOV EAX,[ESP+28]; MOV EDX,[ESP+1C]; MOV [recordedIncoming],EAX;
        // MOV [ECX+1E4],EAX; MOV EAX,ECX; RET 28.
        BYTE fixture[]={0x8b,0x44,0x24,0x28,0x8b,0x54,0x24,0x1c,0xa3,0,0,0,0,0x89,0x81,0xe4,1,0,0,0x8b,0xc1,0xc2,0x28,0};
        *reinterpret_cast<uintptr_t*>(fixture+9)=reinterpret_cast<uintptr_t>(&recordedIncoming);
        memcpy(code,fixture,sizeof(fixture));FlushInstructionCache(GetCurrentProcess(),code,sizeof(fixture));
        Check(PrepareCamera(index,code),"prepare x86 trampoline");
        Check(InstallCamera(index,index?reinterpret_cast<void*>(&Camera1):reinterpret_cast<void*>(&Camera0)),"install real detour");
        auto constructor=reinterpret_cast<CameraCtor>(code);
        for(float incoming:{70.0f,35.0f,90.0f}){
            int before=updates;auto result=constructor(scene,nullptr,&target,nullptr,Vec{1,2,3},Rot{4,5,6},incoming);
            Check(result==scene,"thiscall return and stack cleanup");
            Near(recordedIncoming,incoming,"native constructor receives original FOV");
            Near(matrixFov,regiment::CorrectFov(incoming,3440.0/1440,70,true),"matrix rebuild receives corrected FOV");
            Check(updates==before+1,"one matrix rebuild per scene");
        }
        Check(Replace(code,cameras[index].saved,8),"restore original bytes");
        constructor(scene,nullptr,&target,nullptr,Vec{},Rot{},70);Near(*reinterpret_cast<float*>(scene+0x1e4),70,"detour rollback");
        VirtualFree(code,0,MEM_RELEASE);VirtualFree(cameras[index].code,0,MEM_RELEASE);
    }
}
void TestD3D(){
    HINSTANCE instance=GetModuleHandleW(nullptr);WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=instance;wc.lpszClassName=L"RegimentFovTests";
    RegisterClassW(&wc);HWND window=CreateWindowW(wc.lpszClassName,L"Hidden renderer test",WS_OVERLAPPEDWINDOW,0,0,800,600,nullptr,nullptr,instance,nullptr);
    Check(window!=nullptr,"create hidden test window");IDirect3D9* api=Direct3DCreate9(D3D_SDK_VERSION);Check(api!=nullptr,"create D3D9");
    D3DPRESENT_PARAMETERS p{};p.Windowed=TRUE;p.SwapEffect=D3DSWAPEFFECT_DISCARD;p.hDeviceWindow=window;p.BackBufferWidth=800;p.BackBufferHeight=600;p.BackBufferFormat=D3DFMT_X8R8G8B8;
    IDirect3DDevice9* device=nullptr;
    HRESULT hr=api->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,window,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&p,&device);
    Check(SUCCEEDED(hr),"create hidden D3D9 hardware device");
    outputWidth=0;outputHeight=0;resolutionOsdPending=false;
    ObserveResolution(device);Check(outputWidth==800&&outputHeight==600&&!resolutionOsdPending,"real back buffer establishes initial resolution without resize event");
    for(int pass=0;pass<4;++pass){
        currentFov=102.7f;lastCamera=GetTickCount();osdUntil=GetTickCount()+5000;
        osdSetting=static_cast<Setting>(pass/2);currentViewmodelFov=115.2f;lastViewmodel=GetTickCount();
        device->SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE);device->SetRenderState(D3DRS_FOGENABLE,TRUE);device->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
        device->SetRenderState(D3DRS_FILLMODE,D3DFILL_WIREFRAME);
        device->Clear(0,nullptr,D3DCLEAR_TARGET,0xff112233,1,0);Check(SUCCEEDED(device->BeginScene()),"begin scene");DrawOsd(device);device->EndScene();
        DWORD value=0;device->GetFVF(&value);Check(value==(D3DFVF_XYZ|D3DFVF_DIFFUSE),"FVF restored");device->GetRenderState(D3DRS_FOGENABLE,&value);Check(value==TRUE,"fog state restored");device->GetRenderState(D3DRS_ALPHABLENDENABLE,&value);Check(value==FALSE,"blend state restored");device->GetRenderState(D3DRS_FILLMODE,&value);Check(value==D3DFILL_WIREFRAME,"fill mode restored");
        IDirect3DSurface9 *rt=nullptr,*copy=nullptr;device->GetRenderTarget(0,&rt);D3DSURFACE_DESC desc{};rt->GetDesc(&desc);
        Check(SUCCEEDED(device->CreateOffscreenPlainSurface(desc.Width,desc.Height,desc.Format,D3DPOOL_SYSTEMMEM,&copy,nullptr)),"create readback surface");
        Check(SUCCEEDED(device->GetRenderTargetData(rt,copy)),"OSD readback");D3DLOCKED_RECT lock{};Check(SUCCEEDED(copy->LockRect(&lock,nullptr,D3DLOCK_READONLY)),"read OSD pixels");
        int white=0;for(UINT y=0;y<desc.Height;++y){DWORD* row=reinterpret_cast<DWORD*>(static_cast<BYTE*>(lock.pBits)+y*lock.Pitch);for(UINT x=0;x<desc.Width;++x)if((row[x]&0xffffff)==0xf0f0f0)++white;}
        Check(white>100,"OSD text actually rendered");std::printf("D3D9 OSD pass %d: %d text pixels\n",pass,white);
        copy->UnlockRect();copy->Release();rt->Release();
        if(pass%2==0){p.BackBufferWidth=pass==2?800:1280;p.BackBufferHeight=pass==2?600:720;Check(SUCCEEDED(device->Reset(&p)),"device reset / resolution change");
            ObserveResolution(device);Check(outputWidth==static_cast<int>(p.BackBufferWidth)&&outputHeight==static_cast<int>(p.BackBufferHeight)&&resolutionOsdPending,"real device Reset schedules resolution FOV notification");resolutionOsdPending=false;
            ObserveResolution(device);Check(!resolutionOsdPending,"same real back buffer does not reschedule OSD");}
    }
    device->Release();api->Release();DestroyWindow(window);UnregisterClassW(wc.lpszClassName,instance);
}
#include "TestExtensions.inl"
int main(int argc,char** argv){logPath=L"RuntimeTests.log";TestMath();TestHooks();TestNewHooks();TestControls();TestResolutionOsd();
    if(argc==2&&!strcmp(argv[1],"--skip-graphics"))std::puts("D3D9 rendering tests: SKIPPED (headless CI)");else TestD3D();
    std::printf("PASS: %d checks\n",checks);return 0;}
