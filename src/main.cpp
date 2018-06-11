#include <stdio.h>
#include "AudioFile.h"
#include <string>
#include <math.h>

const double TWO_PI = 2.0 * 3.14159265358979324;

int main()
{
    printf("AudioFile Tests. Make sure to update paths to a legitimate file!\n");
    // load a wave
    // assert all of the properties of the wave
    AudioFile<float> aud;
    auto res = AudioFile<float>::load("/Users/JackCampbell/Git/audiofile_helper/resources/drums_stereo_16.wav", aud);
    assert(res == af_result::success && "Wave file not loaded correctly :(");
    
    // get wave data and process it (modulate or something), save to disk
    // y(t) = in_sig * (A * sinf(2PI * frequency * timeval))
    const int32_t channels = aud.getChannelConfig();
    const int32_t samples = aud.getSamplesPerChannel();
    for(int32_t channel = 0; channel < channels; channel++)
    {
        for(int32_t sample = 0; sample < samples; sample++)
        {
            aud.mAudioData[channel][sample] *= 0.5f * sinf(TWO_PI * 1000.0 * sample);
        }
    }

	// save that wave back to disk under a new name
	res = AudioFile<float>::saveToDisk(aud, "/Users/JackCampbell/Git/audiofile_helper/resources/drums_written.wav");
	assert(res == af_result::success && "Wave file not written correctly :(");
    
    
    
    
    // load that wave again back into memory, assert data is equal to the first wave
    // create audio data in memory, construct an audiofile object with it, save to disk

    res = AudioFile<float>::load("/Users/JackCampbell/Git/audiofile_helper/resources/song.wav", aud);
    assert(res == af_result::success && "Wave file not loaded correctly :(");
    // save that wave back to disk under a new name
    res = AudioFile<float>::saveToDisk(aud, "/Users/JackCampbell/Git/audiofile_helper/resources/song_written.wav");
    assert(res == af_result::success && "Wave file not written correctly :(");
    
    
    return 0;
}
