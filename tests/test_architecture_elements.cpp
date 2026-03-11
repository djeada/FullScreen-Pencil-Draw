/**
 * @file test_architecture_elements.cpp
 * @brief Tests for architecture diagram element classes.
 */
#include "architecture_elements.h"
#include <QGraphicsScene>
#include <QTest>

class TestArchitectureElements : public QObject {
  Q_OBJECT

private slots:

  void allElementsHaveCorrectLabel() {
    // Verify each concrete element creates with the expected label
    ClientElement client;
    QCOMPARE(client.label(), QString("Client"));

    LoadBalancerElement lb;
    QCOMPARE(lb.label(), QString("Load Balancer"));

    ApiGatewayElement gw;
    QCOMPARE(gw.label(), QString("API Gateway"));

    AppServerElement as;
    QCOMPARE(as.label(), QString("App Server"));

    CacheElement cache;
    QCOMPARE(cache.label(), QString("Cache"));

    MessageQueueElement mq;
    QCOMPARE(mq.label(), QString("Queue"));

    DatabaseElement db;
    QCOMPARE(db.label(), QString("Database"));

    ObjectStorageElement os;
    QCOMPARE(os.label(), QString("Storage"));

    AuthElement auth;
    QCOMPARE(auth.label(), QString("Auth"));

    MonitoringElement mon;
    QCOMPARE(mon.label(), QString("Monitor"));

    // New elements
    UserElement user;
    QCOMPARE(user.label(), QString("User"));

    UserGroupElement ug;
    QCOMPARE(ug.label(), QString("Users"));

    CloudElement cloud;
    QCOMPARE(cloud.label(), QString("Cloud"));

    CDNElement cdn;
    QCOMPARE(cdn.label(), QString("CDN"));

    DNSElement dns;
    QCOMPARE(dns.label(), QString("DNS"));

    FirewallElement fw;
    QCOMPARE(fw.label(), QString("Firewall"));

    ContainerElement ct;
    QCOMPARE(ct.label(), QString("Container"));

    ServerlessElement sl;
    QCOMPARE(sl.label(), QString("Serverless"));

    VirtualMachineElement vm;
    QCOMPARE(vm.label(), QString("VM"));

    MicroserviceElement ms;
    QCOMPARE(ms.label(), QString("Microservice"));

    APIElement api;
    QCOMPARE(api.label(), QString("API"));

    NotificationElement notif;
    QCOMPARE(notif.label(), QString("Notification"));

    SearchElement search;
    QCOMPARE(search.label(), QString("Search"));

    LoggingElement log;
    QCOMPARE(log.label(), QString("Logging"));
  }

  void allElementsBoundingRectConsistent() {
    // All elements share the same card dimensions
    const QRectF expected(0, 0, 142.0, 106.0);

    UserElement user;
    QCOMPARE(user.boundingRect(), expected);

    CloudElement cloud;
    QCOMPARE(cloud.boundingRect(), expected);

    FirewallElement fw;
    QCOMPARE(fw.boundingRect(), expected);

    ContainerElement ct;
    QCOMPARE(ct.boundingRect(), expected);

    ServerlessElement sl;
    QCOMPARE(sl.boundingRect(), expected);

    APIElement api;
    QCOMPARE(api.boundingRect(), expected);

    LoggingElement log;
    QCOMPARE(log.boundingRect(), expected);
  }

  void elementsAreSelectableAndMovable() {
    UserElement user;
    QVERIFY(user.flags() & QGraphicsItem::ItemIsSelectable);
    QVERIFY(user.flags() & QGraphicsItem::ItemIsMovable);

    CloudElement cloud;
    QVERIFY(cloud.flags() & QGraphicsItem::ItemIsSelectable);
    QVERIFY(cloud.flags() & QGraphicsItem::ItemIsMovable);

    MicroserviceElement ms;
    QVERIFY(ms.flags() & QGraphicsItem::ItemIsSelectable);
    QVERIFY(ms.flags() & QGraphicsItem::ItemIsMovable);
  }

  void elementsCanBeAddedToScene() {
    QGraphicsScene scene;
    auto *user = new UserElement();
    auto *cloud = new CloudElement();
    auto *fw = new FirewallElement();

    scene.addItem(user);
    scene.addItem(cloud);
    scene.addItem(fw);

    QCOMPARE(scene.items().size(), 3);
  }

  void totalElementCount() {
    // Verify we have the expected total number of element types (24)
    // by checking that all 24 element classes can be instantiated
    QVector<ArchitectureElementItem *> elements;
    elements << new ClientElement() << new LoadBalancerElement()
             << new ApiGatewayElement() << new AppServerElement()
             << new CacheElement() << new MessageQueueElement()
             << new DatabaseElement() << new ObjectStorageElement()
             << new AuthElement() << new MonitoringElement()
             << new UserElement() << new UserGroupElement()
             << new CloudElement() << new CDNElement() << new DNSElement()
             << new FirewallElement() << new ContainerElement()
             << new ServerlessElement() << new VirtualMachineElement()
             << new MicroserviceElement() << new APIElement()
             << new NotificationElement() << new SearchElement()
             << new LoggingElement();

    QCOMPARE(elements.size(), 24);

    QGraphicsScene scene;
    for (auto *e : elements)
      scene.addItem(e);
    QCOMPARE(scene.items().size(), 24);
  }
};

QTEST_MAIN(TestArchitectureElements)
#include "test_architecture_elements.moc"
