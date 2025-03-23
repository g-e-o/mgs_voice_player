#ifndef VOXMANAGER_H
#define VOXMANAGER_H

#include <QString>
#include <QBuffer>
#include <QWidget>
#include <QtMultimedia/QAudioOutput>
#include <QtMultimedia/QMediaPlayer>
#include <qjsonarray.h>

#define GetChar(offset)  ((unsigned char)mVoxData[offset])
#define GetShort(offset) (*(unsigned short*)&mVoxData.data()[offset])
#define GetInt(offset)   (*(unsigned int*)&mVoxData.data()[offset])

#define GetHighNibble(byte) ((byte >> 4) & 0xF)
#define GetLowNibble(byte) ((byte >> 0) & 0xF)

typedef enum
{
    Sound = 1,
    Lang1Header,
    Lang1Captions,
    Lang2Header,
    Demo,
    Lang2Captions,
    Unk,
    Init = 16,
    End = 240
} VoxCodes;

typedef enum
{
    Sleep = 1,
    CharaName = 2,
    CharaTrack = 4,
    AnimEyes = 8,
    AnimLips1 = 9,
    AnimLips2 = 10,
    AnimLips3 = 11,
    AnimLips4 = 12
} AnimCodes;

class VoxManager : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int sampleRate MEMBER mSampleRate NOTIFY voxUpdated)
    Q_PROPERTY(int channels MEMBER mChannels NOTIFY voxUpdated)
    Q_PROPERTY(bool isReady MEMBER mReady NOTIFY readyChanged)
    Q_PROPERTY(QBuffer *playerBuffer MEMBER mPlayerBuffer NOTIFY voxUpdated)
    Q_PROPERTY(QString fileName MEMBER mFileName NOTIFY voxUpdated)
    Q_PROPERTY(QJsonArray captionsLang1 MEMBER mCaptionsLang1 NOTIFY voxUpdated)
    Q_PROPERTY(QJsonArray captionsLang2 MEMBER mCaptionsLang2 NOTIFY voxUpdated)
    Q_PROPERTY(QJsonArray trackChangesLang1 MEMBER mTrackChangesLang1 NOTIFY voxUpdated)
    Q_PROPERTY(QJsonArray trackChangesLang2 MEMBER mTrackChangesLang2 NOTIFY voxUpdated)
    Q_PROPERTY(QStringList charas MEMBER mCharas NOTIFY voxUpdated)
public:
    VoxManager();
    Q_INVOKABLE void open_vox_file(QString path);
    Q_INVOKABLE bool export_vag(QString path);
    Q_INVOKABLE bool export_wav(QString path);
    Q_INVOKABLE bool export_srt(QString path);
    Q_INVOKABLE void set_player(QMediaPlayer *player);

private:
    bool mReady;
    QMediaPlayer *mPlayer;
    QBuffer *mPlayerBuffer;
    int mSampleRate;
    int mChannels;
    QString mVoxPath;
    QString mFileName;
    QByteArray mVoxData;
    QByteArray mVagData;
    QByteArray mWavData;
    QJsonArray mCaptionsLang1;
    QJsonArray mCaptionsLang2;
    QJsonArray mTrackChangesLang1;
    QJsonArray mTrackChangesLang2;
    QStringList mCharas;

    void parse_vox_file();
    QJsonArray read_lips(int i, int trackDuration);
    QJsonArray read_captions(int i);
    int getMSTime(int time);
    QString msTimeToTimestamp(int ms);

signals:
    void voxUpdated();
    void readyChanged(bool isReady);
    void isLoadingChanged(bool isLoading);
};

#endif // VOXMANAGER_H
