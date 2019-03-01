#pragma once

#include "IDirect3DDevice9Hook.h"

void FixedFunctionStateToVertexShader(const DeviceState& state, IDirect3DVertexShader9Hook** const outVertShader, IDirect3DDevice9Hook* const dev);
void FixedFunctionStateToPixelShader(const DeviceState& state, IDirect3DPixelShader9Hook** const outPixelShader, IDirect3DDevice9Hook* const dev);
void SetFixedFunctionVertexShaderState(const DeviceState& state, IDirect3DDevice9Hook* const dev);
void SetFixedFunctionPixelShaderState(const DeviceState& state, IDirect3DDevice9Hook* const dev);
