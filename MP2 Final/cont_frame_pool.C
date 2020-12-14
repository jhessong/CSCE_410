/*
 File: ContFramePool.C
 
 Author:
 Date  : 
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define MB * (0x1 << 20)
#define KB * (0x1 << 10)

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/
ContFramePool* ContFramePool::pool_list_head;
ContFramePool* ContFramePool::pool_list;
/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no,
                             unsigned long _n_info_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!
    assert(_n_frames <= FRAME_SIZE*8);
    
    base_frame_no = _base_frame_no;
    nframes = _n_frames;
    nFreeFrames = _n_frames;
    info_frame_no = _info_frame_no;
    n_info_frames = _n_info_frames;

    if(info_frame_no == 0) {
        bitmap = (unsigned char *) (base_frame_no * FRAME_SIZE);
    } else {
        bitmap = (unsigned char *) (info_frame_no * FRAME_SIZE);
    }

    assert ((nframes % 8 ) == 0);

    for(int i=0; i*8 < _n_frames*2; i++) {
        bitmap[i] = 0x0;
    }

    if(_info_frame_no == 0) {
        bitmap[0] = 0x40;
        nFreeFrames--;
    }

    if(ContFramePool::pool_list_head==NULL) {
        ContFramePool::pool_list=this;
        ContFramePool::pool_list_head=this;
    }
    else {
        ContFramePool::pool_list->next = this;
        ContFramePool::pool_list=this;
    }

    next = NULL;
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!


    bool flag_search = false;
    bool flag_found = false;
    int i_storage = 0;
    int j_storage = 0;
    unsigned int needed = _n_frames;
    unsigned int frame = base_frame_no;

    if(needed > nFreeFrames) {
        Console::puts("Not enough frames available");
    }

    for(unsigned int i = 0; i < nframes/4; i++) {
        unsigned char mask = 0xC0;
        for (int j = 0; j < 4; j++) {
            if((bitmap[i] & mask) == 0) {
                if (flag_search) {
                    needed--;
                }
                else {
                    frame += i*4 + j;
                    i_storage = i;
                    j_storage = j;
                    flag_search = true;
                    needed--;
                }
            }
            else {
                if(flag_search) {
                    i_storage = 0;
                    j_storage = 0;
                    frame = base_frame_no;
                    needed = _n_frames;
                    flag_search = false;
                }
            }
            mask = mask>>2;
            if (needed == 0) {
                flag_found = true;
                break;
            }
        }
        if (needed == 0) {
            flag_found = true;
            break;
        }
    }
    if (!flag_found) {
        Console::puts("No free frame found");
        return 0;
    }
    int set = _n_frames;
    unsigned char mask1 = 0x40;
    unsigned char mask2 = 0xC0;
    mask1 = mask1 >> (j_storage*2);
    mask2 = mask2 >> (j_storage*2);
    bitmap[i_storage] = (bitmap[i_storage] & ~mask2) | mask1;
    j_storage++;
    set--;
    unsigned char a = 0xC0;
    a = a>>(j_storage*2);
    while(set > 0 && j_storage < 4) {
        bitmap[i_storage] = bitmap[i_storage] | a;
        a = a>>2;
        j_storage++;
        set--;
    }
    for(int i = i_storage + 1; i < nframes/4; i++) {
        a = 0xC0;
        for (int j = 0; j < 4; j++) {
            if (set == 0) {
                break;
            }
            bitmap[i] = bitmap[i] | a;
            a = a >> 2;
            set--;
        }
        if(set == 0) {
            break;
        }
    }
    if (flag_search) {
        nFreeFrames -= _n_frames;
        return frame;
    }
    else {
        Console::puts("No free frame found");
        return 0;
    }
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!

    if(_base_frame_no < base_frame_no || base_frame_no + nframes < _base_frame_no + _n_frames) {
        Console::puts("out of range, cannot mark inaccessible");
    }
    else {
        unsigned char a = 0x80;
        unsigned char mask1 = 0xC0;
        nFreeFrames -= _n_frames;
        int difference = (_base_frame_no - base_frame_no)*2;
        int i_storage = difference/8;
        int j_storage = (difference % 8)/2;
        int set = _n_frames;
        a = a >> (j_storage*2);
        mask1 = mask1 >> (j_storage*2);
        while(set > 0 && j_storage < 4) {
            bitmap[i_storage] = (bitmap[i_storage] & ~mask1) | a;
            set--;
            j_storage++;
            mask1 = mask1>>2;
            a = a >> 2;
        }
        for(int i = i_storage + 1; i < i_storage + _n_frames/4; i++) {
            a = 0xC0;
            mask1 = 0xC0;
            for(int j = 0; j < 4; j++) {
                if(set == 0) {
                    break;
                }
                bitmap[i] = (bitmap[i] & ~mask1) | a;
                set--;
                mask1 = mask1 >>2;
                a >> a>>2;
            }
            if (set == 0) {
                break;
            }
        }
    }
}

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    // TODO: IMPLEMENTATION NEEEDED!

    ContFramePool* curr = ContFramePool::pool_list_head;
    while ((curr->base_frame_no > _first_frame_no || curr->base_frame_no + curr->nframes <= _first_frame_no)) {
        if (curr->next == NULL) {
            Console::puts("Frame not found\n");
            return;
        }
        else {
            curr = curr->next;
        }
    }
    unsigned char mask1 = 0x80;
    unsigned char a = 0xC0;
    unsigned char* bitmap_pointer = curr->bitmap;
    int difference = (_first_frame_no - curr->base_frame_no)*2;
    int i_storage = difference/8;
    int j_storage = (difference%8)/2;
    mask1 = mask1 >> j_storage*2;
    a = a >> j_storage*2;
    if(((bitmap_pointer[i_storage]^mask1) & a) == a) {
        bitmap_pointer[i_storage] = bitmap_pointer[i_storage] & (~a);
        a = a>>2;
        j_storage++;
        curr->nFreeFrames++;
        while(j_storage < 4) {
            if((bitmap_pointer[i_storage] & a) == a) {
                bitmap_pointer[i_storage] = bitmap_pointer[i_storage] & (~a);
                a = a>>2;
                j_storage++;
                curr->nFreeFrames++;
            }
            else {
                return;
            }
        }
        for(int i = i_storage+1; i < (curr->base_frame_no + curr->nframes)/4; i++) {
            a = 0xC0;
            for(int j = 0; j < 4; j++) {
                if((bitmap_pointer[i] & a) == a) {
                    bitmap_pointer[i] = bitmap_pointer[i] & (~a);
                    curr->nFreeFrames++;
                    a = a>>2;
                }
                else {
                    return;
                }
            }
        }
    }
    else {
        Console::puts("Frame not head of sequence\n");
    }
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!
    return (_n_frames*2)/(8*4 KB) + ((_n_frames*2) % (8*4 KB) > 0 ? 1 : 0);
}
