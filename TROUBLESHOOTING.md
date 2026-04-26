# UFO Pawn Not Spawning - Troubleshooting

## Quick Fixes to Try

### 1. **Rebuild C++ Code**
- Close the Unreal Editor
- Right-click `UFO.sln` → Generate Visual Studio project files
- Open in Visual Studio and Build → Build Solution
- Reopen the project in Unreal Editor

### 2. **Check Level Configuration**
- Open your level (L_SpaceArena)
- Go to Window → World Settings
- Ensure "GameMode Override" is set to **AUFOGameMode**
- Do NOT leave it empty or set to default

### 3. **Add a PlayerStart Actor (if not present)**
- In the level, right-click → Place Actor
- Search for "PlayerStart"
- Place it near where you want the UFO to spawn (e.g., 2000, 0, 500)
- Note: UnrealEditor will usually auto-position if missing, but being explicit helps

### 4. **Clean Intermediate Files**
- Close Unreal Editor
- Delete the `Intermediate` folder from the project
- Reopen project and let it recompile
- This forces a full rebuild and can fix mysterious spawn issues

### 5. **Check for Compile Errors**
- In Unreal Editor, go to Tools → Compile C++
- Check the Output Log (Window → Developer Tools → Output Log)
- Look for any red errors related to UFOPawn

### 6. **Verify Input Setup**
- Make sure these assets exist in Content folder:
  - `IA_LeftTrackball` (Input Action)
  - `IA_RightTrackball` (Input Action)
  - `IMC_Default` (Input Mapping Context)
- If missing, recreate them per the SETUP_GUIDE.txt

### 7. **Check Sun Collision**
- If the sun has a large collision sphere, it might be blocking the spawn
- The `SetActorLocation()` call in UFOPawn::BeginPlay should work around this
- If sun collision is the issue, try disabling sun's collision temporarily to test

## If Still Not Working

1. **Check the Output Log** for any error messages
2. **Try Possess command**: In Play mode, press the backtick key (`)  and type:
   ```
   Possess AUFOPawn
   ```
3. **Print debug info** - Add this to see if BeginPlay is being called:
   ```cpp
   UE_LOG(LogTemp, Warning, TEXT("UFO Pawn BeginPlay called"));
   ```

## Expected Behavior When Fixed

1. Close editor, rebuild
2. Open level
3. Press Play
4. UFO appears ~2000 units away from sun
5. Sun, planets, and skybox are visible
6. Trackball controls are responsive
