/**
 * @file element_bank_panel.h
 * @brief Panel providing a bank of common elements for computer science
 *        architecture diagrams.
 *
 * The ElementBankPanel displays categorized, pre-built diagram elements such
 * as servers, databases, clients, firewalls, and cloud components.  The user
 * clicks an element to place it on the canvas.
 */
#ifndef ELEMENT_BANK_PANEL_H
#define ELEMENT_BANK_PANEL_H

#include <QDockWidget>
#include <QString>
#include <QVector>

class QToolButton;
class QVBoxLayout;

/**
 * @brief Describes a single element in the bank.
 */
struct ElementInfo {
  QString id;       ///< Unique element identifier (e.g. "server")
  QString label;    ///< Display label shown under the icon
  QString icon;     ///< Unicode glyph used as visual icon
  QString tooltip;  ///< Tooltip text for hover
  QString category; ///< Category the element belongs to
};

/**
 * @brief Dock-widget panel that exposes a library of reusable CS architecture
 *        diagram elements.
 *
 * Elements are organised into collapsible categories.  Clicking an element
 * emits the elementSelected() signal which the canvas uses to create the
 * appropriate shape group at the viewport centre.
 */
class ElementBankPanel : public QDockWidget {
  Q_OBJECT

public:
  explicit ElementBankPanel(QWidget *parent = nullptr);

signals:
  /**
   * @brief Emitted when the user clicks an element in the bank.
   * @param elementId Identifier of the selected element.
   */
  void elementSelected(const QString &elementId);

private:
  /// Populate the built-in element library.
  static QVector<ElementInfo> defaultElements();

  /// Build the UI for one category inside the given layout.
  void addCategory(QVBoxLayout *layout, const QString &category,
                   const QVector<ElementInfo> &elements);
};

#endif // ELEMENT_BANK_PANEL_H
