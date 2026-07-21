## Simple DISSCO Project with LASSIE: Score Output

## 0. Download DISSCO

- Clone the repository: **git clone https://github.com/cmp-illinois/dissco.git**

- Enter the project directory: **cd dissco**

(Linux)
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
  
   1. only check the boxes **Score Printing** and **Ouput Particel**

   2. set **Piece Duration** to 30 and click OK

   <img width="290" height="350" alt="image" src="https://github.com/user-attachments/assets/df17078f-d41f-4ad5-aefe-83f06f405b12" />

## 3. Build a Top event – this is your piece and the root node of the structure

(the default name of the newly created Top object is 0)

- Click the wedge next to **Folder Top** then double click **Top 0**
- Set **Number of Children to Create** to 1
- Enter 0 for **Child Start Time** and select **EDU** beneath it

(NOTE: Refer to the manual for more information about EDU)

- Set **Child Type** to 0
- Set **Child Duration** to 180 (Also select **EDU**) and **Max Child Duration** to 30
<img width="2000" height="1222" alt="image" src="https://github.com/user-attachments/assets/205fa7f2-4062-4793-8833-4a7a08fa677d" />

(we will complete the Top event in step 7)

## 4. Create a Bottom event - this event creates start times, durations, pitches, and dynamics of sounds

- Click **Folder Bottom**
- Click the **New Object** button in the toolbar
- Name it n1 and click OK

(**NOTE:** Bottom names must begin with a lower-case **n** !)

- Click the wedge next to the **Folder Bottom** and double-click **Bottom n1**
- Set **Number of Children** to Create to 7  (these will be your sounds)
- Click **Insert Function** next to **Child Start Time** and choose **Select**  (we are selecting the starting times (EDUs) for our 7 children (notes))
  1. Click **fn** next to **Choice Index** and choose **CURRENT_CHILD_NUM**, hit OK <img width="290" height="350" alt="image" src="https://github.com/user-attachments/assets/a6f5e375-3c2c-49a5-b12c-7a92a308cecd" />
  2. Click **Add New Node**  **7 times** and put the individual node values as 0, 3, 6, 9, 12, 15, and 18 <img width="290" height="320" alt="image" src="https://github.com/user-attachments/assets/c8657ef2-205e-4170-b6fb-63a3718f06e7" />
  3. Hit OK

- Select **EDU** beneath **Child Start Time**
- Click **Insert Function** next to **Child Type** and choose **RandInt**

(allows us to randomly choose between 2 note types which we create in the next step)

  1. keep **Lower Bound** at 0
  2. Set **Upper Bound** to 1, hit OK
<img width="290" height="320" alt="image" src="https://github.com/user-attachments/assets/bb6eb9ad-78b6-4011-bc43-040831f8d49a" />

- Set **Child Duration** to **3 EDUs**
- Ser *Max Child Duration** to 30
<img width="2322" height="1336" alt="image" src="https://github.com/user-attachments/assets/22e7c777-db52-43e0-9732-21d9cf090713" />

(we will complete the Bottom event in step 6)

## 5. Create Note events - we are creating the types of notes we randomly want to appear

- Click **Folder Note**
- Click the **New Object** button in the toolbar
- Name it no1 and click OK
- Click the **New Object** button in the toolbar
- Name it no2 and click OK
<img width="200" height="300" alt="image" src="https://github.com/user-attachments/assets/98f95c0c-10cd-4b1b-8f41-d4d39d326eba" />

- Click the wedge next to the **Folder Note** and double-click **Note no1**
- Set **Staff Number** to 0
- Check the **null box** (Note: this is a note with no notation)
<img width="2312" height="1290" alt="image" src="https://github.com/user-attachments/assets/25e36761-8aed-46c2-b42d-095d5a8cb30d" />

- Click the wedge next to the **Folder Note** and double-click **Note no2**
- Set **Staff Number** to 0
- Check the **accent box**
<img width="2306" height="1284" alt="image" src="https://github.com/user-attachments/assets/af2204c0-21ac-413e-9126-e205ff19d9fd" />

## 6. Complete the Bottom event - we are setting the bottom event’s children (note types),
pitches, and dynamic

- Double click on the **Bottom n1** event to bring it back
- Scrolling down, drag your **Note no1** and **Note no2** into the white box underneath where it says **Child Type | Class | Name**
<img width="2302" height="1252" alt="image" src="https://github.com/user-attachments/assets/1bd977a8-3af2-444e-8cc4-1f32816dd066" />

- Click **Insert Function** in the **Value** field below **Frequency** and choose **Select**
  1. Click **fn** next to **Choice Index** and choose **CURRENT_CHILD_NUM**, hit OK
  2. Click **Add New Node** **7 times** and put the individual node values as 48, 48, 55, 55, 57, 57, and 55 <img width="280" height="320" alt="image" src="https://github.com/user-attachments/assets/2bace7b4-8d61-454b-97af-7d40bd23e051" />
  3. Hit OK

- Set **Loudness** to 100
<img width="2314" height="1296" alt="image" src="https://github.com/user-attachments/assets/d40d4906-864a-4726-97f3-de90fbc4a789" />

## 7. Return to the Top event and add Bottom as child of Top

- Double click on the **Top 0** event
- Drag **Bottom n1** into the white box underneath where it says **Child Type |Class |Name**

<img width="2312" height="1292" alt="image" src="https://github.com/user-attachments/assets/18bd982a-9ff5-47ed-8c97-3c99cc247140" />

## 8. Save Project and Synthesize

- Click the **Save** button in the toolbar
- From the **Project** menu in the toolbar, select **Run**
<img width="2310" height="1038" alt="image" src="https://github.com/user-attachments/assets/d429aa88-602b-424a-9394-ad2702d61188" />

- Type 123 (or any sequence of letters and/or numbers) into the **Random Seed** window and click OK.

- The pdf sheet music output is in the folder ScoreFiles, in the same folder as the project. This tutorial only used randomness placing notation on notes. Through
experimentation, it’s possible to randomize pitches, start times, and durations to have unique variations of our theme and whole new pieces!
