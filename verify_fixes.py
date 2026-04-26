#!/usr/bin/env python3
"""
UFO Pawn Spawn Fix - Verification Script
Audits all 10 fixes are in place in the source code
"""

import re
import sys
from pathlib import Path

class FixVerifier:
    def __init__(self, project_root):
        self.project_root = Path(project_root)
        self.fixes = {
            1: {
                "name": "Ticking enabled",
                "file": "Source/UFO/UFOPawn.cpp",
                "pattern": r"PrimaryActorTick\.bCanEverTick\s*=\s*true",
                "found": False
            },
            2: {
                "name": "Dynamic collision",
                "file": "Source/UFO/UFOPawn.cpp",
                "pattern": r"SetCollisionObjectType\(ECC_WorldDynamic\)",
                "found": False
            },
            3: {
                "name": "Safe spawn location",
                "file": "Source/UFO/UFOPawn.cpp",
                "pattern": r"SetActorLocation\(SafeLocation\)",
                "found": False
            },
            4: {
                "name": "DummyRoot fallback",
                "file": "Source/UFO/UFOPawn.cpp",
                "pattern": r"DummyRoot.*CreateDefaultSubobject.*USceneComponent",
                "found": False
            },
            5: {
                "name": "Mesh visibility",
                "file": "Source/UFO/UFOPawn.cpp",
                "pattern": r"bVisualizeComponent\s*=\s*true",
                "found": False
            },
            6: {
                "name": "Force-spawn loop",
                "file": "Source/UFO/UFOGameMode.cpp",
                "pattern": r"RestartPlayer\(PlayerController\)",
                "found": False
            },
            7: {
                "name": "Deferred timer",
                "file": "Source/UFO/UFOPlayerController.cpp",
                "pattern": r"GetTimerManager\(\)\.SetTimer",
                "found": False
            },
            8: {
                "name": "TimerManager include",
                "file": "Source/UFO/UFOPlayerController.cpp",
                "pattern": r'#include\s+"TimerManager\.h"',
                "found": False
            },
            9: {
                "name": "World include",
                "file": "Source/UFO/UFOPlayerController.cpp",
                "pattern": r'#include\s+"Engine/World\.h"',
                "found": False
            },
            10: {
                "name": "FTimerHandle member",
                "file": "Source/UFO/UFOPlayerController.h",
                "pattern": r"FTimerHandle\s+PawnCheckTimer",
                "found": False
            }
        }
    
    def verify_all(self):
        print("=" * 60)
        print("UFO PAWN SPAWN FIX - CODE VERIFICATION")
        print("=" * 60)
        print()
        
        found_count = 0
        
        for fix_num, fix_info in self.fixes.items():
            result = self._verify_fix(fix_num)
            if result:
                found_count += 1
                print(f"✅ Fix #{fix_num}: {fix_info['name']}")
            else:
                print(f"❌ Fix #{fix_num}: {fix_info['name']}")
        
        print()
        print("=" * 60)
        print(f"RESULT: {found_count}/10 fixes verified in place")
        print("=" * 60)
        print()
        
        if found_count == 10:
            print("SUCCESS: All 10 fixes are implemented!")
            print()
            print("Next steps:")
            print("1. Rebuild UFO.sln in Visual Studio (Development Editor config)")
            print("2. Reopen Unreal Editor")
            print("3. Open your level and set GameMode Override to AUFOGameMode")
            print("4. Press Play and check Output Log for 'UFOPawn::BeginPlay called'")
            print("5. Verify pawn is visible and controls work")
            return 0
        else:
            print(f"FAILURE: Only {found_count}/10 fixes found")
            print("Some fixes are missing from source code!")
            return 1
    
    def _verify_fix(self, fix_num):
        fix_info = self.fixes[fix_num]
        file_path = self.project_root / fix_info["file"]
        
        if not file_path.exists():
            print(f"   ERROR: File not found: {file_path}")
            return False
        
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
                if re.search(fix_info["pattern"], content, re.MULTILINE | re.DOTALL):
                    return True
        except Exception as e:
            print(f"   ERROR reading {file_path}: {e}")
            return False
        
        return False

if __name__ == "__main__":
    project_root = r"c:\Users\w2lf\Documents\Unreal Projects\UFO"
    
    if len(sys.argv) > 1:
        project_root = sys.argv[1]
    
    verifier = FixVerifier(project_root)
    exit_code = verifier.verify_all()
    sys.exit(exit_code)
