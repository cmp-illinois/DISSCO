#ifndef PROJECTXMLWRITER_HPP
#define PROJECTXMLWRITER_HPP

#include "event_struct.hpp"

#include <QString>

class QXmlStreamWriter;

namespace ProjectXmlWriter {

// Write either plain text or a well-formed inline XML fragment as children of
// the element currently open on writer.
void writeInlineXml(QXmlStreamWriter& writer, const QString& value);

// Write the on-disk representation shared by HEvent and Bottom modifiers.
void writeModifier(QXmlStreamWriter& writer, const Modifier& modifier);

// Write <ExtraInfo> for a Bottom event, including the normalized carrier
// phase and any modifiers.
void writeBottomExtraInfo(QXmlStreamWriter& writer, const ExtraInfo& extraInfo);

} // namespace ProjectXmlWriter

#endif // PROJECTXMLWRITER_HPP
