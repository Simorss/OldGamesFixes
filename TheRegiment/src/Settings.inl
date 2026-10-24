bool Flag(const wchar_t* section,const wchar_t* key,bool fallback){return GetPrivateProfileIntW(section,key,fallback?1:0,iniPath.c_str())!=0;}
void LoadSettings(){
    // Add the optional master switch to existing INIs, including schema 3.
    // Init checks it before this function: disabled configurations stay untouched.
    wchar_t master[16];GetPrivateProfileStringW(L"Main",L"Enabled",L"",master,16,iniPath.c_str());
    if(!master[0]&&!WritePrivateProfileStringW(L"Main",L"Enabled",L"1",iniPath.c_str()))Log("Could not add Main.Enabled to INI; default remains enabled.");
    baseFov=Number(L"FOV",L"BaseFOV",70,30,110);step=Number(L"FOV",L"Step",1,.1,10);
    horPlus=Flag(L"FOV",L"HorPlus",true);persist=Flag(L"FOV",L"SaveFOV",true);
    viewmodelEnabled=Flag(L"Viewmodel",L"Enabled",true);
    viewmodelFov=Number(L"Viewmodel",L"BaseFOV",70,20,140);
    viewmodelStep=Number(L"Viewmodel",L"Step",1,.1,10);
    viewmodelHorPlus=Flag(L"Viewmodel",L"HorPlus",true);viewmodelPersist=Flag(L"Viewmodel",L"SaveFOV",true);
    mouseEnabled=Flag(L"Mouse",L"Enabled",true);
    noAcceleration=Flag(L"Mouse",L"DisableAcceleration",true);noSmoothing=Flag(L"Mouse",L"DisableSmoothing",true);
    osdMs=std::clamp(static_cast<int>(GetPrivateProfileIntW(L"OSD",L"DurationMs",2200,iniPath.c_str())),0,10000);
    const wchar_t* names[4]={L"CameraDecrease",L"CameraIncrease",L"ViewmodelDecrease",L"ViewmodelIncrease"};
    const int defaults[4]={VK_F7,VK_F8,VK_F11,VK_F12};
    int keys[4];for(int i=0;i<4;++i)keys[i]=Key(names[i],defaults[i]);
    bool duplicate=false;for(int i=0;i<4;++i)for(int j=0;j<i;++j)duplicate=duplicate||keys[i]==keys[j];
    if(duplicate){std::copy(std::begin(defaults),std::end(defaults),keys);Log("Duplicate hotkeys; restored F7/F8 and F11/F12.");}
    for(int i=0;i<2;++i){hotkeys[i].decrease=keys[i*2];hotkeys[i].increase=keys[i*2+1];}
    if(GetPrivateProfileIntW(L"Main",L"ConfigVersion",0,iniPath.c_str())<3){
        bool complete=true;
        for(int i=0;i<4;++i){wchar_t value[20];std::swprintf(value,20,L"0x%02X",keys[i]);complete=WritePrivateProfileStringW(L"Keys",names[i],value,iniPath.c_str())&&complete;}
        for(const wchar_t* key:{L"Decrease",L"Increase",L"SensitivityDecrease",L"SensitivityIncrease"})complete=WritePrivateProfileStringW(L"Keys",key,nullptr,iniPath.c_str())&&complete;
        for(const wchar_t* key:{L"Sensitivity",L"StepPercent",L"SaveSensitivity"})complete=WritePrivateProfileStringW(L"Mouse",key,nullptr,iniPath.c_str())&&complete;
        struct Item{const wchar_t* section;const wchar_t* key;double value;};
        Item items[]={{L"Viewmodel",L"Enabled",viewmodelEnabled?1.:0.},{L"Viewmodel",L"BaseFOV",viewmodelFov},{L"Viewmodel",L"HorPlus",viewmodelHorPlus?1.:0.},{L"Viewmodel",L"Step",viewmodelStep},{L"Viewmodel",L"SaveFOV",viewmodelPersist?1.:0.},
            {L"Mouse",L"Enabled",mouseEnabled?1.:0.},{L"Mouse",L"DisableAcceleration",noAcceleration?1.:0.},{L"Mouse",L"DisableSmoothing",noSmoothing?1.:0.}};
        for(auto& item:items){wchar_t value[40];std::swprintf(value,40,L"%.6f",item.value);complete=WritePrivateProfileStringW(item.section,item.key,value,iniPath.c_str())&&complete;}
        if(complete)complete=WritePrivateProfileStringW(L"Main",L"ConfigVersion",L"3",iniPath.c_str())!=FALSE;
        Log("Configuration migration %s; FOV values preserved, obsolete sensitivity settings removed. Mouse sensitivity belongs to game menu.",complete?"complete":"incomplete (INI may be read-only)");
    }
}
