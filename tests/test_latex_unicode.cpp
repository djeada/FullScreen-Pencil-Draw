/**
 * @file test_latex_unicode.cpp
 * @brief Tests for LaTeX-to-Unicode conversion in the text renderer.
 *
 * The functions under test are helpers inside latex_text_item.cpp.
 * We duplicate key logic here so it can be unit-tested in isolation.
 */
#include <QMap>
#include <QRegularExpression>
#include <QString>
#include <QTest>
#include <functional>

// ---- Duplicated symbol tables from latex_text_item.cpp ----

namespace LatexSymbols {
const QMap<QString, QString> greekLetters = {
    {"alpha", "α"},   {"beta", "β"},       {"gamma", "γ"},    {"delta", "δ"},
    {"epsilon", "ε"}, {"varepsilon", "ɛ"}, {"zeta", "ζ"},     {"eta", "η"},
    {"theta", "θ"},   {"vartheta", "ϑ"},   {"iota", "ι"},     {"kappa", "κ"},
    {"lambda", "λ"},  {"mu", "μ"},         {"nu", "ν"},       {"xi", "ξ"},
    {"omicron", "ο"}, {"pi", "π"},         {"varpi", "ϖ"},    {"rho", "ρ"},
    {"varrho", "ϱ"},  {"sigma", "σ"},      {"varsigma", "ς"}, {"tau", "τ"},
    {"upsilon", "υ"}, {"phi", "φ"},        {"varphi", "ϕ"},   {"chi", "χ"},
    {"psi", "ψ"},     {"omega", "ω"},      {"Gamma", "Γ"},    {"Delta", "Δ"},
    {"Theta", "Θ"},   {"Lambda", "Λ"},     {"Xi", "Ξ"},       {"Pi", "Π"},
    {"Sigma", "Σ"},   {"Phi", "Φ"},        {"Psi", "Ψ"},      {"Omega", "Ω"}};

const QMap<QString, QString> mathSymbols = {
    {"cdot", "·"},        {"times", "×"},
    {"div", "÷"},         {"pm", "±"},
    {"leq", "≤"},         {"le", "≤"},
    {"geq", "≥"},         {"ge", "≥"},
    {"neq", "≠"},         {"ne", "≠"},
    {"approx", "≈"},      {"equiv", "≡"},
    {"infty", "∞"},       {"partial", "∂"},
    {"nabla", "∇"},       {"sum", "∑"},
    {"prod", "∏"},        {"int", "∫"},
    {"rightarrow", "→"},  {"to", "→"},
    {"leftarrow", "←"},   {"gets", "←"},
    {"Rightarrow", "⇒"},  {"Leftarrow", "⇐"},
    {"in", "∈"},          {"notin", "∉"},
    {"subset", "⊂"},      {"subseteq", "⊆"},
    {"cup", "∪"},         {"cap", "∩"},
    {"emptyset", "∅"},    {"forall", "∀"},
    {"exists", "∃"},      {"lnot", "¬"},
    {"neg", "¬"},         {"land", "∧"},
    {"lor", "∨"},         {"sqrt", "√"},
    {"angle", "∠"},       {"degree", "°"},
    {"ldots", "…"},       {"cdots", "⋯"},
    {"left", ""},         {"right", ""},
    {"middle", ""},       {"big", ""},
    {"Big", ""},          {"bigg", ""},
    {"Bigg", ""},         {"langle", "⟨"},
    {"rangle", "⟩"},      {"lfloor", "⌊"},
    {"rfloor", "⌋"},      {"lceil", "⌈"},
    {"rceil", "⌉"},       {"textbf", ""},
    {"textit", ""},       {"textrm", ""},
    {"mathrm", ""},       {"mathbf", ""},
    {"mathit", ""},       {"mathcal", ""},
    {"mathbb", ""},       {"mathfrak", ""},
    {"quad", "  "},       {"qquad", "    "}};

const QMap<QChar, QString> superscripts = {
    {'0', "⁰"}, {'1', "¹"}, {'2', "²"}, {'3', "³"}, {'4', "⁴"},
    {'5', "⁵"}, {'6', "⁶"}, {'7', "⁷"}, {'8', "⁸"}, {'9', "⁹"},
    {'+', "⁺"}, {'-', "⁻"}, {'=', "⁼"}, {'(', "⁽"}, {')', "⁾"},
    {'a', "ᵃ"}, {'b', "ᵇ"}, {'n', "ⁿ"}, {'i', "ⁱ"}, {'x', "ˣ"}};

const QMap<QChar, QString> subscripts = {
    {'0', "₀"}, {'1', "₁"}, {'2', "₂"}, {'3', "₃"}, {'4', "₄"},
    {'5', "₅"}, {'6', "₆"}, {'7', "₇"}, {'8', "₈"}, {'9', "₉"},
    {'+', "₊"}, {'-', "₋"}, {'i', "ᵢ"}, {'n', "ₙ"}, {'x', "ₓ"}};

const QMap<QChar, QString> mathbb = {
    {'N', "ℕ"}, {'Z', "ℤ"}, {'Q', "ℚ"}, {'R', "ℝ"}, {'C', "ℂ"}};

const QMap<QChar, QString> mathcal = {
    {'L', "ℒ"}, {'F', "ℱ"}, {'H', "ℋ"}};

const QMap<QChar, QString> mathfrak = {
    {'A', "𝔄"}, {'B', "𝔅"}};

const QMap<QChar, QString> mathItalic = {
    {'a', "𝑎"}, {'b', "𝑏"}, {'x', "𝑥"}, {'y', "𝑦"}, {'z', "𝑧"},
    {'A', "𝐴"}, {'B', "𝐵"}, {'n', "𝑛"}, {'k', "𝑘"}};
} // namespace LatexSymbols

// ---- Duplicated conversion functions from latex_text_item.cpp ----

static QString latexCommandToUnicode(const QString &cmd) {
  if (LatexSymbols::greekLetters.contains(cmd))
    return LatexSymbols::greekLetters[cmd];
  if (LatexSymbols::mathSymbols.contains(cmd))
    return LatexSymbols::mathSymbols[cmd];
  return "\\" + cmd;
}

static QString latexToHtml(const QString &latex) {
  QString result = latex;

  auto processMatches =
      [](QString &str, const QRegularExpression &pattern,
         std::function<QString(const QRegularExpressionMatch &)> transform) {
        QList<QPair<qsizetype, QPair<qsizetype, QString>>> replacements;
        QRegularExpressionMatchIterator it = pattern.globalMatch(str);
        while (it.hasNext()) {
          QRegularExpressionMatch match = it.next();
          replacements.append({match.capturedStart(),
                               {match.capturedLength(), transform(match)}});
        }
        for (int i = replacements.size() - 1; i >= 0; --i) {
          qsizetype pos = replacements[i].first;
          qsizetype len = replacements[i].second.first;
          const QString &replacement = replacements[i].second.second;
          str.replace(pos, len, replacement);
        }
      };

  // Process \mathbb{X}
  static QRegularExpression mathbbPattern("\\\\mathbb\\{(\\w)\\}");
  processMatches(result, mathbbPattern, [](const QRegularExpressionMatch &m) {
    QChar ch = m.captured(1)[0];
    return LatexSymbols::mathbb.value(ch, m.captured(1));
  });

  // Process \mathcal{X}
  static QRegularExpression mathcalPattern("\\\\mathcal\\{(\\w)\\}");
  processMatches(result, mathcalPattern, [](const QRegularExpressionMatch &m) {
    QChar ch = m.captured(1)[0];
    return LatexSymbols::mathcal.value(ch, m.captured(1));
  });

  // Process \mathfrak{X}
  static QRegularExpression mathfrakPattern("\\\\mathfrak\\{(\\w)\\}");
  processMatches(result, mathfrakPattern, [](const QRegularExpressionMatch &m) {
    QChar ch = m.captured(1)[0];
    return LatexSymbols::mathfrak.value(ch, m.captured(1));
  });

  // Process \text{...}
  static QRegularExpression textPattern("\\\\text\\{([^}]*)\\}");
  processMatches(result, textPattern, [](const QRegularExpressionMatch &m) {
    return m.captured(1);
  });

  // Process \textbf{...}
  static QRegularExpression textbfPattern("\\\\textbf\\{([^}]*)\\}");
  processMatches(result, textbfPattern, [](const QRegularExpressionMatch &m) {
    return "<b>" + m.captured(1) + "</b>";
  });

  // Process \textit{...}
  static QRegularExpression textitPattern("\\\\textit\\{([^}]*)\\}");
  processMatches(result, textitPattern, [](const QRegularExpressionMatch &m) {
    return "<i>" + m.captured(1) + "</i>";
  });

  // Process \mathrm{...}
  static QRegularExpression mathrmPattern("\\\\mathrm\\{([^}]*)\\}");
  processMatches(result, mathrmPattern, [](const QRegularExpressionMatch &m) {
    return m.captured(1);
  });

  // Process \mathbf{...}
  static QRegularExpression mathbfPattern("\\\\mathbf\\{([^}]*)\\}");
  processMatches(result, mathbfPattern, [](const QRegularExpressionMatch &m) {
    return "<b>" + m.captured(1) + "</b>";
  });

  // Process \mathit{...}
  static QRegularExpression mathitPattern("\\\\mathit\\{([^}]*)\\}");
  processMatches(result, mathitPattern, [](const QRegularExpressionMatch &m) {
    return "<i>" + m.captured(1) + "</i>";
  });

  // Process \textrm{...}
  static QRegularExpression textrmPattern("\\\\textrm\\{([^}]*)\\}");
  processMatches(result, textrmPattern, [](const QRegularExpressionMatch &m) {
    return m.captured(1);
  });

  // Process \binom{n}{k}
  static QRegularExpression binomPattern("\\\\binom\\{([^}]*)\\}\\{([^}]*)\\}");
  processMatches(result, binomPattern, [](const QRegularExpressionMatch &m) {
    return "(" + m.captured(1) + " choose " + m.captured(2) + ")";
  });

  // Accent commands
  static QRegularExpression hatPattern("\\\\hat\\{([^}]*)\\}");
  processMatches(result, hatPattern, [](const QRegularExpressionMatch &m) {
    return m.captured(1) + QString(QChar(0x0302));
  });

  static QRegularExpression barPattern("\\\\bar\\{([^}]*)\\}");
  processMatches(result, barPattern, [](const QRegularExpressionMatch &m) {
    return m.captured(1) + QString(QChar(0x0304));
  });

  static QRegularExpression vecPattern("\\\\vec\\{([^}]*)\\}");
  processMatches(result, vecPattern, [](const QRegularExpressionMatch &m) {
    return m.captured(1) + QString(QChar(0x20D7));
  });

  static QRegularExpression dotPattern("\\\\dot\\{([^}]*)\\}");
  processMatches(result, dotPattern, [](const QRegularExpressionMatch &m) {
    return m.captured(1) + QString(QChar(0x0307));
  });

  static QRegularExpression ddotPattern("\\\\ddot\\{([^}]*)\\}");
  processMatches(result, ddotPattern, [](const QRegularExpressionMatch &m) {
    return m.captured(1) + QString(QChar(0x0308));
  });

  static QRegularExpression tildePattern("\\\\tilde\\{([^}]*)\\}");
  processMatches(result, tildePattern, [](const QRegularExpressionMatch &m) {
    return m.captured(1) + QString(QChar(0x0303));
  });

  static QRegularExpression overlinePattern("\\\\overline\\{([^}]*)\\}");
  processMatches(
      result, overlinePattern, [](const QRegularExpressionMatch &m) {
        QString content = m.captured(1);
        QString result;
        for (QChar ch : content) {
          result += ch;
          result += QChar(0x0305);
        }
        return result;
      });

  static QRegularExpression underlinePattern("\\\\underline\\{([^}]*)\\}");
  processMatches(
      result, underlinePattern, [](const QRegularExpressionMatch &m) {
        QString content = m.captured(1);
        QString result;
        for (QChar ch : content) {
          result += ch;
          result += QChar(0x0332);
        }
        return result;
      });

  // Process \frac{a}{b}
  static QRegularExpression fracPattern("\\\\frac\\{([^}]*)\\}\\{([^}]*)\\}");
  processMatches(result, fracPattern, [](const QRegularExpressionMatch &m) {
    QString num = m.captured(1);
    QString den = m.captured(2);
    QString superNum;
    for (QChar ch : num) {
      superNum += LatexSymbols::superscripts.value(ch, QString(ch));
    }
    QString subDen;
    for (QChar ch : den) {
      subDen += LatexSymbols::subscripts.value(ch, QString(ch));
    }
    return superNum + QString("⁄") + subDen;
  });

  // Process ^{...}
  static QRegularExpression supBracePattern("\\^\\{([^}]*)\\}");
  processMatches(result, supBracePattern, [](const QRegularExpressionMatch &m) {
    QString content = m.captured(1);
    QString superscript;
    for (QChar ch : content) {
      superscript += LatexSymbols::superscripts.value(ch, QString(ch));
    }
    return superscript;
  });

  // Process ^x
  static QRegularExpression supPattern("\\^(\\w)");
  processMatches(result, supPattern, [](const QRegularExpressionMatch &m) {
    QString ch = m.captured(1);
    if (ch.length() == 1 && LatexSymbols::superscripts.contains(ch[0]))
      return LatexSymbols::superscripts[ch[0]];
    return ch;
  });

  // Process _{...}
  static QRegularExpression subBracePattern("_\\{([^}]*)\\}");
  processMatches(result, subBracePattern, [](const QRegularExpressionMatch &m) {
    QString content = m.captured(1);
    QString subscript;
    for (QChar ch : content) {
      subscript += LatexSymbols::subscripts.value(ch, QString(ch));
    }
    return subscript;
  });

  // Process _x
  static QRegularExpression subPattern("_(\\w)");
  processMatches(result, subPattern, [](const QRegularExpressionMatch &m) {
    QString ch = m.captured(1);
    if (ch.length() == 1 && LatexSymbols::subscripts.contains(ch[0]))
      return LatexSymbols::subscripts[ch[0]];
    return ch;
  });

  // Process \sqrt{...}
  static QRegularExpression sqrtPattern("\\\\sqrt\\{([^}]*)\\}");
  result.replace(sqrtPattern, "√\\1");

  // Process \sqrt[n]{...}
  static QRegularExpression nthRootPattern("\\\\sqrt\\[(\\d+)\\]\\{([^}]*)\\}");
  processMatches(result, nthRootPattern, [](const QRegularExpressionMatch &m) {
    QString n = m.captured(1);
    QString content = m.captured(2);
    QString superN;
    for (QChar ch : n) {
      superN += LatexSymbols::superscripts.value(ch, QString(ch));
    }
    return superN + "√" + content;
  });

  // Process \sqrt followed by a single character
  static QRegularExpression sqrtSimplePattern("\\\\sqrt(\\w)");
  result.replace(sqrtSimplePattern, "√\\1");

  // Replace remaining \commands with Unicode symbols
  static QRegularExpression cmdPattern("\\\\(\\w+)");
  processMatches(result, cmdPattern, [](const QRegularExpressionMatch &m) {
    return latexCommandToUnicode(m.captured(1));
  });

  return result;
}

// ---- Test class ----

class TestLatexUnicode : public QObject {
  Q_OBJECT
private slots:
  void testGreekLetters();
  void testCommonAliases();
  void testSuperscripts();
  void testSubscripts();
  void testFractions();
  void testSqrt();
  void testMathbb();
  void testMathcal();
  void testTextCommand();
  void testTextFormatting();
  void testBinom();
  void testAccents();
  void testOverlineUnderline();
  void testDelimiterSizing();
  void testCommandFallback();
};

void TestLatexUnicode::testGreekLetters() {
  QString result = latexToHtml("\\alpha + \\beta");
  QVERIFY(result.contains("α"));
  QVERIFY(result.contains("β"));
}

void TestLatexUnicode::testCommonAliases() {
  // \to should produce →
  QString toResult = latexToHtml("\\to");
  QVERIFY(toResult.contains("→"));

  // \gets should produce ←
  QString getsResult = latexToHtml("\\gets");
  QVERIFY(getsResult.contains("←"));

  // \le and \ge should produce ≤ and ≥
  QString leResult = latexToHtml("\\le");
  QVERIFY(leResult.contains("≤"));

  QString geResult = latexToHtml("\\ge");
  QVERIFY(geResult.contains("≥"));

  // \ne should produce ≠
  QString neResult = latexToHtml("\\ne");
  QVERIFY(neResult.contains("≠"));
}

void TestLatexUnicode::testSuperscripts() {
  // Simple superscript
  QString result = latexToHtml("x^2");
  QVERIFY(result.contains("²"));

  // Braced superscript
  QString resultBrace = latexToHtml("x^{23}");
  QVERIFY(resultBrace.contains("²"));
  QVERIFY(resultBrace.contains("³"));
}

void TestLatexUnicode::testSubscripts() {
  // Simple subscript
  QString result = latexToHtml("x_0");
  QVERIFY(result.contains("₀"));

  // Braced subscript
  QString resultBrace = latexToHtml("x_{12}");
  QVERIFY(resultBrace.contains("₁"));
  QVERIFY(resultBrace.contains("₂"));
}

void TestLatexUnicode::testFractions() {
  QString result = latexToHtml("\\frac{1}{2}");
  // Should contain fraction slash ⁄
  QVERIFY(result.contains("⁄"));
  // Should contain superscript 1 and subscript 2
  QVERIFY(result.contains("¹"));
  QVERIFY(result.contains("₂"));
}

void TestLatexUnicode::testSqrt() {
  QString result = latexToHtml("\\sqrt{x}");
  QVERIFY(result.contains("√"));
}

void TestLatexUnicode::testMathbb() {
  QString result = latexToHtml("\\mathbb{R}");
  QCOMPARE(result, QString("ℝ"));
}

void TestLatexUnicode::testMathcal() {
  QString result = latexToHtml("\\mathcal{L}");
  QCOMPARE(result, QString("ℒ"));
}

void TestLatexUnicode::testTextCommand() {
  // \text{hello} should produce plain "hello"
  QString result = latexToHtml("\\text{hello}");
  QVERIFY(result.contains("hello"));
  QVERIFY(!result.contains("\\text"));
}

void TestLatexUnicode::testTextFormatting() {
  // \textbf{bold} should produce <b>bold</b>
  QString bf = latexToHtml("\\textbf{bold}");
  QVERIFY(bf.contains("<b>bold</b>"));

  // \textit{italic} should produce <i>italic</i>
  QString it = latexToHtml("\\textit{italic}");
  QVERIFY(it.contains("<i>italic</i>"));

  // \mathrm{dx} should produce plain "dx"
  QString rm = latexToHtml("\\mathrm{dx}");
  QVERIFY(rm.contains("dx"));
  QVERIFY(!rm.contains("\\mathrm"));

  // \mathbf{F} should produce <b>F</b>
  QString mbf = latexToHtml("\\mathbf{F}");
  QVERIFY(mbf.contains("<b>F</b>"));
}

void TestLatexUnicode::testBinom() {
  QString result = latexToHtml("\\binom{n}{k}");
  QVERIFY(result.contains("choose"));
  QVERIFY(result.contains("("));
  QVERIFY(result.contains(")"));
}

void TestLatexUnicode::testAccents() {
  // \hat{x} should contain combining circumflex (U+0302)
  QString hat = latexToHtml("\\hat{x}");
  QVERIFY(hat.contains(QChar(0x0302)));

  // \bar{x} should contain combining macron (U+0304)
  QString bar = latexToHtml("\\bar{x}");
  QVERIFY(bar.contains(QChar(0x0304)));

  // \vec{v} should contain combining right arrow above (U+20D7)
  QString vec = latexToHtml("\\vec{v}");
  QVERIFY(vec.contains(QChar(0x20D7)));

  // \dot{x} should contain combining dot above (U+0307)
  QString dot = latexToHtml("\\dot{x}");
  QVERIFY(dot.contains(QChar(0x0307)));

  // \tilde{x} should contain combining tilde (U+0303)
  QString tilde = latexToHtml("\\tilde{x}");
  QVERIFY(tilde.contains(QChar(0x0303)));
}

void TestLatexUnicode::testOverlineUnderline() {
  // \overline{AB} should contain combining overline (U+0305) for each char
  QString ol = latexToHtml("\\overline{AB}");
  QVERIFY(ol.contains(QChar(0x0305)));
  QVERIFY(ol.contains("A"));
  QVERIFY(ol.contains("B"));

  // \underline{xy} should contain combining low line (U+0332)
  QString ul = latexToHtml("\\underline{xy}");
  QVERIFY(ul.contains(QChar(0x0332)));
}

void TestLatexUnicode::testDelimiterSizing() {
  // \left and \right should be stripped (produce empty string)
  QString result = latexToHtml("\\left( x \\right)");
  QVERIFY(result.contains("("));
  QVERIFY(result.contains(")"));
  QVERIFY(!result.contains("\\left"));
  QVERIFY(!result.contains("\\right"));
}

void TestLatexUnicode::testCommandFallback() {
  // Unknown commands should keep the backslash prefix
  QString result = latexToHtml("\\unknowncmd");
  QCOMPARE(result, QString("\\unknowncmd"));
}

QTEST_MAIN(TestLatexUnicode)
#include "test_latex_unicode.moc"
