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

#include <QByteArray>
#include <QColor>
#include <QJsonObject>
#include <QPixmap>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QStringList>

class QGraphicsItem;
class QGraphicsPixmapItem;
class QGraphicsScene;
class ItemStore;
class LayerManager;

enum class ProjectSaveError {
  None,
  InvalidArguments,
  UnsupportedContent, ///< Items with no editable representation
  SerializationFailed,
  OpenFailed,  ///< Missing directory, permissions, read-only media
  WriteFailed, ///< Short write, disk full
  CommitFailed,
  VerifyFailed ///< Written file did not read back identically
};

struct ProjectSaveOptions {
  /// The user approved flattening unsupported items into images.
  bool allowRasterFallback = false;
  /// The opened base image that sits outside every layer (optional).
  const QGraphicsPixmapItem *backgroundImage = nullptr;
};

struct ProjectSaveStatus {
  ProjectSaveError error = ProjectSaveError::None;
  QString message;              ///< Human readable, includes the path
  QStringList unsupportedItems; ///< Descriptions of offending items
  QStringList rasterizedItems;  ///< Items flattened with user approval
  bool ok() const { return error == ProjectSaveError::None; }
};

struct ProjectLoadExtras {
  int formatVersion = 0;
  bool hasBackgroundImage = false;
  QPixmap backgroundImage;
  QPointF backgroundImagePos;
  qreal backgroundImageZ = -1000;
};

/**
 * @brief Serializes and deserializes native project files.
 *
 * The .fspd format is a JSON document (see docs/FILE_FORMAT.md) storing:
 * - Canvas dimensions, background colour and the opened base image
 * - All layers with id, name, type, visibility, lock, opacity, blend mode
 * - Every item with a stable id, its layer order, common state (position,
 *   z, visibility, opacity, transform, rotation/scale, lock) and a
 *   type-specific payload. Vector items stay vectors; raster content
 *   (images, brush strokes, raster layer tiles) is stored as PNG.
 *
 * Saving never silently drops or flattens content: items without a native
 * representation make the save fail with SaveError::UnsupportedContent
 * unless the caller explicitly allows (user-approved) rasterization.
 */
class ProjectSerializer {
public:
  /// Current on-disk format. Version 1 files are still read.
  static constexpr int FORMAT_VERSION = 2;

  /**
   * @brief File extension for native project files
   */
  static constexpr const char *FILE_EXTENSION = ".fspd";

  /**
   * @brief File filter string for file dialogs
   */
  static const QString fileFilter();

  using SaveError = ProjectSaveError;
  using SaveOptions = ProjectSaveOptions;
  using SaveStatus = ProjectSaveStatus;
  using LoadExtras = ProjectLoadExtras;

  /**
   * @brief Save the current project state to a file
   *
   * The whole document is serialized and validated in memory first, then
   * written with QSaveFile: the target is only replaced after every byte
   * was written and committed, and the result is read back and compared.
   * On any failure the previous file is left untouched.
   *
   * @return true if saved successfully; details in @p status.
   */
  static bool saveProject(const QString &filePath, QGraphicsScene *scene,
                          ItemStore *itemStore, LayerManager *layerManager,
                          const QRectF &sceneRect,
                          const QColor &backgroundColor,
                          const SaveOptions &options = SaveOptions(),
                          SaveStatus *status = nullptr);

  /**
   * @brief Serialize the document to bytes without touching the disk.
   * @return Empty on failure (see @p status).
   */
  static QByteArray serializeProject(ItemStore *itemStore,
                                     LayerManager *layerManager,
                                     const QRectF &sceneRect,
                                     const QColor &backgroundColor,
                                     const SaveOptions &options = SaveOptions(),
                                     SaveStatus *status = nullptr);

  /**
   * @brief Atomically replace @p filePath with @p data and verify it.
   */
  static bool writeFileAtomically(const QString &filePath,
                                  const QByteArray &data,
                                  SaveStatus *status = nullptr);

  /**
   * @brief Items in the document that have no native representation and
   *        could only be saved by rasterizing them.
   */
  static QStringList findUnsupportedItems(ItemStore *itemStore,
                                          LayerManager *layerManager);

  /**
   * @brief Load a project from a file
   *
   * The file is parsed and every item is rebuilt before the current
   * document is touched, so a corrupt file leaves the open document as is.
   *
   * @param[out] sceneRect The loaded scene rectangle
   * @param[out] backgroundColor The loaded background color
   * @param[out] extras Base image and format information (optional)
   * @return true if loaded successfully
   */
  static bool loadProject(const QString &filePath, QGraphicsScene *scene,
                          ItemStore *itemStore, LayerManager *layerManager,
                          QRectF &sceneRect, QColor &backgroundColor,
                          LoadExtras *extras = nullptr,
                          QString *errorMessage = nullptr);

  /**
   * @brief Serialize one item (and its children) to the project format.
   * @param allowRasterFallback Flatten items without a native format into
   *        an image instead of failing (used for in-memory copies).
   * @param unsupported Receives a description of each item that has no
   *        native format (rasterized or skipped).
   * @return An empty object for items that cannot be represented.
   */
  static QJsonObject serializeItem(QGraphicsItem *item,
                                   bool allowRasterFallback = true,
                                   QStringList *unsupported = nullptr);

  /**
   * @brief Recreate an item from serializeItem() output.
   * @return A new, unregistered item, or nullptr (e.g. for wires, which need
   *         their elements and are only resolved by loadProject()).
   */
  static QGraphicsItem *deserializeItem(const QJsonObject &obj,
                                        QString *errorMessage = nullptr);

  /// Short user-facing description of an item ("brush stroke at 10, 20").
  static QString describeItem(const QGraphicsItem *item);

  /// Test hook: make the next writes fail at a given stage.
  enum class WriteFault { None, Open, ShortWrite, Commit };
  static void setWriteFaultForTesting(WriteFault fault);

private:
  static QJsonObject serializePen(const QPen &pen);
  static QPen deserializePen(const QJsonObject &obj);

  static QJsonObject serializeBrush(const QBrush &brush);
  static QBrush deserializeBrush(const QJsonObject &obj);

  static QJsonObject serializeTransform(const QTransform &t);
  static QTransform deserializeTransform(const QJsonObject &obj);
};

#endif // PROJECT_SERIALIZER_H
