#ifndef PROJECTVIEW_HPP
#define PROJECTVIEW_HPP

#include "../windows/MainWindow.hpp"
#include "../dialogs/ProjectPropertiesDialog.hpp"
#include "../dialogs/FileNewObject.hpp"
#include "../core/project_struct.hpp"
#include "../../LASS/src/LASS.h"
#include "../core/EnvelopeLibraryEntry.hpp"

#include <QXmlStreamWriter>

class MainWindow;
class EnvelopeLibraryEntry;
class EventAttributesViewController;
class PaletteViewController;

class ProjectView : public QObject {
    Q_OBJECT
    
    public:
        /* init/constructor to spawn a project_view inside of mainwin corresponding to a particular filepath and project_title; i.e., open project */
        ProjectView(MainWindow* _mainWindow, QString _pathAndName);
        ~ProjectView() override;

        /* function to write to the xml .dissco file */
        void save();
        void writeInlineXml(QXmlStreamWriter& xmlWriter, const QString& xmlString);

        /* set properties pop up function */
        void setProperties();

        /* new object pop up function */
        void insertObject();   

        /* functions for modifying the Envelope list */
        // EnvelopeLibraryEntry* getEnvelopeLibraryEntries();
        // EnvelopeLibraryEntry* createNewEnvelope();
        // EnvelopeLibraryEntry* duplicateEnvelope(EnvelopeLibraryEntry* _originalEnvelope);
        // void deleteEnvelope(EnvelopeLibraryEntry* toDelete);

        // ObjectWindow and Palette methods
        void updatePaletteView() const;
        void showAttributes(QString eventType, int index);

        // Remove the event at index from the backend and the palette.
        // If it is currently being viewed, the attributes panel is blanked.
        void deleteEvent(const QString& typeStr, int index);

        // Append a copy of the event at index to the backend and the palette.
        void duplicateEvent(const QString& typeStr, int index);

        // Two-way name sync between palette and attributes view
        void updatePaletteItemName(const QString& typeStr, int index, const QString& name);
        void updateAttributesNameEntry(const QString& typeStr, int index, const QString& name);

        // Rebuild all visible LayerBox UIs to reflect backend mutations
        // (e.g. after reconciling references to a renamed event).
        void reloadAllLayerBoxes();
        //void showAttributes(class IEvent* event);

    private:
        /* storing pointers */
        MainWindow* mainWindow = nullptr;
        ProjectPropertiesDialog* projectPropertiesDialog = nullptr;
        FileNewObject* newObject = nullptr;
        PaletteViewController* paletteView = nullptr;
        EventAttributesViewController* eventAttributesView = nullptr;
        
        /* storing saved state of project */
        bool modifiedButNotSaved;

        /* set properties insert function button */
        void propertiesInsertFunction();

        QMap<QString, bool> default_note_modifiers{
            {"+8va", true},          {"-8va", true},
            {"accent", true},        {"coda", true},
            {"downbow", true},       {"downmordent", true},
            {"downprall", true},     {"espressivo", true},
            {"fermata", true},       {"flageolet", true},
            {"halfopen", true},      {"lheel", true},
            {"lineprall", true},     {"longfermata", true},
            {"ltoe", true},          {"marcato", true},
            {"mordent", true},       {"null", true},
            {"open", true},          {"portato", true},
            {"prall", true},         {"pralldown", true},
            {"prallmordent", true},  {"prallprall", true},
            {"prallup", true},       {"reverseturn", true},
            {"rheel", true},         {"rtoe", true},
            {"segno", true},         {"shortfermata", true},
            {"snappizzicato", true}, {"staccatissimo", true},
            {"staccato", true},      {"stopped", true},
            {"tenuto", true},        {"trill", true},
            {"turn", true},          {"upbow", true},
            {"upmordent", true},     {"upprall", true},
            {"varcoda", true},       {"verylongfermata", true}};
};

#endif
