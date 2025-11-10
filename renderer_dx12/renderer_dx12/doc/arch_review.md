# DirectX12 渲染架构评审

## 架构设计原则 （作为辅助编程的AI，你不应该修改这一部分）

这个项目的目标是完成一个灵活的，具有高可用性的渲染架构，可以方便得试验各种新渲染技术，一个工业级的游戏/渲染引擎不是这个项目的目标，进行渲染架构技术review时必须要考虑这一点，不要过度在意大型项目使用这个架构的问题，但是review必须严格，不留颜面。

## 核心问题（按现状排序）

- ~~**光照系统设计存在概念混淆（严重问题）**~~：✅ **已解决** - 已完成统一光照系统重构，详见"已完成的改进"章节。

- **帧资源体系进展良好**：已完成 per-frame command allocator 与 fence 生命周期管理（`FrameResource` 结构），`WaitForPreviousFrame` 与 `WaitForGpuIdle` 同步机制运作正常。常量缓冲目前由各材质类独立管理，暂无统一资源池，但对于实验性架构已基本满足需求。建议：补充多 Pass 渲染示例与资源复用模式文档。
- **命令队列基本完备**：Direct 和 Copy 队列已稳定运行，compute 队列暂未引入但接口预留清晰。对于当前 PBR/Texture/Font 渲染管线已足够，无紧急需求。建议：如后续引入计算着色器实验，再按需扩展。
- **窗口/配置热重建缺失**：未支持 Resize、MSAA、HDR 等动态更新，实验新技术时需要重启程序。这是当前最大的可用性瓶颈。建议：优先实现窗口 Resize 与交换链重建逻辑。
- **日志与错误处理持续改善**：初始化阶段的 `DxgiResourceManager` 日志详细且规范，`LogInitializationFailure` 覆盖主要失败路径。运行期缺少性能统计与 GPU 事件追踪。建议：引入可选的 PIX 标记与帧时间统计输出。
- **离屏渲染与材质系统设计优秀**：`RenderTargetDescriptor`、`CreateRenderTarget`、`ScreenQuad` 材质注入、外部 CBV 支持等均已落地，灵活性高。当前 `default_offscreen_handle_` 机制运作良好，支持多目标实验。建议：补充 UAV/DSV 创建接口与使用示例。

## 已完成的改进

- ✅ 单例移除：`DirectX12Device` 通过 `DirectX12DeviceConfig` + `Create` 工厂实例化，可多实例并使用 `std::shared_ptr` 管理。
- ✅ 初始化模块化：设备、队列、交换链、离屏资源等拆分为独立私有方法，返回 `HRESULT` 便于溯源。
- ✅ 资源管理统一：关键 DXGI/D3D12 资源由 `ComPtr` / 管理器负责，辅助日志输出。
- ✅ 离屏渲染抽象：`RenderTargetDescriptor`、`CreateRenderTarget`、句柄化的 `BeginDrawToOffScreen`/`EndDrawToOffScreen` 已落地。
- ✅ ScreenQuad 解耦：材质可由外部注入并复用，支持替换 vertex/index buffer 视图及外部 CBV。
- ✅ **统一光照系统重构（2025-11-10 完成）**：
  - 创建 `Lighting::SceneLight` 统一光源类，支持方向光/点光源/聚光灯，包含完整参数（方向、颜色、强度、类型、衰减等）
  - 创建 `Lighting::LightManager` 多光源管理器，支持动态添加/删除/查询光源，最多支持8个光源
  - 移除 `Graphics` 类中的冗余字段（`light_` 和 `pbr_light_direction_`），统一使用 `LightManager`
  - 为 `ModelMaterial` 和 `PBRMaterial` 添加 `UpdateFromLight(SceneLight*)` 接口，从统一光源按需提取参数
  - Cube（Blinn-Phong）和 PBR Sphere 现在共用同一个光源，保证光照一致性，符合 DRY 原则

## 待推进事项

1. ~~**统一光照系统重构（高优先级）**~~：✅ **已完成**
   - ✅ 已设计 `SceneLight` 统一光源类，包含完整参数（方向、颜色、强度、类型、衰减等）
   - ✅ 已重构 `Graphics` 类，移除 `pbr_light_direction_` 冗余字段，统一使用 `LightManager`
   - ✅ 各材质类已提供 `UpdateFromLight` 接口，从统一光源按需提取参数
   - ✅ 已实现 `LightManager`，支持点光源/方向光/聚光灯数组管理

2. **窗口热重建（高优先级）**：实现 `OnResize` 接口，支持交换链、深度缓冲、RTV 的安全重建与资源迁移，提升实验迭代效率。

3. **资源池示例补充**：为 `FrameResource` 添加常量缓冲池管理示例，展示跨帧资源复用模式，减轻实验者理解负担。

4. **日志系统增强**：引入可选的性能统计（帧时间、GPU 占用）与 PIX 事件标记，辅助性能调试与分析。

5. **离屏渲染扩展**：补充 UAV（用于计算着色器）与 DSV（深度模板视图）创建接口，完善 `RenderTargetDescriptor` 能力。

6. **多材质/多 Pass 示例**：提供延迟渲染或后处理管线的参考实现，展示 `ScreenQuad` 与离屏目标的协同工作流程。

## 开放问题

- ~~**光照模型统一 vs 分离**~~：✅ **已解决** - 已实现统一光源数据存储（`SceneLight` + `LightManager`），材质通过 `UpdateFromLight` 按需提取与转换，成功平衡了灵活性与维护性。
- **多光源实战验证**：当前 `LightManager` 已支持最多8个光源的数组管理，但尚未在实际场景中测试多光源渲染。建议：先实现简单的2-3光源场景（如主光源+补光），验证常量缓冲传递与 Shader 兼容性。
- **光照剔除与优化**：多光源场景下是否需要引入光照剔除（Tiled/Clustered Lighting）？建议：待多光源实战验证后，根据性能瓶颈决定优化策略。
- **RenderGraph 需求评估**：当前架构已支持离屏渲染与多材质组合，是否需要引入完整的 RenderGraph 系统？建议：先通过手动编排多 Pass 示例验证需求，再决定是否抽象。
- **资源热重载优先级**：Shader 热重载对快速实验有显著价值，但需处理 PSO 重建与根签名兼容性。建议：优先实现 Shader 文件变更监听与 PSO 重建，纹理/模型热重载可延后。
- **多 GPU 支持必要性**：当前单例移除已为多实例奠定基础，但多 GPU 调度与资源迁移复杂度高。建议：除非明确实验需求，否则暂不引入。
- **调试工具集成度**：PIX 标记、GPU 验证层、调试层是否默认启用？建议：通过 `DirectX12DeviceConfig` 提供调试开关，避免影响发布构建性能。

## 总体结论

核心框架已完成关键里程碑：单例拆除、初始化模块化、帧资源环管理、离屏渲染抽象、材质解耦等均已落地并稳定运行。当前架构在"可实验性"维度表现优秀，支持 PBR、Texture、Font 等多种材质的灵活组合与离屏渲染实验。

**主要优势**：
- 资源管理清晰，`ComPtr` 与 `DxgiResourceManager` 减少内存泄漏风险
- 帧同步机制健壮，`WaitForPreviousFrame` 与 `WaitForGpuIdle` 运作良好
- 离屏渲染与材质注入灵活度高，易于扩展新渲染技术
- **统一光照系统架构优秀**：`SceneLight` + `LightManager` 提供清晰的光源管理，`UpdateFromLight` 接口实现材质解耦，为多光源实验奠定基础

**核心短板**：
- 窗口热重建缺失，限制快速迭代能力（当前最高优先级）
- 运行期日志与性能统计薄弱，调试体验有待提升
- 缺少多 Pass 渲染参考实现，降低复杂管线的上手效率
- 多光源场景尚未实战验证，Shader 需适配多光源数组传递

**演进建议**：
1. **短期**：实现窗口 Resize 与交换链重建，补充 UAV/DSV 接口
2. **中期**：引入 Shader 热重载与性能统计工具，提供延迟渲染示例，扩展多光源场景实验（基于已完成的 LightManager）
3. **长期**：根据实际实验需求评估 RenderGraph 与计算队列的引入时机

整体架构已具备高可用性基础，后续应专注于提升实验迭代速度与调试便利性，而非过度抽象化。
