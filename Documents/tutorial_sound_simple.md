# Simple DISSCO Project with LASSIE: Sound Synthesis 2.2.0

## 1. Download DISSCO

- Download and install the complete DISSCO application (`DISSCO-*`) by following the [Downloading DISSCO page](../DOWNLOAD.md). This includes LASSIE, which is required for this tutorial.

> **macOS:** A macOS standalone appimage is not currently available. [macOS users must build from source](../BUILDING_MACOS.md#quick-start-with-homebrew). Follow the instructions starting at the "Quick Start with Homebrew" section.

## 2. Launch LASSIE (if DISSCO is already installed, start here)

- Open LASSIE using the instructions for your operating system on the [Downloading DISSCO page](../DOWNLOAD.md).

- Wait for the LASSIE window to appear.

- Click **New Project** in the toolbar.

In the New Project dialog, choose your home directory as the project location
(for example, `/home/yourNetID` on Linux or `/Users/yourname` on macOS).
Avoid saving the project inside the DISSCO or LASSIE application/source folder.

- Name your project `tutorial` and click **Save**.
<img width="600" height="400" alt="image" src="https://github.com/user-attachments/assets/246bf997-d6d6-48d0-8669-adec0122aee4" />

(Note: Once you choose a folder for your project, do not move or rename it later. Otherwise, DISSCO may not be able to locate the project files.)

(**Note:** Once you choose a folder for your project, do not move or rename it later. Otherwise, DISSCO may not be able to locate the project files.)

- A new window, **Project Properties**, will appear
  
   1. only check the boxes **Sound Synthesis** and **Ouput Particel**

   2. set **Piece Duration** to 30 and click OK
<img width="290" height="350" alt="image" src="https://github.com/user-attachments/assets/979aeb4a-6604-4bdc-bed0-72f2227bb829" />


## 3. Build a Top event – this is your piece and the root node of the structure

(the default name of the newly created Top object is 0)

- Click the wedge next to **Folder Top** then double click **Top 0**
- Set **Number of Children to Create** to 1
- Set **Child Start Time** to 0
- Set **Child Type** to 0
- Set **Child Duration** and **Max Child Duration** to 30
<img width="2008" height="1262" alt="image" src="https://github.com/user-attachments/assets/387a2a67-1fc2-4725-a608-c644c38dfdaa" />

## 4. Create a Bottom event - this event creates start times, durations, pitches, and dynamics of sounds

- Click **Folder Bottom**
- Click the **New Object** button in the toolbar
- Name it s1 and click OK (NOTE: Bottom names must begin with a lower-case s !)
- Click the wedge next to the **Folder Bottom** and double click **Bottom s1**
- Set **Number of Children to Create** to 5  (these will be your sounds)
- Click **Insert Function** next to **Child Start Time** and choose **Random**

  1. we are randomizing the starting times for our 5 children (sounds)
  
    - keep **Lower Bound** at 0
  
    - set **Higher Bound** to 1, hit OK
  <img width="290" height="300" alt="image" src="https://github.com/user-attachments/assets/5e015d37-5224-4cf1-9675-298cd8dbdf66" />

  2. Set **Child Type** to 0
 
  3. Set **Child Duration** and **Max Child Duration** to 5  (each sound will be 5 seconds long)
  <img width="1988" height="1226" alt="image" src="https://github.com/user-attachments/assets/c8494fb6-3e13-4272-8c9d-e31c17805751" />
(we will come back to complete the bottom event in step 7)

## 5. Create a simple Envelope

- Click **Envelope Library** button at bottom of the window
- Click **Create New Envelope** bottom (in the top) to create a new envelope item
- Select the envelope item labeled 1 in the upper blank rectangle
- Right-click the lower rectangle (graph) and select **Insert Node**
- Moving around the node, you should notice the X and Y values below the graph
changing accordingly. Drag the node to the top center or input the X and Y values to
be **X value: 0.5000** and **Y value: 1.0000**
<img width="500" height="300" alt="image" src="https://github.com/user-attachments/assets/e0b0ffbb-b9a0-4f25-913e-46c5febfbe69" />

- In the bottom left and right corners of the graph, there are 2 nodes. Be sure their
values are **X value: 0.0000** and **Y value: 0.0000**, **X value: 1.0000** and **Y value: 0.0000** to
avoid clipping in your piece

(NOTE: Refer to the manual for more information about envelopes)

## 6. Create a Spectrum  ( this will determine the internal structure of your sound(s) )

- Click **Folder Spectrum**
- Click **New Object** button in the toolbar
- Name it sp1 and click OK
- Click the wedge next to the **Folder Spectrum** and double click **Spectrum sp1**
- Set **Deviation** to 0
- Click **fn** next to **Partial 1** and select the **EnvLib** function

  1. Set **Envelope Number** to 1 (the envelope you created), and Scaling Factor to 1.0 and then hit OK

<img width="358" height="344" alt="Screenshot 2026-06-23 at 1 13 43 PM" src="https://github.com/user-attachments/assets/9eca0e1e-2c09-40fb-9d47-23bfa07d6c50" />

<img width="1004" height="605" alt="Screenshot 2026-06-23 at 1 14 43 PM" src="https://github.com/user-attachments/assets/43360dd5-dfce-4a9d-8c05-4f72c96a09da" />

## 7.  Complete the Bottom event

- Double click on the **Bottom s1** event to bring it back
- Scrolling down, drag your **Spectrum sp1** into the white box underneath where it says
**Child Type | Class | Name**
<img width="3024" height="1832" alt="image" src="https://github.com/user-attachments/assets/3f56e72b-9755-484a-b2cb-7da4eade4ee2" />

- Click **Insert Function** in the Value field below **Frequency** and choose **RandomInt**
- 
  1. Set **Lower Bound** to 30
  2. Set **Higher Bound** to 90, hit OK
(Note: as reference, 48 = C4) - this randomly assigns pitch values to each child between MIDI 18 - 78 (F#2 - F#7)
<img width="2980" height="1832" alt="image" src="https://github.com/user-attachments/assets/faf263bd-46ca-4c29-ab0b-47aed4f10fd5" />

- (Loudness) Set **Loudness** to 128
<img width="3014" height="1250" alt="image" src="https://github.com/user-attachments/assets/3909518f-88b9-47b2-b59e-592b09bdd19d" />

- (Spatialization) Click **Insert Function** next to **Spatialization** and choose **SPA**  (determines the locations of sounds in space)
 
   1. Click the **fn (function) button** next to the Envelope
   2. Select the **EnvLib** function
   3. Set **Envelope Number** to 1 (the envelope you created), **Scaling Factor** to 1.0, then hit OK (on both **Function Generator** and **SPA** windows)
<img width="3016" height="1862" alt="image" src="https://github.com/user-attachments/assets/11921491-02e4-4745-b783-2fc6a5debb73" />

- (Reverb) Click **Insert Function** next to **Reverb** and choose **REV_Simple**  (this sets the reverberation quality of the room)
  1. Set **Room Size** to 0.5, hit OK
<img width="290" height="310" alt="image" src="https://github.com/user-attachments/assets/9ced5e42-85f5-4b77-8b14-78faeee501f6" />


- What we have now: 
<img width="3012" height="1884" alt="image" src="https://github.com/user-attachments/assets/6a24694d-c31e-4e0e-86e6-06bbde2a3bf0" />

## 8. Return to the Top event and add Bottom as child of Top

- Double click on the **Top 0** event
- Drag **Bottom s1** into the white box underneath where it says **Child Type |Class |Name**
<img width="3024" height="1892" alt="image" src="https://github.com/user-attachments/assets/39f93a3a-6477-49bd-a825-dd7d041bdb08" />


## 9.  Save Project and Synthesize

- Click the **Save** button in the toolbar
- From the **Project** menu in the toolbar, select **run**
<img width="300" height="500" alt="image" src="https://github.com/user-attachments/assets/f466c51d-024c-4350-ae15-98746d9a6933" />

- Type apple (or any sequence of letters and/or numbers) into the **Random Seed
window** and click OK
<img width="300" height="220" alt="image" src="https://github.com/user-attachments/assets/61facf22-f99a-42ae-81b5-697e131303fd" />

- Type 1(Default) into the **Number of runs**
<img width="300" height="220" alt="image" src="https://github.com/user-attachments/assets/bb509bd6-65fd-4dfd-8c57-c12557bdce0c" />

- The .aiff output is in the folder **SoundFiles**, in the same folder as the project (ex.
Tutorial/SoundFiles/Tutorial.aiff)
  1. To listen to the file, open the `.aiff` file in Audacity (or any sound editor/media
player/digital audio workstation).

<img width="2246" height="1438" alt="image" src="https://github.com/user-attachments/assets/454450e5-8812-4f55-943d-b379d15b86c6" />
  2. How can we make a more interesting piece..?

## 10. Increasing piece complexity
(**Open Project** we creat before)
- Click **Project** in the toolbar and select **Set Properties**
- Change the **Piece Duration** from 30 to **60**
<img width="290" height="350" alt="image" src="https://github.com/user-attachments/assets/0dad5609-e8a7-49bd-bd08-4b13756ab993" />

- Select Top 0 and change **Number of Children to Create** from 1 to **10**
<img width="1952" height="724" alt="image" src="https://github.com/user-attachments/assets/81193fc5-b2b4-4923-a139-4d86f27c918f" />

- Click **Insert Function** next to **Child Start Time** and choose **Random**
- Keep **Lower Bound** at **0**
- Set **Higher Bound** to **30**, hit OK
<img width="3016" height="1182" alt="image" src="https://github.com/user-attachments/assets/57a0eeaf-32b8-4097-80c4-bd8568b4b7dc" />
- Save Project and Synthesize (Step 9)

<img width="2218" height="1402" alt="image" src="https://github.com/user-attachments/assets/73d3af6d-c7f8-4f08-b0d8-a3e611119913" />


50 sounds will now be generated and now likely more interesting. Congratulations on creating
your first DISSCO piece!
