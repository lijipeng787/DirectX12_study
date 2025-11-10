# DirectX12 渲染架构下一步实施计划

## 实施规划原则 （作为辅助编程的AI，你不应该修改这一部分）

这个项目的目标是完成一个灵活的，具有高可用性的渲染架构，可以方便得试验各种新渲染技术，一个工业级的游戏/渲染引擎不是这个项目的目标，进行渲染架构技术下一步实施细则时必须要考虑这一点。

## 目标

落实技术路线图阶段 1 的关键改动，为后续帧资源与命令调度改造打基础。

## 任务 1：DirectX12Device 构造模式重构

- ✅ 设计新的实例化接口，允许外部传入配置结构体（屏幕尺寸、同步策略等）。
- ✅ 移除 `GetD3d12DeviceInstance` 单例入口，提供智能指针或工厂函数返回实例，在单 GPU 场景下确保可灵活创建/销毁设备。
- ✅ 确保构造/析构流程覆盖设备、队列、交换链等资源的初始化与销毁。

## 任务 2：初始化流程模块化

- ✅ 将当前 `Initialize` 拆分为若干私有方法：`CreateDevice`、`CreateCommandQueues`、`CreateSwapChain`、`CreateDescriptorHeaps`、`CreateOffscreenResources` 等。
- ✅ 每个步骤返回 `HRESULT` 或包装结果，统一在高层处理错误与日志输出。
- ✅ 引入 RAII 或 ScopeGuard，确保任一阶段失败时释放已创建的资源。

## 任务 3：统一资源管理策略

- ✅ 替换手动 `Release` 为 `ComPtr` 或自定义句柄类，保持风格一致。
- ✅ 为关键资源（工厂、适配器、交换链等）建立包装类，负责生命周期与调试信息输出。
- ✅ 新增基础日志接口，记录资源创建失败原因以及关键配置参数。

## 任务 4：离屏渲染构建抽象

- ✅ 定义 `RenderTargetDescriptor`（尺寸、格式、用途），允许外部请求离屏纹理。
- ✅ 重构 `BeginDrawToOffScreen/EndDrawToOffScreen`，改为操作外部提供的目标句柄。
- ✅ 针对现有 ScreenQuad 保持兼容，提供默认 off-screen 目标的便捷工厂。

## 任务 5：ScreenQuad 解耦材质

- ✅ 将 `ScreenQuadMaterial` 从 `ScreenQuad` 类中分离，允许通过构造或 setter 注入。
- ✅ 暴露 `VertexBufferView`/`IndexBufferView` 的更新接口，方便实验不同顶点布局。
- ✅ 整理常量缓冲接口，允许自定义布局或绑定外部 CBV 资源。

## 任务 6：统一光照系统重构（2025-11-10 完成）

- ✅ 设计并实现 `Lighting::SceneLight` 统一光源类，支持方向光/点光源/聚光灯三种类型。
- ✅ 实现 `Lighting::LightManager` 多光源管理器，支持动态添加/删除/查询光源（最多8个）。
- ✅ 移除 `Graphics` 类中的冗余字段（`light_` 和 `pbr_light_direction_`），统一使用 `LightManager`。
- ✅ 为 `ModelMaterial` 和 `PBRMaterial` 添加 `UpdateFromLight(SceneLight*)` 接口。
- ✅ 确保 Cube（Blinn-Phong）和 PBR Sphere 共用同一光源，验证光照一致性。

## 验收标准

- ✅ 完成以上任务后，可在不依赖单例的情况下初始化渲染设备并渲染 ScreenQuad。
- ✅ 初始化阶段已具备详细日志（`DxgiResourceManager`、`LogInitializationFailure`），错误回滚通过 `ResetDeviceState` 实现。
- ✅ Off-screen 渲染目标可动态生成并绑定自定义材质，支持不同尺寸/格式的试验（`CreateRenderTarget` + `RenderTargetDescriptor`）。
- ✅ Frame 资源环已完成（`FrameResource` 结构 + per-frame allocator + fence），支持稳定的双缓冲渲染。
- ✅ 统一光照系统运作良好，Cube 和 PBR Sphere 共用同一光源，验证通过。
- ⏳ 窗口 Resize 与资源热重建仍待实现，需补齐 OnResize 接口与交换链重建逻辑。
- ⏳ 多光源场景实战验证待完成，需测试 2-3 光源场景与 Shader 适配。

## 后续重点方向

### 阶段 6：窗口与配置热重建（高优先级）

- **任务 6.1**：实现 `OnResize` 接口，监听窗口尺寸变更事件。
- **任务 6.2**：扩展 `DirectX12Device::ReinitializeSwapChain`，安全释放并重建交换链、RTV、深度缓冲。
- **任务 6.3**：更新 `viewport_` 与 `scissor_rect_`，重新计算投影矩阵与正交矩阵。
- **任务 6.4**：提供 MSAA/HDR 切换接口，通过配置结构体触发资源重建。
- **验收标准**：窗口拖拽改变尺寸后，渲染内容无撕裂且正确缩放，无资源泄漏。

### 阶段 7：性能调试工具集成

- **任务 7.1**：在 `DirectX12DeviceConfig` 中添加 `enable_pix_markers` 与 `enable_gpu_validation` 开关。
- **任务 7.2**：封装 PIX 事件标记宏，在关键渲染 Pass 添加标记（如 `BeginEvent("DrawPBRModel")`）。
- **任务 7.3**：引入帧时间统计，输出平均 FPS 与 GPU 时间戳到 Debug 输出或文件日志。
- **任务 7.4**：补充 GPU 验证层错误捕获，在调试构建中默认启用。
- **验收标准**：通过 PIX 工具可清晰查看各 Pass 耗时，GPU 验证层可捕获资源屏障错误。

### 阶段 8：多 Pass 渲染示例

- **任务 8.1**：设计延迟渲染管线（GBuffer Pass + Lighting Pass），演示多离屏目标协作。
- **任务 8.2**：提供后处理 Pass 示例（如 Bloom、Tone Mapping），展示 `ScreenQuad` 与离屏纹理的复用。
- **任务 8.3**：编写使用文档，说明如何组合多个 `RenderTargetHandle` 与自定义材质。
- **验收标准**：延迟渲染管线稳定运行，后处理效果正确，代码注释清晰易懂。

### 阶段 9：Shader 热重载（中优先级）

- **任务 9.1**：扩展 `ShaderLoader`，添加文件监听功能（Windows API `FindFirstChangeNotification`）。
- **任务 9.2**：实现 PSO 与根签名的运行时重建，确保资源绑定兼容性。
- **任务 9.3**：提供热重载失败回滚机制，避免崩溃或渲染错误。
- **验收标准**：修改 HLSL 文件后，无需重启程序即可看到效果更新，失败时恢复旧版本 Shader。

### 阶段 10：多光源场景实战验证（中优先级）

- **任务 10.1**：在 `Graphics::Initialize` 中创建 2-3 个不同类型的光源（主方向光 + 点光源 + 补光）。
- **任务 10.2**：扩展 Shader（`lightPS.hlsl` 和 `pbrPS.hlsl`），支持多光源数组的常量缓冲传递。
- **任务 10.3**：修改材质类的 `UpdateFromLight`，支持传递多光源数据到 GPU。
- **任务 10.4**：在场景中验证多光源效果，确保光照累加正确、无性能问题。
- **验收标准**：场景中可见多个光源的综合照明效果，帧率稳定，光照计算正确。
