#include "VagConverter.h"
#include <QDebug>
#include <QFile>
#include <QBuffer>
#include <QtEndian>

VagConverter::VagConverter() {}

/*
 * Based on https://github.com/ColdSauce/psxsdk/blob/master/tools/vag2wav.c
 */
QByteArray VagConverter::vag2Wav(QByteArray &vag)
{
    double f[5][2] = { { 0.0, 0.0 },
                      {   60.0 / 64.0,  0.0 },
                      {  115.0 / 64.0, -52.0 / 64.0 },
                      {   98.0 / 64.0, -55.0 / 64.0 },
                      {  122.0 / 64.0, -60.0 / 64.0 } };

    double samples[28];
    char vag_name[17];
    int predict_nr, shift_factor, flags;
    int i;
    int d, s;
    static double s_1 = 0.0;
    static double s_2 = 0.0;
    int sz;
    unsigned int samp_freq;
    unsigned int data_size;

    mOffset = 0;

    fread(vag_name, sizeof(char), 4, vag);

    if(strncmp(vag_name, "VAGp", 4))
    {
        qDebug("Not VAG format. Aborting.\n");
        qDebug() << vag_name;
        return NULL;
    }

    fseek(vag, 4, SEEK_SET);
    i = fgetc(vag) << 24;
    i |= fgetc(vag) << 16;
    i |= fgetc(vag) << 8;
    i |= fgetc(vag);

    //qDebug("Version: %x\n", i);

    fseek(vag, 12, SEEK_SET);
    data_size = fgetc(vag) << 24;
    data_size |= fgetc(vag) << 16;
    data_size |= fgetc(vag) << 8;
    data_size |= fgetc(vag);

    //qDebug("Data size: %d bytes", data_size);

    fseek(vag, 32, SEEK_SET);
    fread(vag_name, sizeof(char), 16, vag);
    vag_name[16] = 0;

    //qDebug("Name: %s", vag_name);

    fseek(vag, 16, SEEK_SET);

    samp_freq = fgetc(vag) << 24;
    samp_freq |= fgetc(vag) << 16;
    samp_freq |= fgetc(vag) << 8;
    samp_freq |= fgetc(vag);

    //qDebug("Sampling frequency: %d", samp_freq);

    fseek( vag, 64, SEEK_SET );

    QByteArray pcm;

    //qDebug() << "vag offset" << mOffset;
    int vag_offset = mOffset;
    int pcm_offset = 0;
    mOffset = pcm_offset;

    // Write header chunk
    fprintf(pcm, "RIFF");
    // Skip file size field for now
    fseek(pcm, 4, SEEK_CUR);

    fprintf(pcm, "WAVE");
    // Write fmt chunk
    fprintf(pcm, "fmt ");
    // Write chunk 1 size in little endian format
    fputc(0x10, pcm);
    fputc(0, pcm);
    fputc(0, pcm);
    fputc(0, pcm);
    // Write audio format (1 = PCM)
    fputc(1, pcm);
    fputc(0, pcm);
    // Number of channels (1)
    fputc(1, pcm);
    fputc(0, pcm);
    // Write sample rate
    fputc((samp_freq & 0xff), pcm);
    fputc((samp_freq >> 8) & 0xff, pcm);
    fputc(0, pcm);
    fputc(0, pcm);
    // Write byte rate (SampleRate * NumChannels * BitsPerSample/8)
    // That would be 44100*1*(16/8), thus 88200.
    fputc(((samp_freq*2) & 0xff), pcm);
    fputc(((samp_freq*2) >> 8) & 0xff, pcm);
    fputc(((samp_freq*2) >> 16) & 0xff, pcm);
    fputc(((samp_freq*2) >> 24) & 0xff, pcm);
    // Write block align (NumChannels * BitsPerSample/8), thus 2
    fputc(2, pcm);
    fputc(0, pcm);
    // Write BitsPerSample
    fputc(16, pcm);
    fputc(0, pcm);

    // Write data chunk
    fprintf(pcm, "data");

    // Skip SubChunk2Size, we will return to it later
    fseek(pcm, 4, SEEK_CUR);

    // Now write data...

    while( vag_offset < (data_size + 48)) {
        predict_nr = vag[vag_offset++];
        shift_factor = predict_nr & 0xf;
        predict_nr >>= 4;
        flags = vag[vag_offset++];
        if ( flags == 7 )
            break;
        for ( i = 0; i < 28; i += 2 ) {
            d = vag[vag_offset++];
            s = ( d & 0xf ) << 12;
            if ( s & 0x8000 )
                s |= 0xffff0000;
            samples[i] = (double) ( s >> shift_factor  );
            s = ( d & 0xf0 ) << 8;
            if ( s & 0x8000 )
                s |= 0xffff0000;
            samples[i+1] = (double) ( s >> shift_factor  );
        }

        for ( i = 0; i < 28; i++ ) {
            samples[i] = samples[i] + s_1 * f[predict_nr][0] + s_2 * f[predict_nr][1];
            s_2 = s_1;
            s_1 = samples[i];
            d = (int) ( samples[i] + 0.5 );
            fputc( d & 0xff, pcm );
            fputc( d >> 8, pcm );
        }
    }

    sz = pcm.size();

    // Now write ChunkSize
    fseek(pcm, 4, SEEK_SET);

    fputc((sz - 8) & 0xff, pcm);
    fputc(((sz - 8)>> 8) & 0xff, pcm);
    fputc(((sz - 8)>> 16) & 0xff, pcm);
    fputc(((sz - 8)>> 24) & 0xff, pcm);

    // Now write Subchunk2Size
    fseek(pcm, 40, SEEK_SET);

    fputc((sz - 44) & 0xff, pcm);
    fputc(((sz - 44) >> 8) & 0xff, pcm);
    fputc(((sz - 44) >> 16) & 0xff, pcm);
    fputc(((sz - 44) >> 24) & 0xff, pcm);

    return pcm;
}

QByteArray VagConverter::monoVagToWav(QByteArray &vagData, QString name, int channels, int sampleRate)
{
    VagHeader vag_header = {0};

    // Build VAG header
    strcpy(vag_header.magic, "VAGp");
    vag_header.version = qFromBigEndian(3);
    vag_header.dataSize = qFromBigEndian((int)vagData.size());
    vag_header.sampleRate = qFromBigEndian(sampleRate);
    vag_header.channels = channels;
    if (name.length() > 11) {
        name = name.mid(0, 11);
    }
    strcpy(vag_header.name, QString(name + ".vag").toLocal8Bit());
    // Create VAG
    QByteArray vag = QByteArray::fromRawData((char*)&vag_header, sizeof(vag_header)).append(vagData);
    // Convert VAG to WAV
    return vag2Wav(vag);
}

QByteArray VagConverter::stereoVagToWav(QByteArray &leftChannel, QByteArray &rightChannel, QString name, int channels, int sampleRate)
{
    QByteArray wavLeft = monoVagToWav(leftChannel, name, channels, sampleRate);
    QByteArray wavRight = monoVagToWav(rightChannel, name, channels, sampleRate);
    return mergeMonoToStereo(wavLeft, wavRight, sampleRate);
}

QByteArray VagConverter::mergeMonoToStereo(const QByteArray &leftMono, const QByteArray &rightMono, int sampleRate) {
    if (leftMono.isEmpty() || rightMono.isEmpty()) {
        qWarning() << "One or both input WAV data are empty.";
        return QByteArray();
    }

    QByteArray stereoWav;
    QDataStream leftStream(leftMono);
    QDataStream rightStream(rightMono);
    leftStream.setByteOrder(QDataStream::LittleEndian);
    rightStream.setByteOrder(QDataStream::LittleEndian);

    // Read and validate WAV headers
    QByteArray leftHeader = leftMono.left(44);
    QByteArray rightHeader = rightMono.left(44);
    QByteArray leftData = leftMono.mid(44);
    QByteArray rightData = rightMono.mid(44);

    if (leftData.size() != rightData.size()) {
        qWarning() << "Mono WAV files must have the same sample count.";
        return QByteArray();
    }

    // Create stereo header
    QByteArray stereoHeader = leftHeader;
    QDataStream stereoStream(&stereoHeader, QIODevice::WriteOnly);
    stereoStream.setByteOrder(QDataStream::LittleEndian);

    // Modify the header for stereo output
    stereoStream.device()->seek(22);
    stereoStream.writeRawData("\x02", 1); // NumChannels = 2
    stereoStream.device()->seek(28);
    int byteRate = sampleRate * 2 * 16 / 8; // Stereo, 16-bit
    stereoStream.device()->seek(28);
    stereoStream << byteRate;
    stereoStream.device()->seek(32);
    quint16 blockAlign = 2 * 16 / 8; // 2 channels, 16-bit
    stereoStream << blockAlign;
    stereoStream.device()->seek(40);
    int dataSize = leftData.size() * 2;
    stereoStream << dataSize;

    stereoWav.append(stereoHeader);

    // Interleave audio data
    for (int i = 0; i < leftData.size(); i += 2) {
        stereoWav.append(leftData.mid(i, 2));  // Left channel sample
        stereoWav.append(rightData.mid(i, 2)); // Right channel sample
    }

    return stereoWav;
}

int VagConverter::fseek(QByteArray &stream, long offset, int whence)
{
    long newPos = 0;
    switch (whence) {
        case SEEK_SET: newPos = offset; break;
        case SEEK_CUR: newPos = mOffset + offset; break;
        case SEEK_END: newPos = stream.size() + offset; break;
        default: return -1;
    }
    mOffset = newPos;
    while (mOffset > stream.size()) {
        stream.append((char)0);
    }
    return 0;
}

long VagConverter::ftell(QByteArray &stream)
{
    return mOffset;
}

int VagConverter::fgetc(QByteArray &stream)
{
    if (mOffset >= stream.size()) return EOF;
    return static_cast<unsigned char>(stream[mOffset++]);
}

int VagConverter::fputc(int c, QByteArray &stream)
{
    if (mOffset >= stream.size()) {
        stream.append(static_cast<char>(c));
    } else {
        stream[mOffset] = static_cast<char>(c);
    }
    mOffset++;
    return c;
}

size_t VagConverter::fread(void *ptr, size_t size, size_t nmemb, QByteArray &stream) {
    size_t totalBytes = size * nmemb;
    if (mOffset >= stream.size()) return 0;

    size_t bytesToRead = qMin(totalBytes, static_cast<size_t>(stream.size() - mOffset));
    memcpy(ptr, stream.constData() + mOffset, bytesToRead);
    mOffset += bytesToRead;

    return bytesToRead / size;
}

void VagConverter::fprintf(QByteArray &stream, QString s)
{
    stream.append(s.toLocal8Bit());
    mOffset += s.length();
}
