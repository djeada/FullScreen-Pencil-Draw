/**
 * @file test_undo_redo_manager.cpp
 * @brief Tests for UndoRedoManager, including the configurable
 *        step / memory-budget history policy.
 */
#include <QtTest/QtTest>

#include "../src/core/action.h"
#include "../src/core/undo_redo_manager.h"

// Minimal concrete Action used only by these tests.
class StubAction : public Action {
public:
  explicit StubAction(int *undoCounter = nullptr, int *redoCounter = nullptr)
      : undoCounter_(undoCounter), redoCounter_(redoCounter) {}

  void undo() override {
    if (undoCounter_)
      ++(*undoCounter_);
  }
  void redo() override {
    if (redoCounter_)
      ++(*redoCounter_);
  }
  QString description() const override { return "Stub"; }

private:
  int *undoCounter_;
  int *redoCounter_;
};

// Action that reports an ItemId, so discard notifications can be observed.
class StubItemAction : public Action {
public:
  explicit StubItemAction(const ItemId &id) : id_(id) {}

  void undo() override {}
  void redo() override {}
  QString description() const override { return "StubItem"; }
  void collectReferencedItems(QVector<ItemId> &out) const override {
    out.append(id_);
  }

private:
  ItemId id_;
};

// Action with a configurable memory footprint (like a raster edit).
class HeavyAction : public Action {
public:
  HeavyAction(std::size_t bytes, const ItemId &id = ItemId())
      : bytes_(bytes), id_(id) {}
  void undo() override {}
  void redo() override {}
  std::size_t memoryCost() const override { return bytes_; }
  void collectReferencedItems(QVector<ItemId> &out) const override {
    if (id_.isValid())
      out.append(id_);
  }

private:
  std::size_t bytes_;
  ItemId id_;
};

// Action whose undo pushes another action (like a replay callback that
// commits an in-progress gesture).
class PushingAction : public Action {
public:
  explicit PushingAction(UndoRedoManager *mgr) : mgr_(mgr) {}
  void undo() override { mgr_->push(std::make_unique<StubAction>()); }
  void redo() override {}

private:
  UndoRedoManager *mgr_;
};

class TestUndoRedoManager : public QObject {
  Q_OBJECT

private slots:

  // Basic push / undo / redo still works.
  void testBasicPushUndoRedo() {
    UndoRedoManager mgr;
    QVERIFY(!mgr.canUndo());
    QVERIFY(!mgr.canRedo());

    int undos = 0, redos = 0;
    mgr.push(std::make_unique<StubAction>(&undos, &redos));
    QVERIFY(mgr.canUndo());
    QVERIFY(!mgr.canRedo());

    mgr.undo();
    QCOMPARE(undos, 1);
    QVERIFY(!mgr.canUndo());
    QVERIFY(mgr.canRedo());

    mgr.redo();
    QCOMPARE(redos, 1);
    QVERIFY(mgr.canUndo());
    QVERIFY(!mgr.canRedo());
  }

  // Pushing more than the step limit drops the oldest actions.
  void testPushEnforcesLimit() {
    UndoRedoManager mgr;
    constexpr std::size_t limit = UndoRedoManager::kDefaultMaxSteps;

    for (std::size_t i = 0; i < limit + 10; ++i) {
      mgr.push(std::make_unique<StubAction>());
    }

    // We should be able to undo exactly `limit` times.
    std::size_t undone = 0;
    while (mgr.canUndo()) {
      mgr.undo();
      ++undone;
    }
    QCOMPARE(undone, limit);
  }

  // Redo that re-fills the undo stack also respects the limit.
  void testRedoEnforcesLimit() {
    UndoRedoManager mgr;
    constexpr std::size_t limit = UndoRedoManager::kDefaultMaxSteps;

    // Fill the undo stack to capacity.
    for (std::size_t i = 0; i < limit; ++i) {
      mgr.push(std::make_unique<StubAction>());
    }

    // Undo all, then redo all.
    for (std::size_t i = 0; i < limit; ++i) {
      mgr.undo();
    }
    for (std::size_t i = 0; i < limit; ++i) {
      mgr.redo();
    }

    // Should still be capped at `limit`.
    std::size_t undone = 0;
    while (mgr.canUndo()) {
      mgr.undo();
      ++undone;
    }
    QCOMPARE(undone, limit);
  }

  // The default policy matches the documented values.
  void testDefaultPolicy() {
    UndoRedoManager mgr;
    QCOMPARE(mgr.policy().maxSteps, UndoRedoManager::kDefaultMaxSteps);
    QCOMPARE(mgr.policy().memoryBudgetBytes,
             UndoRedoManager::kDefaultMemoryBudgetBytes);
    QCOMPARE(HistoryPolicy().maxSteps, UndoRedoManager::kDefaultMaxSteps);
  }

  // clear() empties both stacks.
  void testClear() {
    UndoRedoManager mgr;
    mgr.push(std::make_unique<StubAction>());
    mgr.push(std::make_unique<StubAction>());
    mgr.undo();
    QVERIFY(mgr.canUndo());
    QVERIFY(mgr.canRedo());

    mgr.clear();
    QVERIFY(!mgr.canUndo());
    QVERIFY(!mgr.canRedo());
  }

  // Oldest action is the one dropped when the limit is exceeded.
  void testOldestActionDropped() {
    UndoRedoManager mgr;
    constexpr std::size_t limit = UndoRedoManager::kDefaultMaxSteps;

    int firstUndos = 0;
    mgr.push(std::make_unique<StubAction>(&firstUndos, nullptr));

    // Push `limit` more actions → the first one should be evicted.
    for (std::size_t i = 0; i < limit; ++i) {
      mgr.push(std::make_unique<StubAction>());
    }

    // Undo everything that remains – the first action's counter must stay 0.
    while (mgr.canUndo()) {
      mgr.undo();
    }
    QCOMPARE(firstUndos, 0);
  }

  // Actions evicted by the history limit must report their items so parked
  // undo snapshots can be released instead of leaking.
  void testEvictedActionsNotifyDiscardListeners() {
    UndoRedoManager mgr;
    QVector<ItemId> discarded;
    mgr.addDiscardListener(
        [&discarded](const ItemId &id) { discarded.append(id); });

    const ItemId first = ItemId::generate();
    mgr.push(std::make_unique<StubItemAction>(first));
    for (std::size_t i = 0; i < UndoRedoManager::kDefaultMaxSteps; ++i) {
      mgr.push(std::make_unique<StubItemAction>(ItemId::generate()));
    }

    QCOMPARE(discarded.size(), 1);
    QCOMPARE(discarded.first(), first);
  }

  // Pushing after an undo drops the redo stack – those actions are gone for
  // good and must be reported too.
  void testInvalidatedRedoStackNotifiesDiscardListeners() {
    UndoRedoManager mgr;
    QVector<ItemId> discarded;
    mgr.addDiscardListener(
        [&discarded](const ItemId &id) { discarded.append(id); });

    const ItemId undone = ItemId::generate();
    mgr.push(std::make_unique<StubItemAction>(undone));
    mgr.undo();
    QVERIFY(mgr.canRedo());

    mgr.push(std::make_unique<StubItemAction>(ItemId::generate()));
    QVERIFY(!mgr.canRedo());
    QCOMPARE(discarded.size(), 1);
    QCOMPARE(discarded.first(), undone);
  }

  // clear() destroys both stacks; every referenced item must be reported.
  void testClearNotifiesDiscardListenersForBothStacks() {
    UndoRedoManager mgr;
    QVector<ItemId> discarded;
    mgr.addDiscardListener(
        [&discarded](const ItemId &id) { discarded.append(id); });

    mgr.push(std::make_unique<StubItemAction>(ItemId::generate()));
    mgr.push(std::make_unique<StubItemAction>(ItemId::generate()));
    mgr.undo(); // one action moves to the redo stack

    mgr.clear();
    QCOMPARE(discarded.size(), 2);
    QVERIFY(!mgr.canUndo());
    QVERIFY(!mgr.canRedo());
  }

  // Delete -> undo -> delete again: the invalidated first delete references
  // the same item as the new one, so it must not be reported (its snapshot
  // is still needed to undo the second delete).
  void testDiscardSkipsItemsStillReferenced() {
    UndoRedoManager mgr;
    QVector<ItemId> discarded;
    mgr.addDiscardListener(
        [&discarded](const ItemId &id) { discarded.append(id); });

    const ItemId item = ItemId::generate();
    mgr.push(std::make_unique<StubItemAction>(item));
    mgr.undo();
    mgr.push(std::make_unique<StubItemAction>(item));
    QVERIFY(discarded.isEmpty());

    // Same for history eviction: an older entry for an item that a newer
    // entry still references stays quiet.
    for (std::size_t i = 0; i < UndoRedoManager::kDefaultMaxSteps - 2; ++i) {
      mgr.push(std::make_unique<StubItemAction>(ItemId::generate()));
    }
    mgr.push(std::make_unique<StubItemAction>(item));
    QVERIFY(!discarded.contains(item));
  }

  // Canvas and PDF viewer share one history; closing the PDF must only drop
  // the PDF's own actions.
  void testClearOwnedByKeepsOtherOwners() {
    UndoRedoManager mgr;
    int canvasOwner = 0, pdfOwner = 0;
    int canvasUndos = 0;
    mgr.push(std::make_unique<StubAction>(&canvasUndos), &canvasOwner);
    mgr.push(std::make_unique<StubAction>(), &pdfOwner);
    mgr.push(std::make_unique<StubAction>(), &pdfOwner);
    mgr.undo(); // one PDF action on the redo stack

    QCOMPARE(mgr.undoOwner(), static_cast<const void *>(&pdfOwner));
    QCOMPARE(mgr.redoOwner(), static_cast<const void *>(&pdfOwner));
    mgr.clearOwnedBy(&pdfOwner);
    QVERIFY(mgr.canUndo());
    QVERIFY(!mgr.canRedo());
    mgr.undo();
    QCOMPARE(canvasUndos, 1);
    QVERIFY(!mgr.canUndo());
  }

  // A push from inside undo() is queued until the replay finished, so the
  // undone action still lands on the redo stack first (and is then
  // invalidated by the new action, as for any regular push).
  void testPushDuringReplayIsDeferred() {
    UndoRedoManager mgr;
    mgr.push(std::make_unique<StubAction>());
    mgr.push(std::make_unique<PushingAction>(&mgr));
    mgr.undo();
    QVERIFY(!mgr.canRedo()); // the deferred push invalidated redo
    QVERIFY(mgr.canUndo());
    mgr.undo(); // the pushed stub
    mgr.undo(); // the first stub
    QVERIFY(!mgr.canUndo());
  }

  // ---- History policy (#168) ----

  void testMemoryBudgetEvictsOldestFirst() {
    UndoRedoManager mgr;
    constexpr std::size_t mb = 1024 * 1024;
    mgr.setPolicy({/*maxSteps=*/0, /*memoryBudgetBytes=*/10 * mb});
    QVector<ItemId> discarded;
    mgr.addDiscardListener(
        [&discarded](const ItemId &id) { discarded.append(id); });
    QList<ItemId> ids;
    for (int i = 0; i < 10; ++i) {
      ids.append(ItemId::generate());
      mgr.push(std::make_unique<HeavyAction>(3 * mb, ids.last()));
    }
    // 3 MiB steps under a 10 MiB budget: three stay, oldest go first.
    QCOMPARE(mgr.undoCount(), std::size_t(3));
    QVERIFY(mgr.memoryUsage() <= 10 * mb);
    QCOMPARE(discarded.size(), 7);
    QCOMPARE(discarded.first(), ids.first());
    QVERIFY(!discarded.contains(ids.last()));
  }

  void testLatestActionSurvivesAnOversizedBudget() {
    UndoRedoManager mgr;
    mgr.setPolicy({0, 1000});
    mgr.push(std::make_unique<HeavyAction>(5000));
    mgr.push(std::make_unique<HeavyAction>(5000));
    // The most recent step always stays undoable.
    QCOMPARE(mgr.undoCount(), std::size_t(1));
    QVERIFY(mgr.canUndo());
  }

  void testUnlimitedStepsWithSmallActions() {
    UndoRedoManager mgr;
    mgr.setPolicy({0, 0}); // both limits disabled
    for (int i = 0; i < 1000; ++i)
      mgr.push(std::make_unique<StubAction>());
    QCOMPARE(mgr.undoCount(), std::size_t(1000));
  }

  void testTighterPolicyTrimsExistingHistoryAndReleasesSnapshots() {
    UndoRedoManager mgr;
    QVector<ItemId> discarded;
    mgr.addDiscardListener(
        [&discarded](const ItemId &id) { discarded.append(id); });
    for (int i = 0; i < 20; ++i)
      mgr.push(std::make_unique<StubItemAction>(ItemId::generate()));
    mgr.setPolicy({5, 0});
    QCOMPARE(mgr.undoCount(), std::size_t(5));
    QCOMPARE(discarded.size(), 15);
  }

  void testRedoStackIsTrimmedWhenOverBudget() {
    UndoRedoManager mgr;
    mgr.setPolicy({0, 0});
    for (int i = 0; i < 5; ++i)
      mgr.push(std::make_unique<HeavyAction>(1000));
    for (int i = 0; i < 4; ++i)
      mgr.undo();
    QCOMPARE(mgr.redoCount(), std::size_t(4));
    mgr.setPolicy({0, 2500});
    // One undo step is kept; redo steps furthest from now are dropped.
    QCOMPARE(mgr.undoCount(), std::size_t(1));
    QVERIFY(mgr.memoryUsage() <= 2500);
    QCOMPARE(mgr.redoCount(), std::size_t(1));
  }

  void testRedoInvalidationReleasesMemory() {
    UndoRedoManager mgr;
    mgr.setPolicy({0, 0});
    mgr.push(std::make_unique<HeavyAction>(4000));
    mgr.push(std::make_unique<HeavyAction>(4000));
    mgr.undo();
    QCOMPARE(mgr.memoryUsage(), std::size_t(8000));
    mgr.push(std::make_unique<HeavyAction>(10));
    QCOMPARE(mgr.memoryUsage(), std::size_t(4010));
    QVERIFY(!mgr.canRedo());
  }

  void testCompositeCostIsTheSumOfItsSteps() {
    auto composite = std::make_unique<CompositeAction>();
    composite->addAction(std::make_unique<HeavyAction>(1000));
    composite->addAction(std::make_unique<HeavyAction>(2000));
    QVERIFY(composite->memoryCost() >= 3000);
    UndoRedoManager mgr;
    mgr.push(std::move(composite));
    QVERIFY(mgr.memoryUsage() >= 3000);
  }
};

QTEST_MAIN(TestUndoRedoManager)
#include "test_undo_redo_manager.moc"
