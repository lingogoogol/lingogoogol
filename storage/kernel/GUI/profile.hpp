// Included in this module's global fragment: WIC is a Windows COM API and must
// not become a second module-owned declaration of the Windows interfaces.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincodec.h>
#undef min
#undef max

template<class T> struct storage_com_release_t
{
    void operator()(T* value) const { if (value) value->Release(); }
};
