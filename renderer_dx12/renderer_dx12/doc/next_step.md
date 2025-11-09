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

- 定义 `RenderTargetDescriptor`（尺寸、格式、用途），允许外部请求离屏纹理。
- 重构 `BeginDrawToOffScreen/EndDrawToOffScreen`，改为操作外部提供的目标句柄。
- 针对现有 ScreenQuad 保持兼容，提供默认 off-screen 目标的便捷工厂。

## 任务 5：ScreenQuad 解耦材质

- 将 `ScreenQuadMaterial` 从 `ScreenQuad` 类中分离，允许通过构造或 setter 注入。
- 暴露 `VertexBufferView`/`IndexBufferView` 的更新接口，方便实验不同顶点布局。
- 整理常量缓冲接口，允许自定义布局或绑定外部 CBV 资源。

## 验收标准

- ✅ 完成以上任务后，可在不依赖单例的情况下初始化渲染设备并渲染 ScreenQuad。
- ⏳ 设备初始化任意阶段失败时，资源可自动释放，日志输出含错误码与描述。（待完善）
- ⏳ Off-screen 渲染目标可通过配置调整尺寸与格式，ScreenQuad 支持绑定自定义材质。（待完善）
