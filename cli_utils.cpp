/*
CLI Utils: Cross-platform code for CLI finding and hooking
*/

#include <cstring>
#include <cstdint>
#include <string>
#include "idasdk.h"
#include "cli_utils.h"
#include "macro_editor.h"
#include <idacpp/callbacks/callbacks.hpp>

using namespace idacpp::callbacks;

#ifdef _WIN32
    #include <windows.h>
    #include <winternl.h>
#else
    #include <dlfcn.h>
    #ifdef __APPLE__
        #include <mach-o/dyld.h>
        #include <mach-o/getsect.h>
    #else
        #include <link.h>
        #include <elf.h>
    #endif
#endif

//-------------------------------------------------------------------------
// CLI Finding Implementation
//-------------------------------------------------------------------------

// Optimized search for a byte pattern in memory
// Uses memchr to quickly find first byte candidates, then validates full pattern
static const uint8_t* bin_search(const uint8_t* start, size_t size, const uint8_t* pattern, size_t pattern_len)
{
    if (size < pattern_len || pattern_len == 0)
        return nullptr;

    const uint8_t first_byte = pattern[0];
    const uint8_t* search_end = start + size - pattern_len;
    const uint8_t* p = start;

    while (p <= search_end)
    {
        // Use memchr to quickly find next occurrence of first byte
        p = (const uint8_t*)memchr(p, first_byte, search_end - p + 1);
        if (!p)
            return nullptr;

        // Found first byte - check if rest of pattern matches
        if (pattern_len == 1 || memcmp(p + 1, pattern + 1, pattern_len - 1) == 0)
            return p;

        // Move to next position
        ++p;
    }

    return nullptr;
}

//-------------------------------------------------------------------------
// Search for a pointer to target address in memory range
static const cli_t* find_cli_struct(const uint8_t* base, size_t size, const uint8_t* target_str)
{
    uintptr_t target_addr = (uintptr_t)target_str;

    // Search for pointer to the target string
    const uint8_t* ptr = base;
    const uint8_t* end = base + size - sizeof(uintptr_t);

    while (ptr <= end)
    {
        uintptr_t potential_ptr = *(const uintptr_t*)ptr;

        // Check if this points to our target string
        if (potential_ptr == target_addr)
        {
            // We found a pointer to the string
            // The cli_t structure should be nearby (likely -8 bytes for lname field)
            // cli_t layout: size(8), flags(4+padding), sname(8), lname(8), ...
            // So lname is at offset 0x10

            const cli_t* potential_cli = (const cli_t*)(ptr - offsetof(cli_t, lname));

            // Validate: check if size field matches expected cli_t size
            if (potential_cli->size == sizeof(cli_t))
            {
                // Additional validation: check if sname looks valid
                if (potential_cli->sname != nullptr)
                {
                    return potential_cli;
                }
            }
        }

        ptr++;
    }

    return nullptr;
}

//-------------------------------------------------------------------------
#ifdef _WIN32

// Windows implementation
cli_t* find_cli_in_module(const char* module_name, const char* target_string)
{
    HMODULE hModule = GetModuleHandleA(module_name);
    if (!hModule)
        return nullptr;

    // Get DOS header
    PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)hModule;
    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
        return nullptr;

    // Get NT headers - handle both PE32 and PE32+ (64-bit)
    uint8_t* base = (uint8_t*)hModule;
    PIMAGE_NT_HEADERS nt_headers = (PIMAGE_NT_HEADERS)(base + dos_header->e_lfanew);
    if (nt_headers->Signature != IMAGE_NT_SIGNATURE)
        return nullptr;

    // Get module size based on PE format
    size_t module_size;
    WORD magic = nt_headers->OptionalHeader.Magic;

    if (magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) // PE32+ (64-bit)
    {
        PIMAGE_NT_HEADERS64 nt_headers64 = (PIMAGE_NT_HEADERS64)nt_headers;
        module_size = nt_headers64->OptionalHeader.SizeOfImage;
    }
    else if (magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) // PE32 (32-bit)
    {
        PIMAGE_NT_HEADERS32 nt_headers32 = (PIMAGE_NT_HEADERS32)nt_headers;
        module_size = nt_headers32->OptionalHeader.SizeOfImage;
    }
    else
    {
        return nullptr; // Unknown PE format
    }

    // Search for the target string
    size_t target_len = strlen(target_string);
    const uint8_t* found_str = bin_search(
        base,
        module_size,
        (const uint8_t*)target_string,
        target_len
    );

    if (!found_str)
        return nullptr;

    // Search for cli_t structure pointing to this string
    const cli_t* cli = find_cli_struct(base, module_size, found_str);

    return const_cast<cli_t*>(cli);
}

#elif defined(__linux__)

// Linux implementation
cli_t* find_cli_in_module(const char* module_name, const char* target_string)
{
    void* handle = dlopen(module_name, RTLD_NOLOAD | RTLD_NOW);
    if (!handle)
        return nullptr;

    struct link_map* map;
    if (dlinfo(handle, RTLD_DI_LINKMAP, &map) != 0)
    {
        dlclose(handle);
        return nullptr;
    }

    uint8_t* base = (uint8_t*)map->l_addr;

    // Parse ELF header to get module size
    Elf64_Ehdr* ehdr = (Elf64_Ehdr*)base;
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0)
    {
        dlclose(handle);
        return nullptr;
    }

    // Calculate total size from program headers
    Elf64_Phdr* phdr = (Elf64_Phdr*)(base + ehdr->e_phoff);
    size_t module_size = 0;

    for (int i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdr[i].p_type == PT_LOAD)
        {
            size_t end = phdr[i].p_vaddr + phdr[i].p_memsz;
            if (end > module_size)
                module_size = end;
        }
    }

    // Search for the target string
    size_t target_len = strlen(target_string);
    const uint8_t* found_str = bin_search(
        base,
        module_size,
        (const uint8_t*)target_string,
        target_len
    );

    if (!found_str)
    {
        dlclose(handle);
        return nullptr;
    }

    // Search for cli_t structure pointing to this string
    const cli_t* cli = find_cli_struct(base, module_size, found_str);

    dlclose(handle);
    return const_cast<cli_t*>(cli);
}

#elif defined(__APPLE__)

// macOS implementation
cli_t* find_cli_in_module(const char* module_name, const char* target_string)
{
    void* handle = dlopen(module_name, RTLD_NOLOAD | RTLD_NOW);
    if (!handle)
        return nullptr;

    Dl_info info;
    if (dladdr(handle, &info) == 0)
    {
        dlclose(handle);
        return nullptr;
    }

    uint8_t* base = (uint8_t*)info.dli_fbase;

    // Parse Mach-O header
    struct mach_header_64* mh = (struct mach_header_64*)base;
    if (mh->magic != MH_MAGIC_64)
    {
        dlclose(handle);
        return nullptr;
    }

    // Calculate module size by finding the highest address
    struct load_command* lc = (struct load_command*)(base + sizeof(struct mach_header_64));
    size_t module_size = 0;

    for (uint32_t i = 0; i < mh->ncmds; i++)
    {
        if (lc->cmd == LC_SEGMENT_64)
        {
            struct segment_command_64* seg = (struct segment_command_64*)lc;
            size_t end = seg->vmaddr + seg->vmsize;
            if (end > module_size)
                module_size = end;
        }
        lc = (struct load_command*)((uint8_t*)lc + lc->cmdsize);
    }

    // Search for the target string
    size_t target_len = strlen(target_string);
    const uint8_t* found_str = bin_search(
        base,
        module_size,
        (const uint8_t*)target_string,
        target_len
    );

    if (!found_str)
    {
        dlclose(handle);
        return nullptr;
    }

    // Search for cli_t structure pointing to this string
    const cli_t* cli = find_cli_struct(base, module_size, found_str);

    dlclose(handle);
    return const_cast<cli_t*>(cli);
}

#else
    #error "Unsupported platform"
#endif

//-------------------------------------------------------------------------
// Helper functions for common CLI types

// Find Python CLI in IDAPython module
cli_t* find_python_cli()
{
#ifdef _WIN32
    const char* module_name = "idapython3.dll";
#elif defined(__linux__)
    const char* module_name = "idapython3.so";
#elif defined(__APPLE__)
    const char* module_name = "idapython3.dylib";
#else
    return nullptr;
#endif

    return find_cli_in_module(module_name, "Python - IDAPython plugin");
}

// Find IDC CLI in IDA main module
cli_t* find_idc_cli()
{
#ifdef _WIN32
    const char* module_name = "ida.exe";
#elif defined(__linux__)
    const char* module_name = "ida";
#elif defined(__APPLE__)
    const char* module_name = "ida";
#else
    return nullptr;
#endif

    return find_cli_in_module(module_name, "IDC - Native built-in language");
}

//-------------------------------------------------------------------------
// CLI Hooking Implementation
//-------------------------------------------------------------------------

// Define callback registry for CLI execute_line hooks
DEFINE_CALLBACK_REGISTRY(cli_execute_registry, decltype(cli_t::execute_line), MAX_CLIS)

// Context structure to allow hooking CLIs
struct cli_ctx_t
{
    const cli_t *old_cli;
    cli_t new_cli;
    callback_handle_t cb_handle;
};

static cli_ctx_t g_cli_ctx[MAX_CLIS] = {};

//-------------------------------------------------------------------------
const cli_t *hook_cli(const cli_t *cli)
{
    for (int i=0; i < qnumber(g_cli_ctx); ++i)
    {
        // Find empty slot
        auto &ctx = g_cli_ctx[i];
        if (ctx.old_cli != nullptr)
            continue;

        // Register callback with lambda that captures old_cli
        auto result = cli_execute_registry.register_callback(
            [&ctx](const char *line) -> bool {
                std::string repl = macro_replacer(line);
                return ctx.old_cli->execute_line(repl.c_str());
            }
        );

        if (!result)
            break;

        ctx.old_cli = cli;
        ctx.new_cli = *cli;
        ctx.cb_handle = result->first;
        ctx.new_cli.execute_line = result->second;
        return &ctx.new_cli;
    }
    return nullptr;
}

//-------------------------------------------------------------------------
const cli_t *unhook_cli(const cli_t *cli)
{
    for (auto &ctx: g_cli_ctx)
    {
        if (ctx.old_cli != cli)
            continue;

        // Unregister the callback
        cli_execute_registry.unregister_callback(ctx.cb_handle);

        ctx.old_cli = nullptr;
        ctx.cb_handle = INVALID_CALLBACK_HANDLE;

        return &ctx.new_cli;
    }
    return nullptr;
}

//-------------------------------------------------------------------------
// CLI Installation
//-------------------------------------------------------------------------

// Ignore UI hooks when set
bool g_b_ignore_ui_notification = false;

// [Un]install a CLI (asynchronously) using a UI request
void request_install_cli(const cli_t *cli, bool install)
{
    extern bool g_b_ignore_ui_notification;
    class cli_install_req_t: public ui_request_t
    {
        const cli_t *cli;
        bool install;
    public:
        cli_install_req_t(const cli_t *cli, bool install) : cli(cli), install(install)
        {
        }

        virtual bool idaapi run()
        {
            g_b_ignore_ui_notification = true;
            if (install)
                install_command_interpreter(cli);
            else
                remove_command_interpreter(cli);
            g_b_ignore_ui_notification = false;

            return false;
        }
    };

    execute_ui_requests(
        new cli_install_req_t(cli, install),
        nullptr);
}