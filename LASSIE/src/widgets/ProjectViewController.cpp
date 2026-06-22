/* The 'project' object that keeps track of project details to deploy
 * in the associated window (currently, the project view).
 *
 * Copyright (c) 2025, DISSCO authors */

#include <QFile>
#include <QString>
#include <QDir>
#include <QFileInfo>
#include <QIODevice>
#include <QXmlStreamWriter>
#include <QDebug>
#include <QTextStream>
#include <QDomDocument>

#include <QDialog>
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

namespace PVCHelper {
    QList<QStandardItem*> make_child_palette_tuple(const QString& type, const QString& name) {
        auto* typeItem = new QStandardItem(type);
        auto* nameItem = new QStandardItem(name);
        typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
        typeItem->setData(type, Qt::UserRole + 1);
        typeItem->setData(name, Qt::UserRole + 2);
        nameItem->setData(type, Qt::UserRole + 1);
        nameItem->setData(name, Qt::UserRole + 2);
        return {typeItem, nameItem};
    }
}
/* ProjectView constructor initializing values for XML file*/
ProjectView::ProjectView(MainWindow* _mainWindow, QString _pathAndName) {

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
    QDomDocument tempDoc;
    QString wrappedXml = QString("<root>%1</root>").arg(xmlString.trimmed());
    if (tempDoc.setContent(wrappedXml)) {
        QDomNodeList children = tempDoc.documentElement().childNodes();
        for (int i = 0; i < children.count(); ++i) {
            QDomNode node = children.at(i);
            if (node.isElement()) {
                // Write raw XML
                QString rawXml;
                QTextStream stream(&rawXml);
                node.save(stream, 0);
                rawXml.remove(QRegularExpression("[\\n\\t\\r]+"));
                rawXml.replace(QRegularExpression(">\\s+<"), "><");
                xmlWriter.writeCharacters("");
                xmlWriter.device()->write(rawXml.toUtf8());
            } else {
                // Write plain text (like "1 - ") normally so it's safely escaped
                xmlWriter.writeCharacters(node.nodeValue());
            }
        }
        // QString str = tempDoc.toString();
        // str.remove(QRegularExpression("[\\n\\t\\r]+"));
        // str.replace(QRegularExpression(">\\s+<"), "><");
        // xmlWriter.writeCharacters("");
        // xmlWriter.device()->write(str.toUtf8());
    } else {
        // String/Number 
        xmlWriter.writeCharacters(xmlString);
    }
}

// Returns the modifier field value only if it is active for the modifier's type;
// returns an empty string for fields disabled by the type, so CMOD ignores them.
static QString modActiveField(const Modifier& mod, int fieldIdx) {
    // columns: prob(0), amp(1), rate(2), width(3), spread(4), dir(5), vel(6)
    static const bool table[7][7] = {
        /* TREMOLO  */ { true,  true,  true,  false, false, false, false },
        /* VIBRATO  */ { true,  true,  true,  false, false, false, false },
        /* GLISSANDO*/ { true,  true,  false, false, false, false, false },
        /* DETUNE   */ { true,  false, false, false, true,  true,  true  },
        /* AMPTRANS */ { true,  true,  true,  true,  false, false, false },
        /* FREQTRANS*/ { true,  true,  true,  true,  false, false, false },
        /* WAVE_TYPE*/ { false, true,  false, false, false, false, false },
    };
    if (mod.type >= 7 || !table[mod.type][fieldIdx]) return {};
    switch (fieldIdx) {
        case 0: return mod.probability;
        case 1: return mod.amplitude;
        case 2: return mod.rate;
        case 3: return mod.width;
        case 4: return mod.detune_spread;
        case 5: return mod.detune_direction;
        case 6: return mod.detune_velocity;
        default: return {};
    }
}

/* Function that creates and saves the xml .dissco file */
void ProjectView::save(){
    qDebug() << "In Project View Save Function";

    eventAttributesView->saveCurrentShownEventData();

    ProjectManager *pm = Inst::get_project_manager();
    modifiedButNotSaved = false; // changes bool value because file is being saved

    // ensure directory exists before creating file
    QFileInfo fileInfo = pm->fileinfo();
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qDebug() << "Failed to create directory:" << dir.absolutePath();
            return;
        }
    }

    // creates the file with the specified /path/name.dissco
    QFile file(pm->fileinfo().absoluteFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file:" << file.fileName();
        qDebug() << "Error reason:" << file.errorString();
        return;
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
                    while (libSeg != NULL){
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
                    }

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
                    //xmlWriter.writeCharacters("");
                    for (const Modifier& itemMod : item.modifiers) {
                        xmlWriter.writeStartElement("Modifier");
                            xmlWriter.writeStartElement("Type");
                                xmlWriter.writeCharacters(QString("%1").arg(itemMod.type));
                            xmlWriter.writeEndElement();
                            xmlWriter.writeStartElement("ApplyHow");
                                xmlWriter.writeCharacters(itemMod.applyhow_flag ? "0" : "1");
                            xmlWriter.writeEndElement();
                            xmlWriter.writeStartElement("Probability");
                                writeInlineXml(xmlWriter, modActiveField(itemMod, 0));
                            xmlWriter.writeEndElement();
                            xmlWriter.writeStartElement("Amplitude");
                                writeInlineXml(xmlWriter, modActiveField(itemMod, 1));
                            xmlWriter.writeEndElement();
                            xmlWriter.writeStartElement("Rate");
                                writeInlineXml(xmlWriter, modActiveField(itemMod, 2));
                            xmlWriter.writeEndElement();
                            xmlWriter.writeStartElement("Width");
                                writeInlineXml(xmlWriter, modActiveField(itemMod, 3));
                            xmlWriter.writeEndElement();
                            xmlWriter.writeStartElement("DetuneSpread");
                                writeInlineXml(xmlWriter, modActiveField(itemMod, 4));
                            xmlWriter.writeEndElement();
                            xmlWriter.writeStartElement("DetuneDirection");
                                writeInlineXml(xmlWriter, modActiveField(itemMod, 5));
                            xmlWriter.writeEndElement();
                            xmlWriter.writeStartElement("DetuneVelocity");
                                writeInlineXml(xmlWriter, modActiveField(itemMod, 6));
                            xmlWriter.writeEndElement();
                            xmlWriter.writeStartElement("GroupName");
                                writeInlineXml(xmlWriter, itemMod.group_name);
                            xmlWriter.writeEndElement();
                            xmlWriter.writeStartElement("PartialResultString");
                                writeInlineXml(xmlWriter, itemMod.applyhow_flag ? itemMod.partialresult_string : QString{});
                            xmlWriter.writeEndElement();
                        xmlWriter.writeEndElement();
                    }
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
                    xmlWriter.writeStartElement("ExtraInfo");
                        xmlWriter.writeStartElement("FrequencyInfo");
                            xmlWriter.writeStartElement("FrequencyFlag");
                                xmlWriter.writeCharacters(QString("%1").arg(item.extra_info.freq_info.freq_flag));
                            xmlWriter.writeEndElement();
                            xmlWriter.writeStartElement("FrequencyContinuumFlag");
                                xmlWriter.writeCharacters(QString("%1").arg(item.extra_info.freq_info.continuum_flag));
                            xmlWriter.writeEndElement(); 
                            xmlWriter.writeStartElement("FrequencyEntry1");
                                writeInlineXml(xmlWriter, item.extra_info.freq_info.entry_1);
                            xmlWriter.writeEndElement(); 
                            xmlWriter.writeStartElement("FrequencyEntry2");
                                writeInlineXml(xmlWriter, item.extra_info.freq_info.entry_2);
                            xmlWriter.writeEndElement(); 
                        xmlWriter.writeEndElement(); 
                        xmlWriter.writeStartElement("Loudness");
                            writeInlineXml(xmlWriter, item.extra_info.loudness);
                        xmlWriter.writeEndElement(); 
                        xmlWriter.writeStartElement("Spatialization");
                            writeInlineXml(xmlWriter, item.extra_info.spa);
                        xmlWriter.writeEndElement();
                        xmlWriter.writeStartElement("Reverb");
                            writeInlineXml(xmlWriter, item.extra_info.reverb);
                        xmlWriter.writeEndElement(); 
                        xmlWriter.writeStartElement("Filter");
                            writeInlineXml(xmlWriter, item.extra_info.filter);
                        xmlWriter.writeEndElement(); 
                        xmlWriter.writeStartElement("ModifierGroup");
                            writeInlineXml(xmlWriter, item.extra_info.modifier_group);
                        xmlWriter.writeEndElement(); 
                        xmlWriter.writeStartElement("Modifiers");
                        for (const Modifier& itemMod : item.extra_info.modifiers) {
                            xmlWriter.writeStartElement("Modifier");
                                xmlWriter.writeStartElement("Type");
                                    xmlWriter.writeCharacters(QString("%1").arg(itemMod.type));
                                xmlWriter.writeEndElement();
                                xmlWriter.writeStartElement("ApplyHow");
                                    xmlWriter.writeCharacters(itemMod.applyhow_flag ? "0" : "1");
                                xmlWriter.writeEndElement();
                                xmlWriter.writeStartElement("Probability");
                                    writeInlineXml(xmlWriter, modActiveField(itemMod, 0));
                                xmlWriter.writeEndElement();
                                xmlWriter.writeStartElement("Amplitude");
                                    writeInlineXml(xmlWriter, modActiveField(itemMod, 1));
                                xmlWriter.writeEndElement();
                                xmlWriter.writeStartElement("Rate");
                                    writeInlineXml(xmlWriter, modActiveField(itemMod, 2));
                                xmlWriter.writeEndElement();
                                xmlWriter.writeStartElement("Width");
                                    writeInlineXml(xmlWriter, modActiveField(itemMod, 3));
                                xmlWriter.writeEndElement();
                                xmlWriter.writeStartElement("DetuneSpread");
                                    writeInlineXml(xmlWriter, modActiveField(itemMod, 4));
                                xmlWriter.writeEndElement();
                                xmlWriter.writeStartElement("DetuneDirection");
                                    writeInlineXml(xmlWriter, modActiveField(itemMod, 5));
                                xmlWriter.writeEndElement();
                                xmlWriter.writeStartElement("DetuneVelocity");
                                    writeInlineXml(xmlWriter, modActiveField(itemMod, 6));
                                xmlWriter.writeEndElement();
                                xmlWriter.writeStartElement("GroupName");
                                    writeInlineXml(xmlWriter, itemMod.group_name);
                                xmlWriter.writeEndElement();
                                xmlWriter.writeStartElement("PartialResultString");
                                    writeInlineXml(xmlWriter, itemMod.applyhow_flag ? itemMod.partialresult_string : QString{});
                                xmlWriter.writeEndElement();
                            xmlWriter.writeEndElement();
                        }
                        xmlWriter.writeEndElement();
                    xmlWriter.writeEndElement(); 
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
    file.close();
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
        QString new_title = projectPropertiesDialog->ui->titleEntry->text();
        pm->fileflag() = projectPropertiesDialog->ui->flagEntry->text();
        pm->numchannels() = projectPropertiesDialog->ui->numChannelsEntry->text();
        pm->samplerate() = projectPropertiesDialog->ui->rateEntry->text();
        pm->samplesize() = projectPropertiesDialog->ui->sizeEntry->text();
        pm->numthreads() = projectPropertiesDialog->ui->numThreadsEntry->text();
        pm->synthesis() = projectPropertiesDialog->ui->synthesisCheckBox->isChecked();
        pm->score() = projectPropertiesDialog->ui->scoreCheckBox->isChecked();
        pm->grandstaff() = projectPropertiesDialog->ui->staffCheckBox->isChecked();
        pm->numstaffs() = projectPropertiesDialog->ui->numStaffEntry->text();
        pm->outputparticel() = projectPropertiesDialog->ui->particelBox->isChecked();
        QString new_topevent = projectPropertiesDialog->ui->topEventEntry->text();
        pm->duration() = projectPropertiesDialog->ui->durationEntry->text();

        if (new_title != pm->title()) {
            QString old_pathAndName = pm->fileinfo().absoluteFilePath();
            QString new_pathAndName = pm->fileinfo().absolutePath() + "/" + new_title + ".dissco";
            if (QFile::rename(old_pathAndName, new_pathAndName)) {
                pm->title() = new_title;
            }
        }

        MUtilities::modified();
        delete projectPropertiesDialog;
        projectPropertiesDialog = NULL;
    }
}

void ProjectView::propertiesInsertFunction() {
    if (!projectPropertiesDialog) return;
    ProjectManager *pm = Inst::get_project_manager();
    auto* generator = new FunctionGenerator(mainWindow, functionReturnFloat, pm->duration());
    if (generator->exec() == QDialog::Accepted) {
        QString result = generator->getResultString();
        if (!result.isEmpty()) {
            pm->duration() = result;
            projectPropertiesDialog->ui->durationEntry->setText(pm->duration());
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

    // Remove from the palette model. Use the quiet variant because the
    // backend removeAt above has already synced state — letting the
    // rowsAboutToBeRemoved signal fire would double-remove from the backend.
    paletteView->removeFolderRowQuiet(typeStr, index);

    // Refresh any visible layerboxes so the cascaded-delete is reflected in UI.
    if (!stale.isEmpty()) reloadAllLayerBoxes();
}

void ProjectView::duplicateEvent(const QString& typeStr, int index)
{
    ProjectManager* pm = Inst::get_project_manager();

    QStandardItem* folder = paletteView->folderForType(typeStr);
    if (!folder) return;

    // Copies element at index, appends " (copy)" to .name, appends to list, returns new name
    auto dup = [index](auto& list) -> QString {
        auto copy = list[index];
        copy.name += " (copy)";
        list.append(copy);
        return copy.name;
    };

    Eventtype etype = eventtypeFromString(typeStr);
    QString newName;
    if      (etype == high)    newName = dup(pm->highevents());
    else if (etype == mid)     newName = dup(pm->midevents());
    else if (etype == low)     newName = dup(pm->lowevents());
    else if (etype == bottom) {
        BottomEvent copy = pm->bottomevents()[index];
        copy.event.name += " (copy)";
        newName = copy.event.name;
        pm->bottomevents().append(copy);
    }
    else if (etype == sound)   newName = dup(pm->spectrumevents());
    else if (etype == note)    newName = dup(pm->noteevents());
    else if (etype == env)     newName = dup(pm->envelopeevents());
    else if (etype == sieve)   newName = dup(pm->sieveevents());
    else if (etype == spa)     newName = dup(pm->spaevents());
    else if (etype == pattern) newName = dup(pm->patternevents());
    else if (etype == reverb)  newName = dup(pm->reverbevents());
    else if (etype == filter)  newName = dup(pm->filterevents());
    else return;

    folder->appendRow(PVCHelper::make_child_palette_tuple(typeStr, newName));
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
