#include "ResultStringBox.hpp"

#include <QTextDocument>
#include <QTextEdit>
#include <QTextOption>
#include <QtMath>

void ResultStringBox::setUp(QTextEdit* box)
{
    box->setAcceptRichText(false);
    box->setLineWrapMode(QTextEdit::WidgetWidth);
    box->setWordWrapMode(QTextOption::WrapAnywhere);
    box->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    box->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

int ResultStringBox::fitHeight(QTextEdit* box, int minimumLines, int maximumLines)
{
    const int frame = 2 * box->frameWidth();
    // The document height already includes the document margins.
    const int margins = frame + qCeil(2 * box->document()->documentMargin());
    const int lineSpacing = box->fontMetrics().lineSpacing();
    const int textHeight = qCeil(box->document()->size().height()) + frame;
    const int height = qBound(minimumLines * lineSpacing + margins, textHeight,
                              maximumLines * lineSpacing + margins);
    const int change = height - box->minimumHeight();
    if (change != 0)
        box->setFixedHeight(height);
    return change;
}
