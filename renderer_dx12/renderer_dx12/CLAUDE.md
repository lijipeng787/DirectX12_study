# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

This is a Visual Studio C++ DirectX 12 renderer project using MSBuild:

- **Primary build tool**: Open `renderer_dx12.vcxproj` in Visual Studio
- **Supported configurations**: Debug/Release for Win32/x64
- **Required dependencies**: DirectX 12, DirectXTK, D3DCompiler
- **Platform target**: Windows 10+ (uses DirectX 12 API)
- **Language standard**: C++17

Key libraries linked:
- `d3d12.lib`, `dxgi.lib`, `d3dcompiler.lib` - DirectX 12 core
- `DirectXTK.lib` - DirectX Toolkit utilities
- `dinput8.lib`, `dsound.lib` - Input/Audio
- `pdh.lib`, `winmm.lib` - Performance monitoring

## High-Level Architecture

### Core Application Flow
- **Entry point**: `main.cpp:WinMain` → `System` class initialization
- **System class** (`System.h/cpp`): Manages window, input, and graphics lifecycle
- **Graphics class** (`Graphics.h/cpp`): Primary rendering coordinator
- **DirectX12Device** (`DirectX12Device.h/cpp`): D3D12 device abstraction and resource management

### Rendering Architecture

The project follows a **modular, scene-based rendering architecture** designed for experimentation:

#### Device Management
- `DirectX12Device`: Centralized D3D12 device, command queues, swap chain, and frame synchronization
- `DxgiResourceManager`: DXGI adapter enumeration and swap chain creation
- Frame-based resource management with `FrameResource` struct for per-frame command allocators
- Offscreen render target system using handles (`RenderTargetHandle`)

#### Scene System
Three main scene types implementing different rendering techniques:
- **BumpMappingScene**: Normal mapping demonstration
- **SpecularMappingScene**: Specular mapping with reflections
- **ReflectionScene**: Real-time reflections using render-to-texture

Each scene follows the pattern:
```cpp
class XScene {
    bool Initialize();
    void Update(float delta_seconds);
    bool Render(view_matrix, projection_matrix);
    void SetRotationAngle(float radians);
};
```

#### Material System
Unified material architecture with external constant buffer injection:
- **Base classes**: `Material` → specialized materials (PBR, BumpMap, SpecularMap, etc.)
- **Model classes**: Handle geometry, textures, and material binding
- **ScreenQuad**: Full-screen quad for post-processing effects

#### Lighting System
Unified lighting via `Lighting::LightManager` and `Lighting::SceneLight`:
- Supports directional, point, and spot lights
- Materials use `UpdateFromLight(SceneLight*)` to extract parameters
- Shared lighting state across all rendering techniques

### Shader Organization
Shaders are located in `shader/` directory:
- `reflection.hlsl` - Real-time reflection vertex/pixel shaders
- `bumpMap.hlsl` - Normal mapping shaders
- `specMap.hlsl` - Specular mapping shaders
- `pbr.hlsl` - Physically-based rendering (vertex + pixel)
- `light.hlsl` - Basic lighting with fog (vertex + pixel)
- `texture.hlsl` - Simple texture rendering (vertex + pixel)
- `font.hlsl` - Text rendering with alpha masking (vertex + pixel)

Note: Shaders are excluded from x64 Debug builds in the project file.

### Resource Management
- **TextureLoader**: DDS texture loading and GPU upload
- **ShaderLoader**: HLSL compilation and bytecode caching
- **Font system**: Bitmap font rendering with glyph mapping
- **Model loading**: Text-based model format from `data/` directory

### Data Organization
- `data/` - Models (`.txt`), textures (`.dds`), fonts
- `data/pbr/` - PBR-specific assets
- `shader/` - HLSL shader source files
- `doc/` - Architecture documentation (Chinese)

## Key Design Patterns

### Factory Pattern
```cpp
std::shared_ptr<DirectX12Device> device = DirectX12Device::Create(config);
```

### RAII Resource Management
Extensive use of `Microsoft::WRL::ComPtr` for D3D12 resources and `std::shared_ptr` for application objects.

### Command Pattern
Graphics commands are recorded to command lists, then executed:
```cpp
device->BeginPopulateGraphicsCommandList();
// Record commands
device->EndPopulateGraphicsCommandList();
device->ExecuteDefaultGraphicsCommandList();
```

### Scene Graph
Each scene manages its own models, materials, and rendering state independently.

## Development Notes

- This is an **experimental rendering framework**, not a production engine
- Architecture prioritizes **flexibility for trying new techniques** over performance
- The codebase includes detailed Chinese documentation in `doc/arch_review.md`
- Frame synchronization uses fence-based GPU/CPU coordination
- Offscreen rendering is supported through the render target handle system
- Multiple command queues (Graphics, Copy) are available but Compute queue is not currently used

## Common Development Workflows

When adding new rendering techniques:
1. Create new Scene class following existing pattern
2. Implement associated Material and Model classes
3. Add required shaders to `shader/` directory
4. Register scene in `Graphics` class
5. Update input handling if needed

The architecture is specifically designed to make this workflow straightforward for experimenting with DirectX 12 features.