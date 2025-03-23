#ifndef VAGCONVERTER_H
#define VAGCONVERTER_H

#include <qstringview.h>

typedef struct {
    char     magic[4];
    uint32_t version;
    uint32_t reserved;
    uint32_t dataSize;
    uint32_t sampleRate;
    char     reserved2[10];
    uint16_t channels;
    char     name[16];
    uint32_t reserved3[4];
} VagHeader;

class VagConverter
{
public:
    VagConverter();
    QByteArray vag2Wav(QByteArray &vag);
    QByteArray monoVagToWav(QByteArray &vagData, QString name, int channels, int sampleRate);
    QByteArray stereoVagToWav(QByteArray &leftChannel, QByteArray &rightChannel, QString name, int channels, int sampleRate);
    QByteArray mergeMonoToStereo(const QByteArray &leftMono, const QByteArray &rightMono, int sampleRate);
private:
    int fseek(QByteArray &stream, long offset, int whence);
    long ftell(QByteArray &stream);
    int fgetc(QByteArray &stream);
    int fputc(int c, QByteArray &stream);
    size_t fread(void *ptr, size_t size, size_t nmemb, QByteArray &stream);
    void fprintf(QByteArray &stream, QString s);

    int mOffset;
};

#endif // VAGCONVERTER_H
