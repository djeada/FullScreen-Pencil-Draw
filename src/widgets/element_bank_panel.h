/**
 * @file element_bank_panel.h
 * @brief Panel providing a bank of diagram elements.
 *
 * The ElementBankPanel is a single dock widget that lets the user switch
 * between the Architecture and Electronics domains via a custom-painted
 * segmented control.  Elements are shown in collapsible categories using
 * custom-painted cards with smooth hover / press feedback.
 */
#ifndef ELEMENT_BANK_PANEL_H
#define ELEMENT_BANK_PANEL_H

#include <QDockWidget>
#include <QHash>
#include <QPainterPath>
#include <QString>
#include <QVariantAnimation>
#include <QVector>
#include <QWidget>

class QScrollArea;
class QVBoxLayout;

// ---------------------------------------------------------------------------
// ElementInfo – metadata for a single element
// ---------------------------------------------------------------------------

/**
 * @brief Describes a single element in the bank.
 */
struct ElementInfo {
  QString id;       ///< Unique element identifier (e.g. "server")
  QString label;    ///< Display label shown under the icon
  QString icon;     ///< Resource path to SVG icon
  QString tooltip;  ///< Tooltip text for hover
  QString category; ///< Category the element belongs to
};

// ---------------------------------------------------------------------------
// ElementCard – a single custom-painted element tile
// ---------------------------------------------------------------------------

/**
 * @brief Custom-painted element tile with icon, label, and animated feedback.
 */
class ElementCard : public QWidget {
  Q_OBJECT
public:
  explicit ElementCard(const ElementInfo &info, QWidget *parent = nullptr);

signals:
  void clicked(const QString &elementId);

protected:
  void paintEvent(QPaintEvent *event) override;
  void enterEvent(QEnterEvent *event) override;
  void leaveEvent(QEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;

private:
  void rebuildIconPixmap();

  ElementInfo info_;
  QIcon icon_;
  QPixmap iconPixmap_;
  qreal iconDpr_ = 1.0;
  bool iconDark_ = false;
  QVariantAnimation hoverAnim_;
  QVariantAnimation pressAnim_;
  qreal hoverProgress_ = 0.0;
  qreal pressProgress_ = 0.0;
  bool pressed_ = false;
};

// ---------------------------------------------------------------------------
// CategorySection – collapsible category header + grid of cards
// ---------------------------------------------------------------------------

class QGridLayout;

/**
 * @brief Collapsible section: a clickable header label + a grid of
 *        ElementCard widgets.
 */
class CategorySection : public QWidget {
  Q_OBJECT
public:
  explicit CategorySection(const QString &title,
                           const QVector<ElementInfo> &elements,
                           QWidget *parent = nullptr);

  void setCollapsed(bool collapsed);
  bool isCollapsed() const { return collapsed_; }

signals:
  void elementClicked(const QString &elementId);

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;

private:
  void rebuildGrid();

  QString title_;
  QVector<ElementInfo> elements_;
  bool collapsed_ = false;
  QWidget *cardContainer_;
  QGridLayout *cardGrid_;
  QVariantAnimation chevronAnim_;
  qreal chevronAngle_ = 0.0; ///< 0 = expanded (▼), 90 = collapsed (▶)
};

// ---------------------------------------------------------------------------
// DomainSwitcher – segmented control for Architecture / Electronics
// ---------------------------------------------------------------------------

/**
 * @brief Custom-painted segmented control with animated selection indicator.
 */
class DomainSwitcher : public QWidget {
  Q_OBJECT
public:
  explicit DomainSwitcher(QWidget *parent = nullptr);

  int currentIndex() const { return currentIndex_; }
  void setCurrentIndex(int index);

signals:
  void indexChanged(int index);

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;

private:
  int currentIndex_ = 0;
  QStringList labels_ = {"Architecture", "Electronics"};
  QVariantAnimation slideAnim_;
  qreal indicatorX_ = 0.0;
};

// ---------------------------------------------------------------------------
// ElementBankPanel – the dock widget container
// ---------------------------------------------------------------------------

/**
 * @brief Dock-widget panel that exposes a library of reusable diagram
 *        elements.
 *
 * A single panel covers both Architecture and Electronics domains, with a
 * segmented control at the top to switch between them.  Elements are
 * organised into collapsible categories with custom-painted cards.
 */
class ElementBankPanel : public QDockWidget {
  Q_OBJECT

public:
  /// Which element domain this panel shows.
  enum class Domain { Architecture, Electronics };

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

  /// Return only elements belonging to the given domain.
  static QVector<ElementInfo> elementsForDomain(Domain domain);

  void switchDomain(int index);
  void buildDomainContent(Domain domain);
  void applyTheme();

  DomainSwitcher *switcher_;
  QScrollArea *scrollArea_;
  QWidget *contentWidget_;
  QVBoxLayout *contentLayout_;
  Domain currentDomain_ = Domain::Architecture;
};

#endif // ELEMENT_BANK_PANEL_H
