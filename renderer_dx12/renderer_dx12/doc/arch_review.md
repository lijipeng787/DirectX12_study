# DirectX12 渲染架构评审与开发手册

## 1. 架构设计原则 （AI 助手请勿修改）

本项目旨在构建一个**灵活、高可用**的渲染架构，专门用于**快速试验**各种新渲染技术。它**不是**一个追求极致性能的工业级游戏引擎。在进行架构评审和代码修改时，必须始终权衡“灵活性”与“复杂度”，避免过度抽象。

## 2. 系统架构概览 (System Architecture)

### 2.1 构建与环境
- **构建系统**: Visual Studio (MSBuild), C++17 标准
- **核心依赖**: DirectX 12, DirectXTK, D3DCompiler, Win32 API
- **目录结构**:
  - `src/lib`: 核心实现 (`System`, `Graphics`, `Device` 等)
  - `src/include`: 头文件
  - `shader/`: HLSL 着色器 (VS/PS)
  - `data/`: 资产文件 (模型 .txt, 纹理 .dds/tga, 字体)

### 2.2 核心模块
1.  **应用层 (`System`)**: 管理 Win32 窗口消息、输入 (`Input`) 和主循环。
2.  **渲染协调 (`Graphics`)**: 负责场景组装、资源加载 (`TextureLoader`, `ShaderLoader`) 和帧渲染流程。
3.  **设备层 (`DirectX12Device`)**: 
    - 封装 D3D12 设备、命令队列 (Direct/Copy)、交换链。
    - 实现**帧资源环 (Frame Resources)**：每帧独立的命令分配器与 Fence 同步，确保 CPU/GPU 并行效率。
    - 提供**离屏渲染 (Off-screen)** 支持：通过 `RenderTargetHandle` 管理 RTV/SRV 资源。

### 2.3 场景与材质系统
项目采用**模块化场景**设计，每个场景类（如 `PBRScene`, `ReflectionScene`）独立管理其模型与状态：
- **场景 (Scene)**: 
  - 统一接口：通过上下文结构（`SceneInitializeContext`, `SceneRenderContext`）传递依赖，确保签名一致性。
  - 类型擦除包装器：使用外部多态模式（External Polymorphism），允许异构场景存储在 `std::vector<Scene>` 中，无需虚继承。
  - 编译期检查：通过 SFINAE 检测可选接口（如 `RenderReflection`），避免强制实现不需要的方法。
- **材质 (Material)**: 
  - 统一基类 `Material`，派生出 `PBRMaterial`, `BumpMapMaterial` 等。
  - 支持外部常量缓冲 (Constant Buffer) 注入，便于参数调整。
- **光照 (Lighting)**:
  - `LightManager`: 统一管理方向光、点光源、聚光灯。
  - `SceneLight`: 数据结构对齐 HLSL，支持多光源数组传递。

### 2.4 关键设计模式
- **RAII**: 使用 `Microsoft::WRL::ComPtr` 自动管理 D3D12 资源生命周期。
- **工厂模式**: 如 `DirectX12Device::Create`，隐藏复杂初始化逻辑。
- **命令模式**: 显式录制 (`BeginPopulate` -> Record -> `EndPopulate`) 与执行 (`Execute`) 分离。
- **类型擦除 (Type Erasure)**: 场景系统通过 Concept-Model 模式实现运行时多态，保持接口统一性的同时避免传统虚继承的侵入性。

## 3. 核心问题复盘 (Status Review)

### 当前痛点与挑战

#### 🚨 关键阻塞问题（必须立即解决）
1.  **资源加载性能灾难 (CONFIRMED)**:
    - 现状：`Model.cpp` 和 `TextureLoader.cpp` 中，**每一个** Buffer (Vertex/Index) 和 Texture 的创建都会**新建**一个 `ID3D12CommandQueue` (Direct)、Allocator、CommandList 和 Fence，并执行**同步等待** (`WaitForSingleObject`)。
    - 严重性：极度低效。Direct Queue 创建开销巨大，且不仅失去了并行性，还频繁触发内核态切换。
    - 优先级：**P0**，架构级缺陷，必须重构为使用共享 Copy Queue 或 Ring Buffer 上传。

2.  **零测试覆盖**:
    - 现状：没有任何单元测试、集成测试或自动化验证。
    - 问题：每次修改都可能破坏现有功能，无法保证代码质量。
    - 优先级：P1，在继续添加功能前必须建立基础测试框架。

3.  **Shader 模型落后**:
    - 现状：所有 Shader 编译硬编码使用 `vs_5_0` / `ps_5_0`。
    - 问题：这是 DX11 时代的 Shader 模型。DX12 的高级特性（如 Bindless Descriptors, Wave Intrinsics）需要 SM 5.1 或 SM 6.0+。
    - 优先级：P2，阻碍向现代架构演进。

#### ⚠️ 次要但重要的问题
4.  **调试手段原始**:
    - 现状：依赖 `OutputDebugString`，没有结构化日志系统。
    - 需求：集成 spdlog 或类似库，支持日志级别和文件输出。

5.  **错误处理落后**:
    - 现状：使用 `bool` 返回 + `HRESULT`，没有异常或 `Result<T>` 类型。
    - 问题：错误信息丢失，难以追踪失败原因。
    - 建议：引入 `std::optional` 或 `std::expected` (C++23) / 自定义 Result 类型。

6.  **构建系统落后**:
    - 现状：仅支持 Visual Studio 项目文件，不跨平台。
    - 需求：添加 CMake 支持，至少支持 Windows。

## 4. 开发工作流指南

### 添加新渲染技术 (New Feature Workflow)
1.  **Shader**: 在 `shader/` 编写新的 HLSL (VS/PS)。
2.  **Material**: 继承 `Material` 类，实现参数绑定与 PSO 创建 (使用 `PipelineStateBuilder`)。
3.  **Model**: 创建对应的 Model 类，绑定 Geometry 与 Material。
4.  **Scene**: 创建 `XScene` 类，在 `Graphics` 中注册并实例化。

### 调试建议
- 检查 VS 输出窗口的 "Debug" 面板，`DxgiResourceManager` 会输出详细的显存与适配器信息。
- 遇到 `DeviceRemoved` 错误时，优先检查资源状态屏障 (Resource Barrier) 是否匹配。

## 5. 总体结论（客观评估）

### 已完成的改进
- ✅ 移除单例，采用依赖注入
- ✅ 使用ComPtr进行资源管理
- ✅ 实现帧资源环（Frame Resources）
- ✅ 统一光照系统（LightManager）
- ✅ 离屏渲染抽象（RenderTargetHandle）
- ✅ 窗口Resize支持（经代码审查确认已实现）

### 严重缺陷

**基础功能缺失：**
- ❌ **资源加载使用错误模式**（每次创建临时command queue，严重性能Bug）
- ❌ 零测试覆盖
- ❌ 没有资源缓存和管理系统
- ❌ 没有现代日志系统

**架构问题：**
- ❌ **Shader Model 过旧** (SM 5.0)，不支持现代 DX12 特性
- ❌ 场景系统虽然使用了 Type Erasure，但渲染循环 (`Graphics::Render`) 仍是简单的线性遍历，缺乏 Render Graph。
- ❌ 错误处理方式原始（bool + HRESULT）

**距离"现代渲染架构"的差距：**
- ❌ 没有 Render Graph
- ❌ 没有多线程命令录制
- ❌ 没有异步资源加载
- ❌ 没有 bindless descriptors (受限于 SM 5.0)
- ❌ 没有 GPU-driven rendering
- ❌ 没有 async compute 利用

**现实评估：** 
目前代码是一个**带有严重性能陷阱的 DX12 入门级项目**。虽然实现了一些 DX12 基础（如 Fence 同步），但在资源管理方面犯了原则性错误。Window Resize 的实现是正确的，这是一个亮点。但 Shader 模型和渲染管线的陈旧使其无法直接作为现代渲染技术的实验场，除非先升级基础架构。

### 优先行动项（按重要性）
1. **P0 - 立即修复**：资源加载重构（不再每次创建 Queue）
2. **P1 - 本月完成**：添加基础测试 + 资源管理系统
3. **P2 - 基础设施**：日志系统 + CMake 支持 + 升级 Shader Model
4. **P3 - 暂缓**：延迟渲染、Render Graph 等高级特性
