#include <stdio.h>
#include "AudioFile.h"

int main()
{
    printf("hello world");
    
    // TESTS!
    
    // load a wave
    // assert all of the properties of the wave
    AudioFile<float> aud;
    auto res = AudioFile<float>::load("/Users/jcampbell/git/audiofile/resources/drums.wav", aud);
    assert(res == af_result::success);
    // save that wave back to disk under a new name
    
    
    // load that wave again back into memory, assert data is equal to the first wave
    
    
    // get wave data and process it (modulate or something), save to disk
    
    
    // create audio data in memory, construct an audiofile object with it, save to disk
    
    return 0;
}
