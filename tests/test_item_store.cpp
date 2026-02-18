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
 * - Pointer safety mechanisms (itemAboutToBeDeleted signal, subscriber
 * notification)
 * - Code path tolerance for missing items
 * - SceneController graceful handling of deleted items
 */
#include "../src/core/item_id.h"
#include "../src/core/item_ref.h"
#include "../src/core/item_store.h"
#include "../src/core/layer.h"
#include "../src/core/scene_controller.h"
#include <QGraphicsItemGroup>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QtTest/QtTest>
#include <vector>

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
    QVERIFY(ref); // Boolean conversion
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

  // ========== Pointer Safety Tests ==========
  // These tests verify the mechanisms that prevent use-after-free

  void testItemAboutToBeDeletedSignal() {
    QGraphicsScene scene;
    ItemStore store(&scene);

    auto *rect = new QGraphicsRectItem(0, 0, 100, 100);
    ItemId id = store.registerItem(rect);

    // Track if signal was emitted
    ItemId deletedId;
    bool signalEmitted = false;
    QObject::connect(&store, &ItemStore::itemAboutToBeDeleted,
                     [&](const ItemId &id) {
                       deletedId = id;
                       signalEmitted = true;
                     });

    // Schedule deletion
    store.scheduleDelete(id);

    // Verify signal was emitted with correct ID
    QVERIFY(signalEmitted);
    QCOMPARE(deletedId, id);

    // Clean up
    store.flushDeletions();
  }

  void testItemRefInvalidAfterDeletion() {
    QGraphicsScene scene;
    ItemStore store(&scene);

    auto *rect = new QGraphicsRectItem(0, 0, 100, 100);
    ItemId id = store.registerItem(rect);

    // Create an ItemRef to track the item
    ItemRef ref(&store, id);
    QVERIFY(ref.isValid());
    QCOMPARE(ref.get(), rect);

    // Delete the item
    store.scheduleDelete(id);

    // Ref should now be invalid (returns nullptr)
    QVERIFY(!ref.isValid());
    QCOMPARE(ref.get(), nullptr);

    // Clean up
    store.flushDeletions();
  }

  void testSubscriberClearsStoredIdOnDeletion() {
    QGraphicsScene scene;
    ItemStore store(&scene);

    auto *rect = new QGraphicsRectItem(0, 0, 100, 100);
    ItemId id = store.registerItem(rect);

    // Simulate a subscriber that caches an ItemId
    ItemId cachedId = id;
    bool cleared = false;

    // Subscribe to deletion signal and clear cached ID
    QObject::connect(&store, &ItemStore::itemAboutToBeDeleted,
                     [&](const ItemId &deletedId) {
                       if (cachedId == deletedId) {
                         cachedId = ItemId(); // Clear the cached ID
                         cleared = true;
                       }
                     });

    // Delete the item
    store.scheduleDelete(id);

    // Verify subscriber cleared its cached ID
    QVERIFY(cleared);
    QVERIFY(!cachedId.isValid());

    // Clean up
    store.flushDeletions();
  }

  void testCodePathToleratesMissingItem() {
    QGraphicsScene scene;
    ItemStore store(&scene);

    auto *rect = new QGraphicsRectItem(0, 0, 100, 100);
    ItemId id = store.registerItem(rect);

    // Delete the item
    store.scheduleDelete(id);
    store.flushDeletions();

    // Attempt to access the item - should return nullptr, not crash
    QGraphicsItem *item = store.item(id);
    QCOMPARE(item, nullptr);

    // Verify store operations handle missing items gracefully
    QVERIFY(!store.contains(id));

    // ItemRef should also handle this gracefully
    ItemRef ref(&store, id);
    QVERIFY(!ref.isValid());
    QCOMPARE(ref.get(), nullptr);
  }

  void testItemRefTypedAccessWithDeletedItem() {
    QGraphicsScene scene;
    ItemStore store(&scene);

    auto *rect = new QGraphicsRectItem(0, 0, 100, 100);
    ItemId id = store.registerItem(rect);

    ItemRef ref(&store, id);

    // Delete the item
    store.scheduleDelete(id);
    store.flushDeletions();

    // Typed access should return nullptr, not crash
    auto *asRect = ref.getAs<QGraphicsRectItem>();
    QCOMPARE(asRect, nullptr);
  }

  void testMultipleSubscribersNotified() {
    QGraphicsScene scene;
    ItemStore store(&scene);

    auto *rect = new QGraphicsRectItem(0, 0, 100, 100);
    ItemId id = store.registerItem(rect);

    // Multiple subscribers tracking the same item
    int notificationCount = 0;
    ItemId subscriber1Id = id;
    ItemId subscriber2Id = id;

    QObject::connect(&store, &ItemStore::itemAboutToBeDeleted,
                     [&](const ItemId &deletedId) {
                       if (subscriber1Id == deletedId) {
                         subscriber1Id = ItemId();
                         notificationCount++;
                       }
                     });

    QObject::connect(&store, &ItemStore::itemAboutToBeDeleted,
                     [&](const ItemId &deletedId) {
                       if (subscriber2Id == deletedId) {
                         subscriber2Id = ItemId();
                         notificationCount++;
                       }
                     });

    // Delete the item
    store.scheduleDelete(id);

    // Both subscribers should have been notified
    QCOMPARE(notificationCount, 2);
    QVERIFY(!subscriber1Id.isValid());
    QVERIFY(!subscriber2Id.isValid());

    // Clean up
    store.flushDeletions();
  }

  void testSceneControllerRemoveItemGracefully() {
    QGraphicsScene scene;
    SceneController controller(&scene);

    auto *rect = new QGraphicsRectItem(0, 0, 100, 100);
    ItemId id = controller.addItem(rect);

    // Remove the item
    bool removed = controller.removeItem(id);
    QVERIFY(removed);

    // Attempting to remove again should return false, not crash
    controller.flushDeletions();
    bool removedAgain = controller.removeItem(id);
    QVERIFY(!removedAgain);

    // Verify item accessor returns nullptr
    QCOMPARE(controller.item(id), nullptr);
  }

  void testSceneControllerMoveItemWithMissingItem() {
    QGraphicsScene scene;
    SceneController controller(&scene);

    auto *rect = new QGraphicsRectItem(0, 0, 100, 100);
    ItemId id = controller.addItem(rect);

    // Remove the item
    controller.removeItem(id);
    controller.flushDeletions();

    // Attempting to move a deleted item should return false, not crash
    bool moved = controller.moveItem(id, QPointF(50, 50));
    QVERIFY(!moved);
  }

  void testItemRefResolutionAfterRestore() {
    QGraphicsScene scene;
    ItemStore store(&scene);

    auto *rect = new QGraphicsRectItem(0, 0, 100, 100);
    ItemId id = store.registerItem(rect);

    // Create ref before deletion
    ItemRef ref(&store, id);
    QVERIFY(ref.isValid());

    // Delete with keepSnapshot for undo
    store.scheduleDelete(id, true);
    QVERIFY(!ref.isValid());

    // Restore the item (undo operation)
    bool restored = store.restoreItem(id);
    QVERIFY(restored);

    // Ref should be valid again after restore
    QVERIFY(ref.isValid());
    QCOMPARE(ref.get(), rect);
  }

  // ========== ScaleLayer Tests ==========

  void testScaleLayerScalesItems() {
    QGraphicsScene scene;
    SceneController controller(&scene);
    LayerManager layerManager(&scene);
    controller.setLayerManager(&layerManager);

    Layer *layer = layerManager.activeLayer();
    QVERIFY(layer);

    // Add two items
    auto *rect1 = new QGraphicsRectItem(0, 0, 50, 50);
    rect1->setPos(0, 0);
    controller.addItem(rect1);

    auto *rect2 = new QGraphicsRectItem(0, 0, 50, 50);
    rect2->setPos(100, 0);
    controller.addItem(rect2);

    QCOMPARE(layer->itemCount(), 2);

    // Scale the layer by 2x
    int scaled = controller.scaleLayer(layer, 2.0, 2.0);
    QCOMPARE(scaled, 2);

    // Items should have scale transforms applied
    QTransform t1 = rect1->transform();
    QTransform t2 = rect2->transform();

    // Both items should have 2x scale transform
    QVERIFY(qFuzzyCompare(t1.m11(), 2.0));
    QVERIFY(qFuzzyCompare(t1.m22(), 2.0));
    QVERIFY(qFuzzyCompare(t2.m11(), 2.0));
    QVERIFY(qFuzzyCompare(t2.m22(), 2.0));
  }

  void testScaleLayerEmptyLayer() {
    QGraphicsScene scene;
    SceneController controller(&scene);
    LayerManager layerManager(&scene);
    controller.setLayerManager(&layerManager);

    Layer *layer = layerManager.activeLayer();
    QVERIFY(layer);
    QCOMPARE(layer->itemCount(), 0);

    int scaled = controller.scaleLayer(layer, 2.0, 2.0);
    QCOMPARE(scaled, 0);
  }

  void testScaleLayerNullLayer() {
    QGraphicsScene scene;
    SceneController controller(&scene);

    int scaled = controller.scaleLayer(nullptr, 2.0, 2.0);
    QCOMPARE(scaled, 0);
  }

  void testScaleLayerNonUniform() {
    QGraphicsScene scene;
    SceneController controller(&scene);
    LayerManager layerManager(&scene);
    controller.setLayerManager(&layerManager);

    Layer *layer = layerManager.activeLayer();
    QVERIFY(layer);

    auto *rect = new QGraphicsRectItem(0, 0, 50, 50);
    rect->setPos(0, 0);
    controller.addItem(rect);

    // Scale non-uniformly (2x width, 3x height)
    int scaled = controller.scaleLayer(layer, 2.0, 3.0);
    QCOMPARE(scaled, 1);

    QTransform t = rect->transform();
    QVERIFY(qFuzzyCompare(t.m11(), 2.0));
    QVERIFY(qFuzzyCompare(t.m22(), 3.0));
  }

  // ========== Layer Merge Tests ==========

  void testMergeItemsCreatesGroup() {
    QGraphicsScene scene;
    SceneController controller(&scene);
    LayerManager manager(&scene);
    controller.setLayerManager(&manager);

    // Add items to active layer
    auto *rect1 = new QGraphicsRectItem(0, 0, 50, 50);
    auto *rect2 = new QGraphicsRectItem(60, 0, 50, 50);
    ItemId id1 = controller.addItem(rect1);
    ItemId id2 = controller.addItem(rect2);

    Layer *layer = manager.activeLayer();
    QVERIFY(layer);
    QCOMPARE(layer->itemCount(), 2);

    // Merge items
    QList<ItemId> ids = {id1, id2};
    ItemId groupId = manager.mergeItems(ids);

    QVERIFY(groupId.isValid());
    // Layer should now have 1 item (the group)
    QCOMPARE(layer->itemCount(), 1);
    QVERIFY(layer->containsItem(groupId));

    // The group should be a QGraphicsItemGroup
    QGraphicsItem *groupItem = controller.itemStore()->item(groupId);
    QVERIFY(groupItem);
    QVERIFY(dynamic_cast<QGraphicsItemGroup *>(groupItem) != nullptr);

    // Original items should no longer be in ItemStore
    QVERIFY(!controller.itemStore()->contains(id1));
    QVERIFY(!controller.itemStore()->contains(id2));
  }

  void testMergeItemsRequiresAtLeastTwo() {
    QGraphicsScene scene;
    SceneController controller(&scene);
    LayerManager manager(&scene);
    controller.setLayerManager(&manager);

    auto *rect = new QGraphicsRectItem(0, 0, 50, 50);
    ItemId id = controller.addItem(rect);

    // Merging a single item should fail
    QList<ItemId> ids = {id};
    ItemId groupId = manager.mergeItems(ids);
    QVERIFY(!groupId.isValid());
  }

  void testMergeItemsFromDifferentLayersFails() {
    QGraphicsScene scene;
    SceneController controller(&scene);
    LayerManager manager(&scene);
    controller.setLayerManager(&manager);

    // Add item to first layer
    auto *rect1 = new QGraphicsRectItem(0, 0, 50, 50);
    ItemId id1 = controller.addItem(rect1);

    // Create second layer and add item to it
    manager.createLayer("Layer 2");
    manager.setActiveLayer(1);
    auto *rect2 = new QGraphicsRectItem(60, 0, 50, 50);
    ItemId id2 = controller.addItem(rect2);

    // Merging items from different layers should fail
    QList<ItemId> ids = {id1, id2};
    ItemId groupId = manager.mergeItems(ids);
    QVERIFY(!groupId.isValid());
  }

  void testFlattenAllMergesLayers() {
    QGraphicsScene scene;
    SceneController controller(&scene);
    LayerManager manager(&scene);
    controller.setLayerManager(&manager);

    // Add items to first layer
    auto *rect1 = new QGraphicsRectItem(0, 0, 50, 50);
    controller.addItem(rect1);

    // Create second layer and add item
    manager.createLayer("Layer 2");
    manager.setActiveLayer(1);
    auto *rect2 = new QGraphicsRectItem(60, 0, 50, 50);
    controller.addItem(rect2);

    QCOMPARE(manager.layerCount(), 2);

    // Flatten all
    Layer *flattened = manager.flattenAll();
    QVERIFY(flattened);
    QCOMPARE(manager.layerCount(), 1);
    QCOMPARE(flattened->itemCount(), 2);
    QCOMPARE(flattened->name(), "Flattened");
  }

  void testMergeDownCombinesLayers() {
    QGraphicsScene scene;
    SceneController controller(&scene);
    LayerManager manager(&scene);
    controller.setLayerManager(&manager);

    // Add item to first layer (index 0)
    auto *rect1 = new QGraphicsRectItem(0, 0, 50, 50);
    controller.addItem(rect1);

    // Create second layer (index 1) and add item
    manager.createLayer("Layer 2");
    manager.setActiveLayer(1);
    auto *rect2 = new QGraphicsRectItem(60, 0, 50, 50);
    controller.addItem(rect2);

    QCOMPARE(manager.layerCount(), 2);

    // Merge layer 1 down into layer 0
    bool merged = manager.mergeDown(1);
    QVERIFY(merged);
    QCOMPARE(manager.layerCount(), 1);

    Layer *remaining = manager.layer(0);
    QVERIFY(remaining);
    QCOMPARE(remaining->itemCount(), 2);
  }

  // ========== Blend Mode Tests ==========

  void testBlendModeDefaultNormal() {
    Layer layer("Test");
    QCOMPARE(layer.blendMode(), Layer::BlendMode::Normal);
  }

  void testBlendModeSetGet() {
    Layer layer("Test");

    layer.setBlendMode(Layer::BlendMode::Multiply);
    QCOMPARE(layer.blendMode(), Layer::BlendMode::Multiply);

    layer.setBlendMode(Layer::BlendMode::Screen);
    QCOMPARE(layer.blendMode(), Layer::BlendMode::Screen);

    layer.setBlendMode(Layer::BlendMode::Overlay);
    QCOMPARE(layer.blendMode(), Layer::BlendMode::Overlay);
  }

  void testBlendModeToCompositionMode() {
    QCOMPARE(Layer::toCompositionMode(Layer::BlendMode::Normal),
             QPainter::CompositionMode_SourceOver);
    QCOMPARE(Layer::toCompositionMode(Layer::BlendMode::Multiply),
             QPainter::CompositionMode_Multiply);
    QCOMPARE(Layer::toCompositionMode(Layer::BlendMode::Screen),
             QPainter::CompositionMode_Screen);
    QCOMPARE(Layer::toCompositionMode(Layer::BlendMode::Overlay),
             QPainter::CompositionMode_Overlay);
    QCOMPARE(Layer::toCompositionMode(Layer::BlendMode::Darken),
             QPainter::CompositionMode_Darken);
    QCOMPARE(Layer::toCompositionMode(Layer::BlendMode::Lighten),
             QPainter::CompositionMode_Lighten);
    QCOMPARE(Layer::toCompositionMode(Layer::BlendMode::ColorDodge),
             QPainter::CompositionMode_ColorDodge);
    QCOMPARE(Layer::toCompositionMode(Layer::BlendMode::ColorBurn),
             QPainter::CompositionMode_ColorBurn);
    QCOMPARE(Layer::toCompositionMode(Layer::BlendMode::HardLight),
             QPainter::CompositionMode_HardLight);
    QCOMPARE(Layer::toCompositionMode(Layer::BlendMode::SoftLight),
             QPainter::CompositionMode_SoftLight);
    QCOMPARE(Layer::toCompositionMode(Layer::BlendMode::Difference),
             QPainter::CompositionMode_Difference);
    QCOMPARE(Layer::toCompositionMode(Layer::BlendMode::Exclusion),
             QPainter::CompositionMode_Exclusion);
  }

  void testBlendModeDuplicateLayer() {
    QGraphicsScene scene;
    SceneController controller(&scene);
    LayerManager manager(&scene);
    controller.setLayerManager(&manager);

    Layer *layer = manager.activeLayer();
    QVERIFY(layer);
    layer->setBlendMode(Layer::BlendMode::Screen);

    Layer *copy = manager.duplicateLayer(0);
    QVERIFY(copy);
    QCOMPARE(copy->blendMode(), Layer::BlendMode::Screen);
  }

  void testBlendModeMoveConstructor() {
    Layer original("Test");
    original.setBlendMode(Layer::BlendMode::Overlay);

    Layer moved(std::move(original));
    QCOMPARE(moved.blendMode(), Layer::BlendMode::Overlay);
  }
};

QTEST_MAIN(TestItemStore)
#include "test_item_store.moc"
