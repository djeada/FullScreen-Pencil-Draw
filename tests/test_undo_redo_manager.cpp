/**
 * @file test_undo_redo_manager.cpp
 * @brief Tests for UndoRedoManager, including the max-30-action limit
 *        on the undo stack.
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

  // Pushing more than kMaxUndoSteps drops the oldest actions.
  void testPushEnforcesLimit() {
    UndoRedoManager mgr;
    constexpr std::size_t limit = UndoRedoManager::kMaxUndoSteps;

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
    constexpr std::size_t limit = UndoRedoManager::kMaxUndoSteps;

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

  // The constant itself must be 30.
  void testMaxIs30() {
    QCOMPARE(UndoRedoManager::kMaxUndoSteps, std::size_t(30));
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
    constexpr std::size_t limit = UndoRedoManager::kMaxUndoSteps;

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
    for (std::size_t i = 0; i < UndoRedoManager::kMaxUndoSteps; ++i) {
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
};

QTEST_MAIN(TestUndoRedoManager)
#include "test_undo_redo_manager.moc"
