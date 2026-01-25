/**
 * @file test_item_store.cpp
 * @brief Unit tests for ItemStore, ItemId, and ItemRef classes.
 *
 * Tests cover:
 * - ItemId generation and comparison
 * - ItemStore registration and lookup
 * - Deferred deletion
 * - ItemRef resolution
 * - Undo/redo with item restoration
 */
#include <QtTest/QtTest>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsPathItem>
#include <vector>
#include "../src/core/item_id.h"
#include "../src/core/item_store.h"
#include "../src/core/item_ref.h"
#include "../src/core/scene_controller.h"

class TestItemStore : public QObject {
  Q_OBJECT

private slots:
  void initTestCase() {
    // Initialize test case
  }
  
  void cleanupTestCase() {
    // Clean up test case
  }

  // ItemId tests
  void testItemIdGeneration() {
    ItemId id1 = ItemId::generate();
    ItemId id2 = ItemId::generate();
    
    QVERIFY(id1.isValid());
    QVERIFY(id2.isValid());
    QVERIFY(id1 != id2);
  }
  
  void testItemIdNullByDefault() {
    ItemId id;
    QVERIFY(id.isNull());
    QVERIFY(!id.isValid());
  }
  
  void testItemIdEquality() {
    ItemId id1 = ItemId::generate();
    ItemId id2 = id1;
    
    QVERIFY(id1 == id2);
    
    ItemId id3 = ItemId::generate();
    QVERIFY(id1 != id3);
  }
  
  void testItemIdStringConversion() {
    ItemId id1 = ItemId::generate();
    QString str = id1.toString();
    
    QVERIFY(!str.isEmpty());
    
    ItemId id2 = ItemId::fromString(str);
    QVERIFY(id1 == id2);
  }

  // ItemStore tests
  void testItemStoreRegistration() {
    QGraphicsScene scene;
    ItemStore store(&scene);
    
    auto *rect = new QGraphicsRectItem(0, 0, 100, 100);
    ItemId id = store.registerItem(rect);
    
    QVERIFY(id.isValid());
    QCOMPARE(store.itemCount(), 1);
    QVERIFY(store.contains(id));
    QCOMPARE(store.item(id), rect);
    
    // Item should be in scene
    QCOMPARE(rect->scene(), &scene);
  }
  
  void testItemStoreIdForItem() {
    QGraphicsScene scene;
    ItemStore store(&scene);
    
    auto *rect = new QGraphicsRectItem(0, 0, 100, 100);
    ItemId id = store.registerItem(rect);
    
    ItemId foundId = store.idForItem(rect);
    QVERIFY(foundId.isValid());
    QCOMPARE(foundId, id);
  }
  
  void testItemStoreUnregister() {
    QGraphicsScene scene;
    ItemStore store(&scene);
    
    auto *rect = new QGraphicsRectItem(0, 0, 100, 100);
    ItemId id = store.registerItem(rect);
    
    QGraphicsItem *unregistered = store.unregisterItem(id);
    QCOMPARE(unregistered, rect);
    QCOMPARE(store.itemCount(), 0);
    QVERIFY(!store.contains(id));
    
    // Clean up
    delete rect;
  }
  
  void testItemStoreDeferredDeletion() {
    QGraphicsScene scene;
    ItemStore store(&scene);
    
    auto *rect = new QGraphicsRectItem(0, 0, 100, 100);
    ItemId id = store.registerItem(rect);
    
    // Schedule for deletion
    store.scheduleDelete(id);
    
    // Item should be removed from tracking
    QVERIFY(!store.contains(id));
    QCOMPARE(store.item(id), nullptr);
    
    // Item should be removed from scene
    QCOMPARE(rect->scene(), nullptr);
    
    // Flush deletions to actually delete
    store.flushDeletions();
    // At this point, rect is deleted - don't access it
  }
  
  void testItemStoreSnapshotForUndo() {
    QGraphicsScene scene;
    ItemStore store(&scene);
    
    auto *rect = new QGraphicsRectItem(0, 0, 100, 100);
    ItemId id = store.registerItem(rect);
    
    // Schedule deletion but keep for undo
    store.scheduleDelete(id, true);
    
    // Item should not be in active tracking
    QVERIFY(!store.contains(id));
    QCOMPARE(store.item(id), nullptr);
    
    // Restore the item
    bool restored = store.restoreItem(id);
    QVERIFY(restored);
    QVERIFY(store.contains(id));
    QCOMPARE(store.item(id), rect);
    
    // Item should be back in scene
    QCOMPARE(rect->scene(), &scene);
  }
  
  void testItemStoreClear() {
    QGraphicsScene scene;
    ItemStore store(&scene);
    
    auto *rect1 = new QGraphicsRectItem(0, 0, 100, 100);
    auto *rect2 = new QGraphicsRectItem(50, 50, 100, 100);
    
    store.registerItem(rect1);
    store.registerItem(rect2);
    QCOMPARE(store.itemCount(), 2);
    
    store.clear();
    store.flushDeletions();
    QCOMPARE(store.itemCount(), 0);
  }

  // ItemRef tests
  void testItemRefResolution() {
    QGraphicsScene scene;
    ItemStore store(&scene);
    
    auto *rect = new QGraphicsRectItem(0, 0, 100, 100);
    ItemId id = store.registerItem(rect);
    
    ItemRef ref(&store, id);
    QVERIFY(ref.isValid());
    QCOMPARE(ref.get(), rect);
    QVERIFY(ref);  // Boolean conversion
  }
  
  void testItemRefNullAfterDeletion() {
    QGraphicsScene scene;
    ItemStore store(&scene);
    
    auto *rect = new QGraphicsRectItem(0, 0, 100, 100);
    ItemId id = store.registerItem(rect);
    
    ItemRef ref(&store, id);
    QVERIFY(ref.isValid());
    
    // Delete the item
    store.scheduleDelete(id);
    store.flushDeletions();
    
    // Ref should now return nullptr
    QVERIFY(!ref.isValid());
    QCOMPARE(ref.get(), nullptr);
  }
  
  void testItemRefTypedAccess() {
    QGraphicsScene scene;
    ItemStore store(&scene);
    
    auto *rect = new QGraphicsRectItem(0, 0, 100, 100);
    ItemId id = store.registerItem(rect);
    
    ItemRef ref(&store, id);
    
    // Correct type cast
    auto *asRect = ref.getAs<QGraphicsRectItem>();
    QVERIFY(asRect != nullptr);
    QCOMPARE(asRect, rect);
    
    // Wrong type cast
    auto *asPath = ref.getAs<QGraphicsPathItem>();
    QCOMPARE(asPath, nullptr);
  }

  // SceneController tests
  void testSceneControllerAddItem() {
    QGraphicsScene scene;
    SceneController controller(&scene);
    
    auto *rect = new QGraphicsRectItem(0, 0, 100, 100);
    ItemId id = controller.addItem(rect);
    
    QVERIFY(id.isValid());
    QCOMPARE(controller.item(id), rect);
    QCOMPARE(rect->scene(), &scene);
  }
  
  void testSceneControllerRemoveItem() {
    QGraphicsScene scene;
    SceneController controller(&scene);
    
    auto *rect = new QGraphicsRectItem(0, 0, 100, 100);
    ItemId id = controller.addItem(rect);
    
    bool removed = controller.removeItem(id);
    QVERIFY(removed);
    QCOMPARE(controller.item(id), nullptr);
  }
  
  void testSceneControllerRestoreItem() {
    QGraphicsScene scene;
    SceneController controller(&scene);
    
    auto *rect = new QGraphicsRectItem(0, 0, 100, 100);
    ItemId id = controller.addItem(rect);
    
    // Remove with keepForUndo
    controller.removeItem(id, true);
    QCOMPARE(controller.item(id), nullptr);
    
    // Restore
    bool restored = controller.restoreItem(id);
    QVERIFY(restored);
    QCOMPARE(controller.item(id), rect);
    QCOMPARE(rect->scene(), &scene);
  }
  
  void testSceneControllerMoveItem() {
    QGraphicsScene scene;
    SceneController controller(&scene);
    
    auto *rect = new QGraphicsRectItem(0, 0, 100, 100);
    ItemId id = controller.addItem(rect);
    
    QPointF newPos(50, 75);
    bool moved = controller.moveItem(id, newPos);
    QVERIFY(moved);
    QCOMPARE(rect->pos(), newPos);
  }

  // Stress tests
  void testRapidCreateDelete() {
    QGraphicsScene scene;
    ItemStore store(&scene);
    
    // Create and delete many items rapidly
    for (int i = 0; i < 100; ++i) {
      auto *rect = new QGraphicsRectItem(0, 0, 10, 10);
      ItemId id = store.registerItem(rect);
      store.scheduleDelete(id);
    }
    
    // All should be pending deletion
    QCOMPARE(store.itemCount(), 0);
    
    // Flush and ensure no crashes
    store.flushDeletions();
  }
  
  void testEraseUndoRedoCycle() {
    QGraphicsScene scene;
    ItemStore store(&scene);
    
    // Create items
    std::vector<ItemId> ids;
    for (int i = 0; i < 5; ++i) {
      auto *rect = new QGraphicsRectItem(i * 20, 0, 15, 15);
      ids.push_back(store.registerItem(rect));
    }
    QCOMPARE(store.itemCount(), 5);
    
    // "Erase" items (keep for undo)
    for (const ItemId &id : ids) {
      store.scheduleDelete(id, true);
    }
    QCOMPARE(store.itemCount(), 0);
    
    // Undo - restore items
    for (const ItemId &id : ids) {
      bool restored = store.restoreItem(id);
      QVERIFY(restored);
    }
    QCOMPARE(store.itemCount(), 5);
    
    // Redo - delete again
    for (const ItemId &id : ids) {
      store.scheduleDelete(id, true);
    }
    QCOMPARE(store.itemCount(), 0);
    
    // Final cleanup
    store.flushDeletions();
  }
};

QTEST_MAIN(TestItemStore)
#include "test_item_store.moc"
