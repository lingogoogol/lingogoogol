$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$failures = [System.Collections.Generic.List[string]]::new()

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        $failures.Add($Message)
    }
}

$trackedText = git -C $repo grep -n -I -e 'BEGIN PRIVATE KEY' -- . 2>$null
Assert-True ($LASTEXITCODE -eq 1) 'A private key is still tracked in the repository.'

$engineInterface = Get-Content -Raw (Join-Path $repo 'lgo/GUI/engine/.cppm')
Assert-True ($engineInterface -match '^export module lgo\.GUI\.engine;') 'The GUI engine interface is not declared as a module interface.'

$engineImplementation = Get-Content -Raw (Join-Path $repo 'lgo/GUI/engine/.cpp')
Assert-True ($engineImplementation -match '^module lgo\.GUI\.engine;') 'The GUI engine implementation is not attached to its module.'
Assert-True ($engineImplementation -notmatch '\b(drawFence|m_queue|swapChainExtent|vertexInputInfo|allocInfo|dst_stage_mask|dst_access_mask|ppQueueCreateInfos)\b') 'The Vulkan engine contains stale, misspelled, or undeclared migration identifiers.'
Assert-True ($engineImplementation -cnotmatch '\beCounterclockwise\b') 'The Vulkan front-face enum uses incorrect capitalization.'
Assert-True ($engineImplementation -notmatch '\b(Microsoft::WRL|ID3D12|InvalidateRect|TrackMouseEvent)\b') 'The Vulkan engine still contains active DirectX/Win32 implementation code.'
Assert-True ($engineImplementation -notmatch '\b(VKAPI_ATTR|VKAPI_CALL|VkDebugUtils|VK_DEBUG_UTILS|VK_QUEUE_FAMILY_IGNORED|VK_EXT_DEBUG_UTILS_EXTENSION_NAME|VK_KHR_SWAPCHAIN_EXTENSION_NAME|VK_NULL_HANDLE)\b') 'The engine uses Vulkan C macros or types that are not exported by the named module.'
Assert-True ($engineImplementation -match 'glfwSetCursorPosCallback' -and $engineImplementation -match 'glfwSetKeyCallback') 'GLFW input callbacks are not connected.'
Assert-True ($engineImplementation -match 'm_retired\.emplace') 'Callbacks removed during dispatch are not retired safely.'
Assert-True ($engineImplementation -notmatch 'PhysicalDeviceExtendedDynamicStateFeaturesEXT') 'The engine still requires an unnecessary Vulkan extension feature.'
Assert-True ($engineImplementation -notmatch 'Vulkan_version_encode\(0, 1, 4, 0\)') 'The engine unnecessarily requires Vulkan 1.4.'
Assert-True ($engineImplementation -match 'else[\s\S]*?extent = surface_capability\.currentExtent;[\s\S]*?m_swap_chain_extent = extent;') 'The swap-chain extent is assigned between the if and else branches.'
$acquirePosition = $engineImplementation.IndexOf('acquireNextImage')
$resetFencePosition = $engineImplementation.IndexOf('resetFences')
$submitPosition = $engineImplementation.IndexOf('.submit(')
Assert-True ($acquirePosition -ge 0 -and $resetFencePosition -gt $acquirePosition -and $submitPosition -gt $resetFencePosition) 'The draw fence is reset before swap-chain image acquisition succeeds.'

$devInterface = Get-Content -Raw (Join-Path $repo 'lgo/dev/.cppm')
Assert-True ($devInterface -match '^export module lgo\.dev;') 'The lgo.dev umbrella module is empty or invalid.'

$versionInterface = Get-Content -Raw (Join-Path $repo 'lgo/math/ver/.cppm')
$versionImplementation = Get-Content -Raw (Join-Path $repo 'lgo/math/ver/.cpp')
Assert-True ($versionInterface -match '^export module lgo\.math\.ver;') 'The version module interface and implementation files are reversed.'
Assert-True ($versionImplementation -match '^module lgo\.math\.ver;') 'The version module implementation is not attached to its module.'

$errorInterface = Get-Content -Raw (Join-Path $repo 'lgo/dev/error/.cppm')
$errorImplementation = Get-Content -Raw (Join-Path $repo 'lgo/dev/error/.cpp')
Assert-True ($errorInterface -match 'auto what\(\) const noexcept -> const char\* override;') 'error_t does not correctly override std::exception::what().' 
Assert-True ($errorImplementation -notmatch '::what\([^)]*\).*\boverride\b') 'An out-of-class function definition contains the illegal override specifier.'
Assert-True ($errorImplementation -notmatch 'internal_error_t::internal_error_t\([^)]*=') 'The internal-error definition repeats a default argument.'

$legacyError = Get-Content -Raw (Join-Path $repo 'compilercpp/lib/error.hpp')
Assert-True ($legacyError -match 'auto what\(\) const noexcept -> const char\* override') 'The legacy storage error type hides std::exception::what(), losing useful worker failure details.'

$stringCMake = Get-Content -Raw (Join-Path $repo 'lgo/container/str/CMakeLists.txt')
Assert-True ($stringCMake -match 'target_link_libraries\s*\(\s*lgo\.container\.str\b') 'String-module dependencies are attached to the wrong CMake target.'
$stringImplementation = Get-Content -Raw (Join-Path $repo 'lgo/container/str/.cpp')
Assert-True ($stringImplementation -notmatch '\b(CP_UTF8|MB_ERR_INVALID_CHARS|WC_ERR_INVALID_CHARS)\b') 'The string module uses Windows macros that cannot cross a module boundary.'
Assert-True ($stringImplementation -match 'output_size <= 0') 'String conversion failures are silently ignored.'

$mathOpCMake = Get-Content -Raw (Join-Path $repo 'lgo/math/op/CMakeLists.txt')
Assert-True ($mathOpCMake -match 'target_link_libraries[\s\S]*?lgo\.math\.vec') 'The math operation module does not declare its vector-module dependency.'

$fileCMake = Get-Content -Raw (Join-Path $repo 'lgo/io/file/CMakeLists.txt')
Assert-True ($fileCMake -match 'target_link_libraries[\s\S]*?lgo\.dev\.error') 'The file module does not declare its error-module dependency.'
Assert-True ($fileCMake -match 'target_link_libraries[\s\S]*?external\.Windows') 'The file module does not declare its Windows-module dependency.'

$fileImplementation = Get-Content -Raw (Join-Path $repo 'lgo/io/file/.cpp')
Assert-True ($fileImplementation -notmatch 'create_(?:i|f)stream\([^)]*=[^)]*\)') 'File-stream definitions repeat default arguments from their declarations.'
Assert-True ($fileImplementation -match 'reinterpret_cast<char\*>\(out\.data\(\)\)') 'Binary file reads pass an unsigned-byte pointer to std::istream::read.'
Assert-True ($fileImplementation -match 'end_position < 0') 'Invalid file sizes can be converted into huge buffer allocations.'

$streamImplementation = Get-Content -Raw (Join-Path $repo 'lgo/io/stream/.cpp')
$streamInterface = Get-Content -Raw (Join-Path $repo 'lgo/io/stream/.cppm')
Assert-True ($streamImplementation -notmatch 'for \(std::uint8_t i\{ 0 \}; i < (len|in\.size\(\));') 'String serialization still uses an overflowing 8-bit loop counter.'
Assert-True ($streamImplementation -match 'length_error') 'Oversized length-prefixed strings are not rejected.'
Assert-True ($streamInterface -notmatch '#include\s+[<"]header\.hpp[>"]') 'The stream module includes a nonexistent local header.'

$storageImplementation = Get-Content -Raw (Join-Path $repo 'storage/storage.cpp')
Assert-True ($storageImplementation -notmatch 'do\s*\{[\s\S]*?\}\s*while \(response_code / 100 != 2\)') 'HTTP requests still retry every non-2xx response forever.'
Assert-True ($storageImplementation -match 'max_attempts') 'HTTP retries do not have a finite attempt limit.'
Assert-True ($storageImplementation -notmatch 'QueryInterface\([^)]*\)[^{]*\{\s*throw;') 'The file dialog still terminates from QueryInterface.'
Assert-True ($storageImplementation -match 'HRESULT_FROM_WIN32\(ERROR_CANCELLED\)') 'Cancelling the file dialog is not handled normally.'
Assert-True ($storageImplementation -match 'font_face\{\s*font_path\.string\(\),\s*static_cast<std::uint64_t>\(std_font_size\.x\)\s*\}') 'Storage ignores the standard UI font size and renders unreadably small text.'
Assert-True ($storageImplementation -match 'add_object_shared<text_line_std_input_t>[\s\S]*?size_1D\{\s*0x180\s*\}') 'The clear confirmation input has no fixed clickable width.'
Assert-True ($storageImplementation -match 'CoInitializeEx\([^;]*COINIT_APARTMENTTHREADED') 'The upload dialogs can run before COM is initialized.'
Assert-True ($storageImplementation -match 'CoUninitialize\(\)') 'The storage application does not release its COM apartment.'
Assert-True ($storageImplementation -match 'path_from_utf8\(obj_ptr->m_name\)') 'Upload jobs reinterpret UTF-8 Windows paths using the active ANSI code page.'
Assert-True ($storageImplementation -notmatch 'std::filesystem::u8path') 'Storage still uses the deprecated MSVC UTF-8 path conversion that falls back to the active code page.'
Assert-True ($storageImplementation -match 'storage worker failed:') 'An exception in an upload or delete worker still terminates the whole application.'
$backgroundWorkers = [regex]::Match($storageImplementation, 'auto upload\(loading_list_t\*[\s\S]*?auto upload_guard\(').Value
Assert-True ($backgroundWorkers -notmatch '(?:add_object|get_object_shared|set_text)') 'A storage background worker still mutates DirectX UI objects and can crash the graphics driver.'
Assert-True ($storageImplementation -match 'auto refresh_progress_ui\(') 'Queued storage work has no main-thread progress renderer.'
Assert-True ($storageImplementation -match 'refresh_progress_ui\(uploading_list') 'The main message loop does not refresh upload progress.'
Assert-True ($storageImplementation -match 'std::atomic_uint64_t m_size') 'Progress totals are read and written across threads without atomic synchronization.'
$progressRenderer = [regex]::Match($storageImplementation, 'auto refresh_progress_ui\([\s\S]*?\n\}').Value
Assert-True ($progressRenderer -notmatch 'm_progress_text->set_text') 'Progress refresh repeatedly rebuilds the full label and can crash the DirectX driver.'
Assert-True ($storageImplementation -match 'std::shared_ptr<rect_t> m_progress_fill') 'Storage progress does not use a stable rectangle primitive.'
Assert-True ($storageImplementation -match 'std::shared_ptr<text_line_std_t> m_progress_percent_text') 'Storage does not display a numeric completion percentage.'
Assert-True ($storageImplementation -match 'm_last_ui_percent == percent') 'Storage progress is not coalesced to integer-percent updates.'
Assert-True ($progressRenderer -match 'm_progress_percent_text->set_text\([^;]*std::to_string\(percent\) \+ "%"\)') 'Storage does not update the visible 0-100% completion number.'
Assert-True ($progressRenderer -match 'm_progress_fill->set_size') 'Storage progress does not visibly resize its proportional fill.'
Assert-True ($progressRenderer -match 'progress_div->space_2D\(std_margin\)\.x') 'Storage progress uses a fixed width instead of the available panel width.'
Assert-True ($storageImplementation -match 'CURLOPT_XFERINFOFUNCTION') 'Storage only updates upload progress after an entire multipart batch finishes.'
Assert-True ($storageImplementation -match 'm_loading->m_progress\.store') 'The cURL transfer callback does not publish in-flight upload progress.'
$deleteProgress = [regex]::Match($storageImplementation, 'else if \(!delete_id\.empty\(\)\)[\s\S]*?delete_id\.erase\([^;]+;').Value
Assert-True ($deleteProgress -match 'delete_batch_max\{\s*100\s*\}') 'Clear no longer processes up to 100 items per batch.'
Assert-True ($deleteProgress -notmatch 'delete_batch_target|delete_progress_steps') 'Clear still splits work into artificial progress batches.'
Assert-True ($deleteProgress -notmatch 'delete_transfer_progress') 'Clear reports request-upload bytes as if Google had already deleted those items.'
Assert-True ($deleteProgress -notmatch 'm_progress\.(?:store|fetch_add)|\+\+[^;]*m_progress') 'Background blob garbage collection still controls the user-visible clear percentage.'
Assert-True ($deleteProgress -notmatch '\+\+delete_id\[i\]\.m_loading->m_progress') 'Clear progress still jumps only after the whole delete batch completes.'
$clearSetup = [regex]::Match($storageImplementation, 'else if \(obj_ptr->m_type == "delete_all"\)[\s\S]*?else \{\s*throw internal_error_t').Value
Assert-True ($clearSetup -match 'obj_ptr->m_size\.store\(1\)[\s\S]*?data_set\(curl_upload\(\), folder_root_id, folder_root_data\)[\s\S]*?obj_ptr->m_progress\.store\(1\)') 'Clear does not report 100% after the root index is successfully cleared.'
Assert-True ($clearSetup -match 'obj_ptr->m_progress\.store\(1\)[\s\S]*?obj_ptr->m_processed\.store\(true\)') 'Clear waits for background garbage collection before starting the completed-progress removal timer.'
Assert-True ($storageImplementation -match 'm_completed_at[\s\S]*?std::chrono::seconds\{\s*1\s*\}') 'Completed progress bars disappear before users can see 100%.'
$downloadImplementation = [regex]::Match($storageImplementation, 'auto file_download\([\s\S]*?auto download_guard\(').Value
Assert-True ($storageImplementation -match 'auto unique_download_path\(') 'Downloads overwrite an existing file or folder instead of choosing a unique destination.'
Assert-True ($storageImplementation -match 'auto safe_download_child\(') 'Folder downloads accept unsafe remote child names such as .. or absolute paths.'
Assert-True ($downloadImplementation -notmatch 'std::filesystem::remove_all') 'Downloading a folder recursively deletes an existing local folder.'
Assert-True ($downloadImplementation -match 'create_ofstream_destroy\(path_from_utf8\(downloading\.m_name\)\)') 'Empty downloads are not created and existing files can retain stale trailing bytes.'
Assert-True ($storageImplementation -match 'loading->m_name = path_to_utf8\(unique_download_path') 'Download destinations are stored in a locale-dependent narrow encoding.'
Assert-True ($downloadImplementation -match 'downloading\.m_name = path_to_utf8\(file\)[\s\S]*?safe_download_child\(folder') 'Nested Unicode download paths are converted through the active ANSI code page.'
Assert-True ($storageImplementation -match 'auto download_transfer_progress\(') 'Downloads do not publish incremental receive progress.'
Assert-True ($downloadImplementation -match 'curl_download_progress_guard_t') 'Download requests do not enable the CURL receive-progress callback.'
Assert-True ($downloadImplementation -match 'download batch contains no data') 'An empty remote download batch can leave the worker in an infinite loop.'
Assert-True ($downloadImplementation -match 'download exceeds declared file size') 'Malformed remote data can write beyond the declared file size.'
Assert-True ($downloadImplementation -match 'unique_download_path\(safe_download_child\(folder') 'Duplicate names inside a downloaded folder overwrite one another.'
Assert-True ($storageImplementation -match 'std::atomic_bool m_failed') 'Failed downloads have no terminal UI state and remain stuck forever.'
Assert-True ($storageImplementation -match 'auto mark_download_failed\(') 'Download failures are not recorded without terminating the worker.'
Assert-True ($downloadImplementation -match 'catch \(const std::exception& error\)[\s\S]*?mark_download_failed[\s\S]*?continue;') 'A failed download setup terminates the download worker instead of continuing with later jobs.'
Assert-True ($downloadImplementation -match 'std::erase_if\(file_list[\s\S]*?m_loading == failed_loading') 'A failed folder download leaves partial child jobs queued.'
Assert-True ($downloadImplementation -match 'catch \(const std::exception& error\)[\s\S]*?mark_download_failed[\s\S]*?std::erase_if\(file_list') 'A failed download batch terminates the download worker instead of isolating the failed job.'

$rectPrimitiveImplementation = Get-Content -Raw (Join-Path $repo 'compilercpp/GUI/primitive/rect_impl.hpp')
$rectPrimitiveSetSize = [regex]::Match($rectPrimitiveImplementation, 'auto rect_primitive_t::set_size\([\s\S]*?\n\}').Value
Assert-True ($rectPrimitiveImplementation -match 'm_clip_follows_size') 'Resizable unclipped rectangles cannot distinguish their automatic clip from an explicit clip.'
Assert-True ($rectPrimitiveSetSize -match 'm_clip_size\s*=\s*size') 'A resized rectangle remains clipped to its original dimensions.'

$fontFaceImplementation = Get-Content -Raw (Join-Path $repo 'compilercpp/GUI/freetype/font_face.hpp')
$fontCharImplementation = Get-Content -Raw (Join-Path $repo 'compilercpp/GUI/freetype/font_char.hpp')
Assert-True ($fontFaceImplementation -match 'size->metrics\.height\s*>>\s*6') 'Scaled FreeType line height is not converted from 26.6 pixels.'
Assert-True ($fontFaceImplementation -match 'size->metrics\.ascender\s*>>\s*6') 'Scaled FreeType ascender is not converted from 26.6 pixels.'
Assert-True ($fontFaceImplementation -match 'size->metrics\.descender\s*>>\s*6') 'Scaled FreeType descender is not converted from 26.6 pixels.'
Assert-True ($fontCharImplementation -match 'glyph->advance\.x\s*>>\s*6') 'Glyph advance is not converted from 26.6 pixels.'
Assert-True ($fontCharImplementation -match 'static_cast<std::int64_t>\(face_handle->glyph->bitmap_top\)\s*-\s*static_cast<std::int64_t>\(face_handle->glyph->bitmap\.rows\)') 'Negative glyph bearings underflow to a huge unsigned offset.'
$legacyDirectXInit = Get-Content -Raw (Join-Path $repo 'compilercpp/GUI/directx/init.hpp')
$legacyDirectXCreate = Get-Content -Raw (Join-Path $repo 'compilercpp/GUI/directx/create.hpp')
$legacyDirectXInfoQueue = Get-Content -Raw (Join-Path $repo 'compilercpp/GUI/directx/info_queue.hpp')
$legacyDirectXPipeline = Get-Content -Raw (Join-Path $repo 'compilercpp/GUI/directx/pipeline_state.hpp')
$legacyRect = Get-Content -Raw (Join-Path $repo 'compilercpp/GUI/object/rect.hpp')
$legacyRectPrimitive = Get-Content -Raw (Join-Path $repo 'compilercpp/GUI/primitive/rect_impl.hpp')
$legacyRectBorder = Get-Content -Raw (Join-Path $repo 'compilercpp/GUI/object/rect_border.hpp')
$legacyText = Get-Content -Raw (Join-Path $repo 'compilercpp/GUI/object/text.hpp')
$legacyTexturePixelShader = Get-Content -Raw (Join-Path $repo 'compilercpp/shader/texture_pixel.hlsl')
Assert-True ($legacyDirectXInit -match '#ifndef NDEBUG[\s\S]*?SetEnableGPUBasedValidation') 'Release builds still enable expensive DirectX GPU validation.'
Assert-True ($legacyDirectXCreate -match 'FAILED\(device\.As\(&out\)\)[\s\S]*?return out') 'Release builds treat the optional DirectX info queue as mandatory.'
Assert-True ($legacyDirectXInfoQueue -match 'if \(!info_queue\)[\s\S]*?return;') 'A missing optional DirectX info queue is dereferenced.'
Assert-True ($legacyDirectXPipeline -match 'FrontCounterClockwise\s*=\s*false') 'Top-left DirectX geometry is culled because its front-face winding is reversed.'
Assert-True (($legacyRect | Select-String -Pattern 'primitive\s*=\s*engine->add_rect' -AllMatches).Matches.Count -eq 2) 'Legacy rectangles discard the render primitive returned by the engine.'
Assert-True ($legacyRectPrimitive -match 'float pos_y\{\s*1\.0f\s*-') 'DirectX maps top-left GUI coordinates to the bottom of the viewport.'
Assert-True ($legacyRectPrimitive -match 'float size_y\{\s*-') 'DirectX rectangle height grows upward instead of downward.'
Assert-True ($legacyRectPrimitive -match 'm_pos\.x \+ m_texture_pos\.x' -and $legacyRectPrimitive -match 'm_pos\.y \+ m_texture_pos\.y') 'Textured rectangles ignore their intended glyph position.'
Assert-True ($legacyRectPrimitive -match 'm_texture_axis_x\.x' -and $legacyRectPrimitive -match 'm_texture_axis_y\.y') 'Textured rectangles stretch glyph bitmaps to the full line box.'
Assert-True ($legacyRectPrimitive -match 'scissor_rect\.top\s*=\s*static_cast<LONG>\(m_clip_pos\.y\)') 'The DirectX scissor rectangle uses an inverted Y origin.'
Assert-True ($legacyRectPrimitive -match 'scissor_rect\.bottom\s*=\s*static_cast<LONG>\(m_clip_pos\.y \+ m_clip_size\.y\)') 'The DirectX scissor rectangle ends above its GUI object.'
Assert-True ($legacyRectBorder -match '\(m_depth_range\.far - m_depth_range\.near\) / 3') 'Border, content, and text are not allocated distinct depth layers.'
Assert-True ($legacyRectBorder -match 'm_depth_range\.near \+ depth_distance \* 2') 'The border is not behind its content.'
Assert-True ($legacyRectBorder -match 'm_depth_range\.near \+ depth_distance[^*]') 'The button content is not behind its text.'
Assert-True ($legacyText -match 'm_clip_pos\{ pos \}[\s\S]*?m_clip_size\{ size \}') 'Text glyphs start with an empty clipping rectangle.'
Assert-True ($legacyTexturePixelShader -match 'float4\s*\(\s*1\.0f\s*,\s*1\.0f\s*,\s*1\.0f\s*,') 'Glyph coverage is multiplied twice by alpha blending.'
Assert-True ($legacyText -match 'm_clip_pos = pos;[\s\S]*?m_clip_size = size;') 'Reinitialized text glyphs retain an empty clipping rectangle.'

$legacyTextInput = Get-Content -Raw (Join-Path $repo 'compilercpp/GUI/object/text_input.hpp')
Assert-True ($legacyTextInput -notmatch 'push_back\(static_cast<char>\(in\)\)') 'Text input truncates Unicode characters to a single byte.'
Assert-True ($legacyTextInput -match 'to_string\(std::wstring\{\s*in\s*\}\)') 'Text input does not encode Windows Unicode input as UTF-8.'
Assert-True ($legacyTextInput -match 'to_wstring\(\*text\)[\s\S]*?pop_back\(\)[\s\S]*?to_string\(') 'Backspace does not remove a complete Unicode character.'

$legacyDepthTracker = Get-Content -Raw (Join-Path $repo 'compilercpp/GUI/object/depth_tracker.hpp')
Assert-True ($legacyDepthTracker -match 'm_depth\s*==\s*depth[\s\S]*?i->first\s*>\s*id') 'Same-depth controls do not prefer the most recently added overlay.'
$legacyEngine = Get-Content -Raw (Join-Path $repo 'compilercpp/GUI/primitive/engine_def.hpp')
Assert-True ($legacyEngine -match 'm_retired' -and $legacyEngine -match 'callback\.m_calling_callback[\s\S]*?callback\.m_retired\.push_back') 'A callback can still delete its own std::function while it is executing.'
$legacyButton = Get-Content -Raw (Join-Path $repo 'compilercpp/GUI/object/button.hpp')
$legacyClickArea = Get-Content -Raw (Join-Path $repo 'compilercpp/GUI/object/click_area.hpp')
Assert-True ($legacyClickArea -match 'GUI action failed:') 'An exception from a button callback can still escape the Win32 window procedure.'
Assert-True ($legacyButton -match 'm_click_area\{[\s\S]*?depth_range\.near\s*-\s*1\.0f') 'Button hit areas share depth with visual primitives and can be blocked by them.'
Assert-True ($legacyTextInput -match 'm_click_area\{[\s\S]*?depth_range\.near\s*-\s*1\.0f') 'Text-input hit areas share depth with visual primitives and can be blocked by them.'

$storageSources = @(
    Get-Content -Raw (Join-Path $repo 'storage/storage.cpp')
    Get-Content -Raw (Join-Path $repo 'storage/api/.cppm')
    Get-Content -Raw (Join-Path $repo 'storage/multipart/.cppm')
) -join "`n"
Assert-True ($storageSources -notmatch 'compilercpp/lib/\.hpp') 'Active storage sources include a header that was deleted during migration.'

$rootCMake = Get-Content -Raw (Join-Path $repo 'CMakeLists.txt')
Assert-True ($rootCMake -notmatch '[A-Za-z]:[/\\]') 'The root CMake configuration contains a machine-specific absolute path.'
Assert-True ($rootCMake -match 'option\(LINGOGOOGOL_BUILD_STORAGE') 'The incomplete legacy storage GUI cannot be isolated from the default build.'

$gitignore = Get-Content -Raw (Join-Path $repo '.gitignore')
Assert-True ($gitignore -match '(?m)^/tests/\*\.png$') 'Generated GUI test screenshots can be accidentally committed.'
Assert-True ($gitignore -match '(?m)^/work/$') 'Local review artifacts under work/ can be accidentally committed.'

$slangCMake = Get-Content -Raw (Join-Path $repo 'lgo/GUI/engine/CMakeLists.txt')
Assert-True ($slangCMake -match 'find_program\(SLANG_COMPILER') 'The Slang compiler is not discovered during configuration.'
Assert-True ($slangCMake -match 'OUTPUT "\$\{SPIRV_OUTPUT\}"[\s\S]*?-o "\$\{SPIRV_OUTPUT\}"') 'The declared and actual shader outputs do not match.'
$shaderTargetPosition = $slangCMake.IndexOf('add_slang_target(')
$shaderDependencyPosition = $slangCMake.IndexOf('add_dependencies(')
Assert-True ($shaderTargetPosition -ge 0 -and $shaderDependencyPosition -gt $shaderTargetPosition) 'The engine depends on the shader target before that target is created.'

$trackedArtifacts = git -C $repo ls-files -- '*.exe' '*.obj' '*.pdb' '*.dll'
$existingTrackedArtifacts = @($trackedArtifacts | Where-Object { Test-Path -LiteralPath (Join-Path $repo $_) })
Assert-True ($existingTrackedArtifacts.Count -eq 0) 'Generated binary artifacts are still present in the working tree.'

if ($failures.Count -ne 0) {
    $failures | ForEach-Object { Write-Error $_ -ErrorAction Continue }
    exit 1
}

Write-Host 'All regression checks passed.'
