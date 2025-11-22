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

### 当前痛点与挑战
1.  **窗口热重建缺失 (Critical)**: 
    - 现状：改变窗口大小会导致拉伸或 crash，无法处理交换链重建。
    - 影响：严重阻碍了不同分辨率下的 UI 与后处理测试。
2.  **调试手段有限**:
    - 现状：依赖 `OutputDebugString` 和 HRESULT。
    - 需求：缺乏 PIX 事件标记 (Event Markers) 和实时的 GPU 计时统计。
3.  **多光源实战**:
    - 现状：架构支持多光源，但目前场景多为单光源测试，尚未验证 4+ 光源下的性能与 Shader 复杂性。

## 4. 开发工作流指南

### 添加新渲染技术 (New Feature Workflow)
1.  **Shader**: 在 `shader/` 编写新的 HLSL (VS/PS)。
2.  **Material**: 继承 `Material` 类，实现参数绑定与 PSO 创建 (使用 `PipelineStateBuilder`)。
3.  **Model**: 创建对应的 Model 类，绑定 Geometry 与 Material。
4.  **Scene**: 创建 `XScene` 类，在 `Graphics` 中注册并实例化。

### 调试建议
- 检查 VS 输出窗口的 "Debug" 面板，`DxgiResourceManager` 会输出详细的显存与适配器信息。
- 遇到 `DeviceRemoved` 错误时，优先检查资源状态屏障 (Resource Barrier) 是否匹配。

## 5. 总体结论

核心框架已完成关键的现代化重构（单例拆除、初始化模块化、帧资源环、统一光照）。架构在“可扩展性”上表现优秀，成功支撑了 PBR、法线贴图、实时反射等多种技术。

**下一步重心**: 必须从“架构重构”转向“交互与调试能力提升”，优先解决窗口 Resize 问题，并引入性能分析工具，使框架真正具备“实验台”的完整能力。
