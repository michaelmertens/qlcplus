/*
  Q Light Controller
  midiplugin.cpp

  Copyright (c) Heikki Junnila

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <QDebug>

#include "configuremidiplugin.h"
#include "midioutputdevice.h"
#include "midiinputdevice.h"
#include "midienumerator.h"
#include "midiprotocol.h"
#include "midiplugin.h"

/*****************************************************************************
 * Initialization
 *****************************************************************************/

void MidiPlugin::init()
{
    qDebug() << Q_FUNC_INFO;

    m_enumerator = new MidiEnumerator(this);
    connect(m_enumerator, SIGNAL(configurationChanged()),
            this, SIGNAL(configurationChanged()));
    m_enumerator->rescan();
}

MidiPlugin::~MidiPlugin()
{
    qDebug() << Q_FUNC_INFO;

    Q_ASSERT(m_enumerator != NULL);
    delete m_enumerator;
}

QString MidiPlugin::name()
{
    return QString("MIDI");
}

int MidiPlugin::capabilities() const
{
    return QLCIOPlugin::Output | QLCIOPlugin::Input | QLCIOPlugin::Feedback;
}

/*****************************************************************************
 * Outputs
 *****************************************************************************/

void MidiPlugin::openOutput(quint32 output)
{
    qDebug() << "MIDI plugin open output: " << output;

    MidiOutputDevice* dev = outputDevice(output);
    if (dev != NULL)
        dev->open();
}

void MidiPlugin::closeOutput(quint32 output)
{
    qDebug() << Q_FUNC_INFO;

    MidiOutputDevice* dev = outputDevice(output);
    if (dev != NULL)
        dev->close();
}

QStringList MidiPlugin::outputs()
{
    //qDebug() << Q_FUNC_INFO;

    QStringList list;
    int i = 1;

    QListIterator <MidiOutputDevice*> it(m_enumerator->outputDevices());
    while (it.hasNext() == true)
        list << QString("%1: %2").arg(i++).arg(it.next()->name());

    return list;
}

QString MidiPlugin::pluginInfo()
{
    QString str;

    str += QString("<HTML>");
    str += QString("<HEAD>");
    str += QString("<TITLE>%1</TITLE>").arg(name());
    str += QString("</HEAD>");
    str += QString("<BODY>");

    str += QString("<P>");
    str += QString("<H3>%1</H3>").arg(name());
    str += tr("This plugin provides input/output support for MIDI devices.");
    str += QString("</P>");

    return str;
}

QString MidiPlugin::outputInfo(quint32 output)
{
    qDebug() << Q_FUNC_INFO;

    QString str;

    if (output == QLCIOPlugin::invalidLine())
    {
        str += QString("<BR><B>%1</B>").arg(tr("No output support available."));
        return str;
    }

    MidiOutputDevice* dev = outputDevice(output);
    if (dev != NULL)
    {
        QString status;
        str += QString("<H3>%1 %2</H3>").arg(tr("Output")).arg(outputs()[output]);
        str += QString("<P>");
        if (dev->isOpen() == true)
            status = tr("Open");
        else
            status = tr("Not Open");
        str += QString("%1: %2").arg(tr("Status")).arg(status);
        str += QString("</P>");
    }
    else
    {
        if (output < (quint32)outputs().length())
            str += QString("<H3>%1 %2</H3>").arg(tr("Invalid Output")).arg(outputs()[output]);
    }

    str += QString("</BODY>");
    str += QString("</HTML>");

    return str;
}

void MidiPlugin::writeUniverse(quint32 output, const QByteArray& universe)
{
    MidiOutputDevice* dev = outputDevice(output);
    if (dev != NULL)
        dev->writeUniverse(universe);
}

MidiOutputDevice* MidiPlugin::outputDevice(quint32 output) const
{
    if (output < quint32(m_enumerator->outputDevices().size()))
        return m_enumerator->outputDevices().at(output);
    else
        return NULL;
}

/*****************************************************************************
 * Inputs
 *****************************************************************************/

void MidiPlugin::openInput(quint32 input)
{
    qDebug() << "MIDI Plugin open Input: " << input;

    MidiInputDevice* dev = inputDevice(input);
    if (dev != NULL && dev->isOpen() == false)
    {
        dev->open();
        connect(dev, SIGNAL(valueChanged(QVariant,ushort,uchar)),
                this, SLOT(slotValueChanged(QVariant,ushort,uchar)));
    }
}

void MidiPlugin::closeInput(quint32 input)
{
    qDebug() << Q_FUNC_INFO;

    MidiInputDevice* dev = inputDevice(input);
    if (dev != NULL && dev->isOpen() == true)
    {
        dev->close();
        disconnect(dev, SIGNAL(valueChanged(QVariant,ushort,uchar)),
                   this, SLOT(slotValueChanged(QVariant,ushort,uchar)));
    }
}

QStringList MidiPlugin::inputs()
{
    //qDebug() << Q_FUNC_INFO;

    QStringList list;
    int i = 1;

    QListIterator <MidiInputDevice*> it(m_enumerator->inputDevices());
    while (it.hasNext() == true)
        list << QString("%1: %2").arg(i++).arg(it.next()->name());

    return list;
}

QString MidiPlugin::inputInfo(quint32 input)
{
    qDebug() << Q_FUNC_INFO;

    QString str;
/*
    str += QString("<HTML>");
    str += QString("<HEAD>");
    str += QString("<TITLE>%1</TITLE>").arg(name());
    str += QString("</HEAD>");
    str += QString("<BODY>");
*/
    if (input == QLCIOPlugin::invalidLine())
    {
        str += QString("<BR><B>%1</B>").arg(tr("No input support available."));
        return str;
    }

    MidiInputDevice* dev = inputDevice(input);
    if (dev != NULL)
    {
        QString status;
        str += QString("<H3>%1 %2</H3>").arg(tr("Input")).arg(inputs()[input]);
        str += QString("<P>");
        if (dev->isOpen() == true)
            status = tr("Open");
        else
            status = tr("Not Open");
        str += QString("%1: %2").arg(tr("Status")).arg(status);
        str += QString("</P>");
    }
    else
    {
        if (input < (quint32)inputs().length())
            str += QString("<H3>%1 %2</H3>").arg(tr("Invalid Input")).arg(inputs()[input]);
    }

    str += QString("</BODY>");
    str += QString("</HTML>");

    return str;
}

void MidiPlugin::sendFeedBack(quint32 output, quint32 channel, uchar value, const QString &)
{
    MidiOutputDevice* dev = outputDevice(output);
    if (dev != NULL)
    {
        qDebug() << "[sendFeedBack] Channel:" << channel << ", value:" << value;
        uchar cmd = 0;
        uchar data1 = 0, data2 = 0;
        bool data2valid = false;
        if (QLCMIDIProtocol::feedbackToMidi(channel, value, dev->midiChannel(),
                                        &cmd, &data1, &data2, &data2valid) == true)
        {
            qDebug() << Q_FUNC_INFO << "cmd:" << cmd << "data1:" << data1 << "data2:" << data2;
            dev->writeFeedback(cmd, data1, data2);
        }
    }
}

MidiInputDevice* MidiPlugin::inputDevice(quint32 input) const
{
    if (input < quint32(m_enumerator->inputDevices().size()))
        return m_enumerator->inputDevices().at(input);
    else
        return NULL;
}

void MidiPlugin::slotValueChanged(const QVariant& uid, ushort channel, uchar value)
{
    for (int i = 0; i < m_enumerator->inputDevices().size(); i++)
    {
        MidiInputDevice* dev = m_enumerator->inputDevices().at(i);
        if (dev->uid() == uid)
        {
            emit valueChanged(i, channel, value);
            break;
        }
    }
}

/*****************************************************************************
 * Configuration
 *****************************************************************************/

void MidiPlugin::configure()
{
    qDebug() << Q_FUNC_INFO;
    ConfigureMidiPlugin cmp(this);
    cmp.exec();
}

bool MidiPlugin::canConfigure()
{
    qDebug() << Q_FUNC_INFO;
    return true;
}

/*****************************************************************************
 * Plugin export
 *****************************************************************************/
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
Q_EXPORT_PLUGIN2(midiplugin, MidiPlugin)
#endif
