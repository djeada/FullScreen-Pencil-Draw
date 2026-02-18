/**
 * @file project_serializer.h
 * @brief Serialization/deserialization of native project files (.fspd).
 *
 * ProjectSerializer handles saving and loading the complete state of a
 * layered drawing project, including all graphics items, layer structure,
 * and canvas properties.
 */
#ifndef PROJECT_SERIALIZER_H
#define PROJECT_SERIALIZER_H

#include <QColor>
#include <QJsonObject>
#include <QRectF>
#include <QString>

class QGraphicsItem;
class QGraphicsScene;
class ItemStore;
class LayerManager;

/**
 * @brief Serializes and deserializes native project files.
 *
 * The .fspd format is a JSON-based file that stores:
 * - Canvas dimensions and background color
 * - All layers with their properties (name, visibility, locked, opacity)
 * - All graphics items with type-specific data (paths, rects, ellipses,
 *   lines, pixmaps, text) including pen, brush, position, and transform.
 */
class ProjectSerializer {
public:
  /**
   * @brief File extension for native project files
   */
  static constexpr const char *FILE_EXTENSION = ".fspd";

  /**
   * @brief File filter string for file dialogs
   */
  static const QString fileFilter();

  /**
   * @brief Save the current project state to a file
   * @param filePath Path to the output file
   * @param scene The graphics scene to serialize
   * @param itemStore The item store for resolving items
   * @param layerManager The layer manager to serialize
   * @param sceneRect The scene rectangle
   * @param backgroundColor The canvas background color
   * @return true if saved successfully
   */
  static bool saveProject(const QString &filePath, QGraphicsScene *scene,
                          ItemStore *itemStore, LayerManager *layerManager,
                          const QRectF &sceneRect,
                          const QColor &backgroundColor);

  /**
   * @brief Load a project from a file
   * @param filePath Path to the input file
   * @param scene The graphics scene to populate
   * @param itemStore The item store for registering items
   * @param layerManager The layer manager to populate
   * @param[out] sceneRect The loaded scene rectangle
   * @param[out] backgroundColor The loaded background color
   * @return true if loaded successfully
   */
  static bool loadProject(const QString &filePath, QGraphicsScene *scene,
                          ItemStore *itemStore, LayerManager *layerManager,
                          QRectF &sceneRect, QColor &backgroundColor);

private:
  static QJsonObject serializeItem(QGraphicsItem *item);
  static QGraphicsItem *deserializeItem(const QJsonObject &obj);

  static QJsonObject serializePen(const QPen &pen);
  static QPen deserializePen(const QJsonObject &obj);

  static QJsonObject serializeBrush(const QBrush &brush);
  static QBrush deserializeBrush(const QJsonObject &obj);

  static QJsonObject serializeTransform(const QTransform &t);
  static QTransform deserializeTransform(const QJsonObject &obj);

  static constexpr int FORMAT_VERSION = 1;
};

#endif // PROJECT_SERIALIZER_H
