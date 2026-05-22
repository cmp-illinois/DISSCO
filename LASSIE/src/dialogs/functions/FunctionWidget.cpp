#include "FunctionWidget.hpp"

#include <QXmlStreamWriter>

QString FunctionWidget::readInner(QXmlStreamReader& r) {
    // Captures the inner content of the element at the current
    // StartElement. For text-only elements returns the text; for elements
    // with nested children returns the serialized inner XML (e.g.
    // "<Fun>...</Fun>" for an embedded function). On return the reader
    // is positioned at the matching EndElement.
    QString result;
    QXmlStreamWriter w(&result);
    bool hasChildElement = false;
    QString textBuffer;
    int depth = 0;
    while (!r.atEnd()) {
        r.readNext();
        switch (r.tokenType()) {
            case QXmlStreamReader::StartElement: {
                hasChildElement = true;
                w.writeStartElement(r.name().toString());
                const auto attrs = r.attributes();
                for (const auto& a : attrs)
                    w.writeAttribute(a.name().toString(), a.value().toString());
                ++depth;
                break;
            }
            case QXmlStreamReader::EndElement:
                if (depth == 0)
                    return hasChildElement ? result : textBuffer;
                w.writeEndElement();
                --depth;
                break;
            case QXmlStreamReader::Characters:
                if (hasChildElement) w.writeCharacters(r.text().toString());
                else textBuffer += r.text().toString();
                break;
            default: break;
        }
    }
    return hasChildElement ? result : textBuffer;
}

QString FunctionWidget::nextChildInner(QXmlStreamReader& r) {
    if (!r.readNextStartElement()) return QString();
    return readInner(r);
}
