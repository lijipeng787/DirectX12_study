# DirectX12 渲染架构技术路线图 (Technical Roadmap)

## 1. 路线图概览
本路线图旨在规划项目从“基础重构”向“成熟实验平台”演进的路径。我们坚持**“灵活性优先”**原则，确保架构能低成本地接入新技术。

## 2. 阶段演进

### ✅ 阶段 1：基础设施重构 (Infrastructure Refactoring)
*状态: 已完成 (2025 Q4)*

- **目标**: 消除历史债务，建立现代 C++ 资源管理与模块化架构。
- **关键成果**:
  - 单例移除与 RAII 资源管理。
  - 帧资源环 (Frame Resources) 实现并行渲染。
  - 统一光照系统 (LightManager)。
  - 实时反射管线验证 (ReflectionScene)。

### 🔄 阶段 2：交互性与调试体系 (Interaction & Debugging)
*状态: 进行中 (2026 Q1)*

- **目标**: 提升开发体验，支持动态环境调整与深度性能分析。
- **核心任务**:
  1.  **窗口系统重构**: 支持 Resolution Scaling, Window Resize, Fullscreen 切换。(优先级: **最高**)
  2.  **调试增强**: 集成 PIX Event Markers，GPU Timestamp 查询。
  3.  **热重载 (Hot Reload)**: 实现 Shader 运行时编译与 PSO 动态替换。

### 📅 阶段 3：高级渲染管线 (Advanced Rendering Pipelines)
*状态: 规划中 (2026 Q2)*

- **目标**: 验证架构对复杂多 Pass 渲染的支持能力。
- **核心任务**:
  1.  **延迟渲染 (Deferred Shading)**: 实现 G-Buffer 生成与光照合成 Pass。
  2.  **后处理栈 (Post-Process Stack)**: 串联 Bloom, Tone Mapping, FXAA 等效果。
  3.  **计算着色器 (Compute Shader)**: 引入 Compute Queue，实验 GPU 粒子或剔除算法。

### 📅 阶段 4：数据驱动与工具链 (Data-Driven & Tools)
*状态: 远期规划*

- **目标**: 降低硬编码比例，提升资产管线效率。
- **核心任务**:
  1.  **Render Graph**: 数据驱动的 Pass 依赖图构建。
  2.  **材质编辑器集成**: 简单的参数序列化与 UI 编辑。

## 3. 支撑措施
- **文档**: 持续更新 `arch_review.md` 与本路线图，确保设计决策可追溯。
- **测试**: 为核心数学库与资源加载器添加单元测试。
- **规范**: 严格执行 C++17 标准与 Google 代码风格。

## 4. 优先级总结
1.  **Immediate (本周)**: 窗口 Resize 适配 (OnResize)。
2.  **High (本月)**: PIX 集成与性能统计。
3.  **Medium (下月)**: Shader 热重载。
4.  **Low (按需)**: 延迟渲染与 Compute Shader。
