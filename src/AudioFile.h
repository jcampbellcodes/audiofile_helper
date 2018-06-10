#ifndef af_file
#define af_file
#include <vector>
#include <chrono>
#include <assert.h>
#include <stdio.h>
#include "af_helpers.h"

#define bigendian 1

#ifdef lilendian
#define fourccRIFF 'RIFF'
#define fourccDATA 'data'
#define fourccFMT 'fmt '
#define fourccWAVE 'WAVE'
#endif


#ifdef bigendian
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#endif


/*
 
 The canonical WAVE format starts with the RIFF header:
 
 0         4   ChunkID          Contains the letters "RIFF" in ASCII form
 (0x52494646 big-endian form).
 
 4         4   ChunkSize        36 + SubChunk2Size, or more precisely:
 4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
 This is the size of the rest of the chunk
 following this number.  This is the size of the
 entire file in bytes minus 8 bytes for the
 two fields not included in this count:
 ChunkID and ChunkSize.
 
 8         4   Format           Contains the letters "WAVE"
 (0x57415645 big-endian form).
 
 
 
 The "WAVE" format consists of two subchunks: "fmt " and "data":
 The "fmt " subchunk describes the sound data's format:
 
 12        4   Subchunk1ID      Contains the letters "fmt "
 (0x666d7420 big-endian form).
 
 16        4   Subchunk1Size    16 for PCM.  This is the size of the
 rest of the Subchunk which follows this number.
 
 20        2   AudioFormat      PCM = 1 (i.e. Linear quantization)
 Values other than 1 indicate some
 form of compression.
 
 22        2   NumChannels      Mono = 1, Stereo = 2, etc.
 
 24        4   SampleRate       8000, 44100, etc.
 
 28        4   ByteRate         == SampleRate * NumChannels * BitsPerSample/8
 
 32        2   BlockAlign       == NumChannels * BitsPerSample/8
 The number of bytes for one sample including
 all channels. I wonder what happens when
 this number isn't an integer?
 
 34        2   BitsPerSample    8 bits = 8, 16 bits = 16, etc.
 
 The "data" subchunk contains the size of the data and the actual sound:
 
 36        4   Subchunk2ID      Contains the letters "data"
 (0x64617461 big-endian form).
 
 40        4   Subchunk2Size    == NumSamples * NumChannels * BitsPerSample/8
 This is the number of bytes in the data.
 You can also think of this as the size
 of the read of the subchunk following this
 number.
 
 44        *   Data             The actual sound data.
 
 
 */

/// A handle to an audio file (WAV only)
template <typename SampleType>
class AudioFile
{
public:
    // io operations
    static af_result load(const char* path, AudioFile<SampleType>& outFile, af_sync_option async = af_sync_option::sync)
    {
        // Doing it the C way
        // open file
        uint32_t * buffer = nullptr;
        FILE* pF = fopen(path, "rb");
        if(!pF){ return af_result::failure; }
        
        // get file size
        size_t sz;
        fseek(pF, 0, SEEK_END);
        sz = ftell(pF);
        rewind(pF);
        buffer = new uint32_t[sz];
        assert(buffer);
        size_t result = fread(buffer, 1, sz, pF);
        if(result != sz) { return af_result::failure; }
        
        // now that we have it in the buffer, close the file
        fclose(pF);
        
        uint32_t* wav_iter = buffer;
        // Time to parse the wave
        // chunkid, 4 bytes, big "RIFF"
        assert(*wav_iter == fourccRIFF);
        wav_iter++;
        // chunk size, 4 bytes, little
        size_t chunksz = *wav_iter;
        wav_iter++;
        // format, 4 bytes, big, "WAVE"
        assert(*wav_iter == fourccWAVE);
        wav_iter++;
        // fmt subchunk id, 4 bytes, big, "fmt "
        assert(*wav_iter == fourccFMT);
        wav_iter++;
        // fmt subchunk size , 4 bytes, little. 16 for PCM
        assert(*wav_iter == 16); // Only support PCM wave data for now.
        wav_iter++;
        // fmt fmt, 2 bytes, little. PCM = 1
        uint16_t* frmatAndNumChans = (uint16_t *)&wav_iter[0];
        assert(*frmatAndNumChans == 1); // only support PCM
        frmatAndNumChans++;
        // fmt num channels, 2 bytes, little
        uint16_t numChans = *frmatAndNumChans; // set num channels
        outFile.mChannelConfig = static_cast<Channels>(numChans);
        wav_iter++;
        // fmt sample rate, 4 bytes, little
        outFile.mSampleRate = *wav_iter;
        wav_iter++;
        // fmt byterate, 4 bytes, little
		outFile.mByteRate = *wav_iter;
        wav_iter++;
        // fmt blockalign, 2 bytes, little --> we may start caring about this
        uint16_t blockAlign = *(uint16_t *)&wav_iter[0];
		outFile.mBlockAlign = blockAlign;
        // fmt bits per sample, 2 bytes, little
        outFile.mBitsPerSample = *(((uint16_t *)&wav_iter[0]) + 1);
        wav_iter++;
        // data subchunk, 4 bytes, big, little, "data"
        assert(*wav_iter == fourccDATA);
        wav_iter++;
        // data subchunk size, 4 bytes, little
        uint32_t audioDataSz = *wav_iter;
        wav_iter++;
        // audio data (deep copy to internal vector), subchunk sz bytes, little?
        outFile.mAudioData.resize(audioDataSz);
        memcpy(&outFile.mAudioData[0], wav_iter, audioDataSz);
        // clean up
        delete[] buffer;
        return af_result::success;
    }
    
    static af_result saveToDisk(AudioFile<SampleType>& af, const char* filename)
    {
        /* get audio data and create a new wave on the disk at the specified location */
        
        // first, write all the metadata into a char buffer
        // then add audio data at the end
        
		// how much space do we need???? eh???
		const size_t sz = af.getAudioSize() + 40; // size of wave header
		std::vector<uint32_t> outBuf(sz / sizeof(uint32_t));

		// Time to parse the wave
		// chunkid, 4 bytes, big "RIFF"
        outBuf[0] = fourccRIFF;
		// chunk size, 4 bytes, little
		size_t chunksz = sz - 4; // size of file minus "RIFF literal"
		outBuf[1] = (chunksz);
		// format, 4 bytes, big, "WAVE"
		outBuf[2] = (fourccWAVE);
		// fmt subchunk id, 4 bytes, big, "fmt "
		outBuf[3] = (fourccFMT);
		// fmt subchunk size , 4 bytes, little. 16 for PCM
		outBuf[4] = (16);
		// fmt fmt, 2 bytes, little. PCM = 1, 2 bytes for channel config
        uint16_t* fmtAndConfig = (uint16_t *)&outBuf[5];
        *fmtAndConfig = uint16_t(1);
        fmtAndConfig[1] = static_cast<uint16_t>(af.getChannelConfig());
		// fmt sample rate, 4 bytes, little
		outBuf[6] = (af.getSampleRate());
		// fmt byterate, 4 bytes, little
		outBuf[7] = (af.mByteRate);
		// fmt blockalign, 2 bytes, little --> we may start caring about this
		// fmt bits per sample, 2 bytes, little
        uint16_t* smplData = (uint16_t *)&outBuf[8];
        smplData[0] = af.mBlockAlign;
        smplData[1] = af.mBitsPerSample;
		// data subchunk, 4 bytes, big, little, "data"
		outBuf[9] = (fourccDATA);
		// data subchunk size, 4 bytes, little
		outBuf[10] = (af.getAudioSize());
		// audio data (deep copy to internal vector), subchunk sz bytes, little?
		memcpy(&outBuf[11], af.getAudioData(), af.getAudioSize());
        // then do file ops and write to disk
        
		FILE* pF = fopen(filename, "wb");
		if (!pF) { return af_result::failure; }

		// get file size
		size_t result = fwrite(&outBuf[0], 1, sz, pF);
		if (result != sz) { return af_result::failure; }
		// now that we have it in the buffer, close the file
		fclose(pF);

		return af_result::success;
    }
    
    // big four
    AudioFile() :
    mAudioData(),
    mChannelConfig(Channels::None),
    mSampleRate(-1)
    {};
    
    AudioFile(const AudioFile&) = default;
    AudioFile& operator= (const AudioFile&) = default;
    ~AudioFile() = default;
    
    // accessors
    const uint8_t* getAudioData() const
    {
        return &mAudioData[0];
    }
    
    uint32_t getAudioSize() const
    {
        return mAudioData.size();
    }
    
    Channels getChannelConfig() const
    {
        return mChannelConfig;
    }
    
    int32_t getSampleRate()
    {
        return mSampleRate;
    }
    
    std::chrono::milliseconds getTimeLength() const
    {
        /* not implemented yet */
        assert(false);
    }
    
    // transport
    af_result play() const
    {
        /* pass through to audio file player */
        return af_result::failure;
    }
    
    af_result stop() const
    {
        /* pass through to audio file player */
        return af_result::failure;
    }
    
    af_result pause() const
    {
        /* pass through to audio file player */
        return af_result::failure;
    }
    
private:
    //
    std::vector<uint8_t> mAudioData;
    Channels mChannelConfig;
    int32_t mSampleRate;
    uint16_t mBitsPerSample;
	uint32_t mByteRate;
	uint16_t mBlockAlign;
};
#endif
