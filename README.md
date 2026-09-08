# lingogoogol

Experimental Windows C++23 modules, GUI, compiler, and storage code.

## Requirements

- Windows 10 or newer
- CMake 4.0–4.4
- A C++23 compiler with C++ standard-library module support
- Vulkan SDK
- Project dependencies supplied through a vcpkg toolchain

Configure dependencies through `CMAKE_TOOLCHAIN_FILE`; do not add machine-specific
include or library directories to `CMakeLists.txt`.

The legacy storage GUI is disabled by default while its old DirectX widget layer is
ported to the new Vulkan engine. Enable it explicitly with
`-DLINGOGOOGOL_BUILD_STORAGE=ON` when working on that migration.

## Storage credentials

The `storage` application requires these process environment variables:

- `LINGOGOOGOL_SERVICE_ACCOUNT_EMAIL`
- `LINGOGOOGOL_SERVICE_ACCOUNT_PRIVATE_KEY`

The private key may contain real newlines or escaped `\n` sequences. Never commit
credentials, `.env` files, PEM files, or service-account JSON files. The previously
committed Google service-account key must be revoked before the storage application
is used again.

## Regression checks

Run the fast source-level checks from PowerShell:

```powershell
./tests/regression.ps1
```

These checks do not replace a compiler build. A clean configure and build should
also run in CI on a machine with the requirements above.
