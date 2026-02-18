/**
 * @file element_bank_panel.cpp
 * @brief Implementation of the ElementBankPanel widget.
 */
#include "element_bank_panel.h"
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static QFrame *createBankSeparator(QWidget *parent) {
  auto *line = new QFrame(parent);
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Sunken);
  line->setStyleSheet("QFrame { background-color: rgba(255, 255, 255, 0.1); "
                      "max-height: 1px; margin: 4px 8px; }");
  return line;
}

// ---------------------------------------------------------------------------
// Default element library
// ---------------------------------------------------------------------------

QVector<ElementInfo> ElementBankPanel::defaultElements() {
  return {
      {"client", "Client", "💻", "Client application", "Architecture"},
      {"load_balancer", "Load Bal.", "⚖", "Load balancer", "Architecture"},
      {"api_gateway", "Gateway", "⛩", "API gateway", "Architecture"},
      {"app_server", "App Server", "⬡", "Application server / microservice",
       "Architecture"},
      {"cache", "Cache", "⧖", "Cache (Redis, Memcached)", "Architecture"},
      {"message_queue", "Queue", "☰", "Message queue / broker", "Architecture"},
      {"database", "Database", "⛁", "Database", "Architecture"},
      {"object_storage", "Storage", "🗄", "Object / file storage",
       "Architecture"},
      {"auth", "Auth", "🔐", "Authentication / identity service",
       "Architecture"},
      {"monitoring", "Monitor", "📊", "Monitoring / logging system",
       "Architecture"},
  };
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

ElementBankPanel::ElementBankPanel(QWidget *parent)
    : QDockWidget("Elements", parent) {
  setObjectName("ElementBankPanel");
  setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable |
              QDockWidget::DockWidgetFloatable);
  setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

  // Scroll area
  auto *scrollArea = new QScrollArea(this);
  scrollArea->setWidgetResizable(true);
  scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scrollArea->setFrameShape(QFrame::NoFrame);

  auto *container = new QWidget(scrollArea);
  auto *mainLayout = new QVBoxLayout(container);
  mainLayout->setContentsMargins(8, 8, 8, 8);
  mainLayout->setSpacing(6);
  mainLayout->setAlignment(Qt::AlignTop);

  // Group elements by category, preserving insertion order.
  QVector<ElementInfo> all = defaultElements();
  QVector<QString> orderedCategories;
  QHash<QString, QVector<ElementInfo>> groups;
  for (const auto &e : all) {
    if (!groups.contains(e.category)) {
      orderedCategories.append(e.category);
    }
    groups[e.category].append(e);
  }

  for (int i = 0; i < orderedCategories.size(); ++i) {
    const QString &cat = orderedCategories[i];
    addCategory(mainLayout, cat, groups[cat]);
    if (i + 1 < orderedCategories.size()) {
      mainLayout->addWidget(createBankSeparator(container));
    }
  }

  mainLayout->addStretch();
  scrollArea->setWidget(container);
  setWidget(scrollArea);
  setFixedWidth(200);

  // Styling – consistent with the existing ToolPanel dark theme.
  setStyleSheet(R"(
    QDockWidget {
      background-color: #1a1a1e;
      color: #f8f8fc;
      font-weight: 500;
    }
    QDockWidget::title {
      background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                  stop:0 #2a2a30, stop:1 #242428);
      padding: 10px 12px;
      font-weight: 600;
      border-bottom: 1px solid rgba(255, 255, 255, 0.06);
    }
    QScrollArea {
      background-color: #1a1a1e;
      border: none;
    }
    QToolButton {
      background-color: rgba(255, 255, 255, 0.06);
      color: #e0e0e6;
      border: 1px solid rgba(255, 255, 255, 0.08);
      border-radius: 6px;
      padding: 4px;
      min-width: 52px;
      min-height: 52px;
      max-width: 52px;
      max-height: 52px;
      font-weight: 500;
      font-size: 10px;
    }
    QToolButton:hover {
      background-color: rgba(255, 255, 255, 0.1);
      border: 1px solid rgba(59, 130, 246, 0.3);
    }
    QToolButton:pressed {
      background-color: rgba(59, 130, 246, 0.25);
    }
  )");
}

// ---------------------------------------------------------------------------
// Category builder
// ---------------------------------------------------------------------------

void ElementBankPanel::addCategory(QVBoxLayout *layout, const QString &category,
                                   const QVector<ElementInfo> &elements) {
  // Category heading
  auto *heading = new QLabel(category, this);
  heading->setStyleSheet(
      "QLabel { color: #a0a0a8; font-size: 11px; font-weight: 600; "
      "padding: 2px 0px; }");
  heading->setAlignment(Qt::AlignLeft);
  layout->addWidget(heading);

  // Grid of element buttons (3 columns)
  auto *gridWidget = new QWidget(this);
  auto *grid = new QGridLayout(gridWidget);
  grid->setSpacing(4);
  grid->setContentsMargins(0, 0, 0, 0);

  for (int i = 0; i < elements.size(); ++i) {
    const ElementInfo &info = elements[i];

    auto *btn = new QToolButton(gridWidget);
    btn->setText(info.icon + "\n" + info.label);
    btn->setToolTip(info.tooltip);
    btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btn->setFixedSize(52, 52);
    btn->setIconSize(QSize(18, 18));

    connect(btn, &QToolButton::clicked, this,
            [this, id = info.id]() { emit elementSelected(id); });

    grid->addWidget(btn, i / 3, i % 3);
  }

  gridWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  layout->addWidget(gridWidget, 0, Qt::AlignHCenter);
}
