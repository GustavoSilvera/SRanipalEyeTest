# SRanipalEyeTest
Testing for a potential bug in the [`SRanipal`](https://developer.vive.com/resources/vive-sense/sdk/vive-eye-tracking-sdk-sranipal/) Eye tracker UE4 implementation

SRanipal forum post (with video) here: 

[https://forum.vive.com/topic/9306-possible-bug-in-unreal-sdk-for-leftright-eye-gazes/](https://forum.vive.com/topic/9306-possible-bug-in-unreal-sdk-for-leftright-eye-gazes/?ct=1613756396)

In particular, the left and right eye gaze rays are swapped. 
- Exhibit 1: Crossing eyes should cross the lines
  - But instead the lines diverge
- Exhibit 2: Closing **left** eye should only move **right** gaze 
  - But instead the **left** eye gaze moves
- Exhibit 3: Closing **right** eys should only move **left** gaze
  - But instead the **right** eye gaze moves
