#ifndef af_file
#define af_file
#include <vector>
#include <chrono>
#include <assert.h>
#include <stdio.h>
#include "af_helpers.h"
#include <math.h>

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
        //assert(*wav_iter == 16); // Only support PCM wave data for now.
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
        
        int32_t deadMan = 0;
        // now search for data, ignore all this parameter stuff from weird wavs
        uint8_t* dataFinder = (uint8_t *)&wav_iter[0];
        while(*(uint32_t *)(dataFinder) != fourccDATA)
        {
            if(deadMan++ > 32000)
            {
                return af_result::failure;
            }
            dataFinder++;
        }
        wav_iter = (uint32_t*)dataFinder;
        // data subchunk, 4 bytes, big, little, "data"
        assert(*wav_iter == fourccDATA);
        wav_iter++;
        // data subchunk size, 4 bytes, little
        uint32_t audioDataBytes = (*wav_iter);
        wav_iter++;
        // audio data (deep copy to internal vector), subchunk sz bytes, little?
        std::vector<int16_t> audioShorts(audioDataBytes / sizeof(int16_t));
        memcpy(&audioShorts[0], wav_iter, audioDataBytes);
        
        int32_t samplesPerChannel = (audioDataBytes / sizeof(int16_t)) / numChans;
        // make array of channels
        for(int32_t i = 0; i < numChans; i++)
        {
            outFile.mAudioData.emplace_back( samplesPerChannel );
        }
        
        SampleType M = pow(2.0, outFile.mBitsPerSample - 1);
        for(int32_t channel = 0; channel < numChans; channel++)
        {
            //printf("\n\n\n\n ***********CHANNEL: %d*************\n\n\n\n", channel);
            for(int32_t sample = channel; sample < samplesPerChannel; sample++)
            {
                //printf("\nChannel: %d, Sample: %d, (sample * numChans) - channel = %d\n", channel, sample, (sample * numChans) - channel);
                outFile.mAudioData[channel][sample - channel] = static_cast<SampleType>( audioShorts[(sample * numChans) - channel] / M);
            }
        }
        
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
        int16_t numChans = int16_t(af.getChannelConfig());
		const size_t sz = ((af.getSamplesPerChannel() * numChans) * sizeof(int16_t)) + 40; // size of wave header
		std::vector<uint32_t> outBuf(sz);

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
        fmtAndConfig[1] = numChans;
		// fmt sample rate, 4 bytes, little
		outBuf[6] = (af.getSampleRate());
		// fmt byterate, 4 bytes, little
		outBuf[7] = (af.mByteRate);
		// fmt blockalign, 2 bytes, little --> we may start caring about this
		// fmt bits per sample, 2 bytes, little
        uint16_t* smplData = (uint16_t *)&outBuf[8];
        smplData[0] = af.mBlockAlign;
        smplData[1] = 16;//af.mBitsPerSample;
		// data subchunk, 4 bytes, big, little, "data"
		outBuf[9] = (fourccDATA);
		// data subchunk size, 4 bytes, little
        int32_t numSamples = af.getSamplesPerChannel() * numChans;
		outBuf[10] = ((numSamples) * sizeof(float));
		// audio data (deep copy to internal vector), subchunk sz bytes, little?
        
        //go back to shorts
        std::vector<int16_t> audioShorts;
        double M = pow(2, af.mBitsPerSample - 1);
        float scaledSample = 0.0f;
        
        
        // Re-interleave the samples
        int32_t channel = 0;
        for(int32_t sample = 0; sample < af.getSamplesPerChannel();)
        {
            scaledSample = af.mAudioData[channel][sample] * M;
            // clip if needed
            if(scaledSample < -M) scaledSample = -M;
            else if(scaledSample > M - 1) scaledSample = M - 1;
            
            
            audioShorts.push_back(static_cast<int16_t>(scaledSample));
            
            // increment the loop
            channel++;
            if(channel == numChans)
            {
                channel = 0;
                sample++;
            }
        }
        
		memcpy(&outBuf[11], reinterpret_cast<uint8_t*>(&audioShorts[0]), audioShorts.size() * sizeof(int16_t));
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
    SampleType** getAudioData()
    {
        return reinterpret_cast<SampleType**>((&mAudioData[0]));
    }
    
    size_t getSamplesPerChannel() const
    {
        if(mAudioData.size())
        {
            return mAudioData[0].size();
        }
        else
        {
            return 0;
        }
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
    
    std::vector<std::vector<SampleType>> mAudioData;
    
private:
    //
    
    Channels mChannelConfig;
    int32_t mSampleRate;
    uint16_t mBitsPerSample;
	uint32_t mByteRate;
	uint16_t mBlockAlign;
};
#endif
