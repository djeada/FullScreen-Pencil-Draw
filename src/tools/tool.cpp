/**
 * @file tool.cpp
 * @brief Abstract base class for all drawing tools implementation.
 */
#include "tool.h"

Tool::Tool(SceneRenderer *renderer) : renderer_(renderer) {}

Tool::~Tool() = default;

void Tool::activate() {}

void Tool::deactivate() {}
