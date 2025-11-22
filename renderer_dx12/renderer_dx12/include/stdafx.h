#pragma once

#include "targetver.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// Standard library
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <map>
#include <unordered_map>

// Local
#include "Logger.h"
#include "TypeDefine.h"