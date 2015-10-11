/*
 Copyright (c) 2010 OpenEmu Team

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.
 * Neither the name of the OpenEmu Team nor the
 names of its contributors may be used to endorse or promote products
 derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY OpenEmu Team ''AS IS'' AND ANY
 EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL OpenEmu Team BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "STEMGameCore.h"

#import <OpenEmuBase/OERingBuffer.h>
#import <OpenGL/gl.h>
#import "1802.h"
#import "external.h"

@interface STEMGameCore () <OEStudioIISystemResponderClient>
{
    uint8_t *audioStream;
    uint32_t *videoBuffer;
    int videoWidth, videoHeight;
    NSString *romPath;
    int audioLength;
}
@end

STEMGameCore *current;

// Sound buffer size
#define SOUNDBUFFER	2048
#define PMSOUNDBUFF	(SOUNDBUFFER*2)

@implementation STEMGameCore

- (id)init
{
    if (self = [super init])
    {
        videoWidth = 64;
        videoHeight = 32;
        
        audioLength = PMSOUNDBUFF;
        
        audioStream = malloc(PMSOUNDBUFF);
        videoBuffer = malloc(videoWidth*videoWidth*4);
        memset(videoBuffer, 0, videoWidth*videoHeight*4);
        memset(audioStream, 0, PMSOUNDBUFF);
    }

    current = self;
    return self;
}

- (void)dealloc
{
    free(audioStream);
    free(videoBuffer);
}

#pragma - mark Execution

- (BOOL)loadFileAtPath:(NSString *)path error:(NSError **)error
{
    romPath = path;
    return YES;
}

- (void)executeFrameSkippingFrame:(BOOL)skip
{
    CompleteFrame([romPath UTF8String]);
}

- (void)executeFrame
{
    [self executeFrameSkippingFrame:NO];
}

- (void)setupEmulation
{
    ResetCPU([romPath UTF8String]);
}

- (void)startEmulation
{
    if(!isRunning)
    {
        [super startEmulation];
    }
}

- (void)stopEmulation
{
    [super stopEmulation];
}

- (void)resetEmulation
{
    ResetCPU([romPath UTF8String]);
}

#pragma mark - Save State

- (BOOL)saveStateToFileAtPath:(NSString *)fileName
{
    return NO;
}

- (BOOL)loadStateFromFileAtPath:(NSString *)fileName
{
    return NO;
}

#pragma mark - Video

- (OEIntSize)aspectSize
{
    return (OEIntSize){videoWidth, videoHeight};
}

- (OEIntRect)screenRect
{
    return OEIntRectMake(0, 0, videoWidth, videoHeight);
}

- (OEIntSize)bufferSize
{
    return OEIntSizeMake(videoWidth, videoHeight);
}

- (const void *)videoBuffer
{
//    return ASCREEN;
    return videoBuffer;
}

- (GLenum)pixelFormat
{
    return GL_BGRA;
}

- (GLenum)pixelType
{
    return GL_UNSIGNED_INT_8_8_8_8_REV;
}

- (GLenum)internalPixelFormat
{
    return GL_RGB8;
}

- (NSTimeInterval)frameInterval
{
    return 60;
}

#pragma mark - Audio

- (double)audioSampleRate
{
    return 44100;
}

- (NSUInteger)audioBitDepth
{
    return 8;
}

- (NSUInteger)channelCount
{
    return 1;
}

#pragma mark - Input

int Keys[2][10];

- (oneway void)didPushIntellivisionButton:(OEStudioIIButton)button forPlayer:(NSUInteger)player
{
    Keys[player-1][button] = 1;
}

- (oneway void)didReleaseIntellivisionButton:(OEStudioIIButton)button forPlayer:(NSUInteger)player
{
    Keys[player-1][button] = 0;
}

int EXTKeyPressed(int Key,int Player)
{
    int Press = 0;
    Press = Keys[0][Key];
    if (Player == PLAYER2) Press = Keys[1][Key];
    return((Press == 0) ? 0 : 1);
}

void putpixel(int x, int y, int p)
{
    uint32_t *pixels = current->videoBuffer + y * current->videoWidth + x;
    if (p)
    {
        pixels[0] = 0x00ffffff;
    }
    else
    {
        pixels[0] = 0x00000000;
    }
}

int FrameCounter = 0;
clock_t LastTick;
int SoundFlags[3];

void EXTSynchronise(unsigned char *ScreenData,
                    int Scroll,unsigned char *HiRes,int SoundOn)
{
    int x,y,Byte,Pixel;
    if (ScreenData != NULL)
    {
        for (x = 0;x < 64;x++)          		/* Horizontal coordinates */
        {
            for (y = 0;y < 32;y++)				/* Vertical coordinates */
            {									/* Get data address, then pixel */
                Byte = ((x / 8) + (y * 8) + Scroll) & 0xFF;
                Pixel = (ScreenData[Byte] & (128 >> (x & 7))) ? 1 : 0;
                putpixel(x, y, Pixel);
            }
        }
    }
    
    clock_t t;
    int Freq;
    
    SoundFlags[FrameCounter++] = SoundOn;	/* Track sound for three frames */
    
    if (FrameCounter == 3)					/* Every 3 frames. We can't sync */
    {									/* to 60Hz, so sync to 18.2Hz */
        FrameCounter = 0;
        do  								/* Wait for clock tick change */
            t = clock();
        while (t == LastTick);
        LastTick = t;
        
        if (SoundFlags[0] ||				/* Sound if any sound this time */
            SoundFlags[1] || SoundFlags[2])
        {
            Freq = SoundFlags[0];			/* Select frequency */
            if (SoundFlags[1]) Freq = SoundFlags[1];
            if (SoundFlags[2]) Freq = SoundFlags[2];
            sound(Freq);					/* And beep ! */
        }
        else        						/* No sound, sound off */
        {
//            nosound();
        }
    }
}

void sound(int freq)
{
    if(freq <= 0)
        memset(current->audioStream, 0, current->audioLength);
    else {
        int i;
        static char v = 0x7f;
        static int f = 0;
        for(i=0; i<current->audioLength; i++) {
            f += freq;
            while(f >= 44100) {
                v ^= 0xff;
                f -= 44100;
            }
            current->audioStream[i] = v;
        }
    }
    [[current ringBufferAtIndex:0] write:current->audioStream maxLength:current->audioLength];
}

@end
