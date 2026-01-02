/**
 * @file tool_manager.cpp
 * @brief Tool manager implementation.
 */
#include "tool_manager.h"
#include "arrow_tool.h"
#include "circle_tool.h"
#include "eraser_tool.h"
#include "fill_tool.h"
#include "line_tool.h"
#include "pan_tool.h"
#include "pen_tool.h"
#include "rectangle_tool.h"
#include "selection_tool.h"
#include "text_tool.h"
#include "tool.h"

ToolManager::ToolManager(Canvas *canvas, QObject *parent)
    : QObject(parent), canvas_(canvas), activeTool_(nullptr),
      activeToolType_(ToolType::Pen) {
  initializeTools();
  setActiveTool(ToolType::Pen);
}

ToolManager::~ToolManager() = default;

void ToolManager::initializeTools() {
  registerTool(ToolType::Pen, std::make_unique<PenTool>(canvas_));
  registerTool(ToolType::Eraser, std::make_unique<EraserTool>(canvas_));
  registerTool(ToolType::Text, std::make_unique<TextTool>(canvas_));
  registerTool(ToolType::Fill, std::make_unique<FillTool>(canvas_));
  registerTool(ToolType::Line, std::make_unique<LineTool>(canvas_));
  registerTool(ToolType::Arrow, std::make_unique<ArrowTool>(canvas_));
  registerTool(ToolType::Rectangle, std::make_unique<RectangleTool>(canvas_));
  registerTool(ToolType::Circle, std::make_unique<CircleTool>(canvas_));
  registerTool(ToolType::Selection, std::make_unique<SelectionTool>(canvas_));
  registerTool(ToolType::Pan, std::make_unique<PanTool>(canvas_));
}

void ToolManager::registerTool(ToolType type, std::unique_ptr<Tool> tool) {
  tools_[type] = std::move(tool);
}

void ToolManager::setActiveTool(ToolType type) {
  auto it = tools_.find(type);
  if (it == tools_.end())
    return;

  // Deactivate current tool
  if (activeTool_) {
    activeTool_->deactivate();
  }

  // Activate new tool
  activeToolType_ = type;
  activeTool_ = it->second.get();

  if (activeTool_) {
    activeTool_->activate();
  }

  emit toolChanged(type, activeTool_);
}

Tool *ToolManager::tool(ToolType type) const {
  auto it = tools_.find(type);
  if (it != tools_.end()) {
    return it->second.get();
  }
  return nullptr;
}
