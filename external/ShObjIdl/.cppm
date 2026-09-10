module;

#define WIN32_LEAN_AND_MEAN
#include <ShObjIdl.h>

export module external.ShObjIdl;

export
{
    using ::FILEOPENDIALOGOPTIONS;
    using ::IFileDialogEvents;
    using ::IShellItem;
    using ::IFileDialog;
    using ::IFileOpenDialog;
    using ::IShellItemArray;
    using ::FDE_OVERWRITE_RESPONSE;
    using ::FDE_SHAREVIOLATION_RESPONSE;
    
    using ::tagCLSCTX;
    using enum ::tagCLSCTX;
    using ::SIGDN;
    using enum ::SIGDN;
    using ::_FILEOPENDIALOGOPTIONS;
    using enum ::_FILEOPENDIALOGOPTIONS;

    using ::CLSID_FileOpenDialog;
}
