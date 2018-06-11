#include <stdio.h>
#include "AudioFile.h"
#include <string>
#include <math.h>

const double TWO_PI = 2.0 * 3.14159265358979324;

/*
 
 TODO -> 
 Add more unit tests
 Add variable bit rate support
 Add little vs big endian support
 
 */

int main()
{
    printf("AudioFile Tests. Make sure to update paths to a legitimate file!\n");
    // load a wave
    // assert all of the properties of the wave
    AudioFile<double> aud;
    auto res = AudioFile<double>::load("/Users/JackCampbell/Git/audiofile_helper/resources/drums_stereo_16.wav", aud);
    assert(res == af_result::success && "Wave file not loaded correctly :(");
    
    // get wave data and process it (modulate or something), save to disk
    // y(t) = in_sig * (A * sinf(2PI * frequency * timeval))
    float azimuth_deg = 0.0f;
    
    // pan!
    for(int32_t sample = 0; sample < aud.getSamplesPerChannel(); sample++)
    {
        float azimuth_rad = azimuth_deg * 2.0f*M_PI / 360.0f;
        float gainL = -sinf( azimuth_rad/2 - M_PI/4);
        float gainR =  cosf( azimuth_rad/2 - M_PI/4);
        aud.mAudioData[0][sample] *= gainL;
        aud.mAudioData[1][sample] *= gainR;
        azimuth_deg += 0.005f;
    }
    
    // save that wave back to disk under a new name
	res = AudioFile<double>::saveToDisk(aud, "/Users/JackCampbell/Git/audiofile_helper/resources/drums_panned.wav");
	assert(res == af_result::success && "Wave file not written correctly :(");
    
    // load that wave again back into memory, assert data is equal to the first wave
    // create audio data in memory, construct an audiofile object with it, save to disk
    res = AudioFile<double>::load("/Users/JackCampbell/Git/audiofile_helper/resources/song.wav", aud);
    assert(res == af_result::success && "Wave file not loaded correctly :(");
    const int32_t channels = aud.getChannelConfig();
    const int32_t samples = aud.getSamplesPerChannel();
    
    for(int32_t channel = 0; channel < channels; channel++)
    {
        for(int32_t sample = 0; sample < samples; sample++)
        {
            aud.mAudioData[channel][sample] *= 0.8f * sinf(TWO_PI * 25.0 * (float(sample)/aud.getSampleRate()));
        }
    }
    // save that wave back to disk under a new name
    res = AudioFile<double>::saveToDisk(aud, "/Users/JackCampbell/Git/audiofile_helper/resources/song_mod.wav");
    assert(res == af_result::success && "Wave file not written correctly :(");
    
    
    return 0;
}
