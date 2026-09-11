#include "FunctionXmlFormat.hpp"

#include <QDomDocument>
#include <QXmlStreamWriter>

namespace {

bool isXmlWhitespace(const QString& text)
{
    for (QChar c : text)
        if (c != ' ' && c != '\n' && c != '\r' && c != '\t') return false;
    return true;
}

void writeText(QXmlStreamWriter& writer, const QString& text, bool compact)
{
    // Keep actual whitespace inside parameter values without putting literal
    // line breaks or tabs into the one-line entry that receives the result.
    // CR and NBSP also need references in the preview: XML and QTextEdit
    // otherwise normalize them to LF and a regular space, respectively.
    qsizetype start = 0;
    for (qsizetype i = 0; i < text.size(); ++i) {
        const QChar c = text[i];
        if (c != '\r' && c != QChar(0x00a0)
            && !(compact && (c == '\n' || c == '\t')))
            continue;
        writer.writeCharacters(text.mid(start, i - start));
        writer.writeEntityReference(QStringLiteral("#%1").arg(static_cast<uint>(c.unicode())));
        start = i + 1;
    }
    writer.writeCharacters(text.mid(start));
}

QString annotationText(QString text, bool compact)
{
    // Character references are literal in comments and processing instructions.
    // Flatten only their line/tab layout for one-line entries, not the preview
    // or parameter values (whose whitespace is preserved by writeText).
    if (compact)
        text.replace('\n', ' ').replace('\r', ' ').replace('\t', ' ');
    return text;
}

void writeNode(QXmlStreamWriter& writer, const QDomNode& node,
               bool compact, bool preserveSpace = false)
{
    if (node.isElement()) {
        const QDomElement element = node.toElement();
        const QString space = element.attribute(QStringLiteral("xml:space"));
        if (space == QStringLiteral("preserve")) preserveSpace = true;
        else if (space == QStringLiteral("default")) preserveSpace = false;

        bool hasElements = false;
        bool hasText = false;
        for (QDomNode child = node.firstChild(); !child.isNull(); child = child.nextSibling()) {
            hasElements |= child.isElement();
            hasText |= child.isCDATASection()
                || (child.isText() && !isXmlWhitespace(child.nodeValue()));
        }

        const bool parentFormatting = writer.autoFormatting();
        writer.writeStartElement(element.tagName());
        const QDomNamedNodeMap attributes = element.attributes();
        for (int i = 0; i < attributes.count(); ++i) {
            const QDomAttr attribute = attributes.item(i).toAttr();
            writer.writeAttribute(attribute.name(), attribute.value());
        }

        // A List can mix literal values and nested functions. Adding layout
        // whitespace there would change its text, so keep that subtree inline.
        writer.setAutoFormatting(parentFormatting && !hasText && !preserveSpace);
        for (QDomNode child = node.firstChild(); !child.isNull(); child = child.nextSibling()) {
            if (hasElements && !hasText && !preserveSpace && child.isText()
                && !child.isCDATASection() && isXmlWhitespace(child.nodeValue()))
                continue;
            writeNode(writer, child, compact, preserveSpace);
        }
        writer.writeEndElement();
        writer.setAutoFormatting(parentFormatting);
    } else if (node.isText() || node.isCDATASection()) {
        writeText(writer, node.nodeValue(), compact);
    } else if (node.isComment()) {
        writer.writeComment(annotationText(node.nodeValue(), compact));
    } else if (node.isProcessingInstruction()) {
        writer.writeProcessingInstruction(node.nodeName(), annotationText(node.nodeValue(), compact));
    }
}

QString format(const QString& xml, bool compact)
{
    QDomDocument document;
    if (!document.setContent(xml, QDomDocument::ParseOption::PreserveSpacingOnlyNodes)
        || !document.doctype().name().isEmpty())
        return xml;

    QString result;
    QXmlStreamWriter writer(&result);
    writer.setAutoFormatting(!compact);
    writer.setAutoFormattingIndent(4);
    for (QDomNode node = document.firstChild(); !node.isNull(); node = node.nextSibling())
        writeNode(writer, node, compact);
    return result.trimmed();
}

} // namespace

QString FunctionXmlFormat::preview(const QString& xml)
{
    // The editor shows a compact expression; saved project XML has its own
    // independent pretty printer in ProjectXmlWriter.
    return format(xml, true);
}

QString FunctionXmlFormat::compact(const QString& xml)
{
    return format(xml, true);
}
