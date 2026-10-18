export module storage.dialog;

import std;

import external.Windows;
import external.ShObjIdl;

export
{
    class dialog_t: public IFileDialogEvents
    {
    private:
        bool m_ok{ false };
        std::vector<std::string> m_result{};
    public:
        dialog_t(bool folder);

        auto ok() -> bool;
        auto result() -> std::vector<std::string>;

        auto __stdcall OnFileOk(IFileDialog* in) -> HRESULT override;
        auto __stdcall OnFolderChange(IFileDialog*) -> HRESULT override;
        auto __stdcall OnFolderChanging(IFileDialog*, IShellItem*) -> HRESULT override;
        auto __stdcall OnOverwrite(IFileDialog*, IShellItem*, FDE_OVERWRITE_RESPONSE*) -> HRESULT override;
        auto __stdcall OnSelectionChange(IFileDialog*) -> HRESULT override;
        auto __stdcall OnShareViolation(IFileDialog*, IShellItem*, FDE_SHAREVIOLATION_RESPONSE*) -> HRESULT override;
        auto __stdcall OnTypeChange(IFileDialog*) -> HRESULT override;
        auto __stdcall AddRef() -> ULONG override;
        auto __stdcall Release() -> ULONG override;
        auto __stdcall QueryInterface(REFIID, void**) -> HRESULT override;
    };
}
