/*

Pencil - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation;

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/
#include <QtDebug>
#include <QMediaPlayer>
#include "object.h"
#include "layersound.h"


LayerSound::LayerSound(Object* object) : LayerImage(object)
{
    m_eType = Layer::SOUND;
    name = QString(tr("Sound Layer"));
}

LayerSound::~LayerSound()
{
    while (!sound.empty())
        delete sound.takeFirst();
}


void LayerSound::paintImages(QPainter& painter, TimeLineCells* cells, int x, int y, int width, int height, bool selected, int frameSize)
{
    Q_UNUSED(cells);
    Q_UNUSED(width);
    Q_UNUSED(selected);

    for (int i = 0; i < sound.size(); i++)
    {
        qreal h = x + (framesPosition.at(i)-1)*frameSize+2;
        if (framesSelected.at(i))
        {
            painter.setBrush(QColor(60,60,60));
            h = h + frameOffset*frameSize;
        }
        else
        {
            painter.setBrush(QColor(125,125,125));
        }
        QPointF points[3] = { QPointF(h, y+4), QPointF(h, y+height-4), QPointF(h+15, y+0.5*height) };
        painter.drawPolygon( points, 3 );
        painter.drawText(QPoint( h + 20, y+(2*height)/3), framesFilename.at(i) );
    }
}

bool LayerSound::addImageAtFrame(int frameNumber)
{
    int index = getIndexAtFrame(frameNumber);
    if (index == -1)
    {
        sound.append(NULL);
        soundFilepath.append("");
        framesPosition.append(frameNumber);
        framesSelected.append(false);
        framesFilename.append("");
        framesModified.append(false);
        soundSize.append(0);
        bubbleSort();
        return true;
    }
    else
    {
        return false;
    }
}

void LayerSound::removeImageAtFrame(int frameNumber)
{
    int index = getIndexAtFrame(frameNumber);
    if (index != -1  && framesPosition.size() != 0)
    {
        delete sound.at(index);

        soundFilepath.removeAt(index);
        framesPosition.removeAt(index);
        framesSelected.removeAt(index);
        framesFilename.removeAt(index);
        framesModified.removeAt(index);
        soundSize.removeAt(index);
        bubbleSort();
    }
}

void LayerSound::loadSoundAtFrame(QString filePathString, int frameNumber)
{
    int index = getIndexAtFrame(frameNumber);
    if (index == -1)
        addImageAtFrame(frameNumber);
    index = getIndexAtFrame(frameNumber);

    QFileInfo fi(filePathString);
    if (fi.exists())
    {
        //Phonon::MediaObject* media = new Phonon::MediaObject();
        //connect(media, SIGNAL(totalTimeChanged(qint64)), this, SLOT(addTimelineKey(qint64)));
        //media->setCurrentSource(filePathString);
        QMediaPlayer* pPlayer = new QMediaPlayer( this );
        pPlayer->setMedia( QUrl::fromLocalFile(filePathString) );

        // quick and dirty trick to calculate soundSize
        // totalTime() return a value only after a call to media.play()
        //  ( and when signal totaltimechanged is emitted totalTime() returns the correct value )
        
        sound[index] = pPlayer;
        soundFilepath[index] = filePathString;
        framesFilename[index] = fi.fileName();
        framesModified[index] = true;
    }
    else
    {
        sound[index] = NULL;
        soundFilepath[index] = tr("Wrong file");
        framesFilename[index] = tr("Wrong file") + filePathString;
    }
}

void LayerSound::swap(int i, int j)
{
    LayerImage::swap(i, j);
    sound.swap(i, j);
    soundFilepath.swap(i,j);
}


bool LayerSound::saveImage(int index, QString path, int layerNumber)
{
    Q_UNUSED(layerNumber);
    
    QFile originalFile( soundFilepath.at(index) );
    originalFile.copy( path + "/" + framesFilename.at(index) );
    framesModified[index] = false;

    return true;
}

void LayerSound::playSound(int frame, int fps)
{
    for (int i = 0; i < sound.size(); ++i)
    {
        QMediaPlayer* media = sound.at(i);
        if (media != NULL && visible)
        {
            int position = framesPosition.at(i);
            if (frame < position)
            {
                media->stop();
            }
            else
            {
                int offsetInMs = floor((frame - position) * float(1000) / fps);
                if (media->state() == QMediaPlayer::PlayingState)
                {
                    if (fabs((float)media->position() - offsetInMs) > 500.0f)
                        media->setPosition(offsetInMs);
                }
                else
                {
                    if (frame > position)
                    {
                        media->pause();
                        media->setPosition(offsetInMs);
                    }
                    if (offsetInMs < soundSize[i])
                    {                        
                        media->play();
                    }
                }
            }
        }
    }
}



void LayerSound::stopSound()
{
    for(int i=0; i < sound.size(); i++)
    {
        Q_ASSERT( sound[i] );
        sound[i]->stop();
    }
}


QDomElement LayerSound::createDomElement(QDomDocument& doc)
{
    QDomElement layerTag = doc.createElement("layer");
    layerTag.setAttribute("id",id);
    layerTag.setAttribute("name", name);
    layerTag.setAttribute("visibility", visible);
    layerTag.setAttribute("type", type());
    for (int index=0; index < framesPosition.size() ; index++)
    {
        QDomElement soundTag = doc.createElement("sound");
        soundTag.setAttribute("position", framesPosition.at(index));
        soundTag.setAttribute("src", framesFilename.at(index));
        layerTag.appendChild(soundTag);
    }
    return layerTag;
}

void LayerSound::loadDomElement(QDomElement element, QString dataDirPath)
{
    if (!element.attribute("id").isNull()) id = element.attribute("id").toInt();
    name = element.attribute("name");
    visible = (element.attribute("visibility") == "1");
    m_eType = static_cast<LAYER_TYPE>( element.attribute("type").toInt() );

    QDomNode soundTag = element.firstChild();
    while (!soundTag.isNull())
    {
        QDomElement soundElement = soundTag.toElement();
        if (!soundElement.isNull())
        {
            if (soundElement.tagName() == "sound")
            {
                QString path = dataDirPath + "/" + soundElement.attribute("src"); // the file is supposed to be in the data directory
     //qDebug() << "LAY_SOUND  dataDirPath=" << dataDirPath << "   ;path=" << path;  //added for debugging puproses
                QFileInfo fi(path);
                if (!fi.exists()) path = soundElement.attribute("src");
                int position = soundElement.attribute("position").toInt();
                loadSoundAtFrame( path, position );
            }
        }
        soundTag = soundTag.nextSibling();
    }
}
