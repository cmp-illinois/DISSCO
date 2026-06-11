#ifndef SPAFUNCTION_HPP
#define SPAFUNCTION_HPP

#include "../FunctionWidget.hpp"

class QRadioButton;
class QScrollArea;
class QVBoxLayout;
class SPAChannel;

/** SPA. A method radio (STEREO/MULTI_PAN/POLAR) determines how many
 *  channels are present and how they're labeled; an apply radio
 *  (SOUND/PARTIAL) controls whether channel rows can be expanded into
 *  per-partial rows. Switching either radio rebuilds the channel list
 *  from scratch.
 *
 *  @code{.xml}
 *  <Fun><Name>SPA</Name>
 *    <Method>STEREO|MULTI_PAN|POLAR</Method>
 *    <Apply>SOUND|PARTIAL</Apply>
 *    <Channels>
 *      <Partials><P>V</P>...</Partials>...
 *    </Channels>
 *  </Fun>
 *  @endcode
 */
class SPAFunction : public FunctionWidget {
    Q_OBJECT

public:
    explicit SPAFunction(QWidget* parent = nullptr);

    CMODFunction id() const override { return CMODFunction::functionSPA; }
    QString xmlName() const override { return QStringLiteral("SPA"); }
    QString displayName() const override { return QStringLiteral("SPA"); }
    QList<FunctionReturnType> supportedReturnTypes() const override {
        return { FunctionReturnType::functionReturnSPA };
    }

    QString buildXMLString() const override;
    void populateFromXML(QXmlStreamReader& reader) override;
    void reset() override;

private:
    SPAChannel* insertChannel(SPAChannel* prevChan);
    void removeChannel(SPAChannel* chan);
    void clearChannels();
    void rebuildChannelsForCurrentMethod();
    void syncInsertRow(int index);
    void syncRemoveRow(int index);
    void updateLabels();
    QString currentMethod() const;
    QString currentApply() const;

    QRadioButton* m_stereo;
    QRadioButton* m_multiPan;
    QRadioButton* m_polar;
    QRadioButton* m_applySound;
    QRadioButton* m_applyPartial;
    QScrollArea*  m_scrollArea;
    QVBoxLayout*  m_channelsLayout;
    QList<SPAChannel*> m_channels;
};

#endif // SPAFUNCTION_HPP
