#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
int main() {
    void* input=nullptr;
    HRESULT status=DirectInput8Create(GetModuleHandleW(nullptr),DIRECTINPUT_VERSION,IID_IDirectInput8A,&input,nullptr);
    if(SUCCEEDED(status) && input)static_cast<IDirectInput8A*>(input)->Release();
    return 0;
}
