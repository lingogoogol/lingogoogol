module storage.dialog;

import std;

import external.Windows;
import external.objbase;
import external.ShObjIdl;

import lgo.container.str;

dialog_t::dialog_t(bool folder)
{
	IFileDialog* dialog{};
	CoCreateInstance
	(
		CLSID_FileOpenDialog, 
		nullptr, 
		CLSCTX_INPROC_SERVER, 
		IID_PPV_ARGS0(&dialog), 
		IID_PPV_ARGS1(&dialog)
	);
	DWORD dialog_event_id{};
	dialog->Advise(this, &dialog_event_id);
	FILEOPENDIALOGOPTIONS dialog_option{};
	dialog->GetOptions(&dialog_option);
	dialog->SetOptions(dialog_option | FOS_FORCEFILESYSTEM | FOS_ALLOWMULTISELECT | (folder ? FOS_PICKFOLDERS : 0));
	dialog->Show(nullptr);
    return;
}

auto dialog_t::ok() -> bool
{
    return m_ok;
}

auto dialog_t::result() -> std::vector<std::string>
{
    return m_result;
}

auto __stdcall dialog_t::OnFileOk(IFileDialog* in) -> HRESULT
{
    IFileOpenDialog* dialog{};
    in->QueryInterface(IID_PPV_ARGS0(&dialog), IID_PPV_ARGS1(&dialog));
    IShellItemArray* result_array{};
    dialog->GetResults(&result_array);
    DWORD result_count{};
    result_array->GetCount(&result_count);
    m_result.resize(result_count);
    for (DWORD i{ 0 }; i < result_count; ++i)
    {
        IShellItem* result{};
        result_array->GetItemAt(i, &result);
        LPWSTR result_strc{};
        result->GetDisplayName(SIGDN_FILESYSPATH, &result_strc);
        m_result[i] = lgo::to_string(std::wstring{ result_strc });
    }
    m_ok = true;
    return S_OK;
}

auto __stdcall dialog_t::OnFolderChange(IFileDialog*) -> HRESULT
{
    return S_OK;
}

auto __stdcall dialog_t::OnFolderChanging(IFileDialog*, IShellItem*) -> HRESULT
{
    return S_OK;
}

auto __stdcall dialog_t::OnOverwrite(IFileDialog*, IShellItem*, FDE_OVERWRITE_RESPONSE*) -> HRESULT
{
    return S_OK;
}

auto __stdcall dialog_t::OnSelectionChange(IFileDialog*) -> HRESULT
{
    return S_OK;
}

auto __stdcall dialog_t::OnShareViolation(IFileDialog*, IShellItem*, FDE_SHAREVIOLATION_RESPONSE*) -> HRESULT
{
    return S_OK;
}

auto __stdcall dialog_t::OnTypeChange(IFileDialog*) -> HRESULT
{
    return S_OK;
}

auto __stdcall dialog_t::AddRef() -> ULONG
{
    return 0;
}
auto __stdcall dialog_t::Release() -> ULONG
{
    return 0;
}

auto __stdcall dialog_t::QueryInterface(REFIID, void**) -> HRESULT
{
    throw;
}
