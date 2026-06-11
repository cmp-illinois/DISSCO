#ifndef REVFUNCTIONBASE_HPP
#define REVFUNCTIONBASE_HPP

#include "../FunctionWidget.hpp"

class QRadioButton;
class QScrollArea;
class QVBoxLayout;
class REVChannel;

/** Base for the three REV variants (Simple, Medium, Advanced). They
 *  share the same Apply radio (SOUND/PARTIAL) and channel list; only
 *  the XML name and the per-channel field set differ -- which
 *  REVChannel already handles via its revMode constructor argument. */
class RevFunctionBase : public FunctionWidget {
    Q_OBJECT

public:
    /** mode: 0 = Simple, 1 = Medium, 2 = Advanced. Matches the legacy
     *  REVMethodFlag values and REVChannel's revMode argument. */
    explicit RevFunctionBase(int mode, QWidget* parent = nullptr);

    QList<FunctionReturnType> supportedReturnTypes() const override {
        return { FunctionReturnType::functionReturnREV };
    }

    QString buildXMLString() const override;
    void populateFromXML(QXmlStreamReader& reader) override;
    void reset() override;

protected:
    int mode() const { return m_mode; }

private:
    REVChannel* insertChannel(REVChannel* prevChan);
    void removeChannel(REVChannel* chan);
    void clearChannels();
    void rebuildDefaults();
    void updateLabels();
    QString currentApply() const;

    int m_mode;
    QRadioButton* m_applySound;
    QRadioButton* m_applyPartial;
    QScrollArea*  m_scrollArea;
    QVBoxLayout*  m_channelsLayout;
    QList<REVChannel*> m_channels;
};

#endif // REVFUNCTIONBASE_HPP
