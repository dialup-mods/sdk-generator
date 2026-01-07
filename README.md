<h1 align="center">🌐 Dial-Up Unreal Engine SDK Generator 🌐</h1>
<p align="center">*Best viewed in 800x600*</p>

**Next generation Unreal Engine SDK generator built from scratch** for cross-DLL safety and clean code generation.

### Generator Codebase
- **9,378 LOC** core generator
- CRTP-based Layout Traits for compile-time metadata
- Complete Unreal Engine object reflection
- Clean architecture, separation of concerns, extendable
- Strong typing throughout

### Generated SDK Output
- **1,002,364 → 369,359 LOC** (–63.2%)*
- **633,005 fewer lines to parse through**
- A decade of cargo cult, gone.
- Type-erased containers (TArray, TMap)
- Cross-DLL memory safety
- Complete class definitions with proper inheritance (no fake script glue!)

### Binary Size
- **47 MB → 14 MB** (–70.2%)*
- Smaller footprint, faster loading
- Massively reduced symbol table overhead

### Compilation Performance
- **25.7% faster** build times*
- Optimized dependency graph
- Clean code paths

### Code Quality
- **0 compiler warnings**
- Achieved via correct typing and intentional casts
- --checks-\*, cppcoreguidelines-\*, modernize-\*, bugprone-\*, readability-\*
- Real modern best practices
- **Professional AF** ✅

*Generated with Rocket League compared to the leading community generator

## Compile once, use everywhere

With the [DialUp-SDK plugin](https://github.com/dialup-mods/sdk-plugin), you can ship the SDK as a DLL. It's yours, *absolutely FREE!*

- Dynamically load into plugins
- Supports multiple instances
- No symbol duplication / ODR hell
- Compile and link against the SDK once
- Speed up development without PCH rabbit holes

## Core Type Ownership Changes

**IMPORTANT:** If migrating from existing SDK generators, note that caches have moved to `Runtime::` where they belong.

### What Changed

**Before:** Lookups and caches lived inside reflected types (FName, UObject)  
**Now:** Reflected types are pure data structures, Runtime handles all lookups

### Migration Examples

```c++
// FName lookups
FName::GetEntry(id)        → Runtime::getFNameEntry(id)

// UObject class finding  
UObject::FindClass("...")  → Runtime::findClass("...")
```

### Why This Matters

**Separation of concerns:**
- A cache of UObjects is not a property of UObject
- A cache of FNames is not something an FName should own  
- Finding an existing FName by name is not the duty of an FName
- *Cross-pollination inhibits understanding*

**Benefits:**
- ✓ Proper ownership boundaries
- ✓ Less bug-prone (no static state in reflected types)
- ✓ Easier to reason about (data vs utilities)
- ✓ Reflects reality

Most code can be updated with simple find/replace.

## Quick Start

### 1. Prerequisites

#### Install Dependencies

Run the following in Powershell as administrator to install: Git, CMake, Ninja, Clang, MSYS2

```powershell
iwr -useb https://raw.githubusercontent.com/dialup-mods/dialup/main/tools/install_build_deps.ps1 | iex
```

#### Clone and Install Build Tools
```bash
git clone --recursive git@github.com:dialup-mods/dialup.git
cd dialup
make install-tools
```

#### Install the injector:
```sh
cd injector
make configure
make build
make install
```

NOTE: Using other generators is possible, but unsupported. The generated output should go to `../sdk-plugin/generated` for building the SDK plugin; this code path is untested outside of running the official injector within the current build system.

### 2. Create a Game Profile
```sh
cd ../sdk-generator
cp config/default.yaml config/mygame.yaml
cp -r config/default config/mygame
```

### 3. Configure Your Game

1. **Edit `/config/mygame.yaml`** - Update patterns, offsets, and blacklist for your game. Edits made here do not require a recompile before re-running
2. **Edit `/config/mygame/Schema.h` and `/config/mygame/Schema.cpp`** - Define core game types

See the [Schema documentation](src/schema/README.md) for details on customizing SDK generation

### 4. Generate Your SDK
```sh
make configure game=mygame
make build
make inject
```

The generated SDK will be output to `../sdk-plugin/generated/`.

### Next Steps

Once generated, [build the DialUp SDK plugin](../sdk-plugin) to compile your SDK into a reusable library that can be safely shared across multiple plugins - no duplicate caches, no ODR violations, no random crashes.

## Troubleshooting

`log.txt` is generated in the current directory.

## Roadmap

- Handle type name / function name collisions (e.g. SearchOwnerStatus)
- Improve TypeRules / blacklisting system
- Make "interesting flags" configurable
- Address "class" vs "struct" prefix handling
- Review and update remaining fixmes
