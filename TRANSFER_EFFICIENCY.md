# 單一服務帳戶傳輸效率調查

## 2026-11-11：固定 content 容量與片內 zstd

content 一律使用 Base64URL，每片最多 18,750 原始 bytes，完整 description 為 25,000 ASCII chars／UTF-8 bytes。保留預設關閉的 `content_zstd`，只在固定切片內壓縮，不擴大容量。本次沒有呼叫 Google，沒有新增節流，也沒有變更並行、重試、帳戶／專案配置、Cloud／服務帳戶程式或 blob_queue refill。

| 新上傳設定 | 固定原始 bytes / content 邏輯請求 | 完整 description |
| --- | ---: | --- |
| `content_zstd=false` | 18,750 | 25,000 ASCII chars / UTF-8 bytes |
| `content_zstd=true` | 18,750，不擴大切片 | 壓縮有利時縮短；否則保存 raw |

zstd level 3 在固定切片內壓縮，只有 compressed < raw 才存 `c=zstd` 與 `r=raw bytes`，再以 Base64URL 編碼。原始切片邊界不變，壓縮輸出必定能放入同一 content blob。解壓前限制原始長度，驗證單一完整 frame、無尾隨內容及精確輸出大小。**目前 zstd 只節省 payload／wire bytes，不減少請求；合用 batch gzip 的收益須實測。** 若要利用壓縮減少請求，需新增持久化的逐片 offset／raw-length manifest，先準備可重播的可變切片，並以它取代所有 seek、配置及 checkpoint 算法；這超過此次固定切片設計，未實作。

每個新任務在同一 intent 記錄內保存有效的 `content_zstd`，先落盤才開始準備。上傳預配置、chunk 大小、content 數量及 seekg 都使用既有 `content_size_max_base64=18750` 常數，進度與續傳仍以原始 bytes 計算。舊 journal 無欄位時不壓縮；設定切換不影響既有任務。下載每個 content GET 取 `name,description`，依壓縮標記解碼，不增加 GET 次數；未標記即 Base64URL／未壓縮，帶 `e` 的 blob 明確回報不支援編碼。亂序回應 checkpoint 重播走相同解碼器，舊 description-only 快取仍可讀。目錄複製既有的 name 複製流程會保留壓縮標記。

batch PATCH 與逐筆 PATCH 都明確使用 `application/json; charset=UTF-8`；curl body 與 Content-Length 沿用 bytes。content 的 Base64URL payload 檢查上限為 25,000 bytes；上層 folder／chunk JSON 索引維持既有保守大小檢查。

設定表單沿用文字驗證／checkbox／取消／背景套用；預設 `content_zstd=false`。移除編碼欄位後，`batch_request`／`batch_gzip`／`content_zstd` 對應 `values[8]`／`values[9]`／`values[10]`。CMake 可選 `find_package(zstd QUIET CONFIG)`，找不到可正常建置；GUI 停用壓縮選項，手動 true 對新任務無效並寫日誌。已有 zstd 的下載或任務續傳會明確報需要支援 zstd 的建置。MSYS2 `E:/msys64/clang64` 已有開發標頭，未執行 pacman；將 zstd 公開標頭複製到建置目錄並僅繼承套件的連結相依，避免其 CRT 標頭干擾 LLVM-MinGW。可用 `-DCMAKE_DISABLE_FIND_PACKAGE_zstd=TRUE` 測停用路徑，發布須附 libzstd.dll。

離線測試 target：`-DLGO_BUILD_CONTENT_TESTS=ON`，執行 `ctest --test-dir <build> -R content-codec --output-on-failure`；使用實際 storage.encode／storage.state 模組。涵蓋 0–200 bytes、18,750 bytes 滿容量的隨機／重複資料、JSON 往返、舊格式、zstd 標記與 raw fallback，及超量切片、不支援編碼、非法長度、截斷／損壞／尾隨壓縮資料。另在全新隔離目錄測試設定落盤／重讀、任務紀錄重啟還原及切換設定後仍保留原壓縮選項。

本機驗證：移除編碼選項後，指定 `rebuild.ps1` 完整成功，已安裝至 `E:/lingogoogol-runtime/bin/storage.exe` 並打包 29 個 DLL。有 zstd 的 codec 測試通過（CTest 1/1），`git diff --check` 通過。首次 install 遇到既有執行中的程序（PID 43808）鎖住執行檔，將舊檔改名保留為同目錄的 `storage.running-43808.exe` 後重新執行即成功；既有程序持續執行，沒有啟動新版程式。本次未重測無 zstd 建置。

實際驗收仍需壓縮關／開，上傳→下載→`compare.exe` 逐位元組比較，涵蓋空檔、最後不足片、跨 100-content chunk 的檔案、暫停／重啟及設定切換續傳，確認 name 壓縮標記與內容完整；本次無 Google／憑證操作。

分支：`perf-transfer`；原始調查基準：`c0144333191d50b3be63aa96473bbcdc56b193e1`。本次加入可選的 batch gzip 與前景 batch 大小；預設 `batch_gzip=false`、`batch_size=100`。沒有修改 Cloud／帳戶／金鑰／OAuth 管理、跨帳戶配置與路由、背景佇列補充數量、worker 名額、重試上限或派送節流。

## Batch 請求 gzip：可選的 zlib 建置相依

先前只檢查本機缺少開發相依，便停止實作，沒有充分考慮作者使用 MinGW 的環境。此處依作者提供的環境資訊修正：MSYS2／MinGW libcurl 依賴 zlib，其工具鏈通常已有 zlib 標頭及 libz；本機沒有工具鏈不能據此推定作者環境缺少 zlib。

- [external/curl/CMakeLists.txt](external/curl/CMakeLists.txt) 以 `$<IF:$<BOOL:${MINGW}>,curl,libcurl>` 區分連結名稱；[storage/CMakeLists.txt](storage/CMakeLists.txt) 從 `CMAKE_FIND_ROOT_PATH/bin/libcurl.dll` 安裝 MinGW DLL，這些都是目標工具鏈的線索。
- [batch CMake](storage/google/batch/CMakeLists.txt) 現在呼叫 `find_package(ZLIB)`；找到時僅對 `storage.google.batch` 連結 `ZLIB::ZLIB`，並設 PRIVATE `LGO_HAVE_ZLIB=1`。找不到時仍能建置，沒有強制新增相依。可用 `-DCMAKE_DISABLE_FIND_PACKAGE_ZLIB=TRUE` 明確測試停用情境；自訂路徑可使用 `ZLIB_ROOT`，交叉編譯需確認找到的是 Windows 目標函式庫。[CMake FindZLIB 文件](https://cmake.org/cmake/help/latest/module/FindZLIB.html)
- `zlib.h` 僅在 batch 實作單元的 global module fragment 條件式 include，不匯出 C API；GUI 透過匯出的 `google_batch_t::gzip_available()` 查詢能力，不依賴其他 target 的編譯巨集。動態連結時須發布對應的 zlib DLL。

curl 內部能處理 gzip 回應，不代表程式可直接呼叫其內部 zlib。[CURLOPT_ACCEPT_ENCODING](https://curl.se/libcurl/c/CURLOPT_ACCEPT_ENCODING.html) 用於協商及解壓回應；[curl_mime_encoder](https://curl.se/libcurl/c/curl_mime_encoder.html) 支援的 MIME 編碼也不包含 gzip。現有 [external.curl](external/curl/.cppm) 沒有提供整個 HTTP 請求 body 的 gzip 編碼介面，因此不能靠新增標頭或這些選項完成本次需求。

`settings.json` 新增布林 `batch_gzip`，預設關閉。表單在 `batch_request` 後加入 checkbox，沿用取消／驗證／背景套用流程；無 ZLIB 時顯示原因並停用，手動設定為 true 也會忽略，啟動及套用設定時寫入原因日誌。程序以 atomic 保存新派送使用的開關，每次 `method_post()` 擷取一次，既有請求的編碼不因設定切換而改變。

### 編碼、重試與日誌

[google_batch_t](storage/google/batch/.cpp) 的 `method_post()` 先產生 multipart body，再交給 curl read callback 傳送。`resume()` 會排除已成功的子請求並重新產生 body；部分成功後的下一次嘗試不能沿用前一次整批的壓縮資料。相同待送內容的傳輸重試、seek 及暫停／續傳則必須從壓縮資料的開頭重新讀取。

壓縮只套用至 `POST https://www.googleapis.com/batch/drive/v3`，包含原本的背景補充 batch；逐筆模式、其他 endpoint 不套用。使用 `deflateInit2(..., 1, Z_DEFLATED, 15+16, 8, Z_DEFAULT_STRATEGY)`，以 gzip wrapper 及 level 1 編碼，循環輸出至 `Z_STREAM_END`，RAII 清理 zlib 狀態；過大的輸入會回報錯誤，避免 uInt 截短。[zlib 手冊](https://zlib.net/manual.html)

原 multipart Content-Type／boundary 保留，壓縮時設定 `Content-Encoding: gzip` 與壓縮後 Content-Length。curl 的 POST 建立及暫停後 `bind_transport()` 都設定 `CURLOPT_POSTFIELDSIZE_LARGE`；batch 重試重新編碼後也更新長度，read callback 與 seek 始終操作保留的二進位 wire buffer。[libcurl 長度選項](https://curl.se/libcurl/c/CURLOPT_POSTFIELDSIZE_LARGE.html)

[curl_easy_t](storage/http/curl/easy/.cpp) 新增 protected virtual `request_body()`，讓日誌及回應快取鍵取得邏輯 body；batch 回傳未壓縮 multipart，逐筆回傳原本的 body。既有未壓縮快取鍵保持不變，gzip 開關不會改變成功回應的識別。外層日誌保留實際 `Content-Encoding: gzip`／壓縮後長度，但 body 顯示未壓縮內容；batch 子請求的日誌標頭另註記 `gzip`。憑證遮蔽流程照常使用可讀的原始內容。

## 前景 batch_size：只拆 HTTP envelope

`settings.json` 新增整數 `batch_size`，預設 100、合法範圍 1–100；設定表單放在 worker 數量之後。載入 JSON 先取原始表示再用 `from_chars` 驗證，避免負數轉無號或小數被截短；保存時也驗證範圍。

[request.cpp](storage/kernel/load/request.cpp) 將原本逐筆模式的拆分擴充成連續子範圍：同一組上下載 blob 仍沿用原本帳戶、ID、chunk／content 分組與完成回呼，僅以最多 `batch_size` 筆形成 HTTP batch。每個成功 envelope 由既有 worker 保存回應；全部完成後合併回原本的索引、保存整組回應，再觸發原有進度保存／下載寫入。這也保留下載亂序內容使用的整組 `checkpoint_key()`。程序內暫停保留已建立的子範圍，重新啟動後相同設定可重播已保存的 envelope；若在整組尚未完成時改變大小或模式後重啟，新的 envelope 邊界可能無法命中舊的部分回應快取，尚未標記整組完成的 blob 可能重送，沿用同一 ID。已完成整組的快取與進度不受影響。

[directory.cpp](storage/kernel/load/directory.cpp) 的複製／搬移／刪除只調整每個同帳戶 batch 收集的筆數，保留既有進度保存頻率；[runtime](storage/kernel/runtime/.cpp) 的清除依設定形成同帳戶 envelope，每個 envelope 完成後照常落盤。GUI 的目錄待送視窗也按 `worker_count * batch_size` 計算，既有 worker 名額與派送時間界線不變。`run_clear_if_full()` 全庫沒有呼叫點，因此保留這個通用 helper 的 100 上限；實際派送走上述三個入口。`blob_queue` 程式碼與每批 100 筆的補充行為完全未改。

設定套用於後續建立的 envelope，已派出的請求保留原編碼、大小及重試狀態。類似 Python 用戶端曾量測到每批 8 筆、64 個並行 worker 比更大的 batch 快，這只是調校參考，不能當作本 C++ 專案已實測的結論。作者應固定網路、資料與 worker 數量比較 1／8／100；預設仍是 100。較小批次會增加外層 HTTP 次數，Drive 邏輯請求數及既有並行名額不變。

## 下載與目錄瀏覽：保留現有讀取流程

### 現有請求數

以下以預設 `batch_size=100` 計算：單一服務帳戶、全新下載、正常由現有上傳流程產生的索引，排除 OAuth、重試、已保存回應重播及另行開啟目錄的請求。HTTP 數量計算 batch 的外層 POST 一次；Drive 邏輯 GET 數量另外計算其子請求。gzip 只影響位元組數，不改變請求數。

本專案有兩層分塊，不能把 `chunk` 與實際保存 Base64URL 資料的 `content` blob 混為一談：

- `N`：`download_chunk_t` 的數量，每個 chunk 索引最多指向 100 個 content blob。
- `C`：所有 content blob 的總數，每個保存最多 18,750 bytes 的原始資料。
- `P = max(1, ceil(N / 100))`：file 索引頁數；空檔案也有一頁。
- `B = ceil(N / 100)`：chunk 索引 GET 的 batch 數；空檔案為 0。

| 階段 | 現有讀取方式 | batch 開啟時的 HTTP 次數 | 邏輯 GET 次數 |
| --- | --- | --- | --- |
| 準備檔案大小 | `file_download()`：main file 的 `GET fields=name` | 1 | 1 |
| file 索引 | `download_file_load_begin()`：依 `next` 逐頁 `GET fields=name,description`，包含再讀一次 main file | P | P |
| chunk 索引 | `download_chunk_load_begin()`：每批最多 100 個 `GET fields=name,description` | B | N |
| content 資料 | `download_chunk_transfer_begin()`：每個 chunk 一批，含其全部 `GET fields=name,description` | N | C |

因此 batch 開啟時是 **`1 + P + B + N` 個 HTTP 請求**；關閉時是 **`1 + P + N + C` 個 HTTP 請求**。兩者邏輯 GET 數量相同。相關實作位於 [storage/kernel/load/.cpp](storage/kernel/load/.cpp)；逐筆模式的拆分位於 [request.cpp](storage/kernel/load/request.cpp)。

| 範例 | N（chunk 索引） | C（content 資料） | batch HTTP：修改前 → 修改後 | 逐筆 HTTP：修改前 → 修改後 |
| --- | --- | --- | --- | --- |
| 空檔案 | 0 | 0 | 2 → 2 | 2 → 2 |
| 8 個資料分塊 | 1 | 8 | 4 → 4 | 11 → 11 |
| 201 個資料分塊 | 3 | 201 | 6 → 6 | 206 → 206 |
| 201 個完整 chunk | 201 | 20,100 | 208 → 208 | 20,305 → 20,305 |

若問題中的「N chunks」指最底層資料分塊，則令資料分塊數為 `M`，先代入 `C=M`、`N=ceil(M/100)`，再使用上式。以上是程式碼推導，沒有在本機執行下載量測。

當 `batch_size=S<100` 時，原本每個含 g 筆子請求的 HTTP batch 會拆成 `ceil(g/S)` 個；不跨原本 chunk 群組合併。例如 201 個 content 分為 100／100／1，S=8 時 content envelope 共 13+13+1=27 個，加上準備、file 索引及 chunk 索引各一次，共 30 次 HTTP。Drive 邏輯 GET 數不變。

目錄內容由 `list_folder_content()` 沿 folder blob 的 `next` 串列讀取：**D 個索引頁就是 D 次逐筆 `GET name,description`**，其 description 已包含每個項目的 ID、帳戶、類型、名稱及檔案大小；顯示每列不再逐一 GET 子檔案。[GUI 的 `list()`](storage/kernel/GUI/.cpp) 為顯示路徑，另對目前資料夾至根目錄之間的每個非根節點各送一次 `GET name`。深度為 d、根為 0 時，一次瀏覽合計 **`D + d` 次 HTTP**，修改前後相同。

整個資料夾下載則由 `folder_download()` 遞迴讀取每個資料夾的所有索引頁，再為每個檔案執行上述流程；不包含 GUI 路徑顯示所用的 GET。[progress.cpp](storage/kernel/load/progress.cpp) 會恢復檔案清單、索引與成功回應，因此續傳通常少於上述全新下載的請求數。

### 不實作 files.list 預取的原因

[blob 建立程式](storage/google/drive/blob/.cpp) 只寫入 Drive 的 `name` 與 `description`；使用 appdata 時，所有 blob 的實體 parent 都是 `appDataFolder`，其餘不指定實體 parent。邏輯 file／folder／chunk 的 `parent` 存在 **Drive name 字串內的 JSON 欄位**，不是 Drive 的 `parents`。後續更新也只改 name／description，沒有設定 `appProperties`。

因此，即使只有一個服務帳戶，不同檔案、索引、root 與預備 blob 仍共用實體空間。`q="'<邏輯父 ID>' in parents"` 無法取回其邏輯子節點；列舉帳戶根目錄或 appDataFolder 則會混入其他檔案與預備 blob。[Drive 搜尋規則](https://developers.google.com/workspace/drive/api/guides/ref-search-terms) 的 `name contains` 只做前綴比對，不能當作 JSON `parent` 欄位的子字串查詢；`fullText` 搜尋也不能保證等同該欄位的精確比對。

部分 content 名稱的序列化確實以 `parent` 開頭，可以構想用完整 JSON 前綴篩選；但 chunk 名稱先包含 `account`，不能通用地採用相同查詢。單帳戶下也可構想比對整個序列化 name；那會依賴完整欄位、空白與鍵排序，並非本次要求的實體 parent／appProperties 查詢。既有資料沒有宣告這種名稱查詢契約，而預設每個 content 群組已只占一個 batch，因此本次不加入依賴 JSON 字串格式的預取。

[files.list](https://developers.google.com/workspace/drive/api/reference/rest/v3/files/list) 可將 pageSize 設為 1000 並取得 description，但伺服器仍可能提早分頁，必須處理 nextPageToken；它不提供「整個邏輯父節點必定一次取完」的保證。在缺少精確篩選欄位的資料模型下，掃描整個帳戶會下載無關內容，且沒有可按檔案大小估算的請求數上限。預設每個 chunk 的 content 已經只用一個 batch，換成 list 也不保證減少 HTTP 次數。

既有索引提供明確 ID 與陣列順序，batch parser 會檢查 Content-ID、重複／缺漏回應，下載時也檢查索引筆數、Base64URL 解碼及最終檔案大小；但沒有遠端檔案內容 hash。`storage_digest()` 用於本機檢查點／快取鍵，不是端對端內容校驗。不能把 list 回傳順序當成檔案順序，也不能將搜尋結果直接當作完整索引。

依本次範圍，保留現有以 ID 讀取的流程，沒有加入 list 或 fallback；預設批次大小下的下載及瀏覽請求數不變。

## 作者環境需要確認的項目

以下是先前 batch gzip／batch_size 調查留下的驗收項目；當時僅做來源碼與靜態檢查。本次已在 MSYS2／LLVM-MinGW 完成編譯及上述離線測試，沒有執行 Google 網路整合測試。仍需確認：

- 有／無 ZLIB 兩種 configure 與 module 編譯／連結；無 ZLIB 時 checkbox 停用、JSON 的 true 被忽略並有日誌說明，動態建置確認 zlib DLL 可載入。
- gzip 關／開各上傳後下載並逐位元組比對，另測逐筆模式不帶 gzip 標頭；在設定套用前已派出的請求及其重試維持原編碼。
- `batch_size=1/8/100`，檢查每個前景 envelope 的 Content-ID 筆數與資料完整性；複製／搬移／刪除及清除也遵守上限，背景補充仍 100 筆。JSON／表單拒絕 0、101、負數、小數、字串及溢位，取消不保存。
- 上傳中途暫停／繼續及關閉／重啟續傳，含同一 chunk 內部分小批次已完成、下載亂序完成；原 ID、成功 envelope 快取及最終內容保持正確。
- 注入部分成功、內層 401、連線失敗並重試，驗證 gzip 可解壓、boundary 完整、Content-Length／POSTFIELDSIZE 等於 wire bytes，且僅重送未成功子請求；更換／重用 handle 後長度正確。
- `http_log_codes=200,401,403,503`，確認外層及內層 request／response 配對、gzip 註記、可讀未壓縮 body、憑證遮蔽；關閉 gzip 後沒有遺留編碼標頭。
