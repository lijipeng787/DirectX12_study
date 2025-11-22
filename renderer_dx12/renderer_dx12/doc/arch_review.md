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

### 已解决的关键问题
- ✅ **单例耦合**: 已移除 `DirectX12Device` 单例，支持显式构造与配置，提升了测试灵活性。
- ✅ **光照系统碎片化**: 之前光照逻辑散落在 Graphics 和各 Shader 中，现已通过 `LightManager` 和统一 Shader 结构解决。
- ✅ **反射实现**: 成功验证了 Render-To-Texture 技术 (`ReflectionScene`)，证明了架构对多 Pass 渲染的支持能力。
- ✅ **窗口Resize支持**: 实现了完整的窗口调整大小功能，包括WM_SIZE消息处理、资源重建、投影矩阵更新。支持拖动边框、最大化/最小化等操作。

### 当前痛点与挑战

#### 🚨 关键阻塞问题（必须立即解决）
1.  **资源加载性能灾难**:
    - 现状：`Model.cpp`每次加载模型都创建临时command queue/allocator/list。
    - 问题：这是错误的D3D12使用模式，导致严重的性能和资源浪费。
    - 影响：加载10个模型会创建10个临时command queue，这在D3D12中是反模式。
    - 优先级：P0，架构级bug。

2.  **零测试覆盖**:
    - 现状：没有任何单元测试、集成测试或自动化验证。
    - 问题：每次修改都可能破坏现有功能，无法保证代码质量。
    - 优先级：P1，在继续添加功能前必须建立基础测试框架。

3.  **资源管理缺失**:
    - 现状：没有纹理/模型缓存，重复加载相同资源浪费显存。
    - 问题：无法追踪资源生命周期，可能导致显存泄漏。
    - 优先级：P1，基础设施缺失。

#### ⚠️ 次要但重要的问题
4.  **调试手段原始**:
    - 现状：依赖 `OutputDebugString`，没有结构化日志系统。
    - 需求：集成spdlog或类似库，支持日志级别和文件输出。

5.  **错误处理落后**:
    - 现状：使用bool返回+HRESULT，没有异常或Result<T>类型。
    - 问题：错误信息丢失，难以追踪失败原因。

6.  **构建系统落后**:
    - 现状：仅支持Visual Studio项目文件，不跨平台。
    - 需求：添加CMake支持，至少支持Windows + Linux (Vulkan)。

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
- ✅ Builder模式封装PSO和RootSignature创建

### 严重缺陷

**基础功能缺失：**
- 窗口resize不支持（已存在1年+，Critical）
- 资源加载使用错误模式（每次创建临时command queue）
- 零测试覆盖
- 没有资源缓存和管理系统
- 没有现代日志系统

**架构问题：**
- Scene系统过度工程化（type erasure用于5个场景）
- 设备类职责过多，违反单一职责原则
- 材质系统僵化，扩展困难
- 错误处理方式原始（bool + HRESULT）

**距离"现代渲染架构"的差距：**
- ❌ 没有Render Graph
- ❌ 没有多线程命令录制
- ❌ 没有异步资源加载
- ❌ 没有bindless descriptors
- ❌ 没有GPU-driven rendering
- ❌ 没有async compute利用

**现实评估：** 这是一个**2017年水平的D3D12学习项目**，不是"现代渲染架构"。代码质量中等，基础功能不完整，无法作为可靠的实验平台。

### 优先行动项（按重要性）
1. **P0 - 立即修复**：资源加载重构
2. **P1 - 本月完成**：添加基础测试 + 资源管理系统 + 日志系统
3. **P2 - 下月考虑**：PIX集成 + shader热重载
4. **P3 - 暂缓**：延迟渲染、compute shader等高级特性
