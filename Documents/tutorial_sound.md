# Simple DISSCO Project with LASSIE: Sound Synthesis 2.2.0

## 0. Download DISSCO

- Clone the repository: **git clone https://github.com/cmp-illinois/dissco.git**

- Enter the project directory: **cd dissco**

## 1. Build DISSCO

- Create a build directory and enter it:

mkdir build

cd build

- Generate the build files:

cmake ..

- Compile the program:

cmake --build .

(Be patient, as compiling may take a few minutes.)

## 2. Launch LASSIE (if you already downloaded and compiled DISSCO start here)

- Make sure you are inside the build directory: cd ~/dissco/build

- Open the application: **./LASSIE/LASSIE**

- Wait for the LASSIE window to appear.

- Click **New Project** in the toolbar.

- Choose a folder, name your project tutorial, and click **Save**.

(**Note:** Once you choose a folder for your project, do not move or rename it later. Otherwise, DISSCO may not be able to locate the project files.)

- A new window, **Project Properties**, will appear
  
   1. only check the boxes **Sound Synthesis** and **Ouput Particel**

   2. set **Piece Duration** to 30 and click OK
  <img width="290" height="350" alt="image" src="https://github.com/user-attachments/assets/b6759cad-2652-41aa-87e9-3993c3c92352" />

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
- Click **Create New Envelope** bottom (three bottoms in the top) to create a new envelope item
- Select the envelope item labeled 1 in the upper blank rectangle
- Right-click the lower rectangle (graph) and select **Insert Node**
- Moving around the node, you should notice the X and Y values below the graph
changing accordingly. Drag the node to the top center or input the X and Y values to
be **X value: 0.5000** and **Y value: 1.0000**
<img width="1194" height="888" alt="image" src="https://github.com/user-attachments/assets/d9077d6b-199b-4797-9e99-91c50bfe8421" />

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
<img width="1978" height="1252" alt="image" src="https://github.com/user-attachments/assets/c98f13a1-2cc0-4834-8475-eb8f0206654a" />

- Click **Insert Function** in the Value field below **Frequency** and choose **RandomInt**
- 
  1. Set **Lower Bound** to 30
  2. Set **Higher Bound** to 90, hit OK
(Note: as reference, 48 = C4) - this randomly assigns pitch values to each child between MIDI 27 - 78 (F#2 - F#7)
<img width="2012" height="1046" alt="image" src="https://github.com/user-attachments/assets/d8138913-133d-4503-9ad8-45308132ad00" />

- (Loudness) Set **Loudness** to 100
<img width="600" height="300" alt="image" src="https://github.com/user-attachments/assets/d3bc8f6a-c846-47a9-b6a8-fb5cf38499c6" />

- (Spatialization) Click **Insert Function** next to **Spatialization** and choose **SPA**  (determines the locations of sounds in space)
 
   1. Click the **fn (function) button** next to the Envelope
   2. Select the **EnvLib** function
   3. Set **Envelope Number** to 1 (the envelope you created), **Scaling Factor** to 1.0, then hit OK (on both **Function Generator** and **SPA** windows)
<img width="290" height="310" alt="image" src="https://github.com/user-attachments/assets/df101459-f9d8-4d47-8054-75acef0b081a" />

- (Reverb) Click **Insert Function** next to **Reverb** and choose **REV_Simple**  (this sets the reverberation quality of the room)
  1. Set **Room Size** to 0.5, hit OK
<img width="290" height="310" alt="image" src="https://github.com/user-attachments/assets/b341ea34-482b-4958-b37b-fa9f64ccf00c" />

- What we have now: 
<img width="2014" height="1220" alt="image" src="https://github.com/user-attachments/assets/ab985592-b908-41dc-ba4f-579d6d0e8ebc" />

## 8. Return to the Top event and add Bottom as child of Top

- Double click on the **Top 0** event
- Drag **Bottom s1** into the white box underneath where it says **Child Type |Class |Name**
<img width="1988" height="1218" alt="image" src="https://github.com/user-attachments/assets/0a317960-904a-4575-b23b-ebd2f0ef1444" />

## 9.  Save Project and Synthesize

- Click the **Save** button in the toolbar
- From the **Project** menu in the toolbar, select **run**
<img width="600" height="240" alt="image" src="https://github.com/user-attachments/assets/0ec8bc75-73d5-4b3e-8462-3ff1caacb9b9" />

- Type apple (or any sequence of letters and/or numbers) into the **Random Seed
window** and click OK
<img width="250" height="120" alt="image" src="https://github.com/user-attachments/assets/e459f3a2-18c6-471f-8ffa-cc9e3f893d78" />

- The .aiff output is in the folder **SoundFiles**, in the same folder as the project (ex.
Tutorial/SoundFiles/Tutorial.aiff)
  1. To listen to the file, it can be opened in a Audacity (or any sound editor/media
player/digital audio workstation)  (In terminal: cd SoundFiles --> **open tutorial_0**  )

<img width="2246" height="1438" alt="image" src="https://github.com/user-attachments/assets/454450e5-8812-4f55-943d-b379d15b86c6" />
  2. How can we make a more interesting piece..?

## 10. Increasing piece complexity
(**Open Project** we creat before)
- Click **Project** in the toolbar and select **Set Properties**
- Change the **Piece Duration** from 30 to **60**
<img width="290" height="350" alt="image" src="https://github.com/user-attachments/assets/96b90962-4cde-44c3-8a88-add2b57c3fc6" />

- Select Top 0 and change **Number of Children to Create** from 1 to **10**
<img width="1952" height="724" alt="image" src="https://github.com/user-attachments/assets/eb90371f-8898-4d11-be8a-3726e835f5b9" />

- Click **Insert Function** next to **Child Start Time** and choose **Random**
- Keep **Lower Bound** at **0**
- Set **Higher Bound** to **30**, hit OK
- Save Project and Synthesize (Step 9)

<img width="2218" height="1402" alt="image" src="https://github.com/user-attachments/assets/73d3af6d-c7f8-4f08-b0d8-a3e611119913" />


50 sounds will now be generated and now likely more interesting. Congratulations on creating
your first DISSCO piece!
