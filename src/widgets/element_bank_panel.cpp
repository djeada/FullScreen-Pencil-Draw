/**
 * @file element_bank_panel.cpp
 * @brief Implementation of the ElementBankPanel widget.
 */
#include "element_bank_panel.h"
#include "../core/theme_manager.h"
#include "animated_button.h"
#include <QFrame>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QScrollArea>
#include <QSet>
#include <QToolButton>
#include <QVBoxLayout>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static QFrame *createBankSeparator(QWidget *parent) {
  auto *line = new QFrame(parent);
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Sunken);
  line->setObjectName("bankSeparator");
  line->setFixedHeight(1);
  return line;
}

// ---------------------------------------------------------------------------
// Default element library
// ---------------------------------------------------------------------------

QVector<ElementInfo> ElementBankPanel::defaultElements() {
  return {
      // People
      {"user", "User", ":/ui-icons/arch_user.svg",
       "Human / end user", "People"},
      {"user_group", "Users", ":/ui-icons/arch_user_group.svg",
       "Group of users", "People"},
      {"client", "Client", ":/ui-icons/arch_client.svg", "Client application",
       "People"},

      // Cloud & Network
      {"cloud", "Cloud", ":/ui-icons/arch_cloud.svg",
       "Cloud provider", "Cloud"},
      {"cdn", "CDN", ":/ui-icons/arch_cdn.svg",
       "Content delivery network", "Cloud"},
      {"dns", "DNS", ":/ui-icons/arch_dns.svg",
       "Domain name system", "Cloud"},
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
      {"api", "API", ":/ui-icons/arch_api.svg",
       "REST / GraphQL API", "Backend"},
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
      {"logging", "Logging", ":/ui-icons/arch_logging.svg",
       "Log aggregation", "Data"},
      {"monitoring", "Monitor", ":/ui-icons/arch_monitor.svg",
       "Monitoring / logging system", "Data"},

      // Electronics – Passive
      {"resistor", "Resistor", ":/ui-icons/elec_resistor.svg",
       "Resistor", "Passive"},
      {"capacitor", "Capacitor", ":/ui-icons/elec_capacitor.svg",
       "Capacitor", "Passive"},
      {"inductor", "Inductor", ":/ui-icons/elec_inductor.svg",
       "Inductor", "Passive"},
      {"fuse", "Fuse", ":/ui-icons/elec_fuse.svg",
       "Fuse", "Passive"},
      {"crystal", "Crystal", ":/ui-icons/elec_crystal.svg",
       "Crystal oscillator", "Passive"},
      {"transformer", "Transformer", ":/ui-icons/elec_transformer.svg",
       "Transformer", "Passive"},

      // Electronics – Semiconductor
      {"diode", "Diode", ":/ui-icons/elec_diode.svg",
       "Diode", "Semiconductor"},
      {"led", "LED", ":/ui-icons/elec_led.svg",
       "Light-emitting diode", "Semiconductor"},
      {"transistor", "Transistor", ":/ui-icons/elec_transistor.svg",
       "Bipolar junction transistor", "Semiconductor"},
      {"mosfet", "MOSFET", ":/ui-icons/elec_mosfet.svg",
       "MOSFET", "Semiconductor"},
      {"opamp", "Op-Amp", ":/ui-icons/elec_opamp.svg",
       "Operational amplifier", "Semiconductor"},
      {"voltage_regulator", "Regulator", ":/ui-icons/elec_voltage_regulator.svg",
       "Voltage regulator", "Semiconductor"},

      // Electronics – Power
      {"battery", "Battery", ":/ui-icons/elec_battery.svg",
       "Battery", "Power"},
      {"ground", "Ground", ":/ui-icons/elec_ground.svg",
       "Ground", "Power"},
      {"elec_switch", "Switch", ":/ui-icons/elec_switch.svg",
       "Switch (SPST)", "Power"},
      {"relay", "Relay", ":/ui-icons/elec_relay.svg",
       "Relay", "Power"},
      {"motor", "Motor", ":/ui-icons/elec_motor.svg",
       "DC motor", "Power"},
      {"power_supply", "PSU", ":/ui-icons/elec_power_supply.svg",
       "Power supply unit", "Power"},

      // Electronics – IC & Module
      {"microcontroller", "MCU", ":/ui-icons/elec_microcontroller.svg",
       "Microcontroller", "IC & Module"},
      {"ic_chip", "IC Chip", ":/ui-icons/elec_ic_chip.svg",
       "Integrated circuit", "IC & Module"},
      {"sensor", "Sensor", ":/ui-icons/elec_sensor.svg",
       "Sensor", "IC & Module"},
      {"antenna", "Antenna", ":/ui-icons/elec_antenna.svg",
       "Antenna", "IC & Module"},
      {"speaker", "Speaker", ":/ui-icons/elec_speaker.svg",
       "Speaker", "IC & Module"},
      {"connector", "Connector", ":/ui-icons/elec_connector.svg",
       "Connector / header", "IC & Module"},
  };
}

// ---------------------------------------------------------------------------
// Domain filter
// ---------------------------------------------------------------------------

QVector<ElementInfo>
ElementBankPanel::elementsForDomain(Domain domain) {
  static const QSet<QString> archCategories = {
      "People", "Cloud", "Compute", "Backend", "Data"};
  static const QSet<QString> elecCategories = {
      "Passive", "Semiconductor", "Power", "IC & Module"};

  const QSet<QString> &wanted =
      (domain == Domain::Architecture) ? archCategories : elecCategories;

  QVector<ElementInfo> result;
  for (const auto &e : defaultElements()) {
    if (wanted.contains(e.category))
      result.append(e);
  }
  return result;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

ElementBankPanel::ElementBankPanel(Domain domain, QWidget *parent)
    : QDockWidget(domain == Domain::Architecture ? "Architecture"
                                                 : "Electronics",
                  parent) {
  setObjectName(domain == Domain::Architecture ? "ArchitectureBankPanel"
                                               : "ElectronicsBankPanel");
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
  mainLayout->setContentsMargins(12, 14, 12, 14);
  mainLayout->setSpacing(14);
  mainLayout->setAlignment(Qt::AlignTop);

  // Group elements by category, preserving insertion order.
  QVector<ElementInfo> all = elementsForDomain(domain);
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
  setMinimumWidth(250);
  setMaximumWidth(360);

  connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this,
          [this]() { applyTheme(); });
  applyTheme();
}

void ElementBankPanel::applyTheme() {
  const bool darkTheme = ThemeManager::instance().isDarkTheme();

  if (darkTheme) {
    setStyleSheet(R"(
      QDockWidget {
        background-color: #10161d;
        color: #f4efe8;
        font-weight: 500;
      }
      QDockWidget::title {
        background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                    stop:0 #17212b, stop:0.55 #121922, stop:1 #10161d);
        padding: 12px 14px;
        font-weight: 700;
        letter-spacing: 0.6px;
        border-bottom: 1px solid rgba(255, 244, 230, 0.08);
      }
      QScrollArea {
        background-color: #10161d;
        border: none;
      }
      QWidget {
        background-color: #10161d;
      }
      QFrame#bankSeparator {
        background-color: rgba(249, 115, 22, 0.25);
        margin: 6px 8px;
      }
      QLabel[categoryHeading="true"] {
        color: #e8ddd0;
        font-size: 12px;
        font-weight: 700;
        padding: 6px 4px 2px 4px;
        letter-spacing: 1.0px;
        text-transform: uppercase;
      }
      QToolButton {
        background-color: #17212b;
        color: #f4efe8;
        border: 1px solid rgba(255, 244, 230, 0.10);
        border-radius: 10px;
        padding: 6px 4px;
        min-width: 96px;
        min-height: 56px;
        max-width: 96px;
        max-height: 56px;
        font-weight: 600;
        font-size: 11px;
      }
      QToolButton:hover {
        background-color: #1d2934;
        border: 1px solid rgba(249, 115, 22, 0.50);
        color: #ffffff;
      }
      QToolButton:pressed {
        background-color: rgba(249, 115, 22, 0.18);
        border: 1px solid rgba(249, 115, 22, 0.60);
      }
    )");
  } else {
    setStyleSheet(R"(
      QDockWidget {
        background-color: #f5efe6;
        color: #31261d;
        font-weight: 500;
      }
      QDockWidget::title {
        background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                    stop:0 #fff9f1, stop:0.55 #f7efe5, stop:1 #efe4d5);
        color: #31261d;
        padding: 12px 14px;
        font-weight: 700;
        letter-spacing: 0.6px;
        border-bottom: 1px solid #ddcfbc;
      }
      QScrollArea {
        background-color: #f5efe6;
        border: none;
      }
      QWidget {
        background-color: #f5efe6;
      }
      QFrame#bankSeparator {
        background-color: rgba(234, 88, 12, 0.22);
        margin: 6px 8px;
      }
      QLabel[categoryHeading="true"] {
        color: #5a4a3a;
        font-size: 12px;
        font-weight: 700;
        padding: 6px 4px 2px 4px;
        letter-spacing: 1.0px;
        text-transform: uppercase;
      }
      QToolButton {
        background-color: #fff9f1;
        color: #31261d;
        border: 1px solid #ddcfbc;
        border-radius: 10px;
        padding: 6px 4px;
        min-width: 96px;
        min-height: 56px;
        max-width: 96px;
        max-height: 56px;
        font-weight: 600;
        font-size: 11px;
      }
      QToolButton:hover {
        background-color: #fff4e7;
        border: 1px solid rgba(234, 88, 12, 0.45);
        color: #1a1008;
      }
      QToolButton:pressed {
        background-color: #f6dfca;
        border: 1px solid rgba(234, 88, 12, 0.55);
      }
    )");
  }
}

// ---------------------------------------------------------------------------
// Category builder
// ---------------------------------------------------------------------------

void ElementBankPanel::addCategory(QVBoxLayout *layout, const QString &category,
                                   const QVector<ElementInfo> &elements) {
  // Category heading
  auto *heading = new QLabel(category, this);
  heading->setProperty("categoryHeading", true);
  heading->setAlignment(Qt::AlignLeft);
  layout->addWidget(heading);

  // Grid of element buttons (2 columns for better readability)
  auto *gridWidget = new QWidget(this);
  auto *grid = new QGridLayout(gridWidget);
  grid->setSpacing(8);
  grid->setContentsMargins(0, 2, 0, 4);

  for (int i = 0; i < elements.size(); ++i) {
    const ElementInfo &info = elements[i];

    auto *btn = new AnimatedToolButton(gridWidget);
    QIcon icon(info.icon);
    btn->setText(info.label);
    btn->setIcon(icon);
    btn->setToolTip(info.tooltip);
    btn->setToolButtonStyle(icon.isNull() ? Qt::ToolButtonTextOnly
                                          : Qt::ToolButtonTextUnderIcon);
    btn->setFixedSize(96, 56);
    btn->setIconSize(QSize(20, 20));
    btn->setVariant(AnimatedButtonBase::Variant::PanelTile);

    connect(btn, &QToolButton::clicked, this,
            [this, id = info.id]() { emit elementSelected(id); });

    grid->addWidget(btn, i / 2, i % 2);
  }

  gridWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  layout->addWidget(gridWidget, 0, Qt::AlignHCenter);
}
