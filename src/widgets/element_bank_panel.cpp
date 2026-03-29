/**
 * @file element_bank_panel.cpp
 * @brief Implementation of the ElementBankPanel widget with custom-painted
 *        element cards, collapsible categories, and a domain switcher.
 */
#include "element_bank_panel.h"
#include "../core/theme_manager.h"
#include <QEnterEvent>
#include <QGridLayout>
#include <QIcon>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QScrollBar>
#include <QSet>
#include <QVBoxLayout>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

/// Number of card columns in the element grid.
constexpr int kGridColumns = 2;

/// Card dimensions.
constexpr int kCardWidth = 108;
constexpr int kCardHeight = 72;
constexpr int kCardIconSize = 26;
constexpr qreal kCardRadius = 10.0;

/// Category header height.
constexpr int kCategoryHeaderHeight = 32;

/// Switcher dimensions.
constexpr int kSwitcherHeight = 36;
constexpr qreal kSwitcherRadius = 10.0;
constexpr qreal kSwitcherIndicatorRadius = 8.0;

QColor blend(const QColor &from, const QColor &to, qreal t) {
  t = qBound(0.0, t, 1.0);
  return QColor::fromRgbF(from.redF() + (to.redF() - from.redF()) * t,
                          from.greenF() + (to.greenF() - from.greenF()) * t,
                          from.blueF() + (to.blueF() - from.blueF()) * t,
                          from.alphaF() + (to.alphaF() - from.alphaF()) * t);
}

bool isDark() { return ThemeManager::instance().isDarkTheme(); }

// -- Colour palettes --------------------------------------------------------
struct CardPalette {
  QColor bg;
  QColor bgHover;
  QColor bgPressed;
  QColor border;
  QColor borderHover;
  QColor text;
  QColor textSub;
  QColor shadow;
};

CardPalette cardPalette() {
  if (isDark())
    return {QColor("#1e2c3c"), QColor("#263a4e"),  QColor("#2b1f15"),
            QColor(255, 244, 230, 35), QColor(249, 115, 22, 140),
            QColor("#f0e6da"), QColor("#a89888"), QColor(0, 0, 0, 60)};
  return {QColor("#ffffff"), QColor("#fff4e7"),  QColor("#f6dfca"),
          QColor("#d0c4b4"), QColor(234, 88, 12, 110),
          QColor("#2c2016"), QColor("#7a6a58"), QColor(0, 0, 0, 22)};
}

struct SwitcherPalette {
  QColor bg;
  QColor indicator;
  QColor textActive;
  QColor textInactive;
  QColor border;
};

SwitcherPalette switcherPalette() {
  if (isDark())
    return {QColor("#0c1218"), QColor("#1e3048"), QColor("#f4efe8"),
            QColor("#8a8078"), QColor(255, 244, 230, 20)};
  return {QColor("#e4dace"), QColor("#ffffff"), QColor("#2c2016"),
          QColor("#8a7a68"), QColor("#ccc0b0")};
}

struct CategoryPalette {
  QColor bg;
  QColor text;
  QColor chevron;
  QColor separator;
};

CategoryPalette categoryPalette() {
  if (isDark())
    return {QColor(26, 38, 54, 0), QColor("#e0d6ca"), QColor("#b0a090"),
            QColor(249, 115, 22, 55)};
  return {QColor(255, 255, 255, 0), QColor("#4a3a28"), QColor("#7a6a58"),
          QColor(234, 88, 12, 40)};
}

QColor panelBg() { return isDark() ? QColor("#10161d") : QColor("#f5efe6"); }

} // namespace

// ===========================================================================
// ElementCard
// ===========================================================================

ElementCard::ElementCard(const ElementInfo &info, QWidget *parent)
    : QWidget(parent), info_(info) {
  setFixedSize(kCardWidth, kCardHeight);
  setCursor(Qt::PointingHandCursor);
  setToolTip(info.tooltip);
  setMouseTracking(true);

  QIcon icon(info.icon);
  if (!icon.isNull()) {
    const int dpr = 2;
    iconPixmap_ =
        icon.pixmap(QSize(kCardIconSize, kCardIconSize) * dpr);
    iconPixmap_.setDevicePixelRatio(dpr);
  }

  hoverAnim_.setDuration(160);
  hoverAnim_.setEasingCurve(QEasingCurve::OutCubic);
  connect(&hoverAnim_, &QVariantAnimation::valueChanged, this,
          [this](const QVariant &v) {
            hoverProgress_ = v.toReal();
            update();
          });

  pressAnim_.setDuration(100);
  pressAnim_.setEasingCurve(QEasingCurve::OutCubic);
  connect(&pressAnim_, &QVariantAnimation::valueChanged, this,
          [this](const QVariant &v) {
            pressProgress_ = v.toReal();
            update();
          });
}

void ElementCard::paintEvent(QPaintEvent *) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);

  const CardPalette pal = cardPalette();
  const qreal h = hoverProgress_, pr = pressProgress_;

  // -- shadow (subtle, only on hover) ------------------------------------
  if (h > 0.01) {
    QColor sh = pal.shadow;
    sh.setAlphaF(sh.alphaF() * h * 0.6);
    p.setPen(Qt::NoPen);
    p.setBrush(sh);
    QRectF shadowRect = QRectF(rect()).adjusted(1.5, 2.0, -0.5, 1.0);
    p.drawRoundedRect(shadowRect, kCardRadius, kCardRadius);
  }

  // -- card body ---------------------------------------------------------
  QRectF body = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
  body.translate(0.0, pr * 1.0);

  QColor fill = blend(pal.bg, pal.bgHover, h);
  fill = blend(fill, pal.bgPressed, pr);
  QColor border = blend(pal.border, pal.borderHover, h);

  // glow ring on hover
  if (h > 0.01) {
    QColor glow = pal.borderHover;
    glow.setAlphaF(0.10 + h * 0.10);
    p.setPen(Qt::NoPen);
    p.setBrush(glow);
    p.drawRoundedRect(body.adjusted(-1.2, -1.2, 1.2, 1.2),
                      kCardRadius + 1.2, kCardRadius + 1.2);
  }

  p.setPen(QPen(border, 1.2));
  p.setBrush(fill);
  p.drawRoundedRect(body, kCardRadius, kCardRadius);

  // -- icon --------------------------------------------------------------
  const qreal iconY = body.top() + 12;
  if (!iconPixmap_.isNull()) {
    const qreal iconW = kCardIconSize;
    const qreal iconX = body.center().x() - iconW / 2.0;
    p.drawPixmap(QPointF(iconX, iconY), iconPixmap_);
  }

  // -- label -------------------------------------------------------------
  QFont font;
  font.setPixelSize(11);
  font.setWeight(QFont::DemiBold);
  p.setFont(font);
  p.setPen(pal.text);

  QRectF labelRect(body.left() + 4, iconY + kCardIconSize + 5,
                   body.width() - 8, 16);
  p.drawText(labelRect, Qt::AlignHCenter | Qt::AlignTop, info_.label);
}

void ElementCard::enterEvent(QEnterEvent *) {
  hoverAnim_.stop();
  hoverAnim_.setStartValue(hoverProgress_);
  hoverAnim_.setEndValue(1.0);
  hoverAnim_.start();
}

void ElementCard::leaveEvent(QEvent *) {
  hoverAnim_.stop();
  hoverAnim_.setStartValue(hoverProgress_);
  hoverAnim_.setEndValue(0.0);
  hoverAnim_.start();
  if (pressed_) {
    pressed_ = false;
    pressAnim_.stop();
    pressAnim_.setStartValue(pressProgress_);
    pressAnim_.setEndValue(0.0);
    pressAnim_.start();
  }
}

void ElementCard::mousePressEvent(QMouseEvent *e) {
  if (e->button() == Qt::LeftButton) {
    pressed_ = true;
    pressAnim_.stop();
    pressAnim_.setStartValue(pressProgress_);
    pressAnim_.setEndValue(1.0);
    pressAnim_.start();
  }
}

void ElementCard::mouseReleaseEvent(QMouseEvent *e) {
  if (pressed_ && e->button() == Qt::LeftButton) {
    pressed_ = false;
    pressAnim_.stop();
    pressAnim_.setStartValue(pressProgress_);
    pressAnim_.setEndValue(0.0);
    pressAnim_.start();
    if (rect().contains(e->pos()))
      emit clicked(info_.id);
  }
}

// ===========================================================================
// CategorySection
// ===========================================================================

CategorySection::CategorySection(const QString &title,
                                 const QVector<ElementInfo> &elements,
                                 QWidget *parent)
    : QWidget(parent), title_(title), elements_(elements) {
  setCursor(Qt::ArrowCursor);

  auto *vbox = new QVBoxLayout(this);
  vbox->setContentsMargins(0, 0, 0, 0);
  vbox->setSpacing(0);

  // Reserve header space via fixed-height spacer (we paint the header)
  auto *headerSpacer = new QWidget(this);
  headerSpacer->setFixedHeight(kCategoryHeaderHeight);
  headerSpacer->setAttribute(Qt::WA_TransparentForMouseEvents);
  vbox->addWidget(headerSpacer);

  // Card container
  cardContainer_ = new QWidget(this);
  cardGrid_ = new QGridLayout(cardContainer_);
  cardGrid_->setSpacing(8);
  cardGrid_->setContentsMargins(4, 4, 4, 8);
  vbox->addWidget(cardContainer_);

  rebuildGrid();

  chevronAnim_.setDuration(200);
  chevronAnim_.setEasingCurve(QEasingCurve::InOutCubic);
  connect(&chevronAnim_, &QVariantAnimation::valueChanged, this,
          [this](const QVariant &v) {
            chevronAngle_ = v.toReal();
            update();
          });
}

void CategorySection::setCollapsed(bool collapsed) {
  if (collapsed_ == collapsed)
    return;
  collapsed_ = collapsed;
  cardContainer_->setVisible(!collapsed_);

  chevronAnim_.stop();
  chevronAnim_.setStartValue(chevronAngle_);
  chevronAnim_.setEndValue(collapsed_ ? 90.0 : 0.0);
  chevronAnim_.start();
}

void CategorySection::paintEvent(QPaintEvent *) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);
  const CategoryPalette pal = categoryPalette();

  // -- header background -------------------------------------------------
  QRectF headerRect(0, 0, width(), kCategoryHeaderHeight);

  // -- chevron -----------------------------------------------------------
  {
    p.save();
    const qreal cx = 14;
    const qreal cy = headerRect.center().y();
    p.translate(cx, cy);
    p.rotate(chevronAngle_);
    p.translate(-cx, -cy);

    QPainterPath chevron;
    chevron.moveTo(10, cy - 4);
    chevron.lineTo(14, cy + 2);
    chevron.lineTo(18, cy - 4);
    p.setPen(QPen(pal.chevron, 1.8, Qt::SolidLine, Qt::RoundCap,
                  Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    p.drawPath(chevron);
    p.restore();
  }

  // -- title text --------------------------------------------------------
  QFont font;
  font.setPixelSize(11);
  font.setWeight(QFont::Bold);
  font.setLetterSpacing(QFont::AbsoluteSpacing, 1.0);
  p.setFont(font);
  p.setPen(pal.text);
  p.drawText(QRectF(28, 0, width() - 36, kCategoryHeaderHeight),
             Qt::AlignVCenter | Qt::AlignLeft, title_.toUpper());

  // -- bottom separator --------------------------------------------------
  if (!collapsed_) {
    p.setPen(Qt::NoPen);
    p.setBrush(pal.separator);
    p.drawRect(QRectF(12, kCategoryHeaderHeight - 1, width() - 24, 1));
  }
}

void CategorySection::mousePressEvent(QMouseEvent *e) {
  if (e->position().y() < kCategoryHeaderHeight) {
    setCollapsed(!collapsed_);
    e->accept();
  } else {
    QWidget::mousePressEvent(e);
  }
}

void CategorySection::rebuildGrid() {
  // Clear existing
  while (cardGrid_->count()) {
    QLayoutItem *item = cardGrid_->takeAt(0);
    if (item->widget())
      item->widget()->deleteLater();
    delete item;
  }

  for (int i = 0; i < elements_.size(); ++i) {
    auto *card = new ElementCard(elements_[i], cardContainer_);
    connect(card, &ElementCard::clicked, this,
            &CategorySection::elementClicked);
    cardGrid_->addWidget(card, i / kGridColumns, i % kGridColumns,
                         Qt::AlignCenter);
  }
}

// ===========================================================================
// DomainSwitcher
// ===========================================================================

DomainSwitcher::DomainSwitcher(QWidget *parent) : QWidget(parent) {
  setFixedHeight(kSwitcherHeight + 16); // 8px padding top + bottom
  setCursor(Qt::PointingHandCursor);

  slideAnim_.setDuration(220);
  slideAnim_.setEasingCurve(QEasingCurve::InOutCubic);
  connect(&slideAnim_, &QVariantAnimation::valueChanged, this,
          [this](const QVariant &v) {
            indicatorX_ = v.toReal();
            update();
          });
}

void DomainSwitcher::setCurrentIndex(int index) {
  if (index == currentIndex_ || index < 0 || index >= labels_.size())
    return;
  currentIndex_ = index;

  const qreal margin = 6;
  const qreal segW = (width() - 2 * margin) / labels_.size();
  const qreal target = margin + index * segW;

  slideAnim_.stop();
  slideAnim_.setStartValue(indicatorX_);
  slideAnim_.setEndValue(target);
  slideAnim_.start();

  emit indexChanged(index);
}

void DomainSwitcher::paintEvent(QPaintEvent *) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);
  const SwitcherPalette pal = switcherPalette();

  const qreal topPad = 8;
  const QRectF area(6, topPad, width() - 12, kSwitcherHeight);
  const qreal segW = area.width() / labels_.size();

  // -- track background --------------------------------------------------
  p.setPen(QPen(pal.border, 1.0));
  p.setBrush(pal.bg);
  p.drawRoundedRect(area, kSwitcherRadius, kSwitcherRadius);

  // -- sliding indicator -------------------------------------------------
  // Ensure indicatorX_ is initialised on first paint
  if (slideAnim_.state() != QVariantAnimation::Running &&
      qFuzzyCompare(indicatorX_, 0.0) && currentIndex_ == 0) {
    indicatorX_ = area.left();
  }

  const qreal indPad = 3;
  QRectF indRect(indicatorX_ + indPad, area.top() + indPad,
                 segW - 2 * indPad, area.height() - 2 * indPad);
  p.setPen(Qt::NoPen);
  p.setBrush(pal.indicator);
  p.drawRoundedRect(indRect, kSwitcherIndicatorRadius,
                    kSwitcherIndicatorRadius);

  // -- labels ------------------------------------------------------------
  QFont font;
  font.setPixelSize(12);
  font.setWeight(QFont::DemiBold);
  p.setFont(font);

  for (int i = 0; i < labels_.size(); ++i) {
    QRectF labelRect(area.left() + i * segW, area.top(), segW,
                     area.height());
    p.setPen(i == currentIndex_ ? pal.textActive : pal.textInactive);
    p.drawText(labelRect, Qt::AlignCenter, labels_[i]);
  }
}

void DomainSwitcher::mousePressEvent(QMouseEvent *e) {
  const qreal margin = 6;
  const qreal segW = (width() - 2 * margin) / labels_.size();
  int index = static_cast<int>((e->pos().x() - margin) / segW);
  index = qBound(0, index, labels_.size() - 1);
  setCurrentIndex(index);
}

// ===========================================================================
// Default element library
// ===========================================================================

QVector<ElementInfo> ElementBankPanel::defaultElements() {
  return {
      // People
      {"user", "User", ":/ui-icons/arch_user.svg", "Human / end user",
       "People"},
      {"user_group", "Users", ":/ui-icons/arch_user_group.svg",
       "Group of users", "People"},
      {"client", "Client", ":/ui-icons/arch_client.svg", "Client application",
       "People"},

      // Cloud & Network
      {"cloud", "Cloud", ":/ui-icons/arch_cloud.svg", "Cloud provider",
       "Cloud"},
      {"cdn", "CDN", ":/ui-icons/arch_cdn.svg", "Content delivery network",
       "Cloud"},
      {"dns", "DNS", ":/ui-icons/arch_dns.svg", "Domain name system", "Cloud"},
      {"firewall", "Firewall", ":/ui-icons/arch_firewall.svg",
       "Firewall / WAF", "Cloud"},
      {"load_balancer", "Load Balancer", ":/ui-icons/arch_load_balancer.svg",
       "Load balancer", "Cloud"},
      {"api_gateway", "Gateway", ":/ui-icons/arch_gateway.svg", "API gateway",
       "Cloud"},

      // Compute
      {"app_server", "App Server", ":/ui-icons/arch_app_server.svg",
       "Application server / microservice", "Compute"},
      {"container", "Container", ":/ui-icons/arch_container.svg",
       "Container (Docker)", "Compute"},
      {"serverless", "Serverless", ":/ui-icons/arch_serverless.svg",
       "Serverless / FaaS (Lambda)", "Compute"},
      {"virtual_machine", "VM", ":/ui-icons/arch_virtual_machine.svg",
       "Virtual machine", "Compute"},
      {"microservice", "Microservice", ":/ui-icons/arch_microservice.svg",
       "Microservice", "Compute"},

      // Backend Services
      {"api", "API", ":/ui-icons/arch_api.svg", "REST / GraphQL API",
       "Backend"},
      {"auth", "Auth", ":/ui-icons/arch_auth.svg",
       "Authentication / identity service", "Backend"},
      {"cache", "Cache", ":/ui-icons/arch_cache.svg",
       "Cache (Redis, Memcached)", "Backend"},
      {"message_queue", "Queue", ":/ui-icons/arch_queue.svg",
       "Message queue / broker", "Backend"},
      {"notification", "Notification", ":/ui-icons/arch_notification.svg",
       "Notification / push service", "Backend"},
      {"search", "Search", ":/ui-icons/arch_search.svg",
       "Search engine (Elasticsearch)", "Backend"},

      // Data & Observability
      {"database", "Database", ":/ui-icons/arch_database.svg", "Database",
       "Data"},
      {"object_storage", "Storage", ":/ui-icons/arch_storage.svg",
       "Object / file storage", "Data"},
      {"logging", "Logging", ":/ui-icons/arch_logging.svg", "Log aggregation",
       "Data"},
      {"monitoring", "Monitor", ":/ui-icons/arch_monitor.svg",
       "Monitoring / logging system", "Data"},

      // Electronics – Passive
      {"resistor", "Resistor", ":/ui-icons/elec_resistor.svg", "Resistor",
       "Passive"},
      {"capacitor", "Capacitor", ":/ui-icons/elec_capacitor.svg", "Capacitor",
       "Passive"},
      {"inductor", "Inductor", ":/ui-icons/elec_inductor.svg", "Inductor",
       "Passive"},
      {"fuse", "Fuse", ":/ui-icons/elec_fuse.svg", "Fuse", "Passive"},
      {"crystal", "Crystal", ":/ui-icons/elec_crystal.svg",
       "Crystal oscillator", "Passive"},
      {"transformer", "Transformer", ":/ui-icons/elec_transformer.svg",
       "Transformer", "Passive"},

      // Electronics – Semiconductor
      {"diode", "Diode", ":/ui-icons/elec_diode.svg", "Diode",
       "Semiconductor"},
      {"led", "LED", ":/ui-icons/elec_led.svg", "Light-emitting diode",
       "Semiconductor"},
      {"transistor", "Transistor", ":/ui-icons/elec_transistor.svg",
       "Bipolar junction transistor", "Semiconductor"},
      {"mosfet", "MOSFET", ":/ui-icons/elec_mosfet.svg", "MOSFET",
       "Semiconductor"},
      {"opamp", "Op-Amp", ":/ui-icons/elec_opamp.svg",
       "Operational amplifier", "Semiconductor"},
      {"voltage_regulator", "Regulator",
       ":/ui-icons/elec_voltage_regulator.svg", "Voltage regulator",
       "Semiconductor"},

      // Electronics – Power
      {"battery", "Battery", ":/ui-icons/elec_battery.svg", "Battery",
       "Power"},
      {"ground", "Ground", ":/ui-icons/elec_ground.svg", "Ground", "Power"},
      {"elec_switch", "Switch", ":/ui-icons/elec_switch.svg", "Switch (SPST)",
       "Power"},
      {"relay", "Relay", ":/ui-icons/elec_relay.svg", "Relay", "Power"},
      {"motor", "Motor", ":/ui-icons/elec_motor.svg", "DC motor", "Power"},
      {"power_supply", "PSU", ":/ui-icons/elec_power_supply.svg",
       "Power supply unit", "Power"},

      // Electronics – IC & Module
      {"microcontroller", "MCU", ":/ui-icons/elec_microcontroller.svg",
       "Microcontroller", "IC & Module"},
      {"ic_chip", "IC Chip", ":/ui-icons/elec_ic_chip.svg",
       "Integrated circuit", "IC & Module"},
      {"sensor", "Sensor", ":/ui-icons/elec_sensor.svg", "Sensor",
       "IC & Module"},
      {"antenna", "Antenna", ":/ui-icons/elec_antenna.svg", "Antenna",
       "IC & Module"},
      {"speaker", "Speaker", ":/ui-icons/elec_speaker.svg", "Speaker",
       "IC & Module"},
      {"connector", "Connector", ":/ui-icons/elec_connector.svg",
       "Connector / header", "IC & Module"},
  };
}

// ---------------------------------------------------------------------------
// Domain filter
// ---------------------------------------------------------------------------

QVector<ElementInfo> ElementBankPanel::elementsForDomain(Domain domain) {
  static const QSet<QString> archCategories = {"People", "Cloud", "Compute",
                                               "Backend", "Data"};
  static const QSet<QString> elecCategories = {"Passive", "Semiconductor",
                                               "Power", "IC & Module"};

  const QSet<QString> &wanted =
      (domain == Domain::Architecture) ? archCategories : elecCategories;

  QVector<ElementInfo> result;
  for (const auto &e : defaultElements()) {
    if (wanted.contains(e.category))
      result.append(e);
  }
  return result;
}

// ===========================================================================
// ElementBankPanel – construction
// ===========================================================================

ElementBankPanel::ElementBankPanel(QWidget *parent)
    : QDockWidget("Element Library", parent) {
  setObjectName("ElementBankPanel");
  setFeatures(QDockWidget::DockWidgetClosable |
              QDockWidget::DockWidgetMovable |
              QDockWidget::DockWidgetFloatable);
  setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

  // -- outer container (switcher + scroll area) ---------------------------
  auto *outer = new QWidget(this);
  auto *outerLayout = new QVBoxLayout(outer);
  outerLayout->setContentsMargins(0, 0, 0, 0);
  outerLayout->setSpacing(0);

  // Domain switcher
  switcher_ = new DomainSwitcher(outer);
  outerLayout->addWidget(switcher_);

  // Scroll area
  scrollArea_ = new QScrollArea(outer);
  scrollArea_->setWidgetResizable(true);
  scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scrollArea_->setFrameShape(QFrame::NoFrame);
  outerLayout->addWidget(scrollArea_);

  // Content widget
  contentWidget_ = new QWidget(scrollArea_);
  contentLayout_ = new QVBoxLayout(contentWidget_);
  contentLayout_->setContentsMargins(10, 8, 10, 12);
  contentLayout_->setSpacing(4);
  contentLayout_->setAlignment(Qt::AlignTop);
  contentLayout_->addStretch();
  scrollArea_->setWidget(contentWidget_);

  setWidget(outer);
  setMinimumWidth(260);
  setMaximumWidth(380);

  // Wire up domain switcher
  connect(switcher_, &DomainSwitcher::indexChanged, this,
          &ElementBankPanel::switchDomain);

  // Build initial content
  buildDomainContent(Domain::Architecture);

  // Theme
  connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this,
          [this]() { applyTheme(); });
  applyTheme();
}

void ElementBankPanel::switchDomain(int index) {
  Domain domain =
      (index == 0) ? Domain::Architecture : Domain::Electronics;
  if (domain == currentDomain_)
    return;
  currentDomain_ = domain;
  buildDomainContent(domain);
}

void ElementBankPanel::buildDomainContent(Domain domain) {
  // Remove old content
  while (contentLayout_->count()) {
    QLayoutItem *item = contentLayout_->takeAt(0);
    if (item->widget())
      item->widget()->deleteLater();
    delete item;
  }

  // Group elements by category, preserving insertion order.
  QVector<ElementInfo> all = elementsForDomain(domain);
  QVector<QString> orderedCategories;
  QHash<QString, QVector<ElementInfo>> groups;
  for (const auto &e : all) {
    if (!groups.contains(e.category))
      orderedCategories.append(e.category);
    groups[e.category].append(e);
  }

  for (const QString &cat : orderedCategories) {
    auto *section =
        new CategorySection(cat, groups[cat], contentWidget_);
    connect(section, &CategorySection::elementClicked, this,
            &ElementBankPanel::elementSelected);
    contentLayout_->addWidget(section);
  }

  contentLayout_->addStretch();

  // Reset scroll position
  scrollArea_->verticalScrollBar()->setValue(0);
}

void ElementBankPanel::applyTheme() {
  const QColor bg = panelBg();
  const QString bgStr = bg.name();

  // Minimal stylesheet: only background colours for the container widgets.
  // All actual rendering is done in custom paintEvent()s.
  const QString ss = QString(R"(
    QDockWidget { background-color: %1; }
    QScrollArea { background-color: %1; border: none; }
    QWidget { background-color: %1; }
  )")
                         .arg(bgStr);
  setStyleSheet(ss);

  // Force repaint of all children
  update();
  for (auto *child : findChildren<QWidget *>())
    child->update();
}
