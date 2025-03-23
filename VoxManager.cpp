#include "VoxManager.h"
#include "VagConverter.h"
#include <QFile>
#include <QDebug>
#include <QtEndian>
#include <QFileInfo>
#include <qjsonarray.h>
#include <qjsonobject.h>
#include <font.h>

VoxManager::VoxManager()
{
    mReady = false;
    mPlayer = nullptr;
    mChannels = 0;
    mPlayerBuffer = new QBuffer();
}

void VoxManager::set_player(QMediaPlayer *player)
{
    mPlayer = player;
    QAudioOutput *audioOutput = new QAudioOutput;
    mPlayer->setAudioOutput(audioOutput);
}

void VoxManager::open_vox_file(QString path)
{
    qDebug() << "Opening vox file" << path;
    QFile file(path.replace("file:///", ""));
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open vox file:" << path;
        return;
    }
    mVoxData = file.readAll();
    file.close();
    mVoxPath = path;
    mFileName = QFileInfo(mVoxPath).fileName();
    parse_vox_file();
}


#define GetStr(offset) ((const char*)&mVoxData.data()[offset])
/*
 * Based on https://github.com/metalgeardev/MGS1/blob/master/unDemoVox.rb by SecaProject
 */
void VoxManager::parse_vox_file()
{
    auto voxDataSize = mVoxData.size();
    QByteArray vag_data;
    QByteArray vag_data2;
    QJsonArray voiceCaptions;
    QJsonArray radioCaptions;
    QJsonArray voiceTrackChanges;
    QJsonArray radioTrackChanges;
    VagHeader vag_header = {0};
    VagConverter *vagConverter = new VagConverter();
    int trackDuration = 0;

    mSampleRate = 0;
    mChannels = 0;
    mCharas = QStringList();
    mReady = false;
    emit readyChanged(mReady);
    for (int i = 0; i < voxDataSize;) {
        auto code = GetChar(i);
        auto size = GetShort(i+1);
        switch (code) {
        case VoxCodes::Sound:
            //qDebug() << "Voice sound";
            if (size - i > 8192) {
                qDebug("Error: Unexpected vag block size of %x at offset %x", size-i, i);
                mFileName = "Error: Unexpected vag block size.";
                return;
            }
            if (mChannels < 2) {
                vag_data.append(mVoxData.mid(i+4, size-4));
            } else {
                vag_data.append(mVoxData.mid(i+4, (size-4)/2));
                vag_data2.append(mVoxData.mid(i+4+4096, (size-4)/2));
            }
            break;
        case VoxCodes::Lang1Header:
            qDebug() << "Voice header";
            switch (GetChar(i+10)) {
            case 8:
                mSampleRate = 22050;
                break;
            case 12:
                mSampleRate = 32000;
                break;
            case 16:
                mSampleRate = 44100;
                break;
            default:
                qDebug() << "Error: Unknown sample rate case" << GetChar(i+10);
                mFileName = "Error: Unexpected sample rate.";
                return;
            }
            mChannels = GetChar(i+12);
            qDebug("-> Sample rate: %dHz, %d channels", mSampleRate, mChannels);
            break;
        case VoxCodes::Lang1Captions: {
            qDebug() << "Lang1 anims/captions";
            auto trackChanges = read_lips(i, trackDuration);
            for (const QJsonValue &trackChange: trackChanges) {
                voiceTrackChanges.append(trackChange);
            }
            auto captions = read_captions(i);
            for (const QJsonValue &caption : captions) {
                voiceCaptions.append(caption);
            }
            }break;
        case VoxCodes::Lang2Header:
            qDebug() << "Radio header";
            break;
        case VoxCodes::Demo:
            qDebug() << "Demo";
            break;
        case VoxCodes::Lang2Captions: {
            qDebug() << "Lang2 anims/captions";
            auto trackChanges = read_lips(i, trackDuration);
            for (const QJsonValue &trackChange: trackChanges) {
                radioTrackChanges.append(trackChange);
            }
            auto captions = read_captions(i);
            for (const QJsonValue &caption : captions) {
                radioCaptions.append(caption);
            }
            }break;
        case VoxCodes::Unk:
            qDebug() << "Unk";
            break;
        case VoxCodes::Init:
            qDebug() << "Init";
            break;
        case VoxCodes::End:
            qDebug() << "End";
            goto end;
        default:
            qDebug() << "Error: unknown vox code" << code;
            mFileName = "Error: Unexpected vox code.";
            return;
        }
        i += size;
    }
end:
    qDebug() << "compare tracks" << radioTrackChanges.size() << voiceTrackChanges.size();
    mCaptionsLang1 = radioCaptions;
    mCaptionsLang2 = voiceCaptions;
    mTrackChangesLang1 = radioTrackChanges;
    mTrackChangesLang2 = voiceTrackChanges;
    auto voxName = QFileInfo(mVoxPath).completeBaseName();
    if (voxName.length() > 11) {
        voxName = voxName.mid(0, 11);
    }
    voxName += ".vag";
    if (mChannels < 2) {
        mWavData = vagConverter->monoVagToWav(vag_data, voxName, mChannels, mSampleRate);
    } else {
        mWavData = vagConverter->stereoVagToWav(vag_data, vag_data2, voxName, mChannels, mSampleRate);
    }
    mPlayer->stop();
    mPlayerBuffer->close();
    mPlayerBuffer = new QBuffer();
    mPlayerBuffer->setData(mWavData);
    mPlayerBuffer->open(QIODevice::ReadOnly);
    mPlayer->setSourceDevice(mPlayerBuffer);
    mReady = true;
    emit voxUpdated();
    emit readyChanged(mReady);
    mPlayer->play();
    qDebug() << "Finished opening vox file";
}

QJsonArray VoxManager::read_lips(int i, int trackDuration)
{
    auto lipsData = GetShort(i+12);
    int lastTrackStartTime = 0;
    QJsonArray trackChanges;
    int currentTrack = 1;
    for (int j = i+lipsData+4;; j++) {
        int type = GetChar(j);
        auto code = GetHighNibble(type);
        if (code == 0) {
            break;
        }
        auto value = GetLowNibble(type);
        switch (code) {
        case AnimCodes::Sleep: {
            auto time = GetChar(j+1);
            j++;
            trackDuration += time;
        }break;
        case AnimCodes::CharaName: {
            auto chara = qFromBigEndian(GetShort(j+1));
            QString name;
            switch (chara) {
            case 0x21ca: name = "SNAKE"; break;
            case 0x33af: name = "NASTASHA"; break;
            case 0x3d2c: name = "OTACON"; break;
            case 0x6588: name = "CAMPBELL"; break;
            case 0x6c22: name = "MILLER"; break;
            case 0x7982: name = "NINJA"; break;
            case 0x896: // naomi getting arrested in french version?
            case 0x7c90: name = "NOISY"; break;
            case 0x9475: name = "NAOMI"; break;
            case 0x95f2: name = "MERYL"; break;
            case 0x962c: name = "WOLF"; break;
            case 0xd78a: name = "MEYLING"; break;
            case 0xfb95: name = "JIMHOUSEMA"; break;
            default: name = "UNK"; /*qDebug("debug chara %x", chara); exit(1);*/ break;
            }
            j += 2;
            if (name != "UNK" && !mCharas.contains(name)) {
                mCharas.append(name);
            }
        }break;
        case AnimCodes::CharaTrack: {
            //qDebug("Track %d", value);
            if (lastTrackStartTime == trackDuration && trackChanges.size() > 0) {
                trackChanges.pop_back();
            }
            trackChanges.append(QJsonObject{
                {"index", currentTrack},
                {"time", (getMSTime(lastTrackStartTime)) - 500}, // -500 will ensure face will be enabled 0.5s before talking
                {"duration", getMSTime(trackDuration - lastTrackStartTime)}
            });
            lastTrackStartTime = trackDuration;
            currentTrack = value;
            }break;
        case AnimCodes::AnimEyes:
            trackDuration += value;
            break;
        case AnimCodes::AnimLips1:
            trackDuration += value;
            break;
        case AnimCodes::AnimLips2:
            trackDuration += value;
            break;
        case AnimCodes::AnimLips3:
            trackDuration += value;
            break;
        case AnimCodes::AnimLips4:
            trackDuration += value;
            break;
        default:
            qDebug("Unexpected anim code %x, code: %x, value: %x  %x", j, code, value, GetInt(j));
            break;
        }
    }
    trackChanges.append(QJsonObject{
        {"index", currentTrack},
        {"time", (getMSTime(lastTrackStartTime)) - 500},
        {"duration", getMSTime(trackDuration - lastTrackStartTime)}
    });
    return trackChanges;
}

int VoxManager::getMSTime(int time)
{
    if (mSampleRate == 22050) {
        return time * 40.5;
    }
    return time * 42;
}

QJsonArray VoxManager::read_captions(int i)
{
    QJsonArray captions;
    //auto blockSize = GetShort(i+1);
    auto captionsData = GetShort(i+14);
    int captionSize;
    int index = 0;
    for (int j = i+captionsData+4;; j += captionSize) {
        captionSize = GetInt(j);
        int captionTime = GetInt(j+4);
        int captionDuration = GetInt(j+8);
        std::string captionTextStr = std::string(GetStr(j+16));
        decodeMgsString(captionTextStr);
        auto captionText = QString::fromUtf8(captionTextStr);
        //qDebug("CAPTION %x timestamp: %d, duration: %d, text: %s",
        //       j+4, captionTime, captionDuration, captionText.toUtf8().constData());
        captions.append(QJsonObject{
            {"time", getMSTime(captionTime)},
            {"duration", getMSTime(captionDuration)},
            {"text", captionText},
        });
        index++;
        if (captionSize == 0) {
            break;
        }
    }
    return captions;
}

bool VoxManager::export_vag(QString path)
{
    qDebug() << "Export vag:" << path;
    QFile file(path.replace("file:///", ""));
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Could not open vag file for writing:" << path;
        return false;
    }
    file.write(mVagData);
    file.close();
    return true;
}

bool VoxManager::export_wav(QString path)
{
    qDebug() << "Export wav:" << path;
    QFile file(path.replace("file:///", ""));
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Could not open wav file for writing:" << path;
        return false;
    }
    file.write(mWavData);
    file.close();
    return true;
}

bool VoxManager::export_srt(QString path)
{
    qDebug() << "Export .srt:" << path;
    QFile file(path.replace("file:///", ""));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning("Failed to open file for writing: %s", qUtf8Printable(path));
        return false;
    }
    auto captions = mCaptionsLang2;
    QTextStream out(&file);
    for (int i = 0; i < captions.size(); ++i) {
        QJsonObject obj = captions[i].toObject();
        int time = obj["time"].toInt();
        int duration = obj["duration"].toInt();
        QString text = obj["text"].toString();
        QString startTime = msTimeToTimestamp(time);
        QString endTime = msTimeToTimestamp(time + duration);
        out << i + 1 << "\n";
        out << startTime << " --> " << endTime << "\n";
        out << text << "\n\n";
    }
    file.close();
    return true;
}

QString VoxManager::msTimeToTimestamp(int ms) {
    int hours = ms / 3600000;
    int minutes = (ms % 3600000) / 60000;
    int seconds = (ms % 60000) / 1000;
    int milliseconds = ms % 1000;
    return QString("%1:%2:%3,%4")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'))
        .arg(milliseconds, 3, 10, QChar('0'));
}
