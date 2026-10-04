#include "../src/widgets/latex_source.h"
#include <QTest>

class TestLatexSource : public QObject {
  Q_OBJECT
private slots:
  void multilineDisplayMath() {
    const QString math = QStringLiteral(
        "\n\\begin{cases}\n"
        "0 & x\\leq 0 \\\\\n"
        "\\frac{100-x}{100} & 0\\leq x\\leq 100 \\\\\n"
        "0 & 100\\leq x\n\\end{cases}\n");
    QCOMPARE(LatexSource::composeKatexSource("$$" + math + "$$"),
             "{\\displaystyle " + math + "}");
  }

  void simpleDisplayMath() {
    const QString math = "\nf(x,y) \\leftarrow \\text{mapping}\n";
    QCOMPARE(LatexSource::composeKatexSource("$$" + math + "$$"),
             "{\\displaystyle " + math + "}");
  }

  void multipleBlocksAndPlainText() {
    QCOMPARE(LatexSource::composeKatexSource(
                 "$$\nx^2\n$$\np[i,j] = f(i,j)\n\n$$\n\\hat{f}(x,y)\n$$"),
             QString("\\begin{array}{l}{\\displaystyle \nx^2\n} \\\\ "
                     "\\text{p[i,j] = f(i,j)} \\\\  \\\\ "
                     "{\\displaystyle \n\\hat{f}(x,y)\n}\\end{array}"));
  }

  void inlineAndSingleLineDisplayMath() {
    QCOMPARE(LatexSource::composeKatexSource("a $x^2$ and $$y^2$$"),
             QString("\\text{a }{x^2}\\text{ and }{\\displaystyle y^2}"));
  }

  void plainTextEscapingAndRows() {
    QCOMPARE(LatexSource::composeKatexSource("a & {b}\n$x$\n"),
             QString("\\begin{array}{l}\\text{a \\& \\{b\\}} \\\\ {x}"
                     " \\\\ \\end{array}"));
  }

  void displayDelimitersAreFullyConsumed() {
    const auto match = LatexSource::mathPattern().match("$$\nx\n$$");
    QVERIFY(match.hasMatch());
    QCOMPARE(match.captured(), QString("$$\nx\n$$"));
    QCOMPARE(match.captured(1), QString("\nx\n"));
    QVERIFY(!LatexSource::mathPattern().match("plain text").hasMatch());
  }
};

QTEST_APPLESS_MAIN(TestLatexSource)
#include "test_latex_source.moc"
