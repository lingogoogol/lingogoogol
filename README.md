# lingogoogol Storage

**以無上限雲端硬碟容量為核心的 Windows 桌面儲存工具。**

使用 C++23、Vulkan 與 Google Drive API，將檔案分塊儲存於雲端，提供檔案與資料夾的上傳、瀏覽及下載功能。專案本身不設定固定的總容量上限，讓儲存空間隨資料持續擴展。

**歡迎提交 [Pull Request](https://github.com/lingogoogol/lingogoogol/pulls) 與 [Issue](https://github.com/lingogoogol/lingogoogol/issues)！** 功能建議、錯誤回報、文件改善與程式碼貢獻都很歡迎。

## 分支說明

| 分支 | 說明 |
| --- | --- |
| `main` | 由作者親自撰寫的版本。 |
| `storage_ai` | 由 Antigravity 與 Codex 協作開發的版本。 |

## 無上限容量的設計

檔案內容會經過 Base64URL 編碼，分散儲存在多個 Google Drive 物件的 `description` 欄位，再透過索引串接與還原。資料夾結構與檔案名稱也由程式管理，因此可以持續新增分塊，擴展儲存規模。

這裡的「無上限」指程式沒有預設的總容量配額；實際可用規模仍受 Google Drive 的物件數量限制、API 配額、請求頻率、帳戶權限與服務政策影響。

這些資料需要透過本程式還原；在 Google Drive 網頁上看到的物件不會是原始檔案的直接副本。請保留其名稱、描述與索引關係。

## 功能

- 上傳檔案，支援一次選取多個檔案。
- 遞迴上傳資料夾，保留目錄結構。
- 瀏覽雲端目錄，下載檔案或整個資料夾。
- 顯示傳輸進度、預備 blob 數量與 FPS。
- 支援視窗縮放、最大化、最小化及還原；拖曳縮放期間也會重繪與更新 FPS。最小視窗內容尺寸為 800 × 480。

## 使用說明

### 1. 準備執行環境

需要 Windows x64、支援 Vulkan 1.4 的圖形環境，以及可連線至 Google API 的網路。

準備編譯後的 `storage.exe` 與完整執行期資源，放在同一個目錄：

```text
runtime/
├── storage.exe
├── libc++.dll
├── libunwind.dll
├── glfw3.dll
├── libcurl.dll
├── libcrypto-3-x64.dll
├── libssl-3-x64.dll
├── key.json
├── font/
│   └── NotoSansTC-VariableFont_wght.ttf
└── shader/
    ├── solid.spv
    └── texture.spv
```

字型使用 [Noto Sans TC](https://fonts.google.com/noto/specimen/Noto+Sans+TC)。目前 CMake 安裝流程會複製主程式、上述 DLL 與 shader；字型及帳戶設定需要自行準備。

### 2. 設定 Google Drive 存取

1. 在 Google Cloud 專案中啟用 Google Drive API。
2. 準備具有所需 Drive 操作權限的服務帳戶，下載其 JSON 金鑰。
3. 將金鑰命名為 `key.json`，放在執行目錄中。程式會讀取其中的 `client_email` 與 `private_key`。

目前採用服務帳戶驗證。請確認帳戶能建立、讀取與更新所需的 Drive 物件；啟用 API 後若仍遇到權限或配額錯誤，需依 Google 的錯誤回應檢查帳戶設定。

`key.json` 含有私鑰，請妥善保管，勿提交到 Git、Issue 或 Pull Request。

### 3. 啟動程式

在 PowerShell 中先切換到執行目錄，再啟動程式：

```powershell
Set-Location "C:\path\to\runtime"
.\storage.exe
```

程式依目前工作目錄尋找字型、shader、金鑰與狀態檔，請從上述目錄啟動。

首次啟動時，若沒有 `root` 檔案，程式會建立雲端邏輯根目錄並儲存其 ID。背景工作也會預先建立大量空白 blob，累積可供上傳使用的物件。初次準備會產生較多 API 請求，請等待 `blob_queue_count` 累積。

### 4. 上傳、瀏覽與下載

| 操作 | 使用方式 |
| --- | --- |
| 上傳檔案 | 點選「上傳檔案」，選取一個或多個檔案，確認後加入傳輸工作。 |
| 上傳資料夾 | 點選「上傳資料夾」，選取資料夾，程式會遞迴處理其中的檔案與子目錄。 |
| 瀏覽資料夾 | 點選清單中的資料夾名稱，載入該目錄的內容。 |
| 下載 | 點選項目下方的「下載」，再選擇本機目的地資料夾。內容會以原名稱存放在該資料夾下。 |
| 取消選取 | 在檔案或資料夾對話框按「取消」，此次操作不會新增傳輸工作。 |
| 查看傳輸狀態 | 從左側進度區查看工作進度；按住 F3 可顯示額外的連線狀態。 |

**目前下載流程會先移除目的地內的同名檔案或資料夾，再重新建立內容。請選擇空的目的地資料夾，以免覆蓋既有資料。**

目前清單在開啟目錄時載入。上傳完成後若尚未顯示新項目，可重新啟動程式查看；重新啟動會回到 `root` 指定的根目錄。

### 5. 關閉與保留狀態

傳輸完成後，使用視窗右上角的叉叉正常關閉程式。請保留以下檔案：

| 檔案 | 用途 |
| --- | --- |
| `key.json` | 存取雲端資料使用的服務帳戶金鑰。 |
| `root` | 雲端邏輯根目錄的 ID。刪除後，下次啟動會建立另一個根目錄。 |
| `blob_queue` | 尚未使用的雲端 blob ID 佇列，由程式維護。 |

`blob_queue` 啟動時會被讀取並刪除，正常關閉時才寫回。備份或搬移執行環境前，請先正常關閉程式，再一併保留金鑰與狀態檔。

## 目前限制

- 工具列的「新增資料夾」與「清除」仍未完成實作。
- 檔案內容使用 Base64URL 編碼，目前沒有實作檔案內容的端對端加密。
- 請等待傳輸完成後再關閉；目前沒有完整的傳輸中斷續傳機制。

## 從原始碼建置

目前驗證的流程是在 Linux 上使用 llvm-mingw 跨平台編譯 Windows PE 執行檔。

### 相依工具

| 工具／相依 | 需求或目前驗證版本 |
| --- | --- |
| C++ 編譯器 | 支援 C++23、C++ modules 與 `import std;`；目前使用 llvm-mingw 20261020 UCRT／Clang 23.1.4。 |
| CMake | 4.0 以上；目前驗證版本為 4.2.3。 |
| 建置工具 | Ninja。 |
| Shader compiler | Slang 2026.20，需能執行 `slangc`。 |
| 圖形相依 | Windows 版 Vulkan loader、Vulkan-Headers／Vulkan-Hpp modules、GLFW、FreeType。 |
| 其他相依 | Windows 版 curl、OpenSSL，以及 GLM、nlohmann/json 標頭。 |

相依函式庫須對應 Windows x64 目標。此儲存庫目前仍依賴相鄰的 `../CMake`、`../include` 與 `../lib` 配置；工具鏈檔案與部分搜尋路徑也使用開發環境設定。新環境需先補齊這些相依及路徑。

### Configure、Build 與 Install

以下假設原始碼位於 `programming/source`，CMake 輔助檔案位於 `programming/CMake`：

```bash
export PATH="/opt/toolchains/llvm-mingw-20261020-ucrt-ubuntu-22.04-x86_64/bin:$PATH"
project_workspace=/path/to/programming

cmake -S "$project_workspace/source" \
      -B "$project_workspace/build/x64-win32-pe-mingw_Release" \
      -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE="$project_workspace/CMake/llvm-mingw-w64.cmake" \
      -DCMAKE_INSTALL_PREFIX="$project_workspace/install/x64-win32-pe-mingw_Release"

cmake --build "$project_workspace/build/x64-win32-pe-mingw_Release" --parallel 4
cmake --install "$project_workspace/build/x64-win32-pe-mingw_Release"
```

完成後，依「使用說明」補上字型與帳戶設定。建置也會產生 `compare.exe` 與 `filegen.exe` 輔助工具；`compare.exe` 可逐位元組比對下載檔案與原始檔案。

## 歡迎貢獻

歡迎透過 [Issue](https://github.com/lingogoogol/lingogoogol/issues) 回報問題、討論設計或提出功能需求，也歡迎直接提交 [Pull Request](https://github.com/lingogoogol/lingogoogol/pulls) 改善程式與文件。

### 提交 Issue

請盡量提供：

- 預期行為、實際結果，以及能重現問題的操作步驟。
- 作業系統、顯示卡、驅動版本與使用的程式版本或 commit。
- 經過遮蔽敏感資訊的錯誤訊息或 `logfile/` 紀錄；請移除私鑰、存取權杖與私人檔案資訊。

### 提交 Pull Request

1. 以 `storage_ai` 為基礎開發，PR 也請以 `storage_ai` 為目標分支。
2. 每個 PR 聚焦單一問題，說明修改目的與使用者可觀察到的行為變化。
3. 修改程式碼後，確認能成功編譯，並實際驗證相關操作；GUI 修改請包含視窗操作與正常關閉的測試結果。
4. 維持清楚易讀的 C++ 程式碼。遇到特殊語法、跨平台限制或非直覺的實作，請註解說明原因。
5. 功能、操作方式、設定或使用限制有變更時，請同步更新 README，讓文件與程式的實際行為一致。
6. Commit 訊息請使用清楚完整的英文句子，並以句號結尾。

小修正、文件補充與首次貢獻都很歡迎；較大的功能或架構調整，建議先開 Issue 討論。
