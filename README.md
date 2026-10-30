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
- 新增資料夾，剪下／複製並貼上檔案或整個資料夾，以及永久刪除項目與所有子內容。
- 顯示傳輸進度、預備 blob 數量與 FPS。
- 支援視窗縮放、最大化、最小化及還原；拖曳縮放期間也會重繪與更新 FPS。最小視窗內容尺寸為 800 × 600。

## 使用說明

### 1. 準備執行環境

需要 Windows x64、支援 Vulkan 1.3 且具備必要 features 與圖形／呈現佇列的圖形環境，以及可連線至 Google API 的網路。

將執行檔與 DLL 集中放在 `bin/`，字型、shader、設定與狀態保留在其上一層的資料目錄：

```text
runtime/
├── bin/
│   ├── storage.exe
│   ├── compare.exe
│   ├── filegen.exe
│   ├── libc++.dll
│   ├── libunwind.dll
│   ├── glfw3.dll
│   ├── libcurl.dll
│   ├── libcrypto-3-x64.dll
│   └── libssl-3-x64.dll
├── key.json
├── root/
│   └── <資料空間雜湊>
├── blob_queue/
│   └── <資料空間雜湊>
├── font/
│   └── NotoSansTC-VariableFont_wght.ttf
└── shader/
    ├── solid.spv
    └── texture.spv
```

字型使用 [Noto Sans TC](https://fonts.google.com/noto/specimen/Noto+Sans+TC)。目前 CMake 安裝流程會將三個執行檔與上述 DLL 安裝至 `bin/`，將 shader 安裝至 `shader/`；字型及帳戶設定需要自行準備。

### 2. 設定 Google Drive 存取

1. 在 Google Cloud 專案中啟用 Google Drive API。
2. 準備具有所需 Drive 操作權限的服務帳戶，下載其 JSON 金鑰。
3. 將各服務帳戶的 JSON 金鑰放入資料目錄的 `key/` 資料夾，再從右上角帳號選單登入。每個檔案代表一個帳戶，依 JSON 內容的 `client_email` 與 `private_key` 辨識，不限制檔名或副檔名；只讀取該資料夾第一層，忽略無效或非帳戶檔案。`key/` 可以不存在，不影響 Google 使用者帳戶。原有 `key.json` 或設定指定的金鑰路徑仍可用於啟動登入，但只有 `key/` 內的帳戶會自動列在選單。

目前採用服務帳戶驗證。請確認帳戶能建立、讀取與更新所需的 Drive 物件；啟用 API 後若仍遇到權限或配額錯誤，需依 Google 的錯誤回應檢查帳戶設定。

`key/` 內的金鑰與原有 `key.json` 含有私鑰，請妥善保管，勿提交到 Git、Issue 或 Pull Request。

### 3. 啟動程式

可直接雙擊 `bin/storage.exe`，或在 PowerShell 中啟動：

```powershell
Set-Location "C:\path\to\runtime"
.\bin\storage.exe
```

主程式位於 `bin/`（不分大小寫）時，會根據執行檔位置將 CWD 設成 `bin` 的上一層，從那裡尋找字型、shader、金鑰與狀態；不受啟動時的 CWD 影響。開發時直接執行未放進 `bin/` 的主程式，仍使用原本 CWD。`compare.exe` 與 `filegen.exe` 保留命令列啟動時的 CWD，方便使用相對檔案路徑。

首次啟動時，若沒有該帳號的 `root/<資料空間雜湊>` 檔案，程式會建立雲端邏輯根目錄並儲存其 ID。背景工作也會預先建立大量空白 blob，累積可供上傳使用的物件。初次準備會產生較多 API 請求，請等待 `blob_queue_count` 累積。

服務帳戶與 OAuth 帳號採用同一套命名：根目錄 ID 存在 `root/<資料空間雜湊>`，預備 blob ID 清單存在 `blob_queue/<資料空間雜湊>`，不再區分第一個帳號，也不建立 `blob_queue.account`。資料空間識別對服務帳戶為 email，對 OAuth 為 `appdata:<OAuth 用戶端 ID 的 SHA-256>:<使用者 email>`；檔名是該識別字串 UTF-8 位元組的 SHA-256 小寫十六進位值。

舊版資料不會自動搬移。升級前請先關閉程式，依上述規則將舊的 `root`、`root.appdata.<雜湊>` 與 blob 佇列搬到新路徑：已有雜湊後綴的檔案去掉 `root.appdata.` 或 `blob_queue.` 前綴，單一 `root` 使用其所屬服務帳戶的雜湊；單一 `blob_queue` 的歸屬可由舊的 `blob_queue.account` 讀取，該綁定檔不再使用。若舊檔與新資料夾同名，可先暫時改名再建立資料夾；原有 exe 與 DLL 請移入 `bin/`。啟動時不再檢查固定名稱的舊 `root` 或 `blob_queue` 檔案；登入只讀取設定的根目錄 ID 資料夾，不搜尋其他位置。設定的資料夾不存在時自動建立；該帳號的雜湊檔不存在時，建立新的雲端根目錄並保存 ID。若設定的資料夾路徑已被檔案佔用，該次登入會失敗並保留原檔；預備 blob 佇列仍使用 `blob_queue/<資料空間雜湊>`。

設定中的 `root_path` 現在表示根目錄 ID 的資料夾，預設是 `root`；先前版本曾保存的預設值 `root/root` 會解析成 `root`，避免再多一層，已在 `root/root/` 的檔案需先搬到 `root/`。若先前設定的是檔案路徑，請改成放置雜湊檔案的資料夾。新續傳紀錄保存這個資料夾位置；舊紀錄仍以已保存的雲端 root ID 恢復，並使用目前設定的根目錄 ID 資料夾。

### 4. 上傳、瀏覽與下載

| 操作 | 使用方式 |
| --- | --- |
| 上傳檔案 | 點選「上傳檔案」，選取一個或多個檔案，確認後加入傳輸工作。 |
| 上傳資料夾 | 點選「上傳資料夾」，選取資料夾，程式會遞迴處理其中的檔案與子目錄。 |
| 瀏覽資料夾 | 點選清單中的資料夾名稱，載入該目錄的內容。 |
| 下載 | 點選項目右側的「下載」，再選擇本機目的地資料夾。內容會以原名稱存放在該資料夾下。 |
| 新增資料夾 | 點選頂端「新增資料夾」，輸入名稱並確定；允許同名資料夾；空名稱與斜線會被拒絕。 |
| 剪下／複製 | 點選「下載」右側的「剪下」或「複製」，記住單一檔案或資料夾；此時尚未變更雲端內容。 |
| 貼上 | 瀏覽目的目錄後，按頂端「貼上」並確認，將選取項目移至或複製到目前顯示的目錄。允許同名項目；不可貼入自己或自己的子資料夾。上方路徑列的父資料夾按鈕可用來前往其他分支。 |
| 刪除 | 點選「複製」右側的「刪除」，確認後永久刪除此項目及其全部子內容；取消對話框會保留資料。 |
| 取消選取 | 在檔案或資料夾對話框按「取消」，此次操作不會新增傳輸工作。 |
| 查看傳輸狀態 | 從左側進度區查看工作進度；按住 F3 可顯示獨立的連線狀態面板。欄寬與行距依視窗配置，放不下的連線可用「上一頁／下一頁」查看，長文字可在該行水平捲動。 |
| 暫停／繼續傳輸 | 上下載工作在傳輸中顯示「暫停」；暫停後在同一位置顯示「繼續」「取消」，網路失敗時則顯示「重試」「取消」。 |

- **傳輸狀態**：顯示已完成大小、總大小與進度條，支援暫停、繼續、重試及取消。未完成的工作可在重新啟動後接續；失敗時保留原因與已完成的進度。檔案與資料夾的上下載在取消收尾時顯示「取消中」，收尾完成後顯示「已取消」及「確定」；取消與網路失敗訊息使用操作名稱與路徑，不會沿用「準備上傳／下載」的階段文字。
- **上傳與下載**：上傳完成內容及索引後才加入目的目錄，並自動刷新清單；進度達到總大小時，仍需等待「已完成」。下載遇到同名目的地會先確認是否刪除，空檔案與空資料夾也會保留；失敗或取消後的部分下載不代表完整檔案。
- **目錄瀏覽**：上方路徑列可返回任一父目錄。清單分成「檔名／大小／操作」三欄，窄視窗優先保留右側操作按鈕；長名稱、路徑及目錄清單均可捲動，支援橫向滾輪與 Shift＋滾輪。
- **目錄管理**：新增、移動、複製與刪除在背景執行，顯示進度並保存中斷恢復紀錄。剪下保留原有內容，複製建立獨立副本；刪除會永久移除項目及其子內容。目錄操作開始後不提供暫停或取消，網路失敗時可重試。
- **Google 登入**：除了服務帳戶，也可將 Google Cloud 桌面 OAuth 設定存成 CWD 的 `client.json`。按右上角「登入」開啟帳號選單，再按「新增帳戶」完成瀏覽器授權；成功後顯示頭像並加密保存登入權杖。使用者登入只要求 `drive.appdata`，資料存放於應用程式專用的隱藏空間，並與服務帳戶及其他帳戶的資料分開。
- **帳號選單**：整個 dropdown 使用深灰色 `std_dark_gray` 外框，涵蓋帳號列表與底部按鈕。登入後按右上角頭像，登入前按「登入」，即可查看 `key/` 的服務帳戶與目前 OAuth 用戶端曾登入過的帳號。服務帳戶依 email 與檔案路徑排序，列在所有 Google 使用者之前；每個 JSON 各列一筆，即使 email 相同也保留。服務帳戶列與登入後右上角頭像都顯示 `std_green_light` 色塊，列內只提供「登入」，不提供「移除」；新增或移除請自行管理 `key/` 檔案，重新開啟選單即可更新清單。以下頭像快取與「移除」行為適用於 Google 使用者帳戶。每列頭像左側保留 `std_margin`，再依序顯示 email、淺藍色無外框的「登入」「移除」；照片未下載或無法讀取時使用預設頭像。照片、email 與操作欄以深灰色直線分隔，兩個操作間以淺藍色直線分隔。長 email 與多筆帳號可捲動；帳號列表高度包含列間距，全部內容放得下時不顯示右側滑塊，超出時依可見比例與捲動位置顯示。拖曳縮放視窗期間，選單會即時跟隨頭像移動，並調整可見高度。底部提供白色有外框的「新增帳戶」，登入後另有「登出」。目前帳號的「移除」呈灰色且無法操作，必須先登出；「移除」只刪除本機加密憑證與相關金鑰，保留雲端資料、root、預備 blob 佇列與續傳紀錄。登出或移除帳號會像關閉程式一樣停止該帳號的工作並保留檢查點，下次登入後恢復；手動暫停的工作仍保持暫停。登出保留帳號列表；Google 使用者帳戶重新啟動後也維持登出。按選單外側或 Esc 關閉選單。新增、切換、登出與移除帳戶會顯示具名工作列；切換帳戶時包含目標 email。工作完成、失敗或取消後保留結果並提供「確定」，按下後移除該列；先前結果的「確定」不會影響正在執行的新工作。套用設定也使用相同的工作列操作。

### 5. 設定與清除

「設定」使用 lgo.GUI 表單，提供日誌資料夾、字型檔案、帳戶 JSON 檔案、根目錄 ID 資料夾（預設 `root`），以及網路失敗重試次數。次數可填非負整數或「無限」（檔案中以 `-1` 表示）；`0` 表示首次失敗即暫停，預設無限。按「取消」不套用，按「確定」驗證並在背景載入帳戶／根目錄，成功後寫入 CWD 的 `settings.json`。新設定立即生效，字型和日誌路徑也會更新；既有傳輸保留建立工作時的帳戶、根目錄與目的地，新操作使用新設定。每個帳戶的 blob 佇列獨立保存，避免混用 ID。表單支援 UTF-8 中文輸入、Backspace、Ctrl+A 全選及 Ctrl+V 貼上。OAuth 自身的三次嘗試與 HTTP 401 刷新上限仍獨立計算。

設定更新、清除進度及目錄錯誤等紅色提示統一使用淺紅色 `std_red_light`。

Google 使用者登入時，「清除」只列舉並永久刪除此應用程式在 `appDataFolder` 的資料，對話框會顯示此範圍；不列舉或刪除一般 Drive 檔案。每個新建的根目錄與內容 blob 都指定 `parents: ["appDataFolder"]`，清除清單使用 `spaces=appDataFolder`。程式的資料夾、搬移與複製是此資料空間中的邏輯索引操作，不會將檔案移出 appDataFolder，也不使用分享或垃圾桶 API。

服務帳戶模式的「清除」會永久刪除目前帳戶中的**全部 Drive 檔案**，範圍包含應用程式根目錄以外的檔案。lgo.GUI 對話框會顯示帳戶及範圍，必須輸入「清除」並按「清除全部」。確認後先取消此帳戶的傳輸、停止預備 blob 補充，完整列出檔案後才開始刪除；其他帳戶的傳輸保留。完成後建立新的空白應用程式根目錄。進度區會顯示進度及失敗原因，可重試或取消尚未完成的刪除。取消收尾期間保留「取消中」；取消或清除完成後顯示「確定」，按下才移除整列。進行中的「取消」與結束後的「確定」位於同一個起始按鈕位置；網路失敗時「重試」與「取消」並排，以直線分隔。移除結果列不會刪除未完成的續清紀錄，也不會解除該帳戶的清除限制；可重新按頂端「清除」接續。已刪除的檔案無法復原。中斷清除的清單與進度保存在 `clear.<帳戶雜湊>`，下次啟動同帳戶時接續；清除完成前不允許新增該帳戶的傳輸。清除在背景使用 batch API，每批最多刪除 100 個檔案，批次依序執行；只有整批完成才保存新的進度。批次內已成功的刪除不會在網路重試時重送；若程序中斷，尚未保存的該批會重播，已刪除檔案的 404 視為成功。

程式入口除 `std::exception` 外也有 `catch (...)`，非標準 C++ 例外會記錄未知例外訊息並回報失敗。程序被直接終止時則依已落盤的檢查點恢復，不依賴例外處理器執行。

### 6. 關閉與保留狀態

傳輸完成後，使用視窗右上角的叉叉正常關閉程式。請保留以下檔案：

| 檔案 | 用途 |
| --- | --- |
| `key/` | 帳號選單中的服務帳戶 JSON 金鑰，每個檔案代表一個帳戶，檔名不限；資料夾可不存在。 |
| `key.json` | 存取雲端資料使用的服務帳戶金鑰；使用 Google 登入時不需要。 |
| `client.json` | Google 桌面 OAuth 用戶端設定，不包含使用者登入權杖。 |
| `root/<資料空間雜湊>` | 各服務帳戶或 OAuth 資料空間的雲端邏輯根目錄 ID。刪除後，下次登入會建立另一個根目錄；可在設定中更換存放資料夾。 |
| `blob_queue/<資料空間雜湊>` | 同一資料空間的預備 blob ID 清單，檔名與該空間的 root 檔案相同。 |
| `settings.json` | 已套用的設定，未建立時使用原先的預設相對路徑。 |
| `task`、`task.data/` | 未完成工作索引、意圖、blob ID 分配、下載進度及成功回應快取。 |
| `clear.<資料空間雜湊>` | 尚未完成的清除紀錄；服務帳戶涵蓋全部 Drive 檔案，Google 使用者只涵蓋該應用程式的 appDataFolder。 |

佇列在啟動時讀取並保留原檔，分配 ID 或補充後持續保存。工作在網路寫入前先記錄 blob ID，成功回應與下載寫入位置也持續保存；檢查點使用同目錄暫存檔、磁碟 flush 和原子取代。正常關閉、C++ 例外或直接終止程序後，下次啟動會自動恢復未完成的上下載，並使用各工作記錄的帳戶路徑及身分。`task.data/` 會快取已收到的回應以重建排程，下載內容快取可能佔用接近檔案大小的額外空間；工作完成或取消後移除。

手動暫停狀態也會先保存至工作紀錄；重開程式後仍維持暫停，按「繼續」才恢復。暫停時取消會移除續傳紀錄並停止後續工作，保留已下載的部分檔案；取消不會回滾已完成的雲端寫入。

恢復下載時保留已保存的檔案前綴，截掉檢查點之後的尾端，重播已完成回應以重建索引；不會再次刪除整個目的地或重複附加前綴。恢復上傳會使用原 blob ID，並檢查來源檔案大小、修改時間及資料夾清單；來源已改變時顯示失敗，需重新新增工作。發布目錄連結時會檢查項目 ID，避免閃退後重複加入。請保留原始來源、部分下載及舊帳戶的金鑰檔；`task` 必須和 `task.data/` 一併搬移或備份。

## 目前限制

- 顯示卡驅動程式重設、移除或 GPU 異常可能造成 Vulkan 裝置遺失（`VK_ERROR_DEVICE_LOST`）。程式會重新選擇可用的 GPU、重建裝置及繪圖資源，保留目前目錄、傳輸工作與進度；重建期間畫面可能暫停更新，傳輸不會因此重新開始。重建失敗時至少間隔一秒再試，同一個 60 秒區間最多嘗試三次，超過後會關閉並寫回未使用的 `blob_queue`。未完成的傳輸會由磁碟檢查點於下次啟動時恢復；復原過程與失敗原因會記錄在 `logfile/`。
- 啟動會檢查 Vulkan loader、必要的 instance／device extensions、圖形與呈現佇列、必要 features、SDR surface 格式及字型貼圖能力；GPU 不符條件時記錄具體原因，裝置初始化失敗時會嘗試其他合格 GPU。Debug 缺少 validation layer 或 debug-utils 時會略過可選診斷功能。使用 Vulkan 1.3，無須 `VK_KHR_get_surface_capabilities2`。
- swapchain 過期或 suboptimal 會重建；surface 遺失會重建 surface 與 GPU 資源，保留傳輸工作，沿用上述重試上限。fence 與影像擷取每次最多等待 50 毫秒，逾時或尚無可用影像時返回事件迴圈。surface 格式變更時重建 pipeline；透明度模式依支援能力選擇，深度格式優先 D32，必要時退回 D16。
- 不支援的資源尺寸、無適合記憶體類型、損壞的 SPIR-V 標頭及 Vulkan API 失敗會回報原因。無法恢復的錯誤會停止傳輸、保存未使用的佇列並顯示錯誤對話框；程式以非零狀態結束。尚未完成的傳輸會保留跨次啟動續傳的檢查點。validation 訊息只記錄，不會從驅動程式的 C callback 拋出例外。
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
python3 tests/run_task_lifecycle.py
python3 tests/run_gui_settings.py
python3 tests/run_text_scroll.py
python3 tests/run_directory_operations.py
```

測試需要 Wine、Xvfb、xauth 與可用的 Vulkan 驅動（可使用 Mesa Lavapipe），會在暫存目錄中注入 Vulkan 失敗，檢查畫面狀態保留、重新呈現、重建重試上限及關閉時的佇列保存，並以本機模擬 Drive 驗證上下載內容、發布與目錄刷新。只複製字型、shader 與 DLL，不讀取金鑰或連線至雲端；GPU 與 Vulkan 啟動測試可用 `--build` 與 `--runtime` 指定其他建置及安裝目錄。這些測試模擬 API 錯誤，實際驅動重設或硬體移除仍需在 Windows 環境驗證。

GPU 測試也會檢查物件命名的 macro 開關、extension 不可用或命名失敗時的處理，以及視窗縮放和 GPU 重建後的物件命名。可分別在 Release 預設設定與 `DEBUG_VULKAN_NAME=OFF` 編譯後執行。

GUI 命名回歸測試也包含於 GPU 測試程式，檢查自動名稱、複製與指定運算、內嵌子物件和生命週期日誌；可分別以命名開啟／關閉與 `DEBUG_GUI_OBJECT_LOG=ON` 建置驗證。也會以不執行解構函式的程序結束，檢查已 flush 的日誌仍可讀取。

新增 Vulkan 回歸涵蓋資源清理、未知 API 錯誤、記憶體類型位元遮罩、空／未指定／不支援 surface 格式、透明度與深度格式退回、fence／acquire 逾時、out-of-date、surface 遺失、GPU 初始化退回、記憶體不足，以及 shader 損壞時的建構失敗清理。`tests/run_gpu_device_loss.py --mode surface-lost` 可只跑指定項目；可重複 `--mode`，`--mode transfer` 則執行本機上下載整合測試。

資料夾下載測試另涵蓋 201 個分塊跨頁索引與內容亂序、空檔案及目錄、工作最後一次回呼的生命週期，以及準備、索引、內容、解碼、大小與本機輸出錯誤。測試確認失敗原因保留在畫面、失敗後的在途回應不再寫檔，其他下載仍可完成；也會注入八種異常或部分失敗的批次回應，檢查安全重試及已成功子請求的保留。

`run_background_multi.py` 另需要 OpenSSL 指令，用本機自簽 TLS 上傳製造傳送阻塞，檢查持續繪圖與縮放、GUI 輪詢延遲、停止等待、跨 worker 移交 handle、背景解析及例外傳遞。另以慢速上下載驗證在途暫停、繼續後內容一致、同步背景請求暫停、暫停時取消及 handle 重用、其他工作持續運作，以及已完成回應在暫停期間保留。另填滿正式排程器的 64 個傳輸名額，確認暫停後全部歸還、暫存請求不持有 handle、其他工作在按「繼續」前能完成，並驗證快速反覆暫停／繼續及回呼只執行一次。資料夾測試中的八種批次回應錯誤會分別驗證同步與背景重試。Linux 可用 `sudo -n unshare --net python3 tests/run_background_multi.py --offline` 各跑三次介面關閉及丟包測試（需 sudo、unshare 與 nft）；腳本只修改隔離 network namespace 的 loopback，並使用獨立 Wine prefix，不會關閉主機網路或存取雲端帳戶。

`run_token_refresh.py` 另以嚴格的 JSON 子請求驗證批次建立／更新、Host／自訂標頭保存、外層 200 內層 403／404 錯誤摘要、敏感訊息排除，以及無限重試退避與手動重設。它也會嚴格檢查 JWT bearer grant type 與 assertion，使用 OpenSSL 產生一次性的測試金鑰，僅將測試副本的 OAuth endpoint 指向本機模擬服務，其餘使用正式帳戶、簽署、快取與請求實作。測試涵蓋首次背景刷新、`expires_in` 到期、16 個並行及延遲 401、POST／PATCH 重送、批次部分成功與內層 401 後的格式／503 錯誤、同步 easy／multi 重試、持續 401 的上限、OAuth 暫時／永久失敗、異常回應，以及 blob 背景錯誤回報及關閉時的授權等待。所有憑證、token 與佇列均為暫存測試資料，不讀取正式金鑰或呼叫雲端服務。

`python3 tests/run_oauth_login.py` 使用一次性的用戶端設定、本機 OAuth／Drive 模擬服務與軟體測試 KSP，驗證 loopback 回呼、PKCE S256、state／重複參數拒絕、以獨立 OpenSSL 驗證 ES256 簽章、授權碼雜湊 jti、nonce 挑戰、權杖輪替、16 個並行讀取共用背景換發、重新登入更新共用帳戶、舊登入延遲換發不覆寫新憑證、延遲 401、跨程序還原、加密檔竄改拒絕與取消。另驗證雙帳號歷史、加密頭像快取、帳號選取持久化，以及移除後延遲換發不重建憑證；只要求 `drive.appdata`、一般／批次 blob 的 appDataFolder parent，以及清除只列舉和刪除 appDataFolder 的檔案。另驗證服務帳戶依 JSON 內容列在 Google 使用者之前、同 email 的兩種帳戶使用不同憑證與 root、服務帳戶不提供移除且不修改私鑰、列表與工具列的綠色色塊及 GPU 重建恢復、`key/` 不存在時仍可操作；將正式 GUI 放在 `bin/` 並從無關 CWD 啟動及重啟，檢查資料寫入 `bin` 的上一層、root 與佇列只產生雜湊檔名、舊版續傳紀錄仍可恢復、登入前後帳號選單、具名帳戶工作列與完成／取消／失敗後的「確定」、確認舊結果不影響新切換、目前帳號禁止移除、登出後重啟、重新登入恢復暫停工作、移除保留續傳紀錄、32×32 無外框登入、800 像素工具列、主迴圈暫停期間連續縮放仍同步對齊選單與頭像、照片的實際 sRGB 色彩與 GPU 重建恢復、服務帳戶 root 保留與正常關閉，並檢查權杖沒有出現在輸出或日誌。`tests/run_task_lifecycle.py` 另驗證實際傳輸中登出會保留檢查點與暫停狀態，重新登入後只恢復一次並產生完全相同的下載內容。Wine 缺少所需的 CNG ECDSA／RSA-OAEP 實作，因此協定測試僅在測試副本替換 KSP 與瀏覽器啟動器；另用未替換的正式實作確認 CNG 失敗不會降級。原生 Windows KSP 與真實 Google 授權仍需在 Windows 上驗證；測試不讀取正式 client.json／key.json，不操作雲端帳戶。

`python3 tests/run_task_lifecycle.py` 使用本機模擬 Drive，實際強制終止 Windows 程序並重新啟動，驗證上下載續傳、精確位元組內容、blob ID 保存、避免重複發布、切換帳戶後的工作隔離、目的地覆寫拒絕、網路重試／取消、非同步瀏覽、UTF-8 輸入與 lgo.GUI 對話框，以及全部檔案清除中斷後的恢復。另驗證服務帳戶清單依 JSON 內容辨識、不依賴檔名、保留同 email 的多個檔案、忽略無效檔案及子資料夾、不建立缺少的 `key/`；服務帳戶使用同一資料夾內的獨立雜湊檔名、自訂根目錄 ID 資料夾與續傳路徑、舊 `root/root` 設定的修正、固定名稱的舊檔不阻擋啟動或自訂 root 登入、缺少設定資料夾及帳號 root ID 時自動建立且保留不相關舊檔，以及手動暫停在重啟後仍保留、繼續不會清除網路錯誤；以實際滑鼠點擊測試上下載的暫停／繼續／取消，圖形進度與成功／取消／拒絕覆寫後的「確定」，取消收尾期間的「取消中」，進度列移除後平滑往下補位、不超越目標及游標原地連點、不同間距與排列方向下的移除／再新增，以及 F3 面板在 800×600 視窗下的分頁與欄位間距。測試使用一次性帳戶資料及至少 25,000 個佔位 ID，完全不讀取正式憑證或刪除雲端資料。

清除續作測試會在第二批刪除僅套用 50 個子請求、尚未收到回應時強制結束程序，重啟後驗證 404 重播、同批部分 503 重試及成功子請求不重送。

`run_gui_settings.py` 另需 `xdotool`，會操作正式程式入口的設定與清除對話框，驗證取消／確定、Backspace、Ctrl+A、UTF-8 貼上、即時字型與日誌更新、錯誤確認文字及全部 Drive 檔案清除，並以實際滑鼠點擊驗證清除按鈕位置、執行中與網路失敗時取消、重新清除、重試，以及取消／完成後按「確定」移除整列。剪貼簿內容由一次性 Windows 測試工具設定，避免 Wine／X11 剪貼簿橋接影響結果。若有 Pillow，另存對話框截圖至 `/tmp/storage-*.png`。帳戶、檔案與刪除操作均限本機模擬資料。

`python3 tests/run_directory_operations.py` 使用本機模擬 Drive，驗證新增資料夾、名稱驗證、跨頁移動、父目錄更新、同名項目並存與依 ID 刪除、獨立遞迴複製與中斷恢復、自己／子目錄拒絕、遞迴刪除、索引歸屬檢查、與上傳同時更新目錄，以及移動／刪除程序中斷後的接續。另實際操作剪下／複製、切換目錄、貼上確認與取消，並以截圖檢查標準直向分隔線、較暗的圓形滑鼠漸層及縮放。測試不讀取正式帳戶或雲端資料。另驗證多層路徑按鈕、`>` 分隔符的非零文字／裁切寬度與實際截圖像素、原生／Shift 橫向滾輪、路徑捲軸拖曳、任一祖先返回、移動後的父目錄資訊與縮放。另驗證 12 像素欄位標題、檔案與資料夾的三欄對齊、B／kB／MB／GB 的切換邊界、捨去小數及 64 位元整數上限的檔案大小、反覆縮放時大小欄的隱藏／恢復、120 像素檔名下限、右側四個操作的完整位置，以及深灰色欄位與項目分隔線的實際像素。加上 `--scroll-only` 可只驗證路徑列、欄位排版與正式畫面的長清單捲動。加上 `--pool-only` 可只驗證共用 64 個實體 handle、索引探索／複製／刪除並行、混合上下載、只改索引的移動、亂序回應的檢查點、刪除來源後副本仍可完整下載、小檔案網路重試，以及停止 GUI 派送後仍可結束等待中的工作。批次測試使用超過 6,400 個內容 blob，驗證 64 個 HTTP batch 並行、每批最多 100 個子請求、反序子回應、讀寫與刪除的部分 503 重試，以及刪除的 404 重播；成功的寫入與刪除不會隨失敗子請求重送。

目錄操作測試另涵蓋連結移除後、內容刪除完成前的畫面更新：阻塞內容 DELETE、讓目錄刷新失敗，確認舊列控制項已移除、同名項目仍保留、失效剪貼簿已清除，以及較早的目錄結果不會重新套用。另驗證切換目錄尚未完成時仍會移除舊清單中的已刪除項目、刪除正在瀏覽的祖先時立即返回來源目錄，以及內容刪除失敗後可重試完成。加上 `--unlink-only` 可只執行這些檢查。

`python3 tests/run_text_scroll.py` 不使用帳戶或網路，以實際滑鼠操作水平捲軸，驗證原生橫向滾輪、左右 Shift＋滾輪、平滑位移及停止位置、拖曳捕捉與放開、軌道點擊、縮放、延後載入文字、字型重載及隱藏後的事件清理；並檢查 Vulkan 截圖，確認中文字不超出裁切區、黑色滑塊與其餘軌道的預設／自訂雙色，以及文字按鈕變色時滑塊顏色不變。

`python3 tests/run_scroll.py` 驗證通用 `scroll_t` 的 X／Y／雙軸視窗、內容原點、部分可見按鈕的命中範圍、父子裁切交集、巢狀滾輪路由與邊界傳遞、子捲軸拖曳、遮擋、縮放、延後新增文字、內容替換和清理。另驗證不同更新頻率下的加速度／速度上限、提前煞車、精確停止、反向目標、滾輪動畫中間位置、div 動畫預設關閉、連續佈局變更、祖先平移、停用時完成定位與隱藏後更新。另以實際像素確認水平與垂直捲軸的黑色滑塊、原色軌道及自訂雙色。測試透過真實滑鼠事件與 Vulkan 截圖，不需要帳戶或網路。

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
