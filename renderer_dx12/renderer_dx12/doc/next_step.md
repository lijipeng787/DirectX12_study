# DirectX12 渲染架构实施计划 (Implementation Plan)

## 1. 阶段 1：解耦与基础重构

---

## 2. 阶段 2：修复基础缺陷 (当前最高优先级)

**状态：进行中**

本阶段必须修复阻碍项目作为"实验平台"的基础问题。**在这些问题解决前，禁止添加任何新渲染特性。**

### 🚨 任务 2.1：修复资源加载 [P0 - Critical]
**预计工作量：2天**

- [x] **重构 `DirectX12Device`**：
    - [x] 暴露或提供一个共享的 `CommandQueue` (Copy 或 Direct) 用于资源上传。
    - [x] 实现一个简单的 `UploadContext` 或 `ResourceUploader` 类，管理临时的 Upload Heap 和 Command List。
- [x] **重构 `Model.cpp`**：
    - [x] 删除 `CreateBufferOnGpu` 中创建临时 Command Queue/Allocator 的代码。
    - [x] 改用设备提供的上传上下文。
- [x] **重构 `TextureLoader.cpp`**：
    - [x] 删除 `LoadTexturesByNameArray` 和 `CreateTextureFromTga` 中创建临时 Queue 的代码。
    - [x] 统一使用新的上传机制。
- [ ] **验收标准**: 
  - 加载场景时 PIX 捕获显示不再有数百个 Command Queue 创建销毁。
  - 启动速度显著提升。

### ⚠️ 任务 2.2：建立测试基础设施 [P1 - High， 暂时延后]
**预计工作量：3-4天**

- [ ] 添加 Google Test 或 Catch2
- [ ] 添加基础单元测试：
  - [ ] 数学库测试（矩阵、向量运算）
  - [ ] 资源加载测试（模型解析、纹理加载）
  - [ ] 光照系统测试（LightManager）
- [ ] 添加集成测试：
  - [ ] 设备创建/销毁测试
  - [ ] 简单场景渲染测试（无窗口，离屏渲染）
- [ ] 集成到CI（GitHub Actions）
- [ ] **验收标准**: 
  - 至少30%代码覆盖率
  - 所有测试在CI中自动运行

### ✅ 任务 2.3：资源管理系统 [P1 - High]
**预计工作量：2-3天**

- [x] 实现 `ResourceManager` 类：
  - [x] 纹理缓存（基于文件路径）
  - [x] 模型缓存
  - [x] 引用计数管理 (通过 shared_ptr)
- [x] 添加资源统计和调试信息 (GetTextureCount/GetModelCount)
- [x] **验收标准**: 
  - 加载场景时 PIX 捕获显示不再有数百个 Command Queue 创建销毁。 (Already done in 2.1)
  - 多次加载同一纹理只占用1份显存
  - 可查询当前加载的所有资源

### ⚠️ 任务 2.4：日志系统 [P1 - High，暂时延后]
**预计工作量：半天**

- [ ] 集成 spdlog
- [ ] 替换所有 `OutputDebugString` 调用
- [ ] **验收标准**: 
  - 日志有时间戳和来源信息
  - Release构建中Debug日志被优化掉

### 📋 任务 2.5：构建系统现代化 [P2 - Medium，暂时延后]
**预计工作量：1-2天**

- [ ] 添加 CMakeLists.txt
- [ ] 支持 MSVC
- [ ] 添加 vcpkg manifest 管理依赖
- [ ] **验收标准**: 
  - 可以用CMake在Windows构建

---

## 3. 阶段 3：现代化准备 (基础完成后)

**前置条件：完成阶段2的所有P0和P1任务**

### 任务 3.1：升级 Shader Model [P2]
- [x] 将 Shader 编译目标从 `vs_5_0`/`ps_5_0` 升级到 `vs_5_1`/`ps_5_1` 或 `vs_6_0`/`ps_6_0`。
- [x] 验证现有 Shader 的兼容性。

### 任务 3.2：PIX集成 [P2]
- [x] 添加PIX事件标记（`PIXBeginEvent`/`PIXEndEvent`）
- [x] 标记所有主要渲染pass

## 4. 阶段 4：高级渲染特性 (远期)

**前置条件：完成阶段2和3的所有任务**

⚠️ **警告**：在基础功能完善前，**禁止**添加以下特性：
- ❌ 延迟渲染
- ❌ Compute shader
- ❌ Render Graph
- ❌ Bindless Rendering

**原因**：地基不牢，继续盖楼只会增加技术债务。

## 验收标准更新

### 阶段2验收标准
- **资源加载**: 
  - ✅ **彻底消除临时 Command Queue 的创建**。
  - ✅ 资源上传使用批处理或共享队列。
  
- **测试**: 
  - ✅ CI自动运行所有测试。
  
- **资源管理**: 
  - ✅ 资源去重加载。

### 阶段3验收标准
- **Shader**: 
  - ✅ Shader Model >= 5.1。
