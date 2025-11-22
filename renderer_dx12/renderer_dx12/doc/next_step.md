# DirectX12 渲染架构实施计划 (Implementation Plan)

## 1. 阶段 1：解耦与基础重构 (已完成)

本阶段主要目标是消除历史债务，建立现代化的 C++17 资源管理体系与模块化架构。

### ✅ 完成的任务
1.  **Device 构造重构**: 移除了单例，实现了 `DirectX12DeviceConfig` 配置化构造。
2.  **初始化模块化**: 拆分了 `CreateCommandQueues`, `CreateSwapChain` 等独立步骤，增强了错误处理。
3.  **资源管理统一**: 全面采用 `ComPtr` 管理 D3D12 接口，重构了 `DxgiResourceManager`。
4.  **离屏渲染抽象**: 实现了 `RenderTargetDescriptor` 与 `RenderTargetHandle` 系统，支持动态创建离屏目标。
5.  **材质解耦**: `ScreenQuad` 与 `Material` 分离，支持外部注入材质与常量缓冲。
6.  **统一光照系统**: 实现了 `Lighting::SceneLight` 与 `LightManager`，移除了冗余光照代码。
7.  **实时反射验证**: 落地了 `ReflectionScene`，验证了 Render-to-Texture 管线与自定义材质的可行性。

---

## 2. 阶段 2：交互性与调试增强 (当前重点)

本阶段目标是解决架构的“可用性”短板，特别是窗口交互与性能分析能力。

### 任务 2.1：窗口与交换链热重建 (Window Resize) [High Priority]
- [ ] **实现 OnResize 接口**: 在 `System` 类中监听 `WM_SIZE` 消息，传递给 `Graphics` 和 `DirectX12Device`。
- [ ] **交换链重建逻辑**: 
    - 确保 GPU 闲置 (`WaitForGpuIdle`)。
    - 释放旧的 SwapChain buffers 和 RTV heap。
    - 调用 `ResizeBuffers` 并重新创建 RTV。
- [ ] **深度缓冲适配**: 根据新尺寸重建 DepthStencil buffer。
- [ ] **投影矩阵更新**: 更新纵横比 (Aspect Ratio) 并重新计算投影矩阵。

### 任务 2.2：性能调试工具集成 (Performance Tools)
- [ ] **集成 PIX Markers**: 引入 `PIXBeginEvent`, `PIXEndEvent`，在 Command List 中标记 Render Pass（如 "Draw ShadowMap", "Draw PBR"）。
- [ ] **GPU 时间戳查询**: 封装 `ID3D12QueryHeap`，统计关键 Pass 的 GPU 耗时。
- [ ] **FPS 统计优化**: 在 UI 上显示详细的 CPU/GPU 帧时间 (ms)。

### 任务 2.3：Shader 热重载 (Hot Reload)
- [ ] **文件监听**: 使用 Win32 `FindFirstChangeNotification` 监听 `shader/` 目录。
- [ ] **自动重编译**: 检测到文件变更后，后台尝试编译 HLSL。
- [ ] **PSO 重建**: 若编译成功，安全地替换现有的 Pipeline State Object (PSO)，实现运行时效果更新。

---

## 3. 阶段 3：高级渲染特性 (未来规划)

### 任务 3.1：多光源实战与优化
- [ ] 在场景中放置 4-8 个不同颜色的点光源。
- [ ] 优化 Shader 中的循环与分支，评估性能开销。
- [ ] (可选) 引入 Tiled Lighting 或 Cluster Culling 算法实验。

### 任务 3.2：多 Pass 延迟渲染 (Deferred Rendering)
- [ ] 利用现有的 Off-screen 机制建立 G-Buffer (Albedo, Normal, Position/Depth)。
- [ ] 实现 Lighting Pass，读取 G-Buffer 进行光照计算。
- [ ] 验证架构对多 Render Target (MRT) 的支持能力。

## 验收标准 (Acceptance Criteria)
- **Resize**: 拖动窗口边缘，渲染内容无拉伸、无黑边，且程序不崩溃，显存无泄漏。
- **Debug**: 打开 PIX 工具截帧时，能清晰看到标记好的 Pass 结构。
- **Hot Reload**: 修改 `pbr.hlsl` 中的颜色输出并保存，程序运行窗口内立即反映颜色变化。
