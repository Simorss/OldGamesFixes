struct FakeName {const wchar_t* text;BYTE padding[8];};
FakeName fakeNames[]={{L"None",{}},{L"SASWeapon",{}},{L"TestRifle",{}},{L"PlayerInput",{}},{L"Other",{}}};
void* __cdecl Entry(int index){return index>=0&&index<5?&fakeNames[index]:nullptr;}
const wchar_t* __thiscall Name(void* entry){return static_cast<FakeName*>(entry)->text;}
alignas(4) BYTE rootWeapon[0x2c]{},rifleClass[0x2c]{},otherClass[0x2c]{},inputClass[0x2c]{};
void Class(BYTE* cls,int id,void* parent){*reinterpret_cast<int*>(cls+0x20)=id;*reinterpret_cast<void**>(cls+0x28)=parent;}
void Object(BYTE* object,void* cls){*reinterpret_cast<void**>(object+0x24)=cls;}
void PrepareNames(){
    nameEntry=&Entry;nameText=&Name;
    Class(rootWeapon,1,nullptr);Class(rifleClass,2,rootWeapon);Class(otherClass,4,nullptr);Class(inputClass,3,nullptr);
}
float observedFov=0,observedSensitivity=0,observedThreshold=0,observedStrength=0,observedDelta=0;
int forbiddenRendererCalls=0;
int __fastcall RendererSlot(void*,void*){++forbiddenRendererCalls;return 1;}
BYTE observedMode=0;int inputCalls=0;
float simulateMenuSensitivity=-1;
void __cdecl NativeInput(void* object,void* frame,void* result){
    BYTE* input=static_cast<BYTE*>(object);++inputCalls;
    observedMode=input[0x2c];observedStrength=*reinterpret_cast<float*>(input+0x30);
    observedSensitivity=*reinterpret_cast<float*>(input+0x34);observedThreshold=*reinterpret_cast<float*>(input+0x3c);
    if(simulateMenuSensitivity>=0)*reinterpret_cast<float*>(input+0x34)=simulateMenuSensitivity;
    observedDelta=**reinterpret_cast<float**>(static_cast<BYTE*>(frame)+0x10);
    if(result)*static_cast<int*>(result)=123;
}
void TestNewHooks(){
    PrepareNames();
    BYTE actor[0x28]{};Object(actor,rifleClass);
    Check(HasNamedClass(actor,L"SASWeapon"),"weapon subclass filter");
    Check(!HasNamedClass(actor,L"PlayerInput"),"reject unrelated class");
    Class(otherClass,4,otherClass);Object(actor,otherClass);Check(!HasNamedClass(actor,L"SASWeapon"),"class-cycle guard");Class(otherClass,4,nullptr);Object(actor,rifleClass);
    void* rendererTable[11]{};rendererTable[9]=reinterpret_cast<void*>(&RendererSlot);rendererTable[10]=reinterpret_cast<void*>(&RendererSlot);void** renderer=rendererTable;
    BYTE viewport[0x14c]{},canvas[0x1e0]{};
    *reinterpret_cast<int*>(viewport+0x88)=3440;*reinterpret_cast<int*>(viewport+0x8c)=1440;
    *reinterpret_cast<void**>(viewport+0x148)=&renderer;*reinterpret_cast<void**>(canvas+0x1dc)=viewport;
    viewmodelFov=85;viewmodelHorPlus=true;viewmodelEnabled=true;
    double expected=regiment::CorrectViewmodelFov(70,3440.0/1440,85,true);
    float actualViewmodel=ViewmodelFov(canvas,actor,0,1,70);
    Check(forbiddenRendererCalls==0,"regression: renderer interface methods are never called as dimensions");
    Near(actualViewmodel,expected,"viewmodel projection corrected");
    *reinterpret_cast<void**>(viewport+0x148)=reinterpret_cast<void*>(1);
    Near(ViewmodelFov(canvas,actor,0,1,70),expected,"regression: viewmodel does not dereference renderer interface");
    *reinterpret_cast<int*>(viewport+0x88)=0;
    Near(ViewmodelFov(canvas,actor,0,1,70),70,"invalid viewport dimensions leave viewmodel untouched");
    *reinterpret_cast<int*>(viewport+0x88)=3440;
    *reinterpret_cast<int*>(viewport+0x8c)=0;
    Near(ViewmodelFov(canvas,actor,0,1,70),70,"zero viewport height leaves viewmodel untouched");
    *reinterpret_cast<int*>(viewport+0x8c)=1440;
    baseFov=100;Near(ViewmodelFov(canvas,actor,0,1,70),expected,"viewmodel independent of world FOV");baseFov=70;
    Near(ViewmodelFov(canvas,actor,0,1,0),0,"scope hidden-weapon FOV remains zero");
    Near(ViewmodelFov(canvas,actor,1,1,70),70,"wireframe preview unchanged");
    Near(ViewmodelFov(canvas,actor,0,0,70),70,"non-overlay drawing unchanged");
    Object(actor,otherClass);Near(ViewmodelFov(canvas,actor,0,1,70),70,"non-weapon UI drawing unchanged");Object(actor,rifleClass);
    viewmodelEnabled=false;Near(ViewmodelFov(canvas,actor,0,1,70),70,"viewmodel feature can be disabled");viewmodelEnabled=true;
    BYTE* drawCode=static_cast<BYTE*>(VirtualAlloc(nullptr,64,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    BYTE drawFixture[]={0x55,0x8b,0xec,0x6a,0xff,0x8b,0x45,0x14,0xa3,0,0,0,0,0x83,0xc4,0x04,0x5d,0xc2,0x10,0};
    *reinterpret_cast<uintptr_t*>(drawFixture+9)=reinterpret_cast<uintptr_t>(&observedFov);memcpy(drawCode,drawFixture,sizeof(drawFixture));FlushInstructionCache(GetCurrentProcess(),drawCode,64);
    Check(viewmodelHook.Prepare(drawCode,drawFixture,5),"five-byte DrawActor trampoline");
    originalDrawActor=reinterpret_cast<DrawActorFn>(viewmodelHook.code);Check(viewmodelHook.Install(reinterpret_cast<void*>(&DrawActor)),"DrawActor detour installed");
    auto draw=reinterpret_cast<DrawActorFn>(drawCode);draw(canvas,actor,0,1,70);Near(observedFov,expected,"real DrawActor ABI forwards corrected FOV");
    draw(canvas,actor,0,1,0);Near(observedFov,0,"real DrawActor ABI preserves hidden effects");viewmodelHook.Restore();draw(canvas,actor,0,1,70);Near(observedFov,70,"DrawActor rollback");
    VirtualFree(drawCode,0,MEM_RELEASE);VirtualFree(viewmodelHook.code,0,MEM_RELEASE);
    BYTE input[0x80]{};Object(input,inputClass);input[0x2c]=1;*reinterpret_cast<float*>(input+0x30)=.3f;*reinterpret_cast<float*>(input+0x34)=3;*reinterpret_cast<float*>(input+0x3c)=1;
    BYTE before[0x80];memcpy(before,input,sizeof(input));mouseEnabled=true;noAcceleration=true;noSmoothing=true;
    BYTE* inputCode=static_cast<BYTE*>(VirtualAlloc(nullptr,64,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));int nameId=3;playerInputName=&nameId;
    BYTE node[0x24]{},frame[0x14]{};*reinterpret_cast<int*>(node+0x20)=3;*reinterpret_cast<void**>(frame+4)=node;
    float dt=.016f;*reinterpret_cast<float**>(frame+0x10)=&dt;int result=0;
    BYTE inputFixture[]={0x55,0x8b,0xec,0x6a,0xff,0xff,0x75,0x0c,0xff,0x75,0x08,0x51,0xb8,0,0,0,0,0xff,0xd0,0x83,0xc4,0x0c,0x83,0xc4,0x04,0x5d,0xc2,0x08,0};
    *reinterpret_cast<uintptr_t*>(inputFixture+13)=reinterpret_cast<uintptr_t>(&NativeInput);
    memcpy(inputCode,inputFixture,sizeof(inputFixture));FlushInstructionCache(GetCurrentProcess(),inputCode,64);
    Check(inputHook.Prepare(inputCode,inputFixture,5),"five-byte interpreter trampoline");originalProcessInternal=reinterpret_cast<ProcessInternalFn>(inputHook.code);
    Check(inputHook.Install(reinterpret_cast<void*>(&ProcessInternal)),"script interpreter detour installed");auto process=reinterpret_cast<ProcessInternalFn>(inputCode);
    process(input,frame,&result);Check(observedMode==0,"smoothing mode bypassed during script input");Near(observedStrength,0,"smoothing strength removed");Near(observedThreshold,0,"nonlinear mouse threshold removed");Near(observedSensitivity,3,"native menu sensitivity is used during input");Near(observedDelta,.016,"script frame preserved");Check(result==123,"script result pointer preserved");Check(!memcmp(before,input,sizeof(input)),"native properties restored after input");
    *reinterpret_cast<float*>(input+0x34)=.25f;process(input,frame,&result);Near(observedSensitivity,.25,"menu slider change affects next input");Near(*reinterpret_cast<float*>(input+0x34),.25,"menu value remains after input");
    simulateMenuSensitivity=.75f;process(input,frame,&result);simulateMenuSensitivity=-1;
    Near(*reinterpret_cast<float*>(input+0x34),.75,"regression: menu sensitivity changed inside event is never restored away");
    process(input,frame,&result);Near(observedSensitivity,.75,"in-event menu change persists into next frame");Check(observedMode==0&&observedThreshold==0,"filters stay disabled while menu sensitivity changes");
    *reinterpret_cast<int*>(node+0x20)=4;process(input,frame,&result);Check(observedMode==1&&observedThreshold==1,"other script functions are not overridden");*reinterpret_cast<int*>(node+0x20)=3;
    noAcceleration=false;noSmoothing=false;process(input,frame,&result);Check(observedMode==1,"smoothing opt-out");Near(observedThreshold,1,"acceleration opt-out");Near(observedSensitivity,.75,"filter opt-out keeps menu sensitivity");
    mouseEnabled=false;process(input,frame,&result);Check(observedMode==1&&observedThreshold==1,"mouse filters module disabled");mouseEnabled=true;
    Object(input,otherClass);process(input,frame,&result);Check(observedMode==1,"input class filter");Object(input,inputClass);
    inputHook.Restore();process(input,frame,&result);Check(observedMode==1,"interpreter rollback");
    Check(inputCalls==9,"one original script dispatch per call");
    VirtualFree(inputCode,0,MEM_RELEASE);VirtualFree(inputHook.code,0,MEM_RELEASE);
    viewmodelFov=70;noAcceleration=true;noSmoothing=true;
}
void TestControls(){
    static_assert(sizeof(hotkeys)/sizeof(hotkeys[0])==2,"only two FOV key pairs remain");
    regiment::RepeatKey key;
    Check(key.Poll(true,100),"first hotkey press");Check(!key.Poll(true,200),"initial repeat delay");Check(key.Poll(true,450),"held hotkey repeats");Check(!key.Poll(false,451),"release resets hotkey");Check(key.Poll(true,452),"new tap does not wait for repeat timer");
    wchar_t iniFile[MAX_PATH];GetFullPathNameW(L"RuntimeTests.ini",MAX_PATH,iniFile,nullptr);iniPath=iniFile;DeleteFileW(iniPath.c_str());
    WritePrivateProfileStringW(L"FOV",L"BaseFOV",L"78",iniPath.c_str());WritePrivateProfileStringW(L"Keys",L"Decrease",L"Comma",iniPath.c_str());WritePrivateProfileStringW(L"Keys",L"Increase",L"Period",iniPath.c_str());
    LoadSettings();Near(baseFov,78,"migration preserves user's camera FOV");
    Check(GetPrivateProfileIntW(L"Main",L"Enabled",0,iniPath.c_str())==1,"legacy INI gains enabled master switch");
    Check(hotkeys[0].decrease==VK_F7&&hotkeys[0].increase==VK_F8,"world F7/F8 defaults");Check(hotkeys[1].decrease==VK_F11&&hotkeys[1].increase==VK_F12,"viewmodel F11/F12 defaults");Check(GetPrivateProfileIntW(L"Main",L"ConfigVersion",0,iniPath.c_str())==3,"legacy INI migrated to filters-only schema");
    wchar_t value[80];GetPrivateProfileStringW(L"Keys",L"Decrease",L"missing",value,80,iniPath.c_str());Check(!wcscmp(value,L"missing"),"ambiguous legacy key removed");
    WritePrivateProfileStringW(L"Main",L"ConfigVersion",L"2",iniPath.c_str());
    WritePrivateProfileStringW(L"Viewmodel",L"BaseFOV",L"85",iniPath.c_str());
    for(const wchar_t* name:{L"Sensitivity",L"StepPercent",L"SaveSensitivity"})WritePrivateProfileStringW(L"Mouse",name,L"0.005",iniPath.c_str());
    WritePrivateProfileStringW(L"Keys",L"SensitivityDecrease",L"Comma",iniPath.c_str());WritePrivateProfileStringW(L"Keys",L"SensitivityIncrease",L"Period",iniPath.c_str());
    LoadSettings();Near(baseFov,78,"schema 2 cleanup preserves camera FOV");Near(viewmodelFov,85,"schema 2 cleanup preserves viewmodel FOV");
    for(const wchar_t* name:{L"Sensitivity",L"StepPercent",L"SaveSensitivity"}){GetPrivateProfileStringW(L"Mouse",name,L"missing",value,80,iniPath.c_str());Check(!wcscmp(value,L"missing"),"obsolete mouse setting removed");}
    for(const wchar_t* name:{L"SensitivityDecrease",L"SensitivityIncrease"}){GetPrivateProfileStringW(L"Keys",name,L"missing",value,80,iniPath.c_str());Check(!wcscmp(value,L"missing"),"obsolete sensitivity hotkey removed");}
    Check(mouseEnabled&&noAcceleration&&noSmoothing,"removing sensitivity preserves disabled acceleration and smoothing");
    WritePrivateProfileStringW(L"Main",L"Enabled",nullptr,iniPath.c_str());LoadSettings();
    Check(GetPrivateProfileIntW(L"Main",L"Enabled",0,iniPath.c_str())==1,"schema 3 INI gains master switch without resetting FOV");Near(baseFov,78,"master switch preserves existing FOV");
    for(const auto& pair:hotkeys)Check(pair.decrease!=VK_OEM_COMMA&&pair.increase!=VK_OEM_PERIOD,"comma and period are no longer polled by default");
    WritePrivateProfileStringW(L"Keys",L"CameraDecrease",L"F1",iniPath.c_str());WritePrivateProfileStringW(L"Keys",L"CameraIncrease",L"0x71",iniPath.c_str());LoadSettings();Check(hotkeys[0].decrease==VK_F1&&hotkeys[0].increase==VK_F2,"named and hexadecimal FOV keys configurable");
    double oldWorld=baseFov,oldVm=viewmodelFov;ChangeSetting(Setting::Viewmodel,1,GetTickCount());Near(baseFov,oldWorld,"viewmodel key leaves camera unchanged");Save();Near(Number(L"Viewmodel",L"BaseFOV",70,20,140),oldVm+1,"viewmodel persists independently");
    GetPrivateProfileStringW(L"Mouse",L"Sensitivity",L"missing",value,80,iniPath.c_str());Check(!wcscmp(value,L"missing"),"saving FOV never recreates sensitivity setting");
    WritePrivateProfileStringW(L"Keys",L"ViewmodelDecrease",L"F1",iniPath.c_str());LoadSettings();Check(hotkeys[0].decrease==VK_F7&&hotkeys[1].decrease==VK_F11,"duplicate FOV keys reset defaults");
    osdSetting=Setting::Viewmodel;lastViewmodel=0;viewmodelFov=85;char text[80];OsdText(text,sizeof(text));Check(!strcmp(text,"VIEWMODEL BASE: 85.0"),"hidden viewmodel shows reference value");lastViewmodel=GetTickCount();currentViewmodelFov=115.2f;OsdText(text,sizeof(text));Check(!strcmp(text,"VIEWMODEL FOV: 115.2"),"active viewmodel displays current FOV");
    DeleteFileW(iniPath.c_str());baseFov=70;viewmodelFov=70;dirtySettings=0;osdSetting=Setting::Camera;
}
void TestResolutionOsd(){
    void* table[11]{};table[9]=reinterpret_cast<void*>(&Width);table[10]=reinterpret_cast<void*>(&Height);void** target=table;
    auto sample=[&](int width,int height,float incoming=70.f){targetWidth=width;targetHeight=height;return CameraFov(&target,incoming);};
    outputWidth=0;outputHeight=0;resolutionOsdPending=false;lastCamera=0;osdMs=2200;baseFov=70;horPlus=true;dirtySettings=0;
    ObserveOutputSize(1920,1080);Check(!resolutionOsdPending,"initial resolution is baseline, not resize");
    float oldFov=sample(1920,1080);Near(oldFov,regiment::CorrectFov(70,16./9,70,true),"initial display camera FOV");
    osdUntil=17;osdSetting=Setting::Viewmodel;
    ObserveOutputSize(1920,1080);Check(!resolutionOsdPending&&osdUntil==17&&osdSetting==Setting::Viewmodel,"unchanged resolution preserves timer and manual viewmodel OSD");
    ObserveOutputSize(0,0);ObserveOutputSize(32769,1080);Check(outputWidth==1920&&outputHeight==1080&&!resolutionOsdPending,"invalid transient dimensions ignored");
    ObserveOutputSize(3440,1440);Check(resolutionOsdPending&&osdUntil==17,"resolution event waits for new camera instead of showing stale FOV");
    sample(1920,1080);Check(resolutionOsdPending&&osdUntil==17,"old-resolution camera cannot consume resize event");Near(currentFov.load(),oldFov,"stale camera does not replace displayed FOV");
    sample(512,512);Check(resolutionOsdPending&&osdUntil==17,"off-screen pass cannot consume resize event");Near(currentFov.load(),oldFov,"off-screen pass does not overwrite OSD FOV");
    float updated=sample(3440,1440);Near(updated,102.8829,"new ultrawide FOV calculated");
    Check(!resolutionOsdPending&&osdSetting==Setting::Camera&&osdUntil!=17,"new camera triggers camera OSD once");
    DWORD shownUntil=osdUntil.load();Check(static_cast<LONG>(shownUntil-GetTickCount())>0&&shownUntil-GetTickCount()<=2200,"resize OSD uses configured duration");
    char text[80];OsdText(text,sizeof(text));Check(!strcmp(text,"FOV: 102.9"),"resolution OSD reports actual corrected FOV");
    for(int i=0;i<4;++i){ObserveOutputSize(3440,1440);sample(3440,1440);}
    Check(osdUntil==shownUntil&&!resolutionOsdPending,"steady frames do not keep extending OSD");
    sample(512,512);Near(currentFov.load(),updated,"later secondary render targets leave main FOV displayed");
    osdUntil=17;ObserveOutputSize(1720,720);sample(1720,720);
    Check(osdUntil!=17&&!resolutionOsdPending,"same-aspect resolution change also shows OSD");Near(currentFov.load(),updated,"same aspect honestly reports unchanged FOV");
    osdUntil=17;ObserveOutputSize(1280,720);ObserveOutputSize(1024,768);sample(1280,720);
    Check(resolutionOsdPending&&osdUntil==17,"rapid changes wait for latest output dimensions");
    sample(1024,768);Near(currentFov.load(),70,"return to 4:3 restores matching horizontal FOV");Check(osdUntil!=17&&!resolutionOsdPending,"latest resolution confirms pending notification");
    horPlus=false;baseFov=80;osdUntil=17;ObserveOutputSize(1920,1080);sample(1920,1080);
    Near(currentFov.load(),80,"HorPlus disabled reports actual manual FOV");Check(osdUntil!=17&&!resolutionOsdPending,"resize notification also works with HorPlus disabled");
    osdMs=0;ObserveOutputSize(3440,1440);sample(3440,1440);
    Check(!resolutionOsdPending&&static_cast<LONG>(GetTickCount()-osdUntil.load())>=0,"DurationMs zero keeps resize notification hidden");
    Near(baseFov,80,"resize never changes configured reference FOV");Check(dirtySettings==0,"resize never requests INI persistence");
    outputWidth=0;outputHeight=0;resolutionOsdPending=false;lastCamera=0;osdUntil=0;osdMs=2200;baseFov=70;horPlus=true;targetWidth=3440;targetHeight=1440;osdSetting=Setting::Camera;
}
