/* The 'project' object that keeps track of project details to deploy
 * in the associated window (currently, the project view).
 *
 * Copyright (c) 2025, DISSCO authors */

#include <QFile>
#include <QSaveFile>
#include <QString>
#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QIODevice>
#include <QXmlStreamWriter>
#include <QDebug>
#include <QTextStream>
#include <QUuid>
#include <optional>
#include <variant>

#include <QDialog>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QMessageBox>
#include <QScrollArea>
#include <QTreeView>
#include <QStandardItem>

#include "ProjectViewController.hpp"
#include "../windows/MainWindow.hpp"
#include "../ui/ui_mainwindow.h"
#include "../inst.hpp"
#include "../core/LayerReferenceUtils.hpp"
#include "../core/EnvelopeLibraryEntry.hpp"
#include "../core/ProjectXmlWriter.hpp"
#include "../core/ProjectClipboard.hpp"
#include "../dialogs/ProjectPropertiesDialog.hpp"
#include "../ui/ui_ProjectPropertiesDialog.h"
#include "../dialogs/FunctionGenerator.hpp"
#include "../ui/ui_FunctionGenerator.h"
#include "PaletteViewController.hpp"
#include "../core/event_struct.hpp"
#include "EventAttributesViewController.hpp"
#include "../dialogs/FileNewObject.hpp"
#include "../ui/ui_FileNewObject.h"
// #include "../core/IEvent.h"
// #include "muParser.h"

#include "../utilities.hpp"

using enum FunctionReturnType;

struct PaletteEventCopy {
    Eventtype type;
    std::variant<HEvent, BottomEvent, SpectrumEvent, NoteEvent, EnvelopeEvent,
        SieveEvent, SpaEvent, PatternEvent, ReverbEvent, FilterEvent> value;
};

namespace PVCHelper {
    template<typename Visitor>
    void visitEventList(ProjectManager* pm, Eventtype type, Visitor visit) {
        switch (type) {
            case high:    visit(pm->highevents()); break;
            case mid:     visit(pm->midevents()); break;
            case low:     visit(pm->lowevents()); break;
            case bottom:  visit(pm->bottomevents()); break;
            case sound:   visit(pm->spectrumevents()); break;
            case note:    visit(pm->noteevents()); break;
            case env:     visit(pm->envelopeevents()); break;
            case sieve:   visit(pm->sieveevents()); break;
            case spa:     visit(pm->spaevents()); break;
            case pattern: visit(pm->patternevents()); break;
            case reverb:  visit(pm->reverbevents()); break;
            case filter:  visit(pm->filterevents()); break;
            default: break; // Top is a singleton, not a duplicable object.
        }
    }

    std::optional<PaletteEventCopy> copyEvent(ProjectManager* pm,
                                              Eventtype type, int index) {
        std::optional<PaletteEventCopy> result;
        visitEventList(pm, type, [&](const auto& list) {
            if (index >= 0 && index < list.size())
                result = PaletteEventCopy{type, list[index]};
        });
        return result;
    }

    QList<QStandardItem*> make_child_palette_tuple(const QString& type, const QString& name) {
        auto* typeItem = new QStandardItem(type);
        auto* nameItem = new QStandardItem(name);
        typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        typeItem->setData(type, Qt::UserRole + 1);
        typeItem->setData(name, Qt::UserRole + 2);
        nameItem->setData(type, Qt::UserRole + 1);
        nameItem->setData(name, Qt::UserRole + 2);
        return {typeItem, nameItem};
    }

    void renewModifierIds(QList<Modifier>& modifiers) {
        QHash<QString, QString> replacementByOldId;
        for (Modifier& modifier : modifiers) {
            const QString oldId = modifier.instance_id;
            modifier.instance_id =
                QUuid::createUuid().toString(QUuid::WithoutBraces);
            replacementByOldId.insert(oldId, modifier.instance_id);
        }

        for (Modifier& modifier : modifiers) {
            for (ModifierChanceRule& rule : modifier.rules) {
                for (ModifierCondition& condition : rule.conditions) {
                    const auto replacement =
                        replacementByOldId.constFind(condition.modifier_id);
                    if (replacement != replacementByOldId.cend())
                        condition.modifier_id = replacement.value();
                }
            }
        }
    }
}
/* ProjectView constructor initializing values for XML file*/
ProjectView::ProjectView(MainWindow* _mainWindow, QString /*_pathAndName*/) {

    ProjectManager *pm = Inst::get_project_manager();
    qDebug() << "In PV Constructor p:" << pm->get_curr_project();

    mainWindow = _mainWindow;
    modifiedButNotSaved = true;

    // // Initialize PaletteViewController
    paletteView = new PaletteViewController(this);
    mainWindow->ui->paletteLayout->insertWidget(0, paletteView);

    // Initialize Events Attribute View Controller
    mainWindow->ui->eventsScrollArea->setWidgetResizable(true);
    eventAttributesView = new EventAttributesViewController(this);
    mainWindow->ui->eventsScrollArea->setWidget(eventAttributesView);

    updatePaletteView();
}

ProjectView::~ProjectView() {
    // Clean up lazily-created dialogs
    delete projectPropertiesDialog;
    delete newObject;

    // Remove palette widget from layout and delete it
    if (mainWindow && paletteView) {
        mainWindow->ui->paletteLayout->removeWidget(paletteView);
        delete paletteView;
        paletteView = nullptr;
    }

    // Remove event attributes widget from scroll area and delete it
    if (mainWindow) {
        QWidget* old = mainWindow->ui->eventsScrollArea->takeWidget();
        delete old;
        eventAttributesView = nullptr;
    }
}

// Function to write XML Formatting
void ProjectView::writeInlineXml(QXmlStreamWriter& xmlWriter, const QString& xmlString) {
    ProjectXmlWriter::writeInlineXml(xmlWriter, xmlString);
}

/* Function that creates and saves the xml .dissco file */
bool ProjectView::save(){
    qDebug() << "In Project View Save Function";

    eventAttributesView->saveCurrentShownEventData();

    ProjectManager *pm = Inst::get_project_manager();
    const auto modifierListNeedsReview = [](const QList<Modifier>& modifiers) {
        for (const Modifier& modifier : modifiers) {
            if (modifier.usage_metadata_needs_review)
                return true;
        }
        return false;
    };
    bool needsCompatibilityBackup = false;
    needsCompatibilityBackup =
        modifierListNeedsReview(pm->topevent().modifiers);
    for (const HEvent& event : pm->highevents())
        needsCompatibilityBackup = needsCompatibilityBackup
            || modifierListNeedsReview(event.modifiers);
    for (const HEvent& event : pm->midevents())
        needsCompatibilityBackup = needsCompatibilityBackup
            || modifierListNeedsReview(event.modifiers);
    for (const HEvent& event : pm->lowevents())
        needsCompatibilityBackup = needsCompatibilityBackup
            || modifierListNeedsReview(event.modifiers);
    for (const BottomEvent& bottomEvent : pm->bottomevents()) {
        needsCompatibilityBackup = needsCompatibilityBackup
            || bottomEvent.extra_info.modifier_usage_needs_review
            || modifierListNeedsReview(bottomEvent.extra_info.modifiers);
    }

    const QFileInfo originalFileInfo = pm->fileinfo();
    if (needsCompatibilityBackup && originalFileInfo.exists()) {
        const QString backupName =
            originalFileInfo.completeBaseName()
            + QStringLiteral(".pre-modifier-usage.")
            + QUuid::createUuid().toString(QUuid::WithoutBraces)
            + QStringLiteral(".dissco");
        const QString backupPath =
            originalFileInfo.absoluteDir().filePath(backupName);
        // QFile::copy refuses to overwrite an existing file. Combined with a
        // fresh UUID, every conversion save preserves the exact current input
        // instead of silently reusing a stale backup from an earlier save.
        if (!QFile::copy(originalFileInfo.absoluteFilePath(), backupPath)) {
            QMessageBox::critical(
                mainWindow, tr("Could not create compatibility backup"),
                tr("The project was not saved because LASSIE could not "
                   "preserve the original Modifier Group file at:\n%1")
                    .arg(backupPath));
            return false;
        }
        QMessageBox::information(
                mainWindow, tr("Compatibility backup created"),
                tr("This project contains legacy or incomplete Modifier "
                   "Usage data. The original file was preserved at:\n%1\n\n"
                   "The saved project will use only the new Modifier Usage "
                   "format.")
                    .arg(backupPath));
    }

    // ensure directory exists before creating file
    QFileInfo fileInfo = pm->fileinfo();
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qDebug() << "Failed to create directory:" << dir.absolutePath();
            QMessageBox::critical(
                mainWindow, tr("Could not save project"),
                tr("LASSIE could not create the project directory:\n%1")
                    .arg(dir.absolutePath()));
            return false;
        }
    }

    // QSaveFile writes beside the destination and atomically replaces it only
    // after the complete XML document has been written successfully.
    QSaveFile file(pm->fileinfo().absoluteFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file:" << file.fileName();
        qDebug() << "Error reason:" << file.errorString();
        QMessageBox::critical(
            mainWindow, tr("Could not save project"),
            tr("LASSIE could not open the project file for writing:\n%1\n\n%2")
                .arg(QDir::toNativeSeparators(file.fileName()),
                     file.errorString()));
        return false;
    }
    
    // QXmlStreamWriter class writes to the XML file
    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true);
    xmlWriter.writeStartDocument("1.0");
    xmlWriter.writeStartElement("ProjectRoot");
        xmlWriter.writeStartElement("ProjectConfiguration");

            xmlWriter.writeStartElement("Title");	
                xmlWriter.writeCharacters(pm->title());
            xmlWriter.writeEndElement();

            xmlWriter.writeStartElement("FileFlag");	
                xmlWriter.writeCharacters(pm->fileflag());
            xmlWriter.writeEndElement();

            xmlWriter.writeStartElement("TopEvent");	
                xmlWriter.writeCharacters(pm->topevent().name);
            xmlWriter.writeEndElement();

            xmlWriter.writeStartElement("PieceStartTime");	
                xmlWriter.writeCharacters(pm->starttime());
            xmlWriter.writeEndElement();

            xmlWriter.writeStartElement("Duration");	
                writeInlineXml(xmlWriter, pm->duration());
            xmlWriter.writeEndElement();

            xmlWriter.writeStartElement("Synthesis");
                xmlWriter.writeCharacters(pm->synthesis() ? "True" : "False");
            xmlWriter.writeEndElement();

            xmlWriter.writeStartElement("Score");
                xmlWriter.writeCharacters(pm->score() ? "True" : "False");
            xmlWriter.writeEndElement();

            xmlWriter.writeStartElement("GrandStaff");
                xmlWriter.writeCharacters(pm->grandstaff() ? "True" : "False");
            xmlWriter.writeEndElement();

            xmlWriter.writeStartElement("NumberOfStaff");	
                xmlWriter.writeCharacters(pm->numstaffs());
            xmlWriter.writeEndElement();   

            xmlWriter.writeStartElement("NumberOfChannels");	
                xmlWriter.writeCharacters(pm->numchannels());
            xmlWriter.writeEndElement(); 

            xmlWriter.writeStartElement("SampleRate");	
                xmlWriter.writeCharacters(pm->samplerate());
            xmlWriter.writeEndElement(); 

            xmlWriter.writeStartElement("SampleSize");	
                xmlWriter.writeCharacters(pm->samplesize());
            xmlWriter.writeEndElement();

            xmlWriter.writeStartElement("NumberOfThreads");	
                xmlWriter.writeCharacters(pm->numthreads());
            xmlWriter.writeEndElement(); 

            xmlWriter.writeStartElement("OutputParticel");
                xmlWriter.writeCharacters(pm->outputparticel() ? "True" : "False");
            xmlWriter.writeEndElement();

            xmlWriter.writeStartElement("Seed");	
                xmlWriter.writeCharacters(pm->seed());
            xmlWriter.writeEndElement(); 

        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("NoteModifiers");
            xmlWriter.writeStartElement("DefaultModifiers");	
                auto iter = default_note_modifiers.begin();
                while (iter != default_note_modifiers.end()){
                    if ((*iter)) { xmlWriter.writeCharacters("1"); }
                    else { xmlWriter.writeCharacters("0"); }
                    iter++;
                    if (iter!= default_note_modifiers.end()){
                        xmlWriter.writeCharacters(", ");
                    }
                }
            xmlWriter.writeEndElement();	
            /* STILL IN PROGRESS  */
            xmlWriter.writeStartElement("CustomModifiers");	
                xmlWriter.writeCharacters("");
                if (pm->customnotemodifiers().size() != 0){
                    auto iter2 = pm->customnotemodifiers().begin();
                    while (iter2 != pm->customnotemodifiers().end()){
                        xmlWriter.writeStartElement("Modifier");	
                        xmlWriter.writeCharacters(*iter2);
                        xmlWriter.writeEndElement();   
                        iter2 ++;  
                    }
                }
            xmlWriter.writeEndElement();       
        xmlWriter.writeEndElement(); 

        xmlWriter.writeStartElement("EnvelopeLibrary");
            QString stringBuffer;
            if (pm->envlibentries() != NULL){
                EnvelopeLibraryEntry* envLib = pm->envlibentries();
                int count = envLib->count();
                stringBuffer = stringBuffer + "\n" + QString::number(count) + "\n";
                count = 1;
                while (envLib != NULL) {
                    stringBuffer = stringBuffer + "Envelope " + QString::number(count) + "\n";
                    int lineNumber = envLib->head->countNumOfNodes();
                    stringBuffer = stringBuffer + QString::number(lineNumber) + "\n";
                    
                    EnvLibEntryNode* currentNode;
                    EnvLibEntrySeg* libSeg = envLib->head->rightSeg;
                    // Every envelope has at least two nodes, hence at least one segment.
                    do {
                        currentNode = libSeg->leftNode;
                        stringBuffer = stringBuffer + QString::number(currentNode->x, 'f', 3);
                        stringBuffer = stringBuffer + "     ";
                        stringBuffer = stringBuffer + QString::number(currentNode->y, 'f', 3);
                        stringBuffer = stringBuffer + "     ";

                        if (libSeg->segmentType == envSegmentTypeLinear){
                            stringBuffer = stringBuffer + "LINEAR              ";
                        } else if (libSeg->segmentType == envSegmentTypeExponential){
                            stringBuffer = stringBuffer + "EXPONENTIAL         ";
                        } else {
                            stringBuffer = stringBuffer + "CUBIC_SPLINE        ";
                        }

                        if (libSeg->segmentProperty == envSegmentPropertyFlexible){
                            stringBuffer = stringBuffer + "FLEXIBLE    ";
                        } else {
                            stringBuffer = stringBuffer + "FIXED       ";
                        }

                        stringBuffer = stringBuffer + QString::number((libSeg->rightNode->x) - (currentNode->x), 'f', 3) + "\n";
                        libSeg = libSeg->rightNode->rightSeg;
                    } while (libSeg != NULL);

                    currentNode = currentNode->rightSeg->rightNode;
                    stringBuffer = stringBuffer + QString::number(currentNode->x, 'f', 3) + "     ";
                    stringBuffer = stringBuffer + QString::number(currentNode->y, 'f', 3) + "\n";
                    
                    count++;
                    envLib = envLib->next;
                }
                stringBuffer = stringBuffer + "    "; 
            }
            xmlWriter.writeCharacters(stringBuffer);
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("MarkovModelLibrary");
            QString markovBuffer = "\n" + QString::number(pm->markovmodels().size()) + "\n";
            for (int i = 0; i < pm->markovmodels().size(); ++i) {
                markovBuffer += QString::fromStdString(pm->markovmodels()[i].to_str()) + "\n";
            }
            xmlWriter.writeCharacters(markovBuffer);
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("Events");	
            /* STILL IN PROGRESS  */
            /* really ugly to have this be copied when it should be a const reference & implicitly shared, but I'm still mulling how to const-initialize this with the contents of 4 QLists.. -jacob */
            QList<HEvent> pHevents;
            // populate vector with all HEvent subcategories
            pHevents.append(pm->topevent());
            pHevents.append(pm->highevents());
            pHevents.append(pm->midevents());
            pHevents.append(pm->lowevents());

            for (const HEvent& item : pHevents) {
              xmlWriter.writeStartElement("Event");
              xmlWriter.writeAttribute("orderInPalette", item.orderinpalette);	
                xmlWriter.writeStartElement("EventType");
                    xmlWriter.writeCharacters(QString("%1").arg(item.type));	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("Name");
                    xmlWriter.writeCharacters(item.name);	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("MaxChildDuration");
                    writeInlineXml(xmlWriter, item.max_child_duration);	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("EDUPerBeat");
                    writeInlineXml(xmlWriter, item.edu_perbeat);	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("TimeSignature");
                    xmlWriter.writeStartElement("Entry1");
                        xmlWriter.writeCharacters(QString("%1").arg(item.timesig.bar_value));
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("Entry2");
                        xmlWriter.writeCharacters(QString("%1").arg(item.timesig.note_value));
                    xmlWriter.writeEndElement();
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("Tempo");
                    xmlWriter.writeStartElement("MethodFlag");
                        xmlWriter.writeCharacters(QString("%1").arg(item.tempo.method_flag));
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("Prefix");
                        xmlWriter.writeCharacters(QString("%1").arg(item.tempo.prefix));
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("NoteValue");
                        xmlWriter.writeCharacters(QString("%1").arg(item.tempo.note_value));
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("FractionEntry1");
                        xmlWriter.writeCharacters(item.tempo.frentry_1);
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("FractionEntry2");
                        xmlWriter.writeCharacters(item.tempo.frentry_2);
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("ValueEntry");
                        xmlWriter.writeCharacters(item.tempo.valentry);
                    xmlWriter.writeEndElement();
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("NumberOfChildren");
                    xmlWriter.writeStartElement("MethodFlag");
                        xmlWriter.writeCharacters(QString("%1").arg(item.numchildren.method_flag));
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("Entry1");
                        writeInlineXml(xmlWriter, item.numchildren.entry_1);
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("Entry2");
                        writeInlineXml(xmlWriter, item.numchildren.entry_2);
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("Entry3");
                        writeInlineXml(xmlWriter, item.numchildren.entry_3);
                    xmlWriter.writeEndElement();
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("ChildEventDefinition");
                    xmlWriter.writeStartElement("Entry1");
                        writeInlineXml(xmlWriter, item.child_event_def.entry_1);
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("Entry2");
                        writeInlineXml(xmlWriter, item.child_event_def.entry_2);
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("Entry3");
                        writeInlineXml(xmlWriter, item.child_event_def.entry_3);
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("AttackSieve");
                        writeInlineXml(xmlWriter, item.child_event_def.attack_sieve);
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("DurationSieve");
                        writeInlineXml(xmlWriter, item.child_event_def.duration_sieve);
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("DefinitionFlag");
                        xmlWriter.writeCharacters(QString("%1").arg(item.child_event_def.definition_flag));
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("StartTypeFlag");
                        xmlWriter.writeCharacters(QString("%1").arg(item.child_event_def.starttype_flag));
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("DurationTypeFlag");
                        xmlWriter.writeCharacters(QString("%1").arg(item.child_event_def.durationtype_flag));
                    xmlWriter.writeEndElement();
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("Layers");
                for (const Layer& itemLayer : item.event_layers) {
                    xmlWriter.writeStartElement("Layer");
                        xmlWriter.writeStartElement("ByLayer");
                            xmlWriter.writeCharacters(itemLayer.by_layer);
                        xmlWriter.writeEndElement();
                        xmlWriter.writeStartElement("DiscretePackages");
                        for (Package layerPkg : itemLayer.discrete_packages) {
                            xmlWriter.writeStartElement("Package");
                                xmlWriter.writeStartElement("EventName");
                                    xmlWriter.writeCharacters(layerPkg.event_name);
                                xmlWriter.writeEndElement();
                                xmlWriter.writeStartElement("EventType");
                                    xmlWriter.writeCharacters(layerPkg.event_type);
                                xmlWriter.writeEndElement();
                                xmlWriter.writeStartElement("Weight");
                                    xmlWriter.writeCharacters(layerPkg.weight);
                                xmlWriter.writeEndElement();
                                xmlWriter.writeStartElement("AttackEnvelope");
                                    xmlWriter.writeCharacters(layerPkg.attack_envelope);
                                xmlWriter.writeEndElement();
                                xmlWriter.writeStartElement("AttackEnvelopeScale");
                                    xmlWriter.writeCharacters(layerPkg.attackenvelope_scale);
                                xmlWriter.writeEndElement();
                                xmlWriter.writeStartElement("DurationEnvelope");
                                    xmlWriter.writeCharacters(layerPkg.duration_envelope);
                                xmlWriter.writeEndElement();
                                xmlWriter.writeStartElement("DurationEnvelopeScale");
                                    xmlWriter.writeCharacters(layerPkg.durationenvelope_scale);
                                xmlWriter.writeEndElement();    
                            xmlWriter.writeEndElement();
                        }
                        xmlWriter.writeEndElement();
                    xmlWriter.writeEndElement();
                }
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("Spatialization");
                        writeInlineXml(xmlWriter, item.spa);
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("Reverb");
                        writeInlineXml(xmlWriter, item.reverb);
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("Filter");
                        writeInlineXml(xmlWriter, item.filter);
                    xmlWriter.writeEndElement(); 
                    xmlWriter.writeStartElement("Modifiers");
                    for (const Modifier& itemMod : item.modifiers)
                        ProjectXmlWriter::writeModifier(xmlWriter, itemMod);
                    xmlWriter.writeEndElement();
              xmlWriter.writeEndElement();
            }

            const QList<BottomEvent>& pBevents = pm->bottomevents();
            for (const BottomEvent& item : pBevents) {
              xmlWriter.writeStartElement("Event");
              xmlWriter.writeAttribute("orderInPalette", item.event.orderinpalette);	
                xmlWriter.writeStartElement("EventType");
                    xmlWriter.writeCharacters(QString("%1").arg(item.event.type));	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("Name");
                    xmlWriter.writeCharacters(item.event.name);	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("MaxChildDuration");
                    writeInlineXml(xmlWriter, item.event.max_child_duration);	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("EDUPerBeat");
                    xmlWriter.writeCharacters(item.event.edu_perbeat);	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("TimeSignature");
                    xmlWriter.writeStartElement("Entry1");
                        xmlWriter.writeCharacters(item.event.timesig.bar_value);
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("Entry2");
                        xmlWriter.writeCharacters(item.event.timesig.note_value);
                    xmlWriter.writeEndElement();
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("Tempo");
                    xmlWriter.writeStartElement("MethodFlag");
                        xmlWriter.writeCharacters(QString("%1").arg(item.event.tempo.method_flag));
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("Prefix");
                        xmlWriter.writeCharacters(QString("%1").arg(item.event.tempo.prefix));
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("NoteValue");
                        xmlWriter.writeCharacters(QString("%1").arg(item.event.tempo.note_value));
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("FractionEntry1");
                        xmlWriter.writeCharacters(item.event.tempo.frentry_1);
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("FractionEntry2");
                        xmlWriter.writeCharacters(item.event.tempo.frentry_2);
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("ValueEntry");
                        xmlWriter.writeCharacters(item.event.tempo.valentry);
                    xmlWriter.writeEndElement();
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("NumberOfChildren");
                    xmlWriter.writeStartElement("MethodFlag");
                        xmlWriter.writeCharacters(QString("%1").arg(item.event.numchildren.method_flag));
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("Entry1");
                        writeInlineXml(xmlWriter, item.event.numchildren.entry_1);
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("Entry2");
                        writeInlineXml(xmlWriter, item.event.numchildren.entry_2);
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("Entry3");
                        writeInlineXml(xmlWriter, item.event.numchildren.entry_3);
                    xmlWriter.writeEndElement();
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("ChildEventDefinition");
                    xmlWriter.writeStartElement("Entry1");
                        writeInlineXml(xmlWriter, item.event.child_event_def.entry_1);
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("Entry2");
                        writeInlineXml(xmlWriter, item.event.child_event_def.entry_2);
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("Entry3");
                        writeInlineXml(xmlWriter, item.event.child_event_def.entry_3);
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("AttackSieve");
                        xmlWriter.writeCharacters(item.event.child_event_def.attack_sieve);
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("DurationSieve");
                        xmlWriter.writeCharacters(item.event.child_event_def.duration_sieve);
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("DefinitionFlag");
                        xmlWriter.writeCharacters(QString("%1").arg(item.event.child_event_def.definition_flag));
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("StartTypeFlag");
                        xmlWriter.writeCharacters(QString("%1").arg(item.event.child_event_def.starttype_flag));
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("DurationTypeFlag");
                        xmlWriter.writeCharacters(QString("%1").arg(item.event.child_event_def.durationtype_flag));
                    xmlWriter.writeEndElement();
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("Layers");
                for (const Layer& itemLayer : item.event.event_layers) {
                    xmlWriter.writeStartElement("Layer");
                        xmlWriter.writeStartElement("ByLayer");
                            xmlWriter.writeCharacters(itemLayer.by_layer);
                        xmlWriter.writeEndElement();
                        xmlWriter.writeStartElement("DiscretePackages");
                        for (const Package& layerPkg : itemLayer.discrete_packages) {
                            xmlWriter.writeStartElement("Package");
                                xmlWriter.writeStartElement("EventName");
                                    xmlWriter.writeCharacters(layerPkg.event_name);
                                xmlWriter.writeEndElement();
                                xmlWriter.writeStartElement("EventType");
                                    xmlWriter.writeCharacters(layerPkg.event_type);
                                xmlWriter.writeEndElement();
                                xmlWriter.writeStartElement("Weight");
                                    xmlWriter.writeCharacters(layerPkg.weight);
                                xmlWriter.writeEndElement();
                                xmlWriter.writeStartElement("AttackEnvelope");
                                    xmlWriter.writeCharacters(layerPkg.attack_envelope);
                                xmlWriter.writeEndElement();
                                xmlWriter.writeStartElement("AttackEnvelopeScale");
                                    xmlWriter.writeCharacters(layerPkg.attackenvelope_scale);
                                xmlWriter.writeEndElement();
                                xmlWriter.writeStartElement("DurationEnvelope");
                                    xmlWriter.writeCharacters(layerPkg.duration_envelope);
                                xmlWriter.writeEndElement();
                                xmlWriter.writeStartElement("DurationEnvelopeScale");
                                    xmlWriter.writeCharacters(layerPkg.durationenvelope_scale);
                                xmlWriter.writeEndElement();    
                            xmlWriter.writeEndElement();
                        }
                        xmlWriter.writeEndElement();
                    xmlWriter.writeEndElement();
                }
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("Spatialization");
                        writeInlineXml(xmlWriter, item.event.spa);
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("Reverb");
                        writeInlineXml(xmlWriter, item.event.reverb);
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("Filter");
                        writeInlineXml(xmlWriter, item.event.filter);
                    xmlWriter.writeEndElement();  
                    ProjectXmlWriter::writeBottomExtraInfo(xmlWriter, item.extra_info);
              xmlWriter.writeEndElement();  
            }

            const QList<SpectrumEvent>& pSevents = pm->spectrumevents();
            for (const SpectrumEvent& item : pSevents) {
              xmlWriter.writeStartElement("Event");
              xmlWriter.writeAttribute("orderInPalette", item.orderinpalette);	
                xmlWriter.writeStartElement("EventType");
                    xmlWriter.writeCharacters(QString("%1").arg(sound));	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("Name");
                    xmlWriter.writeCharacters(item.name);	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("NumberOfPartials");
                    writeInlineXml(xmlWriter, item.num_partials);	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("Deviation");
                    writeInlineXml(xmlWriter, item.deviation);	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("GenerateSpectrum");
                    writeInlineXml(xmlWriter, item.generate_spectrum);	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("Spectrum");
                    for (QString partial : item.spectrum.partials) {
                        xmlWriter.writeStartElement("Partial");
                            writeInlineXml(xmlWriter, partial);	
                        xmlWriter.writeEndElement();
                    }
                xmlWriter.writeEndElement();
              xmlWriter.writeEndElement();
            }

            const QList<NoteEvent>& pNevents = pm->noteevents();
            for (const NoteEvent& item : pNevents) {
              xmlWriter.writeStartElement("Event");
              xmlWriter.writeAttribute("orderInPalette", item.orderinpalette);	
                xmlWriter.writeStartElement("EventType");
                    xmlWriter.writeCharacters(QString("%1").arg(note));	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("Name");
                    xmlWriter.writeCharacters(item.name);	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("NoteInfo");
                    xmlWriter.writeStartElement("Staffs");
                        xmlWriter.writeCharacters(item.note_info.staffs);	
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("Modifiers");
                    for (QString modifier : item.note_info.modifiers) {
                        xmlWriter.writeStartElement("Modifier");
                            writeInlineXml(xmlWriter, modifier);	
                        xmlWriter.writeEndElement();
                    }
                    xmlWriter.writeEndElement();
                xmlWriter.writeEndElement();
              xmlWriter.writeEndElement();
            }

            const QList<EnvelopeEvent>& pEevents = pm->envelopeevents();
            for (const EnvelopeEvent& item : pEevents) {
              xmlWriter.writeStartElement("Event");
              xmlWriter.writeAttribute("orderInPalette", item.orderinpalette);	
                xmlWriter.writeStartElement("EventType");
                    xmlWriter.writeCharacters(QString("%1").arg(env));	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("Name");
                    xmlWriter.writeCharacters(item.name);	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("EnvelopeBuilder");
                    writeInlineXml(xmlWriter, item.envelope_builder);	
                xmlWriter.writeEndElement();
              xmlWriter.writeEndElement();
            }

            const QList<SieveEvent>& pSivevents = pm->sieveevents();
            for (const SieveEvent& item : pSivevents) {
              xmlWriter.writeStartElement("Event");
              xmlWriter.writeAttribute("orderInPalette", item.orderinpalette);	
                xmlWriter.writeStartElement("EventType");
                    xmlWriter.writeCharacters(QString("%1").arg(sieve));	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("Name");
                    xmlWriter.writeCharacters(item.name);	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("SieveBuilder");
                    writeInlineXml(xmlWriter, item.sieve_builder);	
                xmlWriter.writeEndElement();
              xmlWriter.writeEndElement();
            }

            const QList<SpaEvent>& pSpaevents = pm->spaevents();
            for (const SpaEvent& item : pSpaevents) {
              xmlWriter.writeStartElement("Event");
              xmlWriter.writeAttribute("orderInPalette", item.orderinpalette);	
                xmlWriter.writeStartElement("EventType");
                    xmlWriter.writeCharacters(QString("%1").arg(spa));	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("Name");
                    xmlWriter.writeCharacters(item.name);	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("Spatialization");
                    writeInlineXml(xmlWriter, item.spatialization);	
                xmlWriter.writeEndElement();
              xmlWriter.writeEndElement();
            }

            const QList<PatternEvent>& pPevents = pm->patternevents();
            for (const PatternEvent& item : pPevents) {
              xmlWriter.writeStartElement("Event");
              xmlWriter.writeAttribute("orderInPalette", item.orderinpalette);	
                xmlWriter.writeStartElement("EventType");
                    xmlWriter.writeCharacters(QString("%1").arg(pattern));	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("Name");
                    xmlWriter.writeCharacters(item.name);	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("PatternBuilder");
                    writeInlineXml(xmlWriter, item.pattern_builder);	
                xmlWriter.writeEndElement();
              xmlWriter.writeEndElement();
            }

            const QList<ReverbEvent>& pRevents = pm->reverbevents();
            for (const ReverbEvent& item : pRevents) {
              xmlWriter.writeStartElement("Event");
              xmlWriter.writeAttribute("orderInPalette", item.orderinpalette);	
                xmlWriter.writeStartElement("EventType");
                    xmlWriter.writeCharacters(QString("%1").arg(reverb));	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("Name");
                    xmlWriter.writeCharacters(item.name);	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("Reverberation");
                    writeInlineXml(xmlWriter, item.reverberation);	
                xmlWriter.writeEndElement();
              xmlWriter.writeEndElement();
            }

            const QList<FilterEvent>& pFevents = pm->filterevents();
            for (const FilterEvent& item : pFevents) {
              xmlWriter.writeStartElement("Event");
              xmlWriter.writeAttribute("orderInPalette", item.orderinpalette);	
                xmlWriter.writeStartElement("EventType");
                    xmlWriter.writeCharacters(QString("%1").arg(filter));	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("Name");
                    xmlWriter.writeCharacters(item.name);	
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("FilterBuilder");
                    writeInlineXml(xmlWriter, item.filter_builder);	
                xmlWriter.writeEndElement();
              xmlWriter.writeEndElement();
            }

        xmlWriter.writeEndElement();
    xmlWriter.writeEndElement();
    xmlWriter.writeEndDocument();

    if (xmlWriter.hasError()) {
        const QString error = file.errorString();
        file.cancelWriting();
        QMessageBox::critical(
            mainWindow, tr("Could not save project"),
            tr("An error occurred while writing the project file:\n%1\n\n%2")
                .arg(QDir::toNativeSeparators(file.fileName()), error));
        return false;
    }
    if (!file.commit()) {
        QMessageBox::critical(
            mainWindow, tr("Could not save project"),
            tr("LASSIE could not replace the project file:\n%1\n\n%2")
                .arg(QDir::toNativeSeparators(file.fileName()),
                     file.errorString()));
        return false;
    }

    // A successful migration save is the user's confirmation of the new
    // format. Clear the transient review markers only after the file commit;
    // otherwise a later normal save would back up the already-migrated XML.
    const auto clearModifierReviewFlags = [](QList<Modifier>& modifiers) {
        for (Modifier& modifier : modifiers)
            modifier.usage_metadata_needs_review = false;
    };
    clearModifierReviewFlags(pm->topevent().modifiers);
    for (HEvent& event : pm->highevents())
        clearModifierReviewFlags(event.modifiers);
    for (HEvent& event : pm->midevents())
        clearModifierReviewFlags(event.modifiers);
    for (HEvent& event : pm->lowevents())
        clearModifierReviewFlags(event.modifiers);
    for (BottomEvent& bottomEvent : pm->bottomevents()) {
        bottomEvent.extra_info.modifier_usage_needs_review = false;
        clearModifierReviewFlags(bottomEvent.extra_info.modifiers);
    }

    modifiedButNotSaved = false;
    return true;
}

void ProjectView::setProperties() {
    if (!projectPropertiesDialog) {
        projectPropertiesDialog = new ProjectPropertiesDialog(mainWindow);
        connect(projectPropertiesDialog->ui->insertFunctionButton, &QPushButton::clicked, this, &ProjectView::propertiesInsertFunction);
    }

    ProjectManager *pm = Inst::get_project_manager();

    projectPropertiesDialog->ui->titleEntry->setText(pm->title());
    projectPropertiesDialog->ui->flagEntry->setText(pm->fileflag());
    projectPropertiesDialog->ui->numChannelsEntry->setText(pm->numchannels());
    projectPropertiesDialog->ui->rateEntry->setText(pm->samplerate());
    projectPropertiesDialog->ui->sizeEntry->setText(pm->samplesize());
    projectPropertiesDialog->ui->numThreadsEntry->setText(pm->numthreads());
    projectPropertiesDialog->ui->synthesisCheckBox->setCheckState(pm->synthesis() ? Qt::Checked : Qt::Unchecked);
    projectPropertiesDialog->ui->scoreCheckBox->setCheckState(pm->score() ? Qt::Checked : Qt::Unchecked);
    projectPropertiesDialog->ui->staffCheckBox->setCheckState(pm->grandstaff() ? Qt::Checked : Qt::Unchecked);
    projectPropertiesDialog->ui->numStaffEntry->setText(pm->numstaffs());
    projectPropertiesDialog->ui->particelBox->setCheckState(pm->outputparticel() ? Qt::Checked : Qt::Unchecked);
    projectPropertiesDialog->ui->topEventEntry->setText(pm->topevent().name);
    projectPropertiesDialog->ui->topEventEntry->setEnabled(false);
    projectPropertiesDialog->ui->durationEntry->setText(pm->duration());

 
    if (projectPropertiesDialog->exec() == QDialog::Accepted) {
        bool changed = false;
        const auto updateProperty = [&changed](auto& value, const auto& newValue) {
            if (value != newValue) {
                value = newValue;
                changed = true;
            }
        };
        QString new_title = projectPropertiesDialog->ui->titleEntry->text();
        updateProperty(pm->fileflag(), projectPropertiesDialog->ui->flagEntry->text());
        updateProperty(pm->numchannels(), projectPropertiesDialog->ui->numChannelsEntry->text());
        updateProperty(pm->samplerate(), projectPropertiesDialog->ui->rateEntry->text());
        updateProperty(pm->samplesize(), projectPropertiesDialog->ui->sizeEntry->text());
        updateProperty(pm->numthreads(), projectPropertiesDialog->ui->numThreadsEntry->text());
        updateProperty(pm->synthesis(), projectPropertiesDialog->ui->synthesisCheckBox->isChecked());
        updateProperty(pm->score(), projectPropertiesDialog->ui->scoreCheckBox->isChecked());
        updateProperty(pm->grandstaff(), projectPropertiesDialog->ui->staffCheckBox->isChecked());
        updateProperty(pm->numstaffs(), projectPropertiesDialog->ui->numStaffEntry->text());
        updateProperty(pm->outputparticel(), projectPropertiesDialog->ui->particelBox->isChecked());
        QString new_topevent = projectPropertiesDialog->ui->topEventEntry->text();
        updateProperty(pm->duration(), projectPropertiesDialog->ui->durationEntry->text());

        if (new_title != pm->title()) {
            QString old_pathAndName = pm->fileinfo().absoluteFilePath();
            QString new_pathAndName = pm->fileinfo().absolutePath() + "/" + new_title + ".dissco";
            if (QFile::rename(old_pathAndName, new_pathAndName)) {
                pm->title() = new_title;
                changed = true;
            }
        }

        if (changed)
            MUtilities::modified();
        delete projectPropertiesDialog;
        projectPropertiesDialog = NULL;
    }
}

void ProjectView::propertiesInsertFunction() {
    if (!projectPropertiesDialog) return;
    auto* generator = new FunctionGenerator(mainWindow, functionReturnFloat,
        projectPropertiesDialog->ui->durationEntry->text());
    if (generator->exec() == QDialog::Accepted) {
        QString result = generator->getResultString();
        if (!result.isEmpty()) {
            projectPropertiesDialog->ui->durationEntry->setText(result);
        }
    }
    delete generator;
}

void ProjectView::insertObject() {
    if (!newObject)
        newObject = new FileNewObject(mainWindow);

    const QString selectedType = paletteView->selectedType();
    if (!selectedType.isEmpty())
        newObject->setDefaultType(selectedType);

    {
        ProjectManager *pm = Inst::get_project_manager();
        QMap<QString, QStringList> existingNames;
        auto hnames = [&](QList<HEvent>& list) {
            QStringList names;
            for (const HEvent& e : list) names << e.name;
            return names;
        };
        auto snames = [&](const auto& list) {
            QStringList names;
            for (const auto& e : list) names << e.name;
            return names;
        };
        existingNames["High"]           = hnames(pm->highevents());
        existingNames["Mid"]            = hnames(pm->midevents());
        existingNames["Low"]            = hnames(pm->lowevents());
        {
            QStringList names;
            for (const BottomEvent& e : pm->bottomevents()) names << e.event.name;
            existingNames["Bottom"] = names;
        }
        existingNames["Spectrum"]       = snames(pm->spectrumevents());
        existingNames["Note"]           = snames(pm->noteevents());
        existingNames["Envelope"]       = snames(pm->envelopeevents());
        existingNames["Sieve"]          = snames(pm->sieveevents());
        existingNames["Spatialization"] = snames(pm->spaevents());
        existingNames["Pattern"]        = snames(pm->patternevents());
        existingNames["Reverb"]         = snames(pm->reverbevents());
        existingNames["Filter"]         = snames(pm->filterevents());
        newObject->setExistingNames(existingNames);
    }

    if (newObject->exec() == QDialog::Accepted) {
        ProjectManager *pm = Inst::get_project_manager();
        QString nameStr = newObject->ui->objNameEntry->text();
        QString typeStr;
        QStandardItem* folder = nullptr;

        // Appends a new HEvent (High/Mid/Low) and sets typeStr/folder
        auto addHEvent = [&](QList<HEvent>& list, Eventtype t, const QString& ts, QStandardItem* f) {
            HEvent obj = {};
            obj.type = t;
            obj.name = nameStr;
            obj.event_layers.append(Layer());
            list.push_back(obj);
            typeStr = ts;
            folder = f;
        };

        // Appends a new simple event (with .orderinpalette/.name) and sets typeStr/folder
        auto addSimple = [&](auto& list, const QString& ts, QStandardItem* f) {
            using T = typename std::decay_t<decltype(list)>::value_type;
            T obj = {};
            obj.orderinpalette = QString::number(list.size() + 1);
            obj.name = nameStr;
            list.push_back(obj);
            typeStr = ts;
            folder = f;
        };

        if      (newObject->ui->buttonHigh->isChecked())
            addHEvent(pm->highevents(), high, "High", paletteView->folderHigh);
        else if (newObject->ui->buttonMid->isChecked())
            addHEvent(pm->midevents(),  mid,  "Mid",  paletteView->folderMid);
        else if (newObject->ui->buttonLow->isChecked())
            addHEvent(pm->lowevents(),  low,  "Low",  paletteView->folderLow);
        else if (newObject->ui->buttonBottom->isChecked()) {
            BottomEvent obj = {};
            obj.event.type = bottom;
            obj.event.name = nameStr;
            obj.event.event_layers.append(Layer());
            pm->bottomevents().push_back(obj);
            typeStr = "Bottom";
            folder = paletteView->folderBottom;
        }
        else if (newObject->ui->buttonSpectrum->isChecked())
            addSimple(pm->spectrumevents(),  "Spectrum",       paletteView->folderSpectrum);
        else if (newObject->ui->buttonNote->isChecked())
            addSimple(pm->noteevents(),      "Note",           paletteView->folderNote);
        else if (newObject->ui->buttonEnv->isChecked())
            addSimple(pm->envelopeevents(),  "Envelope",       paletteView->folderEnv);
        else if (newObject->ui->buttonSiv->isChecked())
            addSimple(pm->sieveevents(),     "Sieve",          paletteView->folderSiv);
        else if (newObject->ui->buttonSpa->isChecked())
            addSimple(pm->spaevents(),       "Spatialization", paletteView->folderSpa);
        else if (newObject->ui->buttonPat->isChecked())
            addSimple(pm->patternevents(),   "Pattern",        paletteView->folderPat);
        else if (newObject->ui->buttonRev->isChecked())
            addSimple(pm->reverbevents(),    "Reverb",         paletteView->folderRev);
        else if (newObject->ui->buttonFil->isChecked())
            addSimple(pm->filterevents(),    "Filter",         paletteView->folderFil);

        if (!folder) {
            qWarning() << "insertObject: No folder found for type" << typeStr;
            delete newObject;
            newObject = nullptr;
            return;
        }

        // Create QStandardItems with proper user roles
        folder->appendRow(PVCHelper::make_child_palette_tuple(typeStr, nameStr));

        modifiedButNotSaved = true;
        MUtilities::modified();
        delete newObject;
        newObject = nullptr;
    }
}

static Eventtype eventtypeFromString(const QString& s) {
    if (s == "High")          return high;
    if (s == "Mid")           return mid;
    if (s == "Low")           return low;
    if (s == "Bottom")        return bottom;
    if (s == "Spectrum")      return sound;
    if (s == "Note")          return note;
    if (s == "Envelope")      return env;
    if (s == "Sieve")         return sieve;
    if (s == "Spatialization") return spa;
    if (s == "Pattern")       return pattern;
    if (s == "Reverb")        return reverb;
    if (s == "Filter")        return filter;
    return top; // fallback (should not happen for deletable types)
}

void ProjectView::updatePaletteView() const {
    ProjectManager *pm = Inst::get_project_manager();

    auto makeItems = [](const QString& typeStr, const QString& nameStr,
                        QStandardItem*& outType, QStandardItem*& outName) {
        outType = new QStandardItem(typeStr);
        outName = new QStandardItem(nameStr);
        outType->setFlags(outType->flags() & ~Qt::ItemIsEditable);
        outType->setData(typeStr, Qt::UserRole + 1);
        outType->setData(nameStr, Qt::UserRole + 2);
        outName->setData(typeStr, Qt::UserRole + 1);
        outName->setData(nameStr, Qt::UserRole + 2);
    };

    // Top event is singular — add directly without a loop
    {
        QStandardItem *t, *n;
        makeItems("Top", pm->topevent().name, t, n);
        t->setFlags(t->flags() & ~Qt::ItemIsDragEnabled);
        n->setFlags(n->flags() & ~Qt::ItemIsDragEnabled & ~Qt::ItemIsEditable);
        paletteView->folderTop->appendRow({t, n});
    }

    for (const HEvent& item : pm->highevents()) {
        QStandardItem *t, *n;
        makeItems("High", item.name, t, n);
        paletteView->folderHigh->appendRow({t, n});
    }

    for (const HEvent& item : pm->midevents()) {
        QStandardItem *t, *n;
        makeItems("Mid", item.name, t, n);
        paletteView->folderMid->appendRow({t, n});
    }

    for (const HEvent& item : pm->lowevents()) {
        QStandardItem *t, *n;
        makeItems("Low", item.name, t, n);
        paletteView->folderLow->appendRow({t, n});
    }

    for (const BottomEvent& item : pm->bottomevents()) {
        QStandardItem *t, *n;
        makeItems("Bottom", item.event.name, t, n);
        paletteView->folderBottom->appendRow({t, n});
    }

    for (const SpectrumEvent& item : pm->spectrumevents()) {
        QStandardItem *t, *n;
        makeItems("Spectrum", item.name, t, n);
        paletteView->folderSpectrum->appendRow({t, n});
    }

    for (const NoteEvent& item : pm->noteevents()) {
        QStandardItem *t, *n;
        makeItems("Note", item.name, t, n);
        paletteView->folderNote->appendRow({t, n});
    }

    for (const EnvelopeEvent& item : pm->envelopeevents()) {
        QStandardItem *t, *n;
        makeItems("Envelope", item.name, t, n);
        paletteView->folderEnv->appendRow({t, n});
    }

    for (const SieveEvent& item : pm->sieveevents()) {
        QStandardItem *t, *n;
        makeItems("Sieve", item.name, t, n);
        paletteView->folderSiv->appendRow({t, n});
    }

    for (const SpaEvent& item : pm->spaevents()) {
        QStandardItem *t, *n;
        makeItems("Spatialization", item.name, t, n);
        paletteView->folderSpa->appendRow({t, n});
    }

    for (const PatternEvent& item : pm->patternevents()) {
        QStandardItem *t, *n;
        makeItems("Pattern", item.name, t, n);
        paletteView->folderPat->appendRow({t, n});
    }

    for (const ReverbEvent& item : pm->reverbevents()) {
        QStandardItem *t, *n;
        makeItems("Reverb", item.name, t, n);
        paletteView->folderRev->appendRow({t, n});
    }

    for (const FilterEvent& item : pm->filterevents()) {
        QStandardItem *t, *n;
        makeItems("Filter", item.name, t, n);
        paletteView->folderFil->appendRow({t, n});
    }
}

void ProjectView::showAttributes(QString eventType, int index) {
    qDebug() << "Showing attributes for event:" << eventType << "at index" << index;

    Eventtype type = eventtypeFromString(eventType);
    if (type == top)
        eventAttributesView->showAttributesOfEvent(top, 0);
    else
        eventAttributesView->showAttributesOfEvent(type, index);

    mainWindow->ui->eventsScrollArea->widget()->adjustSize();
    mainWindow->ui->eventsScrollArea->updateGeometry();
}

void ProjectView::deleteEvent(const QString& typeStr, int index)
{
    ProjectManager* pm = Inst::get_project_manager();
    Eventtype etype = eventtypeFromString(typeStr);

    // Look up the event's name so we can find layerbox references to it.
    QString eventName;
    if      (etype == high    && index < pm->highevents().size())     eventName = pm->highevents()[index].name;
    else if (etype == mid     && index < pm->midevents().size())      eventName = pm->midevents()[index].name;
    else if (etype == low     && index < pm->lowevents().size())      eventName = pm->lowevents()[index].name;
    else if (etype == bottom  && index < pm->bottomevents().size())   eventName = pm->bottomevents()[index].event.name;
    else if (etype == sound   && index < pm->spectrumevents().size()) eventName = pm->spectrumevents()[index].name;
    else if (etype == note    && index < pm->noteevents().size())     eventName = pm->noteevents()[index].name;
    else if (etype == env     && index < pm->envelopeevents().size()) eventName = pm->envelopeevents()[index].name;
    else if (etype == sieve   && index < pm->sieveevents().size())    eventName = pm->sieveevents()[index].name;
    else if (etype == spa     && index < pm->spaevents().size())      eventName = pm->spaevents()[index].name;
    else if (etype == pattern && index < pm->patternevents().size())  eventName = pm->patternevents()[index].name;
    else if (etype == reverb  && index < pm->reverbevents().size())   eventName = pm->reverbevents()[index].name;
    else if (etype == filter  && index < pm->filterevents().size())   eventName = pm->filterevents()[index].name;

    // If any layerbox references this event by name, warn the user and require
    // explicit confirmation before cascading the delete.
    LayerRefs::Assoc stale;
    if (!eventName.isEmpty()) {
        stale = LayerRefs::collect(displayStringToEventtypeString(typeStr), eventName);
    }
    if (!stale.isEmpty()) {
        QMessageBox mb;
        mb.setIcon(QMessageBox::Warning);
        mb.setWindowTitle("Delete event?");
        mb.setText(QString("The %1 event \"%2\" is referenced in %3 layerbox child "
                           "package(s). Deleting it will also delete every reference "
                           "to it.\n\nProceed?")
            .arg(typeStr, eventName).arg(LayerRefs::totalCount(stale)));
        QPushButton* proceedBtn = mb.addButton("Delete event and references",
                                               QMessageBox::DestructiveRole);
        QPushButton* cancelBtn  = mb.addButton("Cancel",
                                               QMessageBox::RejectRole);
        mb.setDefaultButton(cancelBtn);
        mb.exec();
        if (mb.clickedButton() != proceedBtn) return;

        LayerRefs::applyDelete(stale);
    }

    // Notify the attributes view before touching the backend
    eventAttributesView->onEventDeleted(etype, index);

    // Remove from the backend list
    if      (etype == high)    pm->highevents().removeAt(index);
    else if (etype == mid)     pm->midevents().removeAt(index);
    else if (etype == low)     pm->lowevents().removeAt(index);
    else if (etype == bottom)  pm->bottomevents().removeAt(index);
    else if (etype == sound)   pm->spectrumevents().removeAt(index);
    else if (etype == note)    pm->noteevents().removeAt(index);
    else if (etype == env)     pm->envelopeevents().removeAt(index);
    else if (etype == sieve)   pm->sieveevents().removeAt(index);
    else if (etype == spa)     pm->spaevents().removeAt(index);
    else if (etype == pattern) pm->patternevents().removeAt(index);
    else if (etype == reverb)  pm->reverbevents().removeAt(index);
    else if (etype == filter)  pm->filterevents().removeAt(index);
    pm->markModified();

    // Remove from the palette model. Use the quiet variant because the
    // backend removeAt above has already synced state — letting the
    // rowsAboutToBeRemoved signal fire would double-remove from the backend.
    paletteView->removeFolderRowQuiet(typeStr, index);

    // Refresh any visible layerboxes so the cascaded-delete is reflected in UI.
    if (!stale.isEmpty()) reloadAllLayerBoxes();
}

void ProjectView::duplicateEvent(const QString& typeStr, int index)
{
    auto* pm = Inst::get_project_manager();
    if (!pm->get_curr_project() || typeStr == "Top") return;
    if (!eventAttributesView->saveCurrentShownEventData()) return;
    const auto copy = PVCHelper::copyEvent(pm, eventtypeFromString(typeStr), index);
    if (copy) insertEventCopy(*copy);
}

void ProjectView::copyEvent(const QString& typeStr, int index)
{
    auto* pm = Inst::get_project_manager();
    if (!pm->get_curr_project() || typeStr == "Top") return;
    // Capture the selected source index before this flush can re-sort its name.
    if (!eventAttributesView->saveCurrentShownEventData()) return;
    const auto copy = PVCHelper::copyEvent(pm, eventtypeFromString(typeStr), index);
    if (copy)
        ProjectClipboard::copy(pm->get_curr_project(), *copy, tr("%1 object").arg(typeStr));
}

bool ProjectView::canPasteEvent() const
{
    return ProjectClipboard::get<PaletteEventCopy>(
        Inst::get_project_manager()->get_curr_project()) != nullptr;
}

void ProjectView::pasteEvent()
{
    const auto* copy = ProjectClipboard::get<PaletteEventCopy>(
        Inst::get_project_manager()->get_curr_project());
    if (!copy) return;
    // The naming dialog can process clipboard changes; do not retain its pointer.
    const PaletteEventCopy snapshot = *copy;
    if (!eventAttributesView->saveCurrentShownEventData()) return;
    insertEventCopy(snapshot);
}

void ProjectView::insertEventCopy(const PaletteEventCopy& snapshot)
{
    auto* pm = Inst::get_project_manager();
    const QString typeStr = eventtypeToDisplayString(snapshot.type);
    QStandardItem* folder = paletteView->folderForType(typeStr);
    if (!folder || !pm->get_curr_project()) return;

    const QList<Layer>* layers = nullptr;
    if (const auto* event = std::get_if<HEvent>(&snapshot.value))
        layers = &event->event_layers;
    else if (const auto* bottomEvent = std::get_if<BottomEvent>(&snapshot.value))
        layers = &bottomEvent->event.event_layers;
    if (layers) {
        for (const Layer& layer : *layers) {
            for (const Package& package : layer.discrete_packages) {
                if (!LayerRefs::referenceExists(package)) {
                    QMessageBox::warning(mainWindow, tr("Copy Object"),
                        tr("The referenced %1/%2 no longer exists. Copy the updated object again.")
                            .arg(eventtypeToDisplayString(package.event_type.toInt()), package.event_name));
                    return;
                }
            }
        }
    }

    const QString oldName = std::visit([](const auto& event) {
        if constexpr (std::is_same_v<std::decay_t<decltype(event)>, BottomEvent>)
            return event.event.name;
        else
            return event.name;
    }, snapshot.value);

    auto nameExists = [&](const QString& name) {
        for (int i = 0; i < folder->rowCount(); ++i) {
            const auto* item = folder->child(i, 1);
            if (item && item->text() == name) return true;
        }
        return false;
    };
    QString suggestedName = oldName + "_copy";
    for (int suffix = 2; nameExists(suggestedName); ++suffix)
        suggestedName = oldName + "_copy" + QString::number(suffix);

    bool ok = false;
    QString newName = QInputDialog::getText(
        mainWindow, tr("Copy Object"),
        tr("Name the new copy of %1/%2:").arg(typeStr, oldName),
        QLineEdit::Normal, suggestedName, &ok).trimmed();
    if (!ok || newName.isEmpty()) return;
    if (nameExists(newName)) {
        QMessageBox::warning(mainWindow, tr("Copy Object"),
                             tr("Object with the same name exists."));
        return;
    }
    if (snapshot.type == bottom && !newName.startsWith('s') && !newName.startsWith('n')) {
        QMessageBox::warning(mainWindow, tr("Copy Object"),
                             tr("Bottom Event names must start with 's' or 'n'."));
        return;
    }

    PVCHelper::visitEventList(pm, snapshot.type, [&](auto& list) {
        using Event = typename std::decay_t<decltype(list)>::value_type;
        Event copy = std::get<Event>(snapshot.value);
        if constexpr (std::is_same_v<Event, BottomEvent>) {
            copy.event.name = newName;
            PVCHelper::renewModifierIds(copy.event.modifiers);
            PVCHelper::renewModifierIds(copy.extra_info.modifiers);
        } else {
            copy.name = newName;
            if constexpr (std::is_same_v<Event, HEvent>)
                PVCHelper::renewModifierIds(copy.modifiers);
        }
        list.append(copy);
    });
    // The palette's insertion callback expects the backend to be updated first.
    folder->appendRow(PVCHelper::make_child_palette_tuple(typeStr, newName));
    pm->markModified();
}

void ProjectView::updatePaletteItemName(const QString& typeStr, int index, const QString& name)
{
    if (paletteView) paletteView->updateItemName(typeStr, index, name);
}

void ProjectView::updateAttributesNameEntry(const QString& typeStr, int index, const QString& name)
{
    if (eventAttributesView) eventAttributesView->updateNameEntryIfShowing(typeStr, index, name);
}

void ProjectView::reloadAllLayerBoxes()
{
    if (eventAttributesView) eventAttributesView->reloadAllLayerBoxes();
}
