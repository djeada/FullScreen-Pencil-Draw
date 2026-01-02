/**
 * @file tool_manager.h
 * @brief Tool manager for centralized tool management.
 */
#ifndef TOOL_MANAGER_H
#define TOOL_MANAGER_H

#include <QObject>
#include <map>
#include <memory>

class Tool;
class Canvas;

/**
 * @brief Manages all drawing tools and tool switching.
 *
 * The ToolManager provides a centralized way to register, activate,
 * and manage tools. It ensures proper cleanup when switching tools.
 */
class ToolManager : public QObject {
  Q_OBJECT

public:
  /**
   * @brief Tool type identifiers
   */
  enum class ToolType {
    Pen,
    Eraser,
    Text,
    Fill,
    Line,
    Arrow,
    Rectangle,
    Circle,
    Selection,
    Pan
  };
  Q_ENUM(ToolType)

  explicit ToolManager(Canvas *canvas, QObject *parent = nullptr);
  ~ToolManager() override;

  /**
   * @brief Register a tool with the manager
   * @param type The tool type identifier
   * @param tool The tool instance (ownership transferred to manager)
   */
  void registerTool(ToolType type, std::unique_ptr<Tool> tool);

  /**
   * @brief Set the active tool
   * @param type The tool type to activate
   */
  void setActiveTool(ToolType type);

  /**
   * @brief Get the currently active tool
   * @return Pointer to the active tool, or nullptr if none
   */
  Tool *activeTool() const { return activeTool_; }

  /**
   * @brief Get a tool by type
   * @param type The tool type
   * @return Pointer to the tool, or nullptr if not found
   */
  Tool *tool(ToolType type) const;

  /**
   * @brief Get the active tool type
   * @return The currently active tool type
   */
  ToolType activeToolType() const { return activeToolType_; }

signals:
  /**
   * @brief Emitted when the active tool changes
   * @param type The new active tool type
   * @param tool Pointer to the new active tool
   */
  void toolChanged(ToolType type, Tool *tool);

private:
  Canvas *canvas_;
  Tool *activeTool_;
  ToolType activeToolType_;
  std::map<ToolType, std::unique_ptr<Tool>> tools_;

  void initializeTools();
};

#endif // TOOL_MANAGER_H
