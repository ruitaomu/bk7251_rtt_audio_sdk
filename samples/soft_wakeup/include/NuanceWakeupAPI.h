/***********************************************************************
 * Copyright 2017, Nuance Communications, Inc.
 *
 *  No part of this manual may be reproduced in any form or by any means,
 *  electronic or mechanical, including photocopying, recording, or by any
 *  information storage and retrieval systems, without the express written
 *  consent of Voice Signal Technologies, Inc.
 *
 *  The content of this manual is furnished for informational use only, is
 *  subject to change without notice, and should not be construed as a
 *  commitment Nuance Communications, Inc. Nuance Communications Inc. assumes
 *  no responsibility or liability for any errors or inaccuracies
 *  that may appear.
 *
 */
/*!
 *  \file NuanceWakeupAPI.h
 *  \brief Nuance Wakeup API (NWA) for low power embedded (DSP) word spotting.
 *   chris.leblanc@nuance.com
 */
#ifndef NWA_INCLUDED
#define NWA_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif
#include <limits.h>

#define NWA_HEAP_REQUEST_SIZE (19 * 1024) /* 19 KB  */

#define NWA_PHRASE_MAX_CHARS  (256)

typedef enum ED_ERROR_E {
    ED_OK = 0,
    ED_MEMORY_ALLOCATED_ERROR = 101,
    ED_TAG_ERROR = 102,
    ED_FILE_OPEN_ERROR = 103,
    ED_NO_PERMISSION = 104
}ED_ERROR;


/*! \breif Create the Nuance Wakeup object by given a language model. Resulting object in idle state.
 *  \param[in] pLanguageModel A pointer to the language data containing the wakeup phrase(s).
 *  \param[in] modelSize Actual size in bytes of the language model to load.
 *  \param[in] pHeapMem Pointer to block of memory used by MxE. Refer to NWA_HEAP_REQUEST_SIZE for a recommendation heap size.
 *  \param[in] heapSize Actual heap size in bytes. Refer to NWA_HEAP_REQUEST_SIZE for a recommendation heap size.
 *  \param[in] frameSamples Sample count for each frame passed into nwa_scanAudioFrame or nwa_scanAudioFrameEx.
 *  \The value should not be greater than 320(20ms 16k 16bit audio).
 *  \return non-zero for error
 */
int nwa_createObject_decode(void * pLanguageModel, size_t modelSize, void * pHeapMem, size_t heapSize, int frameSamples);

/*! \breif Create the Nuance Wakeup object by given a language model. Resulting object in idle state.
 *  \param[in] pLanguageModel A pointer to the language data containing the wakeup phrase(s).
 *  \param[in] modelSize Actual size in bytes of the language model to load.
 *  \param[in] pHeapMem Pointer to block of memory used by MxE. Refer to NWA_HEAP_REQUEST_SIZE for a recommendation heap size.
 *  \param[in] heapSize Actual heap size in bytes. Refer to NWA_HEAP_REQUEST_SIZE for a recommendation heap size.
 *  \param[in] frameSamples Sample count for each frame passed into nwa_scanAudioFrame or nwa_scanAudioFrameEx.
 *  \The value should not be greater than 320(20ms 16k 16bit audio).
 *  \return non-zero for error
 */
int nwa_createObject(void * pLanguageModel, size_t modelSize, void * pHeapMem, size_t heapSize, int frameSamples);
    
/*! \breif Destroy the Nuance Wakeup object. Object must be in idle state.
 *  \return non-zero for error
 */
int nwa_destroyObject(void);
    
/*! \breif Transition to wakeup state.  
 *  \param[in] bUsesMAD specify if the phrase may be chopped from MAD
 *  \return non-zero for error
 */
int nwa_startWakeup(int bUsesMAD);

/*! \breif Transition to idle state.  
 *  \return non-zero for error
 */
int nwa_stopWakeup(void);

/*! \breif Transition to idle state and check if there is a wakeup.
 *  \param[out] pbWakeup Set when a wakeup phrase is recognized.
 *  \return non-zero for error
 */
int nwa_stopWakeupCheck(int * pbWakeup);

/*! \breif Scan the audio samples and searching for the wakeup phrase.
 *  \param[in] pFrame Pointer to the current audio frame to scan.
 *  \param[in] nSamples Number of samples available.
 *  \param[out] pbWakeup Set when a wakeup phrase is recognized.
 *  \return non-zero for error
 */
int nwa_scanAudioFrame(short * pFrame, int nSamples, int * pbWakeup);

#define NWA_13MHZ 1
#define NWA_20MHZ 2
#define NWA_30MHZ 3
#define NWA_40MHZ 4
#define NWA_50MHZ 5
#define NWA_100MHZ INT_MAX
/*! \breif Scan the audio samples and searching for the wakeup phrase.
 *  \param[in] pFrame Pointer to the current audio frame to scan.
 *  \param[in] nSamples Number of samples available.
 *  \param[in] uPSpeed Maximum processing speed.
 *  \param[out] isSpeech Set when Elvis believes the scanned frame is speech.
 *  \param[out] pbWakeup Set when a wakeup phrase is recognized.
 *  \return non-zero for error
 */
int nwa_scanAudioFrameEx(short * pFrame, int nSamples, int uPSpeed, int * isSpeech, int * pbWakeup);

/*! \breif Get info for last recognized wakeup utterance. Used to identify audio.
 *  \param[out] ppWakeupString Pointer to the wakeup unicode(UCS-2) string.
 *  \param[out] pLenPhrase a pointer into which we will write the actual length
 *   of the wakeup phrase.It will be not greater than 256(refer to NWA_PHRASE_MAX_CHARS).
 *  \param[out] pStartFramesBack Number of frames(the frame size is determined by
 *   frameSamples which passed into 'nwa_createObject') back where wakeup audio began.
 *  \param[out] pLengthFrames Length in frames(the frame size is determined by
 *   frameSamples which passed into 'nwa_createObject') of the recognized wakeup phrase.
 */
int nwa_getWakeupInfo(unsigned short ** ppWakeupString, int * pLenPhrase, int * pStartFramesBack, int * pLengthFrames);

/*! \breif Get wakeup score.
 *  \param[out] pScore the score of detected wakeup phrase.
 */
int nwa_getWakeupScore(int *pScore);
    
#ifdef __cplusplus
}
#endif
#endif /* NWA_INCLUDED */
