//
//  af_helpers.h
//  audiofile_helper
//
//  Created by Jack Campbell on 6/3/18.
//

#ifndef af_helpers_h
#define af_helpers_h

enum Channels
{
    Mono = 1,
    Sterep = 2
};

enum af_result
{
    success,
    failure
};

enum af_sync_option
{
    sync = 0,
    async = 1
};

#endif /* af_helpers_h */
