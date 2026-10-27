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

需要 Windows x64、支援 Vulkan 1.3 且具備必要 features 與圖形／呈現佇列的圖形環境，以及可連線至 Google API 的網路。

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

下載會先顯示「準備下載」，在背景列出資料夾內容並計算總大小；準備成功後才排程傳輸，期間仍可操作畫面與瀏覽其他目錄。空資料夾及零位元組檔案也會建立，每個檔案均確認寫入成功且大小符合後才移除工作。下載不需要等待上傳用的 blob 佇列累積；分塊索引或內容回應順序不同時，會等待前面的資料齊備，再依序寫入。批次回應格式不完整時會重試待完成的子請求，保留先前成功的結果。

主迴圈的上下載 multi 傳輸由專用背景執行緒處理，包含 `curl_multi_perform()`、回應解析及重試。單次請求 timeout 維持 **300 秒**，連線 timeout 維持 **100 秒**；Schannel 在斷網或傳送受阻時，即使在一次 perform 內等待 socket，GUI 仍可繼續繪製與處理事件。主執行緒透過佇列提交請求、讀取進度快照及領取結果，工作狀態和畫面由主執行緒更新。背景卡住期間，進度快照可能暫停更新；關閉此傳輸 worker 時會以 socket shutdown 喚醒等待，再停止及回收執行緒。

OAuth access token 的簽署與取得也由背景 worker 處理。建立請求不再等待 token；實際送出前才取得最新授權，等待刷新時其他傳輸仍可運行。同一帳戶的並行請求共用一次刷新，效期使用 OAuth 回覆的 `expires_in`，有新請求且接近到期時會提早刷新；舊 token 尚有效時可繼續使用。

服務帳戶授權使用完整的 JWT bearer grant type：`urn:ietf:params:oauth:grant-type:jwt-bearer`。此值格式錯誤會讓 OAuth 端點回傳 HTTP 400，導致授權與後續目錄載入失敗。

一般請求的 HTTP 401 或批次內層的 401 會淘汰該次使用的 token，等待刷新後再重送，並保留原本的 HTTP method、內容及批次中已成功的子請求。較晚抵達的舊 401 不會清除較新的 token；503 等其他錯誤不會強制刷新。每個請求最多在 401 後刷新重送一次，若新 token 仍被拒絕，會回報錯誤，避免無限重試。OAuth 的網路／5xx 錯誤最多嘗試三次，重試間分別等待一秒與兩秒；4xx 或不合法的 token 回應直接回報錯誤。錯誤依既有工作／程式錯誤處理顯示；帳戶刷新失敗且無有效 token 時不會自動重啟刷新，修正帳戶設定或網路問題後重新啟動程式。失敗日誌不會寫入請求／回應本文，以免洩漏簽署憑證或 token。

瀏覽目錄及上傳完成後刷新清單仍有同步的 Drive API 呼叫，這些操作遇到慢網路時仍可能等待；本機檔案讀寫、編碼及結果套用也仍可能影響單幀時間。正常關閉會停止 token worker，喚醒仍在等待授權的準備工作，並保存未使用的 blob 佇列。

下載準備、資料解析、解碼或本機寫入失敗時，工作會保留在左側並顯示「下載失敗」及原因，同時記錄至 `logfile/`；其他傳輸與畫面操作可繼續。失敗工作不會再排程新請求，已在途的回應收回後不會繼續寫入。已建立的部分檔案會保留，不能視為完整下載。需要重試時，重新點選該項目的「下載」；失敗工作會保留至程式關閉，不會在下次啟動時自動重試。

上傳時會先在既有目錄之外完成檔案內容、分塊索引與資料夾結構；整個 task 的所有上傳請求均獲得 API 成功回應後，才將檔案或上傳資料夾的根目錄連結到原先選定的目錄。多選檔案時，每個檔案各自是一個 task。尚未完成的項目不會出現在既有目錄中；連結成功後，左側的工作進度才會移除。位元組進度已達總大小時，仍可能正在完成索引或連結，請等待工作進度移除。

清單會在開啟目錄時載入。上傳完成並成功連結後，若仍在瀏覽該目的目錄，清單會自動刷新，顯示剛上傳的項目；若已切換至其他目錄，會保留目前畫面，下次開啟目的目錄時載入新內容。

程式入口除 `std::exception` 外也有 `catch (...)`，非標準 C++ 例外會記錄未知例外訊息並回報失敗。

lgo.GUI 文字欄位支援 UTF-8 中文輸入、Backspace、Ctrl+A 全選、Ctrl+C 複製及 Ctrl+V 貼上。

### 5. 關閉與保留狀態

傳輸完成後，使用視窗右上角的叉叉正常關閉程式。請保留以下檔案：

| 檔案 | 用途 |
| --- | --- |
| `key.json` | 存取雲端資料使用的服務帳戶金鑰。 |
| `root` | 雲端邏輯根目錄的 ID。刪除後，下次啟動會建立另一個根目錄。 |
| `blob_queue`、`blob_queue.account`、`blob_queue.<帳戶雜湊>` | 各帳戶尚未使用的 blob ID 與初始佇列的帳戶綁定。 |
| `task`、`task.data/` | 未完成工作索引、意圖、blob ID 分配、下載進度及成功回應快取。 |

佇列在啟動時讀取並保留原檔，分配 ID 或補充後持續保存。工作在網路寫入前先記錄 blob ID，成功回應與下載寫入位置也持續保存；檢查點使用同目錄暫存檔、磁碟 flush 和原子取代。正常關閉、C++ 例外或直接終止程序後，下次啟動會自動恢復未完成的上下載，並使用各工作記錄的帳戶路徑及身分。`task.data/` 會快取已收到的回應以重建排程，下載內容快取可能佔用接近檔案大小的額外空間；工作完成或取消後移除。

恢復下載時保留已保存的檔案前綴，截掉檢查點之後的尾端，重播已完成回應以重建索引；不會再次刪除整個目的地或重複附加前綴。恢復上傳會使用原 blob ID，並檢查來源檔案大小、修改時間及資料夾清單；來源已改變時顯示失敗，需重新新增工作。發布目錄連結時會檢查項目 ID，避免閃退後重複加入。請保留原始來源、部分下載及舊帳戶的金鑰檔；`task` 必須和 `task.data/` 一併搬移或備份。

## 目前限制

- 顯示卡驅動程式重設、移除或 GPU 異常可能造成 Vulkan 裝置遺失（`VK_ERROR_DEVICE_LOST`）。程式會重新選擇可用的 GPU、重建裝置及繪圖資源，保留目前目錄、傳輸工作與進度；重建期間畫面可能暫停更新，傳輸不會因此重新開始。重建失敗時至少間隔一秒再試，同一個 60 秒區間最多嘗試三次，超過後會關閉並寫回未使用的 `blob_queue`。未完成的傳輸會由磁碟檢查點於下次啟動時恢復；復原過程與失敗原因會記錄在 `logfile/`。
- 啟動會檢查 Vulkan loader、必要的 instance／device extensions、圖形與呈現佇列、必要 features、SDR surface 格式及字型貼圖能力；GPU 不符條件時記錄具體原因，裝置初始化失敗時會嘗試其他合格 GPU。Debug 缺少 validation layer 或 debug-utils 時會略過可選診斷功能。使用 Vulkan 1.3，無須 `VK_KHR_get_surface_capabilities2`。
- swapchain 過期或 suboptimal 會重建；surface 遺失會重建 surface 與 GPU 資源，保留傳輸工作，沿用上述重試上限。fence 與影像擷取每次最多等待 50 毫秒，逾時或尚無可用影像時返回事件迴圈。surface 格式變更時重建 pipeline；透明度模式依支援能力選擇，深度格式優先 D32，必要時退回 D16。
- 不支援的資源尺寸、無適合記憶體類型、損壞的 SPIR-V 標頭及 Vulkan API 失敗會回報原因。無法恢復的錯誤會停止傳輸、保存未使用的佇列並顯示錯誤對話框；程式以非零狀態結束。尚未完成的傳輸會保留跨次啟動續傳的檢查點。validation 訊息只記錄，不會從驅動程式的 C callback 拋出例外。
- 工具列的「新增資料夾」與「清除」仍未完成實作。
- 檔案內容使用 Base64URL 編碼，目前沒有實作檔案內容的端對端加密。
- 未完成的傳輸依最後保存的檢查點續傳；取消上傳時，未連結的 blob 可能留在雲端，目前不會自動清理。

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

### Vulkan 物件名稱

Debug 建置會自動定義 `DEBUG_VULKAN_NAME`，透過 `VK_EXT_debug_utils` 為 Vulkan 物件命名，方便在 RenderDoc、Nsight 及驗證訊息中辨認資源。名稱包含用途及 handle，涵蓋裝置、佇列、交換鏈及其影像、指令池與緩衝區、同步物件、pipeline、descriptor、GUI 頂點緩衝區及字型影像與記憶體；縮放視窗或重建 GPU 後也會重新命名。

Release 預設也會定義 `DEBUG_VULKAN_NAME`。它會在建立或重建 Vulkan 物件時組裝名稱並呼叫命名 API，增加字串配置及驅動呼叫成本；名稱不會改變 shader 或繪圖命令的運作。不需要命名時可用 `-DDEBUG_VULKAN_NAME=OFF` 重新設定 CMake 並編譯，便不再組裝命名字串或呼叫命名 API。既有建置目錄會保留 CMake cache，更新預設值後應明確指定 `ON`。

此選項只啟用物件命名，不會額外啟用 validation layers。若 loader 不支援 `VK_EXT_debug_utils`，會跳過命名；命名 API 失敗也不會中止程式。Wine 10 的 Vulkan 橋接層在命名 debug messenger 時會直接 abort，因此 Wine 下略過這一種診斷物件；其他物件及原生 Windows 仍照常命名。

### 矩形 primitive 的執行緒同步

`rect_primitive_t` 的 `set_pos()`、`set_size()`、`set_color()`、`set_image_view()`、`color_enable()`、`texture_enable()`、`inside()` 與 `log()` 可在不同執行緒呼叫，也支援多個執行緒更新同一個 primitive。更新會與繪圖、字型上傳、視窗尺寸更新及 GPU 重建同步；等待 GPU 閒置後，會持續持有引擎鎖直到頂點或 descriptor 更新完成，避免其他執行緒同時使用 `VkQueue`，或在 CPU 寫入頂點時提交繪圖。切換顏色／貼圖模式時會重建對應頂點格式，替換 image view 時會更新 descriptor。

呼叫端仍須保證 primitive、引擎、image view 及裁切座標的生命週期；移除 primitive 或銷毀引擎前，先停止並等待使用它們的執行緒。貼圖模式須先提供有效的 image view；裁切座標透過外部指標引用，外部修改也須自行同步。這項保證限於 primitive，上層 GUI 物件、GLFW 事件處理與視窗操作仍應在 GUI 執行緒進行。

直接透過 `device_get()`、`command_queue_get()` 或 `command_buffer_create()` 操作 Vulkan 資源時，須先取得 `auto guard = engine.gpu_lock();`，並持有至錄製、提交、等待和 command buffer 釋放全部完成。僅鎖住 `flush()` 無法保護後續資源寫入。

可用 `python3 tests/run_gpu_device_loss.py --mode rect-threads` 執行跨執行緒回歸測試；測試會同時更新相同與不同矩形，執行貼圖／顏色切換、字型上傳及視窗縮放，並檢查 Vulkan 呼叫與頂點記憶體寫入沒有交錯，以及更新持續進行時的 GPU 重建。

### GUI 物件名稱與日誌

Debug 建置會自動定義 `DEBUG_GUI_OBJECT_NAME`，Release 預設也會啟用。所有繼承 `GUI_object_t` 的物件（包含容器建立及內嵌子物件）都會取得建構位置與流水號組成的 `m_name`；複製物件會取得新的識別，指定運算則保留目的物件名稱。可用 `name_set()` 指定用途，`name_get()` 讀取名稱，明確呼叫物件的 `log()` 可列出名稱、識別及子物件資訊。

GUI 名稱會增加物件建立時的字串配置、流水號操作與記憶體使用；只啟用命名不會自動寫入生命週期日誌。詳細事件另以 `DEBUG_GUI_OBJECT_LOG` 控制：Debug 自動啟用，Release 預設關閉。啟用後記錄建立、複製、重新命名、顯示、隱藏、容器掛接與銷毀。大量 GUI 更新可能產生大量日誌並增加 I/O 成本，需要排查物件生命週期時才在 Release 開啟。CMake 啟用事件日誌時也會啟用 GUI 命名。

以下為建議的 Release 設定：

```bash
cmake -S "$project_workspace/source" \
      -B "$project_workspace/build/x64-win32-pe-mingw_Release" \
      -DDEBUG_VULKAN_NAME=ON \
      -DDEBUG_GUI_OBJECT_NAME=ON \
      -DDEBUG_GUI_OBJECT_LOG=OFF
cmake --build "$project_workspace/build/x64-win32-pe-mingw_Release" --parallel 4
```

將 `DEBUG_GUI_OBJECT_LOG` 設為 `ON` 可啟用詳細事件；將它與 `DEBUG_GUI_OBJECT_NAME` 都設為 `OFF` 則關閉自動 GUI 命名及事件，手動 `name_set()` 仍可使用。

日誌寫入會重用檔案並同步多執行緒存取，每筆寫入後 `flush()`，不必等到關閉檔案才送出。成功 flush 的紀錄已送交作業系統，通常能在程式閃退後保留；閃退時正在寫入的最後一筆可能不完整。這不是每筆強制寫入實體磁碟的保證，斷電、系統當機或磁碟故障仍可能遺失快取中的資料。目的地無法寫入時會輸出錯誤並繼續執行，避免無限重試。

啟動時若沒有可用的 Vulkan 驅動或合格 GPU，程式會顯示標題為 `Storage` 的 Windows 錯誤訊息視窗，列出失敗原因及日誌位置，然後以非零狀態結束。這個「啟動失敗對話框」不需要 GPU 成功初始化，也會在讀取帳戶金鑰前處理圖形初始化失敗。

GPU 裝置遺失、Vulkan 錯誤與資料夾下載回歸測試可在完成建置與安裝後，從原始碼目錄執行：

```bash
python3 tests/run_gpu_device_loss.py
python3 tests/run_vulkan_startup_error.py
python3 tests/run_folder_download.py
python3 tests/run_background_multi.py
python3 tests/run_token_refresh.py
```

測試需要 Wine、Xvfb、xauth 與可用的 Vulkan 驅動（可使用 Mesa Lavapipe），會在暫存目錄中注入 Vulkan 失敗，檢查畫面狀態保留、重新呈現、重建重試上限及關閉時的佇列保存，並以本機模擬 Drive 驗證上下載內容、發布與目錄刷新。只複製字型、shader 與 DLL，不讀取金鑰或連線至雲端；GPU 與 Vulkan 啟動測試可用 `--build` 與 `--runtime` 指定其他建置及安裝目錄。這些測試模擬 API 錯誤，實際驅動重設或硬體移除仍需在 Windows 環境驗證。

GPU 測試也會檢查物件命名的 macro 開關、extension 不可用或命名失敗時的處理，以及視窗縮放和 GPU 重建後的物件命名。可分別在 Release 預設設定與 `DEBUG_VULKAN_NAME=OFF` 編譯後執行。

GUI 命名回歸測試也包含於 GPU 測試程式，檢查自動名稱、複製與指定運算、內嵌子物件和生命週期日誌；可分別以命名開啟／關閉與 `DEBUG_GUI_OBJECT_LOG=ON` 建置驗證。也會以不執行解構函式的程序結束，檢查已 flush 的日誌仍可讀取。

新增 Vulkan 回歸涵蓋資源清理、未知 API 錯誤、記憶體類型位元遮罩、空／未指定／不支援 surface 格式、透明度與深度格式退回、fence／acquire 逾時、out-of-date、surface 遺失、GPU 初始化退回、記憶體不足，以及 shader 損壞時的建構失敗清理。`tests/run_gpu_device_loss.py --mode surface-lost` 可只跑指定項目；可重複 `--mode`，`--mode transfer` 則執行本機上下載整合測試。

資料夾下載測試另涵蓋 201 個分塊跨頁索引與內容亂序、空檔案及目錄、工作最後一次回呼的生命週期，以及準備、索引、內容、解碼、大小與本機輸出錯誤。測試確認失敗原因保留在畫面、失敗後的在途回應不再寫檔，其他下載仍可完成；也會注入八種異常或部分失敗的批次回應，檢查安全重試及已成功子請求的保留。

`run_background_multi.py` 另需要 OpenSSL 指令，用本機自簽 TLS 上傳製造傳送阻塞，檢查持續繪圖與縮放、GUI 輪詢延遲、停止等待、跨 worker 移交 handle、背景解析及例外傳遞。資料夾測試中的八種批次回應錯誤會分別驗證同步與背景重試。Linux 可用 `sudo -n unshare --net python3 tests/run_background_multi.py --offline` 各跑三次介面關閉及丟包測試（需 sudo、unshare 與 nft）；腳本只修改隔離 network namespace 的 loopback，並使用獨立 Wine prefix，不會關閉主機網路或存取雲端帳戶。

`run_token_refresh.py` 會嚴格檢查 JWT bearer grant type 與 assertion，使用 OpenSSL 產生一次性的測試金鑰，僅將測試副本的 OAuth endpoint 指向本機模擬服務，其餘使用正式帳戶、簽署、快取與請求實作。測試涵蓋首次背景刷新、`expires_in` 到期、16 個並行及延遲 401、POST／PATCH 重送、批次部分成功與內層 401 後的格式／503 錯誤、同步 easy／multi 重試、持續 401 的上限、OAuth 暫時／永久失敗、異常回應，以及 blob 背景錯誤回報及關閉時的授權等待。所有憑證、token 與佇列均為暫存測試資料，不讀取正式金鑰或呼叫雲端服務。

`python3 tests/run_task_lifecycle.py` 使用本機模擬 Drive，強制終止 Windows 程序並重新啟動，驗證上下載續傳、精確位元組內容、blob ID 保存、避免重複發布，以及切換帳戶後的工作隔離。只使用一次性假帳戶與佔位 ID，不讀取正式憑證或連線至雲端。

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
