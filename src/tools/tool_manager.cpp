/**
 * @file tool_manager.cpp
 * @brief Tool manager implementation.
 */
#include "tool_manager.h"
#include "arrow_tool.h"
#include "bezier_tool.h"
#include "circle_tool.h"
#include "eraser_tool.h"
#include "fill_tool.h"
#include "lasso_selection_tool.h"
#include "line_tool.h"
#include "mermaid_tool.h"
#include "pan_tool.h"
#include "pen_tool.h"
#include "rectangle_tool.h"
#include "selection_tool.h"
#include "text_on_path_tool.h"
#include "text_tool.h"
#include "tool.h"

ToolManager::ToolManager(SceneRenderer *renderer, QObject *parent)
    : QObject(parent), renderer_(renderer), activeTool_(nullptr),
      activeToolType_(ToolType::Pen) {
  initializeTools();
  setActiveTool(ToolType::Pen);
}

ToolManager::~ToolManager() = default;

void ToolManager::initializeTools() {
  registerTool(ToolType::Pen, std::make_unique<PenTool>(renderer_));
  registerTool(ToolType::Eraser, std::make_unique<EraserTool>(renderer_));
  registerTool(ToolType::Text, std::make_unique<TextTool>(renderer_));
  registerTool(ToolType::Fill, std::make_unique<FillTool>(renderer_));
  registerTool(ToolType::Line, std::make_unique<LineTool>(renderer_));
  registerTool(ToolType::Arrow, std::make_unique<ArrowTool>(renderer_));
  registerTool(ToolType::Rectangle, std::make_unique<RectangleTool>(renderer_));
  registerTool(ToolType::Circle, std::make_unique<CircleTool>(renderer_));
  registerTool(ToolType::Selection, std::make_unique<SelectionTool>(renderer_));
  registerTool(ToolType::LassoSelection,
               std::make_unique<LassoSelectionTool>(renderer_));
  registerTool(ToolType::Pan, std::make_unique<PanTool>(renderer_));
  registerTool(ToolType::Mermaid, std::make_unique<MermaidTool>(renderer_));
  registerTool(ToolType::Bezier, std::make_unique<BezierTool>(renderer_));
  registerTool(ToolType::TextOnPath,
               std::make_unique<TextOnPathTool>(renderer_));
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
