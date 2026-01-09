/**
 * @file latex_text_item.cpp
 * @brief Implementation of LaTeX-enabled text graphics item.
 */
#include "latex_text_item.h"
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QInputDialog>
#include <QPainter>
#include <QRegularExpression>
#include <QStyleOptionGraphicsItem>
#include <QTextDocument>
#include <cmath>

// Unicode math symbols for LaTeX rendering
namespace LatexSymbols {
// Greek letters
const QMap<QString, QString> greekLetters = {
    {"alpha", "α"},     {"beta", "β"},    {"gamma", "γ"},
    {"delta", "δ"},     {"epsilon", "ε"}, {"zeta", "ζ"},
    {"eta", "η"},       {"theta", "θ"},   {"iota", "ι"},
    {"kappa", "κ"},     {"lambda", "λ"},  {"mu", "μ"},
    {"nu", "ν"},        {"xi", "ξ"},      {"omicron", "ο"},
    {"pi", "π"},        {"rho", "ρ"},     {"sigma", "σ"},
    {"tau", "τ"},       {"upsilon", "υ"}, {"phi", "φ"},
    {"chi", "χ"},       {"psi", "ψ"},     {"omega", "ω"},
    {"Alpha", "Α"},     {"Beta", "Β"},    {"Gamma", "Γ"},
    {"Delta", "Δ"},     {"Epsilon", "Ε"}, {"Zeta", "Ζ"},
    {"Eta", "Η"},       {"Theta", "Θ"},   {"Iota", "Ι"},
    {"Kappa", "Κ"},     {"Lambda", "Λ"},  {"Mu", "Μ"},
    {"Nu", "Ν"},        {"Xi", "Ξ"},      {"Omicron", "Ο"},
    {"Pi", "Π"},        {"Rho", "Ρ"},     {"Sigma", "Σ"},
    {"Tau", "Τ"},       {"Upsilon", "Υ"}, {"Phi", "Φ"},
    {"Chi", "Χ"},       {"Psi", "Ψ"},     {"Omega", "Ω"}};

// Math operators and symbols
const QMap<QString, QString> mathSymbols = {
    {"cdot", "·"},     {"times", "×"},   {"div", "÷"},
    {"pm", "±"},       {"mp", "∓"},      {"leq", "≤"},
    {"geq", "≥"},      {"neq", "≠"},     {"approx", "≈"},
    {"equiv", "≡"},    {"sim", "∼"},     {"propto", "∝"},
    {"infty", "∞"},    {"partial", "∂"}, {"nabla", "∇"},
    {"sum", "∑"},      {"prod", "∏"},    {"int", "∫"},
    {"oint", "∮"},     {"sqrt", "√"},    {"forall", "∀"},
    {"exists", "∃"},   {"nexists", "∄"}, {"in", "∈"},
    {"notin", "∉"},    {"subset", "⊂"},  {"supset", "⊃"},
    {"subseteq", "⊆"}, {"supseteq", "⊇"},{"cup", "∪"},
    {"cap", "∩"},      {"emptyset", "∅"},{"therefore", "∴"},
    {"because", "∵"},  {"land", "∧"},    {"lor", "∨"},
    {"neg", "¬"},      {"Rightarrow", "⇒"},{"Leftarrow", "⇐"},
    {"Leftrightarrow", "⇔"},{"rightarrow", "→"},{"leftarrow", "←"},
    {"leftrightarrow", "↔"},{"uparrow", "↑"},{"downarrow", "↓"},
    {"angle", "∠"},    {"triangle", "△"},{"degree", "°"},
    {"circ", "∘"},     {"bullet", "•"},  {"star", "★"},
    {"ldots", "…"},    {"cdots", "⋯"},   {"vdots", "⋮"},
    {"ddots", "⋱"},    {"prime", "′"},   {"hbar", "ℏ"},
    {"Re", "ℜ"},       {"Im", "ℑ"},      {"wp", "℘"},
    {"aleph", "ℵ"},    {"ell", "ℓ"}};

// Superscript digits for exponents
const QMap<QChar, QString> superscripts = {
    {'0', "⁰"}, {'1', "¹"}, {'2', "²"}, {'3', "³"}, {'4', "⁴"},
    {'5', "⁵"}, {'6', "⁶"}, {'7', "⁷"}, {'8', "⁸"}, {'9', "⁹"},
    {'+', "⁺"}, {'-', "⁻"}, {'=', "⁼"}, {'(', "⁽"}, {')', "⁾"},
    {'n', "ⁿ"}, {'i', "ⁱ"}};

// Subscript digits
const QMap<QChar, QString> subscripts = {
    {'0', "₀"}, {'1', "₁"}, {'2', "₂"}, {'3', "₃"}, {'4', "₄"},
    {'5', "₅"}, {'6', "₆"}, {'7', "₇"}, {'8', "₈"}, {'9', "₉"},
    {'+', "₊"}, {'-', "₋"}, {'=', "₌"}, {'(', "₍"}, {')', "₎"},
    {'a', "ₐ"}, {'e', "ₑ"}, {'o', "ₒ"}, {'x', "ₓ"}, {'h', "ₕ"},
    {'k', "ₖ"}, {'l', "ₗ"}, {'m', "ₘ"}, {'n', "ₙ"}, {'p', "ₚ"},
    {'s', "ₛ"}, {'t', "ₜ"}};
} // namespace LatexSymbols

// LatexTextItem implementation
LatexTextItem::LatexTextItem(QGraphicsItem *parent)
    : QGraphicsObject(parent), textColor_(Qt::white), font_("Arial", 12) {
  setFlags(ItemIsSelectable | ItemIsMovable);
  setAcceptHoverEvents(true);

  // Initialize with empty content rectangle
  contentRect_ = QRectF(0, 0, MIN_WIDTH, MIN_HEIGHT);
}

LatexTextItem::~LatexTextItem() = default;

QRectF LatexTextItem::boundingRect() const {
  return contentRect_.adjusted(-PADDING, -PADDING, PADDING, PADDING);
}

void LatexTextItem::paint(QPainter *painter,
                          const QStyleOptionGraphicsItem *option,
                          QWidget * /*widget*/) {
  // Draw the rendered content
  if (!renderedContent_.isNull()) {
    painter->drawPixmap(PADDING, PADDING, renderedContent_);
  } else if (!text_.isEmpty()) {
    // Fallback: draw plain text if rendering failed
    painter->setFont(font_);
    painter->setPen(textColor_);
    painter->drawText(contentRect_, Qt::AlignLeft | Qt::AlignTop, text_);
  }

  // Draw selection highlight
  if (option->state & QStyle::State_Selected) {
    painter->setPen(QPen(Qt::blue, 1, Qt::DashLine));
    painter->drawRect(boundingRect());
  }
}

void LatexTextItem::setText(const QString &text) {
  text_ = text;
  renderContent();
  update();
  emit textChanged();
}

void LatexTextItem::setTextColor(const QColor &color) {
  textColor_ = color;
  renderContent();
  update();
}

void LatexTextItem::setFont(const QFont &font) {
  font_ = font;
  renderContent();
  update();
}

void LatexTextItem::startEditing() {
  // Get the view widget for the dialog parent
  QWidget *parentWidget = nullptr;
  if (scene() && !scene()->views().isEmpty()) {
    parentWidget = scene()->views().first();
  }

  bool ok;
  QString newText = QInputDialog::getText(
      parentWidget, "Edit Text",
      "Enter text (use $...$ for LaTeX math):",
      QLineEdit::Normal, text_, &ok);

  if (ok) {
    if (newText.isEmpty()) {
      // Don't update if empty - caller should handle removal
      text_ = newText;
    } else {
      text_ = newText;
      renderContent();
      update();
    }
    emit editingFinished();
    emit textChanged();
  }
}

bool LatexTextItem::hasLatex() const {
  static QRegularExpression latexPattern("\\$[^$]+\\$");
  return text_.contains(latexPattern);
}

void LatexTextItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    startEditing();
    event->accept();
  } else {
    QGraphicsObject::mouseDoubleClickEvent(event);
  }
}

void LatexTextItem::renderContent() {
  if (text_.isEmpty()) {
    renderedContent_ = QPixmap();
    contentRect_ = QRectF(0, 0, MIN_WIDTH, MIN_HEIGHT);
    return;
  }

  renderedContent_ = renderLatex(text_);
  contentRect_ =
      QRectF(0, 0, renderedContent_.width() + PADDING * 2,
             renderedContent_.height() + PADDING * 2);
}

QPixmap LatexTextItem::renderLatex(const QString &text) {
  // Parse the text and convert LaTeX expressions
  QString htmlContent;
  static QRegularExpression latexPattern("\\$([^$]+)\\$");
  QRegularExpressionMatchIterator it = latexPattern.globalMatch(text);

  int lastEnd = 0;
  while (it.hasNext()) {
    QRegularExpressionMatch match = it.next();
    // Add plain text before the match
    if (match.capturedStart() > lastEnd) {
      QString plainPart = text.mid(lastEnd, match.capturedStart() - lastEnd);
      htmlContent += plainPart.toHtmlEscaped();
    }
    // Convert LaTeX to HTML
    QString latex = match.captured(1);
    htmlContent += latexToHtml(latex);
    lastEnd = match.capturedEnd();
  }

  // Add remaining plain text
  if (lastEnd < text.length()) {
    htmlContent += text.mid(lastEnd).toHtmlEscaped();
  }

  // If no LaTeX was found, just use plain text
  if (!text.contains('$')) {
    htmlContent = text.toHtmlEscaped();
  }

  // Render the HTML content using QTextDocument
  QTextDocument doc;
  doc.setDefaultFont(font_);
  doc.setHtml(htmlContent);
  doc.setTextWidth(-1); // No word wrap

  // Create the pixmap
  QSizeF size = doc.size();
  QPixmap pixmap(qMax(static_cast<int>(size.width()), MIN_WIDTH),
                 qMax(static_cast<int>(size.height()), MIN_HEIGHT));
  pixmap.fill(Qt::transparent);

  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::TextAntialiasing);

  // Set text color
  QAbstractTextDocumentLayout::PaintContext ctx;
  ctx.palette.setColor(QPalette::Text, textColor_);
  doc.documentLayout()->draw(&painter, ctx);

  return pixmap;
}

QString LatexTextItem::latexToHtml(const QString &latex) {
  QString result = latex;

  // Process fractions: \frac{a}{b}
  static QRegularExpression fracPattern("\\\\frac\\{([^}]*)\\}\\{([^}]*)\\}");
  result.replace(fracPattern, "<sup>\\1</sup>/<sub>\\2</sub>");

  // Process superscripts: ^{...} or ^x
  static QRegularExpression supBracePattern("\\^\\{([^}]*)\\}");
  result.replace(supBracePattern, "<sup>\\1</sup>");

  static QRegularExpression supPattern("\\^(\\w)");
  QRegularExpressionMatchIterator supIt = supPattern.globalMatch(result);
  while (supIt.hasNext()) {
    QRegularExpressionMatch match = supIt.next();
    QString ch = match.captured(1);
    QString superscript;
    if (ch.length() == 1 &&
        LatexSymbols::superscripts.contains(ch[0])) {
      superscript = LatexSymbols::superscripts[ch[0]];
    } else {
      superscript = "<sup>" + ch + "</sup>";
    }
    result.replace(match.captured(0), superscript);
    supIt = supPattern.globalMatch(result); // Re-scan after replacement
  }

  // Process subscripts: _{...} or _x
  static QRegularExpression subBracePattern("_\\{([^}]*)\\}");
  result.replace(subBracePattern, "<sub>\\1</sub>");

  static QRegularExpression subPattern("_(\\w)");
  QRegularExpressionMatchIterator subIt = subPattern.globalMatch(result);
  while (subIt.hasNext()) {
    QRegularExpressionMatch match = subIt.next();
    QString ch = match.captured(1);
    QString subscript;
    if (ch.length() == 1 &&
        LatexSymbols::subscripts.contains(ch[0])) {
      subscript = LatexSymbols::subscripts[ch[0]];
    } else {
      subscript = "<sub>" + ch + "</sub>";
    }
    result.replace(match.captured(0), subscript);
    subIt = subPattern.globalMatch(result); // Re-scan after replacement
  }

  // Process square root: \sqrt{...}
  static QRegularExpression sqrtPattern("\\\\sqrt\\{([^}]*)\\}");
  result.replace(sqrtPattern, "√(\\1)");

  // Process simple \sqrt followed by a single character
  static QRegularExpression sqrtSimplePattern("\\\\sqrt(\\w)");
  result.replace(sqrtSimplePattern, "√\\1");

  // Replace LaTeX commands with Unicode symbols
  static QRegularExpression cmdPattern("\\\\(\\w+)");
  QRegularExpressionMatchIterator cmdIt = cmdPattern.globalMatch(result);
  QMap<int, QPair<QString, QString>> replacements;

  while (cmdIt.hasNext()) {
    QRegularExpressionMatch match = cmdIt.next();
    QString cmd = match.captured(1);
    QString unicode = latexCommandToUnicode(cmd);
    replacements[match.capturedStart()] =
        qMakePair(match.captured(0), unicode);
  }

  // Apply replacements in reverse order to preserve positions
  QList<int> positions = replacements.keys();
  std::sort(positions.begin(), positions.end(), std::greater<int>());
  for (int pos : positions) {
    auto pair = replacements[pos];
    result.replace(pos, pair.first.length(), pair.second);
  }

  return result;
}

QString LatexTextItem::latexCommandToUnicode(const QString &cmd) {
  // Check Greek letters
  if (LatexSymbols::greekLetters.contains(cmd)) {
    return LatexSymbols::greekLetters[cmd];
  }

  // Check math symbols
  if (LatexSymbols::mathSymbols.contains(cmd)) {
    return LatexSymbols::mathSymbols[cmd];
  }

  // Return the original command with backslash if not found
  return "\\" + cmd;
}
