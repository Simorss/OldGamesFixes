#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>
int main(){IDirectInput8W* input=nullptr;HRESULT hr=DirectInput8Create(GetModuleHandleW(nullptr),DIRECTINPUT_VERSION,IID_IDirectInput8W,reinterpret_cast<void**>(&input),nullptr);if(input)input->Release();return FAILED(hr)?1:0;}
