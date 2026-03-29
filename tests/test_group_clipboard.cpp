/**
 * @file test_group_clipboard.cpp
 * @brief Unit tests for clipboard serialization round-trip of grouped items.
 *
 * Validates that QGraphicsItemGroup items (e.g. arrows) preserve their
 * child positions and transforms after a serialize/deserialize round-trip.
 * Regression test for: arrows (and other groups) disappearing or shifting
 * position on copy/paste.
 */
#include <QDataStream>
#include <QGraphicsItemGroup>
#include <QGraphicsLineItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QtTest/QtTest>
#include <cmath>

// ---------------------------------------------------------------------------
// Minimal reproduction of the canvas serialize/deserialize helpers, kept in
// sync with the logic in src/widgets/canvas.cpp so that the test exercises
// the same code paths.
// ---------------------------------------------------------------------------

static void serializeOneItem(QDataStream &ds, QGraphicsItem *item) {
  if (!item)
    return;
  if (auto g = dynamic_cast<QGraphicsItemGroup *>(item)) {
    auto children = g->childItems();
    ds << QString("GroupT") << g->pos() << g->transform()
       << static_cast<qint32>(children.size());
    for (auto *child : children)
      serializeOneItem(ds, child);
  } else if (auto r = dynamic_cast<QGraphicsRectItem *>(item)) {
    ds << QString("RectangleT") << r->rect() << r->pos() << r->pen()
       << r->brush() << r->transform();
  } else if (auto l = dynamic_cast<QGraphicsLineItem *>(item)) {
    ds << QString("LineT") << l->line() << l->pos() << l->pen()
       << l->transform();
  } else if (auto pg = dynamic_cast<QGraphicsPolygonItem *>(item)) {
    ds << QString("PolygonT") << pg->polygon() << pg->pos() << pg->pen()
       << pg->brush() << pg->transform();
  }
}

static constexpr qint32 MAX_GROUP_CHILDREN = 10000;

static QGraphicsItem *deserializeOneItem(QDataStream &ds, const QString &type) {
  if (type == "GroupT") {
    QPointF pos;
    QTransform tr;
    qint32 count;
    ds >> pos >> tr >> count;
    if (count < 0 || count > MAX_GROUP_CHILDREN)
      return nullptr;
    std::unique_ptr<QGraphicsItemGroup> group(new QGraphicsItemGroup());
    // Add children BEFORE setting pos/transform so addToGroup() sees
    // the group at the origin — the serialized child positions (group-
    // local) equal scene coordinates when the group is at the origin,
    // so addToGroup()'s mapping is the identity.
    for (qint32 i = 0; i < count; ++i) {
      if (ds.atEnd())
        break;
      QString childType;
      ds >> childType;
      QGraphicsItem *child = deserializeOneItem(ds, childType);
      if (child)
        group->addToGroup(child);
    }
    group->setPos(pos);
    group->setTransform(tr);
    return group.release();
  } else if (type == "RectangleT") {
    QRectF r;
    QPointF p;
    QPen pn;
    QBrush b;
    QTransform tr;
    ds >> r >> p >> pn >> b >> tr;
    auto n = new QGraphicsRectItem(r);
    n->setPen(pn);
    n->setBrush(b);
    n->setPos(p);
    n->setTransform(tr);
    return n;
  } else if (type == "LineT") {
    QLineF l;
    QPointF p;
    QPen pn;
    QTransform tr;
    ds >> l >> p >> pn >> tr;
    auto n = new QGraphicsLineItem(l);
    n->setPen(pn);
    n->setPos(p);
    n->setTransform(tr);
    return n;
  } else if (type == "PolygonT") {
    QPolygonF pg;
    QPointF p;
    QPen pn;
    QBrush b;
    QTransform tr;
    ds >> pg >> p >> pn >> b >> tr;
    auto n = new QGraphicsPolygonItem(pg);
    n->setPen(pn);
    n->setBrush(b);
    n->setPos(p);
    n->setTransform(tr);
    return n;
  }
  return nullptr;
}

// ---------------------------------------------------------------------------
// Buggy deserializer (pre-fix) — used to prove the test catches the bug.
// ---------------------------------------------------------------------------
static QGraphicsItem *deserializeOneItemBuggy(QDataStream &ds,
                                              const QString &type) {
  if (type == "GroupT") {
    QPointF pos;
    QTransform tr;
    qint32 count;
    ds >> pos >> tr >> count;
    if (count < 0 || count > MAX_GROUP_CHILDREN)
      return nullptr;
    std::unique_ptr<QGraphicsItemGroup> group(new QGraphicsItemGroup());
    group->setPos(pos);
    group->setTransform(tr);
    for (qint32 i = 0; i < count; ++i) {
      if (ds.atEnd())
        break;
      QString childType;
      ds >> childType;
      QGraphicsItem *child = deserializeOneItemBuggy(ds, childType);
      if (child)
        group->addToGroup(child); // BUG: double-maps child position
    }
    return group.release();
  } else if (type == "RectangleT") {
    QRectF r;
    QPointF p;
    QPen pn;
    QBrush b;
    QTransform tr;
    ds >> r >> p >> pn >> b >> tr;
    auto n = new QGraphicsRectItem(r);
    n->setPen(pn);
    n->setBrush(b);
    n->setPos(p);
    n->setTransform(tr);
    return n;
  } else if (type == "LineT") {
    QLineF l;
    QPointF p;
    QPen pn;
    QTransform tr;
    ds >> l >> p >> pn >> tr;
    auto n = new QGraphicsLineItem(l);
    n->setPen(pn);
    n->setPos(p);
    n->setTransform(tr);
    return n;
  } else if (type == "PolygonT") {
    QPolygonF pg;
    QPointF p;
    QPen pn;
    QBrush b;
    QTransform tr;
    ds >> pg >> p >> pn >> b >> tr;
    auto n = new QGraphicsPolygonItem(pg);
    n->setPen(pn);
    n->setBrush(b);
    n->setPos(p);
    n->setTransform(tr);
    return n;
  }
  return nullptr;
}

// ---------------------------------------------------------------------------
// Helper: build an arrow group (line + arrowhead polygon) like Canvas does.
// ---------------------------------------------------------------------------
static QGraphicsItemGroup *
createArrowGroup(const QPointF &start, const QPointF &end, const QPen &pen) {
  auto *line = new QGraphicsLineItem(QLineF(start, end));
  line->setPen(pen);

  double angle = std::atan2(end.y() - start.y(), end.x() - start.x());
  double sz = pen.width() * 4;
  QPolygonF ah;
  ah << end
     << end - QPointF(std::cos(angle - M_PI / 6) * sz,
                      std::sin(angle - M_PI / 6) * sz)
     << end - QPointF(std::cos(angle + M_PI / 6) * sz,
                      std::sin(angle + M_PI / 6) * sz);

  auto *head = new QGraphicsPolygonItem(ah);
  head->setPen(pen);
  head->setBrush(pen.color());

  auto *group = new QGraphicsItemGroup();
  group->addToGroup(line);
  group->addToGroup(head);
  group->setFlags(QGraphicsItem::ItemIsSelectable |
                  QGraphicsItem::ItemIsMovable);
  return group;
}

// ---------------------------------------------------------------------------

class TestGroupClipboard : public QObject {
  Q_OBJECT

private slots:

  // Verify the fixed deserializer preserves child positions.
  void testArrowGroupRoundTrip() {
    QGraphicsScene scene;
    scene.setSceneRect(0, 0, 3000, 2000);

    QPen pen(Qt::white, 3);
    QPointF start(100, 200);
    QPointF end(400, 300);

    auto *arrow = createArrowGroup(start, end, pen);
    scene.addItem(arrow);

    // Simulate user moving the arrow — this makes the group pos non-zero,
    // which is critical for exposing the double-mapping bug.
    arrow->setPos(150, 250);

    // Record original scene bounding rect of the arrow.
    QRectF origBounds = arrow->sceneBoundingRect();
    QPointF origGroupPos = arrow->pos();

    // Record each child's pos (group-local) and sceneBoundingRect.
    QList<QPointF> origChildPos;
    QList<QRectF> origChildBounds;
    for (auto *child : arrow->childItems()) {
      origChildPos.append(child->pos());
      origChildBounds.append(child->sceneBoundingRect());
    }

    // --- Serialize ---
    QByteArray ba;
    {
      QDataStream ds(&ba, QIODevice::WriteOnly);
      serializeOneItem(ds, arrow);
    }

    // --- Deserialize (fixed) ---
    QGraphicsItem *deserialized;
    {
      QDataStream ds(&ba, QIODevice::ReadOnly);
      QString t;
      ds >> t;
      deserialized = deserializeOneItem(ds, t);
    }
    QVERIFY(deserialized != nullptr);

    scene.addItem(deserialized);

    auto *dGroup = dynamic_cast<QGraphicsItemGroup *>(deserialized);
    QVERIFY(dGroup != nullptr);

    // Group position must match.
    QCOMPARE(dGroup->pos(), origGroupPos);

    // Scene bounding rect must match (within floating-point tolerance).
    QRectF newBounds = dGroup->sceneBoundingRect();
    QVERIFY2(
        qAbs(newBounds.x() - origBounds.x()) < 1.0 &&
            qAbs(newBounds.y() - origBounds.y()) < 1.0 &&
            qAbs(newBounds.width() - origBounds.width()) < 1.0 &&
            qAbs(newBounds.height() - origBounds.height()) < 1.0,
        qPrintable(
            QString("Scene bounds mismatch: orig(%1,%2 %3x%4) vs new(%5,%6 "
                    "%7x%8)")
                .arg(origBounds.x())
                .arg(origBounds.y())
                .arg(origBounds.width())
                .arg(origBounds.height())
                .arg(newBounds.x())
                .arg(newBounds.y())
                .arg(newBounds.width())
                .arg(newBounds.height())));

    // Each child's group-local position must match.
    auto newChildren = dGroup->childItems();
    QCOMPARE(newChildren.size(), origChildPos.size());
    for (int i = 0; i < newChildren.size(); ++i) {
      QVERIFY2(
          qAbs(newChildren[i]->pos().x() - origChildPos[i].x()) < 0.01 &&
              qAbs(newChildren[i]->pos().y() - origChildPos[i].y()) < 0.01,
          qPrintable(QString("Child %1 pos mismatch: orig(%2,%3) vs new(%4,%5)")
                         .arg(i)
                         .arg(origChildPos[i].x())
                         .arg(origChildPos[i].y())
                         .arg(newChildren[i]->pos().x())
                         .arg(newChildren[i]->pos().y())));
    }
  }

  // The old (buggy) code fails: child positions get double-mapped.
  void testBuggyDeserializerShiftsPosition() {
    QGraphicsScene scene;
    scene.setSceneRect(0, 0, 3000, 2000);

    QPen pen(Qt::white, 3);
    auto *arrow = createArrowGroup(QPointF(100, 200), QPointF(400, 300), pen);
    scene.addItem(arrow);

    // Move the group so the bug manifests (group pos non-zero).
    arrow->setPos(150, 250);

    QRectF origBounds = arrow->sceneBoundingRect();

    // Serialize
    QByteArray ba;
    {
      QDataStream ds(&ba, QIODevice::WriteOnly);
      serializeOneItem(ds, arrow);
    }

    // Deserialize with BUGGY code
    QGraphicsItem *buggy;
    {
      QDataStream ds(&ba, QIODevice::ReadOnly);
      QString t;
      ds >> t;
      buggy = deserializeOneItemBuggy(ds, t);
    }
    QVERIFY(buggy != nullptr);
    scene.addItem(buggy);

    QRectF buggyBounds = buggy->sceneBoundingRect();

    // The buggy version should NOT match the original bounds
    // (proving the old code had the bug).
    bool positionMatches =
        qAbs(buggyBounds.x() - origBounds.x()) < 1.0 &&
        qAbs(buggyBounds.y() - origBounds.y()) < 1.0 &&
        qAbs(buggyBounds.width() - origBounds.width()) < 1.0 &&
        qAbs(buggyBounds.height() - origBounds.height()) < 1.0;
    QVERIFY2(!positionMatches,
             "Buggy deserializer should NOT preserve position (test validates "
             "the bug exists)");
  }

  // A group at the origin should also round-trip correctly.
  void testGroupAtOriginRoundTrip() {
    QGraphicsScene scene;
    scene.setSceneRect(0, 0, 3000, 2000);

    auto *rect = new QGraphicsRectItem(QRectF(0, 0, 50, 50));
    rect->setPen(QPen(Qt::red, 2));

    auto *group = new QGraphicsItemGroup();
    group->addToGroup(rect);
    group->setPos(0, 0);
    scene.addItem(group);

    QRectF origBounds = group->sceneBoundingRect();
    QPointF origChildPos = group->childItems().first()->pos();

    QByteArray ba;
    {
      QDataStream ds(&ba, QIODevice::WriteOnly);
      serializeOneItem(ds, group);
    }

    QGraphicsItem *deserialized;
    {
      QDataStream ds(&ba, QIODevice::ReadOnly);
      QString t;
      ds >> t;
      deserialized = deserializeOneItem(ds, t);
    }
    QVERIFY(deserialized != nullptr);
    scene.addItem(deserialized);

    auto *dGroup = dynamic_cast<QGraphicsItemGroup *>(deserialized);
    QVERIFY(dGroup != nullptr);

    QRectF newBounds = dGroup->sceneBoundingRect();
    QVERIFY2(qAbs(newBounds.x() - origBounds.x()) < 1.0 &&
                 qAbs(newBounds.y() - origBounds.y()) < 1.0,
             "Group at origin should preserve position");

    QPointF newChildPos = dGroup->childItems().first()->pos();
    QVERIFY2(qAbs(newChildPos.x() - origChildPos.x()) < 0.01 &&
                 qAbs(newChildPos.y() - origChildPos.y()) < 0.01,
             "Child position within group at origin should be preserved");
  }

  // Test that duplicate-style clone+addToGroup preserves positions when
  // the save/restore pattern is applied.
  void testCloneChildPreservesPosition() {
    QGraphicsScene scene;
    scene.setSceneRect(0, 0, 3000, 2000);

    QPen pen(Qt::white, 3);
    auto *arrow = createArrowGroup(QPointF(200, 100), QPointF(500, 400), pen);
    scene.addItem(arrow);

    // Record child positions
    QList<QPointF> origChildPositions;
    QList<QRectF> origChildSceneBounds;
    for (auto *child : arrow->childItems()) {
      origChildPositions.append(child->pos());
      origChildSceneBounds.append(child->sceneBoundingRect());
    }

    // Duplicate group using the fixed pattern (like duplicateSelectedItems)
    auto *newGroup = new QGraphicsItemGroup();
    // Add children FIRST (group at origin) so addToGroup preserves positions
    for (auto *child : arrow->childItems()) {
      QGraphicsItem *newChild = nullptr;
      if (auto l = dynamic_cast<QGraphicsLineItem *>(child)) {
        auto n = new QGraphicsLineItem(l->line());
        n->setPen(l->pen());
        n->setPos(l->pos());
        newChild = n;
      } else if (auto pg = dynamic_cast<QGraphicsPolygonItem *>(child)) {
        auto n = new QGraphicsPolygonItem(pg->polygon());
        n->setPen(pg->pen());
        n->setBrush(pg->brush());
        n->setPos(pg->pos());
        newChild = n;
      }
      if (newChild) {
        newGroup->addToGroup(newChild);
      }
    }
    newGroup->setPos(arrow->pos());
    scene.addItem(newGroup);

    // Verify each child's group-local position is preserved
    auto newChildren = newGroup->childItems();
    QCOMPARE(newChildren.size(), origChildPositions.size());
    for (int i = 0; i < newChildren.size(); ++i) {
      QVERIFY2(
          qAbs(newChildren[i]->pos().x() - origChildPositions[i].x()) < 0.01 &&
              qAbs(newChildren[i]->pos().y() - origChildPositions[i].y()) <
                  0.01,
          qPrintable(QString("Cloned child %1 pos mismatch: orig(%2,%3) vs "
                             "new(%4,%5)")
                         .arg(i)
                         .arg(origChildPositions[i].x())
                         .arg(origChildPositions[i].y())
                         .arg(newChildren[i]->pos().x())
                         .arg(newChildren[i]->pos().y())));
    }
  }
};

QTEST_MAIN(TestGroupClipboard)
#include "test_group_clipboard.moc"
