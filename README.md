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
├── client.json
├── key/
│   ├── accounts.json
│   └── <project ID>-<account ID>.json
├── root/
│   └── <空間 ID>
├── blob_queue/
│   └── <服務帳戶 address 的 SHA-256>
├── font/
│   └── NotoSansTC-VariableFont_wght.ttf
└── shader/
    ├── solid.spv
    └── texture.spv
```

字型使用 [Noto Sans TC](https://fonts.google.com/noto/specimen/Noto+Sans+TC)。目前 CMake 安裝流程會將三個執行檔與上述 DLL 安裝至 `bin/`，將 shader 安裝至 `shader/`；字型及帳戶設定需要自行準備。

### 2. 建立空間與服務帳戶

準備 Google 桌面 OAuth 用戶端設定 `client.json`。使用者須有列出／建立專案、啟用服務、建立服務帳戶及金鑰的權限；Resource Manager、Service Usage 與 IAM API 須在 OAuth 用戶端的呼叫專案啟用。程式使用 `cloud-platform`、`userinfo.email`、`userinfo.profile` 授權，Google 使用者只用來管理 Cloud 資源，實際儲存一律使用空間關聯的服務帳戶。

1. 右上角「空間」→「新增空間」，輸入名稱。
2. 在空間列按「編輯」→「新增帳戶」，選擇 Google 使用者。清單每列為照片、email、「使用」「移除」；「新增使用者」會開啟瀏覽器授權。
3. 使用者的「使用」列出可存取且 ID 以 `storage-ai` 開頭的有效專案，顯示各專案名稱。「新增專案」詢問顯示名稱，ID 使用 `storage-ai-<隨機碼>`。
4. 專案的「使用」列出程式建立的服務帳戶，顯示 address 的 `@` 前半部。「新增帳戶」詢問名稱；名稱作為 account ID，須為 6–30 個小寫英文字母、數字或連字號，以字母開頭、不以連字號結尾。
5. 服務帳戶的「使用」將它關聯到正在編輯的空間，返回關聯清單。回到空間列按「登入」，即可瀏覽與傳輸。

程式啟用選定專案的 Drive API，將新金鑰保存在 `key/<project ID>-<account ID>.json`，另將 address 與金鑰位置登記於 `key/accounts.json`。**不掃描或使用外部放進 `key/` 的私鑰**；請一併保留程式建立的金鑰與登記檔。建立金鑰不自動重送；本機保存失敗時嘗試撤銷本次金鑰。Google 使用者授權由 Windows CNG 加密保存，照片使用本機快取；移除使用者只刪除此電腦的 Cloud 授權與照片。

金鑰含有私鑰，請勿提交到 Git、Issue 或 Pull Request。本版本使用新的空間設定與 blob 格式，不提供舊帳戶、root 或 blob 格式的自動轉換。

### 3. 啟動程式

可直接雙擊 `bin/storage.exe`，或在 PowerShell 中啟動：

```powershell
Set-Location "C:\path\to\runtime"
.\bin\storage.exe
```

主程式位於 `bin/`（不分大小寫）時，會根據執行檔位置將 CWD 設成 `bin` 的上一層，從那裡尋找字型、shader、金鑰與狀態；不受啟動時的 CWD 影響。開發時直接執行未放進 `bin/` 的主程式，仍使用原本 CWD。`compare.exe` 與 `filegen.exe` 保留命令列啟動時的 CWD，方便使用相對檔案路徑。

每個空間各自保存登入狀態及 root；首次登入沒有 root 紀錄的空間時，才建立雲端根目錄。`root/<空間 ID>` 是同時包含 `account` 與 `id` 的 JSON；`account` 是 root blob 所在的服務帳戶 address。每個服務帳戶各有一份 `blob_queue/<address 的 SHA-256>`，多個空間關聯同一帳戶時共用該佇列，不混用其他帳戶的 ID。背景工作會為已登入空間的服務帳戶補充空白 blob。每個帳戶最多 64 個補充 batch 在途，每批 100 筆；每輪只送出 `min(64、可用名額、補足到至少 20,000 筆所需批數)`，所需批數先扣掉在途 batch 已涵蓋的 ID，剩餘缺口除以 100 後向上取整。例如佇列有 19,950 筆且沒有在途請求，只補一批 100 筆。最多只因整批取整而超出目標 99 筆。這是每輪及在途數量限制，不是每分鐘 64 批的限制。

空間可關聯不同專案的多個服務帳戶。上傳配置先比較專案的累計負載，再比較該專案內各服務帳戶的負載，交錯使用專案及服務帳戶，避免先集中使用某一專案。每個 file blob 的最多 100 個 chunk blob 位於同一服務帳戶；每個 chunk blob 的最多 100 個 content blob 也位於同一服務帳戶。負載是累計的邏輯寫入估算，包含上傳配置與背景已排入補充的 blob 建立量（包括在途請求）；一個邏輯寫入對應目前 50 個 Drive quota units。同一筆工作的 HTTP 重試不重複增加此配置估算，這不代表 Google 對重試或所有失敗請求都不計 quota。分配仍先比較專案，再比較帳戶；沒有按分鐘衰減。這是盡量分攤寫入量的策略，不保證不會觸及 Google 的每使用者／每專案配額。


blob 索引均明確保存跨帳戶路由：file name 的 `parent_account` 指向父 folder main blob 帳戶、`next_account` 指向下一頁帳戶、`account` 指向本頁所有 chunk 的帳戶；chunk name 的 `parent_account` 指向父 file main blob 帳戶、`account` 指向其所有 content 的帳戶；content name 保存 `parent_account`。folder name 保存 `parent_account` 與 `next_account`，description 的每個 object 保存其 main blob 的 `account`。沒有 parent／next／children 的欄位值使用字串 `"null"`。

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
- **目錄瀏覽**：上方路徑列可返回任一父目錄；根目錄顯示空間名稱。清單分成「檔名／大小／操作」三欄，窄視窗優先保留右側操作按鈕；長名稱、路徑及目錄清單均可捲動，支援橫向滾輪與 Shift＋滾輪。
- **目錄管理**：新增、移動、複製與刪除在背景執行，顯示進度並保存中斷恢復紀錄。剪下保留原有內容，複製建立獨立副本；刪除會永久移除項目及其子內容。目錄操作開始後不提供暫停或取消，網路失敗時可重試。
- **空間選單**：右上角固定顯示「空間」。各列依序為空間名稱、「登入」「登出」「編輯」「移除」，底部為「新增空間」。編輯清單顯示關聯帳戶的 address 前半部及「移除」，底部有「新增帳戶」及「返回」。所有層級均支援長清單捲動、縮放、Esc 與點選外側關閉。
- **登入與登出**：登入選取該空間瀏覽，其他已登入空間的傳輸繼續執行。登出保存並暫停該空間的未完成工作；再次登入後續傳。完整空間清單、關聯與登入狀態保存在 `settings.json`，重啟後恢復。
- **移除空間／服務帳戶關聯**：每次都顯示需輸入確認文字的對話框，不檢查帳戶是否仍存有 blob。移除空間只刪本機空間設定及 root 紀錄；雲端資料與金鑰保留，同名新空間不會自動找回資料。解除帳戶關聯只改本機設定；該帳戶的 blob 及依賴其索引的資料可能無法存取，重新關聯同一帳戶可恢復。兩者均登出該空間並保留未完成傳輸的檢查點。

### 5. 設定與清除

「設定」使用 lgo.GUI 表單，提供日誌資料夾、字型檔案、Google 使用者 OAuth 用戶端設定、root 紀錄資料夾（預設 `root`），網路失敗重試次數，以及「記錄 HTTP 錯誤碼」、worker 數量與 `batch_request` checkbox。次數可填非負整數或「無限」（檔案中以 `-1` 表示）；`0` 表示首次失敗即暫停，預設無限。按「取消」不套用，按「確定」驗證並在背景載入帳戶／根目錄，成功後寫入 CWD 的 `settings.json`。新設定立即生效，字型和日誌路徑也會更新；既有傳輸保留建立工作時的帳戶、根目錄與目的地，新操作使用新設定。每個帳戶的 blob 佇列獨立保存，避免混用 ID。表單支援 UTF-8 中文輸入、Backspace、Ctrl+A 全選及 Ctrl+V 貼上。OAuth 自身的三次嘗試與 HTTP 401 刷新上限仍獨立計算。

「記錄 HTTP 錯誤碼」可輸入 `403,500,503` 等 400–599 的狀態碼，以逗號或空白分隔；預設留空，表示停用。套用後立即影響所有帳戶與既有背景傳輸，並保存在 `settings.json` 的 `http_log_codes` 字串。符合的每次實際 HTTP 回應（含重試）會將 request／response 的標頭與完整 body 成對寫入目前日誌資料夾的 `.txt`；一般請求包含方法與 URL、程式設定的請求標頭及伺服器回應標頭。Google batch 即使外層為 200，仍逐一比對內層狀態碼，僅記錄符合的子請求與子回應，附上 batch URL、interface 與 Content-ID 供配對；外層本身出錯時則記錄整批。憑證標頭（含 DPoP）、Cookie、已知 OAuth 權杖／assertion／PKCE verifier／私鑰欄位會以 `[REDACTED]` 遮蔽；其他 body 內容原樣保留，可能包含檔名與檔案資料，啟用期間日誌也可能大幅增加。一般請求、batch 子請求、連線錯誤與服務帳戶權杖更新錯誤，會在日誌中附上 `account=<服務帳戶 address>`；以該請求實際綁定的帳戶為準，不依目前畫面選取的空間或預設帳戶推測。錯誤摘要及其原因會一起寫入，避免並行錯誤交錯後無法辨識歸屬。留空並套用即可停止詳細記錄；原有錯誤摘要與帳戶標記持續保留。

`batch_request` 預設勾選，task 使用 batch 傳輸；取消勾選後，task 改用逐筆 HTTP 請求。worker 數量獨立設定為正整數，預設 64，保存為 `settings.json` 的 `worker_count`；切換 batch 模式不會改動此數值。上傳、下載、複製、搬移、刪除及清除都依 `batch_request` 切換請求格式；folder blob 維持逐筆。`blob_queue` 背景補充始終使用原本的 batch。設定保存在 `settings.json` 的布林欄位 `batch_request`。切換只影響後續派送；已派出的請求保留原模式、服務帳戶、重試及進度，直到完成。增加 worker 數量會開放新的傳輸名額；降低數量不取消超出的在途請求，使用數降到新限制以下後才補送。worker 數量限制 task 共用的前景傳輸槽，不改動 `blob_queue` 每個帳戶的 64 個背景補充名額。拆開的同一組 blob 仍保留原本帳戶配置，每個成功的逐筆請求都有檢查點；暫停／續傳不重送已確認完成的請求。F3 分頁顯示所有已建立的傳輸槽，重用最多 64 個文字元件並僅更新目前頁面，避免大量文字元件拖慢畫面。

同一次程式執行使用啟動時決定的日誌檔名；套用設定、登入／登出或變更空間帳戶關聯不會另開新檔。只有變更日誌資料夾時才重新開啟日誌，並沿用相同檔名；切回原資料夾時接續原檔。設定視窗取消或套用後維持空間 dropdown 關閉；從空間選單執行的管理操作仍會回到對應選單。

設定更新、清除進度及目錄錯誤等紅色提示統一使用淺紅色 `std_red_light`。

「清除」會永久刪除目前空間**所有關聯服務帳戶中的全部 Drive 檔案**，包含預備、未連結及 root 以外的 blob。**如果其他空間共用這些服務帳戶，其檔案也可能被清除。** 確認視窗會明確提醒此範圍與共用空間的風險，必須輸入「清除」並按「清除全部」。已刪除的檔案無法復原。

確認後取消所有使用這些服務帳戶的傳輸、停止各帳戶的預備 blob 補充，完整列出全部帳戶檔案後才開始刪除。folder blob 使用單一請求；其他檔案在勾選 `batch_request` 時以每批最多 100 個、同一服務帳戶的 batch 刪除，取消勾選則逐筆刪除，404 重播視為成功。清單、帳戶與完成進度保存在 `clear.<空間 ID 的 SHA-256>`，中斷後可接續。完成後為目前空間建立新的空白 root，並為每個服務帳戶重建共用的預備佇列。共用帳戶的其他空間可能仍指向已刪除的資料，無法用原本的 root 恢復那些檔案。

進度區顯示進度及失敗原因，可重試或取消尚未完成的清除。取消收尾時顯示「取消中」，取消或完成後按「確定」移除結果列；移除結果列不會刪除未完成的續清紀錄。

程式入口除 `std::exception` 外也有 `catch (...)`，非標準 C++ 例外會記錄未知例外訊息並回報失敗。程序被直接終止時則依已落盤的檢查點恢復，不依賴例外處理器執行。

### 6. 關閉與保留狀態

傳輸完成後，使用視窗右上角的叉叉正常關閉程式。請保留以下檔案：

| 檔案 | 用途 |
| --- | --- |
| `key/` | 程式建立的服務帳戶私鑰及 `accounts.json` 登記檔。 |
| `client.json` | Google 桌面 OAuth 用戶端設定。 |
| `cloud_project/` | 尚未確定結果的專案建立 ID 與操作，重試使用同一 ID。 |
| `root/<空間 ID>` | 該空間 root blob 的 `account` 與 `id`。 |
| `blob_queue/<address 的 SHA-256>` | 每個服務帳戶獨立的預備 blob ID 清單。 |
| `settings.json` | 路徑、網路／日誌設定、`spaces` 清單與 `selected_space`。 |
| `task`、`task.data/` | 未完成工作、帳戶及 ID 分配、下載進度與成功回應快取。 |
| `clear.<空間 ID 的 SHA-256>` | 尚未完成的清除清單、帳戶與進度。 |

佇列在啟動時讀取並保留原檔，分配 ID 或補充後持續保存。工作在網路寫入前先記錄 blob ID，成功回應與下載寫入位置也持續保存；檢查點使用同目錄暫存檔、磁碟 flush 和原子取代。正常關閉、C++ 例外或直接終止程序後，下次啟動會自動恢復未完成的上下載，並使用各工作記錄的空間與服務帳戶身分。`task.data/` 會快取已收到的回應以重建排程，下載內容快取可能佔用接近檔案大小的額外空間；上下載完成或取消時，先將工作從續傳索引移除並保存，再由共用背景執行緒刪除快取，避免大量刪檔阻塞畫面。按「確定」移除結果列也不必等待刪檔；正常關閉時會等待背景清理完成。若清理遭中斷或檔案被外部程式鎖住，可能留下不在續傳索引中的快取，已完成的工作不會因此重新恢復。

手動暫停狀態也會先保存至工作紀錄；重開程式後仍維持暫停，按「繼續」才恢復。暫停時取消會移除續傳紀錄並停止後續工作，保留已下載的部分檔案；取消不會回滾已完成的雲端寫入。

上傳及下載會同步保存既有的各層 progress、完成旗標、索引頁及 ID；各檔案／資料夾的排程快照保存在工作目錄內的 `.progress` 檔案，進度更新不重寫整個任務的 ID 分配與路由表。恢復時直接跳過完成的檔案、content 批次及索引頁，重建尚未完成的排程；已送出但尚未確認成功的請求會重新排程，亂序完成的結果也會保留，不把 begin 計數當成成功。成功回應快取只用於尚未保存完成旗標的請求及下載尚未依序寫入的內容，不再從頭重播所有完成請求。

恢復下載會直接載入保存的檔案清單、索引、已寫入的位置及亂序收到的內容，保留磁碟上的完成前綴並截掉檢查點之後的尾端；已完成的部分不再下載或解碼，也不會再次刪除整個目的地。恢復上傳沿用原 blob ID，並檢查來源檔案大小、修改時間及資料夾清單；來源已改變時顯示失敗，需重新新增工作。工作紀錄直接採用目前格式，不偵測、轉換或相容舊版紀錄。發布目錄連結時會檢查項目 ID，避免閃退後重複加入。請保留原始來源、部分下載及服務帳戶的金鑰檔；`task` 必須和 `task.data/` 一併搬移或備份。

原子取代檢查點時，若其他程式短暫占用檔案，會在最多 2 秒內重試；持續失敗時保留原檢查點並記錄 Windows 錯誤碼。這不改變 HTTP 重試間隔。

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

`run_token_refresh.py --modes http-logging` 驗證錯誤碼設定、同步與背景 request／response 記錄、外層 200 的 batch 子回應篩選、重試配對、不同服務帳戶歸屬、handle 改綁帳戶、連線錯誤、停用及憑證遮蔽；`run_oauth_login.py` 另驗證登入 HTTP 錯誤記錄與 DPoP／PKCE 憑證遮蔽。

`run_token_refresh.py` 另以嚴格的 JSON 子請求驗證批次建立／更新、Host／自訂標頭保存、外層 200 內層 403／404 錯誤摘要、敏感訊息排除，以及無限重試退避與手動重設。它也會嚴格檢查 JWT bearer grant type 與 assertion，使用 OpenSSL 產生一次性的測試金鑰，僅將測試副本的 OAuth endpoint 指向本機模擬服務，其餘使用正式帳戶、簽署、快取與請求實作。測試涵蓋首次背景刷新、`expires_in` 到期、16 個並行及延遲 401、POST／PATCH 重送、批次部分成功與內層 401 後的格式／503 錯誤、同步 easy／multi 重試、持續 401 的上限、OAuth 暫時／永久失敗、異常回應，以及 blob 背景錯誤回報及關閉時的授權等待。所有憑證、token 與佇列均為暫存測試資料，不讀取正式金鑰或呼叫雲端服務。

`python3 tests/run_reserve_refill.py` 使用本機延遲回應驗證每帳戶最多 64 個在途補充 batch、接近 20,000 筆時只補足向上取整的缺口、計入在途 ID 而不重複補送，以及從空佇列補滿 20,000 筆。另注入單一子請求 503，檢查只重送失敗部分、重試不重複增加配置估算，並確認背景補充會影響後續的專案與帳戶選擇。

`python3 tests/run_batch_mode.py` 以本機延遲回應驗證 200 個同時在途的逐筆請求、6400 個傳輸槽、傳輸中雙向切換而不重建 worker、僅重送單一失敗請求、逐筆回應檢查點、暫停／繼續與已完成回應的暫停競態，以及傳輸中調高／調低 worker 數量、13 與 64 個名額的派送限制；另用 `xdotool` 操作 worker 欄位與 checkbox，驗證無效數值、取消、保存、兩項設定互相獨立及 F3 分頁。

`python3 tests/run_spaces.py`（另加 `--direct` 驗證逐筆請求，`--workers 7` 可驗證自訂 worker 數量；逐筆測試未指定數量時使用 6400）使用四個合成服務帳戶、兩個專案、兩個共用帳戶的空間及本機 Drive 模擬服務，檢查每服務帳戶的佇列、專案／帳戶交錯配置、上傳程序中斷與續傳、下載內容相同、遞迴複製後刪除來源、搬移、跨頁目錄、101 個 chunk／100 個 content 的索引複製與帳戶欄位，以及清除全部關聯帳戶檔案與中斷接續。伺服器拒絕混合服務帳戶的 batch、超過 100 個子請求及任何放入 batch 的 folder blob，另注入反序子回應與 GET／PATCH／DELETE 的部分 503 失敗。未關聯帳戶的資料須保留；關聯帳戶中 root 以外的資料也須清除。

`python3 tests/run_progress_resume.py` 實際中斷上下載程序，恢復時移除不再需要的成功回應快取，並由伺服器拒絕再次處理已保存完成的 content、chunk 及 file／folder 索引。測試也涵蓋空檔案與空目錄、下載亂序完成但尚未寫入的內容，以及截掉未保存的輸出尾端。`python3 tests/run_checkpoint_recovery.py` 檢查短暫的 Windows 讀取占用，以及含 60,000 個 content 路由和 576 個檔案的大型準備流程不重寫相同的任務狀態；兩者都使用暫存合成資料，不接觸真實雲端。

`python3 tests/run_task_cleanup.py` 在真實 GUI 中完成下載與上傳，各使用 30,000 個合成回應快取檔，檢查收尾延遲、清理期間畫面持續更新、立即移除結果列、完成索引保存、遲到寫入不重建快取，以及清理不影響其他未完成工作；正常結束後快取必須刪除完畢。測試不使用帳戶或雲端資料。

`python3 tests/run_spaces_gui.py` 使用 `xdotool` 實際點擊正式程式入口的空間選單與名稱／確認視窗，驗證空間、使用者照片與 email、專案 ID／狀態篩選、具名建立專案、服務帳戶名稱驗證、只列程式建立的帳戶、關聯、登入／登出及重啟保存；兩種移除都檢查錯誤確認文字、取消與正確確認，並驗證移除不刪除 Cloud 資源或私鑰。也開啟並取消清除警告；另外驗證設定取消／套用後不彈出空間選單，同一次執行在套用設定與空間操作後沿用日誌，變更日誌資料夾保留檔名並可切回原檔接續。OAuth 協定沿用本機 PKCE／DPoP／nonce 模擬服務，僅在測試副本替換 Wine 尚不支援的 Windows CNG KSP。原生 Windows KSP 與真實 Google 授權仍需在 Windows 上驗證。這兩組測試只使用一次性設定、金鑰與佇列，不讀正式憑證或呼叫 Google。

舊的 `run_oauth_login.py`、`run_task_lifecycle.py`、`run_gui_settings.py` 與 `run_directory_operations.py` 場景仍保留作為歷史回歸資料；其單帳戶設定與舊 blob fixture 不適用新的空間格式，目前以以上兩組測試驗證空間功能。

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
