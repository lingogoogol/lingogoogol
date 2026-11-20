module;

#include "android/native/bridge.hpp"

module storage.kernel.GUI;

import std;

// Windows: .cpp (IFileDialog). The Storage Access Framework returns content://
// URIs instead of paths.
auto dialog_file(bool folder) -> std::vector<std::string>
{
    return android_pick_documents(folder);
}
