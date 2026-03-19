lang: zh
---
layout: default
title: Environment Variables
nav_order: 2
parent: 用户文档
description: Environment variables used by SHM-PCC-SDK
lang: zh
---

# Environment Variables

{% include language-switcher.html %}


This document describes the environment variables used by SHM-PCC-SDK.

## Sudo Password

### `SUDO_PASSWORD`

**Description**: Sudo password for environment setup scripts.

**Usage**: 
- Set this variable if you want to avoid password prompts during script execution
- If not set, scripts will prompt for password interactively
- **Security Note**: Never commit this variable to version control

**Example**:
```bash
export SUDO_PASSWORD=your_password_here
```

**Recommended**: Use interactive prompts instead of setting this variable for better security.

## CXL Memory Configuration

### `CXL_MEM_PATH`

**Description**: Path to CXL memory device.

**Default**: Not set

**Example**:
```bash
export CXL_MEM_PATH=/dev/cxl/mem0
```

### `MEMKIND_MEM_TYPE`

**Description**: Memory type for memkind library.

**Options**: 
- `CXL` - Use CXL memory
- `DRAM` - Use DRAM (default)

**Example**:
```bash
export MEMKIND_MEM_TYPE=CXL
```

## Build Configuration

### `CMAKE_BUILD_TYPE`

**Description**: CMake build type.

**Options**: 
- `Release` (default) - Optimized build
- `Debug` - Debug build with symbols
- `RelWithDebInfo` - Release with debug info
- `MinSizeRel` - Minimum size release

**Example**:
```bash
export CMAKE_BUILD_TYPE=Debug
```

### `CMAKE_CXX_STANDARD`

**Description**: C++ standard version.

**Default**: `17`

**Example**:
```bash
export CMAKE_CXX_STANDARD=20
```

## Test Configuration

### `PREALLOC_SIZE`

**Description**: Memory pre-allocation size in MB for large workloads.

**Default**: Not set (no pre-allocation)

**Example**:
```bash
export PREALLOC_SIZE=4096  # Pre-allocate 4GB
```

### `DBG_LEVEL`

**Description**: Debug output level.

**Options**:
- `0` - No debug output (default)
- `1` - Basic debug output
- `2` - Verbose debug output

**Example**:
```bash
export DBG_LEVEL=2
```

## Runtime Configuration

### `LD_LIBRARY_PATH`

**Description**: Library search path.

**Example**:
```bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
```

## Setting Environment Variables

### Temporary (Current Session)

```bash
export SUDO_PASSWORD=your_password
export CXL_MEM_PATH=/dev/cxl/mem0
```

### Persistent (Add to ~/.bashrc or ~/.zshrc)

```bash
# Add to ~/.bashrc or ~/.zshrc

{% include language-switcher.html %}

export CXL_MEM_PATH=/dev/cxl/mem0
export MEMKIND_MEM_TYPE=CXL
```

### Using .env File (Not Recommended for Passwords)

Create a `.env` file in the project root:

```bash
# .env (DO NOT COMMIT THIS FILE)

{% include language-switcher.html %}

CXL_MEM_PATH=/dev/cxl/mem0
MEMKIND_MEM_TYPE=CXL
PREALLOC_SIZE=4096
```

Then source it:
```bash
source .env
```

**Security Warning**: Never commit `.env` files containing passwords or sensitive information to version control.

## Security Best Practices

1. **Never commit passwords**: Use interactive prompts or secure password managers
2. **Use .gitignore**: Ensure `.env` files are in `.gitignore`
3. **Limit scope**: Only set environment variables when needed
4. **Clear after use**: Unset sensitive variables after use
   ```bash
   unset SUDO_PASSWORD
   ```

## Troubleshooting

### Scripts Prompting for Password

If scripts keep prompting for password even after setting `SUDO_PASSWORD`:
- Check if the variable is exported: `echo $SUDO_PASSWORD`
- Ensure the variable is set in the same shell session
- Try using `sudo -v` to cache credentials instead

### CXL Memory Not Found

If CXL memory is not accessible:
- Check if CXL device exists: `ls -la /dev/cxl/`
- Verify permissions: `ls -l /dev/cxl/mem0`
- Check if memkind supports CXL: `memkind --help`

lang: zh
---

For more information, see [User Guide](USER_GUIDE.md) or [Configuration](USER_GUIDE.md#configuration).


