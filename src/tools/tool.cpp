/**
 * @file tool.cpp
 * @brief Abstract base class for all drawing tools implementation.
 */
#include "tool.h"

Tool::Tool(Canvas *canvas) : canvas_(canvas) {}

Tool::~Tool() = default;

void Tool::activate() {}

void Tool::deactivate() {}
