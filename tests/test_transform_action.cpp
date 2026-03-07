/**
 * @file test_transform_action.cpp
 * @brief Tests for TransformAction and TextResizeAction undo/redo.
 */
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QtTest/QtTest>

#include "../src/core/action.h"
#include "../src/core/item_id.h"
#include "../src/core/item_store.h"
#include "../src/core/transform_action.h"
#include "../src/widgets/latex_text_item.h"

class TestTransformAction : public QObject {
  Q_OBJECT

private slots:
  void testTransformActionUndo() {
    QGraphicsScene scene;
    ItemStore store(&scene);

    auto *rect = new QGraphicsRectItem(0, 0, 100, 100);
    scene.addItem(rect);
    ItemId id = store.registerItem(rect);
    QVERIFY(id.isValid());

    QTransform oldTransform;
    QPointF oldPos(10, 20);
    rect->setPos(oldPos);

    // Apply a scale transform
    QTransform newTransform;
    newTransform.scale(2.0, 2.0);
    QPointF newPos(30, 40);
    rect->setTransform(newTransform);
    rect->setPos(newPos);

    TransformAction action(id, &store, oldTransform, newTransform, oldPos,
                           newPos);

    // Undo should restore to old state
    action.undo();
    QCOMPARE(rect->transform(), oldTransform);
    QCOMPARE(rect->pos(), oldPos);

    // Redo should re-apply new state
    action.redo();
    QCOMPARE(rect->transform(), newTransform);
    QCOMPARE(rect->pos(), newPos);
  }

  void testTransformActionRedoUndo() {
    QGraphicsScene scene;
    ItemStore store(&scene);

    auto *rect = new QGraphicsRectItem(0, 0, 50, 50);
    scene.addItem(rect);
    ItemId id = store.registerItem(rect);

    QTransform oldT;
    QPointF oldP(0, 0);

    QTransform newT;
    newT.rotate(45);
    QPointF newP(100, 100);

    rect->setTransform(newT);
    rect->setPos(newP);

    TransformAction action(id, &store, oldT, newT, oldP, newP);

    // Multiple undo/redo cycles
    action.undo();
    QCOMPARE(rect->transform(), oldT);
    QCOMPARE(rect->pos(), oldP);

    action.redo();
    QCOMPARE(rect->transform(), newT);
    QCOMPARE(rect->pos(), newP);

    action.undo();
    QCOMPARE(rect->transform(), oldT);
    QCOMPARE(rect->pos(), oldP);
  }

  void testTextResizeActionUndo() {
    QGraphicsScene scene;
    ItemStore store(&scene);

    auto *textItem = new LatexTextItem();
    scene.addItem(textItem);
    ItemId id = store.registerItem(textItem);
    QVERIFY(id.isValid());

    QFont oldFont("Arial", 14);
    textItem->setFont(oldFont);
    QPointF oldPos(10, 20);
    textItem->setPos(oldPos);

    QFont newFont("Arial", 28);
    QPointF newPos(30, 40);
    textItem->setFont(newFont);
    textItem->setPos(newPos);

    TextResizeAction action(id, &store, oldFont, newFont, oldPos, newPos);

    // Undo should restore old font and position
    action.undo();
    QCOMPARE(textItem->font().pointSize(), oldFont.pointSize());
    QCOMPARE(textItem->pos(), oldPos);

    // Redo should apply new font and position
    action.redo();
    QCOMPARE(textItem->font().pointSize(), newFont.pointSize());
    QCOMPARE(textItem->pos(), newPos);
  }

  void testTextResizeActionMultipleCycles() {
    QGraphicsScene scene;
    ItemStore store(&scene);

    auto *textItem = new LatexTextItem();
    scene.addItem(textItem);
    ItemId id = store.registerItem(textItem);

    QFont oldFont("Monospace", 12);
    textItem->setFont(oldFont);
    QPointF oldPos(0, 0);
    textItem->setPos(oldPos);

    QFont newFont("Monospace", 24);
    QPointF newPos(50, 50);
    textItem->setFont(newFont);
    textItem->setPos(newPos);

    TextResizeAction action(id, &store, oldFont, newFont, oldPos, newPos);

    // Multiple undo/redo cycles
    for (int i = 0; i < 3; ++i) {
      action.undo();
      QCOMPARE(textItem->font().pointSize(), oldFont.pointSize());
      QCOMPARE(textItem->pos(), oldPos);

      action.redo();
      QCOMPARE(textItem->font().pointSize(), newFont.pointSize());
      QCOMPARE(textItem->pos(), newPos);
    }
  }

  void testCompositeTransformAction() {
    QGraphicsScene scene;
    ItemStore store(&scene);

    auto *rect1 = new QGraphicsRectItem(0, 0, 100, 100);
    auto *rect2 = new QGraphicsRectItem(0, 0, 50, 50);
    scene.addItem(rect1);
    scene.addItem(rect2);
    ItemId id1 = store.registerItem(rect1);
    ItemId id2 = store.registerItem(rect2);

    QTransform oldT1, oldT2;
    QPointF oldP1(0, 0), oldP2(200, 200);
    rect1->setPos(oldP1);
    rect2->setPos(oldP2);

    QTransform newT1;
    newT1.scale(2.0, 2.0);
    QPointF newP1(10, 10);
    rect1->setTransform(newT1);
    rect1->setPos(newP1);

    QTransform newT2;
    newT2.scale(2.0, 2.0);
    QPointF newP2(210, 210);
    rect2->setTransform(newT2);
    rect2->setPos(newP2);

    // Create composite action with both transforms
    auto composite = std::make_unique<CompositeAction>();
    composite->addAction(std::make_unique<TransformAction>(id1, &store, oldT1,
                                                           newT1, oldP1,
                                                           newP1));
    composite->addAction(std::make_unique<TransformAction>(id2, &store, oldT2,
                                                           newT2, oldP2,
                                                           newP2));

    // Undo composite should revert both items
    composite->undo();
    QCOMPARE(rect1->transform(), oldT1);
    QCOMPARE(rect1->pos(), oldP1);
    QCOMPARE(rect2->transform(), oldT2);
    QCOMPARE(rect2->pos(), oldP2);

    // Redo composite should restore both items
    composite->redo();
    QCOMPARE(rect1->transform(), newT1);
    QCOMPARE(rect1->pos(), newP1);
    QCOMPARE(rect2->transform(), newT2);
    QCOMPARE(rect2->pos(), newP2);
  }

  void testCompositeWithTextResize() {
    QGraphicsScene scene;
    ItemStore store(&scene);

    auto *rect = new QGraphicsRectItem(0, 0, 100, 100);
    auto *textItem = new LatexTextItem();
    scene.addItem(rect);
    scene.addItem(textItem);
    ItemId rectId = store.registerItem(rect);
    ItemId textId = store.registerItem(textItem);

    // Set initial states
    QTransform oldRectT;
    QPointF oldRectP(0, 0);
    rect->setPos(oldRectP);

    QFont oldFont("Arial", 14);
    QPointF oldTextP(100, 100);
    textItem->setFont(oldFont);
    textItem->setPos(oldTextP);

    // Apply transforms
    QTransform newRectT;
    newRectT.scale(1.5, 1.5);
    QPointF newRectP(5, 5);
    rect->setTransform(newRectT);
    rect->setPos(newRectP);

    QFont newFont("Arial", 21);
    QPointF newTextP(110, 110);
    textItem->setFont(newFont);
    textItem->setPos(newTextP);

    // Create composite action with both transform types
    auto composite = std::make_unique<CompositeAction>();
    composite->addAction(std::make_unique<TransformAction>(
        rectId, &store, oldRectT, newRectT, oldRectP, newRectP));
    composite->addAction(std::make_unique<TextResizeAction>(
        textId, &store, oldFont, newFont, oldTextP, newTextP));

    // Undo
    composite->undo();
    QCOMPARE(rect->transform(), oldRectT);
    QCOMPARE(rect->pos(), oldRectP);
    QCOMPARE(textItem->font().pointSize(), oldFont.pointSize());
    QCOMPARE(textItem->pos(), oldTextP);

    // Redo
    composite->redo();
    QCOMPARE(rect->transform(), newRectT);
    QCOMPARE(rect->pos(), newRectP);
    QCOMPARE(textItem->font().pointSize(), newFont.pointSize());
    QCOMPARE(textItem->pos(), newTextP);
  }

  void testTransformActionDescription() {
    QGraphicsScene scene;
    ItemStore store(&scene);

    auto *rect = new QGraphicsRectItem(0, 0, 10, 10);
    scene.addItem(rect);
    ItemId id = store.registerItem(rect);

    TransformAction tAction(id, &store, QTransform(), QTransform(), QPointF(),
                            QPointF());
    QCOMPARE(tAction.description(), QString("Transform"));

    auto *textItem = new LatexTextItem();
    scene.addItem(textItem);
    ItemId textId = store.registerItem(textItem);

    TextResizeAction trAction(textId, &store, QFont(), QFont(), QPointF(),
                              QPointF());
    QCOMPARE(trAction.description(), QString("Text Resize"));
  }

  void testTransformActionInvalidItem() {
    QGraphicsScene scene;
    ItemStore store(&scene);

    // Use a valid-looking but unregistered ItemId
    ItemId fakeId = ItemId::generate();

    TransformAction action(fakeId, &store, QTransform(), QTransform(),
                           QPointF(), QPointF(10, 10));
    // Should not crash on invalid item
    action.undo();
    action.redo();

    TextResizeAction textAction(fakeId, &store, QFont(), QFont(), QPointF(),
                                QPointF(10, 10));
    // Should not crash on invalid item
    textAction.undo();
    textAction.redo();
  }
};

QTEST_MAIN(TestTransformAction)
#include "test_transform_action.moc"
