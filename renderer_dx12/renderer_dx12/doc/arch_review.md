# DirectX12 渲染架构评审

## 架构设计原则 （作为辅助编程的AI，你不应该修改这一部分）

这个项目的目标是完成一个灵活的，具有高可用性的渲染架构，可以方便得试验各种新渲染技术，一个工业级的游戏/渲染引擎不是这个项目的目标，进行渲染架构技术review时必须要考虑这一点，不要过度在意大型项目使用这个架构的问题，但是review必须严格，不留颜面。

## 评审对象

- `DirectX12Device.cpp`
- `ScreenQuad.h`

## 核心问题（严重程度由高到低）

- **单例限制扩展性**：`DirectX12Device::GetD3d12DeviceInstance()` 使用进程级单例且缺乏线程安全，无法支持多 GPU / 多设备并行实验，限制架构弹性。
- **初始化流程耦合**：`DirectX12Device::Initialize` 集成了适配器选择、交换链、命令队列、离屏资源等全部逻辑，失败时缺乏回滚与日志记录，难以替换单个环节进行实验。
- **帧资源与同步抽象缺失**：仅维护单个 `ID3D12Fence` 与 `frame_index_`，没有 per-frame 资源封装，难以支持多 Pass 或异步执行的研究场景。
- **离屏渲染路径固定**：`BeginDrawToOffScreen` / `EndDrawToOffScreen` 绑定唯一的 `off_screen_texture_`，尺寸与格式写死，无法灵活创建多 RenderTarget 或可变分辨率资源。
- **命令队列功能不足**：只创建 direct/ copy 队列且未暴露管理接口，缺少 compute 队列与多命令列表调度，难以尝试异步计算或复杂录制策略。
- **资源生命周期管理不统一**：`adapter->Release()`、`factory->Release()` 与大量 `ComPtr` 混用，遇到中途失败易泄漏，建议统一采用 RAII/智能指针。
- **ScreenQuad 封装僵硬**：`ScreenQuad` 将 `ScreenQuadMaterial` 内嵌为成员，几何与材质强耦合，限制复用与自定义 Shader/CBV 的自由度。

## 次要问题与改进建议

- `GetVideoCardInfo` 形参 `memory` 以值传递，调用者拿不到显存数值，应改用引用/指针或返回结构体。
- `frame_cout_` 拼写错误、`gpu_work_procress` 变量未使用等细节会干扰迭代效率。
- 错误处理缺乏日志输出，定位问题只能依赖断点。
- 未对窗口 Resize、MSAA、HDR 等可变条件预留接口。
- `ScreenQuadMaterial` 常量缓冲布局写死 `world/view/orthogonality_`，建议暴露自定义 CBV 结构或参数装配接口。

## 最新进展

- ✅ 单例访问接口已移除：`DirectX12Device` 通过 `DirectX12DeviceConfig` 与 `Create` 工厂创建，使用 `std::shared_ptr` 管理生命周期，并向 `ScreenQuad`、`Model`、`Text`、`TextureLoader` 等组件注入依赖。
- ✅ 渲染组件更新：所有依赖已改为通过设备实例创建资源和命令，`Graphics` 初始化/销毁流程同步调整。

## 开放问题

- 是否计划引入 RenderGraph 或 Pass 管线描述以提升组合能力？
- 是否需要支持 Shader/资源的运行时热重载以加快实验迭代？

## 总体结论

当前实现更像是静态 Demo 级别的 DX12 功能集合，距离“灵活、高可用、便于试验新技术”的架构目标仍有较大差距。建议优先解耦设备初始化、引入帧资源/队列/RenderTarget 等基础抽象，并移除单例与硬编码路径，为后续渲染技术实验建立弹性基础。
