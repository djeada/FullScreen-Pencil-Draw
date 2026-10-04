#ifndef LATEX_SOURCE_H
#define LATEX_SOURCE_H

#include <QRegularExpression>
#include <QStringList>

namespace LatexSource {
inline const QRegularExpression &mathPattern() {
  static const QRegularExpression pattern(
      "\\$\\$(.+?)\\$\\$|\\$([^$]+)\\$",
      QRegularExpression::DotMatchesEverythingOption);
  return pattern;
}

// Escape a plain-text run for use inside KaTeX's \text{...}.
inline QString escapeText(const QString &plain) {
  QString out;
  out.reserve(plain.size());
  for (QChar c : plain) {
    switch (c.unicode()) {
    case '\\':
      out += QStringLiteral("\\textbackslash{}");
      break;
    case '{':
    case '}':
    case '#':
    case '%':
    case '&':
    case '_':
    case '$':
      out += QLatin1Char('\\');
      out += c;
      break;
    case '^':
      out += QStringLiteral("\\textasciicircum{}");
      break;
    case '~':
      out += QStringLiteral("\\textasciitilde{}");
      break;
    default:
      out += c;
    }
  }
  return out;
}

// Match whole math blocks before splitting plain text into layout rows.
// Newlines inside environments such as cases must stay in the math source.
inline QString composeKatexSource(const QString &text) {
  QStringList rows{QString()};
  const auto appendPlain = [&rows](const QString &plain) {
    const QStringList lines = plain.split(QLatin1Char('\n'));
    for (qsizetype i = 0; i < lines.size(); ++i) {
      if (i > 0)
        rows.append(QString());
      if (!lines[i].isEmpty())
        rows.last() += QStringLiteral("\\text{") + escapeText(lines[i]) +
                       QLatin1Char('}');
    }
  };

  qsizetype pos = 0;
  auto it = mathPattern().globalMatch(text);
  while (it.hasNext()) {
    const QRegularExpressionMatch m = it.next();
    if (m.capturedStart() > pos)
      appendPlain(text.mid(pos, m.capturedStart() - pos));
    if (m.captured(1).isEmpty())
      rows.last() += QLatin1Char('{') + m.captured(2) + QLatin1Char('}');
    else
      rows.last() += QStringLiteral("{\\displaystyle ") + m.captured(1) +
                     QLatin1Char('}');
    pos = m.capturedEnd();
  }
  if (pos < text.size())
    appendPlain(text.mid(pos));
  if (rows.size() == 1)
    return rows.first();
  return QStringLiteral("\\begin{array}{l}") +
         rows.join(QStringLiteral(" \\\\ ")) + QStringLiteral("\\end{array}");
}
} // namespace LatexSource

#endif // LATEX_SOURCE_H
