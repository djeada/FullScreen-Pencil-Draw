// tool.cpp
#include "tool.h"

Tool::Tool(Canvas *canvas) : canvas_(canvas) {}

Tool::~Tool() = default;

void Tool::activate() {}

void Tool::deactivate() {}
