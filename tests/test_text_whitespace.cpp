/**
 * @file test_text_whitespace.cpp
 * @brief Tests for whitespace preservation in plainTextToHtmlPreservingNewlines.
 *
 * The function under test is a static helper inside latex_text_item.cpp.
 * We duplicate its logic here so it can be unit-tested in isolation.
 */
#include <QTest>
#include <QString>

// Mirror of the static function in latex_text_item.cpp
static QString plainTextToHtmlPreservingNewlines(QString text) {
  text.replace("\r\n", "\n");
  text.replace('\r', '\n');

  QString html = text.toHtmlEscaped();
  html.replace('\n', "<br/>");
  // Preserve tab characters as four non-breaking spaces
  html.replace('\t', "&nbsp;&nbsp;&nbsp;&nbsp;");
  // Preserve runs of multiple spaces: replace each pair of consecutive spaces
  // with a space followed by a non-breaking space so the browser/QTextDocument
  // does not collapse them.
  QString prev;
  do {
    prev = html;
    html.replace("  ", " &nbsp;");
  } while (html != prev);
  return html;
}

class TestTextWhitespace : public QObject {
  Q_OBJECT
private slots:
  void testSingleSpace();
  void testMultipleSpaces();
  void testTab();
  void testMultipleTabs();
  void testNewlines();
  void testMixedWhitespace();
  void testNoWhitespace();
};

void TestTextWhitespace::testSingleSpace() {
  QString result = plainTextToHtmlPreservingNewlines("a b");
  // A single space should remain unchanged
  QCOMPARE(result, QString("a b"));
}

void TestTextWhitespace::testMultipleSpaces() {
  QString result = plainTextToHtmlPreservingNewlines("a    b");
  // Four spaces: should be preserved using &nbsp; entities
  QVERIFY(result.contains("&nbsp;"));
  // The result should not collapse multiple spaces
  QVERIFY(result != "a b");
}

void TestTextWhitespace::testTab() {
  QString result = plainTextToHtmlPreservingNewlines("a\tb");
  // Tab should be converted to four &nbsp; entities
  QVERIFY(result.contains("&nbsp;"));
  QCOMPARE(result, QString("a&nbsp;&nbsp;&nbsp;&nbsp;b"));
}

void TestTextWhitespace::testMultipleTabs() {
  QString result = plainTextToHtmlPreservingNewlines("a\t\tb");
  // Two tabs = eight &nbsp; entities
  QCOMPARE(result, QString("a&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;b"));
}

void TestTextWhitespace::testNewlines() {
  QString result = plainTextToHtmlPreservingNewlines("line1\nline2");
  QCOMPARE(result, QString("line1<br/>line2"));
}

void TestTextWhitespace::testMixedWhitespace() {
  QString result = plainTextToHtmlPreservingNewlines("a\tb\n  c");
  // Tab becomes 4 &nbsp;, newline becomes <br/>, two spaces become " &nbsp;"
  QVERIFY(result.contains("&nbsp;&nbsp;&nbsp;&nbsp;"));
  QVERIFY(result.contains("<br/>"));
  QVERIFY(result.contains("&nbsp;"));
}

void TestTextWhitespace::testNoWhitespace() {
  QString result = plainTextToHtmlPreservingNewlines("hello");
  QCOMPARE(result, QString("hello"));
}

QTEST_MAIN(TestTextWhitespace)
#include "test_text_whitespace.moc"
