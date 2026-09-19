/**
 * @file element_factory.cpp
 * @brief Implementation of the diagram element factory.
 */
#include "element_factory.h"
#include "architecture_elements.h"
#include "electronics_elements.h"
#include <QHash>
#include <functional>

namespace {
// QGraphicsItem::data() key holding the element's bank id.
constexpr int kElementIdDataKey = 0x454c4d; // "ELM"

using Creator = std::function<QGraphicsItem *()>;

const QHash<QString, Creator> &creators() {
  static const QHash<QString, Creator> table = {
      {"client", [] { return new ClientElement(); }},
      {"load_balancer", [] { return new LoadBalancerElement(); }},
      {"api_gateway", [] { return new ApiGatewayElement(); }},
      {"app_server", [] { return new AppServerElement(); }},
      {"cache", [] { return new CacheElement(); }},
      {"message_queue", [] { return new MessageQueueElement(); }},
      {"database", [] { return new DatabaseElement(); }},
      {"object_storage", [] { return new ObjectStorageElement(); }},
      {"auth", [] { return new AuthElement(); }},
      {"monitoring", [] { return new MonitoringElement(); }},
      {"user", [] { return new UserElement(); }},
      {"user_group", [] { return new UserGroupElement(); }},
      {"cloud", [] { return new CloudElement(); }},
      {"cdn", [] { return new CDNElement(); }},
      {"dns", [] { return new DNSElement(); }},
      {"firewall", [] { return new FirewallElement(); }},
      {"container", [] { return new ContainerElement(); }},
      {"serverless", [] { return new ServerlessElement(); }},
      {"virtual_machine", [] { return new VirtualMachineElement(); }},
      {"microservice", [] { return new MicroserviceElement(); }},
      {"api", [] { return new APIElement(); }},
      {"notification", [] { return new NotificationElement(); }},
      {"search", [] { return new SearchElement(); }},
      {"logging", [] { return new LoggingElement(); }},
      {"resistor", [] { return new ResistorElement(); }},
      {"capacitor", [] { return new CapacitorElement(); }},
      {"inductor", [] { return new InductorElement(); }},
      {"fuse", [] { return new FuseElement(); }},
      {"crystal", [] { return new CrystalElement(); }},
      {"transformer", [] { return new TransformerElement(); }},
      {"diode", [] { return new DiodeElement(); }},
      {"led", [] { return new LEDElement(); }},
      {"transistor", [] { return new TransistorElement(); }},
      {"mosfet", [] { return new MOSFETElement(); }},
      {"opamp", [] { return new OpAmpElement(); }},
      {"voltage_regulator", [] { return new VoltageRegulatorElement(); }},
      {"battery", [] { return new BatteryElement(); }},
      {"ground", [] { return new GroundElement(); }},
      {"elec_switch", [] { return new SwitchElement(); }},
      {"relay", [] { return new RelayElement(); }},
      {"motor", [] { return new MotorElement(); }},
      {"power_supply", [] { return new PowerSupplyElement(); }},
      {"microcontroller", [] { return new MicrocontrollerElement(); }},
      {"ic_chip", [] { return new ICChipElement(); }},
      {"sensor", [] { return new SensorElement(); }},
      {"antenna", [] { return new AntennaElement(); }},
      {"speaker", [] { return new SpeakerElement(); }},
      {"connector", [] { return new ConnectorElement(); }},
  };
  return table;
}
} // namespace

QGraphicsItem *createDiagramElement(const QString &elementId) {
  const auto it = creators().constFind(elementId);
  if (it == creators().constEnd())
    return nullptr;
  QGraphicsItem *item = (*it)();
  item->setData(kElementIdDataKey, elementId);
  return item;
}

QString diagramElementId(const QGraphicsItem *item) {
  return item ? item->data(kElementIdDataKey).toString() : QString();
}
