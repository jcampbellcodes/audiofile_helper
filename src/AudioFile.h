#ifndef af_file
#define af_file
#include <vector>
#include <chrono>
#include <assert.h>

#include "af_helpers.h"


// a handle to an audio file
template <typename SampleType>
class AudioFile
{
public:
    // io operations
    static AudioFile<SampleType>& load(const char* path, af_sync_option async = af_sync_option::sync)
    {
        /* handle all nasty wav parsing from a file path here */
    }
    
    static af_result unload(AudioFile<SampleType>& af)
    {
        /* if saved in memory, free it */
    }
    
    static af_result saveToDisk(AudioFile<SampleType>& af, const char* filename)
    {
        /* get audio data and create a new wave on the disk at the specified location */
    }
    
    // big four
    AudioFile() {};
    AudioFile(const AudioFile&) = default;
    AudioFile& operator= (const AudioFile&) = default;
    ~AudioFile() = default;
    
    // accessors
    float** getAudioData() const
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
    std::vector<char> mAudioData;
    Channels mChannelConfig;
    int32_t mSampleRate;
};
#endif
