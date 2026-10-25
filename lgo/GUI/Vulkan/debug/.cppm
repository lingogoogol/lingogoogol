export module lgo.GUI.Vulkan:debug;

import std;
import external.Vulkan;
import external.Windows;

export namespace lgo
{
    inline constexpr bool vulkan_names_enabled =
        #ifdef DEBUG_VULKAN_NAME
        true;
        #else
        false;
        #endif

    // Pass a lambda for computed names: disabled builds never construct those
    // strings. Vulkan-Hpp handles use pointer or integer native handles depending
    // on the platform; do not assume that every handle can be reinterpret_cast.
    template<typename t_handle, typename t_name>
    auto vulkan_name(const vk::raii::Device& device, t_handle handle, t_name&& name) -> bool
    {
        if constexpr (vulkan_names_enabled)
        {
            if (!*device || !handle || !device.getDispatcher()->vkSetDebugUtilsObjectNameEXT) return false;
            if constexpr (t_handle::objectType == vk::ObjectType::eDebugUtilsMessengerEXT)
            {
                // Wine 10 aborts inside its Vulkan thunk when naming a messenger,
                // rather than returning VkResult. Skip only this optional object
                // on Wine; native Windows and all other object types keep names.
                static const bool wine{ GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "wine_get_version") != nullptr };
                if (wine) return false;
            }
            using native_t = typename t_handle::CType;
            std::uint64_t bits{};
            if constexpr (std::is_pointer_v<native_t>)
                bits = reinterpret_cast<std::uintptr_t>(static_cast<native_t>(handle));
            else
                bits = static_cast<std::uint64_t>(static_cast<native_t>(handle));
            try
            {
                std::string label;
                if constexpr (std::invocable<t_name>) label = name();
                else label = name;
                // Repeated GUI primitives and glyphs can share a role. Include the
                // handle so simultaneously live resources still have distinct names.
                label += std::format(" [{:#x}]", bits);
                const vk::DebugUtilsObjectNameInfoEXT info
                {
                    .objectType{ t_handle::objectType },
                    .objectHandle{ bits },
                    .pObjectName{ label.c_str() }
                };
                device.setDebugUtilsObjectNameEXT(info);
                return true;
            }
            catch (...)
            {
                // Diagnostic naming must not turn a working renderer into an
                // application failure, including while a device is being lost.
                return false;
            }
        }
        return false;
    }
}
