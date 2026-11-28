#ifndef RESULTSTRINGBOX_HPP
#define RESULTSTRINGBOX_HPP

class QTextEdit;

// Shared behaviour of the boxes that show a function's XML (the "Result
// String"). The XML has no spaces, so the text wraps at any character when it
// reaches the edge of the box. Wrapping only changes the display: the text
// itself stays on one line.
namespace ResultStringBox {

// Wrap at the box edge and never scroll sideways.
void setUp(QTextEdit* box);

// Give the box the height of its wrapped text, but no less than
// `minimumLines` and no more than `maximumLines` lines. Longer text scrolls.
// Returns the change in the box height, in pixels.
int fitHeight(QTextEdit* box, int minimumLines, int maximumLines);

} // namespace ResultStringBox

#endif // RESULTSTRINGBOX_HPP
