#pragma once

#include <string>

#define ThrowIfFailed(x)  { HRESULT hr_ = (x); if (FAILED(hr_)) { std::string text = "HRESULT failed in file: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__) + "\n"; throw std::runtime_error(text); }  }  

#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p) = nullptr; } } 
#define SAFE_DELETE(p) { if(p) { delete (p); (p) = nullptr; } }

#define SIZE_OF_ARRAY(A) (sizeof(A)/sizeof((A)[0]))